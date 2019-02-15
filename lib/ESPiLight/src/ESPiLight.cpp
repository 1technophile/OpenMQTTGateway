/*
  ESPiLight - pilight 433.92 MHz protocols library for Arduino
  Copyright (c) 2016 Puuu.  All right reserved.

  Project home: https://github.com/puuu/espilight/
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with library. If not, see <http://www.gnu.org/licenses/>
*/

#include <ESPiLight.h>
#include "tools/aprintf.h"

// ESP32 doesn't define ICACHE_RAM_ATTR
#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR IRAM_ATTR
#endif

#ifdef DEBUG
#define Debug(x) Serial.print(x)
#define DebugLn(x) Serial.println(x)
#else
#define Debug(x)
#define DebugLn(x)
#endif

extern "C" {
#include "pilight/libs/pilight/protocols/protocol.h"
}
static protocols_t *used_protocols = nullptr;

volatile PulseTrain_t ESPiLight::_pulseTrains[RECEIVER_BUFFER_SIZE];
bool ESPiLight::_enabledReceiver;
volatile uint8_t ESPiLight::_actualPulseTrain = 0;
uint8_t ESPiLight::_avaiablePulseTrain = 0;
volatile unsigned long ESPiLight::_lastChange =
    0;  // Timestamp of previous edge
volatile uint8_t ESPiLight::_nrpulses = 0;
int16_t ESPiLight::_interrupt = NOT_AN_INTERRUPT;

uint8_t ESPiLight::minrawlen = 5;
uint8_t ESPiLight::maxrawlen = MAXPULSESTREAMLENGTH;
uint16_t ESPiLight::mingaplen = 5100;
uint16_t ESPiLight::maxgaplen = 10000;

static void fire_callback(protocol_t *protocol, ESPiLightCallBack callback);

static protocols_t *get_protocols() {
  if (pilight_protocols == nullptr) {
    ESPiLight::setErrorOutput(Serial);
    protocol_init();
  }
  return pilight_protocols;
}

static protocols_t *get_used_protocols() {
  if (used_protocols == nullptr) {
    used_protocols = get_protocols();
  }
  return used_protocols;
}

static protocols_t *find_protocol_node(const char *name) {
  protocols_t *pnode = get_protocols();
  while (pnode != nullptr) {
    if (strcmp(name, pnode->listener->id) == 0) {
      return pnode;
    }
    pnode = pnode->next;
  }
  return nullptr;
}

static protocol_t *find_protocol(const char *name) {
  protocols_t *pnode = find_protocol_node(name);
  if (pnode != nullptr) {
    return pnode->listener;
  }
  return nullptr;
}

static int create_pulse_train(uint16_t *pulses, protocol_t *protocol,
                              const String &content) {
  Debug("piLightCreatePulseTrain: ");

  if (!json_validate(content.c_str())) {
    Debug("invalid json: ");
    DebugLn(content);
    return ESPiLight::ERROR_INVALID_JSON;
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
  if ((protocol != nullptr) && (protocol->createCode != nullptr) &&
      (protocol->maxrawlen <= MAXPULSESTREAMLENGTH)) {
#pragma GCC diagnostic pop
    Debug("protocol: ");
    Debug(protocol->id);

    protocol->rawlen = 0;
    protocol->raw = pulses;
    JsonNode *message = json_decode(content.c_str());
    int return_value = protocol->createCode(message);
    json_delete(message);
    // delete message created by createCode()
    json_delete(protocol->message);
    protocol->message = nullptr;

    if (return_value == EXIT_SUCCESS) {
      DebugLn(" create Code succeded.");
      return protocol->rawlen;
    } else {
      DebugLn(" create Code failed.");
      return ESPiLight::ERROR_INVALID_PILIGHT_MSG;
    }
  }
  return ESPiLight::ERROR_UNAVAILABLE_PROTOCOL;
}

void ESPiLight::initReceiver(byte inputPin) {
  int16_t interrupt = digitalPinToInterrupt(inputPin);
  if (_interrupt == interrupt) {
    return;
  }
  if (_interrupt >= 0) {
    detachInterrupt((uint8_t)_interrupt);
  }
  _interrupt = interrupt;

  resetReceiver();
  enableReceiver();

  if (interrupt >= 0) {
    attachInterrupt((uint8_t)interrupt, interruptHandler, CHANGE);
  }
}

uint8_t ESPiLight::receivePulseTrain(uint16_t *pulses) {
  uint8_t length = nextPulseTrainLength();

  if (length > 0) {
    volatile PulseTrain_t &pulseTrain = _pulseTrains[_avaiablePulseTrain];
    _avaiablePulseTrain = (_avaiablePulseTrain + 1) % RECEIVER_BUFFER_SIZE;
    for (uint8_t i = 0; i < length; i++) {
      pulses[i] = pulseTrain.pulses[i];
    }
    pulseTrain.length = 0;
  }
  return length;
}

uint8_t ESPiLight::nextPulseTrainLength() {
  return _pulseTrains[_avaiablePulseTrain].length;
}

void ICACHE_RAM_ATTR ESPiLight::interruptHandler() {
  if (!_enabledReceiver) {
    return;
  }

  unsigned long now = micros();
  unsigned int duration = 0;

  volatile PulseTrain_t &pulseTrain = _pulseTrains[_actualPulseTrain];
  volatile uint16_t *codes = pulseTrain.pulses;

  if (pulseTrain.length == 0) {
    duration = now - _lastChange;
    /* We first do some filtering (same as pilight BPF) */
    if (duration > MIN_PULSELENGTH) {
      if (duration < MAX_PULSELENGTH) {
        /* All codes are buffered */
        codes[_nrpulses] = (uint16_t)duration;
        _nrpulses = (uint8_t)((_nrpulses + 1) % MAXPULSESTREAMLENGTH);
        /* Let's match footers */
        if (duration > mingaplen) {
          // Debug('g');
          /* Only match minimal length pulse streams */
          if (_nrpulses >= minrawlen && _nrpulses <= maxrawlen) {
            // Debug(_nrpulses);
            // Debug('l');
            pulseTrain.length = _nrpulses;
            _actualPulseTrain = (_actualPulseTrain + 1) % RECEIVER_BUFFER_SIZE;
          }
          _nrpulses = 0;
        }
      }
      _lastChange = now;
    }
  } else {
    Debug("_!_");
  }
}

void ESPiLight::resetReceiver() {
  for (unsigned int i = 0; i < RECEIVER_BUFFER_SIZE; i++) {
    _pulseTrains[i].length = 0;
  }
  _avaiablePulseTrain = 0;
  _actualPulseTrain = 0;
  _nrpulses = 0;
}

void ESPiLight::enableReceiver() { _enabledReceiver = true; }

void ESPiLight::disableReceiver() { _enabledReceiver = false; }

void ESPiLight::loop() {
  int length = 0;
  uint16_t pulses[MAXPULSESTREAMLENGTH];

  length = receivePulseTrain(pulses);

  if (length > 0) {
    /*
    Debug("RAW (");
    Debug(length);
    Debug("): ");
    for(int i=0;i<length;i++) {
      Debug(pulses[i]);
      Debug(' ');
    }
    DebugLn();
    */
    parsePulseTrain(pulses, (uint8_t)length);
  }
}

ESPiLight::ESPiLight(int8_t outputPin) {
  _outputPin = outputPin;
  _callback = nullptr;
  _rawCallback = nullptr;
  _echoEnabled = false;

  if (_outputPin >= 0) {
    pinMode((uint8_t)_outputPin, OUTPUT);
    digitalWrite((uint8_t)_outputPin, LOW);
  }

  get_protocols();
}

void ESPiLight::setCallback(ESPiLightCallBack callback) {
  _callback = callback;
}

void ESPiLight::setPulseTrainCallBack(PulseTrainCallBack rawCallback) {
  _rawCallback = rawCallback;
}

void ESPiLight::sendPulseTrain(const uint16_t *pulses, size_t length,
                               size_t repeats) {
  if (_outputPin >= 0) {
    bool receiverState = _enabledReceiver;
    _enabledReceiver = (_echoEnabled && receiverState);
    for (unsigned int r = 0; r < repeats; r++) {
      for (unsigned int i = 0; i < length; i += 2) {
        digitalWrite((uint8_t)_outputPin, HIGH);
        delayMicroseconds(pulses[i]);
        digitalWrite((uint8_t)_outputPin, LOW);
        if (i + 1 < length) {
          delayMicroseconds(pulses[i + 1]);
        }
      }
    }
    digitalWrite((uint8_t)_outputPin, LOW);
    _enabledReceiver = receiverState;
  }
}

int ESPiLight::send(const String &protocol, const String &json,
                    size_t repeats) {
  if (_outputPin < 0) {
    DebugLn("No output pin set, cannot send");
    return ERROR_NO_OUTPUT_PIN;
  }
  int length = 0;
  uint16_t pulses[MAXPULSESTREAMLENGTH];

  protocol_t *protocol_listener = find_protocol(protocol.c_str());
  length = create_pulse_train(pulses, protocol_listener, json);
  if (length > 0) {
    /*
    DebugLn();
    Debug("send: ");
    Debug(length);
    Debug(" pulses (");
    Debug(protocol);
    Debug(", ");
    Debug(content);
    DebugLn(")");
    */
    if (repeats == 0) {
      repeats = protocol_listener->txrpt;
    }
    sendPulseTrain(pulses, (unsigned)length, repeats);
  }
  return length;
}

int ESPiLight::createPulseTrain(uint16_t *pulses, const String &protocol_id,
                                const String &content) {
  protocol_t *protocol = find_protocol(protocol_id.c_str());
  return create_pulse_train(pulses, protocol, content);
}

size_t ESPiLight::parsePulseTrain(uint16_t *pulses, uint8_t length) {
  size_t matches = 0;
  protocol_t *protocol = nullptr;
  protocols_t *pnode = get_used_protocols();

  // DebugLn("piLightParsePulseTrain start");
  while ((pnode != nullptr) && (_callback != nullptr)) {
    protocol = pnode->listener;

    if (protocol->parseCode != nullptr && protocol->validate != nullptr) {
      protocol->raw = pulses;
      protocol->rawlen = length;

      if (protocol->validate() == 0) {
        Debug("pulses: ");
        Debug(length);
        Debug(" possible protocol: ");
        DebugLn(protocol->id);

        if (protocol->first > 0) {
          protocol->first = protocol->second;
        }
        protocol->second = micros();
        if (protocol->first == 0) {
          protocol->first = protocol->second;
        }

        /* Reset # of repeats after a certain delay */
        if ((protocol->second - protocol->first) > 500000) {
          protocol->repeats = 0;
        }

        protocol->message = nullptr;
        protocol->parseCode();
        if (protocol->message != nullptr) {
          matches++;
          protocol->repeats++;

          fire_callback(protocol, _callback);

          json_delete(protocol->message);
          protocol->message = nullptr;
        }
      }
    }
    pnode = pnode->next;
  }
  if (_rawCallback != nullptr) {
    (_rawCallback)(pulses, length);
  }

  // Debug("piLightParsePulseTrain end. matches: ");
  // DebugLn(matches);
  return matches;
}

static void fire_callback(protocol_t *protocol, ESPiLightCallBack callback) {
  PilightRepeatStatus_t status = FIRST;
  char *content = json_encode(protocol->message);
  String deviceId = "";
  double itmp;
  char *stmp;

  if ((protocol->repeats <= 1) || (protocol->old_content == nullptr)) {
    status = FIRST;
    json_free(protocol->old_content);
    protocol->old_content = content;
  } else if (!(protocol->repeats & 0x80)) {
    if (strcmp(content, protocol->old_content) == 0) {
      protocol->repeats |= 0x80;
      status = VALID;
    } else {
      status = INVALID;
    }
    json_free(protocol->old_content);
    protocol->old_content = content;
  } else {
    status = KNOWN;
    json_free(content);
  }
  if (json_find_number(protocol->message, "id", &itmp) == 0) {
    deviceId = String((int)round(itmp));
  } else if (json_find_string(protocol->message, "id", &stmp) == 0) {
    deviceId = String(stmp);
  };
  (callback)(String(protocol->id), String(protocol->old_content), status,
             protocol->repeats & 0x7F, deviceId);
}

String ESPiLight::pulseTrainToString(const uint16_t *codes, size_t length) {
  bool match = false;
  int diff = 0;

  uint8_t nrpulses = 0;  // number of pulse types
  uint16_t plstypes[MAX_PULSE_TYPES] = {};

  String data("");
  data.reserve(6 + length);
  data += "c:";
  for (unsigned int i = 0; i < length; i++) {
    match = false;
    for (uint8_t j = 0; j < MAX_PULSE_TYPES; j++) {
      // We device these numbers by 10 to normalize them a bit
      diff = (plstypes[j] / 50) - (codes[i] / 50);
      if ((diff >= -2) && (diff <= 2)) {
        // Write numbers
        data += (char)('0' + ((char)j));
        match = true;
        break;
      }
    }
    if (!match) {
      plstypes[nrpulses++] = codes[i];
      data += (char)('0' + ((char)(nrpulses - 1)));
      if (nrpulses >= MAX_PULSE_TYPES) {
        DebugLn("too many pulse types");
        return String("");
      }
    }
  }
  data += ";p:";
  for (uint8_t i = 0; i < nrpulses; i++) {
    data += plstypes[i];
    if (i + 1 < nrpulses) {
      data += ',';
    }
  }
  data += '@';
  return data;
}

int ESPiLight::stringToPulseTrain(const String &data, uint16_t *codes,
                                  size_t maxlength) {
  unsigned int length = 0;    // length of pulse train
  unsigned int nrpulses = 0;  // number of pulse types
  uint16_t plstypes[MAX_PULSE_TYPES] = {};

  // validate data string
  int scode = data.indexOf('c') + 2;
  if (scode < 0 || (unsigned)scode > data.length()) {
    DebugLn("'c' not found in data string, or has no data");
    return ERROR_INVALID_PULSETRAIN_MSG_C;
  }
  int spulse = data.indexOf('p') + 2;
  if (spulse < 0 || (unsigned)spulse > data.length()) {
    DebugLn("'p' not found in data string, or has no data");
    return ERROR_INVALID_PULSETRAIN_MSG_P;
  }
  // parsing pulse types
  unsigned int start = (unsigned)spulse;
  int end = data.indexOf(',', start);
  while (end > 0) {
    plstypes[nrpulses++] =
        (uint16_t)data.substring(start, (unsigned)end).toInt();
    start = (unsigned)end + 1;
    end = data.indexOf(',', start);
  }
  end = data.indexOf(';', start);
  if (end < 0) {
    end = data.indexOf('@', start);
  }
  if (end < 0) {
    DebugLn("';' or '@' not found in data string");
    return ERROR_INVALID_PULSETRAIN_MSG_END;
  }
  plstypes[nrpulses++] = (uint16_t)data.substring(start, (unsigned)end).toInt();
  // parsing pulses
  int pulse_index = 0;
  for (unsigned int i = (unsigned)scode; i < data.length(); i++) {
    if ((data[i] == ';') || (data[i] == '@')) break;
    if (i >= maxlength) break;
    pulse_index = data[i] - '0';
    if ((pulse_index < 0) || ((unsigned)pulse_index >= nrpulses)) {
      DebugLn("Pulse type not defined");
      return ERROR_INVALID_PULSETRAIN_MSG_TYPE;
    }
    codes[length++] = plstypes[pulse_index];
  }
  return length;
}

void ESPiLight::limitProtocols(const String &protos) {
  if (!json_validate(protos.c_str())) {
    DebugLn("Protocol limit argument is not a valid json message!");
    return;
  }
  JsonNode *message = json_decode(protos.c_str());

  if (message->tag != JSON_ARRAY) {
    DebugLn("Protocol limit argument is not a json array!");
    json_delete(message);
    return;
  }

  if (get_used_protocols() != get_protocols()) {
    protocols_t *pnode = get_used_protocols();
    while (pnode != nullptr) {
      protocols_t *tmp = pnode;
      pnode = pnode->next;
      delete tmp;
    }
  }

  used_protocols = nullptr;
  JsonNode *curr = message->children.head;
  unsigned int proto_count = 0;

  while (curr != nullptr) {
    if (curr->tag != JSON_STRING) {
      DebugLn("Element is not a String");
      curr = curr->next;
      continue;
    }

    protocols_t *templ = find_protocol_node(curr->string_);
    if (templ == nullptr) {
      Debug("Protocol not found: ");
      DebugLn(curr->string_);
      curr = curr->next;
      continue;
    }

    protocols_t *new_node = new protocols_t;
    new_node->listener = templ->listener;
    new_node->next = used_protocols;
    used_protocols = new_node;

    Debug("activated protocol ");
    DebugLn(templ->listener->id);
    proto_count++;

    if (curr == message->children.tail) {
      break;
    }
    curr = curr->next;
  }

  json_delete(message);
}

static String protocols_to_array(protocols_t *pnode) {
  protocols_t *tmp = pnode;
  size_t needed_len = 2;  // []
  while (tmp != nullptr) {
    needed_len += strlen(tmp->listener->id) + 3;  // "xx",
    tmp = tmp->next;
  }

  String ret;
  ret.reserve(needed_len);

  ret += '[';

  bool first = true;
  while (pnode != nullptr) {
    if (first) {
      first = false;
    } else {
      ret += ",";
    }
    ret += '"';
    ret += pnode->listener->id;
    ret += '"';
    pnode = pnode->next;
  }
  ret += "]";

  return ret;
}

String ESPiLight::availableProtocols() {
  return protocols_to_array(get_protocols());
}

String ESPiLight::enabledProtocols() {
  return protocols_to_array(get_used_protocols());
}

void ESPiLight::setEchoEnabled(bool enabled) { _echoEnabled = enabled; }

void ESPiLight::setErrorOutput(Print &output) { set_aprintf_output(&output); }
