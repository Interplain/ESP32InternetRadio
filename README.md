# ESP32 Web Radio Player

ESP32 Build of Internet Radio 
ESP32 WROOM Dev 1 Board 
![image](https://github.com/Interplain/ESP32InternetRadio/assets/32413344/b20f9596-6ab7-4e3c-a97e-0c6342313325)

Parts list
1.30" IIC 128x64 Oled Display
Encoder
VS1053 DAC Audio Board
2 x Button Switches
2 x 10k Resistors
ESP32 WROOM Dev MCU

---

This ESP32-based web radio player allows you to listen to your favorite online radio stations with ease. The project is inspired by Nick Koumaris' work, customized and enhanced by Marc Stähli and myself Interplain. I was to further add buttons for volume control with a bar that increases and decreases with presses. With the addition of WiFi signal strength value that is refreshed each time the Station name is shown. 

## Features

- **Rotary Knob Control**: Change radio stations effortlessly with a rotary knob.
- **Volume Buttons**: Push buttons to raise and lower audio level.
- **128x64 OLED Display**: Provides station and song information at a glance.
- **Stereo Audio**: Enjoy stereo sound with the ESP32.
- **WiFiManager**: Configure WiFi credentials easily via an access point.
- **WiFi Signal Strength**: Value changes each time the Station page is shown.
- **Station Management**: Use the `Stations.h` file to manage your radio stations.

## Getting Started

Follow these steps to set up the ESP32 web radio player:

1. **Clone the Repository**: Clone this repository to your local machine using `git clone`. Arduino IDE 2.2

2. **Hardware Connections**: Connect the ESP32 to your audio output and rotary knob as per the provided pin configurations in the code.

3. **Configure WiFi**: The device can be configured for WiFi using the WiFiManager. Just connect to the "WebRadio_AP" WiFi access point, and it will guide you through the setup.

4. **Upload the Code**: Use the Arduino IDE or PlatformIO to upload the code to your ESP32.

5. **Enjoy Radio**: Power on the ESP32, and use the rotary knob to browse and select your favorite radio stations.

6. **Connecting to Local Network**: Using a browser from a Mobile Phone goto the WiFi settings and find the broadcast ~ WebRadio-APP and then in the Browser type 192.168.4.1 and then enter the Username and password for the Router that is being logged into.

## Station Management

To add or edit radio stations, modify the `Stations.h` file. Each station entry includes a name and the station's URL. Make sure to update the `totalStations` variable accordingly.

```cpp
const char* radioname[] = {
  "Station 1",
  "Station 2",
  // Add your stations here
};

const char* host[] = {
  "http://station1.com/stream",
  "http://station2.com/stream",
  // Add your station URLs here
};

const int totalStations = sizeof(radioname) / sizeof(radioname[0]);
```

## License

This project is open-source and available under the [MIT License](LICENSE).

## Acknowledgments

- Nick Koumaris for the original idea and code.
- Marc Stähli for customization and enhancements.
- The open-source community for their valuable contributions.

---
