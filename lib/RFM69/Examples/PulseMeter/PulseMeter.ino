// Sample RFM69 sketch for PulseMote sketch works with Moteinos equipped with RFM69W/RFM69HW/RFM69CW/RFM69HCW
// Reads an EE-SY310 based water/pulse meter and reports readings like:
//   - GPM - gallons per minute (when actively flowing)
//   - GLM - gallons last minute (gallons used in the last minute)
//   - GAL - total gallons used
// Example: https://lowpowerlab.com/blog/2013/02/02/meet-the-watermote-moteino-based-water-meter-reader-ee-sy310/
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
#include <EEPROM.h>        //included with Arduino IDE (www.arduino.cc)
#include <TimerOne.h>

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID             5
#define GATEWAYID          1
#define NETWORKID          250
#define FREQUENCY          RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY         "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*****************************************************************************************************************************
#define ENABLE_ATC      //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI        -75
//*****************************************************************************************************************************
#define PULSESPERGALLON    45 //how many pulses from sensor equal 1 gallon
#define GPMTHRESHOLD       8000  // GPM will reset after this many MS if no pulses are registered
#define XMITPERIOD         5000  // GPMthreshold should be less than 2*XMITPERIOD
//*********************************************************************************************
#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define INTERRUPTPIN   1  //INT1 = digital pin 11 (must be a hardware interrupt pin!)
#else
  #define LED            9 // Moteinos have LEDs on D9
  #define INTERRUPTPIN   1  //INT1 = digital pin 3 (must be a hardware interrupt pin!)
#endif
//*********************************************************************************************
#define SERIAL_EN        //uncomment this line to enable serial IO (when you debug Moteino and need serial output)
#define SERIAL_BAUD  115200
#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input);}
  #define DEBUGln(input) {Serial.println(input);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
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
SPIFlash flash(8, 0xEF30); //WINDBOND 4MBIT flash chip on CS pin D8 (default for Moteino)

volatile byte ledState = LOW;
volatile unsigned long PulseCounterVolatile = 0; // use volatile for shared variables
unsigned long NOW = 0;
unsigned long PulseCounter = 0;
unsigned long LASTMINUTEMARK = 0;
unsigned long PULSECOUNTLASTMINUTEMARK = 0; //keeps pulse count at the last minute mark

byte COUNTEREEPROMSLOTS = 10;
unsigned long COUNTERADDRBASE = 8; //address in EEPROM that points to the first possible slot for a counter
unsigned long COUNTERADDR = 0;     //address in EEPROM that points to the latest Counter in EEPROM
byte secondCounter = 0;

unsigned long TIMESTAMP_pulse_prev = 0;
unsigned long TIMESTAMP_pulse_curr = 0;
int pulseAVGInterval = 0;
int pulsesPerXMITperiod = 0;
float GPM=0, GLM=0, GAL=0, GALlast=0, GPMlast=0, GLMlast=0;
byte sendLen;
char buff[80];
char* GALstr="99999999999999.99"; //longest expected GAL message
char* GPMstr="99999.99"; //longest expected GPM message
char* GLMstr="9999999.99"; //longest expected GLM message
boolean WPReady = false;

void setup() {
  #ifdef SERIAL_EN
    Serial.begin(SERIAL_BAUD);
  #endif

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);
  pinMode(LED, OUTPUT);

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  //initialize counter from EEPROM
  unsigned long savedCounter = EEPROM_Read_Counter();
  if (savedCounter <=0) savedCounter = 1; //avoid division by 0
  PulseCounterVolatile = PulseCounter = PULSECOUNTLASTMINUTEMARK = savedCounter;
  attachInterrupt(INTERRUPTPIN, pulseCounterInterrupt, RISING);
  Timer1.initialize(XMITPERIOD * 1000L);
  Timer1.attachInterrupt(XMIT);
  
  sprintf(buff, "\nTransmitting at %d Mhz, id:%d nid:%d gid:%d", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915, NODEID, NETWORKID, GATEWAYID);
  DEBUG(buff);
  for (byte i=0;i<strlen(ENCRYPTKEY);i++) DEBUG(ENCRYPTKEY[i]);
  DEBUGln();
  
  if (flash.initialize())
  {
    DEBUGln("FLASH MEM present, ready for wireless programming.");
    WPReady = true;
  }
  else
    DEBUGln("FLASH MEM not found, skipping wireless programming checks.");
}

void loop() {
  if (WPReady && radio.receiveDone())
  {
    DEBUG('[');DEBUG(radio.SENDERID);DEBUG("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      DEBUG((char)radio.DATA[i]);

    // wireless programming token check
    // DO NOT REMOVE, or PulseMote will not be wirelessly programmable any more!
    CheckForWirelessHEX(radio, flash, true);
  }
}

void pulseCounterInterrupt()
{
  noInterrupts();
  ledState = !ledState;
  PulseCounterVolatile++;  // increase when LED turns on
  digitalWrite(LED, ledState);
  NOW = millis();

  //remember how long between pulses (sliding window)
  TIMESTAMP_pulse_prev = TIMESTAMP_pulse_curr;
  TIMESTAMP_pulse_curr = NOW;
  
  if (TIMESTAMP_pulse_curr - TIMESTAMP_pulse_prev > GPMTHRESHOLD)
    //more than 'GPMthreshold' seconds passed since last pulse... resetting GPM
    pulsesPerXMITperiod=pulseAVGInterval=0;
  else
  {
    pulsesPerXMITperiod++;
    pulseAVGInterval += TIMESTAMP_pulse_curr - TIMESTAMP_pulse_prev;
  }
  interrupts();
}

void XMIT()
{
  noInterrupts();
  PulseCounter = PulseCounterVolatile;
  interrupts();
  
  if (millis() - TIMESTAMP_pulse_curr >= 5000)
  {
    ledState = !ledState;
    digitalWrite(LED, ledState);
  }
  
  //calculate Gallons counter 
  GAL = ((float)PulseCounter)/PULSESPERGALLON;
  DEBUG("PulseCounter:");DEBUG(PulseCounter);DEBUG(", GAL: "); DEBUGln(GAL);

  //calculate & output GPM
  GPM = pulseAVGInterval > 0 ? 60.0 * 1000 * (1.0/PULSESPERGALLON)/(pulseAVGInterval/pulsesPerXMITperiod)
                             : 0;
  dtostrf(GAL,3,2, GALstr);
  dtostrf(GPM,3,2, GPMstr);

  pulsesPerXMITperiod = 0;
  pulseAVGInterval = 0;
  secondCounter += XMITPERIOD/1000;

  //once per minute, output a GallonsLastMinute count
  if (secondCounter>=60)
  {
    //DEBUG("60sec mark ... ");
    secondCounter=0;
    GLM = ((float)(PulseCounter - PULSECOUNTLASTMINUTEMARK))/PULSESPERGALLON;
    PULSECOUNTLASTMINUTEMARK = PulseCounter;
    EEPROM_Write_Counter(PulseCounter);
    dtostrf(GLM,3,2, GLMstr);
    sprintf(buff, "GAL:%s GPM:%s GLM:%s", GALstr, GPMstr, GLMstr);
    //DEBUGln("done");
  }
  else
  {
    sprintf(buff, "GAL:%s GPM:%s", GALstr, GPMstr);    
  }

  if (GPM!=GPMlast || GAL!=GALlast || GLM!=GLMlast)
  {
    sendLen = strlen(buff);
    radio.sendWithRetry(GATEWAYID, buff, sendLen);
    GALlast = GAL;
    GPMlast = GPM;
    GLMlast = GLM;
  }
  
  DEBUGln(buff);
}

unsigned long EEPROM_Read_Counter()
{
  return EEPROM_Read_ULong(EEPROM_Read_ULong(COUNTERADDR));
}

void EEPROM_Write_Counter(unsigned long counterNow)
{
  if (counterNow == EEPROM_Read_Counter())
  {
    DEBUG("{EEPROM-SKIP(no changes)}");
    return; //skip if nothing changed
  }
  
  DEBUG("{EEPROM-SAVE(");
  DEBUG(EEPROM_Read_ULong(COUNTERADDR));
  DEBUG(")=");
  DEBUG(PulseCounter);
  DEBUG("}");
    
  unsigned long CounterAddr = EEPROM_Read_ULong(COUNTERADDR);
  if (CounterAddr == COUNTERADDRBASE+8*(COUNTEREEPROMSLOTS-1))
    CounterAddr = COUNTERADDRBASE;
  else CounterAddr += 8;
  
  EEPROM_Write_ULong(CounterAddr, counterNow);
  EEPROM_Write_ULong(COUNTERADDR, CounterAddr);
}

unsigned long EEPROM_Read_ULong(int address)
{
  unsigned long temp;
  for (byte i=0; i<8; i++)
    temp = (temp << 8) + EEPROM.read(address++);
  return temp;
}

void EEPROM_Write_ULong(int address, unsigned long data)
{
  for (byte i=0; i<8; i++)
  {
    EEPROM.write(address+7-i, data);
    data = data >> 8;
  }
}