/*
	Copyright (C) 2017 Oleksii Serdiuk

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

#include "livolo_switch.h"

#include "../../core/binary.h"
#include "../../core/log.h"

#if defined(MODULE) && !defined(_WIN32)
#	include "../../core/dso.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Based on the protocol description from
// http://platenspeler.github.io/DeveloperGuide/433Transmitters/433messaging.html
// with timing adjustments, based on sniffing the remote I own

#define REPEATS		120
#define PULSE_LENGTH	170
#define MIN_RAW_LENGTH	24
#define MAX_RAW_LENGTH	47

#define ID_BITS		16
#define UNIT_BITS	7

static int createHeader() {
	livolo_switch->raw[0] = PULSE_LENGTH*3.5;
	return 1;
}

static int createLow(int i) {
	livolo_switch->raw[i] = PULSE_LENGTH;
	livolo_switch->raw[i+1] = PULSE_LENGTH;
	return 2;
}

static int createHigh(int i) {
	livolo_switch->raw[i] = PULSE_LENGTH*2;
	return 1;
}

static int createBits(int value, int start, int length) {
	int binary[255];
	int l = 0;
	int i = 0, x = start;

	l = decToBin(value, binary);
	// Pad with zeroes
	for(i=0;i<length-1-l;i++) {
		x += createLow(x);
	};
	for(i=0;i<=l;i++) {
		if(binary[i] == 1) {
			x += createHigh(x);
		} else {
			x += createLow(x);
		}
	}
	return x;
}

static int createKeycode(int key) {
	switch(key) {
	case 0:
		// "Off" key
		return 106;
	case 1:
		return 0;
	case 2:
		return 96;
	case 3:
		return 120;
	case 4:
		return 24;
	case 5:
		return 80;
	case 6:
		return 48;
	case 7:
		return 108;
	case 8:
		return 12;
	case 9:
		return 72;
	case 10:
		return 40;
	default:
		return -1;
	}
}

static void createMessage(int id, int key, int off) {
	livolo_switch->message = json_mkobject();

	json_append_member(livolo_switch->message, "id", json_mknumber(id, 0));

	if(off == 1) {
		json_append_member(livolo_switch->message, "off", json_mknumber(off, 0));
	} else {
		json_append_member(livolo_switch->message, "key", json_mknumber(key, 0));
	}
}

static int createCode(JsonNode *code) {
	int id = -1;
	int key = -1;
	int off = 0;
	int x = 0;
	double itmp = -1;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "key", &itmp) == 0)
		key = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0) {
		off = 1;
		key = 0;
	}

	if(off > 0 && key > 0) {
		logprintf(LOG_ERR, "livolo_switch: off and key cannot be combined");
		return EXIT_FAILURE;
	} else if(id == -1 || (key == -1 && off == 0)) {
		logprintf(LOG_ERR, "livolo_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id < 0 || id > 65535) {
		logprintf(LOG_ERR, "livolo_switch: invalid id range");
		return EXIT_FAILURE;
	} else if((key < 1 || key > 10) && off == 0) {
		logprintf(LOG_ERR, "livolo_switch: invalid key range");
		return EXIT_FAILURE;
	} else {
		createMessage(id, key, off);

		x = createHeader();
		x = createBits(id, x, ID_BITS);
		x = createBits(createKeycode(key), x, UNIT_BITS);
		livolo_switch->rawlen = x;
	}

	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -i --id=id\t\t\tsend with id of the remote (0..65535)\n");
	printf("\t -n --key=number\t\t\tsend with key number on the remote (1..10)\n");
	printf("\t -f --off\t\t\tsend turn off code for the remote\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void livoloSwitchInit(void) {
	protocol_register(&livolo_switch);
	protocol_set_id(livolo_switch, "livolo_switch");
	protocol_device_add(livolo_switch, "livolo_switch", "Livolo Switches");
	livolo_switch->devtype = SWITCH;
	livolo_switch->hwtype = RF433;
	livolo_switch->txrpt = REPEATS;
	livolo_switch->minrawlen = MIN_RAW_LENGTH;
	livolo_switch->maxrawlen = MAX_RAW_LENGTH;

	options_add(&livolo_switch->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL,
	            // 16 bits: 0..65535
	            "^([0-5]?[0-9]{1,4}|(6[0-4][0-9]{3})|(65[0-4][0-9]{2})|(655[0-2][0-9])|(6553[0-5]))$");
	options_add(&livolo_switch->options, 'n', "key", OPTION_OPT_VALUE, DEVICES_ID, JSON_NUMBER, NULL,
	            // 7 its: 0..127
	            "^(([0-9]{1,2})|(1[0-1][0-9])|(12[0-7]))$");
	options_add(&livolo_switch->options, 'f', "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&livolo_switch->options, 0, "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, NULL, "^[01]$");
	options_add(&livolo_switch->options, 0, "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, NULL, "^[01]$");

	livolo_switch->createCode=&createCode;
	livolo_switch->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "livolo_switch";
	module->version = "0.1";
	module->reqversion = "7.0";
	module->reqcommit = NULL;
}

void init(void) {
	livoloSwitchInit();
}
#endif
