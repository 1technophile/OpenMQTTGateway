/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    GPIO KeyCode derived from  GPIO Input

    This reads up to 4 gpio with latch signal (gpio line) and combines the bits as KeyKode - can be connected
    to EV1527 receiver.

    Copyright: (c)Grzegorz Rajtar
    
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
#ifdef ZsensorGPIOKeyCode

int InputStateGPIOKeyCode = 0x0f;             // Set to 3 so that it reads on startup
int lastInputStateGPIOKeyCode = 0x0f; 
int lastLatchStateGPIOKeyCode = 0;

void setupGPIOKeyCode() {
  pinMode(GPIOKeyCode_LATCH_PIN, INPUT_PULLUP);  //
  pinMode(GPIOKeyCode_D0_PIN, INPUT_PULLUP);     //
  pinMode(GPIOKeyCode_D1_PIN, INPUT_PULLUP);     //
  pinMode(GPIOKeyCode_D2_PIN, INPUT_PULLUP);     //
  //pinMode(GPIOKeyCode_D3_PIN, INPUT_PULLUP);     //  
}

void MeasureGPIOKeyCode(){

  int latch = digitalRead(GPIOKeyCode_LATCH_PIN);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
  #if defined(ESP8266) || defined(ESP32) 
    yield();
  #endif
    // if the Input state has changed:
    if (latch > 0 && lastLatchStateGPIOKeyCode != latch) {
      int reading = digitalRead(GPIOKeyCode_D0_PIN)
            | (digitalRead(GPIOKeyCode_D1_PIN) << 1) 
            | (digitalRead(GPIOKeyCode_D2_PIN) << 2);
            //| digitalRead(GPIOKeyCode_D3_PIN) << 3;

      char hex[3];

      InputStateGPIOKeyCode = reading;
      sprintf(hex, "%02x", InputStateGPIOKeyCode);
      hex[2] = 0;
      Serial.printf("GPIOKeyCode %s\n", hex);
      client.publish(subjectGPIOKeyCodetoMQTT,hex);
      lastLatchStateGPIOKeyCode = latch;
    }
   
    if (latch != lastLatchStateGPIOKeyCode) {
      lastLatchStateGPIOKeyCode = latch;
       Serial.printf("GPIOKeyCode latch %d\n", latch);
      if (latch == 0)
          client.publish(subjectGPIOKeyCodeStatetoMQTT, "done");
    }

  // save the reading. Next time through the loop, it'll be the lastInputState:
  lastInputStateGPIOKeyCode = InputStateGPIOKeyCode;
  }
}
#endif
