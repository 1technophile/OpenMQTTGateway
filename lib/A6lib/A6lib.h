#ifndef A6lib_h
#define A6lib_h

#include <Arduino.h>
#include "SoftwareSerial.h"

#ifdef DEBUG
#define log(msg) Serial.print(msg)
#define logln(msg) Serial.println(msg)
#else
#define log(msg)
#define logln(msg)
#endif

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define A6_OK 0
#define A6_NOTOK 1
#define A6_TIMEOUT 2
#define A6_FAILURE 3

#define A6_CMD_TIMEOUT 5000


enum call_direction {
    DIR_OUTGOING = 0,
    DIR_INCOMING = 1
};

enum call_state {
    CALL_ACTIVE = 0,
    CALL_HELD = 1,
    CALL_DIALING = 2,
    CALL_ALERTING = 3,
    CALL_INCOMING = 4,
    CALL_WAITING = 5,
    CALL_RELEASE = 7
};

enum call_mode {
    MODE_VOICE = 0,
    MODE_DATA = 1,
    MODE_FAX = 2,
    MODE_VOICE_THEN_DATA_VMODE = 3,
    MODE_VOICE_AND_DATA_VMODE = 4,
    MODE_VOICE_AND_FAX_VMODE = 5,
    MODE_VOICE_THEN_DATA_DMODE = 6,
    MODE_VOICE_AND_DATA_DMODE = 7,
    MODE_VOICE_AND_FAX_FMODE = 8,
    MODE_UNKNOWN = 9
};

struct SMSmessage {
    String number;
    String date;
    String message;
};

struct callInfo {
    int index;
    call_direction direction;
    call_state state;
    call_mode mode;
    int multiparty;
    String number;
    int type;
};


class A6lib {
public:
    A6lib(int transmitPin, int receivePin);
    ~A6lib();

    byte begin(long baudRate);
    byte blockUntilReady(long baudRate);

    void powerCycle(int pin);
    void powerOn(int pin);
    void powerOff(int pin);

    void dial(String number);
    void redial();
    void answer();
    void hangUp();
    callInfo checkCallStatus();
    int getSignalStrength();

    byte sendSMS(String number, String text);
    int getUnreadSMSLocs(int* buf, int maxItems);
    int getSMSLocs(int* buf, int maxItems);
    int getSMSLocsOfType(int* buf, int maxItems, String type);
    SMSmessage readSMS(int index);
    byte deleteSMS(int index);
    byte deleteSMS(int index, int flag);
    byte setSMScharset(String charset);

    void setVol(byte level);
    void enableSpeaker(byte enable);

    String getRealTimeClock();

    SoftwareSerial *A6conn;
private:
    String read();
    byte A6command(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte A6waitFor(const char *resp1, const char *resp2, int timeout, String *response);
    long detectRate();
    char setRate(long baudRate);
};
#endif
