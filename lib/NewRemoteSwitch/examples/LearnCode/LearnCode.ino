/*
 * Demo for RF remote switch receiver. 
 * This example is for the new KaKu / Home Easy type of remotes!
 *
 * For details, see NewRemoteReceiver.h!
 *
 * With this sketch you can control a LED connected to digital pin 13,
 * after the sketch learned the code. After start, the LED starts to blink,
 * until a valid code has been received. The led stops blinking. Now you
 * can control the LED with the remote.
 *
 * Note: only unit-switches are supported in this sketch, no group or dim.
 *
 * Set-up: connect the receiver to digital pin 2 and a LED to digital pin 13.
 */

#include <NewRemoteReceiver.h>

boolean codeLearned = false;
unsigned long learnedAddress;
byte learnedUnit;

void setup() {
  // LED-pin as output
  pinMode(13, OUTPUT);

  // Init a new receiver on interrupt pin 0, minimal 2 identical repeats, and callback set to processCode.
  NewRemoteReceiver::init(0, 2, processCode);
}

void loop() {
  // Blink led until a code has been learned
  if (!codeLearned) {
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
  }
}

// Callback function is called only when a valid code is received.
void processCode(NewRemoteCode receivedCode) {
  // A code has been received.
  // Do we already know the code?
  if (!codeLearned) {
    // No! Let's learn the received code.
    learnedAddress = receivedCode.address;
    learnedUnit = receivedCode.unit;
    codeLearned = true;    
  } else {
    // Yes!    
    // Is the received code identical to the learned code?
    if (receivedCode.address == learnedAddress && receivedCode.unit == learnedUnit) {
      // Yes!
      // Switch the LED off if the received code was "off".
      // Anything else (on, dim, on_with_dim) will switch the LED on.
      if (receivedCode.switchType == NewRemoteCode::off) {
        digitalWrite(13, LOW);
      } else {
        digitalWrite(13, HIGH);
      }
    }
  }
}
