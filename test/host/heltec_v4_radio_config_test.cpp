#include "lora/LoRaRadioConfig.h"

#include <cassert>
#include <iostream>

int main() {
  assert(heltecV4RadioOutputPower(4) == -9);
  assert(heltecV4RadioOutputPower(14) == 1);
  assert(heltecV4RadioOutputPower(28) == 21);

  const LORAConfig_s defaults = {
      868000000, 14, 7, 125000, 5, 8, 0x12, true, false, false};
  assert(validateSX1262Config(defaults));

  LORAConfig_s invalid = defaults;
  invalid.frequency = 149999999;
  assert(!validateSX1262Config(invalid));

  invalid = defaults;
  invalid.frequency = 960000001;
  assert(!validateSX1262Config(invalid));
  invalid.frequency = 150000000;
  assert(validateSX1262Config(invalid));
  invalid.frequency = 960000000;
  assert(validateSX1262Config(invalid));

  invalid = defaults;
  invalid.txPower = 3;
  assert(!validateSX1262Config(invalid));
  invalid.txPower = 29;
  assert(!validateSX1262Config(invalid));
  invalid.txPower = 4;
  assert(validateSX1262Config(invalid));
  invalid.txPower = 28;
  assert(validateSX1262Config(invalid));

  invalid = defaults;
  invalid.spreadingFactor = 4;
  assert(!validateSX1262Config(invalid));
  invalid.spreadingFactor = 13;
  assert(!validateSX1262Config(invalid));
  invalid.spreadingFactor = 5;
  assert(validateSX1262Config(invalid));
  invalid.spreadingFactor = 12;
  assert(validateSX1262Config(invalid));

  invalid = defaults;
  invalid.signalBandwidth = 7799;
  assert(!validateSX1262Config(invalid));
  invalid.signalBandwidth = 500001;
  assert(!validateSX1262Config(invalid));
  invalid.signalBandwidth = 7800;
  assert(validateSX1262Config(invalid));
  invalid.signalBandwidth = 500000;
  assert(validateSX1262Config(invalid));

  invalid = defaults;
  invalid.codingRateDenominator = 4;
  assert(!validateSX1262Config(invalid));
  invalid.codingRateDenominator = 9;
  assert(!validateSX1262Config(invalid));
  invalid.codingRateDenominator = 5;
  assert(validateSX1262Config(invalid));
  invalid.codingRateDenominator = 8;
  assert(validateSX1262Config(invalid));

  std::cout << "Heltec V4 radio configuration tests passed\n";
  return 0;
}
