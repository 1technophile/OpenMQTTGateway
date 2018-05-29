// **********************************************************************************
// IOShield sample sketch works with Moteinos equipped with RFM69W/RFM69HW/RFM69CW/RFM69HCW
// http://www.LowPowerLab.com/IOShield
// **********************************************************************************
// It works with IOShield(s) that have 2 shift registers (74HC595)
// You can daisy chain up to 16 IOShields for a total of 256 outputs/stations/zones
// If you chain more than 2 IOShields be sure to adjust the REGISTERCOUNT setting below in the define section
// Also you must adjust the radio settings: frequency/ HW setting, encryption key etc below in the define section
// It listens for tokens like:
//       - 'ON:N' where N={1..16} - turns ON one output dictated by position N
//       - 'ALL' - turns ON all outputs
//       - 'OFF' turns off all outputs
//       - 'PRG A:n ... Z:m' - runs a program in sequence, first token is station/zone/output number, second is the number of seconds to turn it ON for
//          WARNING: there is no delay between switching zones, so beware of the frequency you switch the valves on/off
// Example use: control your sprinkler controller wirelessly, more at lowpowerlab.com/gateway
// Deploy and forget: wirelessly programmable via Moteino + RFM69_OTA library
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>         //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <RFM69_OTA.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <SPIFlash.h>      //get it here: https://github.com/lowpowerlab/spiflash
#include <SPI.h>           //included with Arduino IDE (www.arduino.cc)

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define GATEWAYID   1
#define NODEID      121
#define NETWORKID   250
#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*****************************************************************************************************************************
#define ENABLE_ATC      //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI        -75
//*****************************************************************************************************************************
#define LATCHPIN              5
#define CLOCKPIN              6
#define DATAPIN               7
#define REGISTERCOUNT         2 //Moteino IOShield has 2 daisy chained registers, if you have more adjust this number
//*****************************************************************************************************************************
#define LED                  9   //pin connected to onboard LED
#define SERIAL_BAUD     115200
#define SERIAL_EN                //comment out if you don't want any serial output

#ifdef SERIAL_EN
  #define DEBUG(input)   Serial.print(input)
  #define DEBUGln(input) Serial.println(input)
#else
  #define DEBUG(input)
  #define DEBUGln(input)
#endif

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

//*****************************************************************************************************************************
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0xEF30 for windbond 4mbit flash (Moteino OEM)
//*****************************************************************************************************************************
SPIFlash flash(8, 0xEF30); //regular Moteinos have FLASH MEM on D8, MEGA has it on D4
char buff[30]; //max radio DATA length = 61
String str;
String substr;

void setup(void)
{
  Serial.begin(SERIAL_BAUD);
  pinMode(LATCHPIN, OUTPUT);
  pinMode(DATAPIN, OUTPUT);
  pinMode(CLOCKPIN, OUTPUT);
  pinMode(LED, OUTPUT);

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  //sprintf(buff, "IOShield : %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  //DEBUGln(buff);
  DEBUGln(F("START"));
  
  if (flash.initialize())
    DEBUGln(F("SPI Flash Init OK"));
  else
    DEBUGln(F("SPI Flash Init FAIL! (is chip present?)"));

  radio.sendWithRetry(GATEWAYID, "START", 5);
  zonesOFF();
  Blink(LED, 100);
  Blink(LED, 100);
  Blink(LED, 100);
  DEBUG("Free RAM:");DEBUGln(checkFreeRAM());
}

byte LEDSTATE=LOW;
uint32_t LEDCYCLETIMER=0;
byte programZone[32]; //used to store program data
unsigned int programSeconds[32]; //used to store program data
byte programLength=0; //how many zones in this program
byte programPointer=0; //keeps track of active zone (when >=0)
unsigned int seconds=0;
byte whichZone;
int index;

void loop()
{
  if (radio.receiveDone())
  {
    DEBUG(F("["));DEBUG(radio.SENDERID);DEBUG(F("] "));
    DEBUG((char *)radio.DATA);
    DEBUG(F(" [RX_RSSI:"));DEBUG(radio.RSSI);DEBUGln("]");

    // wireless programming token check
    // DO NOT REMOVE, or this node will not be wirelessly programmable any more!
    CheckForWirelessHEX(radio, flash, true);

    whichZone=0;
    str = String((char *)radio.DATA);
    str.trim();
    if (str.equals(F("OFF"))) zonesOFF();
    else if (str.equals(F("ALL"))) zonesAllON();  //for testing registers only, you should never turn ON all zones in a sprinkler system
    else if (str.length() >= 4 && str.startsWith("ON:")) //ON:zoneNumber
    {
      str = str.substring(3); //trim "ON:"
      whichZone = str.toInt(); //extract zone id
      if (whichZone > 0) {
        stopAndResetProgram();//this command forces any programs to stop and only set the requested zone/output ON
        zoneON(whichZone);
      }
      else DEBUG("Invalid ON");
    }
    else if (str.length() >= 7 && str.startsWith("PRG ")) //PRG 1:5 2:5 3:10 4:5 .... zoneN:seconds
    {
      str = str.substring(4); //trim "PRG "
      while (str.length()>=3 && programPointer < 255)
      {
        whichZone = str.toInt(); //extract zone id
        index = str.indexOf(':');
        if (index > 0 && str.length() > index) //make sure there's something after the colon
        {
          str = str.substring(index+1); //trim "zoneId:"
          seconds = str.toInt(); //extract seconds
          if (seconds > 0)
          {
            programZone[programLength] = whichZone;
            programSeconds[programLength] = seconds;
            programLength++;
            
            DEBUG("Extracted ZONE");
            DEBUG(whichZone);
            DEBUG(":");
            DEBUG(seconds);
            DEBUGln("s");
          }
          else
          {
            programLength = 0;
            DEBUG("INVALID PRG");
            break;
          }
        }
        else
        {
          programLength = 0;
          DEBUG("INVALID PRG");
          break;
        }
        
        //trim current token and move to next one
        index = str.indexOf(' ');
        if (index > 0 && str.length() > index+3) //X X:X
        {
          str = str.substring(index+1);
        }
        else //EOS?
        {
          DEBUG( programLength>0 ? "DONE" : "INVALID PRG");
          break;
        }
      }
      DEBUG(F("Found programs:"));DEBUGln(programLength);
    }

    //respond to any other ACKs
    if (radio.ACKRequested()) radio.sendACK();
    DEBUGln();
    DEBUG("Free RAM:");DEBUGln(checkFreeRAM());
  }

  handleProgram();
    
  if (millis() - LEDCYCLETIMER > 2000) //flip onboard LED state every so often to indicate activity
  {
    LEDCYCLETIMER = millis();
    LEDSTATE = !LEDSTATE;
    digitalWrite(LED, LEDSTATE);
  }
}

unsigned long programZoneStart=0;
void handleProgram()
{
  if (programLength > 0)
  {
    if (programZoneStart == 0)
    {
      programZoneStart = millis(); //mark start time of a zone      
      zoneON(programZone[programPointer]);
      DEBUG("Running zone ");
      DEBUG(programZone[programPointer]);
      DEBUG(" for ");
      DEBUG((((unsigned long)programSeconds[programPointer])*1000));
      DEBUGln("ms...");
    }
    else if (millis() - programZoneStart > (((unsigned long)programSeconds[programPointer])*1000)) //check if zone time expired, jump to next zone
    {
      programZoneStart=0;
      programLength--;
      if (programLength == 0) //finished
        zonesOFF();
      else programPointer++; //skip to next zone in program
    }
  }
}

void stopAndResetProgram()
{
  programLength=0;
  programPointer=0;
  programZoneStart=0;
}

//turns ON one zone only, all others off
void zoneON(byte which)
{
  //stopAndResetProgram(); //stop any running programs
  if (radio.ACKRequested()) radio.sendACK();
  registersWriteBit(which-1);
  sprintf(buff, "ZONE:%d", which);
  if (radio.sendWithRetry(GATEWAYID, buff, strlen(buff)))
    {DEBUGln(F("..OK"));}
  else {DEBUGln(F("..NOK"));}
}

//all zones OFF
void zonesOFF()
{
  stopAndResetProgram(); //stop any running programs
  if (radio.ACKRequested()) radio.sendACK();
    registersClear();
  if (radio.sendWithRetry(GATEWAYID, "ZONES:OFF", 9))
    {DEBUGln(F("..OK"));}
  else {DEBUGln(F("..NOK"));}
}

//all zones ON - for testing purposes only
void zonesAllON()
{
  stopAndResetProgram(); //stop any running programs
  registersAllOn();
}

void registersClear() {
  digitalWrite(LATCHPIN, LOW);
  for(byte i=0;i<REGISTERCOUNT;i++)
    shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, 0x0);  
  digitalWrite(LATCHPIN, HIGH); 
}

void registersAllOn() {
  digitalWrite(LATCHPIN, LOW);
  for(byte i=0;i<REGISTERCOUNT;i++)
    shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, 0xFF);  
  digitalWrite(LATCHPIN, HIGH);
}

//writes a single bit to a daisy chain of up to 32 shift registers (max 16 IOShields) chained via LATCHPIN, CLOCKPIN, DATAPIN
void registersWriteBit(byte whichPin) {
    byte bitPosition = whichPin % 8;
    int zeroFills = (REGISTERCOUNT - 1) - (whichPin/8);

    if (zeroFills<0) { //whichPin was "out of bounds"
      DEBUGln("requested bit out of bounds (ie learger than available register bits to set)");
      registersClear();
      return; 
    }

    digitalWrite(LATCHPIN, LOW);    
    for (byte i=0;i<zeroFills;i++)
      shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, 0x0);
    
    byte byteToWrite = 0;
    bitSet(byteToWrite, bitPosition);
    shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, byteToWrite);
    
    for (byte i=0;i<REGISTERCOUNT-zeroFills-1;i++)
      shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, 0x0);
    digitalWrite(LATCHPIN, HIGH);
}

//writes a byte stream to the shift register daisy chain (up to 256 daisy chained shift registers)
void registerWriteBytes(const void* buffer, byte byteCount) {
  digitalWrite(LATCHPIN, LOW);
  for (byte i = 0; i < byteCount; i++)
    shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, ((byte*)buffer)[i]);  
  digitalWrite(LATCHPIN, HIGH);
}

void Blink(byte PIN, byte DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS/2);
  digitalWrite(PIN,LOW);
  delay(DELAY_MS/2);  
}


int checkFreeRAM() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
