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

#ifndef _APRINTF_H_
#define _APRINTF_H_

#include <pgmspace.h>

#ifndef __cplusplus
#include <stdio.h>
#define fprintf(stream, fmt, ...) aprintf_P(PSTR(fmt), ##__VA_ARGS__)
#define printf(fmt, ...) aprintf_P(PSTR(fmt), ##__VA_ARGS__)
#endif

#ifdef __cplusplus
#include <Print.h>
void set_aprintf_output(Print *output);
#endif

#ifdef __cplusplus
extern "C" {
#endif
void exit(int n);
int aprintf_P(PGM_P formatP, ...) __attribute__((format(printf, 1, 2)));
#ifdef __cplusplus
}
#endif

#endif  //_APRINTF_H_
