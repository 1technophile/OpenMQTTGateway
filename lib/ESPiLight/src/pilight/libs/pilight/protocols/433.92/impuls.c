/*
	Copyright (C) 2013 Bram1337 & CurlyMo

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
#include "impuls.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	130
#define MAX_PULSE_LENGTH	170
#define AVG_PULSE_LENGTH	150
#define RAW_LENGTH				50

static int validate(void) {
	if(impuls->rawlen == RAW_LENGTH) {
		if(impuls->raw[impuls->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   impuls->raw[impuls->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int systemcode, int programcode, int state) {
	impuls->message = json_mkobject();
	json_append_member(impuls->message, "systemcode", json_mknumber(systemcode, 0));
	json_append_member(impuls->message, "programcode", json_mknumber(programcode, 0));
	if(state == 1) {
		json_append_member(impuls->message, "state", json_mkstring("on"));
	} else {
		json_append_member(impuls->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int x = 0, binary[RAW_LENGTH/4];

	if(impuls->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "impuls: parsecode - invalid parameter passed %d", impuls->rawlen);
		return;
	}

	/* Convert the one's and zero's into binary */
	for(x=0;x<impuls->rawlen-2;x+=4) {
		if(impuls->raw[x+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2)) ||
		   impuls->raw[x+0] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x/4]=1;
		} else {
			binary[x/4]=0;
		}
	}

	int systemcode = binToDec(binary, 0, 4);
	int programcode = binToDec(binary, 5, 9);
	int check = binary[10];
	int state = binary[11];

	if(check != state) {
		createMessage(systemcode, programcode, state);
	}
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		impuls->raw[i]=AVG_PULSE_LENGTH;
		impuls->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		impuls->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		impuls->raw[i+3]=AVG_PULSE_LENGTH;
	}
}

static void createMed(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		impuls->raw[i]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		impuls->raw[i+1]=AVG_PULSE_LENGTH;
		impuls->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		impuls->raw[i+3]=AVG_PULSE_LENGTH;
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		impuls->raw[i]=AVG_PULSE_LENGTH;
		impuls->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		impuls->raw[i+2]=AVG_PULSE_LENGTH;
		impuls->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
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
			createMed(x, x+3);
		}
	}
}

static void createProgramCode(int programcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(programcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createHigh(20+x, 20+x+3);
		}
	}
}

static void createState(int state) {
	if(state == 0) {
		createHigh(40, 43);
	} else {
		createHigh(44, 47);
	}
}

static void createFooter(void) {
	impuls->raw[48]=(AVG_PULSE_LENGTH);
	impuls->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int systemcode = -1;
	int programcode = -1;
	int state = -1;
	double itmp = 0;

	if(json_find_number(code, "systemcode", &itmp) == 0)
		systemcode = (int)round(itmp);
	if(json_find_number(code, "programcode", &itmp) == 0)
		programcode = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(systemcode == -1 || programcode == -1 || state == -1) {
		logprintf(LOG_ERR, "impuls: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(systemcode > 31 || systemcode < 0) {
		logprintf(LOG_ERR, "impuls: invalid systemcode range");
		return EXIT_FAILURE;
	} else if(programcode > 31 || programcode < 0) {
		logprintf(LOG_ERR, "impuls: invalid programcode range");
		return EXIT_FAILURE;
	} else {
		createMessage(systemcode, programcode, state);
		clearCode();
		createSystemCode(systemcode);
		createProgramCode(programcode);
		createState(state);
		createFooter();
		impuls->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -s --systemcode=systemcode\tcontrol a device with this systemcode\n");
	printf("\t -u --programcode=programcode\tcontrol a device with this programcode\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void impulsInit(void) {

	protocol_register(&impuls);
	protocol_set_id(impuls, "impuls");
	protocol_device_add(impuls, "impuls", "Impuls Switches");
	impuls->devtype = SWITCH;
	impuls->hwtype = RF433;
	impuls->minrawlen = RAW_LENGTH;
	impuls->maxrawlen = RAW_LENGTH;
	impuls->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	impuls->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&impuls->options, "s", "systemcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&impuls->options, "u", "programcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&impuls->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&impuls->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&impuls->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&impuls->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	impuls->parseCode=&parseCode;
	impuls->createCode=&createCode;
	impuls->printHelp=&printHelp;
	impuls->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "impuls";
	module->version = "2.4";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	impulsInit();
}
#endif
