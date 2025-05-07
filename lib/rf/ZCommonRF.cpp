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

#if defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)

#  include <ArduinoLog.h>
#  include <config_RF.h>
#  include <main_utils.h>
#  include <mqtt/MQTTPublisher.h>
#  include <rf/AbstractGatewayRF.h>
#  include <rf/RFConfig.h>
#  include <rf/ZCommonRF.h>

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

ZCommonRF::ZCommonRF(MQTTPublisher& publicher, RFConfig& iConfig) : currentReceiver(ACTIVE_NONE), publicher(publicher), iRFConfig(iConfig) {
  iRFConfig.setRFHandler(this);
};

void ZCommonRF::setupCommonRF() {
  iRFConfig.load();
}

bool ZCommonRF::validFrequency(float mhz) {
  //  CC1101 valid frequencies 300-348 MHZ, 387-464MHZ and 779-928MHZ.
  if (mhz >= 300 && mhz <= 348)
    return true;
  if (mhz >= 387 && mhz <= 464)
    return true;
  if (mhz >= 779 && mhz <= 928)
    return true;
  return false;
}

void ZCommonRF::disableCurrentReceiver() {
  Log.trace(F("disableCurrentReceiver: %d" CR), currentReceiver);
  try {
    if (gateways.at(currentReceiver)->disableReceive()) {
    } else {
      Log.error(F("RF ERROR: no receiver disabled" CR));
    }
  } catch (const std::out_of_range& e) {
    Log.error(F("RF ERROR: unsupported receiver %d" CR), iRFConfig.getActiveReceiver());
    currentReceiver = ACTIVE_RECERROR;
  } catch (const std::exception& e) {
    Log.error(F("Exception caught: %s" CR), e.what());
    currentReceiver = ACTIVE_RECERROR;
  } catch (...) {
    Log.error(F("Unknown exception caught" CR));
    currentReceiver = ACTIVE_RECERROR;
  }

  /*
  switch (currentReceiver) {
    case ACTIVE_NONE:
      break;
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      disablePilightReceive();
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      disableRFReceive();
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      disableRTLreceive();
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      disableRF2Receive();
      break;
#  endif
    default:
      Log.error(F("ERROR: unsupported receiver %d" CR), RFConfig.activeReceiver);
  }
  */
}

void ZCommonRF::enableActiveReceiver() {
  Log.trace(F("enableActiveReceiver: %d" CR), iRFConfig.getActiveReceiver());

#  ifdef ZradioCC1101 //receiving with CC1101
  initCC1101();
#  endif

  try {
    if (gateways.at(iRFConfig.getActiveReceiver())->enableReceive(iRFConfig.getFrequency(), RF_RECEIVER_GPIO, RF_EMITTER_GPIO)) {
      currentReceiver = iRFConfig.getActiveReceiver();
    } else {
      Log.error(F("RF ERROR: no receiver enabled" CR));
      currentReceiver = ACTIVE_RECERROR;
    }
  } catch (const std::out_of_range& e) {
    Log.error(F("RF ERROR: unsupported receiver %d" CR), iRFConfig.getActiveReceiver());
    currentReceiver = ACTIVE_RECERROR;
  } catch (const std::exception& e) {
    Log.error(F("Exception caught: %s" CR), e.what());
    currentReceiver = ACTIVE_RECERROR;
  } catch (...) {
    Log.error(F("Unknown exception caught" CR));
    currentReceiver = ACTIVE_RECERROR;
  }

  /*
  switch (RFConfig.activeReceiver) {
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      initCC1101();
      enablePilightReceive();
      currentReceiver = ACTIVE_PILIGHT;
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      initCC1101();
      enableRFReceive();
      currentReceiver = ACTIVE_RF;
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      initCC1101();
      enableRTLreceive();
      currentReceiver = ACTIVE_RTL;
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      initCC1101();
      enableRF2Receive();
      currentReceiver = ACTIVE_RF2;
      break;
#  endif
    case ACTIVE_RECERROR:
      Log.error(F("ERROR: no receiver selected" CR));
      break;
    default:
      Log.error(F("ERROR: unsupported receiver %d" CR), RFConfig.activeReceiver);
  }
  */
}

String ZCommonRF::stateRFMeasures() {
  //Publish RTL_433 state

  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject RFdata = jsonBuffer.to<JsonObject>();

  RFdata["active"] = iRFConfig.getActiveReceiver();
#  if defined(ZradioCC1101) || defined(ZradioSX127x)
  RFdata["frequency"] = RFConfig.frequency;
  if (RFConfig.activeReceiver == ACTIVE_RTL) {
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
  publicher.enqueueJsonObject(RFdata);

  String output;
  serializeJson(RFdata, output);
  return output;
}

void ZCommonRF::XtoRFset(const char* topicOri, JsonObject& RFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRFset)) {
    Log.trace(F("MQTTtoRF json set" CR));

    /*
     * Configuration modifications priorities:
     *  First `init=true` and `load=true` commands are executed (if both are present, INIT prevails on LOAD)
     *  Then parameters included in json are taken in account
     *  Finally `erase=true` and `save=true` commands are executed (if both are present, ERASE prevails on SAVE)
     */
    if (RFdata.containsKey("init") && RFdata["init"].as<bool>()) {
      // Restore the default (initial) configuration
      iRFConfig.reset();

    } else if (RFdata.containsKey("load") && RFdata["load"].as<bool>()) {
      // Load the saved configuration, if not initialised
      iRFConfig.load();
    }

    // Load config from json if available
    iRFConfig.from(RFdata);
    stateRFMeasures();
  }
}

#  if defined(ZwebUI) && defined(ESP32)

void ZCommonRF::addGatewayRF(int gatewayRFId, AbstractGatewayRF* gatewayRF) {
  //gatewayRF->setRFHandler(&this);
  gateways[gatewayRFId] = gatewayRF;
}

AbstractGatewayRF* ZCommonRF::getGatewayRF(int gatewayRFId) {
  return gateways[gatewayRFId];
}

AbstractGatewayRF* ZCommonRF::getCurrentGatewayRF() {
  return gateways[currentReceiver];
}

#  endif

#  if !defined(ZgatewayRFM69) && !defined(ZactuatorSomfy)
// Check if a receiver is available
bool ZCommonRF::validReceiver(int receiver) {
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
      Log.error(F("ERROR: stored receiver %d not available" CR), receiver);
  }
  return false;
}
#  endif

MQTTPublisher& ZCommonRF::getPublisher() {
  return publicher;
}

/***********************************************************************************************
* 
* Private Functions
*
***********************************************************************************************/

#  ifdef ZradioCC1101 //receiving with CC1101
void ZCommonRF::initCC1101() {
  // Loop on getCC1101() until it returns true and break after 10 attempts
  int delayMS = 16;
  int delayMaxMS = 500;
  for (int i = 0; i < 10; i++) {
#    if defined(RF_MODULE_SCK) && defined(RF_MODULE_MISO) && \
        defined(RF_MODULE_MOSI) && defined(RF_MODULE_CS)
    ELECHOUSE_cc1101.setSpiPin(RF_MODULE_SCK, RF_MODULE_MISO, RF_MODULE_MOSI, RF_MODULE_CS);
#    endif
    if (ELECHOUSE_cc1101.getCC1101()) {
      Log.notice(F("C1101 spi Connection OK" CR));
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.SetRx(iRFConfig.getFrequency());
      break;
    } else {
      Log.error(F("C1101 spi Connection Error" CR));
      delay(delayMS);
    }
    // truncated exponential backoff
    delayMS = delayMS * 2;
    if (delayMS > delayMaxMS) delayMS = delayMaxMS;
  }
}
#  endif // ZradioCC1101

#endif // ZgatewayRF || ZgatewayPilight || ZgatewayRTL_433 || ZgatewayRF2 || ZactuatorSomfy
