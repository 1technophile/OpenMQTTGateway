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

#include "aprintf.h"
#include <Esp.h>

static Print *aprintf_print = nullptr;

void set_aprintf_output(Print *output) { aprintf_print = output; }

int aprintf_P(PGM_P formatP, ...) {
  if (aprintf_print == nullptr) {
    return 0;
  }
  va_list arg;
  va_start(arg, formatP);
  char temp[64];
  char *buffer = temp;
  size_t len = vsnprintf_P(temp, sizeof(temp), formatP, arg);
  va_end(arg);
  if (len > sizeof(temp) - 1) {
    buffer = new char[len + 1];
    if (!buffer) {
      return 0;
    }
    va_start(arg, formatP);
    vsnprintf_P(buffer, len + 1, formatP, arg);
    va_end(arg);
  }
  len = aprintf_print->write((const uint8_t *)buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  return len;
}

void exit(int n) {
  if (aprintf_print != nullptr) {
    aprintf_print->print(F("EXIT: "));
    aprintf_print->println(n);
  }
  ESP.restart();
  while (true)
    ;
}
