/*
  OpenMQTTGateway - Home Assistant JSON Templates
  
  Contains predefined Jinja2 templates for Home Assistant value_template fields.
  These templates extract specific values from OpenMQTTGateway JSON messages.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

//=============================================================================
// HOME ASSISTANT VALUE TEMPLATES
// Used in entity value_template fields to extract data from JSON messages
//=============================================================================

// Basic sensor values
#define jsonBatt     "{{ value_json.batt | is_defined }}"
#define jsonLux      "{{ value_json.lux | is_defined }}"
#define jsonPres     "{{ value_json.pres | is_defined }}"
#define jsonFer      "{{ value_json.fer | is_defined }}"
#define jsonFor      "{{ value_json.for | is_defined }}"
#define jsonMoi      "{{ value_json.moi | is_defined }}"
#define jsonHum      "{{ value_json.hum | is_defined }}"
#define jsonStep     "{{ value_json.steps | is_defined }}"
#define jsonWeight   "{{ value_json.weight | is_defined }}"
#define jsonPresence "{{ value_json.presence | is_defined }}"

// Altitude measurements
#define jsonAltim "{{ value_json.altim | is_defined }}"
#define jsonAltif "{{ value_json.altift | is_defined }}"

// Temperature readings (multiple sensors)
#define jsonTempc  "{{ value_json.tempc | is_defined }}"
#define jsonTempc2 "{{ value_json.tempc2 | is_defined }}"
#define jsonTempc3 "{{ value_json.tempc3 | is_defined }}"
#define jsonTempc4 "{{ value_json.tempc4 | is_defined }}"
#define jsonTempf  "{{ value_json.tempf | is_defined }}"

// Generic message fields
#define jsonMsg     "{{ value_json.message | is_defined }}"
#define jsonVal     "{{ value_json.value | is_defined }}"
#define jsonId      "{{ value_json.id | is_defined }}"
#define jsonAddress "{{ value_json.address | is_defined }}"
#define jsonTime    "{{ value_json.time | is_defined }}"
#define jsonCount   "{{ value_json.count | is_defined }}"

// Electrical measurements
#define jsonVolt    "{{ value_json.volt | is_defined }}"
#define jsonCurrent "{{ value_json.current | is_defined }}"
#define jsonPower   "{{ value_json.power | is_defined }}"
#define jsonEnergy  "{{ value_json.energy | is_defined }}"

// GPIO and ADC
#define jsonGpio "{{ value_json.gpio | is_defined }}"
#define jsonAdc  "{{ value_json.adc | is_defined }}"

// Light measurements
#define jsonFtcd "{{ value_json.ftcd | is_defined }}"
#define jsonWm2  "{{ value_json.wattsm2 | is_defined }}"

// Pressure (with conversion)
#define jsonPa "{{ float(value_json.pa) * 0.01 | is_defined }}"

// Status indicators
#define jsonOpen  "{{ value_json.open | is_defined }}"
#define jsonAlarm "{{ value_json.alarm | is_defined }}"
#define jsonRSSI  "{{ value_json.rssi | is_defined }}"

// Power usage indicators (with logic)
#define jsonInuse       "{{ value_json.power | is_defined | float > 0 }}"
#define jsonInuseRN8209 "{% if value_json.power > 0.02 -%} on {% else %} off {%- endif %}"

// Conditional voltage template
#define jsonVoltBM2 "{% if value_json.uuid is not defined and value_json.volt is defined -%} {{value_json.volt}} {%- endif %}"
