#ifndef LoRaRadio_h
#define LoRaRadio_h

#include <Arduino.h>

#include "LoRaRadioConfig.h"

struct LoRaPacketMetrics {
  float rssi = 0;
  float snr = 0;
  float frequencyError = 0;
};

class LoRaRadio {
public:
  bool begin(const LORAConfig_s& config);
  bool apply(const LORAConfig_s& config);
  bool receive(uint8_t* payload, size_t capacity, size_t& length, LoRaPacketMetrics& metrics);
  bool transmit(const uint8_t* payload, size_t length);

  bool ready() const;
  int16_t lastError() const;
  const char* family() const;

private:
  bool startReceive();
  void setError(int16_t state);

  bool ready_ = false;
  int16_t lastError_ = 0;
};

extern LoRaRadio OMGLoRaRadio;

#endif
