/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the ADC value
  
    Copyright: (c)Florian ROBERT
  
    This file is part of OpenMQTTGateway.
    
    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef config_ADC_h
#define config_ADC_h

extern void setupADC();
extern void ADCtoMQTT();
extern void MeasureADC();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define ADCTOPIC "/ADCtoMQTT"

#if !defined(TimeBetweenReadingADC) || (TimeBetweenReadingADC < 200)
#  define TimeBetweenReadingADC 500 // time between 2 ADC readings, minimum 200 to let the time of the ESP to keep the connection
#endif

#ifndef ThresholdReadingADC
#  define ThresholdReadingADC 50 // following the comparison between the previous value and the current one +- the threshold the value will be published or not
#endif

#if !defined(NumberOfReadingsADC) || (NumberOfReadingsADC < 1)
#  define NumberOfReadingsADC 1 // number of readings for better accuracy: avg adc = sum of adc / num readings
#endif

#ifndef MinTimeInSecBetweenPublishingADC
#  define MinTimeInSecBetweenPublishingADC 0 // pub at least at defined interval - useful to publish values in case they do not change so much ; 0 = disabled
#endif

/*-------------------PIN DEFINITIONS----------------------*/
#if defined(ESP8266) || !defined(ADC_GPIO)
#  define ADC_GPIO A0 //on nodeMCU this is D3 pin
#endif

#endif
