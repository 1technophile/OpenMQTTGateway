#include "LoRaPayloadCodec.h"

#include <cstring>

namespace {

constexpr size_t kMaxLoRaPayload = 255;

int8_t hexValue(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  return -1;
}

} // namespace

bool decodeLoRaHex(const char* input, uint8_t* output, size_t capacity, size_t& length) {
  length = 0;
  if (input == nullptr || output == nullptr) return false;

  const size_t characters = std::strlen(input);
  const size_t decodedLength = characters / 2U;
  if ((characters & 1U) != 0U || decodedLength > capacity ||
      decodedLength > kMaxLoRaPayload)
    return false;

  for (size_t index = 0; index < characters; index += 2) {
    const int8_t high = hexValue(input[index]);
    const int8_t low = hexValue(input[index + 1]);
    if (high < 0 || low < 0) {
      length = 0;
      return false;
    }
    output[length++] = static_cast<uint8_t>((high << 4) | low);
  }
  return true;
}

bool copyLoRaText(const char* input, uint8_t* output, size_t capacity, size_t& length) {
  length = 0;
  if (input == nullptr || output == nullptr) return false;

  const size_t inputLength = std::strlen(input);
  if (inputLength > capacity || inputLength > kMaxLoRaPayload) return false;
  std::memcpy(output, input, inputLength);
  length = inputLength;
  return true;
}
