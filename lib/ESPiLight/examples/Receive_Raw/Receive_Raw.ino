/*
 Basic ESPiLight receive raw (pulse train) signal example

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define RECEIVER_PIN 4  // any intterupt able pin
#define TRANSMITTER_PIN 13

ESPiLight rf(TRANSMITTER_PIN);  // use -1 to disable transmitter

// callback function. It is called on successfully received and parsed rc signal
void rfRawCallback(const uint16_t* codes, size_t length) {
  // print pulse lengths
  Serial.print("RAW signal: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print(codes[i]);
    Serial.print(' ');
  }
  Serial.println();

  // format of pilight USB Nano
  String data = rf.pulseTrainToString(codes, length);
  Serial.print("string format: ");
  Serial.print(data);
  Serial.println();
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
