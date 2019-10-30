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