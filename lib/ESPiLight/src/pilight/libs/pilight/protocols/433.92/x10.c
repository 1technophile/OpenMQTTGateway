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
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "x10.h"

#define PULSE_MULTIPLIER	12
#define MIN_PULSE_LENGTH	145
#define MAX_PULSE_LENGTH	155
#define AVG_PULSE_LENGTH	150
#define RAW_LENGTH				68

static char letters[18] = {"MNOPCDABEFGHKL IJ"};

static int validate(void) {
	if(x10->rawlen == RAW_LENGTH) {
		if(x10->raw[x10->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   x10->raw[x10->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(char *id, int state) {
	x10->message = json_mkobject();
	json_append_member(x10->message, "id", json_mkstring(id));
	if(state == 0) {
		json_append_member(x10->message, "state", json_mkstring("on"));
	} else {
		json_append_member(x10->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int x = 0, y = 0, binary[RAW_LENGTH/2];

	if(x10->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "x10: parsecode - invalid parameter passed %d", x10->rawlen);
		return;
	}

	for(x=1;x<x10->rawlen-1;x+=2) {
		if(x10->raw[x] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[y++] = 1;
		} else {
			binary[y++] = 0;
		}
	}

	char id[3];
	int l = letters[binToDecRev(binary, 0, 3)];
	int s = binary[18];
	int i = 1;
	int c1 = (binToDec(binary, 0, 7)+binToDec(binary, 8, 15));
	int c2 = (binToDec(binary, 16, 23)+binToDec(binary, 24, 31));
	if(binary[5] == 1) {
		i += 8;
	}
	if(binary[17] == 1) {
		i += 4;
	}
	i += binToDec(binary, 19, 20);
	if(c1 == 255 && c2 == 255) {
		sprintf(id, "%c%d", l, i);
		createMessage(id, s);
	}
}

static void createLow(int s, int e) {
	int i;
	for(i=s;i<=e;i+=2) {
		x10->raw[i]=(AVG_PULSE_LENGTH*(int)round(PULSE_MULTIPLIER/3));
		x10->raw[i+1]=(AVG_PULSE_LENGTH*(int)round(PULSE_MULTIPLIER/3));
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		x10->raw[i]=(AVG_PULSE_LENGTH*(int)round(PULSE_MULTIPLIER/3));
		x10->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void x10clearCode(void) {
	createLow(0, 15);
	createHigh(16, 31);
	createLow(32, 47);
	createHigh(48, 63);
}

static void createLetter(int l) {
	int binary[255];
	int length = 0;
	int i=0, x=0, y = 0;

	for(i=0;i<17;i++) {
		if((int)letters[i] == l) {
			length = decToBinRev(i, binary);
			for(x=0;x<=length;x++) {
				if(binary[x]==1) {
					y=x*2;
					createHigh(7-(y+1),7-y);
					createLow(23-(y+1),23-y);
				}
			}
			break;
		}
	}
}

static void createNumber(int n) {
	if(n > 8) {
		createHigh(10, 10);
		createLow(26, 26);
		n -= 8;
	}
	if(n > 4) {
		createHigh(34, 34);
		createLow(50, 50);
		n -= 4;
	}
	if(n == 2 || n == 4) {
		createHigh(38, 38);
		createLow(54, 54);
	}
	if(n == 3 || n == 4) {
		createHigh(40, 40);
		createLow(56, 56);
	}
}

static void createState(int state) {
// CMA17 protocol: Off: state=1 and Byte 1-Bit 5 logical High. Also set complement bit.
	if(state == 1) {
		createHigh(36, 36);
		createLow(52, 52);
	}
}

static void createFooter(void) {
	x10->raw[64]=(AVG_PULSE_LENGTH*(int)round(PULSE_MULTIPLIER/3));
	x10->raw[65]=(PULSE_DIV*AVG_PULSE_LENGTH*9);
	x10->raw[66]=(PULSE_DIV*AVG_PULSE_LENGTH*2);
	x10->raw[67]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	char id[4] = {'\0'};
	int state = -1;
	double itmp = -1;
	char *stmp = NULL;

	strcpy(id, "-1");

	if(json_find_string(code, "id", &stmp) == 0)
		strcpy(id, stmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=1;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=0;

	if(strcmp(id, "-1") == 0 || state == -1) {
		logprintf(LOG_ERR, "x10: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if((int)(id[0]) < 65 || (int)(id[0]) > 80) {
		logprintf(LOG_ERR, "x10: invalid id range");
		return EXIT_FAILURE;
	} else if(atoi(&id[1]) < 0 || atoi(&id[1]) > 16) {
		logprintf(LOG_ERR, "x10: invalid id range");
		return EXIT_FAILURE;
	} else {
		createMessage(id, state);
		x10clearCode();
		createLetter((int)id[0]);
		createNumber(atoi(&id[1]));
		createState(state);
		createFooter();
		x10->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void x10Init(void) {
	protocol_register(&x10);
	protocol_set_id(x10, "x10");
	protocol_device_add(x10, "x10", "x10 based devices");
	x10->devtype = SWITCH;
	x10->hwtype = RF433;
	x10->minrawlen = RAW_LENGTH;
	x10->maxrawlen = RAW_LENGTH;
	x10->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	x10->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&x10->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&x10->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&x10->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[ABCDEFGHIJKLMNOP]([1][0-6]{1}|[1-9]{1})$");

	options_add(&x10->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&x10->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	x10->parseCode=&parseCode;
	x10->createCode=&createCode;
	x10->printHelp=&printHelp;
	x10->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "x10";
	module->version = "2.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	x10Init();
}
#endif
