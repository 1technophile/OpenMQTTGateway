/*
  OpenMQTTGateway - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:

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
#ifdef ZgatewayFASTLED

#ifdef ESP8266
#include <FastLED.h>
#else
#include <FastLED.h>
#endif

CRGB leds[FASTLED_NUM_LEDS];

void setupFASTLED() {
  trc(F("FASTLED_DATA_PIN "));
  trc(String(FASTLED_DATA_PIN));
  trc(F("FASTLED_NUM_LEDS "));
  trc(String(FASTLED_NUM_LEDS));
  trc(F("ZgatewayIR setup done "));

  FastLED.addLeds<NEOPIXEL, FASTLED_DATA_PIN>(leds, FASTLED_NUM_LEDS);
}

boolean FASTLEDtoMQTT() {
  return false;
}

void MQTTtoFASTLED(char * topicOri, char * datacallback) {
  trc(String(topicOri));
  long number = (long) strtol( &datacallback[1], NULL, 16);
  if (String(topicOri) == "home/commands/MQTTtoFASTLED") {
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = number;
    }
    FastLED.show();
  } else if (String(topicOri) == "home/commands/MQTTtoFASTLED/scan") {
    for (int j = 10; j > 0; j--) {
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        leds[i] = number;
        leds[i - 1] = CRGB::Black;
        FastLED.show();
        delay(100);
      }
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        int k = FASTLED_NUM_LEDS - i - 1;
        leds[k] = number;
        leds[k + 1] = CRGB::Black;
        FastLED.show();
        delay(100);
      }
    }
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = number;
    }
    FastLED.show();
//  } else if (String(topicOri) == "home/commands/MQTTtoFASTLED/breath") {
//    for (int j = 8200; j > 0; j--) {
//      float breath = (exp(sin(j / 2000.0 * PI)) - 0.36787944) * 108.0;
//      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
//        FastLED.setBrightness(breath);
//        leds[i] = number;
//      }
//      FastLED.show();
//      delay(1);
//    }
//    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
//      leds[i] = number;
//    }
//    FastLED.show();
  } else if (String(topicOri) == "home/commands/MQTTtoFASTLED/alarm") {
    for (int j = 100; j > 0; j--) {
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        leds[i] = number;
      }
      FastLED.show();
      delay(200);
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
      delay(200);
    }
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = number;
    }
    FastLED.show();
  } else if (String(topicOri) == "home/commands/MQTTtoFASTLED/rainbow") {
    for (int j = 0; j < 255; j++) {
      int ihue = j; //some microcontrollers use HSV from 0-255 vs the normal 0-360
      if (ihue > 255) {
        ihue = 0;
      }
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        leds[i] = CHSV(ihue, 255, 255);
      }
      FastLED.show();
      delay(100);
    }
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = number;
    }
    FastLED.show();
  } else {
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = CRGB::Green;
    }
    FastLED.setBrightness(50);
    FastLED.show();
    delay(200);
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  }
}
#endif
