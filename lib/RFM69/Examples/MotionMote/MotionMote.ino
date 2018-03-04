// Sample RFM69 sender/node sketch for the MotionMote
// http://lowpowerlab.com/motionmote
// PIR motion sensor connected to D3 (INT1)
// When RISE happens on D3, the sketch transmits a "MOTION" msg to receiver Moteino and goes back to sleep
// In sleep mode, Moteino + PIR motion sensor use about ~60uA
// IMPORTANT: adjust the settings in the configuration section below !!!

// **********************************************************************************
// Copyright Felix Rusu of LowPowerLab.com, 2016
// RFM69 library and sample code by Felix Rusu - lowpowerlab.com/contact
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
#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>      //comes with Arduino IDE (www.arduino.cc)
#include <LowPower.h> //get library from: https://github.com/lowpowerlab/lowpower
                      //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/
#include <SPIFlash.h> //get it here: https://www.github.com/lowpowerlab/spiflash
#include <SparkFunBME280.h> //get it here: https://github.com/sparkfun/SparkFun_BME280_Breakout_Board/tree/master/Libraries/Arduino/src
#include <Wire.h>     //comes with Arduino

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID        88    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
#define GATEWAYID     1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -75
//#define ENABLE_BME280 //uncomment to allow reading the BME280 (if present)
//*********************************************************************************************
#define ACK_TIME      30  // max # of ms to wait for an ack
#define ONBOARDLED     9  // Moteinos have LEDs on D9
#define LED            5  // MotionOLEDMote has an external LED on D5
#define MOTION_PIN     3  // D3
#define MOTION_IRQ     1  // hardware interrupt 1 (D3) - where motion sensors OUTput is connected, this will generate an interrupt every time there is MOTION
#define BATT_MONITOR  A7  // Sense VBAT_COND signal (when powered externally should read ~3.25v/3.3v (1000-1023), when external power is cutoff it should start reading around 2.85v/3.3v * 1023 ~= 883 (ratio given by 10k+4.7K divider from VBAT_COND = 1.47 multiplier)
#define BATT_FORMULA(reading) reading * 0.00322 * 1.49 // >>> fine tune this parameter to match your voltage when fully charged
                                                       // details on how this works: https://lowpowerlab.com/forum/index.php/topic,1206.0.html
#define DUPLICATE_INTERVAL 20000 //avoid duplicates in 55second intervals (ie mailman sometimes spends 30+ seconds at mailbox)
#define BATT_INTERVAL  300000  // read and report battery voltage every this many ms (approx)

//#define SERIAL_EN             //comment this out when deploying to an installed Mote to save a few KB of sketch size
#define SERIAL_BAUD    115200
#ifdef SERIAL_EN
#define DEBUG(input)   {Serial.print(input); delay(1);}
#define DEBUGln(input) {Serial.println(input); delay(1);}
#define DEBUGFlush() { Serial.flush(); }
#else
#define DEBUG(input);
#define DEBUGln(input);
#define DEBUGFlush();
#endif

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

#define FLASH_SS      8 // and FLASH SS on D8 on regular Moteinos (D23 on MoteinoMEGA)
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)

#ifdef ENABLE_BME280
  BME280 bme280;
#endif

volatile boolean motionDetected=false;
float batteryVolts = 5;
char BATstr[10]; //longest battery voltage reading message = 9chars
char sendBuf[32];
byte sendLen;
#ifdef ENABLE_BME280
  float temperature=0;
  char Fstr[10];
#endif

void motionIRQ(void);
void checkBattery(void);

void setup() {
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif
  
  pinMode(MOTION_PIN, INPUT);
  attachInterrupt(MOTION_IRQ, motionIRQ, RISING);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buff);
  pinMode(ONBOARDLED, OUTPUT);
  pinMode(LED, OUTPUT);
  radio.sendWithRetry(GATEWAYID, "START", 5);
  
#ifdef ENABLE_ATC
  DEBUGln("RFM69_ATC Enabled (Auto Transmission Control)\n");
#endif

  if (flash.initialize()) flash.sleep(); //if Moteino has FLASH-MEM, make sure it sleeps

#ifdef ENABLE_BME280
  bme280.settings.commInterface = I2C_MODE;
  bme280.settings.I2CAddress = 0x77;
  bme280.settings.runMode = 3; //Normal mode
  bme280.settings.tStandby = 0;
  bme280.settings.filter = 0;
  bme280.settings.tempOverSample = 1;
  bme280.settings.pressOverSample = 1;
  bme280.settings.humidOverSample = 1;
#endif
}

void motionIRQ()
{
  motionDetected=true;
  DEBUGln("IRQ");
}

uint16_t batteryReportCycles=0;
uint32_t time=0, now=0, MLO=0, BLO=0;
byte motionRecentlyCycles=0;
void loop() {
  now = millis();
  checkBattery();
  //DEBUG("Slept: ");DEBUG(now-lastSleepTime);DEBUGln("ms");

  if (motionDetected && (time-MLO > DUPLICATE_INTERVAL))
  {
    digitalWrite(LED, HIGH);
    MLO = time; //save timestamp of event

#ifdef ENABLE_BME280
    //read BME sensor
    bme280.begin();
    dtostrf(bme280.readTempF(), 3,2, Fstr);
    bme280.writeRegister(BME280_CTRL_MEAS_REG, 0x00); //sleep the BME280
    sprintf(sendBuf, "MOTION BAT:%sv F:%s", BATstr, Fstr);
#else
    sprintf(sendBuf, "MOTION BAT:%sv", BATstr);
#endif

    sendLen = strlen(sendBuf);

    if (radio.sendWithRetry(GATEWAYID, sendBuf, sendLen))
    {
      DEBUG("MOTION ACK:OK! RSSI:");
      DEBUG(radio.RSSI);
      batteryReportCycles = 0;
    }
    else DEBUG("MOTION ACK:NOK...");

    DEBUG(" VIN: ");
    DEBUGln(BATstr);

    radio.sleep();
    digitalWrite(LED, LOW);
  }
  else if (time-BLO > BATT_INTERVAL)
  {
#ifdef ENABLE_BME280
    //read BME sensor
    bme280.begin();
    dtostrf(bme280.readTempF(), 3,2, Fstr);
    bme280.writeRegister(BME280_CTRL_MEAS_REG, 0x00); //sleep the BME280
    sprintf(sendBuf, "BAT:%sv F:%s", BATstr, Fstr);
#else
    sprintf(sendBuf, "BAT:%sv", BATstr);
#endif
    
    sendLen = strlen(sendBuf);
    BLO = time;
    radio.sendWithRetry(GATEWAYID, sendBuf, sendLen);
    radio.sleep();
    batteryReportCycles=0;
  }
  
  DEBUGFlush();

  //while motion recently happened sleep for small slots of time to better approximate last motion event
  //this helps with debouncing a "MOTION" event more accurately for sensors that fire the IRQ very rapidly (ie panasonic sensors)
  if (motionDetected ||motionRecentlyCycles>0)
  {
    if (motionDetected) motionRecentlyCycles=8;
    else motionRecentlyCycles--;
    motionDetected=false; //do NOT move this after the SLEEP line below or motion will never be detected
    time = time + 250 + millis()-now;
    radio.sleep();
    LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
    DEBUGln("WAKEUP250ms");
  }
  else
  {
    time = time + 8000 + millis()-now;
    radio.sleep();
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    DEBUGln("WAKEUP8s");
  }
  batteryReportCycles++;
}

uint32_t BLR=0;
void checkBattery()
{
  if (time-BLR > 30000) //only read battery every 30s or so
  {
    unsigned int readings=0;
    BLR = time;
    for (byte i=0; i<10; i++) //take 10 samples, and average
      readings+=analogRead(BATT_MONITOR);
    batteryVolts = BATT_FORMULA(readings / 10.0);
    dtostrf(batteryVolts, 3,2, BATstr); //update the BATStr which gets sent every BATT_CYCLES or along with the MOTION message
  }
}