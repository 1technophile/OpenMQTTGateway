// **********************************************************************************************************
// WeatherShield R2 (BME280 sensor) sameple sketch that works with Moteinos equipped with RFM69W/RFM69HW
// It sends periodic weather readings (temp, hum, atm pressure) from WeatherShield to the base node Moteino
// For use with MoteinoMEGA you will have to revisit the pin definitions defined below
// http://www.LowPowerLab.com/WeatherShield
// Example setup (with R1): http://lowpowerlab.com/blog/2015/07/24/attic-fan-cooling-tests/
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
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_OTA.h>     //get it here: https://github.com/lowpowerlab/rfm69
#include <SPIFlash.h>      //get it here: https://github.com/lowpowerlab/spiflash
#include <SPI.h>           //included in Arduino IDE (www.arduino.cc)
#include <Wire.h>          //included in Arduino IDE (www.arduino.cc)
#include <SparkFunBME280.h>//get it here: https://github.com/sparkfun/SparkFun_BME280_Breakout_Board/tree/master/Libraries/Arduino/src
#include <LowPower.h>      //get it here: https://github.com/lowpowerlab/lowpower
                           //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
#define GATEWAYID   1
#define NODEID      40
#define NETWORKID   100
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -75
//*********************************************************************************************
#define SEND_LOOPS   15 //send data this many sleep loops (15 loops of 8sec cycles = 120sec ~ 2 minutes)
#define SLEEP_FASTEST SLEEP_15MS
#define SLEEP_FAST SLEEP_250MS
#define SLEEP_SEC SLEEP_1S
#define SLEEP_LONG SLEEP_2S
#define SLEEP_LONGER SLEEP_4S
#define SLEEP_LONGEST SLEEP_8S
period_t sleepTime = SLEEP_LONGEST; //period_t is an enum type defined in the LowPower library (LowPower.h)
//*********************************************************************************************
#define BATT_MONITOR_EN A3 //enables battery voltage divider to get a reading from a battery, disable it to save power
#define BATT_MONITOR  A7   //through 1Meg+470Kohm and 0.1uF cap from battery VCC - this ratio divides the voltage to bring it below 3.3V where it is scaled to a readable range
#define BATT_CYCLES   2    //read and report battery voltage every this many sleep cycles (ex 30cycles * 8sec sleep = 240sec/4min). For 450 cyclesyou would get ~1 hour intervals
#define BATT_FORMULA(reading) reading * 0.00322 * 1.475  // >>> fine tune this parameter to match your voltage when fully charged
#define BATT_LOW      3.6  //(volts)
#define BATT_READ_LOOPS  SEND_LOOPS*10  // read and report battery voltage every this many sleep cycles (ex 30cycles * 8sec sleep = 240sec/4min). For 450 cycles you would get ~1 hour intervals between readings
//*****************************************************************************************************************************

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

//#define BLINK_EN                 //uncomment to blink LED on every send
//#define SERIAL_EN                //comment out if you don't want any serial output

#ifdef SERIAL_EN
  #define SERIAL_BAUD   115200
  #define DEBUG(input)   {Serial.print(input);}
  #define DEBUGln(input) {Serial.println(input);}
  #define SERIALFLUSH() {Serial.flush();}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
  #define SERIALFLUSH();
#endif
//*****************************************************************************************************************************

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

SPIFlash flash(FLASH_SS, 0xEF30); //WINDBOND 4MBIT flash chip on CS pin D8 (default for Moteino)

BME280 bme280;
char Pstr[10];
char Fstr[10];
char Hstr[10];
char buffer[50];

void setup(void)
{
#ifdef SERIAL_EN
  Serial.begin(SERIAL_BAUD);
#endif
  pinMode(LED, OUTPUT);
  
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

  sprintf(buffer, "WeatherMote - transmitting at: %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buffer);

  //initialize weather shield BME280 sensor
  bme280.settings.commInterface = I2C_MODE;
  bme280.settings.I2CAddress = 0x77;
  bme280.settings.runMode = 3; //Normal mode
  bme280.settings.tStandby = 0;
  bme280.settings.filter = 0;
  bme280.settings.tempOverSample = 1;
  bme280.settings.pressOverSample = 1;
  bme280.settings.humidOverSample = 1;

  radio.sendWithRetry(GATEWAYID, "START", 6);
  Blink(LED, 100);Blink(LED, 100);Blink(LED, 100);

  if (flash.initialize()) flash.sleep();

  for (uint8_t i=0; i<=A5; i++)
  {
    if (i == RF69_SPI_CS) continue;
    if (i == FLASH_SS) continue;
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  
  SERIALFLUSH();
  readBattery();
}

unsigned long doorPulseCount = 0;
char input=0;
double F,P,H;
byte sendLoops=0;
byte battReadLoops=0;
float batteryVolts = 5;
char* BATstr="BAT:5.00v"; //longest battery voltage reading message = 9chars
byte sendLen;

void loop()
{
  if (battReadLoops--<=0) //only read battery every BATT_READ_LOOPS cycles
  {
    readBattery();
    battReadLoops = BATT_READ_LOOPS-1;
  }
  
  if (sendLoops--<=0)   //send readings every SEND_LOOPS
  {
    sendLoops = SEND_LOOPS-1;
    
    //read BME sensor
    bme280.begin();
    P = bme280.readFloatPressure() * 0.0002953; //read Pa and convert to inHg
    F = bme280.readTempF();
    H = bme280.readFloatHumidity();
    bme280.writeRegister(BME280_CTRL_MEAS_REG, 0x00); //sleep the BME280

    dtostrf(F, 3,2, Fstr);
    dtostrf(H, 3,2, Hstr);
    dtostrf(P, 3,2, Pstr);

    sprintf(buffer, "BAT:%sv F:%s H:%s P:%s", BATstr, Fstr, Hstr, Pstr);

    sendLen = strlen(buffer);
    radio.sendWithRetry(GATEWAYID, buffer, sendLen, 1); //retry one time
    DEBUG(buffer); DEBUG(" (packet length:"); DEBUG(sendLen); DEBUGln(")");

    #ifdef BLINK_EN
      Blink(LED, 5);
    #endif
  }
  
  //When this sketch is on a node where you can afford the power to keep the radio awake all the time
  //   you can make it receive messages and also make it wirelessly programmable
  //   otherwise this section can be removed
  if (radio.receiveDone())
  {
    boolean reportStatusRequest=false;
    DEBUG('[');DEBUG(radio.SENDERID);DEBUG("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      DEBUG((char)radio.DATA[i]);

    flash.wakeup();
    // wireless programming token check - this only works when radio is kept awake to listen for WP tokens
    CheckForWirelessHEX(radio, flash, true);

    //first send any ACK to request
    DEBUG("   [RX_RSSI:");DEBUG(radio.RSSI);DEBUG("]");
    if (radio.ACKRequested())
    {
      radio.sendACK();
      DEBUG(" - ACK sent.");
    }
    DEBUGln();
  }
  
  SERIALFLUSH();
  flash.sleep();
  radio.sleep(); //you can comment out this line if you want this node to listen for wireless programming requests
  LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF);
  DEBUGln("WAKEUP");
}

void readBattery()
{
  unsigned int readings=0;
  
  //enable battery monitor on WeatherShield (via mosfet controlled by A3)
  pinMode(BATT_MONITOR_EN, OUTPUT);
  digitalWrite(BATT_MONITOR_EN, LOW);

  for (byte i=0; i<5; i++) //take several samples, and average
    readings+=analogRead(BATT_MONITOR);
  
  //disable battery monitor
  pinMode(BATT_MONITOR_EN, INPUT); //highZ mode will allow p-mosfet to be pulled high and disconnect the voltage divider on the weather shield
    
  batteryVolts = BATT_FORMULA(readings / 5.0);
  dtostrf(batteryVolts,3,2, BATstr); //update the BATStr which gets sent every BATT_CYCLES or along with the MOTION message
  if (batteryVolts <= BATT_LOW) BATstr = "LOW";
}

void Blink(byte PIN, byte DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS/2);
  digitalWrite(PIN,LOW);
  delay(DELAY_MS/2);  
}
