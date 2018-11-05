/*
 Basic ESPiLight transmit example

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define TRANSMITTER_PIN 13

ESPiLight rf(TRANSMITTER_PIN);

void setup() { Serial.begin(115200); }

// Toggle state of elro 800 switch evrey 2 s
void loop() {
  rf.send("elro_800_switch", "{\"systemcode\":17,\"unitcode\":1,\"on\":1}");
  delay(2000);
  rf.send("elro_800_switch", "{\"systemcode\":17,\"unitcode\":1,\"off\":1}");
  delay(2000);
}
