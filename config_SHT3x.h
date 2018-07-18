#define HUM1    Base_Topic Gateway_Name "/SHT3xtoMQTT/SHT3x1/hum"
#define TEMP1   Base_Topic Gateway_Name "/SHT3xtoMQTT/SHT3x1/temp"
#define sht3x_always true // if false when the current value for temp or hum is the same as previous one don't send it by MQTT
#define TimeBetweenReadingSHT3x 30000 // time between 2 DHT readings
/*-------------SHT3x SENSOR TYPE-------------*/
/*-------------------PIN DEFINITIONS----------------------*/
