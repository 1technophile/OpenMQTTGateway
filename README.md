433toMQTTto433  - ESP8266 program for home automation 

   Act as a wifi gateway between your nodes and a MQTT broker
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal

  Contributor:
  - 1technophile

  Based on:
  - MQTT library (https://github.com/knolleary)
  - RCSwitch (https://github.com/sui77/rc-switch)
  - ESP8266Wifi
  
  Project home: https://github.com/1technophile/433toMQTTto433_ESP8266

  Tutorial and schema are there https://1technophile.blogspot.com/2016/09/433tomqttto433-bidirectional-esp8266.html

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Some usefull commands to test gateway with mosquitto
Subscribe to the subject for data receiption from RF signal
mosquitto_sub -t home/433toMQTT

Send data by MQTT to convert it on RF signal
mosquitto_pub -t home/MQTTto433/ -m 1315153
