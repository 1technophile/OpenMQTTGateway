/*
 * This sketch will query the network for hosts providing the service defined by
 * QUESTION_SERVICE and will parse any replies with the aim of saving port and
 * network address in the hosts array.
 */


#include <mdns.h>

#include "secrets.h"  // Contains the following:
// const char* ssid = "Get off my wlan";      //  your network SSID (name)
// const char* pass = "secretwlanpass";       // your network password


#define QUESTION_SERVICE "_mqtt._tcp.local"

// Make this value as large as available ram allows.
#define MAX_MDNS_PACKET_SIZE 512

#define MAX_HOSTS 4
#define HOSTS_SERVICE_NAME 0
#define HOSTS_PORT 1
#define HOSTS_HOST_NAME 2
#define HOSTS_ADDRESS 3
String hosts[MAX_HOSTS][4];  // Array containing information about hosts received over mDNS.


// When an mDNS packet gets parsed this callback gets called once per Query.
// See mdns.h for definition of mdns::Query.
void answerCallback(const mdns::Answer* answer) {

  // A typical PTR record matches service to a human readable name.
  // eg:
  //  service: _mqtt._tcp.local
  //  name:    Mosquitto MQTT server on twinkle.local
  if (answer->rrtype == MDNS_TYPE_PTR and strstr(answer->name_buffer, QUESTION_SERVICE) != 0) {
    unsigned int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_SERVICE_NAME] == answer->rdata_buffer) {
        // Already in hosts[][].
        break;
      }
      if (hosts[i][HOSTS_SERVICE_NAME] == "") {
        // This hosts[][] entry is still empty.
        hosts[i][HOSTS_SERVICE_NAME] = answer->rdata_buffer;
        break;
      }
    }
    if (i == MAX_HOSTS) {
      Serial.print(" ** ERROR ** No space in buffer for ");
      Serial.print('"');
      Serial.print(answer->name_buffer);
      Serial.print('"');
      Serial.print("  :  ");
      Serial.print('"');
      Serial.println(answer->rdata_buffer);
      Serial.print('"');
    }
  }

  // A typical SRV record matches a human readable name to port and FQDN info.
  // eg:
  //  name:    Mosquitto MQTT server on twinkle.local
  //  data:    p=0;w=0;port=1883;host=twinkle.local
  if (answer->rrtype == MDNS_TYPE_SRV) {
    unsigned int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_SERVICE_NAME] == answer->name_buffer) {
        // This hosts entry matches the name of the host we are looking for
        // so parse data for port and hostname.
        char* port_start = strstr(answer->rdata_buffer, "port=");
        if (port_start) {
          port_start += 5;
          char* port_end = strchr(port_start, ';');
          char port[1 + port_end - port_start];
          strncpy(port, port_start, port_end - port_start);
          port[port_end - port_start] = '\0';

          if (port_end) {
            char* host_start = strstr(port_end, "host=");
            if (host_start) {
              host_start += 5;
              hosts[i][HOSTS_PORT] = port;
              hosts[i][HOSTS_HOST_NAME] = host_start;
            }
          }
        }
        break;
      }
    }
    if (i == MAX_HOSTS) {
      Serial.print(" Did not find ");
      Serial.print('"');
      Serial.print(answer->name_buffer);
      Serial.print('"');
      Serial.println(" in hosts buffer.");
    }
  }

  // A typical A record matches an FQDN to network ipv4 address.
  // eg:
  //   name:    twinkle.local
  //   address: 192.168.192.9
  if (answer->rrtype == MDNS_TYPE_A) {
    int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_HOST_NAME] == answer->name_buffer) {
        hosts[i][HOSTS_ADDRESS] = answer->rdata_buffer;
        break;
      }
    }
    if (i == MAX_HOSTS) {
      Serial.print(" Did not find ");
      Serial.print('"');
      Serial.print(answer->name_buffer);
      Serial.print('"');
      Serial.println(" in hosts buffer.");
    }
  }

  Serial.println();
  for (int i = 0; i < MAX_HOSTS; ++i) {
    if (hosts[i][HOSTS_SERVICE_NAME] != "") {
      Serial.print(">  ");
      Serial.print(hosts[i][HOSTS_SERVICE_NAME]);
      Serial.print("    ");
      Serial.print(hosts[i][HOSTS_PORT]);
      Serial.print("    ");
      Serial.print(hosts[i][HOSTS_HOST_NAME]);
      Serial.print("    ");
      Serial.println(hosts[i][HOSTS_ADDRESS]);
    }
  }
}


// buffer can be used bu other processes that need a large chunk of memory.
byte buffer[MAX_MDNS_PACKET_SIZE];
mdns::MDns my_mdns(NULL, NULL, answerCallback, buffer, MAX_MDNS_PACKET_SIZE);


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // setting up Station AP
  WiFi.begin(ssid, pass);

  // Wait for connect to AP
  Serial.print("[Connecting]");
  Serial.print(ssid);
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

  printWifiStatus();

  Serial.println("Connected to wifi");


  // Query for all host information for a paticular service. ("_mqtt" in this case.)
  my_mdns.Clear();
  struct mdns::Query query_mqtt;
  strncpy(query_mqtt.qname_buffer, QUESTION_SERVICE, MAX_MDNS_NAME_LEN);
  query_mqtt.qtype = MDNS_TYPE_PTR;
  query_mqtt.qclass = 1;    // "INternet"
  query_mqtt.unicast_response = 0;
  my_mdns.AddQuery(query_mqtt);
  my_mdns.Send();

  /*
  // Query for all service types on network.
  my_mdns.Clear();
  struct mdns::Query query_services;
  strncpy(query_services.qname_buffer, "_services._dns-sd._udp.local", MAX_MDNS_NAME_LEN);
  query_services.qtype = MDNS_TYPE_PTR;
  query_services.qclass = 1;    // "INternet"
  query_services.unicast_response = 0;
  my_mdns.AddQuery(query_services);
  my_mdns.Send();*/

}

unsigned int last_packet_count = 0;
void loop()
{
  my_mdns.loop();

#ifdef DEBUG_STATISTICS
  // Give feedback on the percentage of incoming mDNS packets that fitted in buffer.
  // Useful for tuning the buffer size to make best use of available memory.
  if(last_packet_count != my_mdns.packet_count && my_mdns.packet_count != 0){
    last_packet_count = my_mdns.packet_count;
    Serial.print("mDNS decode success rate: ");
    Serial.print(100 - (100 * my_mdns.buffer_size_fail / my_mdns.packet_count));
    Serial.print("%\nLargest packet size: ");
    Serial.println(my_mdns.largest_packet_seen);
  }
#endif

  // mDNS not using buffer outside my_mdns.loop() so it can be used for other tasks.
  strncpy((char*)buffer,
          "<html><head>Some webpage that needs a large buffer</head>"
          "<body>big content...</body></html>",
          MAX_MDNS_PACKET_SIZE);
  // Display buffer here....
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
