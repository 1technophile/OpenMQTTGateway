/*
 Basic ESPiLight parsing test

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define PROTOCOL "elro_800_switch"
#define JMESSAGE "{\"systemcode\":17,\"unitcode\":1,\"on\":1}"

ESPiLight rf(-1);  // use -1 to disable transmitter

// callback function. It is called on successfully received and parsed rc signal
void rfRawCallback(const uint16_t *codes, size_t length) {
  // print pulse lengths
  printPulseTrain(codes, length);

  // format of pilight USB Nano
  String data = rf.pulseTrainToString(codes, length);
  Serial.print("string format: ");
  Serial.print(data);
  Serial.println();
}

// callback function. It is called on successfully received and parsed rc signal
void rfCallback(const String &protocol, const String &message, int status,
                size_t repeats, const String &deviceID) {
  Serial.print("parsed message [");
  Serial.print(protocol);  // protocoll used to parse
  Serial.print("][");
  Serial.print(deviceID);  // value of id key in json message
  Serial.print("] (");
  Serial.print(status);
  Serial.print(") ");
  Serial.print(message);  // message in json format
  Serial.println();
}

void printPulseTrain(const uint16_t *codes, int length) {
  Serial.print("RAW signal: ");
  for (int i = 0; i < length; i++) {
    Serial.print(codes[i]);
    Serial.print(' ');
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  // set callback funktion
  rf.setCallback(rfCallback);
  // set callback funktion for raw messages
  rf.setPulseTrainCallBack(rfRawCallback);

  int length = 0;
  uint16_t pulses[MAXPULSESTREAMLENGTH];

  // print free heap memory
  Serial.println();
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  // pulse train from ppilight json message
  length = rf.createPulseTrain(pulses, PROTOCOL, JMESSAGE);
  // print pulse lengths
  printPulseTrain(pulses, length);
  // format of pilight USB Nano
  String data = rf.pulseTrainToString(pulses, length);
  Serial.print("string format: ");
  Serial.print(data);
  Serial.println();

  // pilight USB Nano string to pulses
  length = rf.stringToPulseTrain(data, pulses, MAXPULSESTREAMLENGTH);
  // print pulse lengths
  printPulseTrain(pulses, length);
  // format of pilight USB Nano
  data = rf.pulseTrainToString(pulses, length);
  Serial.print("string format: ");
  Serial.print(data);
  Serial.println();

  // parse pulse train multiple times
  for (int i = 0; i < 5; i++) {
    Serial.println();
    Serial.print("Decoding turn ");
    Serial.println(i);
    Serial.println();
    rf.parsePulseTrain(pulses, length);
    delay(10);
  }
}

void loop() {
  // nothing
}
