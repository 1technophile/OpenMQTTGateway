#include <A6lib.h>

#ifdef ESP8266
#define D0 0
#define D5 8
#define D6 7
#endif

// Instantiate the library with TxPin, RxPin.
A6lib A6l(D6, D5);

int unreadSMSLocs[30] = {0};
int unreadSMSNum = 0;
SMSmessage sms;

void setup() {
    Serial.begin(115200);

    delay(1000);

    // Power-cycle the module to reset it.
    A6l.powerCycle(D0);
    A6l.blockUntilReady(9600);
}

void loop() {
    String myNumber = "+1132352890";

    callInfo cinfo = A6l.checkCallStatus();
    if (cinfo.direction == DIR_INCOMING) {
        if (myNumber.endsWith(cinfo.number)) {
            // If the number that sent the SMS is ours, reply.
            A6l.sendSMS(myNumber, "I can't come to the phone right now, I'm a machine.");
            A6l.hangUp();
        }

        // Get the memory locations of unread SMS messages.
        unreadSMSNum = A6l.getUnreadSMSLocs(unreadSMSLocs, 30);

        for (int i = 0; i < unreadSMSNum; i++) {
            Serial.print("New message at index: ");
            Serial.println(unreadSMSLocs[i], DEC);

            sms = A6l.readSMS(unreadSMSLocs[i]);
            Serial.println(sms.number);
            Serial.println(sms.date);
            Serial.println(sms.message);
        }
        delay(1000);
    }
}
