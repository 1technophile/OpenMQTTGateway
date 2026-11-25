/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between Dimplex Connex electric heaters and an MQTT broker.

   The Connex wall controller talks to its heaters over a plain nRF24L01
   Enhanced ShockBurst link. This gateway:
 - listens passively (no ACK) for wall controller frames and publishes each zone setpoint
 - learns the 2-byte controller ID from the first valid frame while in learn mode
 - sends zone setpoints received over MQTT, as a burst of frames like the wall controller
 - announces one Home Assistant climate entity per zone

   Protocol notes: OpenNRF24L01 Docs/connex.md

   Frame (10 bytes): [id0, id1, seq, 0x01, zone 0-3, setpoint x10 high, low, 0x09, 0x01, 0x00]

   The heaters never transmit (they only send the radio's auto-ack), so the gateway reports the
   last setpoint it sent or heard per zone, with a timestamp. It cannot see changes made on a heater.

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

#ifdef ZgatewayCONNEX
#  include <RF24.h>
#  include <SPI.h>
#  include <math.h>

#  include "TheengsCommon.h"
#  include "config_CONNEX.h"
#  ifdef ZmqttDiscovery
#    include "config_mqttDiscovery.h"
#  endif
#  ifdef ZsensorBME280
#    include "config_BME280.h"
#  endif

static RF24 radio(CONNEX_CE_GPIO, CONNEX_CSN_GPIO);

struct CONNEXConfig_s {
  bool idValid;
  uint8_t controllerId[2];
  uint16_t burstCount;
  uint16_t burstIntervalMs;
  uint16_t burstAcks; // stop the burst after this many acks, 0 = full burst
  uint8_t paLevel; // 0-3, RF24_PA_MIN..RF24_PA_MAX
};
static CONNEXConfig_s CONNEXConfig;

static bool radioOk = false;
static unsigned long radioRetryMs = 0; // last radio init attempt while the radio is down
#  define CONNEX_RADIO_RETRY_MS 30000UL
static bool learning = false;
static unsigned long learnStartMs = 0;
static uint8_t txSeq = 0;

// Last known setpoint per zone: acked by a heater, or heard from the wall controller.
// The setpoint and its source are saved to NVS so they survive a reboot (marked "restored"
// until the zone changes again). A burst nobody acked leaves the known setpoint untouched
// and is reported separately as failedSetpoint.
struct ZoneLast_s {
  bool valid; // setpoint/source hold a known setpoint
  bool restored; // loaded from NVS at boot
  float setpoint;
  char source[11]; // "gateway" or "controller"
  int16_t sent; // frames of the last gateway burst, -1 when none since boot
  int16_t acked;
  bool ok; // last gateway burst was acked
  float failedSetpoint; // setpoint of the last unacked burst, NAN when the last burst was acked
  unsigned long changedAt; // uptime() in s when setpoint changed, unknown once restored
};
static ZoneLast_s zoneLast[CONNEX_ZONES];
// Setpoint waiting to be transmitted per zone (NAN = nothing pending)
static float pendingSetpoint[CONNEX_ZONES];

// Burst currently being transmitted
static struct {
  bool active;
  uint8_t zone;
  float setpoint;
  uint8_t frame[CONNEX_PAYLOAD_SIZE];
  uint16_t sent;
  uint16_t acked;
  unsigned long lastWriteMs;
} burst;

// Receive de-duplication (the wall controller repeats each frame)
static uint8_t lastRxFrame[CONNEX_PAYLOAD_SIZE];
static unsigned long lastRxMs = 0;

/*-------------------- Helpers --------------------*/

static String controllerIdString() {
  char buf[5];
  snprintf(buf, sizeof(buf), "%02X%02X", CONNEXConfig.controllerId[0], CONNEXConfig.controllerId[1]);
  return String(buf);
}

static bool parseControllerId(const char* s, uint8_t* out) {
  if (!s) return false;
  char hex[5];
  size_t n = 0;
  for (const char* p = s; *p && n < 4; p++) { // accept "D677", "d6:77", "D6 77"
    if (isxdigit((unsigned char)*p)) hex[n++] = *p;
  }
  hex[n] = 0;
  if (n != 4) return false;
  unsigned long v = strtoul(hex, nullptr, 16);
  out[0] = (v >> 8) & 0xFF;
  out[1] = v & 0xFF;
  return true;
}

static bool isValidConnexFrame(const uint8_t* b) {
  uint16_t tenths = ((uint16_t)b[5] << 8) | b[6];
  return b[3] == 0x01 && b[4] < CONNEX_ZONES && tenths <= lroundf(CONNEX_MAX_TEMP * 10.0f) &&
         b[7] == 0x09 && b[8] == 0x01 && b[9] == 0x00;
}

// Round to the nearest CONNEX_TEMP_STEP; the result in tenths goes in frame bytes 5 (high) and 6 (low)
static bool encodeSetpoint(float tempC, uint16_t* out) {
  if (isnan(tempC) || tempC < CONNEX_MIN_TEMP || tempC > CONNEX_MAX_TEMP) return false;
  long v = lroundf(roundf(tempC / CONNEX_TEMP_STEP) * CONNEX_TEMP_STEP * 10.0f);
  if (v < lroundf(CONNEX_MIN_TEMP * 10.0f) || v < 0 || v > lroundf(CONNEX_MAX_TEMP * 10.0f)) return false;
  *out = (uint16_t)v;
  return true;
}

// Zone number from a JSON number or a numeric string ("1"); 0 when missing or invalid
static int parseZone(JsonVariant v) {
  if (v.is<int>()) return v.as<int>();
  const char* s = v.as<const char*>();
  if (!s || !*s) return 0;
  char* end;
  long n = strtol(s, &end, 10);
  return *end == 0 ? (int)n : 0;
}

// Temperature from a JSON number or a numeric string ("20.5"); NAN for null, booleans,
// empty or non-numeric strings, so they can't be read as 0 C
static float parseTemperature(JsonVariant v) {
  if (v.is<bool>()) return NAN;
  if (v.is<float>()) return v.as<float>();
  const char* s = v.as<const char*>();
  if (!s || !*s) return NAN;
  char* end;
  float t = strtof(s, &end);
  return *end == 0 ? t : NAN;
}

static const char* const paLevelNames[] = {"min", "low", "high", "max"};

// Accept 0-3 or "min" / "low" / "high" / "max"
static bool parsePaLevel(JsonVariant v, uint8_t* out) {
  if (v.is<int>()) {
    int n = v.as<int>();
    if (n < 0 || n > 3) return false;
    *out = n;
    return true;
  }
  const char* s = v.as<const char*>();
  if (!s) return false;
  for (uint8_t i = 0; i < 4; i++) {
    if (strcasecmp(s, paLevelNames[i]) == 0) {
      *out = i;
      return true;
    }
  }
  return false;
}

static void startListening() {
  if (!radioOk) return;
  radio.startListening();
}

/*-------------------- Last setpoint per zone --------------------*/

#  ifdef ZmqttDiscovery
static void updateZoneDiscovery(uint8_t z);
#  endif

static void zoneLastSave() {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject jo = doc.to<JsonObject>();
  for (uint8_t i = 0; i < CONNEX_ZONES; i++) {
    const ZoneLast_s& z = zoneLast[i];
    if (!z.valid) continue;
    char key[2] = {(char)('1' + i), 0};
    JsonObject zj = jo.createNestedObject(key);
    zj["sp"] = z.setpoint;
    zj["src"] = z.source;
  }
  String conf;
  serializeJson(doc, conf);
  preferences.begin(Gateway_Short_Name, false);
  preferences.putString("CONNEXZones", conf);
  preferences.end();
}

static void zoneLastLoad() {
  preferences.begin(Gateway_Short_Name, true);
  String conf = preferences.isKey("CONNEXZones") ? preferences.getString("CONNEXZones", "{}") : String("{}");
  preferences.end();
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  if (deserializeJson(doc, conf)) return;
  for (uint8_t i = 0; i < CONNEX_ZONES; i++) {
    char key[2] = {(char)('1' + i), 0};
    JsonObject zj = doc[key];
    if (zj.isNull()) continue;
    ZoneLast_s& z = zoneLast[i];
    z.valid = true;
    z.restored = true;
    z.setpoint = zj["sp"] | NAN;
    strlcpy(z.source, zj["src"] | "gateway", sizeof(z.source));
    z.changedAt = 0; // uptime restarted, so when it changed is unknown
  }
  THEENGS_LOG_NOTICE(F("[CONNEX] last setpoints restored: %s" CR), conf.c_str());
}

static void clearZone(uint8_t z) {
  zoneLast[z] = ZoneLast_s{};
  zoneLast[z].setpoint = NAN;
  zoneLast[z].failedSetpoint = NAN;
  zoneLast[z].sent = -1;
}

// A known setpoint: acked by a heater or heard from the wall controller. NVS is only
// written when the setpoint or its source changes, not for repeats of the same value.
static void recordSetpoint(uint8_t zone, const char* source, float setpoint) {
  ZoneLast_s& z = zoneLast[zone];
  bool firstUse = !z.valid;
  // leaving the restored state counts as a change, so age restarts from a known time
  bool changed = firstUse || z.restored || z.setpoint != setpoint || strcmp(z.source, source) != 0;
  z.valid = true;
  z.restored = false;
  z.setpoint = setpoint;
  strlcpy(z.source, source, sizeof(z.source));
  if (changed) {
    z.changedAt = uptime();
    zoneLastSave();
  }
#  ifdef ZmqttDiscovery
  if (firstUse) updateZoneDiscovery(zone); // a zone gets its climate entity once it is used
#  endif
}

// Outcome of a gateway burst. Only an acked burst changes the known setpoint.
static void recordBurst(uint8_t zone, float setpoint, int sent, int acked) {
  ZoneLast_s& z = zoneLast[zone];
  z.sent = sent;
  z.acked = acked;
  z.ok = acked > 0;
  if (z.ok) {
    z.failedSetpoint = NAN;
    recordSetpoint(zone, "gateway", setpoint);
  } else {
    z.failedSetpoint = setpoint; // the heater keeps its previous setpoint
  }
}

static bool zoneHasState(uint8_t z) {
  return zoneLast[z].valid || zoneLast[z].sent >= 0;
}

// Fill a JSON object with a zone's state: known setpoint, source, age (seconds since it
// changed, null when unknown after a reboot) and restored; then the last gateway burst
// (sent/acked/status) and failed_setpoint when that burst got no ack.
static void addZoneLast(JsonObject jo, uint8_t zone) {
  const ZoneLast_s& z = zoneLast[zone];
  if (z.valid) {
    if (!isnan(z.setpoint)) jo["setpoint"] = round(z.setpoint * 10.0f) / 10.0f;
    jo["source"] = z.source;
    if (z.restored) {
      jo["age"] = nullptr; // set before the last reboot, when is unknown
      jo["restored"] = true;
    } else {
      jo["age"] = uptime() - z.changedAt;
    }
  }
  if (z.sent >= 0) {
    jo["sent"] = z.sent;
    jo["acked"] = z.acked;
    jo["status"] = z.ok ? "ok" : "failed";
    if (!isnan(z.failedSetpoint)) jo["failed_setpoint"] = round(z.failedSetpoint * 10.0f) / 10.0f;
  }
}

/*-------------------- Config --------------------*/

static void CONNEXConfig_init() {
  CONNEXConfig.idValid = false;
  CONNEXConfig.controllerId[0] = 0;
  CONNEXConfig.controllerId[1] = 0;
  CONNEXConfig.burstCount = CONNEX_BURST_COUNT;
  CONNEXConfig.burstIntervalMs = CONNEX_BURST_INTERVAL_MS;
  CONNEXConfig.burstAcks = CONNEX_BURST_ACKS;
  CONNEXConfig.paLevel = CONNEX_PA_LEVEL <= 3 ? CONNEX_PA_LEVEL : 1;
}

static void CONNEXConfig_save() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jo = jsonBuffer.to<JsonObject>();
  if (CONNEXConfig.idValid) jo["controllerid"] = controllerIdString();
  jo["burst"] = CONNEXConfig.burstCount;
  jo["interval"] = CONNEXConfig.burstIntervalMs;
  jo["acks"] = CONNEXConfig.burstAcks;
  jo["power"] = paLevelNames[CONNEXConfig.paLevel];
  String conf;
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  int result = preferences.putString("CONNEXConfig", conf);
  preferences.end();
  THEENGS_LOG_NOTICE(F("[CONNEX] config saved: %s, result: %d" CR), conf.c_str(), result);
}

static void CONNEXConfig_fromJson(JsonObject& data) {
  if (data.containsKey("controllerid")) {
    uint8_t id[2];
    if (parseControllerId(data["controllerid"].as<const char*>(), id)) {
      CONNEXConfig.controllerId[0] = id[0];
      CONNEXConfig.controllerId[1] = id[1];
      CONNEXConfig.idValid = true;
    } else {
      THEENGS_LOG_ERROR(F("[CONNEX] invalid controllerid, expected 4 hex digits" CR));
    }
  }
  if (data.containsKey("burst")) {
    int v = data["burst"];
    if (v >= 1 && v <= 1000) CONNEXConfig.burstCount = v;
  }
  if (data.containsKey("interval")) {
    int v = data["interval"];
    if (v >= 1 && v <= 100) CONNEXConfig.burstIntervalMs = v;
  }
  if (data.containsKey("acks")) {
    int v = data["acks"];
    if (v >= 0 && v <= 1000) CONNEXConfig.burstAcks = v;
  }
  if (data.containsKey("power")) {
    uint8_t level;
    if (parsePaLevel(data["power"], &level)) {
      CONNEXConfig.paLevel = level;
      if (radioOk) radio.setPALevel(level, true);
    } else {
      THEENGS_LOG_ERROR(F("[CONNEX] invalid power, expected min, low, high, max or 0-3" CR));
    }
  }
}

static void CONNEXConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  String conf = preferences.isKey("CONNEXConfig") ? preferences.getString("CONNEXConfig", "{}") : String("{}");
  preferences.end();
  if (deserializeJson(jsonBuffer, conf)) {
    THEENGS_LOG_ERROR(F("[CONNEX] stored config unreadable, using defaults" CR));
    return;
  }
  JsonObject jo = jsonBuffer.as<JsonObject>();
  CONNEXConfig_fromJson(jo);
  THEENGS_LOG_NOTICE(F("[CONNEX] config loaded: %s" CR), conf.c_str());
}

/*-------------------- Publishing --------------------*/

// Gateway state. With an error (rejected command) the message is not retained, so the
// retained copy Home Assistant reads at start-up never carries a stale error.
static void publishState(const char* error = nullptr) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject jo = doc.to<JsonObject>();
  jo["radio"] = radioOk ? "ok" : "not responding";
  jo["controllerid"] = CONNEXConfig.idValid ? controllerIdString() : String("");
  jo["learning"] = learning;
  jo["burst"] = CONNEXConfig.burstCount;
  jo["interval"] = CONNEXConfig.burstIntervalMs;
  jo["acks"] = CONNEXConfig.burstAcks;
  jo["power"] = paLevelNames[CONNEXConfig.paLevel];
  // Last setpoint sent or heard per zone (the heaters never report their own setting)
  JsonObject zones = jo.createNestedObject("zones");
  for (uint8_t z = 0; z < CONNEX_ZONES; z++) {
    if (!zoneHasState(z)) continue;
    char key[2] = {(char)('1' + z), 0};
    addZoneLast(zones.createNestedObject(key), z);
  }
  if (error) jo["error"] = error;
  if (doc.overflowed()) THEENGS_LOG_ERROR(F("[CONNEX] state JSON overflowed" CR));
  jo["origin"] = subjectGTWCONNEXtoMQTT;
  jo["retain"] = error == nullptr; // holds the zone setpoints Home Assistant reads at start-up
  enqueueJsonObject(doc);
}

void stateCONNEXMeasures() {
  publishState();
}

// Log and publish a rejected command
static void rejectCommand(const char* error) {
  THEENGS_LOG_ERROR(F("[CONNEX] %s" CR), error);
  publishState(error);
}

/*-------------------- Discovery --------------------*/

#  ifdef ZmqttDiscovery
// Announce one zone as a Home Assistant climate entity. Kept small: the publish queue
// re-parses each message into JSON_MSG_BUFFER_MAX bytes, and the gateway's other entities
// already describe the device (model, version, URL), so only its ids are repeated here.
static void announceClimateZone(uint8_t z) {
  // enqueueJsonObject() serializes straight away, so local Strings outlive the document
  String base = String(mqtt_topic) + String(gateway_name);
  String cmdTopic = base + subjectMQTTtoCONNEX;
  String stateTopic = base + subjectGTWCONNEXtoMQTT;
  String availTopic = base + will_Topic;
  String uniqueId = getUniqueId("connex-zone", String(z + 1));
  String name = String("CONNEX: Zone ") + String(z + 1);
  String discTopic = String(discovery_prefix) + "/climate/" + uniqueId + "/config";
  String cmdTpl = String("{\"zone\":") + String(z + 1) + ",\"temperature\":{{ value }}}";
  String statTpl = String("{{ value_json.zones.get('") + String(z + 1) + "', {}).get('setpoint') }}";
  String mac = getMacAddress();
#    ifdef ZsensorBME280
  String currTempTopic = base + BMETOPIC;
#    endif

  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject sensor = doc.to<JsonObject>();
  sensor["name"] = name.c_str();
  sensor["uniq_id"] = uniqueId.c_str();
  sensor["temp_cmd_t"] = cmdTopic.c_str();
  sensor["temp_cmd_tpl"] = cmdTpl.c_str();
  sensor["temp_stat_t"] = stateTopic.c_str();
  sensor["temp_stat_tpl"] = statTpl.c_str();
  sensor["mode_stat_t"] = stateTopic.c_str();
  sensor["mode_stat_tpl"] = "heat";
  JsonArray modesArr = sensor.createNestedArray("modes");
  modesArr.add("heat");
#    ifdef ZsensorBME280
  sensor["curr_temp_t"] = currTempTopic.c_str();
  sensor["curr_temp_tpl"] = "{{ value_json.tempc }}";
#    endif
  sensor["min_temp"] = CONNEX_MIN_TEMP;
  sensor["max_temp"] = CONNEX_MAX_TEMP;
  sensor["temp_step"] = CONNEX_TEMP_STEP;
  sensor["avty_t"] = availTopic.c_str(); // payloads "online" / "offline" are the defaults
  JsonArray ids = sensor.createNestedObject("dev").createNestedArray("ids");
  ids.add(mac.c_str());
  sensor["topic"] = discTopic.c_str();
  sensor["retain"] = true;
  enqueueJsonObject(doc);
}

static void removeClimateZone(uint8_t z) {
  if (!SYSConfig.discovery) return;
  eraseTopic("climate", getUniqueId("connex-zone", String(z + 1)).c_str());
}

// Announce a zone that has a known setpoint (paired, commanded or heard from the wall
// controller) while a controller ID is set; otherwise remove its entity
static void updateZoneDiscovery(uint8_t z) {
  if (!SYSConfig.discovery) return;
  if (CONNEXConfig.idValid && zoneLast[z].valid) {
    announceClimateZone(z);
  } else {
    removeClimateZone(z);
  }
}
#  endif

void launchCONNEXDiscovery() {
#  ifdef ZmqttDiscovery
  if (!SYSConfig.discovery) return;
  THEENGS_LOG_TRACE(F("[CONNEX] discovery" CR));

  const char* gatewayCONNEX[][13] = {
      {HASS_TYPE_SENSOR, "CONNEX: Controller ID", "connexid", "", "{{ value_json.controllerid }}", "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_BUTTON, "CONNEX: Learn controller", "connexlearn", "", "", "{\"learn\":true}", "", "", stateClassNone, nullptr, nullptr, nullptr, subjectMQTTtoCONNEXset}};
  createDiscoveryFromList(nullptr, gatewayCONNEX, 2, nullptr, nullptr, nullptr,
                          true, subjectGTWCONNEXtoMQTT, will_Topic, nullptr);

  if (!CONNEXConfig.idValid) {
    THEENGS_LOG_NOTICE(F("[CONNEX] no controller ID yet, zone climate entities not announced" CR));
    return;
  }
  for (uint8_t z = 0; z < CONNEX_ZONES; z++) updateZoneDiscovery(z);
#  endif
}

/*-------------------- Setup / loop --------------------*/

// Configure the nRF24L01; false when it doesn't answer (it is retried from the loop)
static bool radioInit() {
  radioRetryMs = millis();
  if (!radio.begin(&SPI) || !radio.isChipConnected()) {
    radioOk = false;
    THEENGS_LOG_ERROR(F("[CONNEX] nRF24L01 not responding, check wiring (CE=%d CSN=%d), retrying every %lus" CR),
                      CONNEX_CE_GPIO, CONNEX_CSN_GPIO, CONNEX_RADIO_RETRY_MS / 1000);
    return false;
  }
  radio.setChannel(CONNEX_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(CONNEXConfig.paLevel, true); // second arg enables the LNA
  radio.setCRCLength(RF24_CRC_16);
  radio.disableDynamicPayloads();
  radio.setPayloadSize(CONNEX_PAYLOAD_SIZE);
  radio.setAutoAck(true); // needed for heater ACKs on our transmissions (pipe 0)
  radio.setRetries(5, 15); // 1500 us x 15, as the wall controller
  radio.openWritingPipe(CONNEX_ADDRESS);
  radio.openReadingPipe(1, CONNEX_ADDRESS);
  radio.setAutoAck(1, false); // listen passively: never ACK the wall controller ourselves
  radio.flush_tx();
  radio.flush_rx();
  radioOk = true;
  startListening();

  THEENGS_LOG_NOTICE(F("[CONNEX] radio ready: CH%d, 250kbps, CRC16, controller %s" CR), CONNEX_CHANNEL,
                     CONNEXConfig.idValid ? controllerIdString().c_str() : "not learned");
  return true;
}

void setupCONNEX() {
  for (uint8_t z = 0; z < CONNEX_ZONES; z++) {
    clearZone(z);
    pendingSetpoint[z] = NAN;
  }
  burst.active = false;

  CONNEXConfig_init();
  CONNEXConfig_load();
  zoneLastLoad();

  SPI.begin(CONNEX_SCK_GPIO, CONNEX_MISO_GPIO, CONNEX_MOSI_GPIO, CONNEX_CSN_GPIO);
  radioInit();
}

// Stop the burst in progress and drop queued setpoints and learn mode, so nothing queued
// under the old controller ID is sent after erase / init
static void cancelRadioWork() {
  if (burst.active) {
    burst.active = false;
    if (radioOk) {
      radio.flush_tx();
      radio.flush_rx();
      startListening();
    }
    THEENGS_LOG_NOTICE(F("[CONNEX] burst for zone %d cancelled" CR), burst.zone + 1);
  }
  for (uint8_t z = 0; z < CONNEX_ZONES; z++) pendingSetpoint[z] = NAN;
  learning = false;
}

static void handleRxFrame(const uint8_t* b) {
  unsigned long now = millis();
  if (now - lastRxMs < CONNEX_RX_DEDUP_MS && memcmp(b, lastRxFrame, CONNEX_PAYLOAD_SIZE) == 0) {
    lastRxMs = now; // a run of repeats is one event, however long it lasts
    return;
  }
  memcpy(lastRxFrame, b, CONNEX_PAYLOAD_SIZE);
  lastRxMs = now;

  THEENGS_LOG_TRACE(F("[CONNEX] frame %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X" CR),
                    b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9]);
  if (!isValidConnexFrame(b)) {
    THEENGS_LOG_TRACE(F("[CONNEX] not a Connex setpoint frame, ignored" CR));
    return;
  }

  if (learning) {
    CONNEXConfig.controllerId[0] = b[0];
    CONNEXConfig.controllerId[1] = b[1];
    CONNEXConfig.idValid = true;
    learning = false;
    CONNEXConfig_save();
    THEENGS_LOG_NOTICE(F("[CONNEX] controller learned: %s" CR), controllerIdString().c_str());
    stateCONNEXMeasures();
    launchCONNEXDiscovery();
  }

  if (!CONNEXConfig.idValid || b[0] != CONNEXConfig.controllerId[0] || b[1] != CONNEXConfig.controllerId[1]) {
    THEENGS_LOG_TRACE(F("[CONNEX] frame from another controller (%02X%02X), ignored" CR), b[0], b[1]);
    return;
  }
  uint8_t zone = b[4];
  float setpoint = (((uint16_t)b[5] << 8) | b[6]) / 10.0f;
  txSeq = b[2] + 1; // keep our sequence ahead of the wall controller's
  THEENGS_LOG_NOTICE(F("[CONNEX] wall controller: zone %d -> %.1f C" CR), zone + 1, setpoint);
  recordSetpoint(zone, "controller", setpoint);
  zoneLast[zone].failedSetpoint = NAN; // the setpoint is known again, drop a stale failure
  publishState();
}

static void startNextBurst() {
  for (uint8_t i = 0; i < CONNEX_ZONES; i++) {
    if (isnan(pendingSetpoint[i])) continue;
    uint16_t v;
    if (!encodeSetpoint(pendingSetpoint[i], &v)) { // validated on entry, but be safe
      pendingSetpoint[i] = NAN;
      continue;
    }
    burst.zone = i;
    burst.setpoint = pendingSetpoint[i];
    pendingSetpoint[i] = NAN;
    uint8_t* p = burst.frame;
    p[0] = CONNEXConfig.controllerId[0];
    p[1] = CONNEXConfig.controllerId[1];
    p[2] = txSeq++;
    p[3] = 0x01;
    p[4] = i;
    p[5] = v >> 8;
    p[6] = v & 0xFF;
    p[7] = 0x09;
    p[8] = 0x01;
    p[9] = 0x00;
    burst.sent = 0;
    burst.acked = 0;
    burst.lastWriteMs = 0;
    burst.active = true;
    radio.stopListening();
    radio.flush_tx();
    THEENGS_LOG_NOTICE(F("[CONNEX] sending zone %d -> %.1f C (%d frames)" CR), i + 1, burst.setpoint, CONNEXConfig.burstCount);
    return;
  }
}

static void serviceBurst() {
  unsigned long now = millis();
  if (burst.sent > 0 && now - burst.lastWriteMs < CONNEXConfig.burstIntervalMs) return;
  burst.lastWriteMs = now;
  if (radio.write(burst.frame, CONNEX_PAYLOAD_SIZE)) burst.acked++;
  burst.sent++;
  bool enoughAcks = CONNEXConfig.burstAcks && burst.acked >= CONNEXConfig.burstAcks;
  if (burst.sent < CONNEXConfig.burstCount && !enoughAcks) return;

  burst.active = false;
  bool ok = burst.acked > 0;
  THEENGS_LOG_NOTICE(F("[CONNEX] zone %d -> %.1f C %s (%d/%d acked)" CR), burst.zone + 1, burst.setpoint,
                     ok ? "OK" : "FAILED", burst.acked, burst.sent);
  recordBurst(burst.zone, burst.setpoint, burst.sent, burst.acked);
  publishState();
  radio.flush_rx(); // drop anything received while we were transmitting
  startListening();
}

void CONNEXtoX() {
  if (!radioOk) {
    if (millis() - radioRetryMs >= CONNEX_RADIO_RETRY_MS && radioInit()) publishState();
    return;
  }

  if (burst.active) {
    serviceBurst();
    return;
  }

  if (learning && millis() - learnStartMs > CONNEX_LEARN_TIMEOUT_MS) {
    learning = false;
    THEENGS_LOG_NOTICE(F("[CONNEX] learn timed out, no controller heard" CR));
    stateCONNEXMeasures();
  }

  uint8_t pipe;
  while (radio.available(&pipe)) {
    uint8_t b[CONNEX_PAYLOAD_SIZE];
    radio.read(b, CONNEX_PAYLOAD_SIZE);
    handleRxFrame(b);
  }

  if (CONNEXConfig.idValid) startNextBurst();
}

/*-------------------- MQTT commands --------------------*/

void XtoCONNEX(const char* topicOri, JsonObject& data) {
  // cmpToMainTopic is a prefix match, so exclude the ...set topic explicitly
  if (cmpToMainTopic(topicOri, subjectMQTTtoCONNEX) && !cmpToMainTopic(topicOri, subjectMQTTtoCONNEXset)) {
    THEENGS_LOG_TRACE(F("[CONNEX] MQTTtoCONNEX json" CR));
    if (!radioOk) {
      rejectCommand("radio not responding, command dropped");
      return;
    }
    if (!CONNEXConfig.idValid) {
      rejectCommand("no controller ID, pair a heater or learn the controller first");
      return;
    }
    int zone = parseZone(data["zone"]);
    float temp = NAN;
    if (data.containsKey("temperature"))
      temp = parseTemperature(data["temperature"]);
    else if (data.containsKey("setpoint"))
      temp = parseTemperature(data["setpoint"]);
    uint16_t v;
    if (zone < 1 || zone > CONNEX_ZONES || !encodeSetpoint(temp, &v)) {
      rejectCommand("invalid command, expected zone 1-4 and a numeric temperature in range");
      return;
    }
    pendingSetpoint[zone - 1] = v / 10.0f; // latest command per zone wins
    return;
  }

  if (cmpToMainTopic(topicOri, subjectMQTTtoCONNEXset)) {
    THEENGS_LOG_TRACE(F("[CONNEX] MQTTtoCONNEXset json" CR));
    bool hadId = CONNEXConfig.idValid;
    bool zonesRemoved = false;
    // init: defaults in RAM (controller ID dropped). erase: also clears NVS and the zones.
    bool doInit = data.containsKey("init") && data["init"].as<bool>();
    bool doErase = data.containsKey("erase") && data["erase"].as<bool>();
    if (doInit || doErase) {
      cancelRadioWork();
      CONNEXConfig_init();
      if (radioOk) radio.setPALevel(CONNEXConfig.paLevel, true);
      if (doErase) {
        preferences.begin(Gateway_Short_Name, false);
        if (preferences.isKey("CONNEXConfig")) preferences.remove("CONNEXConfig");
        if (preferences.isKey("CONNEXZones")) preferences.remove("CONNEXZones");
        preferences.end();
        for (uint8_t z = 0; z < CONNEX_ZONES; z++) clearZone(z);
      }
#  ifdef ZmqttDiscovery
      // without a controller ID the zones can't be commanded, so their entities go
      for (uint8_t z = 0; z < CONNEX_ZONES; z++) removeClimateZone(z);
      zonesRemoved = true;
#  endif
      THEENGS_LOG_NOTICE(F("[CONNEX] %s" CR), doErase ? "stored config and zones erased" : "config reset to defaults");
    }
    if (data.containsKey("load") && data["load"].as<bool>()) {
      CONNEXConfig_load();
      if (radioOk) radio.setPALevel(CONNEXConfig.paLevel, true);
    }
    CONNEXConfig_fromJson(data);
    if (data.containsKey("save") && data["save"].as<bool>()) {
      CONNEXConfig_save();
    }
    // after init / erase, which cancel learn mode, so {"erase":true,"learn":true} works
    if (data.containsKey("learn")) {
      learning = data["learn"].as<bool>();
      learnStartMs = millis();
      THEENGS_LOG_NOTICE(F("[CONNEX] learn mode %s" CR), learning ? "started: change a zone on the wall controller" : "cancelled");
    }
    // announce the zones when the ID became valid or init removed their entities;
    // also on the pair error paths, which return early
    auto announceIfNeeded = [&]() {
      if ((zonesRemoved || !hadId) && CONNEXConfig.idValid) launchCONNEXDiscovery();
    };
    // Pair: put the heater in sync mode (hold V 3 s, then -, +, V), then within 10 s send
    // {"pair":<zone>} or {"pair":<zone>,"temperature":20}. The heater adopts the controller ID of
    // the next frame it hears, so we invent and save an ID if we don't have one yet.
    if (data.containsKey("pair")) {
      int zone = parseZone(data["pair"]);
      float temp = data.containsKey("temperature") ? parseTemperature(data["temperature"]) : 20.0f;
      uint16_t v;
      if (!radioOk) {
        rejectCommand("radio not responding, pair dropped");
        announceIfNeeded();
        return;
      } else if (zone < 1 || zone > CONNEX_ZONES || !encodeSetpoint(temp, &v)) {
        rejectCommand("invalid pair, expected zone 1-4 and a numeric temperature in range");
        announceIfNeeded();
        return;
      } else {
        if (!CONNEXConfig.idValid) {
          uint16_t id;
          do {
            id = (uint16_t)(esp_random() & 0xFFFF);
          } while (id == 0x0000 || id == 0xFFFF);
          CONNEXConfig.controllerId[0] = id >> 8;
          CONNEXConfig.controllerId[1] = id & 0xFF;
          CONNEXConfig.idValid = true;
          CONNEXConfig_save();
          THEENGS_LOG_NOTICE(F("[CONNEX] pair: new controller ID %s" CR), controllerIdString().c_str());
        }
        learning = false;
        pendingSetpoint[zone - 1] = v / 10.0f;
        THEENGS_LOG_NOTICE(F("[CONNEX] pair: sending zone %d -> %.1f C as controller %s" CR), zone, v / 10.0f,
                           controllerIdString().c_str());
      }
    }
    stateCONNEXMeasures();
    announceIfNeeded();
  }
}

#endif
