# Attendance System using ESP8266 & RFID

This project implements an attendance tracking system using an ESP8266 microcontroller paired with an RC-522 RFID scanner. The system reads RFID cards, verifies the data stored on them, and logs attendance status (Present, Late, or Left) to a Google Sheet via a Google Apps Script web app.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Required](#hardware-required)
- [Software Required](#software-required)
- [Installation](#installation)
- [Code Overview](#code-overview)
  - [WriteData.ino](#writedataino)
  - [AttendanceSystem.ino](#attendancesystemino)
  - [Google Apps Script](#google-apps-script)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## Overview

This project uses the ESP8266 along with an RC-522 RFID scanner to automate attendance management. When an RFID card is scanned, the system reads the card data, checks for duplicate scans, determines the attendance status based on a predefined time threshold, and then sends the data (roll number, name, and status) to a Google Sheet for record keeping.

## Features

- **RFID Card Writing:** Program RFID cards with student data (Roll-Name format) using the WriteData.ino sketch.
- **Attendance Tracking:** Read RFID cards with the AttendanceSystem.ino sketch to mark attendance.
- **Time-Based Status:** Automatically assign "Present," "Late," or "Left" status based on the time of scan.
- **Google Sheets Integration:** Log attendance data in real time using a Google Apps Script.
- **Audible Feedback:** Use buzzer signals to indicate successful or failed operations.

## Hardware Required

- **ESP8266** (NodeMCU or similar)
- **RC-522 RFID Reader/Writer**
- **RFID Cards**
- **Buzzer**
- Jumper wires & Breadboard

## Software Required

- Arduino IDE (with ESP8266 board support)
- [MFRC522 Library](https://github.com/miguelbalboa/rfid)
- [ESP8266WiFi Library](https://github.com/esp8266/Arduino)
- [NTPClient Library](https://github.com/arduino-libraries/NTPClient)

## Installation

1. **Hardware Setup:**
   - Connect the RC-522 module to the ESP8266 (e.g., RST_PIN to D3, SS_PIN to D4).
   - Connect the buzzer to the designated pin (D8 in this example).

2. **Software Setup:**
   - Install the required libraries via the Arduino IDE Library Manager.
   - Clone or download the repository to your local machine.

3. **Google Sheet Setup:**
   - Create a Google Sheet and note its ID.
   - Copy the provided Google Apps Script code, paste it into the [Google Apps Script Editor](https://script.google.com), and deploy it as a web app.
   - Update the `SHEET_URL` constant in the AttendanceSystem.ino file with your web app URL.

## Code Overview

### WriteData.ino

This sketch is used to write data (student roll number and name in "Roll-Name" format) to an RFID card.

- **Key Functions:**
  - **`setup()`**: Initializes serial communication, SPI, and the RFID reader.
  - **`loop()`**: Waits for a new card, then handles the writing process.
  - **`handleCard()`**: Logs card details, writes data to the card, and verifies the write operation.

### AttendanceSystem.ino

This sketch handles attendance tracking by reading the RFID card data, validating it, determining attendance status, and sending the data to Google Sheets.

- **Key Functions:**
  - **`setup()`**: Initializes serial, SPI, WiFi, RFID reader, and the NTP time client.
  - **`loop()`**: Continuously checks for new cards and processes them.
  - **`processCard()`**: Manages duplicate scan prevention, reads card data, and validates it.
  - **`getStatus()`**: Determines whether a scan is "Present," "Late," or "Left" based on the current time.
  - **`sendToSheet()`**: Sends attendance data to the Google Sheet via an HTTP GET request.
  - **Buzzer Feedback:** Functions `successBuzzer()` and `failureBuzzer()` provide audible feedback.

### Google Apps Script

This script runs on Google’s servers to log the attendance data into a Google Sheet.

- **Key Features:**
  - **Parameter Validation:** Checks that roll, name, and status parameters are provided.
  - **Sanitization:** Cleans the inputs for security.
  - **Data Logging:** Appends a new row with the timestamp, roll number, name, and status.
  - **Response:** Returns a JSON response with a status code and message.

## Usage

1. **Writing to RFID Cards:**
   - Upload the `WriteData.ino` sketch to your ESP8266.
   - Place an RFID card near the RC-522 reader to write the student data.

2. **Attendance Tracking:**
   - Upload the `AttendanceSystem.ino` sketch.
   - Ensure your ESP8266 is connected to WiFi.
   - When an RFID card is scanned, the system reads the card data, validates it, and sends it to the Google Sheet.
   - The buzzer will provide audible feedback based on the success or failure of the operation.

3. **Viewing Attendance:**
   - Open your Google Sheet to view the updated attendance records.

## Troubleshooting

- **RFID Card Not Reading/Writing:**
  - Verify the wiring connections between the ESP8266 and the RC-522.
  - Ensure the correct pins are defined (D3, D4, etc.).
  - Check the serial monitor for error messages (authentication failures, read/write errors).

- **WiFi Connection Issues:**
  - Confirm that your SSID and password in the code are correct.
  - Check the ESP8266’s connectivity status via the serial monitor.

- **Google Sheet Not Updating:**
  - Ensure the Google Apps Script is deployed correctly as a web app.
  - Confirm that the `SHEET_URL` in the code matches your web app URL.
  - Check the script logs in Google Apps Script Editor for any errors.

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.
