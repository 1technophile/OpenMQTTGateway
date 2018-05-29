/*
* Demo for RF remote switch receiver.
* For details, see RemoteReceiver.h!
*
* This sketch shows the received signals on the serial port.
* Connect the receiver to digital pin 2 on arduino and digital pin 1 on ESP8266.
* Detected codes example:
 Code: 8233372 Period: 273
 unit: 1
 groupBit: 0
 switchType: 0
*/

#include <NewRemoteReceiver.h>

void setup() {
  Serial.begin(115200);

  // Initialize receiver on interrupt 0 (= digital pin 2) for arduino uno, calls the callback "showCode"
  // after 1 identical codes have been received in a row. (thus, keep the button pressed
  // for a moment), on esp8266 use on interrupt 5 = digital pin 1
  //
  // See the interrupt-parameter of attachInterrupt for possible values (and pins)
  // to connect the receiver.

  // if you don't see codes try to reset your board after upload
  
    #ifdef ESP8266
      NewRemoteReceiver::init(5, 2, showCode);
    #else
      NewRemoteReceiver::init(0, 2, showCode);
    #endif
    Serial.println("Receiver initialized");    
}

void loop() {

}

// Callback function is called only when a valid code is received.
void showCode(unsigned int period, unsigned long address, unsigned long groupBit, unsigned long unit, unsigned long switchType) {

  // Print the received code.
  Serial.print("Code: ");
  Serial.print(address);
  Serial.print(" Period: ");
  Serial.println(period);
  Serial.print(" unit: ");
  Serial.println(unit);
  Serial.print(" groupBit: ");
  Serial.println(groupBit);
  Serial.print(" switchType: ");
  Serial.println(switchType);

}
