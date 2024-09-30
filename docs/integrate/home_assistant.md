# Integrate Home Assistant

Home Assistant provide the [MQTT integration](https://www.home-assistant.io/integrations/mqtt/) and through this integration it is possible to exploit and manage the messages published by OpenMQTTGateway.

Once this integration on home assistant is configured with the same MQTT broker, it is possible to create devices manually or through the autodiscovery function.


## Auto discovery

From Home Assistant site 

> The discovery of MQTT devices will enable one to use MQTT devices with only minimal configuration effort on the side of Home Assistant. The configuration is done on the device itself and the topic used by the device.

On OpenMQTTGateway the Home Assistant discovery is enabled by default on all binaries and platformio configurations. Here are a few tips for activating discovery on Home Assistant, but for detailed configuration please refer to the Home Assistant website. 

Enable discovery on your MQTT integration in HASS (activated per default).

![Home Assistant Auto Discovery](../img/OpenMQTTGateway-Configuration-Home-Assistant-Discovery-Integration.png)

The gateway will need an MQTT username and password, you have to create a new user (recommended) into Home Assistant->Configuration->Users (available in admin mode) or use an existing username/pwd combination (not recommended). This user doesn't need to be an administrator.

![Home Assistant Auto Discovery](../img/OpenMQTTGateway-Configuration-Home-Assistant.png)

::: warning Note
The max size of the username and password is 64 characters.
:::

OMG will use the auto discovery functionality of home assistant to create gateway and sensors into your HASS instance automatically.

![Home Assistant Auto Discovery](../img/OpenMQTTGateway_auto_discovery_Gateway_Home_Assistant.gif)

![Home Assistant Auto Discovery](../img/OpenMQTTGateway_auto_discovery_BLE_Sensor_Home_Assistant.gif)

![Home Assistant Auto Discovery](../img/OpenMQTTGateway_Home_Assistant_MQTT_discovery.png)

::: tip INFO
The Bluetooth and the RTL_433 gateway will automatically create devices and entities, the RF gateway will create DeviceTrigger.
The OpenMQTTGateway will also be available as a device to monitor its parameters and control it. The sensors (DHT for example) and actuators (relays) are attached to the gateway.

On first and subsequent startups, auto discovery will start. If you want to prevent this from happening, be sure to manually turn off auto discovery, either by using the UI in Home Assistant, or by publishing to the home/<gatewayname>/commands/MQTTtoSYS/config topic.
30 minutes after its activation the auto discovery will be automatically deactivated, you can reactivate it from the gateway controls. 
Some devices may require a button push or motion/contact event to trigger a message and generate the auto discovery.
:::

## RTL_433 auto discovery specificity

Even if the RTL_433 gateway will create automatically the devices and entities, you may lose the link to them when you change the batteries. This is proper to the RF devices. In this case new device and entities will be created. You may bypass this by creating entities through manual configuration that filter following the device model and other parameters and don't take into account the id.
Example:
```yaml
mqtt:
  sensor:
    - state_topic: "+/+/RTL_433toMQTT/WS2032/+"
```
instead of
```yaml
mqtt:
  sensor:
    - state_topic: "+/+/RTL_433toMQTT/WS2032/47998"
```
Note also that the sensor may leverage channels, types or subtypes, they can be used in the filtering 
Example:
In the example below 9 is the `subtype` and 1 is the `channel`
```yaml
mqtt:
  sensor:
    - state_topic: "+/+/RTL_433toMQTT/Prologue-TH/9/1/+"
```
instead of
```yaml
mqtt:
  sensor:
    - state_topic: "+/+/RTL_433toMQTT/Prologue-TH/9/1/215"
```

Alternatively the rssi signal could be used also.

## MQTT Device Trigger and RF

With OpenMQTTGateway [configured to receive RF signals](../setitup/rf.html) the messages are transmitted as indicated by [RCSwitch based gateway](../use/rf.html#rcswitch-based-gateway), so it is possible to receive a pulse every time the sensor discover a signal. 

With autodiscovery enabled, HomeAssistant will discover a [MQTT Device Trigger](https://www.home-assistant.io/integrations/device_trigger.mqtt/) identified by the value field given in the mqtt argument. 

## Manual integration examples
From @123, @finity, @denniz03, @jrockstad, @anarchking, @dkluivingh

### Door sensor
```yaml
mqtt:
  binary_sensor:
    - name: "test"
      state_topic: "home/OpenMQTTGateway/433toMQTT"
      value_template: >-
        {% if value_json.value == '7821834' %}
          {{'ON'}}
        {% elif value_json.value == '7821838' %}
          {{'OFF'}}
        {% else %}
          {{states('binary_sensor.test') | upper}}
        {% endif %}
      qos: 0
      device_class: opening
```

```yaml
mqtt:
  binary_sensor:
    - name: doorbell
      state_topic: 'home/OpenMQTTGateway/SRFBtoMQTT'
      #value_template: "{{ value_json.raw }}"
      value_template: >- 
        {% if value_json.value == '14163857' %}
          {{'ON'}}
        {% else %}
          {{states('binary_sensor.doorbell') | upper}}
        {% endif %}
      off_delay: 30
      device_class: 'sound'

    - name: light_back_sensor
      state_topic: 'home/OpenMQTTGateway/SRFBtoMQTT'
      #value_template: '{{ value_jason.value }}'
      value_template: >- 
        {% if value_json.value == '1213858' %}
          {{'ON'}}
        {% else %}
          {{states('binary_sensor.light_back_sensor') | upper}}
        {% endif %}
      off_delay: 5

    - name: rf_outlet_sensor
      state_topic: 'home/OpenMQTTGateway/SRFBtoMQTT'
      value_template: >- 
        {% if value_json.value == '16766303' %}
          {{'ON'}}
        {% else %} 
          {{states('binary_sensor.rf_outlet_sensor') | upper}}
        {% endif %}
```

### Motion sensor
```yaml
mqtt:
  binary_sensor:
    - name: "Bewegung_Schlafzimmer"
      #device_class: motion
      state_topic: "home/OpenMQTTGateway1/HCSR501toMQTT"
      value_template: '{{ value_json["presence"] }}'
      payload_on: "true"
      payload_off: "false"
```

### Switches

```yaml
#switches
mqtt:
  switch:
    - name: Plug1
      state_topic: "home/OpenMQTTGateway/SRFBtoMQTT"
      command_topic: "home/OpenMQTTGateway/commands/MQTTtoSRFB"
      value_template: "{{ value_json.value }}"
      payload_on: '{"value":4546575}'
      payload_off: '{"value":4546572}'
      state_on: 4546575
      state_off: 4546572
      qos: "0"
      retain: true
```
### RF gateway mode - Pilight, RF, kaku
insert an include statement in HA configuration.yaml
```bash
grep homed ./configuration.yaml 
mqtt: !include homed-mqtt.yaml
```
In the example, the included file is homed-mqtt.yaml. It provides an mqtt select entity with the ability to show and also change via dropdown - the desired mode of the RF receiver gateway. In the homed-mqtt.yaml snippet, the device section (as it is optional) is ommited,
```yaml
select:
  - name: 'RF: Mode receive'
    unique_id: espdevcho-rf-mode
    #platform: mqtt
    availability_topic: home/espdevcho/LWT # espdevcho is a particular name of the gateway, instead of the default OpenMQTTGateway
    payload_available: online
    payload_not_available: offline
    options: 
      - "Pilight"
      - "RF classic"
      - "RF2 kaku"
    state_topic: home/espdevcho/RFtoMQTT # espdevcho is a particular name of the gateway, instead of the default OpenMQTTGateway
    value_template: >
      {% if value_json.active == 1 %} Pilight
      {% elif value_json.active == 2 %} RF classic
      {% elif value_json.active == 4 %} RF2 kaku
      {% endif %}
    #unit_of_measurement: s
    command_topic: home/espdevcho/commands/MQTTtoRF/config # espdevcho is a particular name of the gateway, instead of the default OpenMQTTGateway
    command_template: >
      {% set value_map = {
             "Pilight": 1,
             "RF classic": 2,
             "RF2 kaku": 4,
         }
      %}
      {"active":{{ value_map[value] }}}
    device:
      configuration_url: http://192.168.1.11/ # device section is optional. It is almost ommited in this example. Values here will update the corresponding device, if it already exist

```

### Mijia Thermometer BLE

```yaml
mqtt:
  sensor:
    - name: "mijia_thermometer_temperature"
      state_topic: 'home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA' # MQTT topic, check MQTT messages; replace AA... with id (BLE MAC) of your device
      unit_of_measurement: '°C'
      value_template: '{{ value_json.tem | is_defined }}'
      expire_after: 21600 # 6 hours
      force_update: true
    - name: "mijia_thermometer_humidity"
      state_topic: 'home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA'
      unit_of_measurement: '%'
      value_template: '{{ value_json.hum | is_defined }}'
      expire_after: 21600 # 6 hours
      force_update: true
    - name: "mijia_thermometer_battery"
      state_topic: 'home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA'
      unit_of_measurement: '%'
      value_template: '{{ value_json.batt | is_defined }}'
      expire_after: 21600 # 6 hours
      force_update: true
```


### Xiaomi Mi Scale V2 BLE (XMTZC05HM)

```yaml
mqtt:
  sensor:
    - name: "Weight"
      state_topic: "home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA" # replace your MQTT topic here
      value_template: '{{ value_json["weight"] }}'
      unit_of_measurement: "kg"
      icon: mdi:weight-kilogram
    
    - name: "Impedance"
      state_topic: "home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA" # replace your MQTT topic here also
      value_template: '{{ value_json["impedance"] }}'
      unit_of_measurement: "Ohm"
      icon: mdi:omega
    
template:
  sensor:
    - name: body_mass_index:
      friendly_name: 'Body Mass Index'
      value_template: >-
        {% set HEIGHT = (1.76)|float %} # replace your height in meters
        {% set WEIGHT = states('sensor.xmtzc05hm_weight')|float %}
        {{- (WEIGHT/(HEIGHT*HEIGHT))|float|round(1) -}}
      icon_template: >
        {{ 'mdi:human' }}
```

### MQTT Room Presence

For the publication into the presence topic the following needs to be activated - [here is the command](../use/ble.html#setting-if-the-gateway-publish-into-home-assistant-home-presence-topic-default-false-available-with-ha-discovery)

```yaml
sensor:
  - platform: mqtt_room
    device_id: XX:XX:XX:XX:XX:XX   #Mac Address of device wanting to track
    name: you_are_in    # home assistant will show a sensor named (you are in) with its value being the name you gave the gateway
    state_topic: "home/presence"
    #timeout:
    #away_timeout:
```

### Temperature sensor

```yaml
mqtt:
  sensor:
    - name: outdoor temp
      state_topic: "home/OpenMQTTGateway/433toMQTT"
      unit_of_measurement: '°C'
      value_template: >
        {% if value_json is defined and value_json.sensor == 125 %}
          {{ value_json.tempc }}
        {% else %}
          {{ states('sensor.outdoor_temp') }}
        {% endif %}
```
