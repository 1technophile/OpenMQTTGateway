// **********************************************************************************
// DoorBellMote sketch works with Moteinos equipped with RFM69W/RFM69HW/RFM69CW/RFM69HCW
// It detects current flow at the doorbell transformer and send a message each time to the gateway
// It can trigger doorbell through a relay powered from pins D6+D7
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
#define GATEWAYID       1
#define NODEID          133
#define NETWORKID       100
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*****************************************************************************************************************************
#define ENABLE_ATC      //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI        -75
//*****************************************************************************************************************************
#define CHIMEPIN              4 // active HIGH chime signal from detector H11AA1 circuit
#define RELAYPIN1             6 //for the bell ring relay we just need 2 digital pins together to activate the relay for a short pulse
#define RELAYPIN2             7 //for the bell ring relay we just need 2 digital pins together to activate the relay for a short pulse
#define DISABLE_RELAY         5 //for the bell disable relay we use a single digital pin through a transistor
#define RELAY_PULSE_MS      250 //just enough that the doorbell chime will trigger
#define RINGDELAY          3000 //time between rings (avoid fast repeated rings)
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
char buff[50];

void setup(void)
{
  Serial.begin(SERIAL_BAUD);
  pinMode(CHIMEPIN, INPUT);
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  pinMode(DISABLE_RELAY, OUTPUT);
  pinMode(LED, OUTPUT);

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  sprintf(buff, "DoorBellMote : %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buff);

  if (flash.initialize())
    DEBUGln("SPI Flash Init OK");
  else
    DEBUGln("SPI Flash Init FAIL! (is chip present?)");

  radio.sendWithRetry(GATEWAYID, "START", 6);
  Blink(LED, 100);
  Blink(LED, 100);
  Blink(LED, 100);
}

uint32_t doorPulseCount = 0;
uint32_t lastStatusTimestamp=0;
uint32_t LEDCYCLETIMER=0;
byte LEDSTATE=LOW;
char input;
boolean ring=false;
boolean disable=false;
byte disableStatus=0;

void loop()
{
  if (Serial.available())
    input = Serial.read();

  if (input=='r')
  {
    DEBUGln("Relay test...");
    pulseRelay();
    input = 0;
  }

  if (millis()-(lastStatusTimestamp)>RINGDELAY)
  {
    if (digitalRead(CHIMEPIN) == HIGH)
    {
      lastStatusTimestamp = millis();
      radio.sendWithRetry(GATEWAYID, "RING", 4);
      Blink(LED,20);
      Blink(LED,20);
      Blink(LED,20);
    }
  }

  if (radio.receiveDone())
  {
    DEBUG('[');DEBUG(radio.SENDERID);DEBUG("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      DEBUG((char)radio.DATA[i]);

    if (radio.DATALEN==4)
      if (radio.DATA[0]=='R' && radio.DATA[1]=='I' && radio.DATA[2]=='N' && radio.DATA[3]=='G')
        ring = true;

    if (radio.DATALEN==6)
      if (radio.DATA[0]=='B' && radio.DATA[1]=='E' && radio.DATA[2]=='L' && radio.DATA[3]=='L' && radio.DATA[4]==':')
        if (radio.DATA[5]=='0')
        {
          disableStatus = 1;
          disable = true;
        }
        else if (radio.DATA[5]=='1')
        {
          disableStatus = 0;
          disable = true;
        }

    // wireless programming token check
    // DO NOT REMOVE, or this Moteino will not be wirelessly programmable any more!
    CheckForWirelessHEX(radio, flash, true);

    //first send any ACK to request
    DEBUG("   [RX_RSSI:");DEBUG(radio.RSSI);DEBUG("]");
    if (radio.ACKRequested())
    {
      radio.sendACK();
      DEBUG(" - ACK sent.");
    }

    if (ring)
    {
      //if other relay is ON we must temporarily turn it off while we pulse the RING relay, to avoid any rail collapse and reset
      if (disableStatus) digitalWrite(DISABLE_RELAY, 0);
      pulseRelay();
      if (disableStatus) digitalWrite(DISABLE_RELAY, 1);
      radio.sendWithRetry(GATEWAYID, "RING OK", 4);
      ring = false;
    }

    if (disable)
    {
      digitalWrite(DISABLE_RELAY, disableStatus); //disable it
      sprintf(buff, "BELL:%d", disableStatus ? 0 : 1);
      radio.sendWithRetry(GATEWAYID, buff, 6);
      disable=false;
    }

    DEBUGln();
  }

  if (millis() - LEDCYCLETIMER > 2000) //flip onboard LED state every so often to indicate activity
  {
    LEDCYCLETIMER = millis();
    LEDSTATE = !LEDSTATE;
    digitalWrite(LED, LEDSTATE);
  }
}

void pulseRelay()
{
  digitalWrite(RELAYPIN1, HIGH);
  digitalWrite(RELAYPIN2, HIGH);
  delay(RELAY_PULSE_MS);
  digitalWrite(RELAYPIN1, LOW);
  digitalWrite(RELAYPIN2, LOW);
}

void Blink(byte PIN, byte DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS/2);
  digitalWrite(PIN,LOW);
  delay(DELAY_MS/2);  
}