// Sample RFM69 sketch for the MotionOLED mote containing the OLED
// Displays any messages on the network on the OLED display and beeps the buzzer every time a message is received
// The side button will step through 10 past received messages
// Library and code by Felix Rusu - felix@lowpowerlab.com
// Get libraries at: https://github.com/LowPowerLab/
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
#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h> //get library from: https://www.github.com/lowpowerlab/spiflash
#include <LowPower.h> //get library from: https://github.com/lowpowerlab/lowpower
#include "U8glib.h"   //get library from: https://code.google.com/p/u8glib/
#include <SPI.h>      //included with Arduino IDE (www.arduino.cc)

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID        122    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************

#define SERIAL_BAUD   115200
#define LED           5 // Moteinos have LEDs on D9, but for MotionMote we are using the external led on D5
#define BUZZER        6
#define BUTTON_INT    1 //user button on interrupt 1
#define BUTTON_PIN    3 //user button on interrupt 1

RFM69 radio;
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE); // I2C / TWI SSD1306 OLED 128x64
bool promiscuousMode = true; //set to 'true' to sniff all packets on the same network

void setup() {
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  pinMode(BUZZER, OUTPUT);

  //configure OLED
  u8g.setRot180(); //flip screen
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 )
    u8g.setColorIndex(255);     // white
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT )
    u8g.setColorIndex(3);         // max intensity
  else if ( u8g.getMode() == U8G_MODE_BW )
    u8g.setColorIndex(1);         // pixel on
  else if ( u8g.getMode() == U8G_MODE_HICOLOR )
    u8g.setHiColorByRGB(255,255,255);
  u8g.begin();
  Serial.flush();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_INT, handleButton, FALLING);
}

#define FLAG_INTERRUPT 0x01
volatile int mainEventFlags = 0;
boolean buttonPressed = false;
void handleButton()
{
  mainEventFlags |= FLAG_INTERRUPT;
}

byte ackCount=0;

#define MSG_MAX_LEN   17    //OLED 1 line max # of chars (16 + EOL)
#define HISTORY_LEN   10    //hold this many past messages
typedef struct {
  char data[MSG_MAX_LEN];
  int rssi;
  byte from;
} Message;
Message * messageHistory = new Message[HISTORY_LEN];

byte lastMessageIndex = HISTORY_LEN;
byte currMessageIndex = HISTORY_LEN;
byte historyLength = 0;
void loop() {
  if (mainEventFlags & FLAG_INTERRUPT)
  {
    LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_ON);
    mainEventFlags &= ~FLAG_INTERRUPT;
    if (!digitalRead(BUTTON_PIN)) {
      buttonPressed=true;
    }
  }

  if (buttonPressed)
  {
    buttonPressed = false;
    Beep(10, false);

    //save non-ACK messages in a circular buffer
    if (!radio.ACK_RECEIVED && historyLength > 1) //only care if at least 2 messages saved. if only 1 message it should be displayed already
    {
      if (currMessageIndex==0)
        currMessageIndex=historyLength-1;
      else currMessageIndex--;
      
      //Serial.print("HIST currIndex/histLen=");Serial.print(currMessageIndex+1);Serial.print("/");Serial.print(historyLength);
      //Serial.print(" - ");
      //Serial.println(messageHistory[currMessageIndex].data);
      
      u8g.firstPage();
      do {
        draw(messageHistory[currMessageIndex].data, messageHistory[currMessageIndex].rssi, messageHistory[currMessageIndex].from, true);
      } while(u8g.nextPage());
      //delay(10); //give OLED time to draw?
    }
  }
  
  if (radio.receiveDone())
  {
    Serial.print('[');Serial.print(radio.SENDERID);Serial.print("] ");
    if (promiscuousMode)
      Serial.print("to [");Serial.print(radio.TARGETID);Serial.print("] ");

    Serial.print((char*)radio.DATA);
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
    Serial.println();
    
    saveToHistory((char *)radio.DATA, radio.RSSI, radio.SENDERID);
    Blink(LED,3);
    Beep(20, true);
    u8g.firstPage();
    do {
      draw((char*)radio.DATA, radio.RSSI, radio.SENDERID, false);
    } while(u8g.nextPage());
    //delay(10); //give OLED time to draw?
  }
  radio.receiveDone();
  Serial.flush();
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
}

float batteryVolts = 5;
char* BATstr="BAT:5.00v";
void draw(char * data, int rssi, byte from, boolean isHist) {
  char buff[20];
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_unifont);
  u8g.drawStr( 0, 10, data);
  sprintf(buff, "ID:%d", from);
  u8g.drawStr( 0, 25, buff);
  sprintf(buff, "RSSI:%d", rssi);
  u8g.drawStr( 60, 25, buff);

  if (!isHist)
  {
    batteryVolts = analogRead(A7) * 0.00322 * 1.42;
    dtostrf(batteryVolts, 3,2, BATstr);
    sprintf(buff, "BAT:%sv", BATstr);
    u8g.drawStr( 0, 55, buff);
  }
}

void Beep(byte theDelay, boolean both)
{
  if (theDelay > 20) theDelay = 20;
  tone(BUZZER, 4200); //4200
  delay(theDelay);
  noTone(BUZZER);
  LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_ON);
  if (both)
  {
    tone(BUZZER, 4500); //4500
    delay(theDelay);
    noTone(BUZZER);
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

void saveToHistory(char * msg, int rssi, byte from)
{
  byte length = strlen(msg);
  byte i = 0;
  if (lastMessageIndex >=9) lastMessageIndex = 0;
  else lastMessageIndex++;
  currMessageIndex = lastMessageIndex;
  if (historyLength < HISTORY_LEN) historyLength++;
  
  //Serial.print("HIST SAVE lastIndex=");Serial.print(lastMessageIndex);Serial.print(" strlen=");Serial.print(length);
  //Serial.print(" msg=[");
  
  for (; i<(MSG_MAX_LEN-1) && (i < length); i++)
  {
    messageHistory[lastMessageIndex].data[i] = msg[i];
    Serial.print(msg[i]);
  }
  //Serial.print("] copied:");
  messageHistory[lastMessageIndex].data[i] = '\0'; //terminate string
  //Serial.println((char*)messageHistory[lastMessageIndex].data);
    
  messageHistory[lastMessageIndex].rssi = rssi;
  messageHistory[lastMessageIndex].from = from;
}