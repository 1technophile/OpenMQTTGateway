/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data to Livolo switch

    Copyright: (c) Robert Csakany
    This file is part of OpenMQTTGateway.
    
    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

     OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
     You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "User_config.h"

#ifdef ZgatewayLivolo

#define LIVOLO_PREAMBLE_DURATION 550
#define LIVOLO_ZERO_DURATION 110
#define LIVOLO_ONE_DURATION 303
#define LIVOLO_NUM_REPEATS 150

bool livoloIsHigh;

void setupLivolo()
{
  trc(F("LIVOLO_EMITTER_PIN "));
  trc(String(LIVOLO_EMITTER_PIN));
  trc(F("ZgatewayLivolo setup done "));
}


void MQTTtoLivolo(char * topicOri, JsonObject& data) 
{
  if (strcmp(topicOri,subjectMQTTtoLivolo) == 0) 
  { 
    trc(F("MQTTtoLivolo json data analysis"));
    const char * remoteIdChar = data["remoteId"];
    const char * keyCodeChar = data["keyCode"];
    uint16_t remoteId = 0;
    uint8_t keyCode = 0;    
    if (_livolo_str_to_uint16(remoteIdChar, &remoteId) && _livolo_str_to_uint8(keyCodeChar, &keyCode)) 
    {
      _livolo_sendButton(remoteId, keyCode);
      trc(F("MQTTtoLivolo sent"));
    } 
    else 
    {
      trc(F("MQTTtoLivolo Fail json"));
    }
  }
}

void _livolo_sendButton(uint16_t remoteId, uint8_t keyId)
{
  trc(F("RemoteID:"));
  trc(remoteId);
  trc(F("KeyID:"));
  trc(keyId);

  // 7 bit Key Id and 16 bit Remote Id
  uint32_t command = ((uint32_t)keyId & 0x7F) | (remoteId << 7);
  _livolo_sendCommand(command, 23);
}


void _livolo_sendCommand(uint32_t command, uint8_t numBits)
{
  for (uint8_t repeat = 0; repeat < LIVOLO_NUM_REPEATS; ++repeat)
  {
    uint32_t mask = (1 << (numBits - 1));
    _livolo_sendPreamble();
    for (uint8_t i = numBits; i > 0; --i)
    {
      if ((command & mask) > 0)
      {
        _livolo_sendOne();
      }
      else
      {
        _livolo_sendZero();
      }
      mask >>= 1;
    }
  }
  _livolo_tx(false);
}

void _livolo_sendOne()
{
  delayMicroseconds(LIVOLO_ONE_DURATION);
  livoloIsHigh = !livoloIsHigh;
  _livolo_tx(livoloIsHigh);
}

void _livolo_sendZero()
{
  delayMicroseconds(LIVOLO_ZERO_DURATION);
  _livolo_tx(!livoloIsHigh);
  delayMicroseconds(LIVOLO_ZERO_DURATION);
  _livolo_tx(livoloIsHigh);
}

void _livolo_sendPreamble()
{
  _livolo_tx(true);
  delayMicroseconds(LIVOLO_PREAMBLE_DURATION);
  _livolo_tx(false);
  livoloIsHigh = false;
}

void _livolo_tx(bool value)
{
  digitalWrite(LIVOLO_EMITTER_PIN, value ? HIGH : LOW);
}


bool _livolo_str_to_uint8(const char *str, uint8_t *res) 
{
    char *end;
    errno = 0;
    long val = strtol(str, &end, 10);
    if (errno || end == str || *end != '\0' || val < 0 || val >= 0x100) 
    {
        return false;
    }
    *res = (uint8_t)val;
    return true;
}

bool _livolo_str_to_uint16(const char *str, uint16_t *res) 
{
    char *end;
    errno = 0;
    long val = strtol(str, &end, 10);
    if (errno || end == str || *end != '\0' || val < 0 || val >= 0x10000) 
    {
        return false;
    }
    *res = (uint16_t)val;
    return true;
}

#endif
