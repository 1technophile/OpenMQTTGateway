#include "lora/LoRaRadio.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#if defined(LORA_RADIO_SX1262)
#  include <RadioLib.h>
#else
#  include <LoRa.h>
#endif

int main() {
  const LORAConfig_s config = {
      868000000, 14, 7, 125000, 5, 8, 0x12, true, false, false};

#if defined(LORA_RADIO_SX1262)
  fake_radiolib::reset();
#else
  LoRa.reset();
#endif

  LoRaRadio radio;
  assert(radio.begin(config));
  assert(radio.ready());
  assert(radio.lastError() == 0);

#if defined(LORA_RADIO_SX1262)
  assert(std::strcmp(radio.family(), "SX1262") == 0);
  assert(fake_radiolib::frequency == 868.0f);
  assert(fake_radiolib::outputPower == 1);
#else
  assert(std::strcmp(radio.family(), "SX127x") == 0);
  assert(LoRa.beginFrequency == 868000000);
  assert(LoRa.txPower == 14);
#endif

  uint8_t received[4] = {};
  size_t receivedLength = 99;
  LoRaPacketMetrics metrics;
#if defined(LORA_RADIO_SX1262)
  const int receiveCallsBeforePacket = fake_radiolib::receiveCalls;
  fake_radiolib::deliver({0x00, 0x7F, 0xFF});
#else
  const int receiveCallsBeforePacket = LoRa.receiveCalls;
  LoRa.deliver({0x00, 0x7F, 0xFF});
#endif
  assert(radio.receive(received, sizeof(received), receivedLength, metrics));
  assert(receivedLength == 3);
  assert(received[0] == 0x00 && received[1] == 0x7F && received[2] == 0xFF);

#if defined(LORA_RADIO_SX1262)
  assert(std::fabs(metrics.rssi - fake_radiolib::rssi) < 0.001f);
  assert(std::fabs(metrics.snr - fake_radiolib::snr) < 0.001f);
  assert(std::fabs(metrics.frequencyError - fake_radiolib::frequencyError) < 0.001f);
  assert(fake_radiolib::receiveCalls > receiveCallsBeforePacket);
#else
  assert(std::fabs(metrics.rssi - static_cast<float>(LoRa.rssi)) < 0.001f);
  assert(std::fabs(metrics.snr - LoRa.snr) < 0.001f);
  assert(std::fabs(metrics.frequencyError - static_cast<float>(LoRa.frequencyError)) < 0.001f);
  assert(LoRa.receiveCalls > receiveCallsBeforePacket);
#endif

  const uint8_t outgoing[] = {0x10, 0x20, 0x30};
#if defined(LORA_RADIO_SX1262)
  fake_arduino::reset();
  const int receiveCallsBeforeTransmit = fake_radiolib::receiveCalls;
#else
  const int receiveCallsBeforeTransmit = LoRa.receiveCalls;
#endif
  assert(radio.transmit(outgoing, sizeof(outgoing)));

#if defined(LORA_RADIO_SX1262)
  assert(fake_radiolib::transmitted == std::vector<uint8_t>({0x10, 0x20, 0x30}));
  assert(fake_radiolib::receiveCalls > receiveCallsBeforeTransmit);
  const std::vector<std::pair<int, int>> expectedFrontEndWrites = {
      {7, HIGH}, {2, HIGH}, {5, HIGH}, {7, HIGH}, {2, HIGH}, {5, LOW}};
  assert(fake_arduino::digitalWrites == expectedFrontEndWrites);
#else
  assert(LoRa.transmitted == std::vector<uint8_t>({0x10, 0x20, 0x30}));
  assert(LoRa.receiveCalls > receiveCallsBeforeTransmit);
#endif

  uint8_t undersizedBuffer[2] = {};
  receivedLength = 99;
#if defined(LORA_RADIO_SX1262)
  const int receiveCallsBeforeOversize = fake_radiolib::receiveCalls;
  fake_radiolib::deliver({0x01, 0x02, 0x03});
#else
  const int receiveCallsBeforeOversize = LoRa.receiveCalls;
  LoRa.deliver({0x01, 0x02, 0x03});
#endif
  assert(!radio.receive(undersizedBuffer, sizeof(undersizedBuffer), receivedLength, metrics));
  assert(receivedLength == 0);
  assert(radio.ready());
  assert(radio.lastError() != 0);
#if defined(LORA_RADIO_SX1262)
  assert(fake_radiolib::receiveCalls > receiveCallsBeforeOversize);
#else
  assert(LoRa.receiveCalls > receiveCallsBeforeOversize);
#endif

#if defined(LORA_RADIO_SX1262)
  fake_radiolib::transmitResult = -77;
  const int receiveCallsBeforeFailure = fake_radiolib::receiveCalls;
#else
  LoRa.endPacketResult = 0;
  const int receiveCallsBeforeFailure = LoRa.receiveCalls;
#endif
  assert(!radio.transmit(outgoing, sizeof(outgoing)));
#if defined(LORA_RADIO_SX1262)
  assert(radio.lastError() == -77);
  assert(fake_radiolib::receiveCalls > receiveCallsBeforeFailure);
  fake_radiolib::transmitResult = RADIOLIB_ERR_NONE;

  fake_radiolib::operationResult = -66;
  const int receiveCallsBeforeApplyFailure = fake_radiolib::receiveCalls;
  assert(!radio.apply(config));
  assert(radio.ready());
  assert(radio.lastError() == -66);
  assert(fake_radiolib::receiveCalls > receiveCallsBeforeApplyFailure);
  fake_radiolib::operationResult = RADIOLIB_ERR_NONE;
#else
  assert(radio.lastError() != 0);
  assert(LoRa.receiveCalls > receiveCallsBeforeFailure);
  LoRa.endPacketResult = 1;

  LoRa.beginPacketResult = 0;
  const int receiveCallsBeforeStartFailure = LoRa.receiveCalls;
  assert(!radio.transmit(outgoing, sizeof(outgoing)));
  assert(radio.lastError() != 0);
  assert(LoRa.receiveCalls > receiveCallsBeforeStartFailure);
  LoRa.beginPacketResult = 1;
#endif

  LoRaRadio failedRadio;
#if defined(LORA_RADIO_SX1262)
  fake_radiolib::reset();
  fake_radiolib::beginResult = -55;
#else
  LoRa.reset();
  LoRa.beginResult = 0;
#endif
  assert(!failedRadio.begin(config));
  assert(!failedRadio.ready());
  assert(failedRadio.lastError() != 0);

  std::cout << "LoRa radio facade tests passed (" << radio.family() << ")\n";
  return 0;
}
