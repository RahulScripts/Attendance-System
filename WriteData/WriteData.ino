#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN D3
#define SS_PIN D4

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

const byte BLOCK_NUM = 6;  // Sector 1 Block 2
const String STUDENT_DATA = "21-RahulHalli"; // Format: Roll-Name (max 15 chars)

void setup() {
  Serial.begin(115200);
  while(!Serial); // [DIAGNOSTIC] Remove in production
  
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16); // Stable communication
  mfrc522.PCD_Init();
  
  // Initialize default key
  for(byte i=0; i<6; i++) key.keyByte[i] = 0xFF;
  
  Serial.println("RFID Writer Ready");
}

void loop() {
  if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    handleCard();
    delay(2000);
  }
}

void handleCard() {
  // [DIAGNOSTIC] Show card info
  logCardDetails();

  if(writeCard()) {
    verifyWrite();
  }
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

bool writeCard() {
  byte data[16] = {0};
  STUDENT_DATA.getBytes(data, 16);
  
  // [DIAGNOSTIC] Show raw data
  Serial.print("Writing: ");
  for(byte i=0; i<16; i++) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
    BLOCK_NUM, 
    &key, 
    &(mfrc522.uid)
  );

  if(status != MFRC522::STATUS_OK) {
    Serial.print("Auth failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  status = mfrc522.MIFARE_Write(BLOCK_NUM, data, 16);
  if(status != MFRC522::STATUS_OK) {
    Serial.print("Write failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  return true;
}

void verifyWrite() {
  byte readBuffer[18];
  byte size = sizeof(readBuffer);
  
  if(mfrc522.MIFARE_Read(BLOCK_NUM, readBuffer, &size) != MFRC522::STATUS_OK) {
    Serial.println("Verification read failed");
    return;
  }

  // [DIAGNOSTIC] Show verification data
  Serial.print("Read back: ");
  for(byte i=0; i<16; i++) {
    Serial.print(readBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  String result = String((char*)readBuffer).substring(0,15);
  Serial.println(result == STUDENT_DATA ? "Verify OK" : "Data mismatch");
}

void logCardDetails() {
  Serial.println("\nCard Detected");
  Serial.print("UID: ");
  for(byte i=0; i<mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print("\nType: ");
  Serial.println(mfrc522.PICC_GetTypeName(mfrc522.PICC_GetType(mfrc522.uid.sak)));
}