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
#include "User_config.h"

#ifdef ZactuatorFASTLED

#  include <FastLED.h>

enum LEDState {
  OFF,
  FIRE,
  GENERAL
};
LEDState currentLEDState;
long lastUpdate = 0;
long currentUpdate = 0;
CRGB leds[FASTLED_NUM_LEDS];
CRGB ledColorBlink[FASTLED_NUM_LEDS];
bool blinkLED[FASTLED_NUM_LEDS];
const long blinkInterval = 300;
const long fireUpdate = 10;
CRGBPalette16 gPal;

void setupFASTLED() {
  Log.notice(F("FASTLED_DATA_GPIO: %d" CR), FASTLED_DATA_GPIO);
  Log.notice(F("FASTLED_NUM_LEDS: %d" CR), FASTLED_NUM_LEDS);
  Log.trace(F("ZactuatorFASTLED setup done " CR));
  FastLED.addLeds<FASTLED_TYPE, FASTLED_DATA_GPIO>(leds, FASTLED_NUM_LEDS);
}

//returns the current step of the animation
int animation_step(int duration, int steps) {
  int currentStep = ((currentUpdate % duration) / ((float)duration)) * steps;
  return currentStep;
}

//returns the number of steps since the last update
int animation_step_count(int duration, int steps) {
  long lastAnimationNumber = lastUpdate / duration;
  long currentAnimationNumber = currentUpdate / duration;
  int lastStep = ((lastUpdate % duration) / ((float)duration)) * steps;
  int currentStep = ((currentUpdate % duration) / ((float)duration)) * steps;

  return currentStep - lastStep + (currentAnimationNumber - lastAnimationNumber) * steps;
}

void FASTLEDLoop() {
  lastUpdate = currentUpdate;
  currentUpdate = millis();

  if (currentLEDState == GENERAL) {
    for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
      int count = animation_step_count(blinkInterval, 2);
      int step = animation_step(blinkInterval, 2);

      if (count > 0) {
        if (blinkLED[i]) {
          if (step == 0) {
            leds[i] = ledColorBlink[i];
          } else {
            ledColorBlink[i] = leds[i];
            leds[i] = CRGB::Black;
          }
        }
      }
    }
  } else if (currentLEDState == FIRE) {
    int count = animation_step_count(fireUpdate, 1);
    if (count > 0) {
      //random16_add_entropy( random(0)); //random() don't exists in ESP framework - workaround?
      Fire2012WithPalette();
    }
  }
  FastLED.show();
}

boolean FASTLEDtoMQTT() {
  return false;
}
#  if jsonReceiving
void MQTTtoFASTLED(char* topicOri, JsonObject& jsonData) {
  currentLEDState = GENERAL;
  //trc(topicOri);
  //number = (long)strtol(&datacallback[1], NULL, 16);

  if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLEDsetled)) {
    Log.trace(F("MQTTtoFASTLED JSON analysis" CR));
    int ledNr = jsonData["led"];
    Log.notice(F("Led numero: %d" CR), ledNr);
    const char* color = jsonData["hex"];
    Log.notice(F("Color hex: %s" CR), color);

    long number = (long)strtol(color, NULL, 16);
    bool blink = jsonData["blink"];
    if (ledNr <= FASTLED_NUM_LEDS) {
      Log.notice(F("Blink: %d" CR), blink);
      blinkLED[ledNr] = blink;
      leds[ledNr] = number;
    }
  }
}
#  endif

#  if simpleReceiving
void MQTTtoFASTLED(char* topicOri, char* datacallback) {
  Log.trace(F("MQTTtoFASTLED: " CR));
  currentLEDState = GENERAL;
  long number = 0;
  if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLED)) {
    number = (long)strtol(&datacallback[1], NULL, 16);
    Log.notice(F("Number: %l" CR), number);
    for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
      leds[i] = number;
    }
    FastLED.show();
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLEDsetbrightness)) {
    number = (long)strtol(&datacallback[1], NULL, 16);
    Log.notice(F("Number: %l" CR), number);
    FastLED.setBrightness(number);
    FastLED.show();
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLEDsetanimation)) {
    String payload = datacallback;
    Log.notice(F("Datacallback: %s" CR), datacallback);
    if (strstr(datacallback, "fire") != NULL) {
      currentLEDState = FIRE;
      gPal = HeatColors_p;
    } else {
      currentLEDState = OFF;
    }
  } else {
    currentLEDState = OFF;
  }
  if (currentLEDState == OFF) {
    for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
  }
}
#  endif
// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
////
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#  define COOLING 55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#  define SPARKING 120
bool gReverseDirection = false;

void Fire2012WithPalette() {
  // Array of temperature readings at each simulation cell
  static byte heat[FASTLED_NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / FASTLED_NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = FASTLED_NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < FASTLED_NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8(heat[j], 240);
    CRGB color = ColorFromPalette(gPal, colorindex);
    int pixelnumber;
    if (gReverseDirection) {
      pixelnumber = (FASTLED_NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

#endif
