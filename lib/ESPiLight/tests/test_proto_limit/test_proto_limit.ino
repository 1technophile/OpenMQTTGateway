/*
 Basic ESPiLight protocol limiting

 https://github.com/puuu/espilight
*/

#include <ESPiLight.h>

#define PROTOCOL "elro_800_switch"
#define JMESSAGE "{\"systemcode\":17,\"unitcode\":1,\"on\":1}"

ESPiLight rf(-1);  // use -1 to disable transmitter

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

void setup() {
  Serial.begin(115200);
  // set callback funktion
  rf.setCallback(rfCallback);

  int length = 0;
  uint16_t pulses[MAXPULSESTREAMLENGTH];

  // pulse train from ppilight json message
  length = rf.createPulseTrain(pulses, PROTOCOL, JMESSAGE);

  // print available and enabled protocols
  Serial.print("Available protocols: ");
  Serial.println(rf.availableProtocols());
  Serial.print("Enabled protocols: ");
  Serial.println(rf.enabledProtocols());

  // parse pulse train multiple times
  for (int i = 0; i < 5; i++) {
    Serial.println();
    Serial.print("Decoding turn ");
    Serial.println(i);
    Serial.println();
    rf.parsePulseTrain(pulses, length);
    delay(10);
  }

  // Limit the protocol
  rf.limitProtocols("[\"" PROTOCOL "\"]");

  // print available and enabled protocols
  Serial.print("Available protocols: ");
  Serial.println(rf.availableProtocols());
  Serial.print("Enabled protocols: ");
  Serial.println(rf.enabledProtocols());

  // parse pulse train multiple times
  for (int i = 0; i < 5; i++) {
    Serial.println();
    Serial.print("Decoding turn ");
    Serial.println(i);
    Serial.println();
    rf.parsePulseTrain(pulses, length);
    delay(10);
  }

  // Reset the filter
  rf.limitProtocols("[]");

  // print available and enabled protocols
  Serial.print("Available protocols: ");
  Serial.println(rf.availableProtocols());
  Serial.print("Enabled protocols: ");
  Serial.println(rf.enabledProtocols());

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
