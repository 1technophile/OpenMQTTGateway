// Sample RFM69 sender/node sketch for mailbox motion sensor
// http://www.lowpowerlab.com/mailbox
// PIR motion sensor connected to D3 (INT1)
// When RISE happens on D3, the sketch transmits a "MOTION" msg to receiver Moteino and goes back to sleep
// It then wakes up every 32 seconds and sends a message indicating when the last
//    motion event happened (days, hours, minutes, seconds ago) and the battery level
// In sleep mode, Moteino + PIR motion sensor use about ~78uA
// Make sure you adjust the settings in the configuration section below !!!
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
#include <RFM69.h>    //get it here: https://github.com/LowPowerLab/RFM69
#include <RFM69_ATC.h>//get it here: https://github.com/lowpowerlab/RFM69
#include <SPIFlash.h> //get it here: https://github.com/lowpowerlab/spiflash
#include <SPI.h>      //included with Arduino IDE (www.arduino.cc)
#include <LowPower.h> //get library from: https://github.com/LowPowerLab/LowPower
                      //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID            55    //unique for each node on same network
#define NETWORKID         100  //the same on all nodes that talk to each other
#define GATEWAYID         1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY         RF69_433MHZ
//#define FREQUENCY         RF69_868MHZ
#define FREQUENCY         RF69_915MHZ
//#define FREQUENCY_EXACT 917000000
#define IS_RFM69HW_HCW    //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
#define ENCRYPTKEY        "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define SENDEVERYXLOOPS   8 //each loop sleeps 8 seconds, so send status message every this many sleep cycles (default "4" = 32 seconds)
//*********************************************************************************************
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI -75
//*********************************************************************************************

#define MOTION_PIN     3  // D3
#define MOTION_IRQ     1  // hardware interrupt 1 (D3) - where motion sensors OUTput is connected, this will generate an interrupt every time there is MOTION
#define BATT_MONITOR  A7  //through 1Meg+470Kohm and 0.1uF cap from battery VCC - this ratio divides the voltage to bring it below 3.3V where it is scaled to a readable range
#define BATT_CYCLES   50  // read and report battery voltage every this many sleep cycles (ex 30cycles * 8sec sleep = 240sec/4min). For 450 cyclesyou would get ~1 hour intervals
#define BATT_FORMULA(reading) reading * 0.00322 * 1.47
#define LED           5  // Moteinos have LEDs on D9
#define DUPLICATE_INTERVAL 55000 //avoid duplicates in 55second intervals (ie mailman sometimes spends 30+ seconds at mailbox)
//#define BLINK_EN         //uncomment to make LED flash when messages are sent, leave out if you want low power

//#define SERIAL_EN      //uncomment this line to enable serial IO debug messages, leave out if you want low power
#ifdef SERIAL_EN
  #define SERIAL_BAUD   115200
  #define DEBUG(input)   {Serial.print(input); delay(1);}
  #define DEBUGln(input) {Serial.println(input); delay(1);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

volatile boolean motionDetected=false;
char sendBuf[61];
byte sendLen;
byte sendLoops=0;
unsigned long MLO=0; //MailLastOpen (ago, in ms)
unsigned long now = 0, time=0, lastSend = 0, temp = 0;
float batteryVolts = 5;
char BATstr[20];
char MLOstr[20];

void setup() {
#ifdef SERIAL_EN
  Serial.begin(SERIAL_BAUD);
#endif  
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

#ifdef FREQUENCY_EXACT
  radio.setFrequency(FREQUENCY_EXACT); //set frequency to some custom frequency
#endif

 radio.setPowerLevel(29);
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  radio.sendWithRetry(GATEWAYID, "START", 6);

  radio.sleep();
  pinMode(MOTION_PIN, INPUT);
  pinMode(BATT_MONITOR, INPUT);
  attachInterrupt(MOTION_IRQ, motionIRQ, RISING);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buff);
  checkBattery();
}

void motionIRQ()
{
  motionDetected=true;
  DEBUGln("IRQ");
}

void loop() {
  now = millis();
  checkBattery();

  if (motionDetected && (time-MLO > DUPLICATE_INTERVAL))
  {
    DEBUG("MOTION");
    MLO = time; //save timestamp of event
    sprintf(sendBuf, "MOTION LO:0s BAT:%sv", BATstr);
    sendLen = strlen(sendBuf);
    if (radio.sendWithRetry(GATEWAYID, sendBuf, sendLen))
    {
     DEBUGln("..OK");
     #ifdef BLINK_EN
       Blink(LED,3);
     #endif
    }
    else DEBUGln("..NOK");
    radio.sleep();
  }
  else sendLoops++;

  //send readings every SENDEVERYXLOOPS
  if (sendLoops>=SENDEVERYXLOOPS)
  {
    sendLoops=0;
    char periodO='X', periodC='X';
    unsigned long lastOpened = (time - MLO) / 1000; //get seconds
    unsigned long LO = lastOpened;
    
    if (lastOpened <= 59) periodO = 's'; //1-59 seconds
    else if (lastOpened <= 3599) { periodO = 'm'; lastOpened/=60; } //1-59 minutes
    else if (lastOpened <= 259199) { periodO = 'h'; lastOpened/=3600; } // 1-71 hours
    else if (lastOpened >= 259200) { periodO = 'd'; lastOpened/=86400; } // >=3 days

    if (periodO == 'd')
      sprintf(MLOstr, "LO:%ldd%ldh", lastOpened, (LO%86400)/3600);
    else if (periodO == 'h')
      sprintf(MLOstr, "LO:%ldh%ldm", lastOpened, (LO%3600)/60);
    else sprintf(MLOstr, "LO:%ld%c", lastOpened, periodO);

    sprintf(sendBuf, "%s BAT:%sv", MLOstr, BATstr);
    sendLen = strlen(sendBuf);
    radio.send(GATEWAYID, sendBuf, sendLen);
    radio.sleep();
    DEBUG(sendBuf); DEBUG(" ("); DEBUG(sendLen); DEBUGln(")"); 
    lastSend = time;
    #ifdef BLINK_EN
      Blink(LED, 5);
    #endif
  }
  
  motionDetected=false; //do NOT move this after the SLEEP line below or motion will never be detected
  time = time + 8000 + millis()-now + 480; //correct millis() resonator drift, may need to be tweaked to be accurate
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  DEBUGln("WAKEUP");
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

byte cycleCount=BATT_CYCLES;
void checkBattery()
{
  if (cycleCount++ == BATT_CYCLES) //only read battery every BATT_CYCLES sleep cycles
  {
    unsigned int reading=0;
    //enable battery monitor
    pinMode(A3, OUTPUT);
    digitalWrite(A3, LOW);
    
    for (byte i=0; i<10; i++)
      reading += analogRead(BATT_MONITOR);
    
    //disable battery monitor
    pinMode(A3, INPUT); //highZ mode will allow p-mosfet to be pulled high and disconnect the voltage divider on the weather shield
    batteryVolts = BATT_FORMULA(reading/10);
    DEBUG("reading:"); DEBUGln(reading);
    dtostrf(batteryVolts, 3,2, BATstr);
    cycleCount = 0;
  }
}