/*
  OpenMQTTGateway - ESP8266 or Arduino program for home automation

  This gateway enables to use RGBLED strips like WS2812

    Copyright: (c)

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
#ifdef ZactuatorFASTLED

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
  trc(F("ZactuatorFASTLED setup done "));
  FastLED.addLeds<NEOPIXEL, FASTLED_DATA_PIN>(leds, FASTLED_NUM_LEDS);
}

boolean FASTLEDtoMQTT() {
  return false;
}

void MQTTtoFASTLED(char * topicOri, char * datacallback) {
  String topic = topicOri;
  long number = (long) strtol( &datacallback[1], NULL, 16);
  trc(F("MQTTtoFASTLED: "));
  trc(topic);
  trc(number);
  if ((topic == subjectMQTTtoFASTLED)){
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = number;
    }
    FastLED.show();
  } else if (topic == subjectMQTTtoFASTLEDscan){
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
  } else if (topic == subjectMQTTtoFASTLEDalarm){
    for (int j = 60; j > 0; j--) {
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        leds[i] = number;
      }
      FastLED.show();
      delay(180);
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
      delay(120);
    }
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = number;
    }
    FastLED.show();
} else if (topic == subjectMQTTtoFASTLEDbreath){
  int initialbrightness = FastLED.getBrightness();
  FastLED.setBrightness(200);
    for (int j = 8000; j > 0; j--) {
      float breath = (exp(sin(millis()/2000.0*PI)) - 0.36787944)*108.0;    
      for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
        leds[i] = number;
        FastLED.setBrightness(breath);
      }
      FastLED.show();
      delay(1);
    }
    delay(200);
    FastLED.setBrightness(initialbrightness);
    for (int i = 0 ; i < FASTLED_NUM_LEDS; i++ ) {
      leds[i] = number;
    }
    FastLED.show();
  } else if (topic == subjectMQTTtoFASTLEDrainbow){
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
