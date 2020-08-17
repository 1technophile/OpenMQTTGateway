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

#ifdef ZactuatorPWMLED

#include "config_PWMLED.h"

static long previousUpdate = 0;    // milliseconds
static long currentUpdate = 0; // milliseconds

static const int kNumChannels = 5;

// These are all in a perceptually linear colour space, but scaled 0-1
static float previousValues[kNumChannels] = {};
static float currentValues[kNumChannels] = {};
static float targetValues[kNumChannels] = {};

// Calibration data
static float calibrationMinLinear[kNumChannels];
static float calibrationMaxLinear[kNumChannels];
static float calibrationGamma[kNumChannels];

static long previousValuesUpdate = 0; // milliseconds
static long targetValuesUpdate = 0; // milliseconds
static bool targetValuesAchieved = false;

static const char* channelJsonKeys[] = {"r", "g", "b", "w0", "w1"};
static const int channelPins[] = {PWMLED_R_PIN, PWMLED_G_PIN, PWMLED_B_PIN, PWMLED_W0_PIN, PWMLED_W1_PIN};

void setupPWMLED()
{
  //Log.notice(F("PWMLED_DATA_PIN: %d" CR), PWMLED_DATA_PIN);
  //Log.notice(F("PWMLED_NUM_LEDS: %d" CR), PWMLED_NUM_LEDS);
  Log.trace(F("ZactuatorPWMLED setup done " CR));
  //FastLED.addLeds<PWMLED_TYPE, PWMLED_DATA_PIN>(leds, PWMLED_NUM_LEDS);

  // Setup 5 PWM channels at the highest frequency we can for full 16-bit
  // duty cycle control.  These channels will be assigned to the pins
  // for R, G, B, W0 and W1 outputs.
  for(int i = 0; i < kNumChannels; ++i)
  {
    ledcSetup(i, 625.0, 16);
    ledcAttachPin(channelPins[i], i);
    calibrationMinLinear[i] = 0.f;
    calibrationMaxLinear[i] = 1.f;
    calibrationGamma[i] = 2.2f;
  }
}

static float perceptualToLinear(float perceptual, int channelIdx)
{
  return pow(perceptual, calibrationGamma[channelIdx]);
}

void PWMLEDLoop()
{
  previousUpdate = currentUpdate;
  currentUpdate = millis();

  if(targetValuesAchieved)
  {
    // Bail
    return;
  }

  // Calculate our current lerp through the current fade
  long fadeTime = targetValuesUpdate - previousValuesUpdate;
  float targetLerpValue = 1.f;
  if(fadeTime > 0)
  {
    targetLerpValue = (float) (currentUpdate - previousValuesUpdate) / (float) fadeTime;
  }
  if(targetLerpValue >= 1.f)
  {
    for(int i = 0; i < kNumChannels; ++i)
    {
      currentValues[i] = targetValues[i];
    }
    targetValuesAchieved = true;
  }
  else
  {
    for(int i = 0; i < kNumChannels; ++i)
    {
      currentValues[i] = ((targetValues[i] - previousValues[i]) * targetLerpValue) + previousValues[i];
    }
  }

  // Now convert these perceptually linear values into an actually linear value
  // and set the appropriate duty cycle for the output.
  for(int i = 0; i < kNumChannels; ++i)
  {
    float linear = perceptualToLinear(currentValues[i], i);

    if(linear > 0.f) // We always treat zero as zero
    {
      // Remap according to the calibration
      linear = (linear * (calibrationMaxLinear[i] - calibrationMinLinear[i])) + calibrationMinLinear[i];
    }

    long dutyCycle = (long) (linear * 65535.f);
    ledcWrite(i, dutyCycle);
    //Log.notice(F("Setting channel %d : %d" CR),i,dutyCycle);
  }

}

boolean PWMLEDtoMQTT()
{
  return false;
}

#ifdef jsonReceiving
void MQTTtoPWMLED(char *topicOri, JsonObject &jsonData)
{
  if (cmpToMainTopic(topicOri, subjectMQTTtoPWMLEDsetleds))
  {
    Log.trace(F("MQTTtoPWMLED JSON analysis" CR));
    //Log.notice(F("MQTTtoPWMLED JSON analysis" CR));
    for(int i = 0; i < kNumChannels; ++i)
    {
      previousValues[i] = currentValues[i];
      JsonVariant value = jsonData[channelJsonKeys[i]];
      if(value.success())
      {
        float targetValue = value.as<float>() * (1.f / 255.f);
        targetValue = std::min(targetValue, 1.f);
        targetValue = std::max(targetValue, 0.f);
        targetValues[i] = targetValue;
      }
    }
    previousValuesUpdate = currentUpdate;
    targetValuesUpdate = currentUpdate;
    JsonVariant fade = jsonData["fade"];
    if(fade.success())
    {
      // "fade" json value is in seconds. Convert to milliseconds
      targetValuesUpdate += (long) (fade.as<float>() * (1000.f));
    }
    targetValuesAchieved = false;
  }
  else if (cmpToMainTopic(topicOri, subjectMQTTtoPWMLEDcalibrateleds))
  {
    for(int i = 0; i < kNumChannels; ++i)
    {
      char key[16];
      snprintf(key, 16, "gamma-%s", channelJsonKeys[i]);
      JsonVariant value = jsonData[key];
      if(value.success())
      {
        float gamma = value.as<float>();
        // Sanity check
        gamma = std::min(gamma, 4.f);
        gamma = std::max(gamma, 0.5f);
        calibrationGamma[i] = gamma;
      }
      snprintf(key, 16, "min-%s", channelJsonKeys[i]);
      value = jsonData[key];
      if(value.success())
      {
        calibrationMinLinear[i] = perceptualToLinear(value.as<float>() * (1.f / 255.f), i);
      }
      snprintf(key, 16, "max-%s", channelJsonKeys[i]);
      value = jsonData[key];
      if(value.success())
      {
        calibrationMaxLinear[i] = perceptualToLinear(value.as<float>() * (1.f / 255.f), i);
      }
    }
  }
}
#endif

#ifdef simpleReceiving
void MQTTtoPWMLED(char *topicOri, char *datacallback)
{
}
#endif

#endif
