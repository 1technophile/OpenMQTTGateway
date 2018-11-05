/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

   This files enables to set your parameter for the radiofrequency gateways (ZgatewayRF and ZgatewayRF2) with RCswitch and newremoteswitch library

    Copyright: (c)Florian ROBERT

    This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*----------------------RFM69 topics & parameters -------------------------*/
// Topic where the message from RFM69 will be published by the gateway,
// appended with the nodeID of the sender
#define subjectRFM69toMQTT  Base_Topic Gateway_Name "/RFM69toMQTT"

// Topic subscribed by the gateway. Messages received will be sent to RFM69
#define subjectMQTTtoRFM69  Base_Topic Gateway_Name "/commands/MQTTtoRFM69"
#define RFM69receiverKey "RCV_" // receiver id will be defined if a subject contains RFM69receiverKey followed by a value of 3 digits
// On reception of an ack from RFM69, the message that has been sent is published here
#define subjectGTWRFM69toMQTT  Base_Topic Gateway_Name "/RFM69toMQTT/acked"
#define defaultRFM69ReceiverId 99

// Default values
const char PROGMEM ENCRYPTKEY[] = "sampleEncryptKey";
const char PROGMEM MDNS_NAME[] = "rfm69gw1";
const char PROGMEM MQTT_BROKER[] = "raspi2";
const char PROGMEM RFM69AP_NAME[] = "RFM69-AP";
#define NETWORKID     200  //the same on all nodes that talk to each other
#define NODEID        10

//Match frequency to the hardware version of the radio
#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY      RF69_915MHZ
#define IS_RFM69HCW    true // set to 'true' if you are using an RFM69HCW module
#define POWER_LEVEL    31

/*-------------------PIN DEFINITIONS----------------------*/
#if defined(ESP8266)
  #define RFM69_CS      D1
  #define RFM69_IRQ     D8   // GPIO15/D8
  #define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
  #define RFM69_RST     D4   // GPIO02/D4
#elif defined(ESP32)
  #define RFM69_CS      1
  #define RFM69_IRQ     8   // GPIO15/D8
  #define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
  #define RFM69_RST     4   // GPIO02/D4
#else
  //RFM69 not tested with arduino
  #define RFM69_CS      10
  #define RFM69_IRQ     0
  #define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
  #define RFM69_RST     9
#endif
