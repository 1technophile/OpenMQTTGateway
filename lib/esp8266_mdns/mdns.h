#ifndef MDNS_H
#define MDNS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DEBUG_STATISTICS      // Record how many incoming packets fitted into data_buffer.
//#define DEBUG_OUTPUT          // Send packet summaries to Serial.
//#define DEBUG_RAW             // Send HEX ans ASCII encoded raw packet to Serial.


#define MDNS_TYPE_A     0x0001
#define MDNS_TYPE_PTR   0x000C
#define MDNS_TYPE_HINFO 0x000D
#define MDNS_TYPE_TXT   0x0010
#define MDNS_TYPE_AAAA  0x001C
#define MDNS_TYPE_SRV   0x0021

#define MDNS_TARGET_PORT 5353
#define MDNS_SOURCE_PORT 5353
#define MDNS_TTL 255

// Make this as big as memory limitations allow.
// This default value can be overridden using the max_packet_size_ parameter of
// MDns().
#define MAX_PACKET_SIZE 1024

// The mDNS spec says this should never be more than 256 (including trailing '\0').
#define MAX_MDNS_NAME_LEN 256  

namespace mdns{

// A single mDNS Query.
typedef struct Query{
#ifdef DEBUG_OUTPUT
  unsigned int buffer_pointer;            // Position of Answer in packet. (Used for debugging only.)
#endif
  char qname_buffer[MAX_MDNS_NAME_LEN];   // Question Name: Contains the object, domain or zone name.
  unsigned int qtype;                     // Question Type: Type of question being asked by client.
  unsigned int qclass;                    // Question Class: Normally the value 1 for Internet (“IN”)
  bool unicast_response;                  // 
  bool valid;                             // False if problems were encountered decoding packet.

  void Display() const;                   // Display a summary of this Answer on Serial port.
} Query;

// A single mDNS Answer.
typedef struct Answer{
#ifdef DEBUG_OUTPUT
  unsigned int buffer_pointer;          // Position of Answer in packet. (Used for debugging only.)
#endif
  char name_buffer[MAX_MDNS_NAME_LEN];  // object, domain or zone name.
  char rdata_buffer[MAX_MDNS_NAME_LEN]; // The data portion of the resource record.
  unsigned int rrtype;                  // ResourceRecord Type.
  unsigned int rrclass;                 // ResourceRecord Class: Normally the value 1 for Internet (“IN”)
  unsigned long int rrttl;              // ResourceRecord Time To Live: Number of seconds ths should be remembered.
  bool rrset;                           // Flush cache of records matching this name.
  bool valid;                           // False if problems were encountered decoding packet.

  void Display() const ;                // Display a summary of this Answer on Serial port.
} Answer;

class MDns {
 private:
 public:
  // Simple constructor does not fire any callbacks on incoming data.
  // Default incoming data_buffer size is used.
  MDns() : MDns(NULL, NULL, NULL, MAX_PACKET_SIZE) {}

  // Simple constructor does not fire any callbacks on incoming data.
  // Args:
  //   max_packet_size_ : Set the data_buffer size allocated to store incoming packets.
  MDns(int max_packet_size_) : MDns(NULL, NULL, NULL, max_packet_size_) {}
  
  // Constructor takes callbacks which fire when mDNS data arrives.
  // Args:
  //   p_packet_function : Callback fires for every mDNS packet that arrives.
  //   p_query_function : Callback fires for every mDNS Query that arrives as part of a packet.
  //   p_answer_function : Callback fires for every mDNS Answer that arrives as part of a packet.
  MDns(std::function<void(const MDns*)> p_packet_function, 
       std::function<void(const Query*)> p_query_function, 
       std::function<void(const Answer*)> p_answer_function) :
    MDns(p_packet_function, p_query_function, p_answer_function, MAX_PACKET_SIZE) { }

  // Constructor takes callbacks which fire when mDNS data arrives.
  // Args:
  //   p_packet_function : Callback fires for every mDNS packet that arrives.
  //   p_query_function : Callback fires for every mDNS Query that arrives as part of a packet.
  //   p_answer_function : Callback fires for every mDNS Answer that arrives as part of a packet.
  //   max_packet_size_ : Set the data_buffer size allocated to store incoming packets.
  MDns(std::function<void(const MDns*)> p_packet_function, 
       std::function<void(const Query*)> p_query_function, 
       std::function<void(const Answer*)> p_answer_function,
       int max_packet_size_) :
#ifdef DEBUG_STATISTICS
       buffer_size_fail(0),
       largest_packet_seen(0),
       packet_count(0),
#endif
       p_packet_function_(p_packet_function),
       p_query_function_(p_query_function),
       p_answer_function_(p_answer_function),
       buffer_pointer(0),
       data_buffer(new byte[max_packet_size_]),
       max_packet_size(max_packet_size_)
       { 
         this->startUdpMulticast();
       };

  // Constructor can be passed the buffer to hold the mDNS data.
  // This way the potentially large buffer can be shared with other processes.
  // Args:
  //   p_packet_function : Callback fires for every mDNS packet that arrives.
  //   p_query_function : Callback fires for every mDNS Query that arrives as part of a packet.
  //   p_answer_function : Callback fires for every mDNS Answer that arrives as part of a packet.
  //   max_packet_size_ : Set the data_buffer size allocated to store incoming packets.
  MDns(std::function<void(const MDns*)> p_packet_function, 
       std::function<void(const Query*)> p_query_function,
       std::function<void(const Answer*)> p_answer_function,
       byte* data_buffer_,
       int max_packet_size_) :
#ifdef DEBUG_STATISTICS
       buffer_size_fail(0),
       largest_packet_seen(0),
       packet_count(0),
#endif
       p_packet_function_(p_packet_function),
       p_query_function_(p_query_function),
       p_answer_function_(p_answer_function),
       buffer_pointer(0),
       data_buffer(data_buffer_),
       max_packet_size(max_packet_size_)
       { 
         this->startUdpMulticast();
       };

  ~MDns();

  // Call this regularly to check for an incoming packet.
  bool loop();
  // Deprecated. Use loop() instead.
  bool Check(){
    return loop();
  }

  // Send this MDns packet.
  void Send() const;

  // Resets everything to represent an empty packet.
  // Do this before building a packet for sending.
  void Clear();

  // Add a query to packet prior to sending.
  // May only be done before any Answers have been added.
  bool AddQuery(const Query& query);

  // Add an answer to packet prior to sending.
  bool AddAnswer(const Answer& answer);
  
  // Display a summary of the packet on Serial port.
  void Display() const;
  
  // Display the raw packet in HEX and ASCII.
  void DisplayRawPacket() const;
 
#ifdef DEBUG_STATISTICS
  // Counter gets increased every time an incoming mDNS packet arrives that does
  // not fit in the data_buffer.
  unsigned int buffer_size_fail;

  // Track the largest mDNS packet that has arrived.
  // Useful for knowing what size to make data_buffer.
  unsigned int largest_packet_seen;

  // How many mDNS packets have arrived so far.
  unsigned int packet_count;
#endif
 private:
  // Initializes udp multicast
  void startUdpMulticast();

  void Parse_Query(Query& query);
  void Parse_Answer(Answer& answer);
  unsigned int PopulateName(const char* name_buffer);
  void PopulateAnswerResult(Answer* answer);

  // Pointer to function that gets called for every incoming mDNS packet.
  std::function<void(const MDns*)> p_packet_function_;

  // Pointer to function that gets called for every incoming query.
  std::function<void(const Query*)> p_query_function_;

  // Pointer to function that gets called for every incoming answer.
  std::function<void(const Answer*)> p_answer_function_;

  // Position in data_buffer while processing packet.
  unsigned int buffer_pointer;

  // Buffer containing mDNS packet.
  byte* data_buffer;

  // Buffer size for incoming MDns packet.
  unsigned int max_packet_size;

  // Size of mDNS packet.
  unsigned int data_size;

  // Query or Answer
  bool type;

  // Whether more follows in another packet.
  bool truncated;

  // Number of Qeries in the packet.
  unsigned int query_count;
  
  // Number of Answers in the packet.
  unsigned int answer_count;
  
  unsigned int ns_count;
  unsigned int ar_count;
};


// Display a byte on serial console in hexadecimal notation,
// padding with leading zero if necisary to provide evenly tabulated display data.
void PrintHex(unsigned char data);

// Extract Name from DNS data. Will follow pointers used by Message Compression.
// TODO Check for exceeding packet size.
int nameFromDnsPointer(char* p_name_buffer, int name_buffer_pos, const int name_buffer_len, 
    const byte* p_packet_buffer, int packet_buffer_pos);
int nameFromDnsPointer(char* p_name_buffer, int name_buffer_pos, const int name_buffer_len,
    const byte* p_packet_buffer, int packet_buffer_pos, const bool recurse);

bool writeToBuffer(const byte value, char* p_name_buffer, int* p_name_buffer_pos, const int name_buffer_len);

int parseText(char* data_buffer, const int data_buffer_len, int const data_len,
    const byte* p_packet_buffer, int packet_buffer_pos);

} // namespace mdns

#endif  // MDNS_H
