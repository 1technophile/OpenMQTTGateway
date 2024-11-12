/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This program enables to:
 - receive MQTT data from a topic and send signal (RF, IR, BLE, GSM)  corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received signals (RF, IR, BLE, GSM)

  Copyright: (c)Florian ROBERT

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
#if defined(ESP32) && defined(USE_BLUFI)
#  include "NimBLEDevice.h"
#  include "esp_blufi_api.h"
#  include "esp_timer.h"

extern "C" {
#  include "esp_blufi.h"
}

static esp_timer_handle_t connection_timer = nullptr;

struct pkt_info {
  uint8_t* pkt;
  int pkt_len;
};

#  define DATA_PACKAGE_VALUE       0x02
#  define DATA_SUBTYPE_CUSTOM_DATA 0x07
#  define FRAME_CTRL_DEFAULT       0x00 // Assuming no encryption or checksum on notifications for simplicity

uint8_t getTypeValue(uint8_t packageType, uint8_t subType) {
  return (packageType << 2) | subType;
}

/* store the station info for send back to phone */
//static bool gl_sta_connected = false;
bool omg_blufi_ble_connected = false;
static uint8_t gl_sta_bssid[6];
static uint8_t gl_sta_ssid[32];
static uint8_t gl_sta_passwd[64];
static int gl_sta_ssid_len;
static bool gl_sta_is_connecting = false;
static esp_blufi_extra_info_t gl_sta_conn_info;

static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t* param);
void wifi_event_handler(arduino_event_id_t event);
esp_err_t blufi_security_init(void);
void blufi_dh_negotiate_data_handler(uint8_t* data, int len, uint8_t** output_data, int* output_len, bool* need_free);
int blufi_aes_encrypt(uint8_t iv8, uint8_t* crypt_data, int crypt_len);
int blufi_aes_decrypt(uint8_t iv8, uint8_t* crypt_data, int crypt_len);
uint16_t blufi_crc_checksum(uint8_t iv8, uint8_t* data, int len);
void blufi_security_deinit(void);

uint8_t getNextSequence() {
  static uint8_t sequence = 0;
  return sequence++;
}

struct ReceivingCommandTaskData {
  uint8_t* data;
  uint32_t data_len;
};

// Task function to handle receivingCommand
void receivingCommandTask(void* pvParameters) {
  ReceivingCommandTaskData* taskData = static_cast<ReceivingCommandTaskData*>(pvParameters);

  DynamicJsonDocument json(JSON_MSG_BUFFER_MAX);
  JsonObject jsonBlufi = json.to<JsonObject>();
  auto error = deserializeJson(json, taskData->data, taskData->data_len);
  if (error) {
    Log.error(F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
  } else {
    if (jsonBlufi.containsKey("target") && jsonBlufi["target"].is<char*>()) {
      char topic[(parameters_size)*2 + jsonBlufi["target"].size() + 1];
      snprintf(topic, sizeof(topic), "%s%s%s", mqtt_topic, gateway_name, jsonBlufi["target"].as<const char*>());
      jsonBlufi.remove("target");
      char jsonStr[JSON_MSG_BUFFER_MAX];
      serializeJson(jsonBlufi, jsonStr);
      receivingDATA(topic, jsonStr);
    } else {
      Log.notice(F("No target found in the received command using SYS target, default index and save command" CR));
      if (!json.containsKey("cnt_index")) {
        json["cnt_index"] = CNT_DEFAULT_INDEX;
        json["save_cnt"] = true;
      }
      char topic[(parameters_size)*2 + strlen(subjectMQTTtoSYSset) + 1];
      snprintf(topic, sizeof(topic), "%s%s%s", mqtt_topic, gateway_name, subjectMQTTtoSYSset);
      char jsonStr[JSON_MSG_BUFFER_MAX];
      serializeJson(jsonBlufi, jsonStr);
      receivingDATA(topic, jsonStr);
    }
  }

  // Clean up dynamically allocated memory, if any
  if (taskData->data) {
    free(taskData->data);
  }
  delete taskData;

  // Delete the task when finished
  vTaskDelete(NULL);
}

// We create a task to remove the load from the Bluetooth task
void createReceivingCommandTask(uint8_t* data, uint32_t data_len) {
  // Allocate memory for task data and copy over the data
  ReceivingCommandTaskData* taskData = new ReceivingCommandTaskData;
  taskData->data = static_cast<uint8_t*>(malloc(data_len));
  memcpy(taskData->data, data, data_len);
  taskData->data_len = data_len;

  // Create the task
  xTaskCreate(receivingCommandTask, "ReceivingCmdTask", 10000, taskData, 5, NULL);
}

void sendCustomDataNotification(const char* message) {
  if (!omg_blufi_ble_connected) {
    return;
  }
  size_t messageLength = strlen(message);
  uint8_t notification[messageLength + 4];
  notification[0] = getTypeValue(DATA_PACKAGE_VALUE, DATA_SUBTYPE_CUSTOM_DATA);
  notification[1] = FRAME_CTRL_DEFAULT;
  notification[2] = getNextSequence();
  notification[3] = static_cast<uint8_t>(messageLength);
  memcpy(&notification[4], message, messageLength);
  esp_blufi_send_custom_data(notification, sizeof(notification));
}

#  ifdef BT_CONNECTION_TIMEOUT_MS
void connection_timeout_callback(void* arg) {
  if (omg_blufi_ble_connected) {
    Log.notice(F("BluFi connection timeout reached. Disconnecting." CR));
    esp_blufi_disconnect();
    omg_blufi_ble_connected = false;
  }
}

void restart_connection_timer() {
  if (connection_timer == nullptr) {
    esp_timer_create_args_t timer_args = {
        .callback = connection_timeout_callback,
        .arg = NULL,
        .name = "blufi_connection_timer"};
    esp_timer_create(&timer_args, &connection_timer);
  }

  esp_timer_stop(connection_timer); // Stop the timer if it's running
  esp_err_t ret = esp_timer_start_once(connection_timer, BT_CONNECTION_TIMEOUT_MS * 1000);
  if (ret != ESP_OK) {
    Log.error(F("Failed to start connection timer: %d" CR), ret);
  }
}

void stop_connection_timer() {
  if (connection_timer != nullptr) {
    esp_timer_stop(connection_timer);
  }
}
#  else
void restart_connection_timer() {}
void stop_connection_timer() {}
#  endif

static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t* param) {
  /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
  switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
      Log.trace(F("BLUFI init finish" CR));
      esp_blufi_adv_start();
      break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
      Log.trace(F("BLUFI deinit finish" CR));
      NimBLEDevice::deinit(true);
      if (connection_timer != nullptr) {
        esp_timer_delete(connection_timer);
        connection_timer = nullptr;
      }
      break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
      Log.trace(F("BLUFI BLE connect" CR));
      gatewayState = GatewayState::ONBOARDING;
      omg_blufi_ble_connected = true;
      restart_connection_timer();
      esp_blufi_adv_stop();
      blufi_security_init();
      break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
      Log.trace(F("BLUFI BLE disconnect" CR));
      omg_blufi_ble_connected = false;
      stop_connection_timer();
      if (mqtt && mqtt->connected()) {
        gatewayState = GatewayState::BROKER_CONNECTED;
      } else if (ethConnected || WiFi.status() == WL_CONNECTED) {
        gatewayState = GatewayState::NTWK_CONNECTED;
      } else if (mqttSetupPending) {
        gatewayState = GatewayState::WAITING_ONBOARDING;
      } else {
        gatewayState = GatewayState::OFFLINE;
      }
      blufi_security_deinit();
      esp_blufi_adv_start();
      break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
      Log.trace(F("BLUFI request wifi connect to AP" CR));
      WiFi.begin((char*)gl_sta_ssid, (char*)gl_sta_passwd);
      gl_sta_is_connecting = true;
      break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
      Log.trace(F("BLUFI request wifi disconnect from AP\n" CR));
      WiFi.disconnect();
      break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
      Log.trace(F("BLUFI report error, error code %d\n" CR), param->report_error.state);
      esp_blufi_send_error_info(param->report_error.state);
      break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
      esp_blufi_extra_info_t info;
      if (gatewayState == GatewayState::NTWK_CONNECTED) {
        memset(&info, 0, sizeof(esp_blufi_extra_info_t));
        memcpy(info.sta_bssid, gl_sta_bssid, 6);
        info.sta_bssid_set = true;
        info.sta_ssid = gl_sta_ssid;
        info.sta_ssid_len = gl_sta_ssid_len;
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
      } else if (gl_sta_is_connecting) {
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONNECTING, 0, &gl_sta_conn_info);
      } else {
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, 0, &gl_sta_conn_info);
      }

      break;
    }
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
      Log.trace(F("BLUFI recv slave disconnect a ble connection" CR));
      esp_blufi_disconnect();
      break;
    case ESP_BLUFI_EVENT_RECV_STA_SSID:
      strncpy((char*)gl_sta_ssid, (char*)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
      gl_sta_ssid[param->sta_ssid.ssid_len] = '\0';
      Log.notice(F("Recv STA SSID %s" CR), gl_sta_ssid);
      break;
    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
      strncpy((char*)gl_sta_passwd, (char*)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
      gl_sta_passwd[param->sta_passwd.passwd_len] = '\0';
      Log.notice(F("Recv STA PASSWORD" CR));
      break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST: {
      WiFi.scanNetworks(true);
      break;
    }
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA: {
      Log.notice(F("Recv Custom Data %" PRIu32 CR), param->custom_data.data_len);
      esp_log_buffer_hex("Custom Data", param->custom_data.data, param->custom_data.data_len);
      createReceivingCommandTask(param->custom_data.data, param->custom_data.data_len);
      break;
    }
    case ESP_BLUFI_EVENT_RECV_USERNAME:
      break;
    case ESP_BLUFI_EVENT_RECV_CA_CERT:
      break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
      break;
    case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
      break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
      break;
      ;
    case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
      break;
    default:
      break;
  }
}

void wifi_event_handler(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      gatewayState = GatewayState::NTWK_CONNECTED;
#  ifndef ESPWifiManualSetup
      wifiManager.stopConfigPortal();
#  endif
      gl_sta_is_connecting = false;
      esp_blufi_extra_info_t info;
      memset(&info, 0, sizeof(esp_blufi_extra_info_t));
      memcpy(info.sta_bssid, gl_sta_bssid, 6);
      info.sta_bssid_set = true;
      info.sta_ssid = gl_sta_ssid;
      info.sta_ssid_len = gl_sta_ssid_len;
      if (omg_blufi_ble_connected == true) {
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
      }

      break;
    }
    case ARDUINO_EVENT_WIFI_SCAN_DONE: {
      uint16_t apCount = WiFi.scanComplete();
      if (apCount == 0) {
        Log.error(F("No AP found" CR));
        break;
      }
      Log.trace(F("AP found, count: %d" CR), apCount);
      esp_blufi_ap_record_t* blufi_ap_list = (esp_blufi_ap_record_t*)malloc(apCount * sizeof(esp_blufi_ap_record_t));
      if (!blufi_ap_list) {
        Log.error(F("Failed to allocate memory for AP list" CR));
        break;
      }
      for (int i = 0; i < apCount; ++i) {
        Log.notice(F("%d: %s, Ch:%d (%ddBm)" CR), i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
        blufi_ap_list[i].rssi = WiFi.RSSI(i);
        size_t ssidLength = strlen(WiFi.SSID(i).c_str());
        if (ssidLength > sizeof(blufi_ap_list[i].ssid) - 1) {
          ssidLength = sizeof(blufi_ap_list[i].ssid) - 1;
        }
        memcpy(blufi_ap_list[i].ssid, WiFi.SSID(i).c_str(), ssidLength);
        blufi_ap_list[i].ssid[ssidLength] = '\0';
      }
      if (omg_blufi_ble_connected == true) {
        if (esp_blufi_send_wifi_list(apCount, blufi_ap_list) != ESP_OK) {
          Log.error(F("Failed to send WiFi list" CR));
        }
      }
      free(blufi_ap_list);
      break;
    }
    default:
      break;
  }
  return;
}

static esp_blufi_callbacks_t example_callbacks = {
    .event_cb = example_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

bool isBlufiConnected() {
  return omg_blufi_ble_connected;
}

bool isStaConnecting() {
  return gl_sta_is_connecting;
}

bool startBlufi() {
  esp_err_t ret = ESP_OK;
  WiFi.onEvent(wifi_event_handler);

  ret = esp_blufi_register_callbacks(&example_callbacks);
  if (ret) {
    Log.error(F("%s blufi register failed, error code = %x" CR), __func__, ret);
    return false;
  }

  if (NimBLEDevice::getInitialized()) {
    NimBLEDevice::deinit(true);
    delay(50);
  }
  esp_blufi_btc_init();
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char advName[17] = {0};
  // Check length of Gateway_Short_Name
  if (strlen(Gateway_Short_Name) > 3) {
    Log.error(F("Gateway_Short_Name is too long, max 3 characters" CR));
    return false;
  }
  snprintf(advName, sizeof(advName), Gateway_Short_Name "_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  NimBLEDevice::init(advName);
  esp_blufi_gatt_svr_init();
  ble_gatts_start();
  Log.notice(F("BLUFI started" CR));
  return esp_blufi_profile_init() == ESP_OK;
}

bool stopBlufi() {
  esp_err_t result = ESP_OK;
  if (omg_blufi_ble_connected) {
    esp_blufi_disconnect();
    delay(50);
  }

  ble_gap_adv_stop();
  result = esp_blufi_profile_deinit();
  if (result != ESP_OK) {
    Log.error(F("Failed to deinit blufi profile: %d" CR), result);
    return false;
  }
  Log.notice(F("BLUFI stopped" CR));
  return true;
}

#endif // defined(ESP32) && defined(USE_BLUFI)