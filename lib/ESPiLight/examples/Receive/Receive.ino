/*
 Basic ESPilight receive example

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define RECEIVER_PIN 4  // any intterupt able pin
#define TRANSMITTER_PIN 13

ESPiLight rf(TRANSMITTER_PIN);  // use -1 to disable transmitter

// callback function. It is called on successfully received and parsed rc signal
void rfCallback(const String &protocol, const String &message, int status,
                size_t repeats, const String &deviceID) {
  Serial.print("RF signal arrived [");
  Serial.print(protocol);  // protocoll used to parse
  Serial.print("][");
  Serial.print(deviceID);  // value of id key in json message
  Serial.print("] (");
  Serial.print(status);  // status of message, depending on repeat, either:
                         // FIRST   - first message of this protocoll within the
                         //           last 0.5 s
                         // INVALID - message repeat is not equal to the
                         //           previous message
                         // VALID   - message is equal to the previous message
                         // KNOWN   - repeat of a already valid message
  Serial.print(") ");
  Serial.print(message);  // message in json format
  Serial.println();

  // check if message is valid and process it
  if (status == VALID) {
    Serial.print("Valid message: [");
    Serial.print(protocol);
    Serial.print("] ");
    Serial.print(message);
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  // set callback funktion
  rf.setCallback(rfCallback);
  // inittilize receiver
  rf.initReceiver(RECEIVER_PIN);
}

void loop() {
  // process input queue and may fire calllback
  rf.loop();
  delay(10);
}
