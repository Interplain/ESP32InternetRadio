
//  Customized by Richard Phillips / AUG 2023
//  - Added rotary knob to change channels
//  - Added 128x64 Oled display
//  - Volume Up/Down Buttons
//  - Stereo version
//  - Added WiFiManger to config WiFi credentials over an access point
//  - Added header file Stations.h for easier stations management
//  - Wifi Manager change
//  - Updated stations list
//  - Refactored code with ESP32_VS1053_Stream Library
//  - Adding Metadata such as song information

#include <VS1053.h>               //https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>  //https://github.com/CelliesProjects/ESP32_VS1053_Stream
#include <ESP32Encoder.h>         //https://github.com/madhephaestus/ESP32Encoder
#include <HTTPClient.h>
#include <EEPROM.h>
#include <U8g2lib.h>              //https://github.com/olikraus/u8g2
#include <Wire.h>
#include <esp_wifi.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include "Stations.h"             // Radio station settings

#define VS1053_CS    32
#define VS1053_DCS   33
#define VS1053_DREQ  35



#define EEPROM_SIZE  128
#define VOLUME       70         // volume level 0-100
#define VOLUME_MIN 0
#define VOLUME_MAX 100
int currentVolume = VOLUME;  // Initialize the volume to the default value

// Define Volume Button Pins
const int buttonAPin = 25;
const int buttonBPin = 34;

// Store the previous button states
int prevButtonAState = HIGH;
int prevButtonBState = HIGH;

// Define initial button states
int buttonAState = HIGH;
int buttonBState = HIGH;

// Define button debounce delay in milliseconds
const unsigned long DEBOUNCE_DELAY = 500;

// Define button press flags
bool buttonAPressed = false;
bool buttonBPressed = false;
unsigned long buttonADebounceTime = 0;
unsigned long buttonBDebounceTime = 0;

ESP32_VS1053_Stream stream;

//***** RotaryEncoder
const int rotaryDT  = 16;
const int rotaryCLK = 17;
const int rotarySW  = 26; // Switch is used to trigger a reset when rotary is pushed
ESP32Encoder encoder;

//***** OLED
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

long interval = 100;
int SECONDS_TO_AUTOSAVE = 100;
long seconds = 0;
long previousMillis = 0;

int radioStation = 0;
int previousRadioStation = -1;

String songinfo;
String eof;

int shift = 128;
int textwidth;
unsigned int currentState = 0;  // variable for statemachine in loop


void setup () {

  //Pin Setup for Volume Buttons
  pinMode(buttonAPin, INPUT_PULLUP);// Volume Down
  pinMode(buttonBPin, INPUT_PULLUP);// Volume Up

  Serial.begin(115200); while (!Serial); delay(200);
  Serial.println("Starting up Web Radio");
  delay(500);

  SPI.begin();

  u8g2.begin();                           // initialize Screen
  u8g2.clearBuffer();                     // clear the internal memory
  u8g2.setFont(u8g2_font_lubB08_te);    // https://github.com/olikraus/u8g2/wiki/fntlistall for fonts
  u8g2.drawStr(2, 22, "Starting Engine");
  u8g2.sendBuffer();
  delay(2000);                            // give enough time to powerup peripherals

  Serial.println("Connecting to WiFi");
  u8g2.clearBuffer();
  u8g2.drawStr(2, 22, "Connecting WiFi");
  u8g2.sendBuffer();
  go_online();

  Serial.println("Setting up EEPROM");
  u8g2.clearBuffer();
  u8g2.drawStr(2, 22, "EEPROM Setup");
  u8g2.sendBuffer();
  EEPROM.begin(EEPROM_SIZE);

  delay(2000); // Some delay to help some EEPROMs

  Serial.println("Setting Input Pins");
  pinMode(rotaryDT, INPUT);
  pinMode(rotaryCLK, INPUT);

  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachHalfQuad(rotaryDT, rotaryCLK);
  pinMode(rotarySW, INPUT_PULLUP);

  Serial.println("Initialising Decoder Module");
  u8g2.clearBuffer();
  u8g2.drawStr(2, 22, "Starting Stream..");
  u8g2.sendBuffer();
  stream.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ);
  // Set the initial volume
  stream.setVolume(VOLUME);

  Serial.println("Reading last Station from EEPROM");
  radioStation = readStationFromEEPROM();
  if (radioStation > totalStations - 1) radioStation = 0; // if EEPROM has not been initialized
  Serial.println("last Radio Station:" + String(radioStation));
  encoder.setCount(radioStation);  // setting rotary encoder to station number

  Serial.println("Setup Done");
  u8g2.setFont(u8g2_font_lubB08_te);

  Serial.print("codec: ");
  Serial.println(stream.currentCodec());

  Serial.print("bitrate: ");
  Serial.print(stream.bitrate());
  Serial.println("kbps");

}

bool showVolumeBar = false; // Flag to indicate whether to show the volume bar screen
unsigned long volumeBarStartTime = 0; // Store the start time when the volume bar screen is shown

void loop() {
stream.loop();
unsigned long currentMillis = millis();

// Calculate volume bar length based on current volume
int volumeBarLength = map(currentVolume, VOLUME_MIN, VOLUME_MAX, 40, 87);
    
 // Read the button states
int buttonAState = digitalRead(buttonAPin);
int buttonBState = digitalRead(buttonBPin);

  // Check for button A press with debounce
  if (buttonAState == LOW && prevButtonAState == HIGH && currentMillis - buttonADebounceTime > DEBOUNCE_DELAY) {
    // Button A was pressed, decrease volume
    currentVolume = max(VOLUME_MIN, currentVolume - 3); // Decrease volume by 3 (adjust as needed)
    stream.setVolume(currentVolume); // Apply the new volume
    buttonADebounceTime = currentMillis;
    showVolumeBar = true; // Set the flag to true when button A is pressed
    volumeBarStartTime = currentMillis; // Reset the volume bar start time
    currentState = 3; // Transition to the volume bar state
  }

  // Check for button B press with debounce
  if (buttonBState == LOW && prevButtonBState == HIGH && currentMillis - buttonBDebounceTime > DEBOUNCE_DELAY) {
    // Button B was pressed, increase volume
    currentVolume = min(VOLUME_MAX, currentVolume + 3); // Increase volume by 3 (adjust as needed)
    stream.setVolume(currentVolume); // Apply the new volume
    buttonBDebounceTime = currentMillis;
    showVolumeBar = true; // Set the flag to true when button B is pressed
    volumeBarStartTime = currentMillis; // Reset the volume bar start time
    currentState = 3; // Transition to the volume bar state
  }

  // Update previous button states
  prevButtonAState = buttonAState;
  prevButtonBState = buttonBState;

     
  if (currentMillis - previousMillis > interval)
  {
    if (encoder.getCount() >= 0 && encoder.getCount() < totalStations) {
      radioStation = encoder.getCount();
    }
    else {
      if (encoder.getCount() < 0) {
        radioStation = totalStations - 1;
      }
      if (encoder.getCount() > totalStations - 1) {
        radioStation = 0;
      }
      encoder.setCount(radioStation);
    }

    if (radioStation != previousRadioStation)   // we are changing the station
    {
      station_connect(radioStation);
      Serial.print("Number of station: ");
      Serial.println(radioStation);
      previousRadioStation = radioStation;
      seconds = 0;
      currentState = 0;                         // starting statemachine from top
      wait(0, 1);                               // clearing wait function with parameter 1
    } else
    
    
    {
      seconds++;
      if (seconds == SECONDS_TO_AUTOSAVE) {
        int readStation = readStationFromEEPROM();
        if (readStation != radioStation)
        {
          writeStationToEEPROM();
        }
      }
    }
    previousMillis = currentMillis;

  }
  if ((!digitalRead(rotarySW))) {
    ESP.restart();   // button pressed, doing a restart
  }

  // Handle the state machine logic
switch (currentState) {
  case 0:
    // Display station name and signal strength
    u8g2.firstPage();
    do {
      int signalStrength = WiFi.RSSI(); // Get WiFi signal strength in dBm

      u8g2.setFont(u8g2_font_profont12_tf); // Choose a font
      u8g2.setCursor(7, 50); // Adjust the position as needed
      u8g2.print("Signal: ");
      u8g2.print(signalStrength);
      u8g2.print(" dBm");      
      u8g2.setCursor(7, 22);
      u8g2.print(String(radioname[radioStation]));
      
      u8g2.drawLine(0, 0, 127, 0);
      u8g2.drawLine(0, 63, 127, 63);
      u8g2.drawStr(0, 55, " "); // Add an empty line space
      u8g2.print(eof);

      } while (u8g2.nextPage());
      currentState++;
   // Check if the volume buttons are pressed to show the volume bar
    if (buttonAPressed || buttonBPressed) {
      currentState = 3; // Transition to the volume bar state
      showVolumeBar = true;
      volumeBarStartTime = currentMillis;
      volumeBarLength = map(currentVolume, VOLUME_MIN, VOLUME_MAX, 0, 127);
      
      // Reset the button press flags
      buttonAPressed = false;
      buttonBPressed = false;
    }
    break;

   case 1:
  // Wait for a few seconds before transitioning to the next state
  if (wait(8000, false)) {
    currentState++;
    // Check if the volume buttons are pressed to show the volume bar
    if (buttonAPressed || buttonBPressed) {
      currentState = 3; // Transition to the volume bar state
      showVolumeBar = true;
      volumeBarStartTime = currentMillis;
      volumeBarLength = map(currentVolume, VOLUME_MIN, VOLUME_MAX, 0, 127);
    }
    shift = 128;
    Serial.println("Switching to track Name.");
  }
  break;

case 2:
  // Display track information animation
  u8g2.firstPage();
  do {
    u8g2.setCursor(shift--, 22);
    u8g2.print(songinfo);
    u8g2.drawLine(0, 0, 127, 0);
    u8g2.drawLine(0, 31, 127, 31);
  } while (u8g2.nextPage());
  // Check if the animation has moved off the left edge before transitioning
  if (shift <= -(textwidth + 5)) { // Adjust the value as needed to ensure the entire title is off screen
    // Animation has moved off the left edge, transition to Case 3
    currentState = 0;
    Serial.println("Switching to Station Name.");
    showVolumeBar = true;
    volumeBarStartTime = currentMillis;
    volumeBarLength = map(currentVolume, VOLUME_MIN, VOLUME_MAX, 0, 127);    
    // Reset the button press flags
    buttonAPressed = false;
    buttonBPressed = false;
  } else if (buttonAPressed || buttonBPressed) {
    currentState = 3; // Transition to the volume bar state
    showVolumeBar = true;
    volumeBarStartTime = currentMillis;
    volumeBarLength = map(currentVolume, VOLUME_MIN, VOLUME_MAX, 0, 127);    
    // Reset the button press flags
    buttonAPressed = false;
    buttonBPressed = false;
  }
  break;


case 3:
  // Display volume bar screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont12_tf); // Choose a font for the volume label
  u8g2.setCursor(47,55); // Position for the volume label
  u8g2.print("Volume"); // Print the volume label

  // Check if button A is pressed to decrease volume
  if (buttonAPressed && currentVolume > VOLUME_MIN) {
    currentVolume = max(VOLUME_MIN, currentVolume - 50); // Decrease volume by 10 (adjust as needed)
    stream.setVolume(currentVolume); // Apply the new volume
    buttonAPressed = false; // Reset the button press flag
  }

  // Check if button B is pressed to increase volume
  if (buttonBPressed && currentVolume < VOLUME_MAX) {
    currentVolume = min(VOLUME_MAX, currentVolume + 50); // Increase volume by 10 (adjust as needed)
    stream.setVolume(currentVolume); // Apply the new volume
    buttonBPressed = false; // Reset the button press flag
  }

  // Calculate the width of the volume bar based on current volume
  int volumeBarWidth = map(currentVolume, VOLUME_MIN, VOLUME_MAX, 0, 80);

  // Calculate the starting position for the volume bar to center it at the bottom
  int volumeBarX = (128 - volumeBarWidth) / 2;
  int volumeBarY = 60;

  u8g2.drawBox(volumeBarX, volumeBarY, volumeBarWidth, 3);
  u8g2.sendBuffer();

  Serial.println("Entering Volume Bar State");

  // Wait for a few seconds before transitioning back
  if (currentMillis - volumeBarStartTime >= 3000) {
    currentState = 0; // Transition back to station info state
    Serial.println("Switching to Station Name.");
    showVolumeBar = false; // Reset the flag
  }
  break;
}
}

void station_connect (int station_no) {
  u8g2.firstPage();
  do {
    u8g2.setCursor(7, 22);
    u8g2.print ("Buffering ...");
    u8g2.drawLine(0, 0, 127, 0);
    u8g2.drawLine(0, 31, 127, 31);
  } while ( u8g2.nextPage() );
  stream.stopSong();
  Serial.println(String(radioname[radioStation]));
  stream.connecttohost(host[radioStation]);
}

void configModeCallback (WiFiManager * myWiFiManager) {
  u8g2.clearBuffer();
  u8g2.drawStr(12, 8,  "WiFi not configured");
  u8g2.drawStr(12, 20, "Connect to WLan:");
  u8g2.drawStr(12, 31, "--> WebRadio-AP");
  u8g2.sendBuffer();
}

void go_online() {

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("WebRadio-AP")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();   // no luck - going to restart the ESP
  }
  Serial.println("Connected... To Network:)");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. Local IP: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("No WiFi connection!");
  }
}

int readStationFromEEPROM() {
  int aa;
  aa = EEPROM.read(0);
  return aa;
}

void writeStationToEEPROM() {
  Serial.println("Saving Radio Station to EEPROM: " + String(radioStation));
  EEPROM.write(0, radioStation);
  EEPROM.commit();
}

bool wait(unsigned long duration, bool clearagain) {
  static unsigned long startTime;
  static bool isStarted = false;

  if (clearagain) isStarted = false;

  if (isStarted == false)                  // if wait period not started yet
  {
    startTime = millis();                  // set the start time of the wait period
    isStarted = true;                      // indicate that it's started
    return false;                          // indicate to caller that wait period is in progress
  }
  if (millis() - startTime >= duration)    // check if wait period has lapsed
  {
    isStarted = false;    // lapsed, indicate no longer started so next time we call the function it will initialise the start time again
    return true;          // indicate to caller that wait period has lapsed
  }
  return false;           // indicate to caller that wait period is in progress
}

void audio_showstreamtitle(const char* info) {
  Serial.printf("streamtitle: %s\n", info);
  songinfo = info;
  textwidth = u8g2.getUTF8Width(info);    // calculate the pixel width of the text
}

void audio_eof_stream(const char* info) {
    Serial.printf("eof: %s\n", info);
  eof = info;
    textwidth = u8g2.getUTF8Width(info); 
}
