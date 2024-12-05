/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

    Supported boards with displays

    HELTEC ESP32 LORA - SSD1306 / Onboard 0.96-inch 128*64 dot matrix OLED display
    LILYGO® LoRa32 V2.1_1.6.1 433 Mhz / https://www.lilygo.cc/products/lora3?variant=42476923879605

    Copyright: (c)Florian ROBERT

    Contributors:
    - 1technophile
    - NorthernMan54

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

#if defined(LVGL_ENABLED)

#  include <ArduinoJson.h>
#  include <ui.h>

#  include "ArduinoLog.h"
#  include "User_config.h"
#  include "config_LVGL.h"

boolean logToOLEDDisplay = LOG_TO_OLED;
boolean jsonDisplay = JSON_TO_OLED;
boolean displayFlip = DISPLAY_FLIP;
boolean displayState = DISPLAY_STATE;
boolean idlelogo = DISPLAY_IDLE_LOGO;
uint8_t displayBrightness = DISPLAY_BRIGHTNESS;

/*
module setup, for use in Arduino setup
*/
void setupLVGL() {
  LVGLConfig_init();
}

unsigned long nextDisplayPage = uptime() + DISPLAY_PAGE_INTERVAL;

/*
module loop, for use in Arduino loop
*/

bool firstRun = true;

const int MAX_TILES = 32;
lv_obj_t* tiles[MAX_TILES] = {nullptr};
int messageCount = 0;
int headIndex = 0; // Tracks the next index to overwrite

void loopLVGL() {
  /*
  Function to check if json messages are in the queue and send them for display

  long enough since the last message and display not being used and a queue message waiting
  */

  if (firstRun) {
    lv_disp_load_scr(ui_mainScreen);
    firstRun = false;
  }

  if (uptime() >= nextDisplayPage && currentWebUIMessage && newSSD1306Message) {
    // Remove the previous tile at the head if it exists
    if (tiles[headIndex] != nullptr) {
      lv_obj_del(tiles[headIndex]); // Free the old tile object
    }

    // Create a new tile and store it at the head index
    tiles[headIndex] = ui_omgLabelContainer_create(ui_mainScreen);
    lv_label_set_text(ui_comp_get_child(tiles[headIndex], UI_COMP_OMGLABELCONTAINER_OMGTITLE), currentWebUIMessage->title);
    lv_label_set_text(ui_comp_get_child(tiles[headIndex], UI_COMP_OMGLABELCONTAINER_OMGLINE1), currentWebUIMessage->line1);
    lv_label_set_text(ui_comp_get_child(tiles[headIndex], UI_COMP_OMGLABELCONTAINER_OMGLINE2), currentWebUIMessage->line2);
    lv_label_set_text(ui_comp_get_child(tiles[headIndex], UI_COMP_OMGLABELCONTAINER_OMGLINE3), currentWebUIMessage->line3);
    lv_label_set_text(ui_comp_get_child(tiles[headIndex], UI_COMP_OMGLABELCONTAINER_OMGLINE4), currentWebUIMessage->line4);

    // Move the head index forward and wrap it around if necessary
    headIndex = (headIndex + 1) % MAX_TILES;

    // Ensure messageCount doesn't exceed MAX_TILES
    if (messageCount < MAX_TILES) {
      messageCount++;
    }

    newSSD1306Message = false;
  }
}

/*
Handler for mqtt commands sent to the module
- log-oled: boolean
  Enable / Disable display of log messages on display
*/

void LVGLConfig_init() {
  displayState = DISPLAY_STATE;
  displayBrightness = DISPLAY_BRIGHTNESS;
  logToOLEDDisplay = LOG_TO_OLED;
  jsonDisplay = JSON_TO_OLED;
  idlelogo = DISPLAY_IDLE_LOGO;
  displayFlip = DISPLAY_FLIP;
  Log.notice(F("LVGL config initialised" CR));
}

#endif
