/*
	Copyright (C) 2014 CurlyMo & wo-rasp

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
#include "beamish_switch.h"

#define PULSE_MULTIPLIER	4
#define NORMAL_REPEATS		10
#define MIN_PULSE_LENGTH	318
#define MAX_PULSE_LENGTH	328
#define AVG_PULSE_LENGTH	323
#define RAW_LENGTH				50

static int map[7] = {0, 192, 48, 12, 3, 15, 195};

static int validate(void) {
	if(beamish_switch->rawlen == RAW_LENGTH) {
		if(beamish_switch->raw[beamish_switch->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   beamish_switch->raw[beamish_switch->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state, int all) {
	beamish_switch->message = json_mkobject();
	json_append_member(beamish_switch->message, "id", json_mknumber(id, 0));
	if(all == 1) {
		json_append_member(beamish_switch->message, "all", json_mknumber(1, 0));
	} else {
		json_append_member(beamish_switch->message, "unit", json_mknumber(unit, 0));
	}
	if(state == 0) {
		json_append_member(beamish_switch->message, "state", json_mkstring("off"));
	}
	if(state == 1) {
		json_append_member(beamish_switch->message, "state", json_mkstring("on"));
	}
}

static void parseCode(void) {
	int i = 0, x = 0, y = 0, binary[RAW_LENGTH/2];
	int id = -1, state = -1, unit = -1, all = 0, code = 0;

	if(beamish_switch->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "beamish_switch: parsecode - invalid parameter passed %d", beamish_switch->rawlen);
		return;
	}

	for(x=0;x<beamish_switch->rawlen;x+=2) {
		if(beamish_switch->raw[x] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	id = binToDecRev(binary, 0, 15);
	code = binToDecRev(binary, 16, 23);

	for(y=0;y<7;y++) {
		if(map[y] == code) {
			unit = y;
			break;
		}
	}

	if(unit == 5) {
		state = 1;
		all = 1;
	}
	if(unit == 6) {
		state = 0;
		all = 1;
	}

	createMessage(id, unit, state, all);
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		beamish_switch->raw[i]=(AVG_PULSE_LENGTH);
		beamish_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		beamish_switch->raw[i]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		beamish_switch->raw[i+1]=(AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createHigh(0,47);
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*2;
			createLow(31-(x+1), 31-x);
		}
	}
}

static void createUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*2;
			createLow(47-(x+1), 47-x);
		}
	}
}

static void createFooter(void) {
	beamish_switch->raw[48]=(AVG_PULSE_LENGTH);
	beamish_switch->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	int all = 0;
	double itmp = -1;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "all", &itmp) == 0)
		all = 1;
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(id == -1 || (unit == -1 && all == 0) || (state == -1 && all == 1)) {
		logprintf(LOG_ERR, "beamish_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 65535 || id < 1) {
		logprintf(LOG_ERR, "beamish_switch: invalid id range");
		return EXIT_FAILURE;
	} else if((unit > 4 || unit < 1) && all == 0) {
		logprintf(LOG_ERR, "beamish_switch: invalid unit range");
		return EXIT_FAILURE;
	} else {
		if(all == 1 && state == 1)
			unit = 5;
		if(all == 1 && state == 0)
			unit = 6;

		createMessage(id, unit, state, all);
		clearCode();
		createId(id);
		unit = map[unit];
		createUnit(unit);
		createFooter();
		beamish_switch->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -a --all\t\t\tsend command to all devices with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void beamishSwitchInit(void) {

	protocol_register(&beamish_switch);
	protocol_set_id(beamish_switch, "beamish_switch");
	protocol_device_add(beamish_switch, "beamish_switch", "beamish_switch Switches");
	beamish_switch->devtype = SWITCH;
	beamish_switch->hwtype = RF433;
	beamish_switch->txrpt = NORMAL_REPEATS;
	beamish_switch->minrawlen = RAW_LENGTH;
	beamish_switch->maxrawlen = RAW_LENGTH;
	beamish_switch->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	beamish_switch->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&beamish_switch->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&beamish_switch->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&beamish_switch->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([1-4])$");
	options_add(&beamish_switch->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
	options_add(&beamish_switch->options, "a", "all", OPTION_NO_VALUE, 0, JSON_NUMBER, NULL, NULL);

	options_add(&beamish_switch->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&beamish_switch->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	beamish_switch->parseCode=&parseCode;
	beamish_switch->createCode=&createCode;
	beamish_switch->printHelp=&printHelp;
	beamish_switch->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "beamish_switch";
	module->version = "1.2";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	beamishSwitchInit();
}
#endif


