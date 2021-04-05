# Integrate Home Assistant
## Auto discovery
Home Assistant discovery is enabled by default on all binaries and platformio configurations except for UNO. With Arduino IDE please read the [advanced configuration section](../upload/advanced-configuration#auto-discovery) of the documentation.

First enable discovery on your MQTT integration in HASS.

![](../img/OpenMQTTGateway-Configuration-Home-Assistant-Discovery-Integration.png)

The gateway will need an MQTT username and password, you have to create a new user(recommended) into Home Assistant->Configuration->Users (available in admin mode) or use an existing username/pwd combination (not recommended). This user doesn't need to be an administrator.

![](../img/OpenMQTTGateway-Configuration-Home-Assistant.png)

::: warning Note
The max size of the username is 30 and 60 for the password.
:::

OMG will use the auto discovery functionality of home assistant to create gateway and sensors into your HASS instance automaticaly.

![](../img/OpenMQTTGateway_auto_discovery_Gateway_Home_Assistant.gif)

![](../img/OpenMQTTGateway_auto_discovery_BLE_Sensor_Home_Assistant.gif)

![](../img/OpenMQTTGateway_Home_Assistant_MQTT_discovery.png)

## Manual integration examples
From @123, @finity, @denniz03, @jrockstad, @anarchking

### Door sensor
```yaml
binary_sensor:
  - platform: mqtt
    name: "test"
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
binary_sensor:
  - platform: mqtt
    name: doorbell
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
  - binary_sensor:
    platform: mqtt
    name: light_back_sensor
    state_topic: 'home/OpenMQTTGateway/SRFBtoMQTT'
    #value_template: '{{ value_jason.value }}'
    value_template: >- 
      {% if value_json.value == '1213858' %}
        {{'ON'}}
      {% else %}
        {{states('binary_sensor.light_back_sensor') | upper}}
      {% endif %}
    off_delay: 5
  - binary_sensor:
    platform: mqtt
    name: rf_outlet_sensor
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
binary_sensor:
      - platform: mqtt
        name: "Bewegung_Schlafzimmer"
        #device_class: motion
        state_topic: "home/OpenMQTTGateway1/HCSR501toMQTT"
        value_template: '{{ value_json["presence"] }}'
        payload_on: "true"
        payload_off: "false"
```

### Switches

```yaml
#switches
switch:
  - platform: mqtt
    name: Plug1
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

### Mijia Thermometer BLE

```yaml
Sensor:
- platform: mqtt
  state_topic: 'home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA' # MQTT topic, check MQTT messages; replace AA... with id (BLE MAC) of your device
  name: "mijia_thermometer_temperature"
  unit_of_measurement: '°C'
  value_template: '{{ value_json.tem | is_defined }}'
  expire_after: 21600 # 6 hours
- platform: mqtt
  state_topic: 'home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA'
  name: "mijia_thermometer_humidity"
  unit_of_measurement: '%'
  value_template: '{{ value_json.hum | is_defined }}'
  expire_after: 21600 # 6 hours
- platform: mqtt
  state_topic: 'home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA'
  name: "mijia_thermometer_battery"
  unit_of_measurement: '%'
  value_template: '{{ value_json.batt | is_defined }}'
  expire_after: 21600 # 6 hours
  ```


### Xiaomi Mi Scale V2 BLE (XMTZC05HM)

```yaml
sensor:
  - platform: mqtt
    name: "Weight"
    state_topic: "home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA" # replace your mqtt topic here
    value_template: '{{ value_json["weight"] }}'
    unit_of_measurement: "kg"
    icon: mdi:weight-kilogram
    
  - platform: mqtt
    name: "Impedance"
    state_topic: "home/OpenMQTTGateway/BTtoMQTT/AAAAAAAAAAAA" # replace your mqtt topic here also
    value_template: '{{ value_json["impedance"] }}'
    unit_of_measurement: "Ohm"
    icon: mdi:omega
    
  - platform: template
    sensors:
      body_mass_index:
        friendly_name: 'Body Mass Index'
        value_template: >-
          {% set HEIGHT = (1.76)|float %} # replace your height in meters
          {% set WEIGHT = states('sensor.xmtzc05hm_weight')|float %}
          {{- (WEIGHT/(HEIGHT*HEIGHT))|float|round(1) -}}
        icon_template: >
          {{ 'mdi:human' }}
```

### MQTT Room Presence

The publication into presence topic needs to be activated [here is the command](../use/ble.md)

```yaml
sensor:
  - platform: mqtt_room
    device_id: XX:XX:XX:XX:XX:XX   #Mac Address of device wanting to track
    name: you_are_in    # home assistant will show a sensor named (you are in) with its value being the name you gave the gateway
    state_topic: "home/home_presence"
    #timeout:
    #away_timeout:
```
