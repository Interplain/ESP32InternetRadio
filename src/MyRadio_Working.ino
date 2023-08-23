//23/08/2023 ESP32 Internet Radio V1
//

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

#define VOLUME       95         // volume level 0-100
#define EEPROM_SIZE  128



ESP32_VS1053_Stream stream;

//***** RotaryEncoder
const int rotaryDT  = 16;
const int rotaryCLK = 17;
const int rotarySW  = 26;          // Switch is used to trigger a reset when rotary is pushed
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

void loop() {
  stream.loop();
  unsigned long currentMillis = millis();
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

  switch (currentState) {                          // non-blocking statemachine to allow button interrupt
    case 0:
      u8g2.firstPage();
      do {
         int signalStrength = WiFi.RSSI(); // Get WiFi signal strength in dBm

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont12_tf); // Choose a font
    u8g2.setCursor(7, 50); // Adjust the position as needed
    u8g2.print("Signal: ");
    u8g2.print(signalStrength);
    u8g2.print(" dBm");
  } while (u8g2.nextPage());
        u8g2.setCursor(7, 22);
        u8g2.print(String(radioname[radioStation]));
        u8g2.print(eof);
        u8g2.drawLine(0, 0, 127, 0);         
        u8g2.drawLine(0, 63, 127, 63);

      } while ( u8g2.nextPage() );
      currentState++;
      break;
      
    case 1:
      if (wait(4000, 0) == true)
      {
        currentState++;        // if lapsed, go to next step (state)
        shift = 128;           // start at the right end of the screen (for case 2)
      }
      break;
    case 2:
      u8g2.firstPage();
      do {
        u8g2.setCursor(shift--, 22);
        u8g2.print(songinfo);
        u8g2.drawLine(0, 0, 127, 0);
        u8g2.drawLine(0, 31, 127, 31);       
      } while ( u8g2.nextPage() );
      if (textwidth < 0) textwidth = 0;
      if (shift <= textwidth * -1) currentState = 0;
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
  u8g2.drawStr(12, 31, "--> WebRadio_AP");
  u8g2.sendBuffer();
}

void go_online() {

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("WebRadio_AP")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();   // no luck - going to restart the ESP
  }
  Serial.println("connected... To Network:)");

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