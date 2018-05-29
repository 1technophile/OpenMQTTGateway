// Sample sketch for the SonarMote - Standalone parking assist with RGB LED indicator, piezo buzzer and OLED display
// This example uses the NewPing library from https://code.google.com/p/arduino-new-ping/
//      but that could be replaced by raw reading of the sonar sensor as seen in other SonarMote examples
// More info/photos at: http://lowpowerlab.com/sonar
// Ultrasonic sensor (HC-SR04) connected to D6 (Trig), D7 (Echo), and power enabled through D5
// This sketch sleeps the Moteino and sensor most of the time. It wakes up every few seconds to take
//   a distance reading. If it detects an approaching object (car) it increases the sampling rate
//   and starts lighting up the LED (from green to yellow to red to blinking red). Once there is no more
//   motion the LED is turned off and the cycle is back to a few seconds in between sensor reads.
// Button is connected on D3. Holding the button for a few seconds enters the "red zone adjust" mode (RZA).
//   By default the red zone limit is at 25cm (LED turns RED below this and starts blinking faster and faster).
//   In RZA, readings are taken for 5 seconds. In this time you have the chance to set a new red zone limit.
//   Valid new red zone readings are between the RED__LIMIT_UPPER (default 25cm) and MAX_ADJUST_DISTANCE (cm).
//   In RZA mode the BLU Led blinks fast to indicate new red limit distance. It blinks slow if the readings are invalid
//   If desired this value could be saved to EEPROM to persist if unit is turned off
// Make sure you adjust the settings in the configuration section below !!!

// **********************************************************************************
// Copyright Felix Rusu, LowPowerLab.com
// Library and code by Felix Rusu - felix@lowpowerlab.com
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
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <LowPower.h> //get library from: https://github.com/lowpowerlab/lowpower
                      //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/
//#define BUZZER_ENABLE  //uncomment this line if you have the BUZZER soldered and want the sketch to make sounds
//#define OLED_ENABLE    //uncomment this line if you have an OLED attached on the SonarMote and want to see the distance printed on it
                         //the OLED drawing will cause a visible delay in the LED blinking, nothing much to do about it without complicating the sketch a lot more

#ifdef OLED_ENABLE
  #include "U8glib.h"   //get library from: https://code.google.com/p/u8glib/
#endif

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
#else
  #define LED           9 // Moteinos have LEDs on D9
#endif

#define TRIG           6  // digital pin wired to TRIG pin of ultrasonic sensor
#define ECHO           7  // digital pin wired to ECHO pin of ultrasonic sensor
#define SENSOR_EN      5  // digital pin that enables power to ultrasonic sensor
#define RED           A0  // pin connected to red LED
#define GRN           A1  // pin connected to green LED
#define BLU           A2  // pin connected to blue LED
#define BUZZER         4
#define BUTTON_INT     1  // user button on interrupt 1 (D3)
#define BUTTON_PIN     3  // user button on interrupt 1 (D3)
#define BUTTON_HOLD_MS 3000  // hold button this many ms before entering red zone adjust
#define MAX_DISTANCE 220  // maximum valid distance
#define MIN_DISTANCE   2  // minimum valid distance

#define GRN_LIMIT_UPPER  180+redZoneAdjust  // upper limit distance for GREEN
#define YLW_LIMIT_UPPER   40+redZoneAdjust  // upper limit distance for YELLOW
#define RED_LIMIT_UPPER   25+redZoneAdjust  // upper limit distance for RED

#define MAX_ADJUST_DISTANCE (MAX_DISTANCE-GRN_LIMIT_UPPER)   //this is the amount by which the RED_LIMIT_UPPER can by increased

//possible states of LED status
#define STATE_SOLID 0
#define STATE_BLINK 1

//possible states for LED color
#define STATE_OFF 0
#define STATE_GRN 1
#define STATE_YLW 2
#define STATE_RED 3

byte state_LED = STATE_SOLID;
byte state_LEDCOLOR = STATE_OFF;
byte state_LEDONOFF = LOW;
byte redZoneAdjust = 0; //this is adjustable via the button (press button for a few seconds, then take a reading)

#define LED_RED {digitalWrite(RED,HIGH);digitalWrite(GRN,LOW);}
#define LED_GRN {digitalWrite(RED,LOW);digitalWrite(GRN,HIGH);}
#define LED_YLW {digitalWrite(RED,HIGH);digitalWrite(GRN,HIGH);}
#define LED_OFF {digitalWrite(RED,LOW);digitalWrite(GRN,LOW);}

#define SERIAL_EN         //uncomment if you want serial debugging output
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

#define SLEEP_MINILOOP SLEEP_15MS
#define SLEEP_LOOP SLEEP_250MS
#define SLEEP_LONG SLEEP_2S
#define SLEEP_LOBATT SLEEP_4S
#define SLEEP_HIBERNATE SLEEP_8S

#define BATT_MONITOR  A7  // Sense VBAT_COND signal (when powered externally should read ~3.25v/3.3v (1000-1023), when external power is cutoff it should start reading around 2.85v/3.3v * 1023 ~= 883 (ratio given by 10k+4.7K divider from VBAT_COND = 1.47 multiplier)
#define BATT_CYCLES   120  // read and report battery voltage every this many sleep cycles (ex 30cycles * 8sec sleep = 240sec/4min). For 450 cyclesyou would get ~1 hour intervals
#define BATT_FORMULA(reading) reading * 0.00322 * 1.475  // >>> fine tune this parameter to match your voltage when fully charged
#define BATT_LOW      3.35

#ifdef OLED_ENABLE
  U8GLIB_SSD1306_128X64 OLED(U8G_I2C_OPT_NONE);	// I2C / TWI SSD1306 OLED 128x64
#endif
void checkBattery(byte samples=10);    //take 10 samples by default
float batteryVolts = 5;
char buff[50];
  
void setup() {
#ifdef SERIAL_EN
  Serial.begin(SERIAL_BAUD); // Open serial monitor at 115200 baud to see ping results.
#endif
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(RED, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BLU, OUTPUT);
  pinMode(SENSOR_EN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(SENSOR_EN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_INT, buttonInterrupt, FALLING);
  
#ifdef OLED_ENABLE
  OLED.setRot180(); //flip screen
  // assign default color value
  if (OLED.getMode() == U8G_MODE_R3G3B2 )
    OLED.setColorIndex(255);     // white
  else if (OLED.getMode() == U8G_MODE_GRAY2BIT)
    OLED.setColorIndex(3);         // max intensity
  else if (OLED.getMode() == U8G_MODE_BW)
    OLED.setColorIndex(1);         // pixel on
  else if (OLED.getMode() == U8G_MODE_HICOLOR)
    OLED.setHiColorByRGB(255,255,255);
  OLED.begin();
  
  OLED.firstPage();
  OLED.setFont(u8g_font_unifont);
  do {
    OLED.drawStr(0, 10, "SonarMote");
  } while(OLED.nextPage());
#endif

#ifdef BUZZER_ENABLE
  buzzer(50,2,100);
#endif

  readDistance(); //first reading seems to always be low
}

#define FLAG_INTERRUPT 0x01
volatile int mainEventFlags = 0;
boolean buttonPressed = false;
void buttonInterrupt()
{
  mainEventFlags |= FLAG_INTERRUPT;
}

long distance=0;
long lastDistance=0;
long lastSigDistance=0;
byte loops=0;
byte miniLoops=0;
byte skipBlinkingLoops=5;
unsigned long now=0;
period_t sleepTime = SLEEP_LONG; //period_t is an enum type defined in the LowPower library (LowPower.h)

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
    detachInterrupt(BUTTON_INT);
    DEBUGln("BUTTON PRESS!");
    unsigned long timestamp = millis();
    while (millis() - timestamp < BUTTON_HOLD_MS)
    {
      if (digitalRead(BUTTON_PIN))
      {
        buttonPressed = false;
        break;
      }
      DEBUG('.');SERIALFLUSH();
      LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_ON);
      timestamp-=40;
    }
    
    //if it's still pressed after BUTTON_HOLD_MS then enter red zone adjust mode
    if (buttonPressed)
    {
      DEBUG("STILL_PRESSED");SERIALFLUSH();
      handleRedZoneAdjust();
    }
    else
    {
      DEBUG("ABORTED");SERIALFLUSH();
    }
    attachInterrupt(BUTTON_INT, buttonInterrupt, FALLING);
  }
  
  if (miniLoops>0) 
  {
    miniLoops--;               //miniloops starts at 
    sleepTime = SLEEP_MINILOOP;
    //EARLY SENSOR WAKEUP
    //When looping fast we need to wake the sensor about 60-75ms before doing a reading (about 5 miniloops assuming 1 miniloop=15ms)
    //otherwise there will be a visible delay in the LED blinking
    if (miniLoops == 6) digitalWrite(SENSOR_EN, HIGH);
    else if (miniLoops == 1) sacrificialPing();  //need 5 miniloops (75ms) between wakeup and dummy reading and another 15ms (1 miniloop) to real reading
                                                 //reading will happen when miniLoops==0
  }
  else if (loops > 0)
  {
    loops--;
    miniLoops=16; //16 "miniloops" form 1 "loop" (~240ms)
    sleepTime = SLEEP_MINILOOP;
  }
  else
    //sleep longer when no significant state changes happened
    //if battery is low, sleep even longer to try to squeeze more life
    sleepTime = (batteryVolts > BATT_LOW ? SLEEP_LONG : SLEEP_LOBATT);
  
  if ((loops == 0 && miniLoops == 0) || ((miniLoops % skipBlinkingLoops) == 0))
  {
    DEBUG('*');
    handleLEDState();
  }
  
  SERIALFLUSH(); //flush any characters in the serial buffer before sleeping otherwise they get lost or garbled
  LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF); //put microcontroller to sleep to save battery life
  if (miniLoops > 0) { DEBUG('.');return; } //as long as we still have miniloops we skip readings
                                            //only proceed to a reading every loop
  now = millis();
  distance = readDistance();
  byte d = millis()-now;
#ifdef OLED_ENABLE
  draw(distance);
#endif
    
  DEBUG("Read: ");
  DEBUG(distance); // Convert ping time to distance in cm and print result (0 = outside set distance range)
  DEBUG("cm");
  DEBUG("  [");
  DEBUG(d);
  DEBUGln("]ms");

  if (distance > MAX_DISTANCE || distance < MIN_DISTANCE)
  {
    DEBUGln("Out of range");
    loops=0;
    lastDistance = distance;
    return;
  }

  if (distance < GRN_LIMIT_UPPER && distance > YLW_LIMIT_UPPER && abs(lastSigDistance-distance)>20)
  {
    if (distance < lastSigDistance)
      loops=12; //begin looping fast only when object is approaching
    lastSigDistance = distance;
  }
  else if (distance < YLW_LIMIT_UPPER && abs(lastSigDistance-distance)>5)
  {
    if (distance < lastSigDistance)
      loops=12; //begin looping fast only when object is approaching
    lastSigDistance = distance;
  }
  
  //if the looping was started, determine the state we're in
  if (loops > 0)
  {
    if (distance >= YLW_LIMIT_UPPER) { state_LEDCOLOR = STATE_GRN; state_LED = STATE_SOLID; }
    else if (distance >= RED_LIMIT_UPPER) { state_LEDCOLOR=STATE_YLW; state_LED = STATE_SOLID; }
    else { state_LEDCOLOR=STATE_RED; state_LED = STATE_BLINK; }
  }
  else { state_LEDCOLOR=STATE_OFF; state_LED = STATE_SOLID; }

  //adjust the blinking rate based on the distance to the object
  if (state_LEDCOLOR==STATE_RED)
  {
    if (distance > RED_LIMIT_UPPER-2)
      skipBlinkingLoops = 8;
    else if (distance > RED_LIMIT_UPPER-6)
      skipBlinkingLoops = 6;
    else if (lastDistance > RED_LIMIT_UPPER-10)
      skipBlinkingLoops = 4;
    else if (lastDistance > RED_LIMIT_UPPER-14)
      skipBlinkingLoops = 2;
    else
    {
      skipBlinkingLoops = 1;
      state_LED = STATE_SOLID;
    }
  }
  else skipBlinkingLoops = 1;
  lastDistance = distance; //remember the last reading
  checkBattery();
  if (batteryVolts < BATT_LOW)
    Blink(BLU);
  else Blink(LED);
  DEBUG("Batt: ");
  DEBUG(batteryVolts);
  DEBUGln("v");
}

float readDistance()
{
  //To save battery we need to sleep the HC-SR04 sonar sensor in between readings.
  //However to get valid readings it requires special waking up and delay
  //It needs to be powered up for ~75ms, then a dummy reading has to be made which is typically bogus
  //Then another 15ms needs to pass before doing the real reading
  //Because the main loop sleeps between readings and we want to avoid visible delays in the LED blinking
  // we need to wake up the sensor in the main loop, look for "EARLY SENSOR WAKEUP".
  // When looping fast in the main loop (aka minilooping, to allow fast LED blinking)
  //  each "miniloop" is about 15ms of sleep time. So we should wake up the sensor 5 miniloops before we do the reading
  //  and do the sacrificial dummy reading 1 miniloop before the real reading. See the "EARLY SENSOR WAKEUP" code in the main loop.

  //when not looping fast in the main loop, just do the special sensor wakeup here
  //first enable sensor power and sleep MCU while sensor settles (needs)75ms
  if (loops == 0 && miniLoops == 0)
  {
    digitalWrite(SENSOR_EN, HIGH);
    //need about 60-75ms after power up before HC-SR04 will be usable, so just sleep in the meantime
    LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
    // do a dummy reading first and wait another 15ms for the real reading
    sacrificialPing();
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
  }
  
  // Now do the real reading
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:  
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIG, LOW);
  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  long duration = pulseIn(ECHO, HIGH);
  digitalWrite(SENSOR_EN, LOW);
  return microsecondsToCentimeters(duration);
}

void sacrificialPing()
{
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIG, LOW);
  pulseIn(ECHO, HIGH);
}

//handles the status and color of the LED depending what state we are in
void handleLEDState()
{
  switch(state_LEDCOLOR)
  {
    case STATE_OFF: LED_OFF; break;    
    case STATE_GRN: LED_GRN; break;
    case STATE_YLW: LED_YLW; break;
    case STATE_RED:
      if (state_LED == STATE_BLINK)
      {
        if (state_LEDONOFF == HIGH)
        {
          LED_OFF;
          state_LEDONOFF = LOW;
        }
        else 
        {
          LED_RED;
          state_LEDONOFF = HIGH;
#ifdef BUZZER_ENABLE
          buzzer(skipBlinkingLoops,0,0);
#endif
        }
      }
      else LED_RED;
      break;
  }
}

void Blink(byte pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(2);
  digitalWrite(pin, LOW);
}

byte cycleCount=BATT_CYCLES;
void checkBattery(byte samples)
{
  if (cycleCount++ == BATT_CYCLES) //only read battery every BATT_CYCLES sleep cycles
  {
    unsigned int readings=0;
    for (byte i=0; i<samples; i++) //take 10 samples, and average
      readings+=analogRead(BATT_MONITOR);
    batteryVolts = BATT_FORMULA((float)readings / samples);
    cycleCount = 0;
  }
}

//enter red zone adjust mode
void handleRedZoneAdjust()
{
  DEBUGln("\nRED_ZONE_ADJUST");SERIALFLUSH();
  LED_OFF; //turn off all other LEDs
  
  unsigned long startTimestamp=millis();
  float distance;
  int adjustTime = 5000;
  byte state = HIGH;
  
  while (millis()-startTimestamp < adjustTime)
  {
    digitalWrite(BLU, state);
    state = state ? LOW : HIGH; //flip state for next loop
    distance = readDistance();
    if (distance > MAX_ADJUST_DISTANCE + RED_LIMIT_UPPER-redZoneAdjust)
      delay(300);
    else if (distance <= RED_LIMIT_UPPER-redZoneAdjust)
      state = HIGH; //keep LED on
    else 
      delay (distance);
    
    DEBUG(distance);DEBUGln("cm");SERIALFLUSH();
    Blink(LED);
  }

  digitalWrite(BLU, LOW); //turn LED off
  if (distance > RED_LIMIT_UPPER-redZoneAdjust && distance <= MAX_ADJUST_DISTANCE + RED_LIMIT_UPPER-redZoneAdjust)
  {
    redZoneAdjust = distance - RED_LIMIT_UPPER - redZoneAdjust;
    DEBUG("New RED_ZONE_SHIFT = "); DEBUGln(redZoneAdjust);SERIALFLUSH();
  }
}

float microsecondsToInches(long microseconds)
{
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74.0 / 2.0f;
}

float microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return (float)microseconds / 29.0f / 2.0f;
}

#ifdef OLED_ENABLE
void draw(byte distance) {
  OLED.firstPage();
  OLED.setFont(u8g_font_unifont);
  sprintf(buff, "Reading: %dcm", distance);

  do {
    OLED.drawStr(0, 10, buff);
  } while(OLED.nextPage());
}
#endif

#ifdef BUZZER_ENABLE
void buzzer(byte soundTime, byte repeats, byte repeatsDelay)
{
  for (byte i=0;i<=repeats;i++)
  {
    tone(BUZZER, 4500); //4500hz makes a nice audible sound from a 3.3v Moteino digital pin
    delay(soundTime);
    noTone(BUZZER);
    if (repeats>0) delay(repeatsDelay);
  }
}
#endif
