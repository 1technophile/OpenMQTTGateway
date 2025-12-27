/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your BLE/433mhz/infrared IR signal and an MQTT broker
   Send and receiving command by MQTT

    Copyright: (c)Florian ROBERT

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

#if defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif
#  include <rf/RFConfiguration.h>

#  include "TheengsCommon.h"
#  include "config_RF.h"

#  ifdef ZgatewayRTL_433
#    include <rtl_433_ESP.h>
extern rtl_433_ESP rtl_433;
#  endif

int currentReceiver = ACTIVE_NONE;
boolean isDriverEnabled = false;
extern void enableActiveReceiver();
extern void disableCurrentReceiver();

// Note: this is currently just a simple wrapper used to make everything work.
// It prevents introducing external dependencies on newly added C++ structures,
// and acts as a first approach to mask the concrete implementations (rf, rf2,
// pilight, etc.). Later this can be extended or replaced by more complete driver
// abstractions without changing the rest of the system.
class ZCommonRFWrapper : public RFReceiver {
public:
  ZCommonRFWrapper() : RFReceiver() {}
  void enable() override { enableActiveReceiver(); }
  void disable() override { disableCurrentReceiver(); }
  int getReceiverID() const override { return currentReceiver; }
};

ZCommonRFWrapper iRFReceiver;
RFConfiguration iRFConfig(iRFReceiver);

// Initialize the CC1101 and tune the RX frequency.
// - Uses truncated exponential backoff to avoid tight retry loops
// - Validates the configured frequency before touching the radio
// - Emits explicit logs for success/failure so watchdog resets are traceable
void initCC1101() {
#  ifdef ZradioCC1101 // receiving with CC1101
  const float freqMhz = iRFConfig.getFrequency();
  if (!iRFConfig.validFrequency(freqMhz)) {
    THEENGS_LOG_ERROR(F("C1101 invalid frequency: %F MHz" CR), freqMhz);
    return;
  }

  int delayMS = 16;
  int delayMaxMS = 500;
  bool connected = false;

  for (int attempt = 1; attempt <= 10; attempt++) {
#    if defined(RF_CC1101_SCK) && defined(RF_CC1101_MISO) && \
        defined(RF_CC1101_MOSI) && defined(RF_CC1101_CS)
    THEENGS_LOG_TRACE(F("initCC1101 with custom SPI pins, SCK=%d, MISO=%d, MOSI=%d, CS=%d" CR), RF_CC1101_SCK, RF_CC1101_MISO, RF_CC1101_MOSI, RF_CC1101_CS);
    ELECHOUSE_cc1101.setSpiPin(RF_CC1101_SCK, RF_CC1101_MISO, RF_CC1101_MOSI, RF_CC1101_CS);
#    endif

    if (ELECHOUSE_cc1101.getCC1101()) {
      connected = true;
      THEENGS_LOG_NOTICE(F("C1101 SPI connection OK on attempt %d" CR), attempt);
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.SetRx(freqMhz);
      THEENGS_LOG_NOTICE(F("C1101 tuned RX to %F MHz" CR), freqMhz);
      break;
    }

    THEENGS_LOG_ERROR(F("C1101 SPI connection error (attempt %d), retrying" CR), attempt);
    delay(delayMS);

    // truncated exponential backoff
    delayMS = delayMS * 2;
    if (delayMS > delayMaxMS) delayMS = delayMaxMS;
  }

  if (!connected) {
    THEENGS_LOG_ERROR(F("C1101 init failed after retries, radio left disabled" CR));
  }
#  else
  THEENGS_LOG_TRACE(F("initCC1101 skipped: ZradioCC1101 not enabled" CR));
#  endif
}

void setupCommonRF() {
  iRFConfig.reInit();
  iRFConfig.loadFromStorage(true);
}

#  if !defined(ZgatewayRFM69) && !defined(ZactuatorSomfy)
// Check if a receiver is available
bool validReceiver(int receiver) {
  switch (receiver) {
#    ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      return true;
#    endif
#    ifdef ZgatewayRF
    case ACTIVE_RF:
      return true;
#    endif
#    ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      return true;
#    endif
#    ifdef ZgatewayRF2
    case ACTIVE_RF2:
      return true;
#    endif
    default:
      THEENGS_LOG_ERROR(F("ERROR: stored receiver %d not available" CR), receiver);
  }
  return false;
}
#  endif

void disableCurrentReceiver() {
  if (!isDriverEnabled) {
    THEENGS_LOG_TRACE(F("Receiver %d is already disabled" CR), currentReceiver);
    return;
  }
  THEENGS_LOG_TRACE(F("disableCurrentReceiver: %d" CR), currentReceiver);
  switch (currentReceiver) {
    case ACTIVE_NONE:
      isDriverEnabled = false;
      break;
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      disablePilightReceive();
      isDriverEnabled = false;
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      disableRFReceive();
      isDriverEnabled = false;
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      disableRTLreceive();
      isDriverEnabled = false;
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      disableRF2Receive();
      isDriverEnabled = false;
      break;
#  endif
    default:
      THEENGS_LOG_ERROR(F("ERROR: unsupported receiver %d" CR), iRFConfig.getActiveReceiver());
  }
}

void enableActiveReceiver() {
  if (isDriverEnabled) {
    THEENGS_LOG_TRACE(F("Receiver %d is already enabled" CR), currentReceiver);
    return;
  }
  THEENGS_LOG_TRACE(F("enableActiveReceiver: %d" CR), iRFConfig.getActiveReceiver());
  switch (iRFConfig.getActiveReceiver()) {
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      initCC1101();
      enablePilightReceive();
      currentReceiver = ACTIVE_PILIGHT;
      isDriverEnabled = true;
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      initCC1101();
      enableRFReceive(iRFConfig.getFrequency(), RF_RECEIVER_GPIO, RF_EMITTER_GPIO);
      currentReceiver = ACTIVE_RF;
      isDriverEnabled = true;
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      initCC1101();
      enableRTLreceive();
      currentReceiver = ACTIVE_RTL;
      isDriverEnabled = true;
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      initCC1101();
      enableRF2Receive();
      currentReceiver = ACTIVE_RF2;
      isDriverEnabled = true;
      break;
#  endif
    case ACTIVE_RECERROR:
      THEENGS_LOG_ERROR(F("ERROR: no receiver selected" CR));
      break;
    default:
      THEENGS_LOG_ERROR(F("ERROR: unsupported receiver %d" CR), iRFConfig.getActiveReceiver());
  }
}

String stateRFMeasures() {
  //Publish RTL_433 state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject RFdata = jsonBuffer.to<JsonObject>();

  // load the configuration
  iRFConfig.toJson(RFdata);

  // load the current state
#  if defined(ZradioCC1101) || defined(ZradioSX127x)
  if (iRFConfig.getActiveReceiver() == ACTIVE_RTL) {
#    ifdef ZgatewayRTL_433
    RFdata["rssithreshold"] = (int)getRTLrssiThreshold();
    RFdata["rssi"] = (int)getRTLCurrentRSSI();
    RFdata["avgrssi"] = (int)getRTLAverageRSSI();
    RFdata["count"] = (int)getRTLMessageCount();
    // Capture high water mark of rtl_433_Decoder stack since it can run out and trigger reboot
    extern TaskHandle_t rtl_433_DecoderHandle;
    RFdata["rtl433_stack"] = (int)uxTaskGetStackHighWaterMark(rtl_433_DecoderHandle);
#    endif
#    ifdef ZradioSX127x
    RFdata["ookthreshold"] = (int)getOOKThresh();
#    endif
  }
#  endif
  RFdata["origin"] = subjectcommonRFtoMQTT;
  enqueueJsonObject(RFdata);

  String output;
  serializeJson(RFdata, output);
  return output;
}

void XtoRFset(const char* topicOri, JsonObject& RFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRFset)) {
    THEENGS_LOG_TRACE(F("MQTTtoRF json set" CR));

    iRFConfig.loadFromMessage(RFdata);

    stateRFMeasures();
  }
}
#endif
