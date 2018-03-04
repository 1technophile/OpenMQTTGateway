// Sample sketch for the SonarMote - Simple distance reading
// http://lowpowerlab.com/sonar
// Ultrasonic sensor (HC-SR04) connected to D6 (Trig), D7 (Echo), and power enabled through D5
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
#define TRIG 6
#define ECHO 7
#define EN   5

void setup() {
  pinMode (TRIG,OUTPUT);//attach pin 2 to vcc
  pinMode (ECHO,OUTPUT);//attach pin 2 to vcc
  pinMode (EN,OUTPUT);//attach pin 5 to GND
  pinMode (ECHO, INPUT);//attach pin 4 to Echo
  // initialize serial communication:
  Serial.begin(115200);
  digitalWrite(EN, LOW);
}

long cm;
void loop()
{
  cm = readDistanceCM();
  Serial.print(cm); Serial.println("cm");
  delay(500);
}

long readDistanceCM()
{
  digitalWrite(EN, HIGH);
  delay(75);
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIG, LOW);
  pulseIn(ECHO, HIGH);
  delay(16);
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIG, LOW);
  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  long duration = pulseIn(ECHO, HIGH);
  digitalWrite(EN, LOW);
  return microsecondsToCentimeters(duration);
}

long microsecondsToInches(long microseconds)
{
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}
