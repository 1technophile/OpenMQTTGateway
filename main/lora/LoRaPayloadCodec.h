#ifndef LoRaPayloadCodec_h
#define LoRaPayloadCodec_h

#include <cstddef>
#include <cstdint>

bool decodeLoRaHex(const char* input, uint8_t* output, size_t capacity, size_t& length);
bool copyLoRaText(const char* input, uint8_t* output, size_t capacity, size_t& length);

#endif
