// **********************************************************************************************************
// GarageMote garage door controller sketch that works with Moteinos equipped with RFM69W/RFM69HW/RFM69CW/RFM69HCW
// Monitors door position (open, closed, closing, unknown)
// Can trigger door open/close
// http://www.LowPowerLab.com/GarageMote
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
//#define WEATHERSHIELD            //uncomment if WeatherShield is present to report temp/humidity/pressure periodically
//#define WEATHERSENDDELAY  300000 // send WeatherShield data every so often (ms)
// ***************************************************************************************************************************
#include <RFM69.h>         //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <RFM69_OTA.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <SPIFlash.h>      //get it here: https://github.com/lowpowerlab/spiflash
#include <SPI.h>           //included with Arduino IDE (www.arduino.cc)

//For WeatherShield with BME280 see the WeatherNode example
#ifdef WEATHERSHIELD
  #include <SFE_BMP180.h>    //get it here: https://github.com/LowPowerLab/SFE_BMP180
  #include <SI7021.h>        //get it here: https://github.com/LowPowerLab/SI7021
  #include <Wire.h>
#endif
//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define GATEWAYID   1
#define NODEID      11
#define NETWORKID   250
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*****************************************************************************************************************************
#define ENABLE_ATC      //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI        -75
//*****************************************************************************************************************************
#define HALLSENSOR1          A0
#define HALLSENSOR1_EN        4
#define HALLSENSOR2          A1
#define HALLSENSOR2_EN        5

#define RELAYPIN1             6
#define RELAYPIN2             7
#define RELAY_PULSE_MS      250  //just enough that the opener will pick it up

#define DOOR_MOVEMENT_TIME 14000 // this has to be at least as long as the max between [door opening time, door closing time]
                                 // my door opens and closes in about 12s
#define STATUS_CHANGE_MIN  1500  // this has to be at least as long as the delay 
                                 // between a opener button press and door movement start
                                 // most garage doors will start moving immediately (within half a second)
//*****************************************************************************************************************************
#define HALLSENSOR_OPENSIDE   0
#define HALLSENSOR_CLOSEDSIDE 1

#define STATUS_CLOSED        0
#define STATUS_CLOSING       1
#define STATUS_OPENING       2
#define STATUS_OPEN          3
#define STATUS_UNKNOWN       4

#define LED                  9   //pin connected to onboard LED
#define LED_PULSE_PERIOD  5000   //5s seems good value for pulsing/blinking (not too fast/slow)
#define SERIAL_BAUD     115200
#define SERIAL_EN                //comment out if you don't want any serial output

#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input); delay(1);}
  #define DEBUGln(input) {Serial.println(input); delay(1);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif

#ifdef WEATHERSHIELD
  SI7021 weatherShield_SI7021;
  SFE_BMP180 weatherShield_BMP180;
#endif

//function prototypes
void setStatus(byte newSTATUS, boolean reportStatus=true);
void reportStatus();
boolean hallSensorRead(byte which);
void pulseRelay();

//global program variables
byte STATUS;
unsigned long lastStatusTimestamp=0;
unsigned long ledPulseTimestamp=0;
unsigned long lastWeatherSent=0;
int ledPulseValue=0;
boolean ledPulseDirection=false; //false=down, true=up
char Pstr[10];
char sendBuf[30];
SPIFlash flash(8, 0xEF30); //WINDBOND 4MBIT flash chip on CS pin D8 (default for Moteino)

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

void setup(void)
{
#ifdef SERIAL_EN
  Serial.begin(SERIAL_BAUD);
#endif
  pinMode(HALLSENSOR1, INPUT);
  pinMode(HALLSENSOR2, INPUT);
  pinMode(HALLSENSOR1_EN, OUTPUT);
  pinMode(HALLSENSOR2_EN, OUTPUT);
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  pinMode(LED, OUTPUT);
  
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  char buff[50];
  sprintf(buff, "GarageMote : %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buff);

  if (hallSensorRead(HALLSENSOR_OPENSIDE)==true)
    setStatus(STATUS_OPEN);
  if (hallSensorRead(HALLSENSOR_CLOSEDSIDE)==true)
    setStatus(STATUS_CLOSED);
  else setStatus(STATUS_UNKNOWN);

#ifdef WEATHERSHIELD
  //initialize weather shield sensors  
  weatherShield_SI7021.begin();
  if (weatherShield_BMP180.begin())
  { DEBUGln("BMP180 init success"); }
  else { DEBUGln("BMP180 init fail\n"); }
#endif
}

unsigned long doorPulseCount = 0;
char input=0;
double P;

void loop()
{
#ifdef SERIAL_EN
  if (Serial.available())
    input = Serial.read();
#endif

  if (input=='r')
  {
    DEBUGln("Relay test...");
    pulseRelay();
    input = 0;
  }
    
  // UNKNOWN => OPEN/CLOSED
  if (STATUS == STATUS_UNKNOWN && millis()-(lastStatusTimestamp)>STATUS_CHANGE_MIN)
  {
    if (hallSensorRead(HALLSENSOR_OPENSIDE)==true)
      setStatus(STATUS_OPEN);
    if (hallSensorRead(HALLSENSOR_CLOSEDSIDE)==true)
      setStatus(STATUS_CLOSED);
  }

  // OPEN => CLOSING
  if (STATUS == STATUS_OPEN && millis()-(lastStatusTimestamp)>STATUS_CHANGE_MIN)
  {
    if (hallSensorRead(HALLSENSOR_OPENSIDE)==false)
      setStatus(STATUS_CLOSING);
  }

  // CLOSED => OPENING  
  if (STATUS == STATUS_CLOSED && millis()-(lastStatusTimestamp)>STATUS_CHANGE_MIN)
  {
    if (hallSensorRead(HALLSENSOR_CLOSEDSIDE)==false)
      setStatus(STATUS_OPENING);
  }

  // OPENING/CLOSING => OPEN (when door returns to open due to obstacle or toggle action)
  //                 => CLOSED (when door closes normally from OPEN)
  //                 => UNKNOWN (when more time passes than normally would for a door up/down movement)
  if ((STATUS == STATUS_OPENING || STATUS == STATUS_CLOSING) && millis()-(lastStatusTimestamp)>STATUS_CHANGE_MIN)
  {
    if (hallSensorRead(HALLSENSOR_OPENSIDE)==true)
      setStatus(STATUS_OPEN);
    else if (hallSensorRead(HALLSENSOR_CLOSEDSIDE)==true)
      setStatus(STATUS_CLOSED);
    else if (millis()-(lastStatusTimestamp)>DOOR_MOVEMENT_TIME)
      setStatus(STATUS_UNKNOWN);
  }
  
  if (radio.receiveDone())
  {
    byte newStatus=STATUS;
    boolean reportStatusRequest=false;
    DEBUG('[');DEBUG(radio.SENDERID);DEBUG("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      DEBUG((char)radio.DATA[i]);

    if (radio.DATALEN==3)
    {
      //check for an OPEN/CLOSE/STATUS request
      if (radio.DATA[0]=='O' && radio.DATA[1]=='P' && radio.DATA[2]=='N')
      {
        if (millis()-(lastStatusTimestamp) > STATUS_CHANGE_MIN && (STATUS == STATUS_CLOSED || STATUS == STATUS_CLOSING || STATUS == STATUS_UNKNOWN))
          newStatus = STATUS_OPENING;
        //else radio.Send(requester, "INVALID", 7);
      }
      if (radio.DATA[0]=='C' && radio.DATA[1]=='L' && radio.DATA[2]=='S')
      {
        if (millis()-(lastStatusTimestamp) > STATUS_CHANGE_MIN && (STATUS == STATUS_OPEN || STATUS == STATUS_OPENING || STATUS == STATUS_UNKNOWN))
          newStatus = STATUS_CLOSING;
        //else radio.Send(requester, "INVALID", 7);
      }
      if (radio.DATA[0]=='S' && radio.DATA[1]=='T' && radio.DATA[2]=='S')
      {
        reportStatusRequest = true;
      }
    }
    
    // wireless programming token check
    // DO NOT REMOVE, or GarageMote will not be wirelessly programmable any more!
    CheckForWirelessHEX(radio, flash, true);

    //first send any ACK to request
    DEBUG("   [RX_RSSI:");DEBUG(radio.RSSI);DEBUG("]");
    if (radio.ACKRequested())
    {
      radio.sendACK();
      DEBUG(" - ACK sent.");
    }
    
    //now take care of the request, if not invalid
    if (STATUS != newStatus)
    {
      pulseRelay();
      setStatus(newStatus);
    }
    if (reportStatusRequest)
    {
      reportStatus();
    }
      
    DEBUGln();
  }
  
  //use LED to visually indicate STATUS
  if (STATUS == STATUS_OPEN || STATUS == STATUS_CLOSED) //solid ON/OFF
  {
    digitalWrite(LED, STATUS == STATUS_OPEN ? LOW : HIGH);
  }
  if (STATUS == STATUS_OPENING || STATUS == STATUS_CLOSING) //pulse
  {
    if (millis()-(ledPulseTimestamp) > LED_PULSE_PERIOD/256)
    {
      ledPulseValue = ledPulseDirection ? ledPulseValue + LED_PULSE_PERIOD/256 : ledPulseValue - LED_PULSE_PERIOD/256;

      if (ledPulseDirection && ledPulseValue > 255)
      {
        ledPulseDirection=false;
        ledPulseValue = 255;
      }
      else if (!ledPulseDirection && ledPulseValue < 0)
      {
        ledPulseDirection=true;
        ledPulseValue = 0;
      }
      
      analogWrite(LED, ledPulseValue);
      ledPulseTimestamp = millis();
    }
  }
  if (STATUS == STATUS_UNKNOWN) //blink
  {
    if (millis()-(ledPulseTimestamp) > LED_PULSE_PERIOD/20)
    {
      ledPulseDirection = !ledPulseDirection;
      digitalWrite(LED, ledPulseDirection ? HIGH : LOW);
      ledPulseTimestamp = millis();
    }
  }
  
#ifdef WEATHERSHIELD
  if (millis()-lastWeatherSent > WEATHERSENDDELAY)
  {
    lastWeatherSent = millis();
    P = getPressure();
    P*=0.0295333727; //transform to inHg
    dtostrf(P, 3,2, Pstr);
    sprintf(sendBuf, "F:%d H:%d P:%s", weatherShield_SI7021.getFahrenheitHundredths(), weatherShield_SI7021.getHumidityPercent(), Pstr);    
    byte sendLen = strlen(sendBuf);
    radio.send(GATEWAYID, sendBuf, sendLen);
  }
#endif
}

//returns TRUE if magnet is next to sensor, FALSE if magnet is away
boolean hallSensorRead(byte which)
{
  //while(millis()-lastStatusTimestamp<STATUS_CHANGE_MIN);
  digitalWrite(which ? HALLSENSOR2_EN : HALLSENSOR1_EN, HIGH); //turn sensor ON
  delay(1); //wait a little
  byte reading = digitalRead(which ? HALLSENSOR2 : HALLSENSOR1);
  digitalWrite(which ? HALLSENSOR2_EN : HALLSENSOR1_EN, LOW); //turn sensor OFF
  return reading==0;
}

void setStatus(byte newSTATUS, boolean reportIt)
{
  if (STATUS != newSTATUS) lastStatusTimestamp = millis();
  STATUS = newSTATUS;
  DEBUGln(STATUS==STATUS_CLOSED ? "CLOSED" : STATUS==STATUS_CLOSING ? "CLOSING" : STATUS==STATUS_OPENING ? "OPENING" : STATUS==STATUS_OPEN ? "OPEN" : "UNKNOWN");
  if (reportIt)
    reportStatus();
}

void reportStatus(void)
{
  char buff[10];
  sprintf(buff, STATUS==STATUS_CLOSED ? "CLOSED" : STATUS==STATUS_CLOSING ? "CLOSING" : STATUS==STATUS_OPENING ? "OPENING" : STATUS==STATUS_OPEN ? "OPEN" : "UNKNOWN");
  byte len = strlen(buff);
  radio.sendWithRetry(GATEWAYID, buff, len);
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
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

#ifdef WEATHERSHIELD
double getPressure()
{
  char status;
  double T,P,p0,a;
  // If you want sea-level-compensated pressure, as used in weather reports,
  // you will need to know the altitude at which your measurements are taken.
  // We're using a constant called ALTITUDE in this sketch:
  
  // If you want to measure altitude, and not pressure, you will instead need
  // to provide a known baseline pressure. This is shown at the end of the sketch.
  // You must first get a temperature measurement to perform a pressure reading.
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.
  status = weatherShield_BMP180.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.
    status = weatherShield_BMP180.getTemperature(T);
    if (status != 0)
    {
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.
      status = weatherShield_BMP180.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.
        status = weatherShield_BMP180.getPressure(P,T);
        if (status != 0)
        {
          return P;
        }
      }
    }        
  }
  return 0;
}
#endif