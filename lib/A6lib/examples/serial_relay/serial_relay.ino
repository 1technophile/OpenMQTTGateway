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
    // Relay things between Serial and the module's SoftSerial.
    while (A6l.A6conn->available() > 0) {
        Serial.write(A6l.A6conn->read());
    }
    while (Serial.available() > 0) {
        A6l.A6conn->write(Serial.read());
    }
}
