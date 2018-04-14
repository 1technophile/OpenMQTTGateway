#include <A6lib.h>

#ifndef ESP8266
#define D0 0
#define D5 8
#define D6 7
#endif

// Instantiate the library with TxPin, RxPin.
A6lib A6l(D6, D5);

void setup() {
    Serial.begin(115200);

    delay(1000);

    // Power-cycle the module to reset it.
    A6l.powerCycle(D0);
    A6l.blockUntilReady(9600);
}

void loop() {
    Serial.println("Checking call status...");
    callInfo cinfo = A6l.checkCallStatus();
    Serial.println("Call status checked.");

    int sigStrength = A6l.getSignalStrength();
    Serial.print("Signal strength percentage: ");
    Serial.println(sigStrength);

    delay(5000);

    if (cinfo.number != NULL) {
        if (cinfo.direction == DIR_INCOMING && cinfo.number == "919999999999") {
            A6l.answer();
        } else {
            A6l.hangUp();
        }
        delay(1000);
    } else {
        Serial.println("No number yet.");
        delay(1000);
    }
}
