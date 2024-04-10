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

#  include "esp_blufi_api.h"

extern "C" {
#  include "esp_blufi.h"
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

static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t* param) {
  /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
  switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
      Log.notice(F("BLUFI init finish" CR));
      esp_blufi_adv_start();
      break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
      Log.notice(F("BLUFI deinit finish" CR));
      NimBLEDevice::deinit(true);
      break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
      omg_blufi_ble_connected = true;
      esp_blufi_adv_stop();
      blufi_security_init();
      break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
      omg_blufi_ble_connected = false;
      blufi_security_deinit();
      if (WiFi.isConnected()) {
        esp_blufi_deinit();
      } else {
        esp_blufi_adv_start();
      }
      break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
      Log.notice(F("BLUFI requset wifi connect to AP" CR));
      WiFi.begin((char*)gl_sta_ssid, (char*)gl_sta_passwd);
      gl_sta_is_connecting = true;
      break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
      Log.notice(F("BLUFI requset wifi disconnect from AP\n" CR));
      WiFi.disconnect();
      break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
      Log.notice(F("BLUFI report error, error code %d\n" CR), param->report_error.state);
      esp_blufi_send_error_info(param->report_error.state);
      break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
      esp_blufi_extra_info_t info;
      if (WiFi.isConnected()) {
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
      Log.notice(F("Recv STA PASSWORD %s" CR), gl_sta_passwd);
      break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST: {
      WiFi.scanNetworks(true);
      break;
    }
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA: {
      Log.notice(F("Recv Custom Data %" PRIu32 CR), param->custom_data.data_len);
      esp_log_buffer_hex("Custom Data", param->custom_data.data, param->custom_data.data_len);

      DynamicJsonDocument json(1024);
      auto error = deserializeJson(json, param->custom_data.data);
      if (error) {
        Log.error(F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
        break;
      }
      if (!json.isNull()) {
        Log.trace(F("\nparsed json, size: %u" CR), json.memoryUsage());
        if (json.containsKey("mqtt_server"))
          strcpy(mqtt_server, json["mqtt_server"]);
        if (json.containsKey("mqtt_port"))
          strcpy(mqtt_port, json["mqtt_port"]);
        if (json.containsKey("mqtt_user"))
          strcpy(mqtt_user, json["mqtt_user"]);
        if (json.containsKey("mqtt_pass"))
          strcpy(mqtt_pass, json["mqtt_pass"]);
        if (json.containsKey("mqtt_topic"))
          strcpy(mqtt_topic, json["mqtt_topic"]);
        if (json.containsKey("mqtt_broker_secure"))
          mqtt_secure = json["mqtt_broker_secure"].as<bool>();
        if (json.containsKey("gateway_name"))
          strcpy(gateway_name, json["gateway_name"]);
        saveConfig();
      }
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
        Log.notice(F("No AP found" CR));
        break;
      }
      wifi_ap_record_t* ap_list = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * apCount);
      if (!ap_list) {
        Log.error(F("malloc error, ap_list is NULL"));
        break;
      }
      ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list));
      esp_blufi_ap_record_t* blufi_ap_list = (esp_blufi_ap_record_t*)malloc(apCount * sizeof(esp_blufi_ap_record_t));
      if (!blufi_ap_list) {
        if (ap_list) {
          free(ap_list);
        }
        Log.error(F("malloc error, blufi_ap_list is NULL" CR));
        break;
      }
      for (int i = 0; i < apCount; ++i) {
        blufi_ap_list[i].rssi = ap_list[i].rssi;
        memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
      }

      if (omg_blufi_ble_connected == true) {
        esp_blufi_send_wifi_list(apCount, blufi_ap_list);
      }

      free(ap_list);
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
  char advName[20] = {0};
  snprintf(advName, sizeof(advName), "OMGBFI_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  NimBLEDevice::init(advName);
  esp_blufi_gatt_svr_init();
  ble_gatts_start();
  return esp_blufi_profile_init() == ESP_OK;
}

#endif // defined(ESP32) && defined(USE_BLUFI)
