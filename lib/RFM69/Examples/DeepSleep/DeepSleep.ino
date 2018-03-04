//***********************************************************************************************************
// Sample sketch that achieves the lowest power on a Moteino of ~6.5uA
// Everything is put to sleep including the MCU, the radio (if any) and the FlashMem chip
//**** SETTINGS *********************************************************************************************
#define WITH_RFM69              //comment this line out if you don't have a RFM69 on your Moteino
#define WITH_SPIFLASH           //comment this line out if you don't have the FLASH-MEM chip on your Moteino
//***********************************************************************************************************

#include <Arduino.h>            // assumes Arduino IDE v1.0 or greater
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

#if defined(WITH_RFM69) || defined(WITH_SPIFLASH)
  #include <SPI.h>                //comes with Arduino IDE (www.arduino.cc)
  #if defined(WITH_RFM69)
    #include <RFM69.h>            //get it here: https://www.github.com/lowpowerlab/rfm69
    RFM69 radio;
    #define NETWORKID 100
    #define NODEID 123
    #define FREQUENCY RF69_915MHZ
  #endif
  #if defined(WITH_SPIFLASH)
    #include <SPIFlash.h>         //get it here: https://www.github.com/lowpowerlab/spiflash
    SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
  #endif
#endif

//watchdog interrupt
ISR (WDT_vect) {
  wdt_disable();
}

void setup () {
#ifdef WITH_RFM69
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.sleep();
#endif

#ifdef WITH_SPIFLASH
  if (flash.initialize())
    flash.sleep();
#endif

//  //optional blink to know radio/flash sleeping went OK
//  pinMode(LED, OUTPUT);
//  digitalWrite(LED, HIGH);
//  delay(30);
//  digitalWrite(LED, LOW);
//  delay(50);
//  digitalWrite(LED, HIGH);
//  delay(50);
//  digitalWrite(LED, LOW);

  for (uint8_t i=0; i<=A5; i++)
  {
#ifdef WITH_RFM69
    if (i == RF69_SPI_CS) continue;
#endif
#ifdef WITH_SPIFLASH
    if (i == FLASH_SS) continue;
#endif
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();
}

void loop () 
{
  // disable ADC
  ADCSRA = 0;  
  // clear various "reset" flags
  MCUSR = 0;
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  //set interrupt mode and an interval
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0); //set WDIE, and 8 seconds delay
  wdt_reset(); //pat the dog...
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  noInterrupts(); // timed sequence follows  
  sleep_enable();

  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit (BODS); 
  interrupts();
  sleep_cpu();

  //cancel sleep as a precaution
  sleep_disable();
}
