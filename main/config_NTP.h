/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

  This module enables NTP support in OpenMQTTGateway

*/
#ifndef config_NTP_h
#define config_NTP_h

#include <Ticker.h>
#include <simpleDSTadjust.h>

// Settings for Central European Time
//#define UTC_OFFSET +1
//struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
//struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour

// Settings for Central Standard Time (North America)
#define UTC_OFFSET -6
struct dstRule StartRule = {"CDT", Second, Sun, Mar, 2, 3600}; // Central Daylight time = UTC/GMT -5 hours
struct dstRule EndRule = {"CST", First, Sun, Nov, 1, 0};       // Central Standard time = UTC/GMT -6 hour

// change for different NTP (time servers)
//#define NTP_SERVERS "0.ch.pool.ntp.org", "1.ch.pool.ntp.org", "2.ch.pool.ntp.org"
#define NTP_SERVERS "0.us.pool.ntp.org", "1.us.pool.ntp.org", "2.us.pool.ntp.org"

// August 1st, 2018
#define NTP_MIN_VALID_EPOCH 1533081600

// Update time from NTP server every 5 hours
#define NTP_UPDATE_INTERVAL_SEC 5*3600

// Initialize a few things
void updateTime();
void printTime(time_t offset);
void secTicker();
time_t dstOffset = 0;
Ticker ticker1;
int32_t tick;
bool updateNTP = false;

// Define our DST rules
simpleDSTadjust dstAdjusted(StartRule, EndRule);

#endif
