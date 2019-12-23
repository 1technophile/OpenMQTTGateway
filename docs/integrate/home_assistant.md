# Integrate Home Assistant
## Auto discovery
So as to enable HASS auto discovery with MQTT you have to uncomment [ZmqttDiscovery](https://github.com/1technophile/OpenMQTTGateway/blob/0180a0dbd55ed8e0799e30ee84f68070a6f478fa/User_config.h#L99) in user_config.h
And enable discovery on your MQTT integration definition in HASS.

OMG will use the auto discovery functionality of home assistant to create sensors and gateways into your HASS instance automaticaly.

## Manual integration examples
From @123, @finity, @denniz03, @jrockstad

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
        value_template: '{{ value_json["hcsr501"] }}'
        payload_on: "true"
        payload_off: "false"
```

### Switches

```yaml
#switches
switch:
- platform: mqtt
  name: kaku_a2
  state_topic: "home/OpenMQTTGateway/commands/MQTTto433"
  command_topic: "home/OpenMQTTGateway/commands/MQTTto433"
  payload_on: "16405"
  payload_off: "16404"
  qos: "0"
  retain: true

#pushbullet
notify:
- platform: pushbullet
  name: hassio
  api_key: <api_key>
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
