#include "LoRaRadioConfig.h"

#include <cstddef>
#include <initializer_list>

bool validateSX1262Config(const LORAConfig_s& config) {
  bool supportedBandwidth = false;
  for (const int32_t bandwidth : {7800, 10400, 15500, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000}) {
    if (config.signalBandwidth == bandwidth) supportedBandwidth = true;
  }
  return config.frequency >= 863000000 && config.frequency <= 928000000 &&
         config.txPower >= 4 && config.txPower <= 28 &&
         config.spreadingFactor >= 5 && config.spreadingFactor <= 12 &&
         supportedBandwidth &&
         config.codingRateDenominator >= 5 && config.codingRateDenominator <= 8 &&
         config.preambleLength >= 6 && config.preambleLength <= 65535;
}

int8_t heltecV4RadioOutputPower(int requestedPower) {
  // Measured gain table from Heltec's KCT8103L implementation. Convert the
  // requested board output into the lower SX1262 output that drives the FEM.
  static constexpr uint8_t gain[] = {
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 12, 12, 11, 11, 10, 9, 8, 7};

  int converted = requestedPower;
  for (size_t radioDbm = 0; radioDbm < sizeof(gain); ++radioDbm) {
    const int boardOutput = static_cast<int>(radioDbm) + gain[radioDbm];
    if (boardOutput > requestedPower ||
        (radioDbm == sizeof(gain) - 1 && boardOutput <= requestedPower)) {
      converted = requestedPower - gain[radioDbm];
      break;
    }
  }

  if (converted < -9) converted = -9;
  if (converted > 22) converted = 22;
  return static_cast<int8_t>(converted);
}
