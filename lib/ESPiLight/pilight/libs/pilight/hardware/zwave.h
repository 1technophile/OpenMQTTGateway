/*
	Copyright (C) 2013 - 2014 CurlyMo

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#ifndef _HARDWARE_ZWAVE_H_
#define _HARDWARE_ZWAVE_H_

#include "../config/hardware.h"

#define COMMAND_CLASS_SWITCH_BINARY 		0x25
#define COMMAND_CLASS_SWITCH_MULTILEVEL 0x26
#define COMMAND_CLASS_SWITCH_ALL 				0x27
#define COMMAND_CLASS_CONFIGURATION			0x70
#define COMMAND_CLASS_POWERLEVEL 				0x73

typedef struct zwave_values_t {
	int nodeId;
	int cmdId;
	int valueId;
	union {
		int number_;
		char *string_;
	};
	int valueType;
	int instance;
	int genre;
	char *label;
	struct zwave_values_t *next;
} zwave_values_t;

struct zwave_values_t *zwave_values;

struct hardware_t *zwave;
void zwaveInit(void);
unsigned long long zwaveGetValueId(int nodeId, int cmdId);
void zwaveSetValue(int nodeId, int cmdId, char *label, char *value);
void zwaveStartInclusion(void);
void zwaveStartExclusion(void);
void zwaveStopCommand(void);
void zwaveSoftReset(void);
void zwaveGetConfigParam(int nodeId, int paramId);
void zwaveSetConfigParam(int nodeId, int paramId, int valueId, int len);

#endif
