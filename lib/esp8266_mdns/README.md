# esp8266_mdns
mDNS queries and responses on esp8266.
Or to describe it another way: An mDNS Client or Bonjour Client library for the esp8266.

This library aims to do the following:
 1. Give access to incoming mDNS packets and decode Question and Answer Records for commonly used record types.
 2. Allow Question and Answer Records for commonly used record types to be sent.

Future goals:
 1. Dynamic buffer paging. Currently one page is read from the network. If the mDNS packet is larger than that page size, any responses in the remainder are lost. (See MAX_PACKET_SIZE in mdns.h.)
 2. Automatic replies to incoming Questions.
 3. Automatic retries when sending packets according to rfc6762.

Requirements
------------
- An Espressif [ESP8266](http://www.esp8266.com/) WiFi enabled SOC.
- The [ESP8266 Arduino](https://github.com/esp8266/Arduino) environment.
- ESP8266WiFi library.
- MDNS support in your operating system/client machines:
  - For Mac OSX support is built in through Bonjour already.
  - For Linux, install [Avahi](http://avahi.org/).
  - For Windows, install [Bonjour](http://www.apple.com/support/bonjour/).

Usage
-----
Find information on how to add a library to your Arduino IDE [here](https://www.arduino.cc/en/Guide/Libraries).

The file [mdns.h](https://github.com/mrdunk/esp8266_mdns/blob/master/mdns.h) is well commented. Try looking there for information on specific methods.

To add a simple mDNS listener to an Aruino sketch which will display all mDNS packets over the serial console try the following:

```
// This sketch will display mDNS (multicast DNS) data seen on the network.

#include <ESP8266WiFi.h>
#include "mdns.h"

// When an mDNS packet gets parsed this optional callback gets called.
void packetCallback(const mdns::MDns* packet){
  packet->Display();
  packet->DisplayRawPacket();
}

// When an mDNS packet gets parsed this optional callback gets called once per Query.
// See mdns.h for definition of mdns::Query.
void queryCallback(const mdns::Query* query){
  query->Display();
}

// When an mDNS packet gets parsed this optional callback gets called once per Query.
// See mdns.h for definition of mdns::Answer.
void answerCallback(const mdns::Answer* answer){
  answer->Display();
}

// Initialise MDns.
// If you don't want the optional callbacks, just provide a NULL pointer as the callback.
mdns::MDns my_mdns(packetCallback, queryCallback, answerCallback);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // setting up Station AP
  WiFi.begin("your_wifi_ssid", "your_wifi_password");

  // Wait for connect to AP
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tries++;
    if (tries > 30) {
      break;
    }
  }
  Serial.println();
}

void loop() {
  my_mdns.loop();
}
```

A more complete example which sends an mDNS Question and parses Answers is available in esp8266_mdns/examples/mdns_test/ .

Troubleshooting
---------------
Run [Wireshark](https://www.wireshark.org/) on a machine connected to your wireless network to confirm what is actually in flight.
The following filter will return only mDNS packets: ```udp.port == 5353``` .
Any mDNS packets seen by Wireshark should also appear on the ESP8266 Serial console.
