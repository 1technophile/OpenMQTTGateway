#include "lora/LoRaPayloadCodec.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
  std::array<uint8_t, 4> output = {0xA5, 0xA5, 0xA5, 0xA5};
  size_t length = 99;

  assert(decodeLoRaHex("00aF7b", output.data(), output.size(), length));
  assert(length == 3);
  assert(output[0] == 0x00);
  assert(output[1] == 0xAF);
  assert(output[2] == 0x7B);
  assert(output[3] == 0xA5);

  length = 99;
  assert(decodeLoRaHex("", output.data(), output.size(), length));
  assert(length == 0);

  length = 99;
  assert(!decodeLoRaHex(nullptr, output.data(), output.size(), length));
  assert(length == 0);
  length = 99;
  assert(!decodeLoRaHex("ABC", output.data(), output.size(), length));
  assert(length == 0);
  length = 99;
  assert(!decodeLoRaHex("GG", output.data(), output.size(), length));
  assert(length == 0);
  length = 99;
  assert(!decodeLoRaHex("0011", output.data(), 1, length));
  assert(length == 0);

  std::array<uint8_t, 257> largeOutput = {};
  largeOutput.back() = 0xA5;
  const std::string hex256(512, 'A');
  length = 99;
  assert(!decodeLoRaHex(hex256.c_str(), largeOutput.data(), 256, length));
  assert(length == 0);
  assert(largeOutput.back() == 0xA5);

  const std::string text255(255, 'x');
  length = 99;
  assert(copyLoRaText(text255.c_str(), largeOutput.data(), 255, length));
  assert(length == 255);
  assert(largeOutput[254] == 'x');

  const std::string text256(256, 'y');
  length = 99;
  assert(!copyLoRaText(text256.c_str(), largeOutput.data(), 256, length));
  assert(length == 0);

  length = 99;
  assert(!copyLoRaText(nullptr, output.data(), output.size(), length));
  assert(length == 0);
  length = 99;
  assert(copyLoRaText("", output.data(), output.size(), length));
  assert(length == 0);

  std::cout << "LoRa payload codec tests passed\n";
  return 0;
}
