// *************************************************************************************************************
//                                          MightyBoost control sample sketch
// *************************************************************************************************************
// Copyright (2015) Felix Rusu of http://lowpowerlab.com
// http://lowpowerlab.com/mightyboost
// MightyBoost is a smart backup PSU controllable by Moteino, and this sketch is a sample control sketch to run
// MightyBoost in this mode.
// Be sure to check back for code updates and patches
// *************************************************************************************************************
// This sketch will provide control over the essential features of MightyBoost:
//   - provide switched 5V power to a sensitive load like RaspberryPi which should not lose power instantly
//   - Control the "5V*" output via Moteino+PowerButton (momentary tactile)
//   - Monitor input supply and switch to battery backup when external power is lost
//   - Monitor battery voltage and issue a shutdown/reboot signal when battery runs low
// This sketch may be extended to include integration with other LowPowerLab automation products
// *************************************************************************************************************
// License
// *************************************************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 2 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE.  See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program; if not, write 
// to the Free Software Foundation, Inc.,                
// 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//                                                        
// Licence can be viewed at                               
// http://www.fsf.org/licenses/gpl.txt                    
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// *************************************************************************************************************
#define LED                 5     // LED pin, should be analog for fading effect (PWM)
#define BUTTON              3     // Power button pin
#define SIG_SHUTOFF         6     // Signal to Pi to ask for a shutdown
#define SIG_BOOTOK         A0     // Signal from Pi that it's OK to cutoff power
                                  // !!NOTE!! Originally this was D7 but it was moved to A0 at least temporarily.
                                  // On MightyBoost R1 you need to connect D7 and A0 with a jumper wire.
                                  // The explanation for this is given here: http://lowpowerlab.com/mightyboost/#source
#define OUTPUT_5V           4     // HIGH on this pin will switch the "5V*" output ON
#define BATTERYSENSE       A7     // Sense VBAT_COND signal (when powered externally should read ~3.25v/3.3v (1000-1023), when external power is cutoff it should start reading around 2.85v/3.3v * 1023 ~= 880 (ratio given by 10k+4.7K divider from VBAT_COND = 1.47 multiplier)
                                  // hence the actual input voltage = analogRead(A7) * 0.00322 (3.3v/1024) * 1.47 (10k+4.7k voltage divider ratio)
                                  // when plugged in this should be 4.80v, nothing to worry about
                                  // when on battery power this should decrease from 4.15v (fully charged Lipoly) to 3.3v (discharged Lipoly)
                                  // trigger a shutdown to the target device once voltage is around 3.4v to allow 30sec safe shutdown
#define LOWBATTERYTHRESHOLD  3.5  // a shutdown will be triggered to the target device when battery voltage drops below this (Volts)

#define RESETHOLDTIME         500 // Button must be hold this many mseconds before a reset is issued (should be much less than SHUTDOWNHOLDTIME)
#define SHUTDOWNHOLDTIME     2000 // Button must be hold this many mseconds before a shutdown sequence is started (should be much less than ForcedShutoffDelay)
#define ShutoffTriggerDelay  6000 // will start checking the SIG_BOOTOK line after this long
#define RecycleTime         50000 // window of time in which SIG_BOOTOK is expected to go HIGH
                                  // should be at least 3000 more than Min
                                  // if nothing happens after this window, if button is 
                                  // still pressed, force cutoff power, otherwise switch back to normal ON state
#define RESETPULSETIME        500 // When reset is issued, the SHUTOFF signal is held HIGH this many ms
#define ForcedShutoffDelay   7500 // when SIG_BOOTOK==0 (PI in unknown state): if button is held
                                  // for this long, force shutdown (this should be less than RecycleTime)
#define ShutdownFinalDelay   4500 // after shutdown signal is received, delay for this long
                                  // to allow all PI LEDs to stop activity (pulse LED faster)

#define PRINTPERIOD              10000

int lastValidReading = 1;
unsigned long lastValidReadingTime = 0;
unsigned long NOW=0;
int PowerState = 0;
long lastPeriod = -1;
float systemVoltage = 5;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(SIG_BOOTOK, INPUT);
  pinMode(SIG_SHUTOFF, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(OUTPUT_5V, OUTPUT);
  pinMode(A7, INPUT);
  digitalWrite(SIG_SHUTOFF, LOW);//added after sudden shutdown quirks, DO NOT REMOVE!
  digitalWrite(OUTPUT_5V, LOW);//added after sudden shutdown quirks, DO NOT REMOVE!
}

void loop() {
  int reading = digitalRead(BUTTON);
  NOW = millis();
  digitalWrite(SIG_SHUTOFF, LOW);//added after sudden shutdown quirks, DO NOT REMOVE!
  boolean batteryLow = systemVoltage < LOWBATTERYTHRESHOLD;

  if (batteryLow || reading != lastValidReading && NOW - lastValidReadingTime > 200)
  {
    lastValidReading = reading;
    lastValidReadingTime = NOW;
    
    if (batteryLow || reading == 0)
    {
      //make sure the button is held down for at least 'RESETHOLDTIME' before taking action (this is to avoid accidental button presses and consequently Pi shutdowns)
      NOW = millis();
      while (!batteryLow && (PowerState == 1 && millis()-NOW < RESETHOLDTIME)) { delay(10); if (digitalRead(BUTTON) != 0) return; }

      //RESETHOLDTIME is satisfied, now check if button still held until SHUTDOWNHOLDTIME is satisfied
      analogWrite(LED, 128); //dim the LED to show something's going on
      while (!batteryLow && (PowerState == 1 && millis()-NOW < SHUTDOWNHOLDTIME))
      {
        if (digitalRead(BUTTON) != 0)
        {
          if (BOOTOK())       //SIG_BOOTOK is HIGH so Pi is running the shutdowncheck.sh script, ready to intercept the RESET PULSE
          {
            digitalWrite(SIG_SHUTOFF, HIGH);
            delay(RESETPULSETIME);
            digitalWrite(SIG_SHUTOFF, LOW);

            NOW = millis();
            boolean recycleDetected=false;
            while (millis()-NOW < RecycleTime) //blink LED while waiting for BOOTOK to go high
            {
              //blink 3 times and pause
              digitalWrite(LED, LOW);
              delay(100);
              digitalWrite(LED, HIGH);
              delay(100);
              digitalWrite(LED, LOW);
              delay(100);
              digitalWrite(LED, HIGH);
              delay(100);
              digitalWrite(LED, LOW);
              delay(100);
              digitalWrite(LED, HIGH);
              delay(500);

              if (!BOOTOK()) recycleDetected = true;
              else if (BOOTOK() && recycleDetected)
                return;
            }
            return; //reboot pulse sent but it appears a reboot failed; exit all checks
          }
          else return; //ignore everything else (button was held for RESETHOLDTIME, but SIG_BOOTOK was LOW)
        }
      }
      
      //SIG_BOOTOK must be HIGH when Pi is ON. During boot, this will take a while to happen (till it executes the "shutdowncheck" script)
      //so I dont want to cutoff power before it had a chance to fully boot up
      if (batteryLow || (PowerState == 1 && BOOTOK()))
      {
        // signal Pi to shutdown
        digitalWrite(SIG_SHUTOFF, HIGH);

        //now wait for the Pi to signal back
        NOW = millis();
        float in, out;
        boolean forceShutdown = true;
        
        while (millis()-NOW < RecycleTime)
        {
          if (in > 6.283) in = 0;
          in += .00628;
          
          out = sin(in) * 127.5 + 127.5;
          analogWrite(LED,out);
          delayMicroseconds(1500);
          
          //account for force-shutdown action (if button held for ForcedShutoffDelay, then force shutdown regardless)
          if (millis()-NOW <= (ForcedShutoffDelay-SHUTDOWNHOLDTIME) && digitalRead(BUTTON) != 0)
            forceShutdown = false;
          if (millis()-NOW >= (ForcedShutoffDelay-SHUTDOWNHOLDTIME) && forceShutdown)
          {
            PowerState = 0;
            digitalWrite(LED, PowerState); //turn off LED to indicate power is being cutoff
            digitalWrite(OUTPUT_5V, PowerState);
            break;
          }
          
          if (millis() - NOW > ShutoffTriggerDelay)
          {
            // Pi signaling OK to turn off
            if (!BOOTOK())
            {
              PowerState = 0;
              digitalWrite(LED, PowerState); //turn off LED to indicate power is being cutoff
              NOW = millis();
              while (millis()-NOW < ShutdownFinalDelay)
              {
                if (in > 6.283) in = 0;
                in += .00628;
                
                out = sin(in) * 127.5 + 127.5;
                analogWrite(LED,out);
                delayMicroseconds(300);
              }
              
              digitalWrite(OUTPUT_5V, PowerState);
              break;
            }
          }
        }
        
        // last chance: if power still on but button still pressed, force cutoff power
        if (PowerState == 1 && digitalRead(BUTTON) == 0)
        {
          PowerState = 0;
          digitalWrite(OUTPUT_5V, PowerState);
        }
        
        digitalWrite(SIG_SHUTOFF, LOW);
      }
      else if (PowerState == 1 && !BOOTOK())
      {
        NOW = millis();
        unsigned long NOW2 = millis();
        int analogstep = 255 / ((ForcedShutoffDelay-SHUTDOWNHOLDTIME)/100); //every 500ms decrease LED intensity
        while (digitalRead(BUTTON) == 0)
        {
          if (millis()-NOW2 > 100)
          {
            analogWrite(LED, 255 - ((millis()-NOW)/100)*analogstep);
            NOW2 = millis();
          }
          if (millis()-NOW > ForcedShutoffDelay-SHUTDOWNHOLDTIME)
          {
            //TODO: add blinking here to signal final shutdown delay
            PowerState = 0;
            digitalWrite(OUTPUT_5V, PowerState);
            break;
          }
        }
      }
      else if (PowerState == 0)
      {
        PowerState = 1;
        digitalWrite(OUTPUT_5V, PowerState); //digitalWrite(LED, PowerState);
      }
    }

    digitalWrite(LED, PowerState);
  }

  int currPeriod = millis()/PRINTPERIOD;
  if (currPeriod != lastPeriod)
  {
    lastPeriod=currPeriod;
    Serial.print("VIN: ");
    systemVoltage = analogRead(BATTERYSENSE) * 0.00322 * 1.47;
    Serial.print(systemVoltage);
    if (systemVoltage > 4.3)
      Serial.println("  (plugged in)");
    else Serial.println("  (running from battery!)");
  }
}

boolean BOOTOK() {
  return analogRead(SIG_BOOTOK) > 800;
}