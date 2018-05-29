/*
 INA226 Bi-directional Current/Power Monitor. Alert with latch Example.
 Read more: http://www.jarzebski.pl/arduino/czujniki-i-sensory/cyfrowy-czujnik-pradu-mocy-ina226.html
 GIT: https://github.com/jarzebski/Arduino-INA226
 Web: http://www.jarzebski.pl
 (c) 2014 by Korneliusz Jarzebski
 */

#include <Wire.h>
#include <INA226.h>

INA226 ina;

void setup() 
{
  Serial.begin(115200);

  Serial.println("Initialize INA226");
  Serial.println("-----------------------------------------------");

  // Default INA226 address is 0x40
  ina.begin();

  // Configure INA226
  ina.configure(INA226_AVERAGES_1, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);

  // Calibrate INA226. Rshunt = 0.01 ohm, Max excepted current = 4A
  ina.calibrate(0.01, 4);

  // Enable Power Over-Limit Alert
  ina.enableOverPowerLimitAlert();
  ina.setPowerLimit(0.130);
  ina.setAlertLatch(true);
}

void loop()
{
  Serial.print("Bus voltage:   ");
  Serial.print(ina.readBusVoltage(), 5);
  Serial.println(" V");

  Serial.print("Bus power:     ");
  Serial.print(ina.readBusPower(), 5);
  Serial.println(" W");


  Serial.print("Shunt voltage: ");
  Serial.print(ina.readShuntVoltage(), 5);
  Serial.println(" V");

  Serial.print("Shunt current: ");
  Serial.print(ina.readShuntCurrent(), 5);
  Serial.println(" A");

  if (ina.isAlert())
  {
    // Latch will be removed here
    Serial.println("ALERT");
  }

  Serial.println("");
  delay(1000);
}

