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
#include "elro_300_switch.h"

#define PULSE_MULTIPLIER	4
#define MIN_PULSE_LENGTH	297
#define MAX_PULSE_LENGTH	307
#define AVG_PULSE_LENGTH	302
#define RAW_LENGTH				116

static int validate(void) {
	if(elro_300_switch->rawlen == RAW_LENGTH) {
		if(elro_300_switch->raw[elro_300_switch->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   elro_300_switch->raw[elro_300_switch->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

/**
 * Creates as System message informing the daemon about a received or created message
 *
 * systemcode : integer number, the 32 bit system code
 * unitcode : unit being adressed, integer number
 * state : either 2 (off) or 1 (on)
 * group : if 1 this affects a whole group of devices
 */
static void createMessage(unsigned long long systemcode, int unitcode, int state, int group) {
	elro_300_switch->message = json_mkobject();
	//aka address
	json_append_member(elro_300_switch->message, "systemcode", json_mknumber((double)systemcode, 0));
	//toggle all or just one unit
	if(group == 1) {
	    json_append_member(elro_300_switch->message, "all", json_mknumber(group, 0));
	} else {
	    json_append_member(elro_300_switch->message, "unitcode", json_mknumber(unitcode, 0));
	}
	//aka command
	if(state == 1) {
		json_append_member(elro_300_switch->message, "state", json_mkstring("on"));
	}
	else if(state == 2) {
		json_append_member(elro_300_switch->message, "state", json_mkstring("off"));
	}
}

/**
 * This is the main method when reading a received code
 * Decodes the received stream
 *
 */
static void parseCode(void) {
	int i = 0, x = 0, binary[RAW_LENGTH/2];

	if(elro_300_switch->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "elro_300_switch: parsecode - invalid parameter passed %d", elro_300_switch->rawlen);
		return;
	}

	//utilize the "code" field
	//at this point the code field holds translated "0" and "1" codes from the received pulses
	//this means that we have to combine these ourselves into meaningful values in groups of 2

	for(i=0; i < elro_300_switch->rawlen; i++) {
		if(elro_300_switch->raw[i] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			if(i&1) {
				binary[x++] = 1;
			} else {
				return; // even pulse lengths must be low
			}
		} else if(i&1) {
			binary[x++] = 0;
		}
	}

	//chunked code now contains "groups of 2" codes for us to handle.
	unsigned long long systemcode = binToDecRevUl(binary, 11, 42);
	int groupcode = binToDec(binary, 43, 46);
	int groupcode2 = binToDec(binary, 49, 50);
	int unitcode = binToDec(binary, 51, 56);
	int state = binToDec(binary, 47, 48);
	int groupRes = 0;

	if(groupcode == 13 && groupcode2 == 2) {
		groupRes = 0;
	} else if(groupcode == 3 && groupcode2 == 3) {
		groupRes = 1;
	} else {
		return;
	}
	if(state < 1 || state > 2) {
		return;
	} else {
		createMessage(systemcode, unitcode, state, groupRes);
	}
}

/**
 * Creates a number of "low" entries (302 302). Note that each entry requires 2 raw positions
 * so e-s should be a multiple of 2
 * s : start position in the raw code (inclusive)
 * e : end position in the raw code (inclusive)
 */
static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		elro_300_switch->raw[i]=(AVG_PULSE_LENGTH);
		elro_300_switch->raw[i+1]=(AVG_PULSE_LENGTH);
	}
}

/**
 * Creates a number of "high" entries (302 1028). Note that each entry requires 2 raw positions
 * so e-s should be a multiple of 2
 * s : start position in the raw code (inclusive)
 * e : end position in the raw code (inclusive)
 */
static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		elro_300_switch->raw[i]=(AVG_PULSE_LENGTH);
		elro_300_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

/**
 * This simply clears the full length of the code to be all "zeroes" (LOW entries)
 */
static void elro300ClearCode(void) {
	createLow(0,116);
}

/**
 * Takes the passed number
 * converts it into raw and inserts them into the raw code at the appropriate position
 *
 * systemcode : unsigned integer number, the 32 bit system code
 */
static void createSystemCode(unsigned long long systemcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;
	length = decToBinRevUl(systemcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[(length)-i]==1) {
			x=i*2;
			createHigh(22+x, 22+x+1);
		}
	}
}

/**
 * Takes the passed number converts it into raw and inserts it into the raw code at the appropriate position
 *
 * unitcode : integer number, id of the unit to control
 */
static void createUnitCode(int unitcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unitcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*2;
			createHigh(102+x, 102+x+1);
		}
	}
}

/**
 * Takes the passed number converts it into raw and inserts it into the raw code at the appropriate position
 *
 * state : integer number, state value to set. can be either 1 (on) or 2 (off)
 */
static void createState(int state) {
	if(state == 1) {
		createHigh(94, 95);
		createLow(96, 97);
	}
	else {
    	createLow(94, 95);
		createHigh(96, 97);
	}
}

/**
 * sets the first group code portions to the appropriate raw values.
 * Fro grouped mode this is the equivalent to 1100 and 11, for non-grouped mode 1011 and 01
 *
 * group : integer value, 1 means grouped enabled, 0 means disabled
 */
static void createGroupCode(int group) {
    if(group == 1) {
		createHigh(86, 89);
		createLow(90, 93);
		createHigh(98, 101);
    } else {
		createHigh(86, 87);
		createLow(88, 89);
		createHigh(90, 93);
		createLow(98, 99);
		createHigh(100, 101);
    }
}

/**
 * Inserts the (as far as is known) fixed message preamble
 * First eleven words are the preamble
 */
static void createPreamble(void) {
	createHigh(0,3);
	createLow(4,9);
	createHigh(10,17);
	createLow(18,21);
}

/**
 * Inserts the message trailer (one HIGH) into the raw message
 */
static void createFooter(void) {
	elro_300_switch->raw[114]=(AVG_PULSE_LENGTH);
	elro_300_switch->raw[115]=(PULSE_DIV*AVG_PULSE_LENGTH);
}


/**
 * Main method for creating a message based on daemon-passed values in the elro_300_switch protocol.
 * code : JSON Message containing the received parameters to use for message creation
 *
 * returns : EXIT_SUCCESS or EXIT_FAILURE on obvious occasions
 */
static int createCode(struct JsonNode *code) {
	unsigned long long systemcode = 0;
	int unitcode = -1;
	int group = 0;
	int state = -1;
	double itmp;

	if(json_find_number(code, "systemcode", &itmp) == 0) {
		systemcode = (unsigned long long)itmp;
	}
	if(json_find_number(code, "unitcode", &itmp) == 0) {
		unitcode = (int)round(itmp);
	}
	if(json_find_number(code, "all", &itmp) == 0) {
	    group = 1;
	    //on the reference remote, group toggles always used a unit code of 56.
    	    //for that reason we are enforcing that here
	    unitcode = 56;
	}

	if(json_find_number(code, "off", &itmp) == 0) {
		state=2;
	}
	else if(json_find_number(code, "on", &itmp) == 0) {
		state=1;
	}

	if(systemcode == 0 || unitcode == -1 || state == -1) {
		logprintf(LOG_ERR, "elro_300_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(systemcode > 4294967295u || unitcode > 99 || unitcode < 0) {
		logprintf(LOG_ERR, "elro_300_switch: values out of valid range");
	} else {
		createMessage(systemcode, unitcode, state, group);
		elro300ClearCode();
		createPreamble();
		createSystemCode(systemcode);
		createGroupCode(group);
		createState(state);
		createUnitCode(unitcode);
		createFooter();
		elro_300_switch->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

/**
 * Outputs help messages directly to the current output target (probably the console)
 */
static void printHelp(void) {
	printf("\t -s --systemcode=systemcode\tcontrol a device with this systemcode\n");
	printf("\t -a --all\t\t\ttoggle switching all devices on or off\n");
	printf("\t -u --unitcode=unitcode\t\tcontrol a device with this unitcode\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
}

/**
 * Main Init method called to init the protocol and register its functions with pilight
 */
#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void elro300SwitchInit(void) {

	protocol_register(&elro_300_switch);
	protocol_set_id(elro_300_switch, "elro_300_switch");
	protocol_device_add(elro_300_switch, "elro_300_switch", "Elro 300 Series Switches");
	elro_300_switch->devtype = SWITCH;
	elro_300_switch->hwtype = RF433;
	elro_300_switch->minrawlen = RAW_LENGTH;
	elro_300_switch->maxrawlen = RAW_LENGTH;
	elro_300_switch->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	elro_300_switch->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&elro_300_switch->options, "s", "systemcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,9}|[1-3][0-9]{9}|4([01][0-9]{8}|2([0-8][0-9]{7}|9([0-3][0-9]{6}|4([0-8][0-9]{5}|9([0-5][0-9]{4}|6([0-6][0-9]{3}|7([01][0-9]{2}|2([0-8][0-9]|9[0-4])))))))))$");
	options_add(&elro_300_switch->options, "u", "unitcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[0-9]{1,2}$");
	options_add(&elro_300_switch->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&elro_300_switch->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&elro_300_switch->options, "a", "all", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);

	options_add(&elro_300_switch->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&elro_300_switch->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");


	elro_300_switch->parseCode=&parseCode;
	elro_300_switch->createCode=&createCode;
	elro_300_switch->printHelp=&printHelp;
	elro_300_switch->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "elro_300_switch";
	module->version = "2.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	elro300SwitchInit();
}
#endif
