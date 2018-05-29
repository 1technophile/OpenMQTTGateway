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
#ifndef DHT_U_H
#define DHT_U_H

#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHT_SENSOR_VERSION 1

class DHT_Unified {
public:
  DHT_Unified(uint8_t pin, uint8_t type, uint8_t count=6, int32_t tempSensorId=-1, int32_t humiditySensorId=-1);
  void begin();

  class Temperature : public Adafruit_Sensor {
  public:
    Temperature(DHT_Unified* parent, int32_t id);
    bool getEvent(sensors_event_t* event);
    void getSensor(sensor_t* sensor);

  private:
    DHT_Unified* _parent;
    int32_t _id;

  };

  class Humidity : public Adafruit_Sensor {
  public:
    Humidity(DHT_Unified* parent, int32_t id);
    bool getEvent(sensors_event_t* event);
    void getSensor(sensor_t* sensor);

  private:
    DHT_Unified* _parent;
    int32_t _id;

  };

  Temperature temperature() {
    return _temp;
  }

  Humidity humidity() {
    return _humidity;
  }

private:
  DHT _dht;
  uint8_t _type;
  Temperature _temp;
  Humidity _humidity;

  void setName(sensor_t* sensor);
  void setMinDelay(sensor_t* sensor);

};

#endif
