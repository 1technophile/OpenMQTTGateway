/*
	Copyright (C) 2014 CurlyMo & wo_rasp

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
#include "quigg_screen.h"

#define	PULSE_QUIGG_SCREEN_SHORT	700
#define	PULSE_QUIGG_SCREEN_LONG	1400
#define	PULSE_QUIGG_SCREEN_FOOTER	81000
#define	PULSE_QUIGG_SCREEN_50		PULSE_QUIGG_SCREEN_SHORT+(PULSE_QUIGG_SCREEN_LONG-PULSE_QUIGG_SCREEN_SHORT)/2

#define LEARN_REPEATS 		4
#define NORMAL_REPEATS		4
#define PULSE_MULTIPLIER	2
#define AVG_PULSE_LENGTH	PULSE_QUIGG_SCREEN_SHORT
#define MIN_PULSE_LENGTH	AVG_PULSE_LENGTH-80
#define MAX_PULSE_LENGTH	AVG_PULSE_LENGTH+260
#define RAW_LENGTH				42

static int validate(void) {
	if(quigg_screen->rawlen == RAW_LENGTH) {
		if(quigg_screen->raw[quigg_screen->rawlen-1] >= (int)(PULSE_QUIGG_SCREEN_FOOTER*0.9) &&
			 quigg_screen->raw[quigg_screen->rawlen-1] <= (int)(PULSE_QUIGG_SCREEN_FOOTER*1.1) &&
			 quigg_screen->raw[0] >= MIN_PULSE_LENGTH &&
			 quigg_screen->raw[0] <= MAX_PULSE_LENGTH) {
		return 0;
		}
	}
	return -1;
}


static void createMessage(int id, int state, int unit, int all, int learn) {
	quigg_screen->message = json_mkobject();
	json_append_member(quigg_screen->message, "id", json_mknumber(id, 0));
	if(all==1) {
		json_append_member(quigg_screen->message, "all", json_mknumber(all, 0));
	} else {
		json_append_member(quigg_screen->message, "unit", json_mknumber(unit, 0));
	}
	if(state==0) {
		json_append_member(quigg_screen->message, "state", json_mkstring("up"));
	} else {
		json_append_member(quigg_screen->message, "state", json_mkstring("down"));
	}

	if(learn == 1) {
		quigg_screen->txrpt = LEARN_REPEATS;
	} else {
		quigg_screen->txrpt = NORMAL_REPEATS;
	}
}

static void parseCode(void) {
	int binary[RAW_LENGTH/2], x = 0, dec_unit[4] = {0, 3, 1, 2};
	int iParity = 1, iParityData = -1;	// init for even parity
	int iSwitch = 0;

	if(quigg_screen->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "quigg_screen: parsecode - invalid parameter passed %d", quigg_screen->rawlen);
		return;
	}

	// 42 bytes are the number of raw bytes
	// Byte 1,2 in raw buffer is the first logical byte, rawlen-3,-2 is the parity bit, rawlen-1 is the footer
	for(x=0; x<quigg_screen->rawlen-1; x+=2) {
		if(quigg_screen->raw[x+1] > PULSE_QUIGG_SCREEN_50) {
			binary[x/2] = 1;
			if((x / 2) > 11 && (x / 2) < 19) {
				iParityData = iParity;
				iParity = -iParity;
			}
		} else {
			binary[x/2] = 0;
		}
	}
	if(iParityData < 0)
		iParityData=0;

	int id = binToDecRev(binary, 0, 11);
	int unit = binToDecRev(binary, 12, 13);
	int all = binToDecRev(binary, 14, 14);
	int state = binToDecRev(binary, 15, 15);
	int screen = binToDecRev(binary, 16, 16);
	int parity = binToDecRev(binary, 19, 19);
	int learn = 0;

	unit = dec_unit[unit];

	// Prepare dim up/down mode for appropriate handling with createMessage
	iSwitch = (screen * 2) + state;	// 00-turn off 01-turn on 10-dim up 11-dim down

	switch(iSwitch) {
		case 0:
		case 1:
			screen=-1;	// mode handled by quigg_GT7000 protocol driver
		break;
		case 2:
		case 3:
		default:
		break;
	}
	if((iParityData == parity) && (screen != -1)) {
		createMessage(id, state, unit, all, learn);
	}
}

static void createZero(int s, int e) {
	int i;
	for(i=s;i<=e;i+=2) {
		quigg_screen->raw[i] = PULSE_QUIGG_SCREEN_SHORT;
		quigg_screen->raw[i+1] = PULSE_QUIGG_SCREEN_LONG;
	}
}

static void createOne(int s, int e) {
	int i;
	for(i=s;i<=e;i+=2) {
		quigg_screen->raw[i] = PULSE_QUIGG_SCREEN_LONG;
		quigg_screen->raw[i+1] = PULSE_QUIGG_SCREEN_SHORT;
	}
}

static void createHeader(void) {
	quigg_screen->raw[0] = PULSE_QUIGG_SCREEN_SHORT;
}

static void createFooter(void) {
	quigg_screen->raw[quigg_screen->rawlen-1] = PULSE_QUIGG_SCREEN_FOOTER;
}

static void clearCode(void) {
	createHeader();
	createZero(1,quigg_screen->rawlen-3);
}

static void createId(int id) {
	int binary[16], length = 0, i = 0, x = 23;

	length = decToBin(id, binary);
	for(i=length;i>=0;i--) {
		if(binary[i] == 1) {
			createOne(x, x+1);
		}
		x = x-2;
	}
}

static void createUnit(int unit) {
	switch (unit) {
		case 0:
			createZero(25, 30);	// Unit 0 screen
			createOne(33, 34);
			createOne(37, 38);
		break;
		case 1:
			createOne(25, 26);	// Unit 1 screen
			createOne(33, 34);
		break;
		case 2:
			createOne(25, 28);	// Unit 2 screen
			createOne(33, 34);
		break;
		case 3:
			createOne(27, 28);	// Unit 3 screen
			createOne(33, 34);
			createOne(37, 38);
		break;
		case 4:
			createOne(25, 30);	// MASTER (all) screen
			createOne(33, 34);
			createOne(37, 38);
		break;
		default:
		break;
	}
}

static void createState(int state) {
	if(state==1) {
		createOne(31, 32);	// dim down
	}
}

static void createParity(void) {
	int i, p = 1;	// init even parity, without system ID
	for(i=25;i<=37;i+=2) {
		if(quigg_screen->raw[i] == PULSE_QUIGG_SCREEN_LONG) {
			p = -p;
		}
	}
	if(p == -1) {
		createOne(39,40);
	}
}

static int createCode(JsonNode *code) {
	double itmp = -1;
	int unit = -1, id = -1, learn = -1, state = -1, all = 0;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "all", &itmp) == 0)
		all = (int)round(itmp);
	if(json_find_number(code, "learn", &itmp) == 0)
		learn = (int)round(itmp);
	if(json_find_number(code, "up", &itmp) == 0)
		state=0;
	if(json_find_number(code, "down", &itmp) == 0)
		state=1;

	if(id==-1 || (unit==-1 && all==0) || state==-1) {
		logprintf(LOG_ERR, "quigg_screen: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 4095 || id < 0) {
		logprintf(LOG_ERR, "quigg_screen: invalid system id range");
		return EXIT_FAILURE;
	} else if((unit > 3 || unit < 0) && all == 0) {
		logprintf(LOG_ERR, "quigg_screen: invalid unit code range");
		return EXIT_FAILURE;
	} else {
		if(unit == -1 && all == 1) {
			unit = 4;
		}
		quigg_screen->rawlen = RAW_LENGTH;
		createMessage(id, state, unit, all, learn);
		clearCode();
		createId(id);
		createUnit(unit);
		createState(state);
		createParity();
		createFooter();
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -i --id=id\t\t\tselect the device units with this system id\n");
	printf("\t -a --id=all\t\t\tcontrol all devices selected with the system id\n");
	printf("\t -u --unit=unit\t\t\tcontrol the individual device unit selected with the system id\n");
	printf("\t -f --up\t\t\tsend an darken DOWN command to the selected device\n");
	printf("\t -l --learn\t\t\temulate learning mode of remote control\n");
	printf("\t -t --down\t\t\tsend an brighten UP command to the selected device\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void quiggScreenInit(void) {

	protocol_register(&quigg_screen);
	protocol_set_id(quigg_screen, "quigg_screen");
	protocol_device_add(quigg_screen, "quigg_screen", "Quigg Switch Screen");
	quigg_screen->devtype = SCREEN;
	quigg_screen->hwtype = RF433;
	quigg_screen->txrpt = NORMAL_REPEATS;                    // SHORT: GT-FSI-04a range: 620... 960
	quigg_screen->minrawlen = RAW_LENGTH;
	quigg_screen->maxrawlen = RAW_LENGTH;
	quigg_screen->maxgaplen = (int)(PULSE_QUIGG_SCREEN_FOOTER*0.9);
	quigg_screen->mingaplen = (int)(PULSE_QUIGG_SCREEN_FOOTER*1.1);

	options_add(&quigg_screen->options, "t", "up", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&quigg_screen->options, "f", "down", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&quigg_screen->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-3])$");
	options_add(&quigg_screen->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([1-9]|[1-9][0-9]|[1-9][0-9][0-9]|[1-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-5])$");
	options_add(&quigg_screen->options, "a", "all", OPTION_NO_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);
	options_add(&quigg_screen->options, "l", "learn", OPTION_HAS_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);

	options_add(&quigg_screen->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&quigg_screen->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	quigg_screen->parseCode=&parseCode;
	quigg_screen->createCode=&createCode;
	quigg_screen->printHelp=&printHelp;
	quigg_screen->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "quigg_screen";
	module->version = "2.2";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	quiggScreenInit();
}
#endif
