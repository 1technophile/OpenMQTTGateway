#ifndef JSON_MSG_BUFFER
#if defined(ESP32)
#  define JSON_MSG_BUFFER 1024 // adjusted to minimum size covering largest home assistant discovery messages
#  if MQTT_SECURE_DEFAULT
#    define JSON_MSG_BUFFER_MAX 2048 // Json message buffer size increased to handle certificate changes through MQTT, used for the queue and the coming MQTT messages
#  else
#    define JSON_MSG_BUFFER_MAX 1024 // Minimum size for the cover MQTT discovery message
#  endif
#elif defined(ESP8266)
#  define JSON_MSG_BUFFER     512 // Json message max buffer size, don't put 768 or higher it is causing unexpected behaviour on ESP8266, certificates handling with ESP8266 is not tested
#  define JSON_MSG_BUFFER_MAX 832 // Minimum size for MQTT discovery message
#endif
#endif