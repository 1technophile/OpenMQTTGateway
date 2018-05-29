#include <Arduino.h>
#include <SoftwareSerial.h>
#include "A6lib.h"


/////////////////////////////////////////////
// Public methods.
//

A6lib::A6lib(int transmitPin, int receivePin) {
#ifdef ESP8266
    A6conn = new SoftwareSerial(receivePin, transmitPin, false, 1024);
#else
    A6conn = new SoftwareSerial(receivePin, transmitPin, false);
#endif
A6conn->setTimeout(100);
}


A6lib::~A6lib() {
    delete A6conn;
}


// Block until the module is ready.
byte A6lib::blockUntilReady(long baudRate) {

    byte response = A6_NOTOK;
    while (A6_OK != response) {
        response = begin(baudRate);
        // This means the modem has failed to initialize and we need to reboot
        // it.
        if (A6_FAILURE == response) {
            return A6_FAILURE;
        }
        delay(1000);
        logln("Waiting for module to be ready...");
    }
    return A6_OK;
}


// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
byte A6lib::begin(long baudRate) {

    A6conn->flush();

    if (A6_OK != setRate(baudRate)) {
        return A6_NOTOK;
    }

    // Factory reset.
    A6command("AT&F", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Echo off.
    A6command("ATE0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Switch audio to headset.
    enableSpeaker(0);

    // Set caller ID on.
    A6command("AT+CLIP=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Set SMS to text mode.
    A6command("AT+CMGF=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Turn SMS indicators off.
    A6command("AT+CNMI=1,0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Set SMS storage to the GSM modem.
    if (A6_OK != A6command("AT+CPMS=SM,ME,SM", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL))
        // This may sometimes fail, in which case the modem needs to be
        // rebooted.
    {
        return A6_FAILURE;
    }

    // Set SMS character set.
    setSMScharset("UCS2");

    return A6_OK;
}


// Reboot the module by setting the specified pin HIGH, then LOW. The pin should
// be connected to a P-MOSFET, not the A6's POWER pin.
void A6lib::powerCycle(int pin) {
    logln("Power-cycling module...");

    powerOff(pin);
    delay(2000);

    powerOn(pin);
	delay(4000);

    powerOff(pin);
    // Give the module some time to settle.
    logln("Done, waiting for the module to initialize...");
    delay(20000);
    logln("Done.");

    A6conn->flush();
}


// Turn the modem power completely off.
void A6lib::powerOff(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}


// Turn the modem power on.
void A6lib::powerOn(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}


// Dial a number.
void A6lib::dial(String number) {
    char buffer[50];

    logln("Dialing number...");

    sprintf(buffer, "ATD%s;", number.c_str());
    A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Redial the last number.
void A6lib::redial() {
    logln("Redialing last number...");
    A6command("AT+DLST", "OK", "CONNECT", A6_CMD_TIMEOUT, 2, NULL);
}


// Answer a call.
void A6lib::answer() {
    A6command("ATA", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Hang up the phone.
void A6lib::hangUp() {
    A6command("ATH", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Check whether there is an active call.
callInfo A6lib::checkCallStatus() {
    char number[50];
    String response = "";
    uint32_t respStart = 0, matched = 0;
    callInfo cinfo = (const struct callInfo) {
        0
    };

    // Issue the command and wait for the response.
    A6command("AT+CLCC", "OK", "+CLCC", A6_CMD_TIMEOUT, 2, &response);

    // Parse the response if it contains a valid +CLCC.
    respStart = response.indexOf("+CLCC");
    if (respStart >= 0) {
        matched = sscanf(response.substring(respStart).c_str(), "+CLCC: %d,%d,%d,%d,%d,\"%s\",%d", &cinfo.index, &cinfo.direction, &cinfo.state, &cinfo.mode, &cinfo.multiparty, number, &cinfo.type);
        cinfo.number = String(number);
    }

    uint8_t comma_index = cinfo.number.indexOf('"');
    if (comma_index != -1) {
        logln("Extra comma found.");
        cinfo.number = cinfo.number.substring(0, comma_index);
    }

    return cinfo;
}


// Get the strength of the GSM signal.
int A6lib::getSignalStrength() {
    String response = "";
    uint32_t respStart = 0;
    int strength, error  = 0;

    // Issue the command and wait for the response.
    A6command("AT+CSQ", "OK", "+CSQ", A6_CMD_TIMEOUT, 2, &response);

    respStart = response.indexOf("+CSQ");
    if (respStart < 0) {
        return 0;
    }

    sscanf(response.substring(respStart).c_str(), "+CSQ: %d,%d",
           &strength, &error);

    // Bring value range 0..31 to 0..100%, don't mind rounding..
    strength = (strength * 100) / 31;
    return strength;
}


// Get the real time from the modem. Time will be returned as yy/MM/dd,hh:mm:ss+XX
String A6lib::getRealTimeClock() {
    String response = "";

    // Issue the command and wait for the response.
    A6command("AT+CCLK?", "OK", "yy", A6_CMD_TIMEOUT, 1, &response);
    int respStart = response.indexOf("+CCLK: \"") + 8;
    response.setCharAt(respStart - 1, '-');

    return response.substring(respStart, response.indexOf("\""));
}


// Send an SMS.
byte A6lib::sendSMS(String number, String text) {
    char ctrlZ[2] = { 0x1a, 0x00 };
    char buffer[100];

    if (text.length() > 159) {
        // We can't send messages longer than 160 characters.
        return A6_NOTOK;
    }

    log("Sending SMS to ");
    log(number);
    logln("...");

    sprintf(buffer, "AT+CMGS=\"%s\"", number.c_str());
    A6command(buffer, ">", "yy", A6_CMD_TIMEOUT, 2, NULL);
    delay(100);
    A6conn->println(text.c_str());
    A6conn->println(ctrlZ);
    A6conn->println();

    return A6_OK;
}


// Retrieve the number and locations of unread SMS messages.
int A6lib::getUnreadSMSLocs(int* buf, int maxItems) {
    return getSMSLocsOfType(buf, maxItems, "REC UNREAD");
}

// Retrieve the number and locations of all SMS messages.
int A6lib::getSMSLocs(int* buf, int maxItems) {
    return getSMSLocsOfType(buf, maxItems, "ALL");
}

// Retrieve the number and locations of all SMS messages.
int A6lib::getSMSLocsOfType(int* buf, int maxItems, String type) {
    String seqStart = "+CMGL: ";
    String response = "";

    String command = "AT+CMGL=\"";
    command += type;
    command += "\"";

    // Issue the command and wait for the response.
    byte status = A6command(command.c_str(), "\xff\r\nOK\r\n", "\r\nOK\r\n", A6_CMD_TIMEOUT, 2, &response);

    int seqStartLen = seqStart.length();
    int responseLen = response.length();
    int index, occurrences = 0;

    // Start looking for the +CMGL string.
    for (int i = 0; i < (responseLen - seqStartLen); i++) {
        // If we found a response and it's less than occurrences, add it.
        if (response.substring(i, i + seqStartLen) == seqStart && occurrences < maxItems) {
            // Parse the position out of the reply.
            sscanf(response.substring(i, i + 12).c_str(), "+CMGL: %u,%*s", &index);

            buf[occurrences] = index;
            occurrences++;
        }
    }
    return occurrences;
}

// Return the SMS at index.
SMSmessage A6lib::readSMS(int index) {
    String response = "";
    char buffer[30];

    // Issue the command and wait for the response.
    sprintf(buffer, "AT+CMGR=%d", index);
    A6command(buffer, "\xff\r\nOK\r\n", "\r\nOK\r\n", A6_CMD_TIMEOUT, 2, &response);

    char message[200];
    char number[50];
    char date[50];
    char type[10];
    int respStart = 0, matched = 0;
    SMSmessage sms = (const struct SMSmessage) {
        "", "", ""
    };

    // Parse the response if it contains a valid +CLCC.
    respStart = response.indexOf("+CMGR");
    if (respStart >= 0) {
        // Parse the message header.
        matched = sscanf(response.substring(respStart).c_str(), "+CMGR: \"REC %s\",\"%s\",,\"%s\"\r\n", type, number, date);
        sms.number = String(number);
        sms.date = String(date);
        // The rest is the message, extract it.
        sms.message = response.substring(strlen(type) + strlen(number) + strlen(date) + 24, response.length() - 8);
    }
    return sms;
}

// Delete the SMS at index.
byte A6lib::deleteSMS(int index) {
    char buffer[20];
    sprintf(buffer, "AT+CMGD=%d", index);
    return A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}

// Delete SMS with special flags
byte A6lib::deleteSMS(int index, int flag) {
    char buffer[20];
	String command = "AT+CMGD=";
    command += String(index);
    command += ",";
    command += String(flag);
    return A6command(command.c_str(), "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}  // AT+CMGD=1,4 delete all SMS from the storage area


// Set the SMS charset.
byte A6lib::setSMScharset(String charset) {
    char buffer[30];

    sprintf(buffer, "AT+CSCS=\"%s\"", charset.c_str());
    return A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Set the volume for the speaker. level should be a number between 5 and
// 8 inclusive.
void A6lib::setVol(byte level) {
    char buffer[30];

    // level should be between 5 and 8.
    level = _min(_max(level, 5), 8);
    sprintf(buffer, "AT+CLVL=%d", level);
    A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Enable the speaker, rather than the headphones. Pass 0 to route audio through
// headphones, 1 through speaker.
void A6lib::enableSpeaker(byte enable) {
    char buffer[30];

    // enable should be between 0 and 1.
    enable = _min(_max(enable, 0), 1);
    sprintf(buffer, "AT+SNFS=%d", enable);
    A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}



/////////////////////////////////////////////
// Private methods.
//


// Autodetect the connection rate.

long A6lib::detectRate() {
    unsigned long rate = 0;
    unsigned long rates[] = {9600, 115200};

    // Try to autodetect the rate.
    logln("Autodetecting connection rate...");
    for (int i = 0; i < countof(rates); i++) {
        rate = rates[i];
        A6conn->begin(rate);
        log("Trying rate ");
        log(rate);
        logln("...");

        delay(100);
        if (A6command("\rAT", "OK", "+CME", 2000, 2, NULL) == A6_OK) {
            return rate;
        }
    }

    logln("Couldn't detect the rate.");

    return A6_NOTOK;
}


// Set the A6 baud rate.
char A6lib::setRate(long baudRate) {
    int rate = 0;

    rate = detectRate();
    if (rate == A6_NOTOK) {
        return A6_NOTOK;
    }

    // The rate is already the desired rate, return.
    //if (rate == baudRate) return OK;

    logln("Setting baud rate on the module...");

    // Change the rate to the requested.
    char buffer[30];
    sprintf(buffer, "AT+IPR=%d", baudRate);
    A6command(buffer, "OK", "+IPR=", A6_CMD_TIMEOUT, 3, NULL);

    logln("Switching to the new rate...");
    // Begin the connection again at the requested rate.
    A6conn->begin(baudRate);
    logln("Rate set.");

    return A6_OK;
}


// Read some data from the A6 in a non-blocking manner.
String A6lib::read() {
    String reply = "";
    if (A6conn->available()) {
        reply = A6conn->readString();
    }

    // XXX: Replace NULs with \xff so we can match on them.
    for (int x = 0; x < reply.length(); x++) {
        if (reply.charAt(x) == 0) {
            reply.setCharAt(x, 255);
        }
    }
    return reply;
}


// Issue a command.
byte A6lib::A6command(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response) {
    byte returnValue = A6_NOTOK;
    byte count = 0;

    // Get rid of any buffered output.
    A6conn->flush();

    while (count < repetitions && returnValue != A6_OK) {
        log("Issuing command: ");
        logln(command);

        A6conn->write(command);
        A6conn->write('\r');

        if (A6waitFor(resp1, resp2, timeout, response) == A6_OK) {
            returnValue = A6_OK;
        } else {
            returnValue = A6_NOTOK;
        }
        count++;
    }
    return returnValue;
}


// Wait for responses.
byte A6lib::A6waitFor(const char *resp1, const char *resp2, int timeout, String *response) {
    unsigned long entry = millis();
    int count = 0;
    String reply = "";
    byte retVal = 99;
    do {
        reply += read();
		#ifdef ESP8266
			yield();
		#endif
    } while (((reply.indexOf(resp1) + reply.indexOf(resp2)) == -2) && ((millis() - entry) < timeout));

    if (reply != "") {
        log("Reply in ");
        log((millis() - entry));
        log(" ms: ");
        logln(reply);
    }

    if (response != NULL) {
        *response = reply;
    }

    if ((millis() - entry) >= timeout) {
        retVal = A6_TIMEOUT;
        logln("Timed out.");
    } else {
        if (reply.indexOf(resp1) + reply.indexOf(resp2) > -2) {
            logln("Reply OK.");
            retVal = A6_OK;
        } else {
            logln("Reply NOT OK.");
            retVal = A6_NOTOK;
        }
    }
    return retVal;
}
