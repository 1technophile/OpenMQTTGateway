/*
	Copyright (C) 2014 CurlyMo

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "pilight_firmware_v2.h"

#define PULSE_MULTIPLIER	4
#define MIN_PULSE_LENGTH	175
#define MAX_PULSE_LENGTH	225
#define AVG_PULSE_LENGTH	183
#define RAW_LENGTH				196

static int validate(void) {
	if(pilight_firmware_v2->rawlen == RAW_LENGTH) {
		if(pilight_firmware_v2->raw[pilight_firmware_v2->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   pilight_firmware_v2->raw[pilight_firmware_v2->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV) &&
			 pilight_firmware_v2->raw[1] > AVG_PULSE_LENGTH*PULSE_MULTIPLIER) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int version, int high, int low) {
	pilight_firmware_v2->message = json_mkobject();
	json_append_member(pilight_firmware_v2->message, "version", json_mknumber(version, 2));
	json_append_member(pilight_firmware_v2->message, "lpf", json_mknumber(high*10, 0));
	json_append_member(pilight_firmware_v2->message, "hpf", json_mknumber(low*10, 0));
}

static void parseCode(void) {
	int i = 0, x = 0, binary[RAW_LENGTH/4];

	for(i=0;i<pilight_firmware_v2->rawlen;i+=4) {
		if(pilight_firmware_v2->raw[i+3] < 100) {
			pilight_firmware_v2->raw[i+3]*=10;
		}
		if(pilight_firmware_v2->raw[i+3] > AVG_PULSE_LENGTH*(PULSE_MULTIPLIER/2)) {
			binary[x++] = 1;
		} else {
			binary[x++] = 0;
		}
	}

	int version = binToDec(binary, 0, 15);
	int high = binToDec(binary, 16, 31);
	int low = binToDec(binary, 32, 47);
	createMessage(version, high, low);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void pilightFirmwareV2Init(void) {

  protocol_register(&pilight_firmware_v2);
  protocol_set_id(pilight_firmware_v2, "pilight_firmware");
  protocol_device_add(pilight_firmware_v2, "pilight_firmware", "pilight filter firmware");
  pilight_firmware_v2->devtype = FIRMWARE;
  pilight_firmware_v2->hwtype = HWINTERNAL;
	pilight_firmware_v2->minrawlen = RAW_LENGTH;
	pilight_firmware_v2->maxrawlen = RAW_LENGTH;
	pilight_firmware_v2->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	pilight_firmware_v2->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

  options_add(&pilight_firmware_v2->options, 'v', "version", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[0-9]+$");
  options_add(&pilight_firmware_v2->options, 'l', "lpf", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[0-9]+$");
  options_add(&pilight_firmware_v2->options, 'h', "hpf", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[0-9]+$");

  pilight_firmware_v2->parseCode=&parseCode;
  pilight_firmware_v2->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "pilight_firmware";
	module->version = "2.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	pilightFirmwareV2Init();
}
#endif
