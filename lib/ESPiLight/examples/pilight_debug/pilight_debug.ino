/*
 Basic ESPiLight pilight_debug example

 This example mimic the output of the piligh_debug tool.

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define PULSE_DIV 34

#define RECEIVER_PIN 4  // any intterupt able pin
#define TRANSMITTER_PIN 13

ESPiLight rf(TRANSMITTER_PIN);  // use -1 to disable transmitter

unsigned int normalize(unsigned int i, unsigned int pulselen) {
  double x;
  x = (double)i / pulselen;

  return (unsigned int)(round(x));
}

// callback function. It is called on successfully received and parsed rc signal
void rfRawCallback(const uint16_t* pulses, size_t length) {
  uint16_t pulse;
  uint16_t pulselen = pulses[length - 1] / PULSE_DIV;
  if (pulselen > 25) {
    for (unsigned int i = 3; i < length; i++) {
      if ((pulses[i] / pulselen) >= 2) {
        pulse = pulses[i];
        break;
      }
    }
    if (normalize(pulse, pulselen) > 0 && length > 25) {
      /* Print everything */
      Serial.println("--[RESULTS]--");
      Serial.println();
      Serial.print("time:\t\t");
      Serial.print(millis());
      Serial.println(" ms");
      Serial.println("hardware:\tESPiLight");
      Serial.print("pulse:\t\t");
      Serial.println(normalize(pulse, pulselen));
      Serial.print("rawlen:\t\t");
      Serial.println(length);
      Serial.printf("pulselen:\t");
      Serial.println(pulselen);
      Serial.println();
      Serial.println("Raw code:");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print(pulses[i]);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}

void setup() {
  Serial.begin(115200);
  // set callback funktion for raw messages
  rf.setPulseTrainCallBack(rfRawCallback);
  // inittilize receiver
  rf.initReceiver(RECEIVER_PIN);

  Serial.println(
      "Press and hold one of the buttons on your remote or wait until");
  Serial.println("another device such as a weather station has sent new codes");
  Serial.println(
      "The debugger will automatically reset itself after one second of");
  Serial.println(
      "failed leads. It will keep running until you explicitly stop it.");
}

void loop() {
  // process input queue and may fire calllback
  rf.loop();
  delay(10);
}
