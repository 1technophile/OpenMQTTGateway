/*
 Basic ESPiLight transmit RAW signal example

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define TRANSMITTER_PIN 13

ESPiLight rf(TRANSMITTER_PIN);

void setup() {
  Serial.begin(115200);

  int length = 0;
  uint16_t codes[MAXPULSESTREAMLENGTH];

  // get pulse train from string (format see: pilight USB Nano)
  length = rf.stringToPulseTrain(
      "c:102020202020202020220202020020202200202200202020202020220020202203;p:"
      "279,2511,1395,9486@",
      codes, MAXPULSESTREAMLENGTH);

  // transmit the pulse train
  rf.sendPulseTrain(codes, length);
}

// Toggle state of elro 800 switch evrey 2 s
void loop() {
  // stop
}
