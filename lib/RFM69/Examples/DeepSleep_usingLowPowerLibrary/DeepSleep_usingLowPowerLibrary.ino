//***********************************************************************************************************
// Sample sketch that achieves the lowest power on a Moteino of ~6.5uA
// Everything is put to sleep including the MCU, the radio (if any) and the FlashMem chip
//**** SETTINGS *********************************************************************************************
#define WITH_RFM69              //comment this line out if you don't have a RFM69 on your Moteino
#define WITH_SPIFLASH           //comment this line out if you don't have the FLASH-MEM chip on your Moteino
//***********************************************************************************************************

#include <LowPower.h> //get library from: https://github.com/lowpowerlab/lowpower
                      //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

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
  #endif
  #if defined(WITH_SPIFLASH)
    #include <SPIFlash.h>         //get it here: https://www.github.com/lowpowerlab/spiflash
    SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
  #endif
#endif

void setup () {
#ifdef WITH_RFM69
  radio.sleep();
#endif

#ifdef WITH_SPIFLASH
  if (flash.initialize())
    flash.sleep();
#endif

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
}

void loop () 
{
  //optional blink to know radio/flash sleeping went OK
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(30);
  digitalWrite(LED, LOW);

  //sleep MCU for 8seconds
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}
