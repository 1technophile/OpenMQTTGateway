/*
	Copyright (C) 2014 CurlyMo & Bram1337 & kirichkov

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
#include "rsl366.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	375
#define MAX_PULSE_LENGTH	395
#define AVG_PULSE_LENGTH	480
#define RAW_LENGTH		50

static int validate(void) {
	if(rsl366->rawlen == RAW_LENGTH) {
		if(rsl366->raw[rsl366->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
                   rsl366->raw[rsl366->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV) &&
                   rsl366->raw[rsl366->rawlen-3] <= (AVG_PULSE_LENGTH*PULSE_MULTIPLIER) &&
                   rsl366->raw[rsl366->rawlen-7] <= (AVG_PULSE_LENGTH*PULSE_MULTIPLIER) &&
                   rsl366->raw[rsl366->rawlen-11] <= (AVG_PULSE_LENGTH*PULSE_MULTIPLIER)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int systemcode, int programcode, int state) {
	rsl366->message = json_mkobject();
	json_append_member(rsl366->message, "systemcode", json_mknumber(systemcode, 0));
	json_append_member(rsl366->message, "programcode", json_mknumber(programcode, 0));
	if(state == 1) {
		json_append_member(rsl366->message, "state", json_mkstring("on"));
	} else {
		json_append_member(rsl366->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int x = 0, i = 0, binary[RAW_LENGTH/4];

	if(rsl366->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "rsl366: parsecode - invalid parameter passed %d", rsl366->rawlen);
		return;
	}

	/* Convert the one's and zero's into binary */
	for(x=3;x<rsl366->rawlen;x+=4) {
		if(rsl366->raw[x] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++]=1;
		} else {
			binary[i++]=0;
		}
	}

	//Check if there is a valid systemcode
	if((binary[0]+binary[1]+binary[2]+binary[3]) > 1)
                return;

        //Get systemcode: 1000=>1, 0100=>2, 0010=>3, 0001=>4
        int systemcode = 0;
        for(i=0;i<4;i++) {
        	if(binary[i] == 1)
        		systemcode = i+1;
        }

        //Check if there is a valid programcode
        if((binary[4]+binary[5]+binary[6]+binary[7]) > 1)
                return;

        //Get programcode: 1000=>1, 0100=>2, 0010=>3, 0001=>4
        int programcode = 0;
        for(i=4;i<8;i++) {
        	if(binary[i] == 1)
        		programcode = i-3;
        }

        //Check if a system and programcode was found
        if(systemcode == 0 || programcode == 0)
        	return;

	// There seems to be no check and binary[10] is always a low
	int state = binary[11]^1;

	createMessage(systemcode, programcode, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		rsl366->raw[i]=AVG_PULSE_LENGTH;
		rsl366->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		rsl366->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		rsl366->raw[i+3]=AVG_PULSE_LENGTH;
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		rsl366->raw[i]=AVG_PULSE_LENGTH;
		rsl366->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		rsl366->raw[i+2]=AVG_PULSE_LENGTH;
		rsl366->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createLow(0,47);
}

static void createSystemCode(int systemcode) {
	createHigh((systemcode-1)*4, (systemcode-1)*4+3);
}

static void createProgramCode(int programcode) {
	createHigh(16+(programcode-1)*4, 16+(programcode-1)*4+3);
}

static void createState(int state) {
	if(state == 0) {
		createHigh(44, 47);
	}
}

static void createFooter(void) {
	rsl366->raw[48]=(AVG_PULSE_LENGTH);
	rsl366->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
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
		logprintf(LOG_ERR, "rsl366: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(systemcode > 4 || systemcode < 0) {
		logprintf(LOG_ERR, "rsl366: invalid systemcode range");
		return EXIT_FAILURE;
	} else if(programcode > 4 || programcode < 0) {
		logprintf(LOG_ERR, "rsl366: invalid programcode range");
		return EXIT_FAILURE;
	} else {
		createMessage(systemcode, programcode, state);
		clearCode();
		createSystemCode(systemcode);
		createProgramCode(programcode);
		createState(state);
		createFooter();
		rsl366->rawlen = RAW_LENGTH;
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
void rsl366Init(void) {

	protocol_register(&rsl366);
	protocol_set_id(rsl366, "rsl366");
	protocol_device_add(rsl366, "rsl366", "RSL366 Switches");
	protocol_device_add(rsl366, "promax", "Pro MAX Switches");
	rsl366->devtype = SWITCH;
	rsl366->hwtype = RF433;
	rsl366->minrawlen = RAW_LENGTH;
	rsl366->maxrawlen = RAW_LENGTH;
	rsl366->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	rsl366->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&rsl366->options, "s", "systemcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([1234]{1})$");
	options_add(&rsl366->options, "u", "programcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([1234]{1})$");
	options_add(&rsl366->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&rsl366->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&rsl366->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&rsl366->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	rsl366->parseCode=&parseCode;
	rsl366->createCode=&createCode;
	rsl366->printHelp=&printHelp;
	rsl366->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "rsl366";
	module->version = "2.7";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	rsl366Init();
}
#endif
