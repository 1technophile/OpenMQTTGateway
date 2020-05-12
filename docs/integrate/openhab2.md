# Integrate OPENHAB2

## Auto discovery through home assistant convention

OpenMQTTGateway support autodiscovery of things for OpenHAB 2.4, so as to do that it rely on home assistant auto discovery convention.
So as to use the autodiscovery function you need to have:
* The Jinga transformation addon installed
* The Json transformation addon installed

You need to set `OpenHABAutoDiscovery` to true into `config_mqttDiscovery.h`
`#define OpenHABDiscovery true`

The things will appear in the inbox of the paperUI, add them and links the channels. You should see them into the control panel for further usage.
![](../img/OpenMQTTgateway_OpenHAB_Control.png)

## MQTT 2 manual setup >=Openhab2.4
You should have a mqtt broker installed (either mosquitto or the OpenHAB2 embedded one)

In paper UI
* In bindings add the MQTT Things binding
* Inbox > MQTT Things binding > Add manualy >MQTT Broker : configure your borker
* Inbox > MQTT Things binding > Add manualy >Generic MQTT Thing : select your previously configured broker as a bridge and enter your thing definition (example RF plug, weather station, PIR sensor)
* Configuration > Things > "Your thing" : click on + and add one channel for each thing parameter (example for a weather station, you will have one channel for the temperature, one channel for the humidity etc..)
* For a state channel define the MQTT topic like this:

## For a mi flora or mi jia temperature :

`home/+/BTtoMQTT/C47C9999D1B8`

* Click on show more

* And add in "Incoming values transformation"

`JSONPATH:$.tem`

* Click on Save

* Repeat for each channels and each things
* Configuration > Things > "Your thing" : click on a channel and add 1 or several items per channel defining what you want to display in the sitemap
or
* Define your items in an item file like this by refering to your mqtt things channels:
For a mi flora and mi jia

```java
// MI JIA
Number humidity		"Humidité air[%.1f %%]" <water>		    {channel="mqtt:topic:dc2222e6:humidite-mijia"}
Number temperature 	"Température[%.1f °C]"  <temperature>	    {channel="mqtt:topic:dc2222e6:temperature-mijia"}
Number battery   	"Batterie capteur[%.1f %]"<volt>	    {channel="mqtt:topic:dc2222e6:batterie-mijia"}

// MI FLORA
Number humidity_P	"Hygrométrie plante[%.1f %%]" <water>	    {channel="mqtt:topic:1fb33334:humidite-miflora"}
Number temperature_P 	"Température plante[%.1f °C]" <temperature> {channel="mqtt:topic:1fb33334:temperature-miflora"}
Number fertility_P	"Fertilité plante[%.1f uS/cm]" 	            {channel="mqtt:topic:1fb33334:fertilite-miflora"}
Number lux_P		"Luminiosité plante[%.1f lux]"              {channel="mqtt:topic:1fb33334:lux-miflora"}
```
## For a switch channel, add a channel by choosing the type "On/Off switch"

 define the MQTT state topic like this:
`home/+/433toMQTT`

* Click on show more

* add in "Incoming values transformation"
`JSONPATH:$.value`

* define the command topic like this:
`home/+/commands/MQTTto433`

* add the value corresponding to ON state and to OFF state
1312081
1312084

* Click on Save

* Repeat for each channels and each things

* Configuration > Things > "Your thing" : click on a channel and add 1 or several switch per channel 
or
* Define your items in an item file like this by refering to your mqtt thing channel:

`Switch OMGSwitch "Prise 1" <poweroutlet> {channel="mqtt:topic:08998877:Power1"}`


# Presence detection (from @rickitaly)

in thing file:

```java
Thing mqtt:topic:omgentrance (mqtt:broker:localBroker) {
Channels:
    Type string : blepresence "People Presence"  [ stateTopic="home/home_presence/OpenMQTTGatewayEntrance"]
}
```

in item file

```java
String OMG_BLE_Entrance "BLE Entrance Detector" { channel="mqtt:topic:omgentrance:blepresence" }
Switch   Presence_Keys_Rick      "Rick's Keys"       <keyring>      (People, gKeys)        {expire="240s,OFF"}
```

In rule file:

```java
rule "BLE Presence Detector"
when
Item OMG_BLE_Entrance received update
then
val String msg = (OMG_BLE_Entrance.state as StringType).toString
val String id  = transform("JSONPATH", "$.id", msg).toString
if(id == "xx:xx:xx:xx:xx:xx")
    Presence_Keys_Rick.postUpdate(ON)
end
```


