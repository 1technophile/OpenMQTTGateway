# Sensors
Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

### ADC
The value is between 0 and 1024 and is transmitted via MQTT when it changes.
`home/OpenMQTTGateway/ADCtoMQTT {"value":543}`

### DHT
You will receive every TimeBetweenReadingDHT (set into config_DHT.h) the DHT measurement (30s per default).

`home/OpenMQTTGateway/DHTtoMQTT {"tempc":21,"tempf":69.8,"hum":51}`

If you want to don't resend value when it is the same you can set dht_always = false in config_DHT.h

### HTU21
You will receive the HTU21 sensor readings every TimeBetweenReadinghtu21 (set into config_HTU21.h) (30s by default).

`home/OpenMQTTGateway/CLIMAtoMQTT/htu {"tempc":25.34064,"tempf":77.61314,"hum":56.53052}`

If you don't want to resend values that haven't changed you can set htu21_always = false in config_HTU21.h

### AHTx0 (AHT10 and AHT20)
You will receive the AHT sensor readings every TimeBetweenReadingAHTx0 (set into config_AHTx0.h) (30s by default).

`home/OpenMQTTGateway/CLIMAtoMQTT/aht { "tempc": 27.48108, "tempf": 81.46594, "hum": 48.90614 }`

If you don't want to resend values that haven't changed you can set AHTx0_always = false in config_AHTx0.h

### DS18x20
You will receive the DS18x20 sensor readings every DS1820_INTERVAL_SEC (set into config_DS1820.h) (60s by default).
Each sensor will be published under the following topic using each sensors' address.

`home/OpenMQTTGateway/CLIMAtoMQTT/ds1820/0x0000000000000000 {"tempc":27.8, "tempf":82.04, "type":"DS18B20","res":"12bit\n","addr":"0x28616411907650bc"}`

The units for temperature readings are sent in Celsius by default can be changed to Fahrenheit by setting DS1820_FAHRENHEIT = true in in config_DS1820.h

If you don't want to resend values that haven't changed you can set DS1820_ALWAYS = false in config_DS1820.h

### HCSR501
A boolean value of the PIR sensors state is sent when a state change occurs. The length of time that the PIR stays in a triggered state depends on the PIR hardware and is not changed by OpenMQTTGateway.

`home/OpenMQTTGateway/HCSR501toMQTT {"presence":"false"}`

You can have another PIN mirror the value of the PIR sensor output by adding the following to config_HCSR501.h
This can be useful if you would like to connect an LED to turn on when motion is detected.

`#define HCSR501_LED_NOTIFY_GPIO 4`

This notification pin can be inverted if driving directly or through a transistor/mosfet.
`#define INVERT_LED_NOTIFY true`

### RN8209
You will receive every `TimeBetweenPublishingRN8209` (set into config_RN8209.h) the RN8209 measurements (every 10s per default), or if the difference between the previous power reading and the new reading is more than 1W and more than 10% of the previous reading.
One reading is done every 0.5s.

`home/OpenMQTTGateway/RN8209toMQTT {"volt":120.345,"current":7.9264,"power":954.6132}`
