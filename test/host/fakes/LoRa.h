#ifndef TEST_FAKE_LORA_H
#define TEST_FAKE_LORA_H

#include <cstddef>
#include <cstdint>
#include <vector>

class LoRaClass {
public:
  void reset() {
    *this = LoRaClass{};
  }

  void setPins(int ssValue, int resetValue, int dio0Value) {
    ss = ssValue;
    resetPin = resetValue;
    dio0 = dio0Value;
  }

  int begin(long frequencyValue) {
    beginFrequency = frequencyValue;
    return beginResult;
  }

  void setFrequency(long value) { frequency = value; }
  void setTxPower(int value) { txPower = value; }
  void setSpreadingFactor(int value) { spreadingFactor = value; }
  void setSignalBandwidth(long value) { signalBandwidth = value; }
  void setCodingRate4(int value) { codingRate = value; }
  void setPreambleLength(long value) { preambleLength = value; }
  void setSyncWord(int value) { syncWord = value; }
  void enableCrc() { crc = true; }
  void disableCrc() { crc = false; }
  void enableInvertIQ() { invertIQ = true; }
  void disableInvertIQ() { invertIQ = false; }

  int parsePacket(int = 0) {
    if (!packetPending) return 0;
    packetPending = false;
    readIndex = 0;
    return static_cast<int>(received.size());
  }

  int available() const {
    return static_cast<int>(received.size() - readIndex);
  }

  int read() {
    if (readIndex >= received.size()) return -1;
    return received[readIndex++];
  }

  int packetRssi() const { return rssi; }
  float packetSnr() const { return snr; }
  long packetFrequencyError() const { return frequencyError; }

  int beginPacket(int = 0) {
    transmitted.clear();
    return beginPacketResult;
  }

  size_t write(const uint8_t* payload, size_t length) {
    transmitted.assign(payload, payload + length);
    return writeResultMatchesLength ? length : 0;
  }

  int endPacket(bool = false) { return endPacketResult; }
  void receive(int = 0) { ++receiveCalls; }

  void deliver(std::vector<uint8_t> payload) {
    received = std::move(payload);
    packetPending = true;
  }

  int beginResult = 1;
  int beginPacketResult = 1;
  int endPacketResult = 1;
  bool writeResultMatchesLength = true;
  int ss = -1;
  int resetPin = -1;
  int dio0 = -1;
  long beginFrequency = 0;
  long frequency = 0;
  int txPower = 0;
  int spreadingFactor = 0;
  long signalBandwidth = 0;
  int codingRate = 0;
  long preambleLength = 0;
  int syncWord = 0;
  bool crc = false;
  bool invertIQ = false;
  std::vector<uint8_t> received;
  std::vector<uint8_t> transmitted;
  size_t readIndex = 0;
  bool packetPending = false;
  int receiveCalls = 0;
  int rssi = -80;
  float snr = 7.5f;
  long frequencyError = 120;
};

inline LoRaClass LoRa;

#endif
