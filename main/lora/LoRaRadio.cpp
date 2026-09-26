#include "LoRaRadio.h"

#include <SPI.h>

#include "LoRaRadioPins.h"

#if defined(LORA_RADIO_SX1262)
#  include <RadioLib.h>
#else
#  include <LoRa.h>
#endif

namespace {

constexpr int16_t kRadioError = -1;
#if !defined(LORA_RADIO_SX1262)
constexpr int16_t kPacketTooLongError = -2;
#endif

#if defined(LORA_RADIO_SX1262)
volatile bool packetReceived = false;

void onPacketReceived() {
  packetReceived = true;
}

Module radioModule(LORA_SS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI);
SX1262 radio(&radioModule);

#  if defined(LORA_KCT8103L)
void frontEndOff() {
  pinMode(LORA_PA_CTX, OUTPUT);
  digitalWrite(LORA_PA_CTX, LOW);
  pinMode(LORA_PA_CSD, OUTPUT);
  digitalWrite(LORA_PA_CSD, LOW);
  pinMode(LORA_PA_POWER, OUTPUT);
  digitalWrite(LORA_PA_POWER, LOW);
}

void frontEndReceive() {
  pinMode(LORA_PA_POWER, OUTPUT);
  digitalWrite(LORA_PA_POWER, HIGH);
  delay(1);
  pinMode(LORA_PA_CSD, OUTPUT);
  digitalWrite(LORA_PA_CSD, HIGH);
  delay(1);
  pinMode(LORA_PA_CTX, OUTPUT);
  digitalWrite(LORA_PA_CTX, LORA_RX_LNA);
  delay(1);
}

void frontEndTransmit() {
  pinMode(LORA_PA_POWER, OUTPUT);
  digitalWrite(LORA_PA_POWER, HIGH);
  delay(1);
  pinMode(LORA_PA_CSD, OUTPUT);
  digitalWrite(LORA_PA_CSD, HIGH);
  delay(1);
  pinMode(LORA_PA_CTX, OUTPUT);
  digitalWrite(LORA_PA_CTX, HIGH);
  delay(2);
}

#  else
void frontEndOff() {}
void frontEndReceive() {}
void frontEndTransmit() {}
#  endif

#endif

} // namespace

LoRaRadio OMGLoRaRadio;

bool LoRaRadio::begin(const LORAConfig_s& config) {
  ready_ = false;
  lastError_ = 0;

#ifdef ESP8266
  SPI.begin();
#else
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
#endif

#if defined(LORA_RADIO_SX1262)
  packetReceived = false;
  frontEndOff();
#  ifndef LORA_TCXO_VOLTAGE
#    define LORA_TCXO_VOLTAGE 1.8f
#  endif
  int16_t state = radio.begin(
      static_cast<float>(config.frequency) / 1000000.0f,
      static_cast<float>(config.signalBandwidth) / 1000.0f,
      static_cast<uint8_t>(config.spreadingFactor),
      static_cast<uint8_t>(config.codingRateDenominator),
      config.syncWord,
      heltecV4RadioOutputPower(config.txPower),
      static_cast<uint16_t>(config.preambleLength),
      LORA_TCXO_VOLTAGE);
  if (state != RADIOLIB_ERR_NONE) {
    setError(state);
    return false;
  }
  state = radio.setDio2AsRfSwitch(false);
  if (state != RADIOLIB_ERR_NONE) {
    setError(state);
    return false;
  }
  radio.setPacketReceivedAction(onPacketReceived);
#else
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(config.frequency)) {
    setError(kRadioError);
    return false;
  }
#endif

  ready_ = true;
  if (!apply(config)) {
    ready_ = false;
    return false;
  }
  return true;
}

bool LoRaRadio::apply(const LORAConfig_s& config) {
  if (!ready_) return false;

#if defined(LORA_RADIO_SX1262)
  int16_t state = radio.standby();
  frontEndOff();
  if (state == RADIOLIB_ERR_NONE)
    state = radio.setFrequency(static_cast<float>(config.frequency) / 1000000.0f);
  if (state == RADIOLIB_ERR_NONE)
    state = radio.setOutputPower(heltecV4RadioOutputPower(config.txPower));
  if (state == RADIOLIB_ERR_NONE)
    state = radio.setSpreadingFactor(static_cast<uint8_t>(config.spreadingFactor));
  if (state == RADIOLIB_ERR_NONE)
    state = radio.setBandwidth(static_cast<float>(config.signalBandwidth) / 1000.0f);
  if (state == RADIOLIB_ERR_NONE)
    state = radio.setCodingRate(static_cast<uint8_t>(config.codingRateDenominator));
  if (state == RADIOLIB_ERR_NONE)
    state = radio.setPreambleLength(static_cast<size_t>(config.preambleLength));
  if (state == RADIOLIB_ERR_NONE) state = radio.setSyncWord(config.syncWord);
  if (state == RADIOLIB_ERR_NONE) state = radio.setCRC(config.crc ? 2 : 0);
  if (state == RADIOLIB_ERR_NONE) state = radio.invertIQ(config.invertIQ);
  if (state != RADIOLIB_ERR_NONE) {
    startReceive();
    setError(state);
    return false;
  }
#else
  LoRa.setFrequency(config.frequency);
  LoRa.setTxPower(config.txPower);
  LoRa.setSpreadingFactor(config.spreadingFactor);
  LoRa.setSignalBandwidth(config.signalBandwidth);
  LoRa.setCodingRate4(config.codingRateDenominator);
  LoRa.setPreambleLength(config.preambleLength);
  LoRa.setSyncWord(config.syncWord);
  config.crc ? LoRa.enableCrc() : LoRa.disableCrc();
  config.invertIQ ? LoRa.enableInvertIQ() : LoRa.disableInvertIQ();
#endif

  return startReceive();
}

bool LoRaRadio::receive(uint8_t* payload, size_t capacity, size_t& length, LoRaPacketMetrics& metrics) {
  length = 0;
  if (!ready_ || payload == nullptr || capacity == 0) return false;

#if defined(LORA_RADIO_SX1262)
  if (!packetReceived) return false;
  packetReceived = false;

  const size_t packetLength = radio.getPacketLength();
  if (packetLength == 0 || packetLength > capacity) {
    const int16_t state = packetLength > capacity ? RADIOLIB_ERR_PACKET_TOO_LONG : kRadioError;
    startReceive();
    setError(state);
    return false;
  }

  const int16_t state = radio.readData(payload, packetLength);
  metrics.rssi = radio.getRSSI();
  metrics.snr = radio.getSNR();
  metrics.frequencyError = radio.getFrequencyError();
  const bool resumed = startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    setError(state);
    return false;
  }
  if (!resumed) return false;
  length = packetLength;
  return true;
#else
  const int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return false;
  if (static_cast<size_t>(packetSize) > capacity) {
    while (LoRa.available()) LoRa.read();
    startReceive();
    setError(kPacketTooLongError);
    return false;
  }

  for (int index = 0; index < packetSize; ++index) {
    const int value = LoRa.read();
    if (value < 0) {
      setError(kRadioError);
      startReceive();
      return false;
    }
    payload[index] = static_cast<uint8_t>(value);
  }
  metrics.rssi = static_cast<float>(LoRa.packetRssi());
  metrics.snr = LoRa.packetSnr();
  metrics.frequencyError = static_cast<float>(LoRa.packetFrequencyError());
  if (!startReceive()) return false;
  length = static_cast<size_t>(packetSize);
  return true;
#endif
}

bool LoRaRadio::transmit(const uint8_t* payload, size_t length) {
  if (!ready_ || payload == nullptr || length > 255) {
#if defined(LORA_RADIO_SX1262)
    setError(RADIOLIB_ERR_PACKET_TOO_LONG);
#else
    setError(kPacketTooLongError);
#endif
    return false;
  }

#if defined(LORA_RADIO_SX1262)
  packetReceived = false;
  frontEndTransmit();
  const int16_t state = radio.transmit(payload, length);
  const bool resumed = startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    setError(state);
    return false;
  }
  return resumed;
#else
  if (!LoRa.beginPacket()) {
    startReceive();
    setError(kRadioError);
    return false;
  }
  if (LoRa.write(payload, length) != length) {
    startReceive();
    setError(kRadioError);
    return false;
  }
  const int result = LoRa.endPacket();
  const bool resumed = startReceive();
  if (!result) {
    setError(kRadioError);
    return false;
  }
  return resumed;
#endif
}

bool LoRaRadio::ready() const {
  return ready_;
}

int16_t LoRaRadio::lastError() const {
  return lastError_;
}

const char* LoRaRadio::family() const {
#if defined(LORA_RADIO_SX1262)
  return "SX1262";
#else
  return "SX127x";
#endif
}

bool LoRaRadio::startReceive() {
  if (!ready_) return false;
#if defined(LORA_RADIO_SX1262)
  packetReceived = false;
  frontEndReceive();
  const int16_t state = radio.startReceive();
  setError(state);
  if (state != RADIOLIB_ERR_NONE) ready_ = false;
  return state == RADIOLIB_ERR_NONE;
#else
  LoRa.receive();
  setError(0);
  return true;
#endif
}

void LoRaRadio::setError(int16_t state) {
  lastError_ = state;
}
