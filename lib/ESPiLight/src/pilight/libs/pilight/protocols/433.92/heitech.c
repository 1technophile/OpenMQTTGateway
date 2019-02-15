/*

	Copyright (C) 2015 CurlyMo & Meloen

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
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "../protocol.h"
#include "heitech.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	270
#define MAX_PULSE_LENGTH	290
#define AVG_PULSE_LENGTH	280
#define RAW_LENGTH				50

static int validate(void) {
	if(heitech->rawlen == RAW_LENGTH) {
		if(heitech->raw[heitech->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   heitech->raw[heitech->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int systemcode, int unitcode, int state) {
	heitech->message = json_mkobject();
	json_append_member(heitech->message, "systemcode", json_mknumber(systemcode, 0));
	json_append_member(heitech->message, "unitcode", json_mknumber(unitcode, 0));

	if(state == 0) {
		json_append_member(heitech->message, "state", json_mkstring("on"));
	} else {
		json_append_member(heitech->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int x = 0, binary[RAW_LENGTH/4];

	if(heitech->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "heitech: parsecode - invalid parameter passed %d", heitech->rawlen);
		return;
	}

	for(x=0;x<heitech->rawlen-2;x+=4) {
		if(heitech->raw[x+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x/4]=1;
		} else {
			binary[x/4]=0;
		}
	}

	int systemcode = binToDec(binary, 0, 4);
	int unitcode = binToDec(binary, 5, 9);
	int check = binary[10];
	int state = binary[11];

	if(check != state) {
		createMessage(systemcode, unitcode, state);
	}
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		heitech->raw[i]=AVG_PULSE_LENGTH;
		heitech->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		heitech->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		heitech->raw[i+3]=AVG_PULSE_LENGTH;
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		heitech->raw[i]=AVG_PULSE_LENGTH;
		heitech->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		heitech->raw[i+2]=AVG_PULSE_LENGTH;
		heitech->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createLow(0,47);
}

static void createSystemCode(int systemcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(systemcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createHigh(x, x+3);
		}
	}
}

static void createUnitCode(int unitcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unitcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createHigh(20+x, 20+x+3);
		}
	}
}

static void createState(int state) {
	if(state == 1) {
		createHigh(44, 47);
	} else {
		createHigh(40, 43);
	}
}

static void createFooter(void) {
	heitech->raw[48]=(AVG_PULSE_LENGTH);
	heitech->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}
static int createCode(JsonNode *code) {
	int systemcode = -1;
	int unitcode = -1;
	int state = -1;
	double itmp = 0;

	if(json_find_number(code, "systemcode", &itmp) == 0)
		systemcode = (int)round(itmp);
	if(json_find_number(code, "unitcode", &itmp) == 0)
		unitcode = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=1;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=0;

	if(systemcode == -1 || unitcode == -1 || state == -1) {
		logprintf(LOG_ERR, "heitech: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(systemcode > 31 || systemcode < 0) {
		logprintf(LOG_ERR, "heitech: invalid systemcode range");
		return EXIT_FAILURE;
	} else if(unitcode > 31 || unitcode < 0) {
		logprintf(LOG_ERR, "heitech: invalid unitcode range");
		return EXIT_FAILURE;
	} else {
		createMessage(systemcode, unitcode, state);
		clearCode();
		createSystemCode(systemcode);
		createUnitCode(unitcode);
		createState(state);
		createFooter();
		heitech->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -s --systemcode=systemcode\tcontrol a device with this systemcode\n");
	printf("\t -u --unitcode=unitcode\t\tcontrol a device with this unitcode\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void heitechInit(void) {

	protocol_register(&heitech);
	protocol_set_id(heitech, "heitech");
	protocol_device_add(heitech, "heitech", "Heitech series Switches");
	heitech->devtype = SWITCH;
	heitech->hwtype = RF433;
	heitech->minrawlen = RAW_LENGTH;
	heitech->maxrawlen = RAW_LENGTH;
	heitech->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	heitech->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&heitech->options, "s", "systemcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&heitech->options, "u", "unitcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&heitech->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&heitech->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&heitech->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	heitech->parseCode=&parseCode;
	heitech->createCode=&createCode;
	heitech->printHelp=&printHelp;
	heitech->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "heitech";
	module->version = "1.1";
	module->reqversion = "6.0";
	module->reqcommit = "187";
}

void init(void) {
	heitechInit();
}

#endif
