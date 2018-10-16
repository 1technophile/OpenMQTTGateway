/*
 Basic ESPiLight pilight_raw example

 This example mimic the output of the piligh_raw tool.

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define RECEIVER_PIN 4  // any intterupt able pin
#define TRANSMITTER_PIN 13

ESPiLight rf(TRANSMITTER_PIN);  // use -1 to disable transmitter

// callback function. It is called on successfully received and parsed rc signal
void rfRawCallback(const uint16_t* pulses, size_t length) {
  Serial.print("ESPiLight:");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print(" ");
    Serial.print(pulses[i]);
    if (pulses[i] > 5100) {
      Serial.printf(" -# ");
      Serial.println(i);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // set callback funktion for raw messages
  rf.setPulseTrainCallBack(rfRawCallback);
  // inittilize receiver
  rf.initReceiver(RECEIVER_PIN);
}

void loop() {
  // process input queue and may fire calllback
  rf.loop();
  delay(10);
}
