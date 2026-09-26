#ifndef LoRaRadioConfig_h
#define LoRaRadioConfig_h

#include <cstdint>

struct LORAConfig_s {
  int32_t frequency;
  int txPower;
  int spreadingFactor;
  int32_t signalBandwidth;
  int codingRateDenominator;
  int preambleLength;
  uint8_t syncWord;
  bool crc;
  bool invertIQ;
  bool onlyKnown;
};

int8_t heltecV4RadioOutputPower(int requestedPower);
bool validateSX1262Config(const LORAConfig_s& config);

#endif
