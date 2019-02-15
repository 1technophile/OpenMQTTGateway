/*
	Copyright (C) 2013 CurlyMo & Bram1337

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
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "selectremote.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	391
#define MAX_PULSE_LENGTH	401
#define AVG_PULSE_LENGTH	396
#define RAW_LENGTH				50

static int validate(void) {
	if(selectremote->rawlen == RAW_LENGTH) {
		if(selectremote->raw[selectremote->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   selectremote->raw[selectremote->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int state) {
	selectremote->message = json_mkobject();
	json_append_member(selectremote->message, "id", json_mknumber(id, 0));
	if(state == 1) {
		json_append_member(selectremote->message, "state", json_mkstring("on"));
	} else {
		json_append_member(selectremote->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int binary[RAW_LENGTH/4], x = 0, i = 0;

	if(selectremote->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "selectremote: parsecode - invalid parameter passed %d", selectremote->rawlen);
		return;
	}

	for(x=0;x<selectremote->rawlen-2;x+=4) {
		if(selectremote->raw[x] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	int id = 7-binToDec(binary, 1, 3);
	int state = binary[8];

	createMessage(id, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		selectremote->raw[i]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		selectremote->raw[i+1]=AVG_PULSE_LENGTH;
		selectremote->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		selectremote->raw[i+3]=AVG_PULSE_LENGTH;
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		selectremote->raw[i]=AVG_PULSE_LENGTH;
		selectremote->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		selectremote->raw[i+2]=AVG_PULSE_LENGTH;
		selectremote->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createHigh(0,47);
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	id = 7-id;
	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createLow(4+x, 4+x+3);
		}
	}
}

static void createState(int state) {
	if(state == 1) {
		createLow(32, 35);
	}
}

static void createFooter(void) {
	selectremote->raw[48]=(AVG_PULSE_LENGTH);
	selectremote->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int state = -1;
	double itmp = 0;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(id == -1 || state == -1) {
		logprintf(LOG_ERR, "selectremote: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 7 || id < 0) {
		logprintf(LOG_ERR, "selectremote: invalid id range");
		return EXIT_FAILURE;
	} else {
		createMessage(id, state);
		clearCode();
		createId(id);
		createState(state);
		createFooter();
		selectremote->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -i --id=systemcode\tcontrol a device with this id\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void selectremoteInit(void) {

	protocol_register(&selectremote);
	protocol_set_id(selectremote, "selectremote");
	protocol_device_add(selectremote, "selectremote", "SelectRemote Switches");
	selectremote->devtype = SWITCH;
	selectremote->hwtype = RF433;
	selectremote->minrawlen = RAW_LENGTH;
	selectremote->maxrawlen = RAW_LENGTH;
	selectremote->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	selectremote->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&selectremote->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[0-7]$");
	options_add(&selectremote->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&selectremote->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&selectremote->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&selectremote->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	selectremote->parseCode=&parseCode;
	selectremote->createCode=&createCode;
	selectremote->printHelp=&printHelp;
	selectremote->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "selectremote";
	module->version = "2.4";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	selectremoteInit();
}
#endif
