/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface
   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
  This actor enables to:
 - receive MQTT data from a topic and send Somfy RTS remote control signals corresponding to the received MQTT data
  
    Copyright (C) 2020 Leon Kiefer

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

#ifdef ZactuatorSomfy

#  include <EEPROM.h>
#  include <EEPROMRollingCodeStorage.h>
#  include <SomfyRemote.h>

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

void setupSomfy() {
  pinMode(RF_EMITTER_GPIO, OUTPUT);
  digitalWrite(RF_EMITTER_GPIO, LOW);

#  if defined(ESP32)
  if (!EEPROM.begin(max(4, SOMFY_REMOTE_NUM * 2))) {
    Log.error(F("failed to initialise EEPROM" CR));
  }
#  elif defined(ESP8266)
  EEPROM.begin(max(4, SOMFY_REMOTE_NUM * 2));
#  endif

  Log.trace(F("ZactuatorSomfy setup done " CR));
}

#  if jsonReceiving
void MQTTtoSomfy(char* topicOri, JsonObject& jsonData) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoSomfy)) {
    Log.trace(F("MQTTtoSomfy json data analysis" CR));
    float txFrequency = jsonData["frequency"] | RFConfig.frequency;
#    ifdef ZradioCC1101 // set Receive off and Transmitt on
    disableCurrentReceiver();
    ELECHOUSE_cc1101.SetTx(txFrequency);
    Log.notice(F("Transmit frequency: %F" CR), txFrequency);
#    endif

    const int remoteIndex = jsonData["remote"];
    if (remoteIndex >= SOMFY_REMOTE_NUM) {
      Log.warning(F("ZactuatorSomfy remote does not exist" CR));
      return;
    }
    const String commandData = jsonData["command"];
    const Command command = getSomfyCommand(commandData);

    const int repeat = jsonData["repeat"] | 4;

    EEPROMRollingCodeStorage rollingCodeStorage(EEPROM_ADDRESS_START + remoteIndex * 2);
    SomfyRemote somfyRemote(RF_EMITTER_GPIO, somfyRemotes[remoteIndex], &rollingCodeStorage);
    somfyRemote.sendCommand(command, repeat);
    initCC1101();
    enableActiveReceiver();
  }
}
#  endif

#endif
