// *************************************************************************************************************
// SwitchMote Moteino sample sketch with PIR motion sensor support
// *************************************************************************************************************
// Handles the single 5A relay SwitchMote, as well as the dual 10A relay SwitchMote2x10A
// SwitchMote is a highly integrated wireless AC switch controller based on Moteino
// https://lowpowerlab.com/switchmote
// *************************************************************************************************************
// This sketch will provide the essential features of SwitchMote:
//   - wireless programming
//   - accept ON/OFF commands from a Moteino gateway (format is BTNx:y commands where x={0,1,2}, y={0,1})
//   - control the relay(s) to turn the AC load ON/OFF
//   - control up to 6 LEDs and 3 buttons on front panel
//   - SYNC feature allows out of box synchronization with other SwitchMotes
//     ie - when a button is pressed on this SwitchMote it can trigger the press of a button on another SwitchMote
//     so you can use a SwitchMote button to control a light/set of lights from remote locations
//     this sketch allows up to 5 SYNCs but could be extended
//   - Panasonic PIR motion sensor support in place of middle button
// This sketch may be extended to include integration with other LowPowerLab automation products, for instance to
//    control the GarageMote from a button on the SwitchMote, etc.
// *************************************************************************************************************
// Copyright Felix Rusu 2017, http://www.LowPowerLab.com/contact
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
#include <EEPROMex.h>      //get it here: http://playground.arduino.cc/Code/EEPROMex
#include <RFM69.h>         //get it here: http://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_OTA.h>     //get it here: https://github.com/LowPowerLab/rfm69
#include <SPIFlash.h>      //get it here: http://github.com/lowpowerlab/spiflash
#include <SPI.h>           //comes with Arduino
// **********************************************************************************
//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#define ENABLE_ATC         //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI          -75
// **********************************************************************************
#define GATEWAYID           1  //always send messages to NodeID=1
// **********************************************************************************
#define PIRPRESENT         //un-comment if you have a panasonic PIR instead of the MIDDLE button

#ifdef PIRPRESENT
  #define PIR_POWER        18 //PIR powered by D18
  #define PIR_OUTPUT        5 //PIR output signal to D5 - SHARED WITH middle button!
  #define PIR_DEBOUNCE   6000 //PIR signals valid at least this many ms apart
  #define PIR_LED_ON     3000
#endif
// **********************************************************************************
#define BTNCOUNT            2  //1 or 3 (2 also possible)
#define BTNT                6  //digital pin of top button
#define BTNB                4  //digital pin of bottom button

#define LED_RT             16  //digital pin for TOP RED LED
#define LED_GT             19  //digital pin for TOP GREEN LED
#define LED_RM             15  //digital pin for MIDDLE RED LED
//#define LED_GM             18  //digital pin for MIDDLE GREEN LED
#define LED_RM             15  //digital pin for MIDDLE RED LED
#define LED_RB             14  //digital pin for BOTTOM RED LED
#define LED_GB             17  //digital pin for BOTTOM GREEN LE

#define RELAY1              7  //digital pin connected to MAIN relay
#define RELAY2              3  //digital pin connected to secondary relay (SwitchMote 2x10A only)
#define RELAY1_INDEX        0  //index in btn[] array which is associated with the MAIN relay
#define RELAY2_INDEX        1  //index in btn[] array which is associated with the secondary relay (SwitchMote 2x10A only)

#define BUTTON_BOUNCE_MS  200  //timespan before another button change can occur
#define SYNC_ENTER       3000  //time required to hold a button before SwitchMote enters [SYNC mode]
#define SYNC_TIME       20000  //max time spent in SYNC mode before returning to normal operation (you got this much time to SYNC 2 SMs, increase if need more time to walk)
#define SYNC_MAX_COUNT     10  //max number of SYNC entries (increase for more interactions)
#define SYNC_EEPROM_ADDR   64  //SYNC_TO and SYNC_INFO data starts at this EEPROM address
#define ERASE_HOLD       6000  //time required to hold a button before SYNC data is erased
#define MOTION_TIME_ON  60000  //time to hold a button/output HIGH after a motion triggered command
// **********************************************************************************
//in SYNC_INFO we're storing 4 pieces of information in each byte:
#define SYNC_DIGIT_THISBTN  0 //first digit is the button pressed on this unit which will trigger an action on another unit
#define SYNC_DIGIT_THISMODE 1 //second digit indicates the mode of this unit is in when triggering
#define SYNC_DIGIT_SYNCBTN  2 //third digit indicates the button that should be requested to be "pressed" on the target
#define SYNC_DIGIT_SYNCMODE 3 //fourth digit indicates the mode that should be requested on the target
#define SYNC_MIN_TIME_LIMIT 2000 //minimum time limit since last SYNC before a new sync can be propagated (used to stop circular SYNC loops)
// **********************************************************************************
#define SERIAL_EN             //comment this out when deploying to an installed SM to save a few KB of sketch size
#define SERIAL_BAUD    115200
#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input); delay(1);}
  #define DEBUGln(input) {Serial.println(input); delay(1);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif
// **********************************************************************************
#define LED_PERIOD_ERROR   50
#define LED_PERIOD_OK     200
#define ON                  1
#define OFF                 0
#define PRESSED             0
#define RELEASED            1
#define FLASHMEM_CS         8 //FLASH_MEM SPI CS on D8
// **********************************************************************************
//Structure used to store Moteino configuration in EEPROM
struct configuration {
  byte frequency;
  byte isHW;
  byte nodeID;
  byte networkID;
  char encryptionKey[16];
  byte separator1;
  char description[10];
  byte separator2;
  //byte buttons?
  //byte DEBUG?
} CONFIG;

// *************************************************************************************************************
//compiler wants these function prototype here because of the optional parameter(s)
void action(byte whichButtonIndex, byte whatMode, boolean notifyGateway=true);
boolean checkSYNC(byte nodeIDToSkip=0);
// *************************************************************************************************************
//SYNC data is stored in 2 arrays:
byte SYNC_TO[SYNC_MAX_COUNT];  // stores the address of the remote SM(s) that this SM has to notify/send request to
int SYNC_INFO[SYNC_MAX_COUNT]; // stores the buttons and modes of this and the remote SM as last 4 digits:
                               //   - this SM button # (0,1,2) = least significant digit (SYNC_DIGIT_BTN=0)
                               //   - this button mode (0,1) = second digit ((SYNC_DIGIT_THISMODE=1)
                               //   - remote SM button # (0,1,2) = 3rd digit from right (SYNC_DIGIT_SYNCBTN=2)
                               //   - remote SM mode (0,1) = most significant digit (SYNC_DIGIT_SYNCMODE=3)
                               // the 4 pieces of information require an int (a byte only has up to 3 digits)
#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

SPIFlash flash(FLASHMEM_CS, 0xEF30);     //FLASH MEM CS pin is wired to Moteino D8
unsigned long syncStart=0;
unsigned long now=0;
byte btnIndex=0; // as the sketch loops this index will loop through the available physical buttons
byte mode[] = {ON,ON,ON}; //could use single bytes for efficiency but keeping it separate for clarity
byte btn[] = {BTNT, BTNB};
byte btnLastState[]={RELEASED,RELEASED,RELEASED};
unsigned long btnLastPress[]={0,0,0};
byte btnLEDRED[] = {LED_RT, LED_RB};
byte btnLEDGRN[] = {LED_GT, LED_GB};
uint32_t lastSYNC=0; //remember last status change - used to detect & stop loop conditions in circular SYNC scenarios
char * buff="justAnEmptyString";

#ifdef PIRPRESENT
  boolean PIR_MOTION_RELAY=false;
#endif

void setup(void)
{
  #ifdef SERIAL_EN
    Serial.begin(SERIAL_BAUD);
  #endif
  EEPROM.readBlock(0, CONFIG);
  if (CONFIG.frequency!=RF69_433MHZ && CONFIG.frequency!=RF69_868MHZ && CONFIG.frequency!=RF69_915MHZ) // virgin CONFIG, expected [4,8,9]
  {
    DEBUGln(F("\r\nNo valid config found in EEPROM, setting defaults.."));
    CONFIG.separator1=CONFIG.separator2=0;
    CONFIG.frequency=RF69_915MHZ;
    CONFIG.description[0]=0;
    CONFIG.encryptionKey[0]=0;
    CONFIG.isHW=CONFIG.nodeID=CONFIG.networkID=0;
  }
  
  //if SYNC_INFO[0] == 255 it means it's virgin EEPROM memory, needs initialization (one time ever)
  if (EEPROM.read(SYNC_EEPROM_ADDR+SYNC_MAX_COUNT)==255) eraseSYNC();  
  EEPROM.readBlock<byte>(SYNC_EEPROM_ADDR, SYNC_TO, SYNC_MAX_COUNT);
  EEPROM.readBlock<byte>(SYNC_EEPROM_ADDR+SYNC_MAX_COUNT, (byte *)SYNC_INFO, SYNC_MAX_COUNT*2); //int=2bytes so need to cast to byte array

  radio.initialize(CONFIG.frequency,CONFIG.nodeID,CONFIG.networkID);
  radio.sleep();

  if (CONFIG.isHW) radio.setHighPower(); //must include this only for RFM69HW/HCW!
  if (CONFIG.encryptionKey[0]!=0) radio.encrypt(CONFIG.encryptionKey);
  
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
  DEBUGln(F("\r\nRFM69_ATC Enabled (Auto Transmission Control)"));
#endif
  
  pinMode(LED_RM, OUTPUT);//pinMode(LED_GM, OUTPUT);
  pinMode(LED_RT, OUTPUT);pinMode(LED_GT, OUTPUT);
  pinMode(LED_RB, OUTPUT);pinMode(LED_GB, OUTPUT);
  // by writing HIGH while in INPUT mode, the internal pullup is activated
  // the button will read 1 when RELEASED (because of the pullup)
  // the button will read 0 when PRESSED (because it's shorted to GND)
  //pinMode(BTNM, INPUT);digitalWrite(BTNM, HIGH); //activate pullup
  pinMode(BTNT, INPUT);digitalWrite(BTNT, HIGH); //activate pullup
  pinMode(BTNB, INPUT);digitalWrite(BTNB, HIGH); //activate pullup
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  blinkLED(LED_RM,LED_PERIOD_ERROR,LED_PERIOD_ERROR,3);
  delay(500);
  DEBUGln("Listening for wireless ON/OFF commands...");

  DEBUG(F("|-----------------------------------------------------------"));
  //initialize LEDs according to default modes
  action(btnIndex, OFF, false);btnIndex++;
  action(btnIndex, OFF, false);
  
  displayMainMenu();

#ifdef PIRPRESENT
  pinMode(PIR_POWER, OUTPUT);
  PIR_ONOFF(HIGH); //give power to the PIR
  digitalWrite(PIR_OUTPUT, LOW); //cancel the pullup activation from BTNM above
  digitalWrite(LED_RM, LOW);
#endif
}

byte btnState=RELEASED;
boolean isSyncMode=0;
boolean ignorePress=false;
unsigned long int offTimer=0;
byte offIndex=0;

#ifdef PIRPRESENT
  byte motionDetected=false;
  unsigned long lastMotionTime=0;
#endif

void PIR_ONOFF(byte state) {
  digitalWrite(PIR_POWER, state);
}

void loop()
{
  if(Serial.available())
    handleMenuInput(Serial.read());

#ifdef PIRPRESENT
  //check motion sensor
  if (digitalRead(PIR_OUTPUT) && motionDetected==false && millis()-lastMotionTime > PIR_DEBOUNCE)
  {
    motionDetected=true;
    digitalWrite(LED_RM, HIGH);
    lastMotionTime = millis();

    if (radio.sendWithRetry(GATEWAYID, "MOTION", 6))
    {
      DEBUG("MOTION ACK:OK! RSSI:");
      DEBUGln(radio.RSSI);
    }
    else DEBUGln("MOTION ACK:NOK...");
    
    if (PIR_MOTION_RELAY)
    {
      offTimer = millis();
      if(mode[offIndex] != ON) //only take action when mode is not already ON
      {
        action(offIndex, ON, true); //senderID!=GATEWAYID
        checkSYNC(0);
      }
    }
  }
  else if (motionDetected && millis()-lastMotionTime > PIR_LED_ON) //RED-LED-MIDDLE light up for 3 sec
  {
    motionDetected = false;
    digitalWrite(LED_RM, LOW);
  }
#endif

  //on each loop pass check the next button
  if (isSyncMode==false)
  {
    btnIndex++;
    if (btnIndex>BTNCOUNT-1) btnIndex=0;
  }
  btnState = digitalRead(btn[btnIndex]);
  now = millis();

  if (btnState != btnLastState[btnIndex] && now-btnLastPress[btnIndex] >= BUTTON_BOUNCE_MS) //button event happened
  {
    btnLastState[btnIndex] = btnState;
    if (btnState == PRESSED) btnLastPress[btnIndex] = now;

    //if normal button press, do the RELAY/LED action and notify sync-ed SwitchMotes
    if (btnState == RELEASED && !isSyncMode)
    {
      DEBUG("BTN PRESS: ");DEBUGln(btnIndex);
      
      //when PIR is installed, both button pressed toggles PIR ON/OFF
#ifdef PIRPRESENT
      if (BTNCOUNT == 2)
      {
        byte otherBtnIndex = (btnIndex == 0 ? 1:0);
        if (digitalRead(btn[otherBtnIndex]) == RELEASED && now-btnLastPress[otherBtnIndex] >= BUTTON_BOUNCE_MS && now-btnLastPress[otherBtnIndex] <= 2*BUTTON_BOUNCE_MS)
        {
          //other button was also pressed at same time, toggle PIR-motion->RELAY-ON function
          PIR_MOTION_RELAY = !PIR_MOTION_RELAY;
          DEBUG("PIR_MOTION_RELAY: ");DEBUGln(PIR_MOTION_RELAY);
          return; //don't do anything else
        }
      }
#endif

      ignorePress=false;
      action(btnIndex, mode[btnIndex]==ON ? OFF : ON);
      checkSYNC(0);
    }
  }

  //enter SYNC mode when a button pressed for more than SYNC_ENTER ms
  if (isSyncMode==false && btnState == PRESSED && now-btnLastPress[btnIndex] >= SYNC_ENTER && !ignorePress)
  {
    // first broadcast SYNC token to sync with another SwitchMote that is in SYNC mode
    // "SYNC?" means "is there anyone wanting to Synchronize with me?"
    // response "SYNCx:0" (meaning "OK, SYNC with me and turn my button x OFF")
    // response "SYNCx:1" (meaning "OK, SYNC with me and turn my button x ON")
    // no response means no other SwMote in range is in SYNC mode, so enter SYNC and
    //    listen for another SwMote to broadcast its SYNC token
    if (radio.sendWithRetry(RF69_BROADCAST_ADDR,"SYNC?",5))
    {
      DEBUG(F("GOT SYNC? REPLY FROM ["));
      DEBUG(radio.SENDERID);
      DEBUG(F(":"));DEBUG(radio.DATALEN);DEBUG(F("]:["));
      for (byte i = 0; i < radio.DATALEN; i++)
        DEBUG((char)radio.DATA[i]);
      DEBUGln(F("]"));

      //ACK received, check payload
      if (radio.DATALEN==7 && radio.DATA[0]=='S' && radio.DATA[1]=='Y' && radio.DATA[2]=='N' && radio.DATA[3]=='C' && radio.DATA[5]==':'
          && radio.DATA[4]>='0' && radio.DATA[4]<='2' && (radio.DATA[6]=='0' || radio.DATA[6]=='1'))
      {
        if (addSYNC(radio.SENDERID, radio.DATA[4]-'0', radio.DATA[6]-'0'))
          blinkLED(btnLEDGRN[btnIndex],LED_PERIOD_OK,LED_PERIOD_OK,3);
        else 
          blinkLED(btnLEDRED[btnIndex],LED_PERIOD_ERROR,LED_PERIOD_ERROR,3);

        action(btnIndex, mode[btnIndex]);
        return; //exit SYNC
      }
      else
      {
        DEBUG(F("SYNC ACK mismatch: ["));
        for (byte i = 0; i < radio.DATALEN; i++)
          DEBUG((char)radio.DATA[i]);
        DEBUGln(']');
      }
    }
    else { DEBUGln(F("NO SYNC REPLY ..")); }

    isSyncMode = true;
    DEBUGln(F("SYNC MODE ON"));
    displaySYNC();
    syncStart = now;
  }

  //if button held for more than ERASE_TRIGGER, erase SYNC table
  if (isSyncMode==true && btnState == PRESSED && now-btnLastPress[btnIndex] >= ERASE_HOLD && !ignorePress)
  {
    DEBUG(F("ERASING SYNC TABLE ... "));
    eraseSYNC();
    isSyncMode = false;
    ignorePress = true;
    DEBUGln(F("... DONE"));
    blinkLED(btnLEDRED[btnIndex],LED_PERIOD_ERROR,LED_PERIOD_ERROR,3);
    action(btnIndex, mode[btnIndex], false);
  }

  //SYNC exit condition
  if (isSyncMode)
  {
    syncBlink(btnLEDRED[btnIndex], btnLEDGRN[btnIndex]);
    if (now-syncStart >= SYNC_TIME)
    {
      isSyncMode = false;
      DEBUGln(F("SYNC MODE OFF"));
      action(btnIndex, mode[btnIndex], false);
    }
  }

  //check if any packet received
  if (radio.receiveDone())
  {
    byte senderID = radio.SENDERID;
    DEBUGln();
    DEBUG("[");DEBUG(senderID);DEBUG("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      DEBUG((char)radio.DATA[i]);
    DEBUG(F(" [RX_RSSI:"));DEBUG(radio.RSSI);DEBUG(F("]"));

    // wireless programming token check
    // DO NOT REMOVE, or SwitchMote will not be wirelessly programmable any more!
    CheckForWirelessHEX(radio, flash, true, LED_RM);

    //respond to SYNC request
    if (isSyncMode && radio.DATALEN == 5
        && radio.DATA[0]=='S' && radio.DATA[1]=='Y' && radio.DATA[2]=='N' && radio.DATA[3] == 'C' && radio.DATA[4]=='?')
    {
      sprintf(buff,"SYNC%d:%d",btnIndex, mode[btnIndex]); //respond to SYNC request with this SM's button and mode information
      radio.sendACK(buff, strlen(buff));
      DEBUG(F(" - SYNC ACK sent : "));
      DEBUGln(buff);
      isSyncMode = false;
      action(btnIndex, mode[btnIndex], false);
      return; //continue loop
    }

    //listen for BTNx:y commands where x={0,1,2}, y={0,1}
    if (radio.DATALEN == 6
        && radio.DATA[0]=='B' && radio.DATA[1]=='T' && radio.DATA[2]=='N' && radio.DATA[4] == ':'
        && (radio.DATA[3]>='0' && radio.DATA[3]<='2') && (radio.DATA[5]=='0' || radio.DATA[5]=='1'))
    {
      if (radio.ACKRequested()) radio.sendACK(); //send ACK sooner when a ON/OFF + ACK is requested
      btnIndex = radio.DATA[3]-'0';
      action(btnIndex, (radio.DATA[5]=='1'?ON:OFF), true); //senderID!=GATEWAYID
      checkSYNC(senderID);
    }

    //listen for MOT:x commands where x={0,1,2} - motion activated command to turn ON a button/relay (timed ON)
    if (radio.DATALEN == 5
        && radio.DATA[0]=='M' && radio.DATA[1]=='O' && radio.DATA[2]=='T' && radio.DATA[3] == ':'
        && (radio.DATA[4]>='0' && radio.DATA[4]<='2'))
    {
      if (radio.ACKRequested()) radio.sendACK(); //send ACK sooner when a ON/OFF + ACK is requested
      btnIndex = radio.DATA[4]-'0';
      //if(mode[btnIndex] != ON) //if a light is already ON, ignore MOTION triggered commands and do nothing, uncomment this line to 
      offTimer = millis();
      offIndex = btnIndex;
      if(mode[btnIndex] != ON) //only take action when mode is not already ON
      {
        action(btnIndex, ON, true); //senderID!=GATEWAYID
        checkSYNC(senderID);
      }
    }

    if (radio.ACKRequested()) //dont ACK broadcasted messages except in special circumstances (like SYNCing)
    {
      radio.sendACK();
      DEBUG(F(" - ACK sent"));
      //delay(5);
    }
  }
  
  //check if a motion command timer expired and the corresponding light can turn off    
  if ((offTimer > 0) && (millis() - offTimer > MOTION_TIME_ON))
  {
    offTimer = 0;
    if(mode[offIndex] != OFF)
    {
      DEBUGln(F("OffTimer expired, turning off"));
      action(offIndex, OFF, true);
      checkSYNC(0);
    }
  }
}

//sets the mode (ON/OFF) for the current button (btnIndex) and turns SSR ON if the btnIndex points to BTNSSR
void action(byte whichButtonIndex, byte whatMode, boolean notifyGateway)
{
  DEBUG(F("\r\nbtn["));
  DEBUG(whichButtonIndex);
  DEBUG(F("]:D"));
  DEBUG(btn[whichButtonIndex]);
  DEBUG(F(" - "));
  //DEBUG(btn[whichButtonIndex]==BTNT?F("TOP:    "):btn[whichButtonIndex]==BTNM?F("MAIN:   "):btn[whichButtonIndex]==BTNB?F("BOTTOM: "):F("UNKNOWN"));
  DEBUG(btn[whichButtonIndex]==BTNT?F("TOP:    "):btn[whichButtonIndex]==BTNB?F("BOTTOM: "):F("UNKNOWN"));
  DEBUG(whatMode==ON?F("ON "):F("OFF"));

  mode[whichButtonIndex] = whatMode;

  //change LEDs and relay states
  digitalWrite(btnLEDRED[whichButtonIndex], whatMode == ON ? LOW : HIGH);
  digitalWrite(btnLEDGRN[whichButtonIndex], whatMode == ON ? HIGH : LOW);
  if (whichButtonIndex==RELAY1_INDEX)
    digitalWrite(RELAY1, whatMode == ON ? HIGH : LOW);
  if (whichButtonIndex==RELAY2_INDEX)
    digitalWrite(RELAY2, whatMode == ON ? HIGH : LOW);    //SwitchMote2x10A has 2 10A relays

  //notify gateway
  if (notifyGateway)
  {
    sprintf(buff, "BTN%d:%d", whichButtonIndex,whatMode);
    if (radio.sendWithRetry(GATEWAYID, buff, strlen(buff), 5)) //up to 5 attempts
      {DEBUGln(F("..OK"));}
    else {DEBUGln(F("..NOK"));}
  }
}

long blinkLastCycle=0;
boolean blinkState=0;
void syncBlink(byte LED1, byte LED2)
{
  if (now-blinkLastCycle>=60)
  {
    blinkLastCycle=now;
    blinkState=!blinkState;
    digitalWrite(LED1,blinkState);
    digitalWrite(LED2,!blinkState);
  }
}

//adds a new entry in the SYNC data
boolean addSYNC(byte targetAddr, byte targetButton, byte targetMode)
{
  byte emptySlot=255;
  if (targetAddr==0) return false;

  //traverse all SYNC data and look for an empty slot, or matching slot that should be overwritten
  for (byte i=0; i < SYNC_MAX_COUNT; i++)
  {
    if (SYNC_TO[i]==0 && emptySlot==255) //save first empty slot
      emptySlot=i; //remember first empty slot anyway
    else if (SYNC_TO[i]==targetAddr &&   //save first slot that matches the same button with the same mode in this unit
                                         //but different target unit mode (cant have 2 opposing conditions so just override it)
           getDigit(SYNC_INFO[i],SYNC_DIGIT_SYNCBTN)==targetButton &&
           getDigit(SYNC_INFO[i],SYNC_DIGIT_THISMODE)==mode[btnIndex]) //getDigit(SYNC_INFO[i],SYNC_DIGIT_SYNCNODE)!=targetMode)
    {
      emptySlot=i; //remember matching non-empty slot
      break; //stop as soon as we found a match
    }
  }

  if (emptySlot==255) //means SYNC data is full, do nothing and return
  {
    DEBUGln(F("SYNC data full, aborting..."));
    return false;
  }
  else
  {
    SYNC_TO[emptySlot] = targetAddr;
    SYNC_INFO[emptySlot] = targetMode*1000 + targetButton*100 + mode[btnIndex]*10 + btnIndex;
    DEBUG(F("Saving SYNC_TO["));
    DEBUG(emptySlot);
    DEBUG(F("]="));
    DEBUG(SYNC_TO[emptySlot]);
    DEBUG(F(" SYNC_INFO = "));
    DEBUG(SYNC_INFO[emptySlot]);    
    saveSYNC();
    DEBUGln(F(" .. Saved!"));
    return true;
  }
}

//checks the SYNC table for any necessary requests to other SwitchMotes
boolean checkSYNC(byte nodeIDToSkip)
{
  for (byte i=0; i < SYNC_MAX_COUNT; i++)
  {
    if (SYNC_TO[i]!=0 && SYNC_TO[i]!=nodeIDToSkip && getDigit(SYNC_INFO[i],SYNC_DIGIT_THISBTN)==btnIndex && getDigit(SYNC_INFO[i],SYNC_DIGIT_THISMODE)==mode[btnIndex])
    {
      DEBUGln();
      DEBUG(F(" SYNC["));
      DEBUG(SYNC_TO[i]);
      DEBUG(F(":"));
      DEBUG(getDigit(SYNC_INFO[i],SYNC_DIGIT_SYNCMODE));
      DEBUG(F("]:"));
      sprintf(buff, "BTN%d:%d", getDigit(SYNC_INFO[i],SYNC_DIGIT_SYNCBTN), getDigit(SYNC_INFO[i],SYNC_DIGIT_SYNCMODE));
      if (radio.sendWithRetry(SYNC_TO[i], buff,6))
      {DEBUG(F("OK"));}
      else {DEBUG(F("NOK"));}
    }
  }
}

void eraseSYNC()
{
  for(byte i = 0; i<SYNC_MAX_COUNT; i++)
  {
    SYNC_TO[i]=0;
    SYNC_INFO[i]=0;
  }
  saveSYNC();
}

//saves SYNC_TO and SYNC_INFO arrays to EEPROM
void saveSYNC()
{
  EEPROM.writeBlock<byte>(SYNC_EEPROM_ADDR, SYNC_TO, SYNC_MAX_COUNT);
  EEPROM.writeBlock<byte>(SYNC_EEPROM_ADDR+SYNC_MAX_COUNT, (byte*)SYNC_INFO, SYNC_MAX_COUNT*2);
}

void displaySYNC()
{
  for (byte i=0; i < SYNC_MAX_COUNT; i++)
  {
    DEBUG(SYNC_INFO[i]);
    if (i!=SYNC_MAX_COUNT-1) DEBUG(',');
  }
}

//returns the Nth digit in an integer
byte getDigit(int n, byte pos){return (n/(pos==0?1:pos==1?10:pos==2?100:1000))%10;}

void blinkLED(byte LEDpin, byte periodON, byte periodOFF, byte repeats)
{
  while(repeats-->0)
  {
    digitalWrite(LEDpin, HIGH);
    delay(periodON);
    digitalWrite(LEDpin, LOW);
    delay(periodOFF);
  }
}

/*CONFIGURATION HELPERS*/
void displayMainMenu()
{
  Serial.println();
  Serial.println(F("|-----------------------------------------------------------"));
  Serial.println(F("|  SwitchMote RFM69 configuration menu "));
  Serial.println(F("|  Use Putty or a similar client to setup params"));
  Serial.println(F("|  ArduinoIDE serial monitor doesn't work well"));
  Serial.println(F("|  Don't forget to save 's' and reboot 'r' to apply changes"));
  Serial.println(F("|-----------------------------------------------------------"));
  Serial.print  (F("| f - set frequency band    (set to: "));Serial.print(CONFIG.frequency==RF69_433MHZ?F("433"):CONFIG.frequency==RF69_868MHZ?F("868"):F("915"));Serial.println(F("mhz)"));
  Serial.print  (F("| i - set node ID           (set to: "));Serial.print(CONFIG.nodeID);Serial.println(F(")"));
  Serial.print  (F("| n - set network ID        (set to: "));Serial.print(CONFIG.networkID);Serial.println(")");
  Serial.print  (F("| w - set RFM69 type        (set to: "));Serial.print(CONFIG.isHW?F("HW/HCW"):F("W/CW"));Serial.println(F(")"));
  Serial.print  (F("| e - set encryption key    (set to: '"));Serial.print(CONFIG.encryptionKey);Serial.println(F("')"));
  Serial.print  (F("| d - set description       (set to: '"));Serial.print(CONFIG.description);Serial.println(F("')"));
  Serial.println(F("| s - save CONFIG to EEPROM"));
  Serial.println(F("| E - erase whole EEPROM - [0..1023]"));
  Serial.print  (F("| S - erase SYNC data ["));Serial.print(SYNC_EEPROM_ADDR);Serial.print(F(".."));Serial.print(SYNC_EEPROM_ADDR+3*SYNC_MAX_COUNT-1);Serial.print(F("]:["));
  displaySYNC();
  Serial.println(']');
  Serial.println(F("| r - reboot"));
  Serial.println(F("| ESC - re-display config menu"));
  Serial.println(F("|-----------------------------------------------------------"));
  Serial.println(F("|  Usage ex.: press 'f' to change frequency"));
  Serial.println(F("|-----------------------------------------------------------"));
}

char menu = 0;
byte charsRead = 0;
void handleMenuInput(char c)
{
  switch(menu)
  {
    case 0:
      switch(c)
      {
        case 'f': Serial.print(F("\r\nEnter frequency ('4'= 433mhz, '8'=868mhz, '9'=915mhz): ")); menu=c; break;
        case 'i': Serial.print(F("\r\nEnter node ID (1-255 + <ENTER>): ")); CONFIG.nodeID=0;menu=c; break;
        case 'n': Serial.print(F("\r\nEnter network ID (0-255 + <ENTER>): ")); CONFIG.networkID=0; menu=c; break;
        case 'e': Serial.print(F("\r\nEnter encryption key (type 16 characters): ")); menu=c; break;
        case 'w': Serial.print(F("\r\nIs this RFM69W/CW/HW (0=W/CW, 1=HW/HCW): ")); menu=c; break;
        case 'd': Serial.print(F("\r\nEnter description (10 chars max + <ENTER>): ")); menu=c; break;
        case 's': Serial.print(F("\r\nCONFIG saved to EEPROM!")); EEPROM.writeBlock(0, CONFIG); break;
        case 'E': Serial.print(F("\r\nErasing EEPROM ... ")); menu=c; break;
        case 'S': Serial.print(F("\r\nErasing SYNC EEPROM ... ")); menu=c; break;
        case 'r': Serial.print(F("\r\nRebooting")); resetUsingWatchdog(1); break;
        case  27: displayMainMenu();menu=0;break;
      }
      break;

    case 'f':
      switch(c)
      {
        case '4': Serial.println(F("Set to 433Mhz")); CONFIG.frequency = RF69_433MHZ; menu=0; break;
        case '8': Serial.println(F("Set to 868Mhz")); CONFIG.frequency = RF69_868MHZ; menu=0; break;
        case '9': Serial.println(F("Set to 915Mhz")); CONFIG.frequency = RF69_915MHZ; menu=0; break;
        case  27: displayMainMenu();menu=0;break;
      }
      break;

    case 'i':
      if (c >= '0' && c <= '9')
      {
        if (CONFIG.nodeID * 10 + c - 48 <= 255)
        {
          CONFIG.nodeID = CONFIG.nodeID * 10 + c - 48;
          Serial.print(c);
        }
        else
        {
          Serial.print(" - Set to ");Serial.println(CONFIG.nodeID);
          menu=0;
        }
      }
      else if (c == 13 || c == 27)
      {
        Serial.print(F(" - Set to "));Serial.println(CONFIG.nodeID);
        displayMainMenu();
        menu=0;
      }
      break;

    case 'n':
      if (c >= '0' && c <= '9')
      {
        if (CONFIG.networkID * 10 + c - 48 <= 255)
        {
          CONFIG.networkID = CONFIG.networkID * 10 + c - 48;
          Serial.print(c);
        }
        else
        {
          Serial.print(" - Set to ");Serial.println(CONFIG.networkID);
          menu=0;
        }
      }
      if (c == 13 || c == 27)
      {
        Serial.print(" - Set to ");Serial.println(CONFIG.networkID);
        displayMainMenu();
        menu=0;
      }
      break;

    case 'e':
      if (c >= ' ' && c <= '~') //human readable chars (32 - 126)
        if (++charsRead<=16)
        {
          CONFIG.encryptionKey[charsRead-1] = c;
          CONFIG.encryptionKey[charsRead] = 0;
          Serial.print(c);
        }
      if (charsRead >= 16 || c == 27 || c == 13)
      {
        //Serial.print(" - Set to [");Serial.print(CONFIG.encryptionKey);Serial.println(']');
        Serial.println(F(" - DONE"));
        displayMainMenu();menu=0;charsRead=0;
      }
      break;

    case 'd':
      if (c >= ' ' && c <= '~') //human readable chars (32 - 126)
      {
        if (++charsRead<=10)
        {
          CONFIG.description[charsRead-1] = c;
          CONFIG.description[charsRead] = 0;
          Serial.print(c);
        }
      }
      if (charsRead>=10 || c == 13 || c == 27)
      {
        //Serial.print(" - Set to [");Serial.print(CONFIG.description);Serial.println(']');
        Serial.println(F(" - DONE"));
        displayMainMenu();menu=0;charsRead=0;
      }
      break;

    case 'w':
      switch(c)
      {
        case '0': Serial.println(F("Set to RFM69W\\CW")); CONFIG.isHW = 0; menu=0; break;
        case '1': Serial.println(F("Set to RFM69HW\\HCW")); CONFIG.isHW = 1; menu=0; break;
        case  27: displayMainMenu();menu=0;break;
      }
      break;
    case 'E':
      for (int i=0;i<1024;i++) EEPROM.write(i,255); //eeprom_write_byte((unsigned char *) i, 255);
      Serial.println(F("DONE"));
      //resetUsingWatchdog(1);
      menu=0;
      break;
    case 'S':
      eraseSYNC();
      Serial.println(F("DONE"));
      menu=0;
      break;
  }
}