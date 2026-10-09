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
#  include <TheengsUtils.h>

#  include <vector>

#  include "TheengsCommon.h"
#  include "config_LORA.h"
#  include "mbedtls/aes.h"
#  ifdef ZmqttDiscovery
#    include "config_mqttDiscovery.h"
#  endif
#  ifdef LORA_LORAWAN_JS
#    include <duktape.h>
#  endif

struct LoRaWANDevice {
  uint32_t devAddr;
  uint8_t nwkSKey[16];
  uint8_t appSKey[16];
  String name;
  String model;
  String entities; // serialized JSON array of Home Assistant entity declarations
#  ifdef LORA_LORAWAN_JS
  String decoder;
#  endif
  uint32_t lastFcnt;
  bool seen;
};

static std::vector<LoRaWANDevice> lorawanDevices;
static bool lorawanDiscoveryPending = false;

// NVS keys are limited to 15 characters: "lw" + DevAddr for the device, "lj" + DevAddr for its decoder
#  define LORAWAN_NVS_INDEX "lwdevs"

static void lorawanNvsKey(char* key, const char* prefix, uint32_t devAddr) {
  snprintf(key, 11, "%s%08X", prefix, devAddr);
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

// LoRaWAN 1.0.x FRMPayload encryption (AES-CTR like, uplink direction)
static void lorawanDecrypt(const uint8_t* key, uint32_t devAddr, uint32_t fcnt, const uint8_t* in, uint8_t* out, size_t len) {
  uint8_t a[16] = {0x01, 0, 0, 0, 0, 0,
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
static bool lorawanRunDecoder(const LoRaWANDevice& dev, uint8_t fport, const uint8_t* bytes, size_t len, JsonObject& LORAdata) {
  lorawanStartNtp();
  duk_context* ctx = duk_create_heap(NULL, NULL, NULL, NULL, lorawanDuktapeFatal);
  if (!ctx) {
    LORAdata["decoder_error"] = "out of memory";
    return false;
  }
  bool ok = false;
  if (duk_peval_lstring(ctx, dev.decoder.c_str(), dev.decoder.length()) != 0) {
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
  if (!ok) THEENGS_LOG_WARNING(F("[LoRaWAN] decoder error for %X: %s" CR), dev.devAddr, LORAdata["decoder_error"].as<const char*>());
  return ok;
}
#  endif

/*-------------------- Uplink decoding --------------------*/

int LORAWANtoJson(const uint8_t* p, int len, JsonObject& LORAdata) {
  if (len < 12) return LORAWAN_NOT_HANDLED;
  uint8_t mtype = p[0] >> 5;
  if (mtype != 2 && mtype != 4) return LORAWAN_NOT_HANDLED; // (un)confirmed data up
  uint32_t devAddr = p[1] | (p[2] << 8) | (p[3] << 16) | ((uint32_t)p[4] << 24);
  LoRaWANDevice* dev = lorawanFind(devAddr);
  if (!dev) return LORAWAN_NOT_HANDLED;

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
  snprintf(id, sizeof(id), "%08X", devAddr);
  LORAdata["id"] = id;
  if (dev->name.length()) LORAdata["name"] = dev->name;
  if (dev->model.length()) LORAdata["model"] = dev->model;
  LORAdata["fcnt"] = fcnt;
  if (mtype == 4) LORAdata["confirmed"] = true;

  if (msgLen > hdrLen) {
    uint8_t fport = p[hdrLen];
    size_t n = msgLen - hdrLen - 1;
    uint8_t plain[n > 0 ? n : 1];
    lorawanDecrypt(fport == 0 ? dev->nwkSKey : dev->appSKey, devAddr, fcnt, p + hdrLen + 1, plain, n);
    char hex[n * 2 + 2]; // _rawToHex writes "%02X\r" per byte: needs 2 * size + 2
    TheengsUtils::_rawToHex(plain, hex, n);
    hex[n * 2] = 0;
    LORAdata["fport"] = fport;
    LORAdata["payload"] = hex;
#  ifdef LORA_LORAWAN_JS
    if (fport != 0 && dev->decoder.length()) lorawanRunDecoder(*dev, fport, plain, n, LORAdata);
#  endif
  }
  return LORAWAN_DECODED;
}

/*-------------------- Configuration --------------------*/

static void lorawanToJson(const LoRaWANDevice& dev, JsonObject& jo, bool withKeys) {
  char hex[34]; // _rawToHex writes "%02X\r" per byte: needs 2 * size + 2
  snprintf(hex, 9, "%08X", dev.devAddr);
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
#  ifdef LORA_LORAWAN_JS
  if (!withKeys) jo["decoder"] = dev.decoder.length() > 0;
#  endif
  if (!withKeys && dev.seen) jo["fcnt"] = dev.lastFcnt;
}

static void lorawanSaveIndex() {
  String index;
  for (auto& dev : lorawanDevices) {
    char hex[9];
    snprintf(hex, sizeof(hex), "%08X", dev.devAddr);
    if (index.length()) index += ",";
    index += hex;
  }
  preferences.begin(Gateway_Short_Name, false);
  preferences.putString(LORAWAN_NVS_INDEX, index);
  preferences.end();
}

static void lorawanSaveDevice(const LoRaWANDevice& dev) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
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
  preferences.remove(key);
  preferences.end();
}

// Applies one device declaration, returns false if invalid
static bool lorawanDeviceFromJson(JsonObject& jo, bool save) {
  uint32_t devAddr;
  if (!lorawanParseDevAddr(jo["devaddr"], devAddr)) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] invalid or missing devaddr (8 hex digits)" CR));
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
  LoRaWANDevice candidate = dev ? *dev : LoRaWANDevice{devAddr, {0}, {0}, "", "", "",
#  ifdef LORA_LORAWAN_JS
                                                       "",
#  endif
                                                       0,
                                                       false};
  if (jo.containsKey("nwkskey") && !lorawanParseKey(jo["nwkskey"], candidate.nwkSKey)) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] invalid nwkskey (32 hex digits)" CR));
    return false;
  }
  if (jo.containsKey("appskey") && !lorawanParseKey(jo["appskey"], candidate.appSKey)) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] invalid appskey (32 hex digits)" CR));
    return false;
  }
  if (!dev && !(jo.containsKey("nwkskey") && jo.containsKey("appskey"))) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] new device %X requires nwkskey and appskey" CR), devAddr);
    return false;
  }
  if (jo.containsKey("name")) candidate.name = jo["name"].as<String>();
  if (jo.containsKey("model")) candidate.model = jo["model"].as<String>();
  if (jo.containsKey("entities")) {
    candidate.entities = "";
    if (jo["entities"].is<JsonArray>()) serializeJson(jo["entities"], candidate.entities);
    lorawanDiscoveryPending = true;
  }
  if (dev) {
    *dev = candidate;
  } else {
    if (lorawanDevices.size() >= LORAWAN_MAX_DEVICES) {
      THEENGS_LOG_ERROR(F("[LoRaWAN] maximum number of devices (%d) reached" CR), LORAWAN_MAX_DEVICES);
      return false;
    }
    lorawanDevices.push_back(candidate);
  }
  THEENGS_LOG_NOTICE(F("[LoRaWAN] device %X configured" CR), devAddr);
  if (save) {
    lorawanSaveDevice(candidate);
    lorawanSaveIndex();
  }
  return true;
}

// Handles the "lorawan" key of the LORA config command: an object or an array of device declarations
void LORAWANConfig_fromJson(JsonObject& LORAdata) {
  if (!LORAdata.containsKey("lorawan")) return;
  bool save = LORAdata["save"] | false;
  if (LORAdata["lorawan"].is<JsonArray>()) {
    for (JsonObject jo : LORAdata["lorawan"].as<JsonArray>()) lorawanDeviceFromJson(jo, save);
  } else if (LORAdata["lorawan"].is<JsonObject>()) {
    JsonObject jo = LORAdata["lorawan"].as<JsonObject>();
    lorawanDeviceFromJson(jo, save);
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
    StaticJsonDocument<JSON_MSG_BUFFER> doc;
    if (conf.length() == 0 || deserializeJson(doc, conf)) {
      THEENGS_LOG_ERROR(F("[LoRaWAN] cannot load device %X" CR), devAddr);
      continue;
    }
    JsonObject jo = doc.as<JsonObject>();
    if (!lorawanDeviceFromJson(jo, false)) continue;
#  ifdef LORA_LORAWAN_JS
    lorawanNvsKey(key, "lj", devAddr);
    preferences.begin(Gateway_Short_Name, true);
    if (preferences.isKey(key)) lorawanFind(devAddr)->decoder = preferences.getString(key, "");
    preferences.end();
#  endif
  }
  THEENGS_LOG_NOTICE(F("[LoRaWAN] %d device(s) loaded" CR), lorawanDevices.size());
  lorawanDiscoveryPending = true;
}

#  ifdef LORA_LORAWAN_JS
// Decoder scripts are received raw (not JSON) on <base><gateway>/LORAWANdecoder/<DEVADDR>; an empty payload removes the decoder
void LORAWANdecoderFromMQTT(const char* topic, const char* payload) {
  const char* slash = strrchr(topic, '/');
  uint32_t devAddr;
  if (!slash || !lorawanParseDevAddr(slash + 1, devAddr)) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] decoder topic must end with the 8 hex digits DevAddr: %s" CR), topic);
    return;
  }
  LoRaWANDevice* dev = lorawanFind(devAddr);
  if (!dev) {
    THEENGS_LOG_ERROR(F("[LoRaWAN] decoder received for unknown device %X, configure it first" CR), devAddr);
    return;
  }
  dev->decoder = payload;
  char key[11];
  lorawanNvsKey(key, "lj", devAddr);
  preferences.begin(Gateway_Short_Name, false);
  if (dev->decoder.length()) {
    size_t result = preferences.putString(key, dev->decoder);
    THEENGS_LOG_NOTICE(F("[LoRaWAN] decoder for %X saved (%d bytes), result: %d" CR), devAddr, dev->decoder.length(), result);
  } else {
    preferences.remove(key);
    THEENGS_LOG_NOTICE(F("[LoRaWAN] decoder for %X removed" CR), devAddr);
  }
  preferences.end();
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
    StaticJsonDocument<JSON_MSG_BUFFER> doc;
    if (deserializeJson(doc, dev.entities)) {
      THEENGS_LOG_ERROR(F("[LoRaWAN] invalid entities for %X" CR), dev.devAddr);
      continue;
    }
    char id[9];
    snprintf(id, sizeof(id), "%08X", dev.devAddr);
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
