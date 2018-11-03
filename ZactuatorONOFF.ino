/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

    Output pin High or Low

    Copyright: (c)Florian ROBERT

    Contributors:
    - 1technophile

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
#ifdef ZactuatorONOFF

void setupONOFF() {

  trc(F("ACTUATOR_ONOFF_PIN"));
  for (int i = 0; i < (sizeof(ACTUATOR_ONOFF_PINS) / sizeof(byte)); i++) {
    if ( i > 31 ) {
      break; // In case there are too many pins
    }
    trc(ACTUATOR_ONOFF_PINS[i]);

    //init
    pinMode(ACTUATOR_ONOFF_PINS[i], OUTPUT);
    trc(F("Set to OFF"));
    digitalWrite(ACTUATOR_ONOFF_PINS[i], LOW);

    timer_on_off[i].pin = ACTUATOR_ONOFF_PINS[i];
    timer_on_off[i].onOffdelay = 0;
    timer_on_off[i].milli = 0;
    timer_on_off[i].publishDelay = 0;
  }
  trc(sizeof(timer_on_off));
  trc(F("ZactuatorONOFF setup done "));
}

void MQTTtoONOFF(char * topicOri, char * datacallback) {

  String topic = topicOri;

  if ((topic == subjectMQTTtoONOFF)) {
    bool boolSWITCHTYPE = false;

    int pin = ACTUATOR_ONOFF_PINS[0];
    int command = 0;
    int publishDelay = 0;
    char publishMessage[] = "0";

    trc(datacallback);

    if (strchr(datacallback, MQTTtoONOFFDelimiter) != NULL) {
      pin = getValue(datacallback, MQTTtoONOFFDelimiter, 1).toInt();
      if ( !isValidPin(pin) ) {
        trc(F("Invalid pin given, please update config_ONOFF.h if you really want to use this pin"));
        trc(pin);
        return ;
      }
      command = getValue(datacallback, MQTTtoONOFFDelimiter, 0).toInt();

      // Sometimes you dont want the delay event to be published when it is completed, by default it wont unless it is set
      String over = getValue(datacallback, MQTTtoONOFFDelimiter, 2);
      if ( over != "" )
        publishDelay = over.toInt();

      trc(getValue(datacallback, MQTTtoONOFFDelimiter, 2));
    } else {
      command = atoi(datacallback);
    }

    if ( command > 0 ) {
      boolSWITCHTYPE = true;
      publishMessage[0] = {'1'};
    }

    timer_on_off_struct* timer = getStruct(pin);

    timer->milli = millis();
    timer->publishDelay = publishDelay;
    if ( command > 1 ) {
      timer->onOffdelay  = command;
    }  else {
      timer->onOffdelay  = 0;
    }
    trc(timer->pin);

    trc(F("MQTTtoONOFF"));
    trc(pin);
    trc(command);
    trc(boolSWITCHTYPE);
    digitalWrite(ACTUATOR_ONOFF_PIN, boolSWITCHTYPE);
    boolean result = client.publish(subjectGTWONOFFtoMQTT, publishMessage);// we acknowledge the sending by publishing the value to an acknowledgement topic with a 0 or 1, override can be set to return that value
    if (result) {
      trc(F("MQTTtoONOFF ack pub."));
      trc(publishMessage);
    }
    return ;

  }
  return;
}

void MQTTtoONOFFSwitch(int pin, bool isOn, int publishDelay) {
  char publishMessage[] = "0";

  if ( isOn )
    publishMessage[0] = {'1'};

  digitalWrite(pin, isOn);
  if ( publishDelay > 0 ) {
    trc(F("MQTTtoONOFF ack pub."));
    boolean result = client.publish(subjectGTWONOFFtoMQTT, publishMessage);
    if (result) {
      trc(F("MQTTtoONOFF delay ack pub."));
      trc(publishMessage);
    }
  }
}

bool isValidPin(int pin) {
  for (int i = 0; i < (sizeof(ACTUATOR_ONOFF_PINS) / sizeof(byte)); i++) {
    if ( ACTUATOR_ONOFF_PINS[i] == pin ) {
      return true;
    }
  }
  return false;
}

timer_on_off_struct* getStruct(int pin) {
  for (int i = 0; i < (sizeof(ACTUATOR_ONOFF_PINS) / sizeof(byte)); i++) {
    if ( timer_on_off[i].pin == pin ) {
      return &timer_on_off[i];
    }
  }
  timer_on_off[0];
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

#endif
