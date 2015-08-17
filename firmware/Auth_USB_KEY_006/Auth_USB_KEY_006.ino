/* 
 * Authenticator USB Key
 * 
 * Alistair MacDonald 2015
 *
 * Docuemnts and details at https://github.com/alistairuk/Authenticator-USB-Key
 */

// Include the libries
#include <stdio.h> 
#include "sha1.h"        // https://github.com/maniacbug/Cryptosuite
#include "TOTP.h"        // https://github.com/lucadentella/ArduinoLib_TOTP
#include "swRTC.h"       // https://github.com/leomil72/swRTC
#include "DS1302.h"      // https://github.com/msparks/arduino-ds1302
#include "UsbKeyboard.h" // https://code.google.com/p/vusb-for-arduino/
#include "Base32.h"      // https://github.com/NetRat/Base32

String googleAuthKey = "AAAAAAAAAAAAAAAA"; // Add your Google Authenticator key here

// Globals
byte* hmacKey = NULL;  // Space to hold the decoded key
int hmacKeySize = 0;   // Size of the decoded key
char code[7];          // Space to store the code to type

// Create objects
TOTP totp = TOTP(hmacKey, hmacKeySize);
swRTC swRTC;

// Init the DS1302 RTC object
const int RTCResetPin = A3;
const int RTCDataPin  = A2;
const int RTCClockPin = A1;
DS1302 hwRTC(RTCResetPin, RTCDataPin, RTCClockPin);

// The main setup routine
void setup() {
  // Enable serial for debugging  
  Serial.begin(9600);
  Serial.println("Starting...");

  // (re)Create the Time-based One-time Password object
  byte googleAuthKeyBuffer[16+1];
  googleAuthKey.getBytes(googleAuthKeyBuffer, sizeof(googleAuthKeyBuffer));
  Base32 base32;
  hmacKeySize = base32.fromBase32(googleAuthKeyBuffer, 16, (byte*&)hmacKey);
  totp = TOTP(hmacKey, hmacKeySize);
  
  // Power up the RTC (yes we are using data lines to ppower it)
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);

  // Set the software RTC from the hardware one
  swRTC.stopRTC();
  Time hwTime = hwRTC.time();
  swRTC.setDate(hwTime.date, hwTime.mon, hwTime.yr);
  swRTC.setTime(hwTime.hr, hwTime.min, hwTime.sec);
  swRTC.startRTC();

  // Setup the button
  pinMode(A3, INPUT_PULLUP);
  // pinMode(A2, OUTPUT);
  // digitalWrite(A2, LOW);

  // Setup the uirtual USB
  // disable timer 0 overflow interrupt (used for millis)
  TIMSK0&=!(1<<TOIE0);
}

// A quick function to convert number chars to HID codes
int charToKeycode(char inChar) {
  switch (inChar) {
    case '0': return KEY_0;
    case '1': return KEY_1;
    case '2': return KEY_2;
    case '3': return KEY_3;
    case '4': return KEY_4;
    case '5': return KEY_5;
    case '6': return KEY_6;
    case '7': return KEY_7;
    case '8': return KEY_8;
    case '9': return KEY_9;
  }
}

// A delay routine that does not require interupts
void delayMs(unsigned int ms) {
  for (int i = 0; i < ms; i++) {
    delayMicroseconds(1000);
  }
}

// The main loop
void loop() {

  // Get the current time
  long GMT = swRTC.getTimestamp();

  // Calculate the new code
  char* newCode = totp.getCode(GMT);
  // If it is a new code then remember it and send to debug console
  if(strcmp(code, newCode) != 0) {
    strcpy(code, newCode);
    Serial.print(GMT);
    Serial.print(" - ");
    Serial.println(code);
  }  

  // Keep the USB connection alive
  UsbKeyboard.update();
  
  // Type the code if button pressed
  if (digitalRead(3) == LOW) {
    UsbKeyboard.sendKeyStroke(charToKeycode(code[0]));
    UsbKeyboard.sendKeyStroke(charToKeycode(code[1]));
    UsbKeyboard.sendKeyStroke(charToKeycode(code[2]));
    UsbKeyboard.sendKeyStroke(charToKeycode(code[3]));
    UsbKeyboard.sendKeyStroke(charToKeycode(code[4]));
    UsbKeyboard.sendKeyStroke(charToKeycode(code[5]));
    UsbKeyboard.sendKeyStroke(KEY_ENTER);
    delayMs(1000);
  }

}


