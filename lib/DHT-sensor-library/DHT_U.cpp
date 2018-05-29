// DHT Temperature & Humidity Unified Sensor Library
// Copyright (c) 2014 Adafruit Industries
// Author: Tony DiCola

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include "DHT_U.h"

DHT_Unified::DHT_Unified(uint8_t pin, uint8_t type, uint8_t count, int32_t tempSensorId, int32_t humiditySensorId):
  _dht(pin, type, count),
  _type(type),
  _temp(this, tempSensorId),
  _humidity(this, humiditySensorId)
{}

void DHT_Unified::begin() {
  _dht.begin();
}

void DHT_Unified::setName(sensor_t* sensor) {
  switch(_type) {
    case DHT11:
      strncpy(sensor->name, "DHT11", sizeof(sensor->name) - 1);
      break;
    case DHT21:
      strncpy(sensor->name, "DHT21", sizeof(sensor->name) - 1);
      break;
    case DHT22:
      strncpy(sensor->name, "DHT22", sizeof(sensor->name) - 1);
      break;
    default:
      // TODO: Perhaps this should be an error?  However main DHT library doesn't enforce
      // restrictions on the sensor type value.  Pick a generic name for now.
      strncpy(sensor->name, "DHT?", sizeof(sensor->name) - 1);
      break;
  }
  sensor->name[sizeof(sensor->name)- 1] = 0;
}

void DHT_Unified::setMinDelay(sensor_t* sensor) {
  switch(_type) {
    case DHT11:
      sensor->min_delay = 1000000L;  // 1 second (in microseconds)
      break;
    case DHT21:
      sensor->min_delay = 2000000L;  // 2 seconds (in microseconds)
      break;
    case DHT22:
      sensor->min_delay = 2000000L;  // 2 seconds (in microseconds)
      break;
    default:
      // Default to slowest sample rate in case of unknown type.
      sensor->min_delay = 2000000L;  // 2 seconds (in microseconds)
      break;
  }
}

DHT_Unified::Temperature::Temperature(DHT_Unified* parent, int32_t id):
  _parent(parent),
  _id(id)
{}

bool DHT_Unified::Temperature::getEvent(sensors_event_t* event) {
  // Clear event definition.
  memset(event, 0, sizeof(sensors_event_t));
  // Populate sensor reading values.
  event->version     = sizeof(sensors_event_t);
  event->sensor_id   = _id;
  event->type        = SENSOR_TYPE_AMBIENT_TEMPERATURE;
  event->timestamp   = millis();
  event->temperature = _parent->_dht.readTemperature();
  
  return true;
}

void DHT_Unified::Temperature::getSensor(sensor_t* sensor) {
  // Clear sensor definition.
  memset(sensor, 0, sizeof(sensor_t));
  // Set sensor name.
  _parent->setName(sensor);
  // Set version and ID
  sensor->version         = DHT_SENSOR_VERSION;
  sensor->sensor_id       = _id;
  // Set type and characteristics.
  sensor->type            = SENSOR_TYPE_AMBIENT_TEMPERATURE;
  _parent->setMinDelay(sensor);
  switch (_parent->_type) {
    case DHT11:
      sensor->max_value   = 50.0F;
      sensor->min_value   = 0.0F;
      sensor->resolution  = 2.0F;
      break;
    case DHT21:
      sensor->max_value   = 80.0F;
      sensor->min_value   = -40.0F;
      sensor->resolution  = 0.1F;
      break;
    case DHT22:
      sensor->max_value   = 125.0F;
      sensor->min_value   = -40.0F;
      sensor->resolution  = 0.1F;
      break;
    default:
      // Unknown type, default to 0.
      sensor->max_value   = 0.0F;
      sensor->min_value   = 0.0F;
      sensor->resolution  = 0.0F;
      break;
  }
}

DHT_Unified::Humidity::Humidity(DHT_Unified* parent, int32_t id):
  _parent(parent),
  _id(id)
{}

bool DHT_Unified::Humidity::getEvent(sensors_event_t* event) {
  // Clear event definition.
  memset(event, 0, sizeof(sensors_event_t));
  // Populate sensor reading values.
  event->version           = sizeof(sensors_event_t);
  event->sensor_id         = _id;
  event->type              = SENSOR_TYPE_RELATIVE_HUMIDITY;
  event->timestamp         = millis();
  event->relative_humidity = _parent->_dht.readHumidity();
  
  return true;
}

void DHT_Unified::Humidity::getSensor(sensor_t* sensor) {
  // Clear sensor definition.
  memset(sensor, 0, sizeof(sensor_t));
  // Set sensor name.
  _parent->setName(sensor);
  // Set version and ID
  sensor->version         = DHT_SENSOR_VERSION;
  sensor->sensor_id       = _id;
  // Set type and characteristics.
  sensor->type            = SENSOR_TYPE_RELATIVE_HUMIDITY;
  _parent->setMinDelay(sensor);
  switch (_parent->_type) {
    case DHT11:
      sensor->max_value   = 80.0F;
      sensor->min_value   = 20.0F;
      sensor->resolution  = 5.0F;
      break;
    case DHT21:
      sensor->max_value   = 100.0F;
      sensor->min_value   = 0.0F;
      sensor->resolution  = 0.1F;
      break;
    case DHT22:
      sensor->max_value   = 100.0F;
      sensor->min_value   = 0.0F;
      sensor->resolution  = 0.1F;
      break;
    default:
      // Unknown type, default to 0.
      sensor->max_value   = 0.0F;
      sensor->min_value   = 0.0F;
      sensor->resolution  = 0.0F;
      break;
  }
}
