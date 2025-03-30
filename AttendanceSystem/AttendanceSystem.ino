#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <set>

// ================= CONFIGURATION ================= //
#define RST_PIN D3               // Reset pin for the MFRC522
#define SS_PIN D4                // Slave select pin for the MFRC522
#define BUZZER D8                // Buzzer pin
#define BLOCK_NUM 6              // Block number from which to read data
#define LATE_HOUR 8              // Hour threshold for lateness
#define LATE_MINUTE 40           // Minute threshold for lateness

const char* SSID = "VivoY56";
const char* PASS = "12dhw7v5";
const String SHEET_URL = "https://script.google.com/macros/s/AKfycbz2X95AAPDHLOX57t84yOv_iWUb6c4yYPXB0O6taXm61FS_09_GdtM_-DwXHPHp0TttyA/exec";

std::set<String> scannedCards;
std::set<String> exitScannedCards;

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);

// ================= FUNCTION DECLARATIONS ================= //
void connectWiFi();
void initializeTime();
void checkConnection();
String getUID();
bool isDuplicate(String uid);
String readCard();
bool validateData(String data);
String getStatus(String uid);
void updateAttendance(String uid, String data);
void sendToSheet(String roll, String name, String status);
void successBuzzer();
void failureBuzzer();
void processCard();


// ================= MAIN FUNCTIONS ================= //
void setup() {
  Serial.begin(115200);
  // Wait a moment for Serial Monitor to initialize
  delay(500);
  Serial.println("🚀 Starting up...");

  pinMode(BUZZER, OUTPUT);
  
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);  // Set SPI speed
  mfrc522.PCD_Init();
  Serial.println("🔌 RFID reader initialized!");

  // Initialize key for authentication (default key: 0xFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  connectWiFi();
  initializeTime();
}

void loop() {
  timeClient.update();
  
  // Check for a new RFID card and process it if detected
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    processCard();
  }
  
  checkConnection();
}

// ================= CARD PROCESSING ================= //
void processCard() {
  String uid = getUID();
  
  // Check for duplicate scans within 30 seconds
  if (isDuplicate(uid)) return;

  String data = readCard();
  
  if (validateData(data)) {
    updateAttendance(uid, data);
  } else {
    Serial.println("❗ Invalid card data! Data read: " + data);
    failureBuzzer();
  }

  // Halt PICC and stop encryption
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// ================= NETWORK FUNCTIONS ================= //
void connectWiFi() {
  Serial.print("📡 Connecting to WiFi: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("⏳");
  }
  Serial.println("\n✅ WiFi Connected!");
}

void checkConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🔄 Reconnecting WiFi...");
    WiFi.reconnect();
  }
}

void initializeTime() {
  Serial.println("⏰ Initializing NTP time client...");
  timeClient.begin();
  while (!timeClient.update()) {
    Serial.println("⌛ Updating time...");
    timeClient.forceUpdate();
    delay(500);
  }
  Serial.println("✅ Time synchronized: " + timeClient.getFormattedTime());
}

// ================= RFID FUNCTIONS ================= //
String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println("🆔 Card UID: " + uid);
  return uid;
}

String readCard() {
  byte buffer[18];
  
  for (byte attempt = 0; attempt < 3; attempt++) {
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
      MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
      BLOCK_NUM, 
      &key, 
      &(mfrc522.uid)
    );
    
    if (status != MFRC522::STATUS_OK) {
      Serial.println("🔒 Authentication failed, attempt: " + String(attempt + 1));
      continue;
    }

    byte size = sizeof(buffer);
    if (mfrc522.MIFARE_Read(BLOCK_NUM, buffer, &size) == MFRC522::STATUS_OK) {
      buffer[16] = '\0';  // Ensure string termination
      String result = String((char*)buffer).substring(0, 15);
      result.trim();
      Serial.println("📖 Card data read: " + result);
      return result;
    } else {
      Serial.println("❗ Read failed on attempt: " + String(attempt + 1));
    }
  }
  return "";
}

// ================= ATTENDANCE FUNCTIONS ================= //
bool isDuplicate(String uid) {
  static String lastUID;
  static unsigned long lastScan;
  
  if (uid == lastUID && millis() - lastScan < 30000) {
    Serial.println("🔁 Duplicate scan detected within 30 seconds.");
    failureBuzzer();
    return true;
  }
  
  lastUID = uid;
  lastScan = millis();
  return false;
}

bool validateData(String data) {
  // Valid data must be at least 5 characters and contain a '-' separator
  return data.length() >= 5 && data.indexOf('-') != -1;
}

String getStatus(String uid) {
  int hour = timeClient.getHours();
  int minute = timeClient.getMinutes();

  // If the card is scanned for exit or already scanned, mark as "Left"
  if (exitScannedCards.count(uid)) return "Left";
  if (scannedCards.count(uid)) {
    exitScannedCards.insert(uid);
    return "Left";
  }
  
  scannedCards.insert(uid);
  return (hour > LATE_HOUR || (hour == LATE_HOUR && minute > LATE_MINUTE)) 
    ? "Late" 
    : "Present";
}

void updateAttendance(String uid, String data) {
  int sep = data.indexOf('-');
  String roll = data.substring(0, sep);
  String name = data.substring(sep + 1);
  String status = getStatus(uid);
  
  Serial.printf("👤 %s (%s): %s\n", name.c_str(), roll.c_str(), status.c_str());
  sendToSheet(roll, name, status);
}

// ================= SHEETS INTEGRATION ================= //
void sendToSheet(String roll, String name, String status) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();  // Disable certificate validation (not recommended for production)
  
  HTTPClient https;
  String url = SHEET_URL + "?roll=" + roll + "&name=" + name + "&status=" + status;

  Serial.println("🌐 Sending data to Google Sheet...");
  Serial.println("🔗 URL: " + url);

  if (https.begin(client, url)) {
    // Extend the timeout period to 10 seconds (10000 ms)
    https.setTimeout(10000);
    
    int httpCode = https.GET();
    
    // Diagnostic output for HTTP response with emojis
    Serial.print("📡 HTTP Status Code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      if (httpCode == 200 || httpCode == 302 || httpCode == -11) {
        Serial.println("✅ Request successful! Activating success buzzer.");
        successBuzzer();
      } else {
        Serial.println("❌ Request returned error code. Activating failure buzzer.");
        failureBuzzer();
      }
    } else {
      Serial.println("❌ HTTP request error. Activating failure buzzer.");
      failureBuzzer();
    }
    
    https.end();
  } else {
    Serial.println("⚠️ Unable to connect to URL. Activating failure buzzer.");
    failureBuzzer();
  }
}


// ================= BUZZER FEEDBACK ================= //
void successBuzzer() {
  // Single short beep for success
  Serial.println("🔔 Success buzzer activated!");
  digitalWrite(BUZZER, HIGH);
  delay(200);  // 200ms beep
  digitalWrite(BUZZER, LOW);
}

void failureBuzzer() {
  // Triple short beeps for failure
  Serial.println("🔕 Failure buzzer activated!");
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(100);  // 100ms beep
    digitalWrite(BUZZER, LOW);
    // Short gap between beeps
    delay(i == 0 ? 50 : 100);
  }
}
