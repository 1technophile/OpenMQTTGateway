/*
  OpenMQTTGateway - ESP32, ESP8266 or Arduino program for home automation

  This actuator enables control over the PWM outputs of microcontrollers.

  It uses the highest resolution duty cycles available, to try to
  give good control over LEDs, particularly at low brightness settings.

  It supports different gamma curves to try to give good perceptually
  linear results when used to control LEDs, and it supports calibration
  of the min and max levels for each channel.

  It supports as many channels as the micro-controller can output PWM
  signals for.  Each channel can be named in the config header.

  Supported MQTT topics...

  ".../commands/MQTTtoPWM/set" : Set the state of one or more channels.
  All values are floating point.
 {
    "r"  : 0.0 - 1.0,
    "g"  : 0.0 - 1.0,
    "b"  : 0.0 - 1.0,
    "w0" : 0.0 - 1.0,
    "w1" : 0.0 - 1.0,

    "fade" : <fade time in seconds>
  }

  ".../commands/MQTTtoPWM/calibrate" : Set calibration data
  All values support floating point for greater precision.
  It can be convenient to use the 'retain' feature of MQTT to
  store calibration data.
  Gamma is a power value that is used help make the intensity
  curves respond in a more perceptually linear way.
  If you would like the values to remain linear, set the gamma
  values to 1.0
  Min and max set the levels that 0 and 1 will correspond to.
  Be aware the the min and max values are in the current gamma space
  before the conversion to linear - so if you change the gamma level
  then you will probably need to tune the min and max values again
 {
    "gamma-r" : 0.5 - 4.0,
    "min-r"   : 0.0 - 1.0,
    "max-r"   : 0.0 - 1.0,

    "gamma-g" : 0.5 - 4.0,
    etc...
  }

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

#ifdef ZactuatorPWM

#  include "config_PWM.h"

static long previousUpdateTime = 0; // milliseconds
static long currentUpdateTime = 0; // milliseconds

static const char* channelJsonKeys[] = PWM_CHANNEL_NAMES;
static const int channelPins[] = PWM_CHANNEL_PINS;
static const int kNumChannels = sizeof(channelPins) / sizeof(int);

// These are all in a perceptually linear colour space, but scaled 0-1
static float currentValues[kNumChannels] = {};
static float fadeStartValues[kNumChannels] = {};
static float targetValues[kNumChannels] = {};

static long fadeStartUpdateTime[kNumChannels] = {}; // milliseconds
static long fadeEndUpdateTime[kNumChannels] = {}; // milliseconds
static bool fadeIsComplete = false;

// Calibration data (initialised during setupPWM)
static float calibrationMinLinear[kNumChannels];
static float calibrationMaxLinear[kNumChannels];
static float calibrationGamma[kNumChannels];

#  if defined(ESP32)
// We use the highest resolution duty cycle mode (16-bit) for ESP32, which
// gives us good control LEDs at the very dim end of the range.
static const int kNumDutyCycleBits = 16;
#  elif defined(ESP8266)
// ESP8266 has a 10-bit duty cycle, which is better than 8 bits, but still
// not good enough to give really good dim control over LEDs.
static const int kNumDutyCycleBits = 10;
#  else
// Assume Arduino with 8-bit duty cycle.
// With an 8-bit duty cycle, it's difficult to get good control over
// LEDs at the dark end of the range.
static const int kNumDutyCycleBits = 8;
#  endif

static const float kUNormToDutyCycle = (float)((1 << kNumDutyCycleBits) - 1);

void setupPWM() {
  // Setup the PWM channels at the highest frequency we can for full 16-bit
  // duty cycle control.  These channels will be assigned to the
  // associated output pins.

  // PWM outputs vary the light intensity linearly when used to control
  // LEDs. But our eyes don't perceive actual linear changes as linear.
  // This manifests as a problem when trying to have fine control
  // over LEDs at very low levels.
  // Using an 8-bit duty cycle for example only allows for 256 different
  // linear levels of brightness.  Perceptually, the difference in
  // brightness between levels 1 and 2 is very large - so to get fine
  // control at dark levels, it's important that we use as high
  // resolution PWM as we can.
  for (int i = 0; i < kNumChannels; ++i) {
#  if defined(ESP32)
    // Configure the pin for PWM output.
    // I think this is the fastest frequency that allows for a 16-bit
    // duty cycle on an ESP32
    ledcSetup(i, 625.0, kNumDutyCycleBits);
    ledcAttachPin(channelPins[i], i);
#  endif
    calibrationMinLinear[i] = 0.f;
    calibrationMaxLinear[i] = 1.f;
    calibrationGamma[i] = PWM_DEFAULT_GAMMA;
  }

  Log.trace(F("ZactuatorPWM setup done " CR));
}

// This applies a power curve to the input to try to make the inputs
// be 'perceptually linear', as opposed to actually linear.
static float perceptualToLinear(float perceptual, int channelIdx) {
  return pow(perceptual, calibrationGamma[channelIdx]);
}

// If we're currently fading between states, then update those states
void PWMLoop() {
  previousUpdateTime = currentUpdateTime;
  currentUpdateTime = millis();

  if (fadeIsComplete) {
    return;
  }

  fadeIsComplete = true; //< If any channel isn't finished, we'll set this to false
  for (int i = 0; i < kNumChannels; ++i) {
    // Calculate our lerp value through the current fade
    long totalFadeDuration = fadeEndUpdateTime[i] - fadeStartUpdateTime[i];
    float fadeLerpValue = 1.f;
    if (totalFadeDuration > 0) {
      fadeLerpValue = (float)(currentUpdateTime - fadeStartUpdateTime[i]) / (float)totalFadeDuration;
    }
    if (fadeLerpValue >= 1.f) {
      currentValues[i] = targetValues[i];
    } else {
      currentValues[i] = ((targetValues[i] - fadeStartValues[i]) * fadeLerpValue) + fadeStartValues[i];
      fadeIsComplete = false;
    }
  }

  // Now convert these perceptually linear values into actually linear values
  // and set the appropriate duty cycle for the outputs.
  for (int i = 0; i < kNumChannels; ++i) {
    float linear = perceptualToLinear(currentValues[i], i);

    // We always treat zero as zero so that it's truly off, regardless of the calibration data.
    if (linear > 0.f) {
      // Remap according to the calibration
      linear = (linear * (calibrationMaxLinear[i] - calibrationMinLinear[i])) + calibrationMinLinear[i];
    }

    long dutyCycle = (long)(linear * kUNormToDutyCycle);
#  if defined(ESP32)
    ledcWrite(i, dutyCycle);
#  else
    analogWrite(channelPins[i], dutyCycle);
#  endif
    //Log.notice(F("Setting channel %d : %d" CR),i,dutyCycle);
  }
}

boolean PWMtoMQTT() {
  return false;
}

#  if jsonReceiving
void MQTTtoPWM(char* topicOri, JsonObject& jsonData) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoPWMset)) {
    Log.trace(F("MQTTtoPWM JSON analysis" CR));
    // Parse the target value for each channel
    int modifiedChannelBits = 0;
    for (int i = 0; i < kNumChannels; ++i) {
      fadeStartValues[i] = currentValues[i];
      JsonVariant value = jsonData[channelJsonKeys[i]];
      if (!value.isNull()) {
        float targetValue = value.as<float>();
        targetValue = std::min(targetValue, 1.f);
        targetValue = std::max(targetValue, 0.f);
        targetValues[i] = targetValue;

        // Initially configure as an instantaneous change....
        fadeStartUpdateTime[i] = currentUpdateTime;
        fadeEndUpdateTime[i] = currentUpdateTime;

        modifiedChannelBits |= (1 << i);
      }
    }
    // ...unless there is a "fade" value in the JSON
    JsonVariant fade = jsonData["fade"];
    if (!fade.isNull()) {
      // "fade" json value is in seconds. Convert to milliseconds
      long endUpdateTime = currentUpdateTime + (long)(fade.as<float>() * (1000.f));
      // Set the fadeEndUpdateTime for the channels that were modified in the JSON
      for (int i = 0; i < kNumChannels; ++i) {
        if (modifiedChannelBits & (1 << i)) {
          fadeEndUpdateTime[i] = endUpdateTime;
        }
      }
    }
    fadeIsComplete = false; // The values will start to change during PWMLoop
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoPWMcalibrate)) {
    // Read the optional calibration data for each channel
    for (int i = 0; i < kNumChannels; ++i) {
      char key[64];
      snprintf(key, sizeof(key), "gamma-%s", channelJsonKeys[i]);
      JsonVariant value = jsonData[key];
      if (!value.isNull()) {
        float gamma = value.as<float>();
        // Sanity check
        gamma = std::min(gamma, 4.f);
        gamma = std::max(gamma, 0.5f);
        calibrationGamma[i] = gamma;
      }
      snprintf(key, sizeof(key), "min-%s", channelJsonKeys[i]);
      value = jsonData[key];
      if (!value.isNull()) {
        calibrationMinLinear[i] = perceptualToLinear(value.as<float>(), i);
      }
      snprintf(key, sizeof(key), "max-%s", channelJsonKeys[i]);
      value = jsonData[key];
      if (!value.isNull()) {
        calibrationMaxLinear[i] = perceptualToLinear(value.as<float>(), i);
      }
    }
  }
}
#  endif

#endif
