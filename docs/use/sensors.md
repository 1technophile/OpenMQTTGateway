# Sensors
Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

### ADC
The value is between 0 and 1024 and is tranmitted via Mqtt when it changes.
`home/OpenMQTTGateway/DHTtoMQTT {"value":543}`

### DHT
You will receive every TimeBetweenReadingDHT (set into config_DHT.h) the DHT measurement (30s per default).

`home/OpenMQTTGateway/DHTtoMQTT {"temp":21,"hum":51}`

If you want to don't resend value when it is the same you can set dht_always = false in config_DHT.h

### HTU21
You will receive the HTU21 sensor readings every TimeBetweenReadinghtu21 (set into config_HTU21.h) (30s by default).

`home/OpenMQTTGateway/CLIMAtoMQTT/htu {"tempc":25.34064,"tempf":77.61314,"hum":56.53052}`

If you don't want to resend values that haven't changed you can set htu21_always = false in config_HTU21.h

### DS18x20
You will receive the DS18x20 sensor readings every DS1820_INTERVAL_SEC (set into config_DS1820.h) (60s by default).
Each sensor will be published under the following topic using each sensors' address.

`home/OpenMQTTGateway/CLIMAtoMQTT/ds1820/0x0000000000000000 {"temp":27.8,"unit":"C","type":"DS18B20","res":"12bit\n","addr":"0x28616411907650bc"}`

The units for temperature readings are sent in Celcius by default can be changed to ferenheight by setting DS1820_FAHRENHEIT = true in in config_DS1820.h

If you don't want to resend values that haven't changed you can set DS1820_ALWAYS = false in config_DS1820.h