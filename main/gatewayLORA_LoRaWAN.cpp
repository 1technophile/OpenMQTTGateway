/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR/BLE signal  and a MQTT broker
   Send and receiving command by MQTT

  LoRaWAN ABP uplink decoding for the LORA gateway:
 - devices (DevAddr + session keys) are configured at runtime and stored in NVS
 - the frame MIC is verified with the NwkSKey and the FRMPayload decrypted with the AppSKey
 - optionally (LORA_LORAWAN_JS) the payload is decoded by a TTN style JavaScript decoder
   (`function Decoder(bytes, port)` or `function decodeUplink(input)`) run with Duktape
 - optionally Home Assistant discovery entities are declared per device

  This is meant for single channel reception of ABP devices configured on a fixed frequency and data rate,
  it is not a LoRaWAN network server: no join, no downlink, no MAC command handling.

    This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "User_config.h"

#if defined(ZgatewayLORA) && defined(LORA_LORAWAN)
#  ifndef ESP32
#    error "LORA_LORAWAN is only supported on ESP32"
#  endif
#  include <LoRa.h>
#  include <TheengsUtils.h>

#  include <vector>

#  include "TheengsCommon.h"
#  include "config_LORA.h"
#  include "esp_timer.h"
#  include "mbedtls/aes.h"
#  ifdef ZmqttDiscovery
#    include "config_mqttDiscovery.h"
#  endif
#  ifdef LORA_LORAWAN_JS
#    include <duktape.h>
#  endif

extern LORAConfig_s LORAConfig;
extern void LORAConfig_apply();

static std::vector<LoRaWANDevice> lorawanDevices;

// End of the last received packet (DIO0 = RxDone), the reference of the downlink receive windows
static volatile int64_t lorawanRxDoneUs = 0;
static void IRAM_ATTR lorawanRxDoneIsr() {
  lorawanRxDoneUs = esp_timer_get_time();
}

const std::vector<LoRaWANDevice>& LORAWANdevices() {
  return lorawanDevices;
}
static bool lorawanDiscoveryPending = false;

// NVS keys are limited to 15 characters: "lw" + DevAddr for the device, "lj" + DevAddr for its decoder
#  define LORAWAN_NVS_INDEX        "lwdevs"
#  define LORAWAN_DEVICE_JSON_SIZE 3072 // device declaration with keys and entities, NVS strings are limited to ~4000 bytes

static void lorawanNvsKey(char* key, const char* prefix, uint32_t devAddr) {
  snprintf(key, 11, "%s%08" PRIX32, prefix, devAddr);
}

static bool lorawanParseKey(const char* hex, uint8_t* key) {
  if (!hex || strlen(hex) != 32) return false;
  for (int i = 0; i < 32; i++) {
    if (!isxdigit(hex[i])) return false;
  }
  TheengsUtils::_hexToRaw(hex, key, 16);
  return true;
}

static bool lorawanParseDevAddr(const char* hex, uint32_t& devAddr) {
  if (!hex || strlen(hex) != 8) return false;
  for (int i = 0; i < 8; i++) {
    if (!isxdigit(hex[i])) return false;
  }
  devAddr = strtoul(hex, NULL, 16);
  return true;
}

static LoRaWANDevice* lorawanFind(uint32_t devAddr) {
  for (auto& dev : lorawanDevices) {
    if (dev.devAddr == devAddr) return &dev;
  }
  return nullptr;
}

/*-------------------- Crypto --------------------*/

static void lorawanAesBlock(const uint8_t* key, const uint8_t* in, uint8_t* out) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, key, 128);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, in, out);
  mbedtls_aes_free(&ctx);
}

static void lorawanCmacSubkey(const uint8_t* in, uint8_t* out) {
  uint8_t carry = 0;
  for (int i = 15; i >= 0; i--) {
    out[i] = (in[i] << 1) | carry;
    carry = in[i] >> 7;
  }
  if (in[0] & 0x80) out[15] ^= 0x87;
}

// AES-CMAC (RFC 4493) of B0 | msg, B0 being the LoRaWAN MIC block
static void lorawanMic(const uint8_t* key, const uint8_t* b0, const uint8_t* msg, size_t len, uint8_t* mac) {
  uint8_t zero[16] = {0}, l[16], k1[16], k2[16], x[16], y[16];
  lorawanAesBlock(key, zero, l);
  lorawanCmacSubkey(l, k1);
  lorawanCmacSubkey(k1, k2);
  lorawanAesBlock(key, b0, x); // B0 is always a complete, non final block
  size_t blocks = (len + 15) / 16;
  bool complete = blocks > 0 && (len % 16) == 0;
  if (blocks == 0) blocks = 1;
  for (size_t b = 0; b < blocks; b++) {
    for (int i = 0; i < 16; i++) {
      size_t idx = b * 16 + i;
      uint8_t m = idx < len ? msg[idx] : (idx == len ? 0x80 : 0x00);
      if (b == blocks - 1) m ^= complete ? k1[i] : k2[i];
      y[i] = x[i] ^ m;
    }
    lorawanAesBlock(key, y, x);
  }
  memcpy(mac, x, 16);
}

// LoRaWAN 1.0.x FRMPayload encryption, symmetric (AES-CTR like), dir 0 = uplink, 1 = downlink
static void lorawanCrypt(const uint8_t* key, uint8_t dir, uint32_t devAddr, uint32_t fcnt, const uint8_t* in, uint8_t* out, size_t len) {
  uint8_t a[16] = {0x01, 0, 0, 0, 0, dir,
                   (uint8_t)devAddr, (uint8_t)(devAddr >> 8), (uint8_t)(devAddr >> 16), (uint8_t)(devAddr >> 24),
                   (uint8_t)fcnt, (uint8_t)(fcnt >> 8), (uint8_t)(fcnt >> 16), (uint8_t)(fcnt >> 24), 0, 0};
  uint8_t s[16];
  for (size_t i = 0; i < len; i += 16) {
    a[15] = (uint8_t)(i / 16 + 1);
    lorawanAesBlock(key, a, s);
    for (size_t j = 0; j < 16 && i + j < len; j++) out[i + j] = in[i + j] ^ s[j];
  }
}

/*-------------------- JavaScript decoder --------------------*/

#  ifdef LORA_LORAWAN_JS
extern void ESPRestart(byte reason);

// Decoders commonly timestamp their output (new Date()); the clock is otherwise only set for TLS connections.
// Must be called once the network is up.
static void lorawanStartNtp() {
  static bool started = false;
  if (started || time(nullptr) > 1000000000) return;
  configTime(0, 0, NTP_SERVER);
  started = true;
}

static void lorawanDuktapeFatal(void* udata, const char* msg) {
  THEENGS_LOG_ERROR(F("[LoRaWAN] JS fatal error: %s" CR), msg ? msg : "");
  ESPRestart(8); // Duktape fatal errors are mostly out of memory, the heap cannot be recovered
}

// Runs the device decoder, merges the resulting object into LORAdata, returns false (and sets "decoder_error") on failure
static bool lorawanRunDecoder(const String& script, uint8_t fport, const uint8_t* bytes, size_t len, JsonObject& LORAdata) {
  lorawanStartNtp();
  duk_context* ctx = duk_create_heap(NULL, NULL, NULL, NULL, lorawanDuktapeFatal);
  if (!ctx) {
    LORAdata["decoder_error"] = "out of memory";
    return false;
  }
  bool ok = false;
  if (duk_peval_lstring(ctx, script.c_str(), script.length()) != 0) {
    LORAdata["decoder_error"] = duk_safe_to_string(ctx, -1);
  } else {
    duk_pop(ctx);
    // TTN v3 `decodeUplink({bytes, fPort})` returning {data, errors}, else TTN v2 `Decoder(bytes, port)`
    bool v3 = duk_get_global_string(ctx, "decodeUplink") && duk_is_function(ctx, -1);
    if (!v3) {
      duk_pop(ctx);
      duk_get_global_string(ctx, "Decoder");
    }
    if (!duk_is_function(ctx, -1)) {
      LORAdata["decoder_error"] = "no Decoder(bytes, port) or decodeUplink(input) function";
    } else {
      if (v3) duk_push_object(ctx);
      duk_idx_t arr = duk_push_array(ctx);
      for (size_t i = 0; i < len; i++) {
        duk_push_uint(ctx, bytes[i]);
        duk_put_prop_index(ctx, arr, i);
      }
      if (v3) {
        duk_put_prop_string(ctx, -2, "bytes");
        duk_push_uint(ctx, fport);
        duk_put_prop_string(ctx, -2, "fPort");
      } else {
        duk_push_uint(ctx, fport);
      }
      if (duk_pcall(ctx, v3 ? 1 : 2) != DUK_EXEC_SUCCESS) {
        LORAdata["decoder_error"] = duk_safe_to_string(ctx, -1);
      } else {
        if (v3) {
          if (duk_get_prop_string(ctx, -1, "errors") && duk_is_array(ctx, -1) && duk_get_length(ctx, -1) > 0) {
            duk_get_prop_index(ctx, -1, 0);
            LORAdata["decoder_error"] = duk_safe_to_string(ctx, -1);
            duk_pop(ctx);
          }
          duk_pop(ctx);
          duk_get_prop_string(ctx, -1, "data");
        }
        if (duk_is_object(ctx, -1) && !duk_is_array(ctx, -1)) {
          const char* json = duk_json_encode(ctx, -1);
          DynamicJsonDocument decoded(JSON_MSG_BUFFER);
          DeserializationError error = deserializeJson(decoded, json);
          if (error) {
            LORAdata["decoder_error"] = error.c_str();
          } else {
            for (JsonPair kv : decoded.as<JsonObject>()) {
              LORAdata[kv.key()] = kv.value();
            }
            ok = !LORAdata.containsKey("decoder_error");
          }
        } else if (!LORAdata.containsKey("decoder_error")) {
          LORAdata["decoder_error"] = "decoder did not return an object";
        }
      }
    }
  }
  duk_destroy_heap(ctx);
  return ok;
}

// Runs a decoder script on the given bytes without any device (used to test decoders from the WebUI)
bool LORAWANtestDecoder(const String& script, uint8_t fport, const uint8_t* bytes, size_t len, JsonObject& result) {
  return lorawanRunDecoder(script, fport, bytes, len, result);
}
#  endif

/*-------------------- Downlinks --------------------*/

static bool lorawanParseHex(const String& hex, uint8_t* out, size_t maxLen, size_t& len) {
  if (hex.length() % 2 || hex.length() / 2 > maxLen) return false;
  for (size_t i = 0; i < hex.length(); i++) {
    if (!isxdigit(hex[i])) return false;
  }
  len = hex.length() / 2;
  return len == 0 || TheengsUtils::_hexToRaw(hex.c_str(), out, len);
}

// Queues a downlink, sent in the receive windows following the next uplink of the device
bool LORAWANqueueDownlink(uint32_t devAddr, uint8_t fport, const String& hexIn, const char* source, String* error) {
  LoRaWANDevice* dev = lorawanFind(devAddr);
  String hex = hexIn;
  hex.replace(" ", "");
  hex.toUpperCase();
  uint8_t bytes[LORAWAN_MAX_DOWNLINK_SIZE];
  size_t len;
  const char* problem = nullptr;
  if (!dev)
    problem = "unknown device";
  else if (fport < 1 || fport > 223)
    problem = "fport must be 1 to 223";
  else if (!lorawanParseHex(hex, bytes, LORAWAN_MAX_DOWNLINK_SIZE, len))
    problem = "payload must be an even number of hex digits, up to LORAWAN_MAX_DOWNLINK_SIZE bytes";
  else if (dev->queue.size() >= LORAWAN_MAX_QUEUE)
    problem = "downlink queue full";
  if (problem) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] downlink for %X not queued: %s" CR), devAddr, problem);
    if (error) *error = problem;
    return false;
  }
  dev->queue.push_back({fport, hex, source});
  THEENGS_LOG_NOTICE(F("[LoRaWAN] downlink queued for %X (%s): port %d %s, %d pending" CR), devAddr, source, fport, hex.c_str(), dev->queue.size());
  return true;
}

void LORAWANclearQueue(uint32_t devAddr) {
  LoRaWANDevice* dev = lorawanFind(devAddr);
  if (dev) dev->queue.clear();
}

struct LoRaWANWindow {
  long frequency;
  int sf;
  long bw;
  int power; // dBm, PA_BOOST output: 2 to 20
};

// Receive windows of the device: region defaults derived from the (single) uplink channel, overridable per device
static void lorawanWindows(JsonObject rules, LoRaWANWindow& rx1, LoRaWANWindow& rx2) {
  long f = LORAConfig.frequency;
  int sf = LORAConfig.spreadingFactor;
  long bw = LORAConfig.signalBandwidth;
  int power = LORAConfig.txPower;
  if (f >= 902000000 && f <= 928000000 && bw == 125000) {
    // US915 / AU915: RX1 on the 500 kHz channel (uplink channel modulo 8) with the same SF, RX2 923.3 MHz SF12 500 kHz;
    // downlinks use 500 kHz (about 6 dB less sensitive than the 125 kHz uplinks) and up to 30 dBm are allowed: use the 20 dBm maximum
    long base = f < 915200000 ? 902300000 : 915200000;
    int channel = (f - base + 100000) / 200000;
    rx1 = {923300000L + 600000L * (channel % 8), sf, 500000, 20};
    rx2 = {923300000L, 12, 500000, 20};
  } else if (f >= 863000000 && f <= 870000000) {
    // EU868: RX1 on the uplink channel (14 dBm ERP limit), RX2 869.525 MHz SF12 125 kHz (869.4-869.65 MHz allows 500 mW)
    rx1 = {f, sf, bw, power > 14 ? 14 : power};
    rx2 = {869525000L, 12, 125000, 20};
  } else {
    rx1 = {f, sf, bw, power};
    rx2 = {f, sf, bw, power};
  }
  rx1.frequency = rules["rx1_frequency"] | rx1.frequency;
  rx1.sf = rules["rx1_sf"] | rx1.sf;
  rx1.bw = rules["rx1_bw"] | rx1.bw;
  rx2.frequency = rules["rx2_frequency"] | rx2.frequency;
  rx2.sf = rules["rx2_sf"] | rx2.sf;
  rx2.bw = rules["rx2_bw"] | rx2.bw;
  rx1.power = rules["tx_power"] | rx1.power;
  rx2.power = rules["tx_power"] | rx2.power;
}

// Transmits the frame when esp_timer reaches atUs, then restores the uplink reception settings
static bool lorawanTransmitAt(int64_t atUs, const LoRaWANWindow& w, const uint8_t* frame, size_t len) {
  LoRa.idle();
  LoRa.setFrequency(w.frequency);
  LoRa.setSpreadingFactor(w.sf);
  LoRa.setSignalBandwidth(w.bw);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(LORAConfig.syncWord);
  LoRa.disableCrc(); // LoRaWAN downlinks have no payload CRC
  LoRa.enableInvertIQ(); // and use inverted IQ
  LoRa.setTxPower(w.power);
  LoRa.beginPacket();
  LoRa.write(frame, len);
  bool sent = false;
  int64_t wait = atUs - esp_timer_get_time();
  if (wait > 500) {
    if (wait > 3000) delay((wait - 2000) / 1000);
    while (esp_timer_get_time() < atUs) {
    }
    LoRa.endPacket(); // starts the transmission and waits for TxDone
    sent = true;
  }
  LORAConfig_apply();
  LoRa.receive();
  return sent;
}

static void lorawanSaveFcntDown(const LoRaWANDevice& dev) {
  char key[11];
  lorawanNvsKey(key, "lc", dev.devAddr);
  preferences.begin(Gateway_Short_Name, false);
  preferences.putUInt(key, dev.fcntDown);
  preferences.end();
}

// Does the decoded uplink match a trigger condition: "uplink" or an object of field values that must all be equal
static bool lorawanTriggerMatches(JsonVariant when, JsonObject& LORAdata) {
  if (when.is<const char*>()) return strcmp(when.as<const char*>(), "uplink") == 0;
  if (!when.is<JsonObject>()) return false;
  for (JsonPair kv : when.as<JsonObject>()) {
    if (!LORAdata.containsKey(kv.key())) return false;
    String expected, actual;
    serializeJson(kv.value(), expected);
    serializeJson(LORAdata[kv.key()], actual);
    if (expected != actual) return false;
  }
  return true;
}

// Applies the schedules and triggers of the device, then sends the first queued downlink (and/or the ACK of a
// confirmed uplink) in RX1, or RX2 if RX1 cannot be reached anymore
static void lorawanProcessDownlink(LoRaWANDevice& dev, bool confirmedUplink, int64_t rxDoneUs, JsonObject& LORAdata) {
  DynamicJsonDocument rulesDoc(LORAWAN_DEVICE_JSON_SIZE);
  if (dev.downlinks.length()) deserializeJson(rulesDoc, dev.downlinks);
  JsonObject rules = rulesDoc.as<JsonObject>();
  uint32_t now = millis();
  size_t ruleCount = rules["schedules"].size() + rules["triggers"].size();
  if (dev.ruleLast.size() != ruleCount) dev.ruleLast.assign(ruleCount, 0);
  size_t rule = 0;
  for (JsonObject schedule : rules["schedules"].as<JsonArray>()) {
    uint32_t every = schedule["every"] | 0;
    // schedules fire at the first uplink, then at the first uplink after each interval
    if (every && (dev.ruleLast[rule] == 0 || now - dev.ruleLast[rule] >= every * 1000UL)) {
      if (LORAWANqueueDownlink(dev.devAddr, schedule["fport"] | 0, schedule["hex"] | "", "schedule", nullptr)) dev.ruleLast[rule] = now ? now : 1;
    }
    rule++;
  }
  for (JsonObject trigger : rules["triggers"].as<JsonArray>()) {
    uint32_t cooldown = trigger["cooldown"] | 0;
    if (lorawanTriggerMatches(trigger["when"], LORAdata) && (dev.ruleLast[rule] == 0 || now - dev.ruleLast[rule] >= cooldown * 1000UL)) {
      if (LORAWANqueueDownlink(dev.devAddr, trigger["fport"] | 0, trigger["hex"] | "", "trigger", nullptr)) {
        dev.queue.insert(dev.queue.begin(), dev.queue.back()); // triggers answer this uplink first
        dev.queue.pop_back();
        dev.ruleLast[rule] = now ? now : 1;
      }
    }
    rule++;
  }
  if (dev.queue.empty() && !confirmedUplink) return;
  if (rxDoneUs == 0) {
    LORAdata["downlink_error"] = "no RxDone timestamp";
    return;
  }

  bool hasData = !dev.queue.empty();
  uint8_t frame[13 + LORAWAN_MAX_DOWNLINK_SIZE];
  size_t n = 0;
  frame[n++] = 0x60; // unconfirmed data down
  for (int i = 0; i < 4; i++) frame[n++] = (uint8_t)(dev.devAddr >> (8 * i));
  frame[n++] = (confirmedUplink ? 0x20 : 0x00) | (dev.queue.size() > 1 ? 0x10 : 0x00); // FCtrl: ACK, FPending
  frame[n++] = (uint8_t)dev.fcntDown;
  frame[n++] = (uint8_t)(dev.fcntDown >> 8);
  if (hasData) {
    const LoRaWANDownlink& dl = dev.queue.front();
    uint8_t payload[LORAWAN_MAX_DOWNLINK_SIZE];
    size_t len = 0;
    lorawanParseHex(dl.hex, payload, LORAWAN_MAX_DOWNLINK_SIZE, len);
    frame[n++] = dl.fport;
    lorawanCrypt(dev.appSKey, 1, dev.devAddr, dev.fcntDown, payload, frame + n, len);
    n += len;
  }
  uint8_t b0[16] = {0x49, 0, 0, 0, 0, 1, frame[1], frame[2], frame[3], frame[4],
                    (uint8_t)dev.fcntDown, (uint8_t)(dev.fcntDown >> 8), (uint8_t)(dev.fcntDown >> 16), (uint8_t)(dev.fcntDown >> 24), 0, (uint8_t)n};
  uint8_t mac[16];
  lorawanMic(dev.nwkSKey, b0, frame, n, mac);
  memcpy(frame + n, mac, 4);
  n += 4;

  LoRaWANWindow rx1, rx2;
  lorawanWindows(rules, rx1, rx2);
  int64_t rx1At = rxDoneUs + (int64_t)(rules["rx1_delay"] | 1) * 1000000LL;
  const char* window = "rx1";
  bool sent = lorawanTransmitAt(rx1At, rx1, frame, n);
  if (!sent) {
    window = "rx2";
    sent = lorawanTransmitAt(rx1At + 1000000LL, rx2, frame, n);
  }
  if (!sent) {
    LORAdata["downlink_error"] = "receive windows missed";
    THEENGS_LOG_WARNING(F("[LoRaWAN] downlink for %X: receive windows missed" CR), dev.devAddr);
  } else {
    JsonObject dl = LORAdata.createNestedObject("downlink");
    dl["fcnt"] = dev.fcntDown;
    dl["window"] = window;
    dl["power"] = strcmp(window, "rx1") == 0 ? rx1.power : rx2.power;
    if (confirmedUplink) dl["ack"] = true;
    if (hasData) {
      dl["fport"] = dev.queue.front().fport;
      dl["payload"] = dev.queue.front().hex;
      dl["source"] = dev.queue.front().source;
      dev.queue.erase(dev.queue.begin());
    }
    THEENGS_LOG_NOTICE(F("[LoRaWAN] downlink sent to %X in %s, FCnt %u" CR), dev.devAddr, window, dev.fcntDown);
    dev.fcntDown++;
    lorawanSaveFcntDown(dev);
  }
  if (dev.queue.size()) LORAdata["downlink_pending"] = dev.queue.size();
}

/*-------------------- Uplink decoding --------------------*/

int LORAWANtoJson(const uint8_t* p, int len, JsonObject& LORAdata) {
  if (len < 12) return LORAWAN_NOT_HANDLED;
  uint8_t mtype = p[0] >> 5;
  if (mtype != 2 && mtype != 4) return LORAWAN_NOT_HANDLED; // (un)confirmed data up
  uint32_t devAddr = p[1] | (p[2] << 8) | (p[3] << 16) | ((uint32_t)p[4] << 24);
  LoRaWANDevice* dev = lorawanFind(devAddr);
  if (!dev) return LORAWAN_NOT_HANDLED;

  int64_t rxDoneUs = lorawanRxDoneUs;
  int hdrLen = 8 + (p[5] & 0x0F); // MHDR + DevAddr + FCtrl + FCnt + FOpts
  int msgLen = len - 4; // without MIC
  if (msgLen < hdrLen) return LORAWAN_NOT_HANDLED;
  // Rebuild the 32 bits frame counter from the 16 transmitted bits
  uint32_t fcnt = (dev->lastFcnt & 0xFFFF0000) | (p[6] | (p[7] << 8));
  if (dev->seen && fcnt < dev->lastFcnt) fcnt += 0x10000;

  uint8_t b0[16] = {0x49, 0, 0, 0, 0, 0, p[1], p[2], p[3], p[4],
                    (uint8_t)fcnt, (uint8_t)(fcnt >> 8), (uint8_t)(fcnt >> 16), (uint8_t)(fcnt >> 24), 0, (uint8_t)msgLen};
  uint8_t mac[16];
  lorawanMic(dev->nwkSKey, b0, p, msgLen, mac);
  if (memcmp(mac, p + msgLen, 4) != 0) {
    THEENGS_LOG_WARNING(F("[LoRaWAN] MIC check failed for %X FCnt %u" CR), devAddr, fcnt);
    return LORAWAN_NOT_HANDLED;
  }
  if (dev->seen && fcnt == dev->lastFcnt) {
    THEENGS_LOG_NOTICE(F("[LoRaWAN] Duplicate FCnt %u from %X ignored" CR), fcnt, devAddr);
    return LORAWAN_DROP;
  }
  dev->lastFcnt = fcnt;
  dev->seen = true;

  char id[9];
  snprintf(id, sizeof(id), "%08" PRIX32, devAddr);
  LORAdata["id"] = id;
  if (dev->name.length()) LORAdata["name"] = dev->name;
  if (dev->model.length()) LORAdata["model"] = dev->model;
  LORAdata["fcnt"] = fcnt;
  if (mtype == 4) LORAdata["confirmed"] = true;

  if (msgLen > hdrLen) {
    uint8_t fport = p[hdrLen];
    size_t n = msgLen - hdrLen - 1;
    uint8_t plain[n > 0 ? n : 1];
    lorawanCrypt(fport == 0 ? dev->nwkSKey : dev->appSKey, 0, devAddr, fcnt, p + hdrLen + 1, plain, n);
    char hex[n * 2 + 2]; // _rawToHex writes "%02X\r" per byte: needs 2 * size + 2
    TheengsUtils::_rawToHex(plain, hex, n);
    hex[n * 2] = 0;
    LORAdata["fport"] = fport;
    LORAdata["payload"] = hex;
    dev->lastPayload = hex;
    dev->lastPort = fport;
#  ifdef LORA_LORAWAN_JS
    if (fport != 0 && dev->decoder.length() && !lorawanRunDecoder(dev->decoder, fport, plain, n, LORAdata))
      THEENGS_LOG_WARNING(F("[LoRaWAN] decoder error for %X: %s" CR), devAddr, LORAdata["decoder_error"].as<const char*>());
#  endif
  }
  lorawanProcessDownlink(*dev, mtype == 4, rxDoneUs, LORAdata);
  return LORAWAN_DECODED;
}

/*-------------------- Configuration --------------------*/

static void lorawanToJson(const LoRaWANDevice& dev, JsonObject& jo, bool withKeys) {
  char hex[34]; // _rawToHex writes "%02X\r" per byte: needs 2 * size + 2
  snprintf(hex, 9, "%08" PRIX32, dev.devAddr);
  jo["devaddr"] = hex;
  if (withKeys) {
    TheengsUtils::_rawToHex((byte*)dev.nwkSKey, hex, 16);
    hex[32] = 0;
    jo["nwkskey"] = hex;
    TheengsUtils::_rawToHex((byte*)dev.appSKey, hex, 16);
    hex[32] = 0;
    jo["appskey"] = hex;
  }
  if (dev.name.length()) jo["name"] = dev.name;
  if (dev.model.length()) jo["model"] = dev.model;
  if (dev.entities.length()) {
    if (withKeys)
      jo["entities"] = serialized(dev.entities);
    else
      jo["entities"] = true;
  }
  if (dev.downlinks.length()) {
    if (withKeys)
      jo["downlinks"] = serialized(dev.downlinks);
    else
      jo["downlinks"] = true;
  }
#  ifdef LORA_LORAWAN_JS
  if (!withKeys) jo["decoder"] = dev.decoder.length() > 0;
#  endif
  if (!withKeys && dev.seen) jo["fcnt"] = dev.lastFcnt;
  if (!withKeys) {
    jo["fcnt_down"] = dev.fcntDown;
    if (dev.queue.size()) jo["downlink_pending"] = dev.queue.size();
  }
}

static void lorawanSaveIndex() {
  String index;
  for (auto& dev : lorawanDevices) {
    char hex[9];
    snprintf(hex, sizeof(hex), "%08" PRIX32, dev.devAddr);
    if (index.length()) index += ",";
    index += hex;
  }
  preferences.begin(Gateway_Short_Name, false);
  preferences.putString(LORAWAN_NVS_INDEX, index);
  preferences.end();
}

static void lorawanSaveDevice(const LoRaWANDevice& dev) {
  DynamicJsonDocument doc(LORAWAN_DEVICE_JSON_SIZE);
  JsonObject jo = doc.to<JsonObject>();
  lorawanToJson(dev, jo, true);
  String conf;
  serializeJson(doc, conf);
  char key[11];
  lorawanNvsKey(key, "lw", dev.devAddr);
  preferences.begin(Gateway_Short_Name, false);
  int result = preferences.putString(key, conf);
  preferences.end();
  THEENGS_LOG_NOTICE(F("[LoRaWAN] device %X saved, result: %d" CR), dev.devAddr, result);
}

static void lorawanEraseDevice(uint32_t devAddr) {
  char key[11];
  preferences.begin(Gateway_Short_Name, false);
  lorawanNvsKey(key, "lw", devAddr);
  preferences.remove(key);
  lorawanNvsKey(key, "lj", devAddr);
  if (preferences.isKey(key)) preferences.remove(key);
  lorawanNvsKey(key, "lc", devAddr);
  if (preferences.isKey(key)) preferences.remove(key);
  preferences.end();
}

// Applies one device declaration, returns false if invalid
// Logs a configuration error and reports it to the caller (WebUI) when requested
#  define LORAWAN_CONFIG_ERROR(error, msg)          \
    do {                                            \
      THEENGS_LOG_ERROR(F("[LoRaWAN] %s" CR), msg); \
      if (error) *error = msg;                      \
    } while (0)

// Checks the downlink rules: {"rx1_frequency":..,"schedules":[{"every":s,"fport":p,"hex":".."}],"triggers":[{"when":"uplink"|{..},"fport":p,"hex":"..","cooldown":s}]}
static const char* lorawanCheckRules(JsonObject rules) {
  uint8_t bytes[LORAWAN_MAX_DOWNLINK_SIZE];
  size_t len;
  for (const char* list : {"schedules", "triggers"}) {
    if (rules.containsKey(list) && !rules[list].is<JsonArray>()) return "schedules and triggers must be arrays";
    for (JsonObject rule : rules[list].as<JsonArray>()) {
      int fport = rule["fport"] | 0;
      if (fport < 1 || fport > 223) return "each schedule/trigger needs an fport from 1 to 223";
      if (!lorawanParseHex(String(rule["hex"] | ""), bytes, LORAWAN_MAX_DOWNLINK_SIZE, len)) return "each schedule/trigger needs a hex payload";
      if (strcmp(list, "schedules") == 0 && (rule["every"] | 0) <= 0) return "each schedule needs an interval in seconds (every)";
      if (strcmp(list, "triggers") == 0 && !rule.containsKey("when")) return "each trigger needs a condition (when)";
    }
  }
  return nullptr;
}

// Applies one device declaration, returns false (and the reason in error, if given) if invalid
bool LORAWANconfigureDevice(JsonObject& jo, bool save, String* error) {
  uint32_t devAddr;
  if (!lorawanParseDevAddr(jo["devaddr"], devAddr)) {
    LORAWAN_CONFIG_ERROR(error, "invalid or missing devaddr (8 hex digits)");
    return false;
  }
  LoRaWANDevice* dev = lorawanFind(devAddr);
  if (jo["remove"] | false) {
    if (dev) {
      lorawanDevices.erase(lorawanDevices.begin() + (dev - lorawanDevices.data()));
      THEENGS_LOG_NOTICE(F("[LoRaWAN] device %X removed" CR), devAddr);
    }
    if (save) {
      lorawanEraseDevice(devAddr);
      lorawanSaveIndex();
    }
    return true;
  }
  LoRaWANDevice candidate;
  if (dev)
    candidate = *dev;
  else
    candidate.devAddr = devAddr;
  if (jo.containsKey("nwkskey") && !lorawanParseKey(jo["nwkskey"], candidate.nwkSKey)) {
    LORAWAN_CONFIG_ERROR(error, "invalid nwkskey (32 hex digits)");
    return false;
  }
  if (jo.containsKey("appskey") && !lorawanParseKey(jo["appskey"], candidate.appSKey)) {
    LORAWAN_CONFIG_ERROR(error, "invalid appskey (32 hex digits)");
    return false;
  }
  if (!dev && !(jo.containsKey("nwkskey") && jo.containsKey("appskey"))) {
    LORAWAN_CONFIG_ERROR(error, "a new device requires nwkskey and appskey");
    return false;
  }
  if (jo.containsKey("name")) candidate.name = jo["name"].as<String>();
  if (jo.containsKey("model")) candidate.model = jo["model"].as<String>();
  if (jo.containsKey("entities")) {
    candidate.entities = "";
    if (jo["entities"].is<JsonArray>() && jo["entities"].size() > 0) serializeJson(jo["entities"], candidate.entities);
    lorawanDiscoveryPending = true;
  }
  if (jo.containsKey("downlinks")) {
    candidate.downlinks = "";
    if (jo["downlinks"].is<JsonObject>() && jo["downlinks"].size() > 0) {
      const char* problem = lorawanCheckRules(jo["downlinks"].as<JsonObject>());
      if (problem) {
        LORAWAN_CONFIG_ERROR(error, problem);
        return false;
      }
      serializeJson(jo["downlinks"], candidate.downlinks);
    }
    candidate.ruleLast.clear();
  }
  if (dev) {
    *dev = candidate;
  } else {
    if (lorawanDevices.size() >= LORAWAN_MAX_DEVICES) {
      LORAWAN_CONFIG_ERROR(error, "maximum number of devices reached (LORAWAN_MAX_DEVICES)");
      return false;
    }
    lorawanDevices.push_back(candidate);
  }
  THEENGS_LOG_NOTICE(F("[LoRaWAN] device %X configured" CR), devAddr);
  if (save) {
    lorawanSaveDevice(candidate);
    lorawanSaveIndex();
  }
  // Commands: {"clear_queue":true}, {"send":{"fport":1,"hex":"0100003C"}} (or an array)
  if (jo["clear_queue"] | false) LORAWANclearQueue(devAddr);
  if (jo.containsKey("send")) {
    JsonArray sends = jo["send"].is<JsonArray>() ? jo["send"].as<JsonArray>() : JsonArray();
    bool ok = true;
    if (sends.isNull()) {
      ok = LORAWANqueueDownlink(devAddr, jo["send"]["fport"] | 0, String(jo["send"]["hex"] | ""), "mqtt", error);
    } else {
      for (JsonObject send : sends) ok = LORAWANqueueDownlink(devAddr, send["fport"] | 0, String(send["hex"] | ""), "mqtt", error) && ok;
    }
    if (!ok) return false;
  }
  return true;
}

// Handles the "lorawan" key of the LORA config command: an object or an array of device declarations
void LORAWANConfig_fromJson(JsonObject& LORAdata) {
  if (!LORAdata.containsKey("lorawan")) return;
  bool save = LORAdata["save"] | false;
  if (LORAdata["lorawan"].is<JsonArray>()) {
    for (JsonObject jo : LORAdata["lorawan"].as<JsonArray>()) LORAWANconfigureDevice(jo, save, nullptr);
  } else if (LORAdata["lorawan"].is<JsonObject>()) {
    JsonObject jo = LORAdata["lorawan"].as<JsonObject>();
    LORAWANconfigureDevice(jo, save, nullptr);
  }
}

// Adds the configured devices (without keys) to the LORA state message
void LORAWANState(JsonObject& LORAdata) {
  JsonArray devices = LORAdata.createNestedArray("lorawan");
  for (auto& dev : lorawanDevices) {
    JsonObject jo = devices.createNestedObject();
    lorawanToJson(dev, jo, false);
  }
}

void LORAWANsetup() {
  lorawanDevices.clear();
  preferences.begin(Gateway_Short_Name, true);
  String index = preferences.isKey(LORAWAN_NVS_INDEX) ? preferences.getString(LORAWAN_NVS_INDEX, "") : "";
  preferences.end();
  int start = 0;
  while (start < (int)index.length()) {
    int end = index.indexOf(',', start);
    if (end < 0) end = index.length();
    String hex = index.substring(start, end);
    start = end + 1;
    uint32_t devAddr;
    if (!lorawanParseDevAddr(hex.c_str(), devAddr)) continue;
    char key[11];
    lorawanNvsKey(key, "lw", devAddr);
    preferences.begin(Gateway_Short_Name, true);
    String conf = preferences.isKey(key) ? preferences.getString(key, "") : "";
    preferences.end();
    DynamicJsonDocument doc(LORAWAN_DEVICE_JSON_SIZE);
    if (conf.length() == 0 || deserializeJson(doc, conf)) {
      THEENGS_LOG_ERROR(F("[LoRaWAN] cannot load device %X" CR), devAddr);
      continue;
    }
    JsonObject jo = doc.as<JsonObject>();
    if (!LORAWANconfigureDevice(jo, false, nullptr)) continue;
    lorawanNvsKey(key, "lc", devAddr);
    preferences.begin(Gateway_Short_Name, true);
    if (preferences.isKey(key)) lorawanFind(devAddr)->fcntDown = preferences.getUInt(key, 0);
    preferences.end();
#  ifdef LORA_LORAWAN_JS
    lorawanNvsKey(key, "lj", devAddr);
    preferences.begin(Gateway_Short_Name, true);
    if (preferences.isKey(key)) lorawanFind(devAddr)->decoder = preferences.getString(key, "");
    preferences.end();
#  endif
  }
  THEENGS_LOG_NOTICE(F("[LoRaWAN] %d device(s) loaded" CR), lorawanDevices.size());
  pinMode(LORA_DI0, INPUT);
  attachInterrupt(digitalPinToInterrupt(LORA_DI0), lorawanRxDoneIsr, RISING); // DIO0 is RxDone in receive mode
  lorawanDiscoveryPending = true;
}

#  ifdef LORA_LORAWAN_JS
// Sets (and stores) the decoder of a configured device, an empty script removes it
bool LORAWANsetDecoder(uint32_t devAddr, const String& script) {
  LoRaWANDevice* dev = lorawanFind(devAddr);
  if (!dev) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] decoder received for unknown device %X, configure it first" CR), devAddr);
    return false;
  }
  if (script.length() > LORAWAN_DECODER_MAX_SIZE) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] decoder for %X exceeds %d bytes" CR), devAddr, LORAWAN_DECODER_MAX_SIZE);
    return false;
  }
  dev->decoder = script;
  char key[11];
  lorawanNvsKey(key, "lj", devAddr);
  preferences.begin(Gateway_Short_Name, false);
  if (dev->decoder.length()) {
    size_t result = preferences.putString(key, dev->decoder);
    THEENGS_LOG_NOTICE(F("[LoRaWAN] decoder for %X saved (%d bytes), result: %d" CR), devAddr, dev->decoder.length(), result);
  } else if (preferences.isKey(key)) {
    preferences.remove(key);
    THEENGS_LOG_NOTICE(F("[LoRaWAN] decoder for %X removed" CR), devAddr);
  }
  preferences.end();
  return true;
}

// Decoder scripts are received raw (not JSON) on <base><gateway>/LORAWANdecoder/<DEVADDR>; an empty payload removes the decoder
void LORAWANdecoderFromMQTT(const char* topic, const char* payload) {
  const char* slash = strrchr(topic, '/');
  uint32_t devAddr;
  if (!slash || !lorawanParseDevAddr(slash + 1, devAddr)) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] decoder topic must end with the 8 hex digits DevAddr: %s" CR), topic);
    return;
  }
  LORAWANsetDecoder(devAddr, payload);
}
#  endif

/*-------------------- Home Assistant discovery --------------------*/

#  ifdef ZmqttDiscovery
// Entities are declared per device, e.g.
// [{"key":"door","type":"binary_sensor","class":"door","on":"open","off":"closed"},{"key":"volt","class":"voltage","unit":"V"}]
// Must be called from the main loop as it publishes directly
void launchLORAWANDiscovery(bool overrideDiscovery) {
#    ifdef LORA_LORAWAN_JS
  if (overrideDiscovery) lorawanStartNtp(); // called on MQTT connection: the network is up
#    endif
  if (!overrideDiscovery && !lorawanDiscoveryPending) return;
  lorawanDiscoveryPending = false;
  for (auto& dev : lorawanDevices) {
    if (!dev.entities.length()) continue;
    DynamicJsonDocument doc(LORAWAN_DEVICE_JSON_SIZE);
    if (deserializeJson(doc, dev.entities)) {
      THEENGS_LOG_ERROR(F("[LoRaWAN] invalid entities for %X" CR), dev.devAddr);
      continue;
    }
    char id[9];
    snprintf(id, sizeof(id), "%08" PRIX32, dev.devAddr);
    String topic = String(subjectLORAtoMQTT) + "/" + id;
    const char* deviceName = dev.name.length() ? dev.name.c_str() : id;
    for (JsonObject e : doc.as<JsonArray>()) {
      const char* key = e["key"];
      if (!key) continue;
      const char* type = e["type"] | "sensor";
      bool binary = strcmp(type, "binary_sensor") == 0;
      String uniqueId = String(id) + "-" + key;
      String valueTemplate = String("{{ value_json.") + key + " | is_defined }}";
      createDiscovery(type,
                      (char*)topic.c_str(), e["name"] | key, (char*)uniqueId.c_str(),
                      "", e["class"] | "", (char*)valueTemplate.c_str(),
                      e["on"] | (binary ? "true" : ""), e["off"] | (binary ? "false" : ""), e["unit"] | "",
                      0, "", "", false, "",
                      deviceName, "", dev.model.c_str(), id, false,
                      binary ? stateClassNone : (e["state_class"] | stateClassMeasurement));
    }
  }
}
#  endif

#endif
