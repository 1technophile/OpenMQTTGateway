# Upload with Arduino IDE

* First download the last version of the Arduino IDE from the arduino [website](https://www.arduino.cc/en/Main/Software)
* Add ESP32 boards by following this [tutorial](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md)
* Add ESP8266 boards by following this [tutorial](https://github.com/esp8266/Arduino#installing-with-boards-manager)
* Download OpenMQTTGateway code from the [release page](https://github.com/1technophile/OpenMQTTGateway/releases)
* Download the libraries package corresponding to your board and module wished into the same page (example esp32-m5stick-c-ble-libraries.zip)
* Unzip the libraries into your arduino libraries folder (example D:/Users/XXXX/Documents/Arduino/libraries)
* Open the file main.ino from OpenMQTTGateway/main folder
* Change the settings and the desired gateways into user_config.h (uncomment the modules you want)

*Example for the use of RF gateway*
```C++
#define ZgatewayRF     "RF"       //ESP8266, Arduino, ESP32
//#define ZgatewayIR     "IR"       //ESP8266, Arduino,         Sonoff RF Bridge
//#define ZgatewayLORA   "LORA"       //ESP8266, Arduino, ESP32
//#define ZgatewayPilight "Pilight" //ESP8266, Arduino, ESP32
//#define ZgatewayBT     "BT"       //ESP8266, ESP32
```

* Change the pins or parameters corresponding to the modules choosen, for RF you can change the pins into config_RF.h
* Choose the board on the Arduino IDE
* Select the port corresponding to the board
* Open the serial monitor and set 115200 bauds
* Upload ➡️
* You should see the logs into the serial monitor
