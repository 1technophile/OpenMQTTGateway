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
#include "conrad_rsl_switch.h"

#define LEARN_REPEATS		40
#define NORMAL_REPEATS		10
#define PULSE_MULTIPLIER	6
#define MIN_PULSE_LENGTH	190
#define MAX_PULSE_LENGTH	210
#define AVG_PULSE_LENGTH	200
#define RAW_LENGTH				66

static int codes[5][4][2];

static int validate(void) {
	if(conrad_rsl_switch->rawlen == RAW_LENGTH) {
		if(conrad_rsl_switch->raw[conrad_rsl_switch->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   conrad_rsl_switch->raw[conrad_rsl_switch->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state, int learn) {
	conrad_rsl_switch->message = json_mkobject();

	if(id == 4) {
		json_append_member(conrad_rsl_switch->message, "all", json_mknumber(1, 0));
	} else {
		json_append_member(conrad_rsl_switch->message, "id", json_mknumber(id+1, 0));
	}
	json_append_member(conrad_rsl_switch->message, "unit", json_mknumber(unit+1, 0));
	if(state == 1) {
		json_append_member(conrad_rsl_switch->message, "state", json_mkstring("on"));
	} else {
		json_append_member(conrad_rsl_switch->message, "state", json_mkstring("off"));
	}
	if(learn == 1) {
		conrad_rsl_switch->txrpt = LEARN_REPEATS;
	} else {
		conrad_rsl_switch->txrpt = NORMAL_REPEATS;
	}
}

static void parseCode(void) {
	int x = 0, binary[RAW_LENGTH/2];
	int id = 0, unit = 0, state = 0;

	if(conrad_rsl_switch->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "conrad_rsl_switch: parsecode - invalid parameter passed %d", conrad_rsl_switch->rawlen);
		return;
	}

	/* Convert the one's and zero's into binary */
	for(x=0;x<conrad_rsl_switch->rawlen;x+=2) {
		if(conrad_rsl_switch->raw[x+1] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x/2]=0;
		} else {
			binary[x/2]=1;
		}
	}

	int check = binToDecRev(binary, 0, 7);
	int match = 0;
	for(id=0;id<5;id++) {
		for(unit=0;unit<4;unit++) {
			if(codes[id][unit][0] == check) {
				state = 0;
				match = 1;
			}
			if(codes[id][unit][1] == check) {
				state = 1;
				match = 1;
			}
			if(match) {
				break;
			}
		}
		if(match) {
			break;
		}
	}
	createMessage(id, unit, state, 0);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		conrad_rsl_switch->raw[i]=((PULSE_MULTIPLIER+1)*AVG_PULSE_LENGTH);
		conrad_rsl_switch->raw[i+1]=AVG_PULSE_LENGTH*3;
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		conrad_rsl_switch->raw[i]=AVG_PULSE_LENGTH*3;
		conrad_rsl_switch->raw[i+1]=((PULSE_MULTIPLIER+1)*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	int i = 0, x = 0;
	createHigh(0,65);
	// for(i=0;i<65;i+=2) {
		// x=i*2;
		// createHigh(x,x+1);
	// }

	int binary[255];
	int length = 0;

	length = decToBinRev(23876, binary);
	for(i=0;i<=length;i++) {
		x=i*2;
		if(binary[i]==1) {
			createLow(x+16, x+16+1);
		} else {
			createHigh(x+16, x+16+1);
		}
	}
}

static void createId(int id, int unit, int state) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	int code = codes[id][unit][state];

	length = decToBin(code, binary);
	for(i=0;i<=length;i++) {
		x=i*2;
		if(binary[i]==1) {
			createLow(x, x+1);
		} else {
			createHigh(x, x+1);
		}
	}
}

static void createFooter(void) {
	conrad_rsl_switch->raw[64]=(AVG_PULSE_LENGTH*3);
	conrad_rsl_switch->raw[65]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int state = -1;
	int unit = -1;
	int all = 0;
	int learn = -1;
	double itmp = 0;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "all", &itmp) == 0)
		all = 1;
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;
	if(json_find_number(code, "learn", &itmp) == 0)
			learn = 1;

	if(unit == -1 || (id == -1 && all == 0) || state == -1) {
		logprintf(LOG_ERR, "conrad_rsl_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if((id > 5 || id < 0) && all == 0) {
		logprintf(LOG_ERR, "conrad_rsl_switch: invalid id range");
		return EXIT_FAILURE;
	} else if(unit > 5 || unit < 0) {
		logprintf(LOG_ERR, "conrad_rsl_switch: invalid unit range");
		return EXIT_FAILURE;
	} else {
		if(all == 1) {
			id = 5;
		}
		id -= 1;
		unit -= 1;
		createMessage(id, unit, state, learn);
		clearCode();
		createId(id, unit, state);
		createFooter();
		conrad_rsl_switch->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -i --id=id\tcontrol a device with this id\n");
	printf("\t -i --unit=unit\tcontrol a device with this unit\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -a --all\t\t\tsend an all signal\n");
	printf("\t -l --learn\t\t\tsend multiple streams so switch can learn\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void conradRSLSwitchInit(void) {

	memset(codes, 0, 33);
	codes[0][0][0] = 190;
	codes[0][0][1] = 182;
	codes[0][1][0] = 129;
	codes[0][1][1] = 142;
	codes[0][2][0] = 174;
	codes[0][2][1] = 166;
	codes[0][3][0] = 158;
	codes[0][3][1] = 150;

	codes[1][0][0] = 181;
	codes[1][0][1] = 185;
	codes[1][1][0] = 141;
	codes[1][1][1] = 133;
	codes[1][2][0] = 165;
	codes[1][2][1] = 169;
	codes[1][3][0] = 149;
	codes[1][3][1] = 153;

	codes[2][0][0] = 184;
	codes[2][0][1] = 176;
	codes[2][1][0] = 132;
	codes[2][1][1] = 136;
	codes[2][2][0] = 168;
	codes[2][2][1] = 160;
	codes[2][3][0] = 152;
	codes[2][3][1] = 144;

	codes[3][0][0] = 178;
	codes[3][0][1] = 188;
	codes[3][1][0] = 138;
	codes[3][1][1] = 130;
	codes[3][2][0] = 162;
	codes[3][2][1] = 172;
	codes[3][3][0] = 146;
	codes[3][3][1] = 156;

	codes[4][0][0] = 163;
	codes[4][0][1] = 147;

	protocol_register(&conrad_rsl_switch);
	protocol_set_id(conrad_rsl_switch, "conrad_rsl_switch");
	protocol_device_add(conrad_rsl_switch, "conrad_rsl_switch", "Conrad RSL Switches");
	conrad_rsl_switch->devtype = SWITCH;
	conrad_rsl_switch->hwtype = RF433;
	conrad_rsl_switch->txrpt = NORMAL_REPEATS;
	conrad_rsl_switch->minrawlen = RAW_LENGTH;
	conrad_rsl_switch->maxrawlen = RAW_LENGTH;
	conrad_rsl_switch->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	conrad_rsl_switch->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&conrad_rsl_switch->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[1-4]$");
	options_add(&conrad_rsl_switch->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[1-4]$");
	options_add(&conrad_rsl_switch->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&conrad_rsl_switch->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&conrad_rsl_switch->options, "a", "all", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);
	options_add(&conrad_rsl_switch->options, "l", "learn", OPTION_NO_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);

	options_add(&conrad_rsl_switch->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&conrad_rsl_switch->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	conrad_rsl_switch->parseCode=&parseCode;
	conrad_rsl_switch->createCode=&createCode;
	conrad_rsl_switch->printHelp=&printHelp;
	conrad_rsl_switch->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "conrad_rsl_switch";
	module->version = "2.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	conradRSLSwitchInit();
}
#endif
