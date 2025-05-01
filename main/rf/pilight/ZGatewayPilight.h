#ifndef ZGATEWAY_PILIGHT_H
#define ZGATEWAY_PILIGHT_H

#include "User_config.h"
#ifdef ZgatewayPilight

#  include <ArduinoJson.h>
#  include <ESPiLight.h>
#  include <rf/AbstractGatewayRF.h>
#  include <rf/RFConfig.h>

class ZCommonRF;

class ZGatewayPilight : public AbstractGatewayRF {
public:
  ZGatewayPilight(RFConfig& iRFConfig, int rfEmitterGPIO, ZCommonRF& iZCommonRF);

  ~ZGatewayPilight();

  virtual bool enableReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO);

  virtual bool disableReceive();

  virtual void RFtoX();

  virtual void XtoRF(const char* topicOri, JsonObject& RFdata);
  virtual void XtoRF(const char* topicOri, const char* datacallback) {
    // Not used in this class
  };

private:
  ESPiLight rf = 0;
  RFConfig& iRFConfig;

#  ifdef Pilight_rawEnabled
  // raw output support
  bool pilightRawEnabled = 0;
#  endif

  // Function declarations
  void pilightCallback(const String& protocol, const String& message, int status, size_t repeats, const String& deviceID);
#  ifdef Pilight_rawEnabled
  void pilightRawCallback(const uint16_t* pulses, size_t length);
#  endif
  void savePilightConfig();
  void loadPilightConfig();

  void disablePilightReceive();
  void enablePilightReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO);
};

#endif // ZgatewayPilight

#endif // ZGATEWAY_PILIGHT_H