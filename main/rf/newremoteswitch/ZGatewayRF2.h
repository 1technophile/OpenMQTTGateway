// ZgatewayRF2.h
#ifndef ZGATEWAYRF2_H
#define ZGATEWAYRF2_H

#ifdef ZgatewayRF2

#  include <User_config.h>
#  include <rf/AbstractGatewayRF.h>

struct RF2rxd {
  unsigned int period;
  unsigned long address;
  unsigned long groupBit;
  unsigned long unit;
  unsigned long switchType;
  bool hasNewData;
};

RF2rxd rf2rd;

class ZCommonRF;

class ZGatewayRF2 : public AbstractGatewayRF {
public:
  ZGatewayRF2(SYSConfig_s& gatewayConfig, ZCommonRF& iZCommonRF);

  ~ZGatewayRF2();

  bool enableReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO);

  bool disableReceive();

  void RFtoX();

#  if simpleReceiving
  void XtoRF(const char* topicOri, const char* datacallback);
#  endif

#  if jsonReceiving
  void XtoRF(const char* topicOri, JsonObject& RF2data);
#  endif

private:
  SYSConfig_s gatewayConfig;

#  ifdef ZmqttDiscovery
  void RF2toMQTTdiscovery(JsonObject& data);
#  endif

  void rf2Callback(unsigned int period, unsigned long address, unsigned long groupBit, unsigned long unit, unsigned long switchType);

  /* data */
  void disableRF2Receive();

  void enableRF2Receive(int rfReceiverGPIO, int rfEmitterGPIO);
};

#endif // ZgatewayRF2

#endif // ZGATEWAYRF2_H