#include "User_config.h"

#ifdef ZactuatorSomfy

#  include <EEPROM.h>
#  include <EEPROMRollingCodeStorage.h>
#  include <SomfyRemote.h>

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

void setupSomfy() {
#  ifdef ZradioCC1101 //using with CC1101
  ELECHOUSE_cc1101.Init();
#  endif
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

#  ifdef jsonReceiving
void MQTTtoSomfy(char* topicOri, JsonObject& jsonData) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoSomfy)) {
#    ifdef ZradioCC1101
    ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY_SOMFY);
#    endif
    Log.trace(F("MQTTtoSomfy json data analysis" CR));

    const int remoteIndex = jsonData["remote"];
    if (remoteIndex >= SOMFY_REMOTE_NUM) {
      Log.warning(F("ZactuatorSomfy remote does not exist" CR));
      return;
    }
    const String commandData = jsonData["command"];
    const Command command = getSomfyCommand(commandData);

    EEPROMRollingCodeStorage rollingCodeStorage(EEPROM_ADDRESS_START + remoteIndex * 2);
    SomfyRemote somfyRemote(RF_EMITTER_GPIO, somfyRemotes[remoteIndex], &rollingCodeStorage);
    somfyRemote.sendCommand(command);
#    ifdef ZradioCC1101
    ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY); // set Receive on
#    endif
  }
}
#  endif

#endif
