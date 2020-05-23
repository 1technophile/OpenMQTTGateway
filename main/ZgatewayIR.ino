/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal

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
#include "User_config.h"

#ifdef ZgatewayIR

#if defined(ESP8266) || defined(ESP32)
#include <IRremoteESP8266.h>
#include <IRsend.h> // Needed if you want to send IR commands.
#include <IRrecv.h> // Needed if you want to receive IR commands.
#include <IRutils.h>
#ifdef DumpMode // in dump mode we increase the size of the buffer to catch big codes
IRrecv irrecv(IR_RECEIVER_PIN, 1024, 15U, true);
#else
IRrecv irrecv(IR_RECEIVER_PIN);
#endif
IRsend irsend(IR_EMITTER_PIN);
#else
#include <IRremote.h>
IRrecv irrecv(IR_RECEIVER_PIN);
IRsend irsend; //connect IR emitter pin to D9 on arduino, you need to comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library IRremote.h so as to free pin D3 for RF RECEIVER PIN
#endif

// IR protocol bits definition for Arduino (for ESP9266 they are defined in IRRemoteESP8266.h)
#ifndef NEC_BITS
#define NEC_BITS 32U
#endif
#ifndef SAMSUNG_BITS
#define SAMSUNG_BITS 32U
#endif
#ifndef SHARP_BITS
#define SHARP_ADDRESS_BITS 5U
#define SHARP_COMMAND_BITS 8U
#define SHARP_BITS (SHARP_ADDRESS_BITS + SHARP_COMMAND_BITS + 2) // 15U
#endif
#ifndef RC5_BITS
#define RC5_RAW_BITS 14U
#define RC5_BITS RC5_RAW_BITS - 2U
#endif
#ifndef DISH_BITS
#define DISH_BITS 16U
#endif
#ifndef SONY_12_BITS
#define SONY_12_BITS 12U
#endif
#ifndef LG_BITS
#define LG_BITS 28U
#endif
#ifndef WHYNTER_BITS
#define WHYNTER_BITS 32U
#endif

// The function below comes from IRMQTTServer.INO on IRremoteESP8266 project from @crankyoldgit
uint64_t getUInt64fromHex(char const *str) {
  uint64_t result = 0;
  uint16_t offset = 0;
  // Skip any leading '0x' or '0X' prefix.
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) offset = 2;
  for (; isxdigit((unsigned char)str[offset]); offset++) {
    char c = str[offset];
    result *= 16;
    if (isdigit(c))
      result += c - '0';  // '0' .. '9'
    else if (isupper(c))
      result += c - 'A' + 10;  // 'A' .. 'F'
    else
      result += c - 'a' + 10;  // 'a' .. 'f'
  }
  return result;
}

void setupIR()
{
//IR init parameters
#if defined(ESP8266) || defined(ESP32)
  irsend.begin();
#endif

  irrecv.enableIRIn(); // Start the receiver

  Log.notice(F("IR_EMITTER_PIN: %d " CR), IR_EMITTER_PIN);
  Log.notice(F("IR_RECEIVER_PIN: %d " CR), IR_RECEIVER_PIN);
  Log.trace(F("ZgatewayIR setup done " CR));
}

void IRtoMQTT()
{
  decode_results results;

  if (irrecv.decode(&results))
  {
    Log.trace(F("Creating IR buffer" CR));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject &IRdata = jsonBuffer.createObject();

    Log.trace(F("Rcv. IR" CR));
#ifdef ESP32
    String taskMessage = "Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    //trc(taskMessage);
#endif
    IRdata.set("value", (unsigned long long)(results.value));
    IRdata.set("protocol", (int)(results.decode_type));
    IRdata.set("bits", (int)(results.bits));
#if defined(ESP8266) || defined(ESP32) //resultToHexidecimal is only available with IRremoteESP8266
    String hex = resultToHexidecimal(&results);
    IRdata.set("hex", (char *)hex.c_str());
    IRdata.set("protocol_name", (char *)(typeToString(results.decode_type, false)).c_str());
#endif
    String rawCode = "";
    // Dump data
    for (uint16_t i = 1; i < results.rawlen; i++)
    {
#if defined(ESP8266) || defined(ESP32)
      if (i % 100 == 0)
        yield(); // Preemptive yield every 100th entry to feed the WDT.
      rawCode = rawCode + (results.rawbuf[i] * RAWTICK);
#else
      rawCode = rawCode + (results.rawbuf[i] * USECPERTICK);
#endif
      if (i < results.rawlen - 1)
        rawCode = rawCode + ","; // ',' not needed on last one
    }
    //trc(rawCode);
    IRdata.set("raw", rawCode);
// if needed we directly resend the raw code
#ifdef RawDirectForward
#if defined(ESP8266) || defined(ESP32)
    uint16_t rawsend[results.rawlen];
    for (uint16_t i = 1; i < results.rawlen; i++)
    {
      if (i % 100 == 0)
        yield(); // Preemptive yield every 100th entry to feed the WDT.
#else
    unsigned int rawsend[results.rawlen];
    for (int i = 1; i < results.rawlen; i++)
    {
#endif
      rawsend[i] = results.rawbuf[i];
    }
    irsend.sendRaw(rawsend, results.rawlen, RawFrequency);
    Log.trace(F("raw redirected" CR));
#endif
    irrecv.resume(); // Receive the next value
    unsigned long long MQTTvalue = IRdata.get<unsigned long long>("value");
    //trc(MQTTvalue);
    if ((pubIRunknownPrtcl == false && IRdata.get<int>("protocol") == -1))
    { // don't publish unknown IR protocol 
      Log.notice(F("--no pub unknwn prt--" CR));
    }
    else if (!isAduplicate(MQTTvalue) && MQTTvalue != 0)
    { // conditions to avoid duplications of IR -->MQTT
      Log.trace(F("Adv data IRtoMQTT" CR));
      pub(subjectIRtoMQTT, IRdata);
      Log.trace(F("Store val: %D" CR), MQTTvalue);
      storeValue(MQTTvalue);
      if (repeatIRwMQTT)
      {
        Log.trace(F("Pub. IR for rpt" CR));
        pubMQTT(subjectMQTTtoIR, MQTTvalue);
      }
    }
  }
}

#ifdef jsonReceiving
void MQTTtoIR(char *topicOri, JsonObject &IRdata)
{

  if (cmpToMainTopic(topicOri, subjectMQTTtoIR))
  {
    Log.trace(F("MQTTtoIR json" CR));
    uint64_t data = IRdata["value"];
    const char *raw = IRdata["raw"];
    const char *hex = IRdata["hex"];
    if (hex) {// we privilegiate the hex usage over the value one (less risk of error)
      Log.trace(F("hex: %s" CR), hex);
      data = getUInt64fromHex(hex);
    }
    if (data != 0 || raw)
    {
      Log.trace(F("MQTTtoIR value || raw  detected" CR));
      bool signalSent = false;
      const char *protocol_name = IRdata["protocol_name"];
      unsigned int valueBITS = IRdata["bits"] | 0;
      uint16_t valueRPT = IRdata["repeat"] | repeatIRwNumber;

      if (raw)
      {
        Log.trace(F("Raw: %s" CR), raw);
        unsigned int s = strlen(raw);
        //number of "," value count
        int count = 0;
        for (int i = 0; i < s; i++)
        {
          if (raw[i] == ',')
          {
            count++;
          }
        }
#ifdef IR_GC
        if (strcmp(protocol_name, "GC") == 1)
        { // sending GC data from https://irdb.globalcache.com
          Log.trace(F("GC" CR));
          //buffer allocation from char datacallback
          uint16_t GC[count + 1];
          String value = "";
          int j = 0;
          for (int i = 0; i < s; i++)
          {
            if (raw[i] != ',')
            {
              value = value + String(raw[i]);
            }
            if ((raw[i] == ',') || (i == s - 1))
            {
              GC[j] = value.toInt();
              value = "";
              j++;
            }
          }
          irsend.sendGC(GC, j);
          signalSent = true;
        }
#endif
#ifdef IR_RAW
        if (strcmp(protocol_name, "Raw") == 1)
        { // sending Raw data
          Log.trace(F("Raw" CR));
//buffer allocation from char datacallback
#if defined(ESP8266) || defined(ESP32)
          uint16_t Raw[count + 1];
#else
          unsigned int Raw[count + 1];
#endif
          String value = "";
          int j = 0;
          for (int i = 0; i < s; i++)
          {
            if (raw[i] != ',')
            {
              value = value + String(raw[i]);
            }
            if ((raw[i] == ',') || (i == s - 1))
            {
              Raw[j] = value.toInt();
              value = "";
              j++;
            }
          }
          irsend.sendRaw(Raw, j, RawFrequency);
          signalSent = true;
        }
#endif
      }
      else if (protocol_name && (strcmp(protocol_name, "NEC") != 1))
      {
        Log.trace(F("Using Identified Protocol: %s  bits: %d repeat: %d" CR), protocol_name, valueBITS, valueRPT);
        signalSent = sendIdentifiedProtocol(protocol_name, data, hex, valueBITS, valueRPT);
      }
      else
      {
        Log.trace(F("Using NEC protocol" CR));
        Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
          valueBITS = NEC_BITS;
#if defined(ESP8266) || defined(ESP32)
        irsend.sendNEC(data, valueBITS, valueRPT);
#else
        for (int i = 0; i <= valueRPT; i++)
          irsend.sendNEC(data, valueBITS);
#endif
        signalSent = true;
      }
      if (signalSent)
      { // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
        Log.notice(F("MQTTtoIR OK" CR));
        pub(subjectGTWIRtoMQTT, IRdata);
      }
      irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
    }
    else
    {
      Log.error(F("MQTTtoIR failed json read" CR));
    }
  }
}
#endif


bool sendIdentifiedProtocol(const char *protocol_name, unsigned long long data, const char *hex, unsigned int valueBITS, uint16_t valueRPT)
{
  uint8_t dataarray[valueBITS];
  if(hex)
  {
    const char  *ptr = NULL;
    (strstr(hex, "0x") != NULL) ? ptr = hex += 2 : ptr = hex;
    for (int i = 0; i < sizeof dataarray/sizeof *dataarray; i++) {
        sscanf(ptr, "%2hhx", &dataarray[i]);
        ptr += 2;
    }
    for (int i = 0; i < valueBITS; i++) {
          Log.trace(F("%x"), dataarray[i]);
    }
  }
#ifdef IR_WHYNTER
  if (strcmp(protocol_name, "WHYNTER") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = WHYNTER_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendWhynter(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendWhynter(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_LG
  if (strcmp(protocol_name, "LG") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = LG_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendLG(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendLG(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_SONY
  if (strcmp(protocol_name, "SONY") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = SONY_12_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendSony(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendSony(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_DISH
  if (strcmp(protocol_name, "DISH") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = DISH_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendDISH(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendDISH(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_RC5
  if (strcmp(protocol_name, "RC5") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = RC5_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendRC5(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendRC5(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_RC6
  if (strcmp(protocol_name, "RC6") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = RC6_MODE0_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendRC6(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendRC6(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_SHARP
  if (strcmp(protocol_name, "SHARP") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = SHARP_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendSharpRaw(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendSharpRaw(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_SAMSUNG
  if (strcmp(protocol_name, "SAMSUNG") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = SAMSUNG_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendSAMSUNG(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendSAMSUNG(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_JVC
  if (strcmp(protocol_name, "JVC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = JVC_BITS;
#if defined(ESP8266) || defined(ESP32)
    irsend.sendJVC(data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendJVC(data, valueBITS);
#endif
    return true;
  }
#endif
#ifdef IR_PANASONIC
  if (strcmp(protocol_name, "PANASONIC") == 1)
  {
#if defined(ESP8266) || defined(ESP32)
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = PANASONIC_BITS;
    irsend.sendPanasonic(PanasonicAddress, data, valueBITS, valueRPT);
#else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendPanasonic(PanasonicAddress, data);
#endif
    return true;
  }
#endif

#if defined(ESP8266) || defined(ESP32)
#ifdef IR_COOLIX
  if (strcmp(protocol_name, "COOLIX") == 1)
  {
    Log.trace(F("Sending %s:" CR), protocol_name);
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kCoolixBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kCoolixDefaultRepeat);
    irsend.sendCOOLIX(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_RCMM
  if (strcmp(protocol_name, "RCMM") == 1)
  {
    Log.trace(F("Sending %s:" CR), protocol_name);
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kRCMMBits;
    irsend.sendRCMM(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DENON
  if (strcmp(protocol_name, "DENON") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = DENON_BITS;
    irsend.sendDenon(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_GICABLE
  if (strcmp(protocol_name, "GICABLE") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kGicableBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kGicableMinRepeat);
    irsend.sendGICable(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_SHERWOOD
  if (strcmp(protocol_name, "SHERWOOD") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSherwoodBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kSherwoodMinRepeat);
    irsend.sendSherwood(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MITSUBISHI
  if (strcmp(protocol_name, "MITSUBISHI") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiMinRepeat);
    irsend.sendMitsubishi(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_NIKAI
  if (strcmp(protocol_name, "NIKAI") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kNikaiBits;
    irsend.sendNikai(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MIDEA
  if (strcmp(protocol_name, "MIDEA") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMideaBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMideaMinRepeat);
    irsend.sendMidea(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MAGIQUEST
  if (strcmp(protocol_name, "MAGIQUEST") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMagiquestBits;
    irsend.sendMagiQuest(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_LASERTAG
  if (strcmp(protocol_name, "LASERTAG") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLasertagBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kLasertagMinRepeat);
    irsend.sendLasertag(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_CARRIER_AC
  if (strcmp(protocol_name, "CARRIER_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kCarrierAcBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kCarrierAcMinRepeat);
    irsend.sendCarrierAC(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MITSUBISHI2
  if (strcmp(protocol_name, "MITSUBISHI2") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiMinRepeat);
    irsend.sendMitsubishi2(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_AIWA_RC_T501
  if (strcmp(protocol_name, "AIWA_RC_T501") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kAiwaRcT501Bits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kAiwaRcT501MinRepeats);
    irsend.sendAiwaRCT501(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DAIKIN
  if (strcmp(protocol_name, "DAIKIN") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikinStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikinDefaultRepeat);
    irsend.sendDaikin(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_KELVINATOR
  if (strcmp(protocol_name, "KELVINATOR") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kKelvinatorStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kKelvinatorDefaultRepeat);
    irsend.sendKelvinator(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MITSUBISHI_AC
  if (strcmp(protocol_name, "MITSUBISHI_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiACStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiACMinRepeat);
    irsend.sendMitsubishiAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_SANYO
  if (strcmp(protocol_name, "SANYOLC7461") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSanyoLC7461Bits;
    irsend.sendSanyoLC7461(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_GREE
  if (strcmp(protocol_name, "GREE") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kGreeStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kGreeDefaultRepeat);
    irsend.sendGree(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_ARGO
  if (strcmp(protocol_name, "ARGO") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kArgoStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kArgoDefaultRepeat);
    irsend.sendArgo(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_TROTEC
  if (strcmp(protocol_name, "TROTEC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTrotecStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kTrotecDefaultRepeat);
    irsend.sendTrotec(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_TOSHIBA_AC
  if (strcmp(protocol_name, "TOSHIBA_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kToshibaACBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kToshibaACMinRepeat);
    irsend.sendToshibaAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_FUJITSU_AC
  if (strcmp(protocol_name, "FUJITSU_AC") == 1)
  {
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kFujitsuAcMinRepeat);
    irsend.sendFujitsuAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_HAIER_AC
  if (strcmp(protocol_name, "HAIER_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHaierACStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHaierAcDefaultRepeat);
    irsend.sendHaierAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_HITACHI_AC
  if (strcmp(protocol_name, "HITACHI_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_HITACHI_AC1
  if (strcmp(protocol_name, "HITACHI_AC1") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc1StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAC1(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_HITACHI_AC2
  if (strcmp(protocol_name, "HITACHI_AC2") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc2StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAC2(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_HAIER_AC_YRW02
  if (strcmp(protocol_name, "HAIER_AC_YRW02") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHaierACYRW02StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHaierAcYrw02DefaultRepeat);
    irsend.sendHaierACYRW02(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_WHIRLPOOL_AC
  if (strcmp(protocol_name, "WHIRLPOOL_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kWhirlpoolAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kWhirlpoolAcDefaultRepeat);
    irsend.sendWhirlpoolAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_SAMSUNG_AC
  if (strcmp(protocol_name, "SAMSUNG_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSamsungAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kSamsungAcDefaultRepeat);
    irsend.sendSamsungAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_LUTRON
  if (strcmp(protocol_name, "LUTRON") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLutronBits;
    irsend.sendLutron(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_ELECTRA_AC
  if (strcmp(protocol_name, "ELECTRA_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kElectraAcStateLength;
    irsend.sendElectraAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_PANASONIC_AC
  if (strcmp(protocol_name, "PANASONIC_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kPanasonicAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kPanasonicAcDefaultRepeat);
    irsend.sendPanasonicAC(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_PIONEER
  if (strcmp(protocol_name, "PIONEER") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kPioneerBits;
    irsend.sendPioneer(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_LG2
  if (strcmp(protocol_name, "LG2") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLgBits;
    irsend.sendLG2(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MWM
  if (strcmp(protocol_name, "MWM") == 1)
  {
    irsend.sendMWM(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DAIKIN2
  if (strcmp(protocol_name, "DAIKIN2") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin2StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin2DefaultRepeat);
    irsend.sendDaikin2(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_VESTEL_AC
  if (strcmp(protocol_name, "VESTEL_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kVestelAcBits;
    irsend.sendVestelAc(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_SAMSUNG36
  if (strcmp(protocol_name, "SAMSUNG36") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSamsung36Bits;
    irsend.sendSamsung36(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_TCL112AC
  if (strcmp(protocol_name, "TCL112AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTcl112AcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kTcl112AcDefaultRepeat);
    irsend.sendTcl112Ac(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_TECO
  if (strcmp(protocol_name, "TECO") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTecoBits;
    irsend.sendTeco(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_LEGOPF
  if (strcmp(protocol_name, "LEGOPF") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLegoPfBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kLegoPfMinRepeat);
    irsend.sendLegoPf(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MITSUBISHIHEAVY88
  if (strcmp(protocol_name, "MITSUBISHIHEAVY88") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiHeavy88StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiHeavy88MinRepeat);
    irsend.sendMitsubishiHeavy88(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MITSUBISHIHEAVY152
  if (strcmp(protocol_name, "MITSUBISHIHEAVY152") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiHeavy152StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiHeavy152MinRepeat);
    irsend.sendMitsubishiHeavy152(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DAIKIN216
  if (strcmp(protocol_name, "DAIKIN216") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin216StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin216DefaultRepeat);
    irsend.sendDaikin216(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_SHARP_AC
  if (strcmp(protocol_name, "SHARP_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSharpAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kSharpAcDefaultRepeat);
    irsend.sendSharpAc(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_GOODWEATHER
  if (strcmp(protocol_name, "GOODWEATHER_AC") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kGoodweatherBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kGoodweatherMinRepeat);
    irsend.sendGoodweather(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_INAX
  if (strcmp(protocol_name, "INAX") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kInaxBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kInaxMinRepeat);
    irsend.sendInax(data, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DAIKIN160
  if (strcmp(protocol_name, "DAIKIN160") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin160StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin160DefaultRepeat);
    irsend.sendDaikin160(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_NEOCLIMA
  if (strcmp(protocol_name, "NEOCLIMA") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kNeoclimaStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kNeoclimaMinRepeat);
    irsend.sendNeoclima(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DAIKIN176
  if (strcmp(protocol_name, "DAIKIN176") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin176StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin176DefaultRepeat);
    irsend.sendDaikin176(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DAIKIN128
  if (strcmp(protocol_name, "DAIKIN128") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin128StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin128DefaultRepeat);
    irsend.sendDaikin128(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_AMCOR
  if (strcmp(protocol_name, "AMCOR") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kAmcorStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kAmcorDefaultRepeat);
    irsend.sendAmcor(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_DAIKIN152
  if (strcmp(protocol_name, "DAIKIN152") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin152StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin152DefaultRepeat);
    irsend.sendDaikin152(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MITSUBISHI136
  if (strcmp(protocol_name, "MITSUBISHI136") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishi136StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishi136MinRepeat);
    irsend.sendMitsubishi136(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_MITSUBISHI112
  if (strcmp(protocol_name, "MITSUBISHI112") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishi112StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishi112MinRepeat);
    irsend.sendMitsubishi112(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
#ifdef IR_HITACHI_AC424
  if (strcmp(protocol_name, "HITACHI_AC424") == 1)
  {
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    Log.notice(F("Sending IR signal with %s"), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc424StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAc424(dataarray, valueBITS, valueRPT);
    return true;
  }
#endif
Log.trace(F("At the end" CR));
  return false;
#endif
}
#endif
