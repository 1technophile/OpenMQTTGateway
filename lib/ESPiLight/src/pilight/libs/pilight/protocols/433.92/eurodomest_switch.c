/*
	Copyright (C) 2015 woutput

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
#include "eurodomest_switch.h"

// Timing from http://forum.pilight.org/attachment.php?aid=595

// Increase this value to be more robust, but also create more false positives
#define PEAK_TO_PEAK_JITTER	200

// About 2/3 of the jitter appears to make the pulses longer instead of shorter
// The same jitter appears on every pulse type (as may be expected)

// Short pulse timing
#define MIN_SHORT_PULSE_LENGTH	(AVG_SHORT_PULSE_LENGTH - 0.333 * PEAK_TO_PEAK_JITTER)
#define AVG_SHORT_PULSE_LENGTH	280
#define MAX_SHORT_PULSE_LENGTH	(AVG_SHORT_PULSE_LENGTH + 0.667 * PEAK_TO_PEAK_JITTER)

// Medium pulse timing
#define MIN_MEDIUM_PULSE_LENGTH	(AVG_MEDIUM_PULSE_LENGTH - 0.333 * PEAK_TO_PEAK_JITTER)
#define AVG_MEDIUM_PULSE_LENGTH	868
#define MAX_MEDIUM_PULSE_LENGTH	(AVG_MEDIUM_PULSE_LENGTH + 0.667 * PEAK_TO_PEAK_JITTER)

// Long pulse timing
#define MIN_LONG_PULSE_LENGTH	(AVG_LONG_PULSE_LENGTH - 0.333 * PEAK_TO_PEAK_JITTER)
#define AVG_LONG_PULSE_LENGTH	9660
#define MAX_LONG_PULSE_LENGTH	(AVG_LONG_PULSE_LENGTH + 0.667 * PEAK_TO_PEAK_JITTER)

#define RAW_LENGTH		50
// Two pulses per bit, last two pulses are footer
#define BINARY_LENGTH		24

#define LEARN_REPEATS		40
#define NORMAL_REPEATS		10

static int validate(void) {
	if (eurodomest_switch->rawlen == RAW_LENGTH) {
		if (eurodomest_switch->raw[eurodomest_switch->rawlen - 1] >= MIN_LONG_PULSE_LENGTH &&
		    eurodomest_switch->raw[eurodomest_switch->rawlen - 1] <= MAX_LONG_PULSE_LENGTH) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state, int all, int learn) {
	eurodomest_switch->message = json_mkobject();

	json_append_member(eurodomest_switch->message, "id", json_mknumber(id, 0));

	if (all == 1) {
		json_append_member(eurodomest_switch->message, "all", json_mknumber(all, 0));
	} else {
		json_append_member(eurodomest_switch->message, "unit", json_mknumber(unit, 0));
	}

	if (state == 1) {
		json_append_member(eurodomest_switch->message, "state", json_mkstring("on"));
	} else {
		json_append_member(eurodomest_switch->message, "state", json_mkstring("off"));
	}

	if (learn == 1) {
		eurodomest_switch->txrpt = LEARN_REPEATS;
	} else {
		eurodomest_switch->txrpt = NORMAL_REPEATS;
	}
}

static void parseCode(void) {
	int binary[BINARY_LENGTH], x = 0, i = 0;

	for (x = 0; x < eurodomest_switch->rawlen - 2; x += 2) {
		if ((eurodomest_switch->raw[x] >= MIN_MEDIUM_PULSE_LENGTH) &&
		    (eurodomest_switch->raw[x] <= MAX_MEDIUM_PULSE_LENGTH) &&
		    (eurodomest_switch->raw[x + 1] >= MIN_SHORT_PULSE_LENGTH) &&
		    (eurodomest_switch->raw[x + 1] <= MAX_SHORT_PULSE_LENGTH)) {
			binary[i++] = 0;
		} else if ((eurodomest_switch->raw[x] >= MIN_SHORT_PULSE_LENGTH) &&
			   (eurodomest_switch->raw[x] <= MAX_SHORT_PULSE_LENGTH) &&
			   (eurodomest_switch->raw[x + 1] >= MIN_MEDIUM_PULSE_LENGTH) &&
			   (eurodomest_switch->raw[x + 1] <= MAX_MEDIUM_PULSE_LENGTH)) {
			binary[i++] = 1;
		} else {
			return; // decoding failed, return without creating message
		}
	}

	// The last 4 bits contain the unit, state and all information

	int unit = 0;
	int all = 0;
	int state = 0;

	if (binary[20] == 0 && binary[21] == 0 && binary[22] == 0 && binary[23] == 0) {
	  	unit = 1;
	  	all = 0;
	  	state = 1; // on
	} else if (binary[20] == 0 && binary[21] == 0 && binary[22] == 0 && binary[23] == 1) {
	  	unit = 1;
	  	all = 0;
	  	state = 0; // off
	} else if (binary[20] == 0 && binary[21] == 0 && binary[22] == 1 && binary[23] == 1) {
	  	unit = 2;
	  	all = 0;
	  	state = 0; // off
	} else if (binary[20] == 0 && binary[21] == 0 && binary[22] == 1 && binary[23] == 0) {
	  	unit = 2;
	  	all = 0;
	  	state = 1; // on
	} else if (binary[20] == 0 && binary[21] == 1 && binary[22] == 0 && binary[23] == 1) {
	  	unit = 3;
	  	all = 0;
	  	state = 0; // off
	} else if (binary[20] == 0 && binary[21] == 1 && binary[22] == 0 && binary[23] == 0) {
	  	unit = 3;
	  	all = 0;
	  	state = 1; // on
	} else if (binary[20] == 1 && binary[21] == 0 && binary[22] == 0 && binary[23] == 1) {
	  	unit = 4;
	  	all = 0;
	  	state = 0; // off
	} else if (binary[20] == 1 && binary[21] == 0 && binary[22] == 0 && binary[23] == 0) {
	  	unit = 4;
	  	all = 0;
	  	state = 1; // on
	} else if (binary[20] == 1 && binary[21] == 1 && binary[22] == 1 && binary[23] == 0) {
	  	unit = 0; // not used, all = 1
	  	all = 1;
	  	state = 0; // off
	} else if (binary[20] == 1 && binary[21] == 1 && binary[22] == 0 && binary[23] == 1) {
	  	unit = 0; // not used, all = 1
	  	all = 1;
	  	state = 1; // on
	} else {
		return; // decoding failed, return without creating message
	}

	int id = binToDec(binary, 0, 19);
	createMessage(id, unit, state, all, 0);
}

static void createLow(int s, int e) {
	int i;

	for (i = s; i <= e; i += 2) { // medium - short
		eurodomest_switch->raw[i] = AVG_MEDIUM_PULSE_LENGTH;
		eurodomest_switch->raw[i + 1] = AVG_SHORT_PULSE_LENGTH;
	}
}

static void createHigh(int s, int e) {
	int i;

	for (i = s; i <= e; i += 2) { // short - medium
		eurodomest_switch->raw[i] = AVG_SHORT_PULSE_LENGTH;
		eurodomest_switch->raw[i + 1] = AVG_MEDIUM_PULSE_LENGTH;
	}
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i = 0, x = 0;

	length = decToBinRev(id, binary);
	for (i = 0; i <= length; i++) {
		if (binary[i] == 0) {
			x = i * 2;
			createLow(x, x+1);
		} else { //so binary[i] == 1
			x = i * 2;
			createHigh(x, x + 1);
		}
	}
}

static int createUnitAndStateAndAll(int unit, int state, int all) {
	if (unit == 1 && state == 0 && all == 0) {
		createLow(40, 41);
		createLow(42, 43);
		createLow(44, 45);
		createHigh(46, 47);
	} else if (unit == 1 && state == 1 && all == 0) {
		createLow(40, 41);
		createLow(42, 43);
		createLow(44, 45);
		createLow(46, 47);
	} else if (unit == 2 && state == 0 && all == 0) {
		createLow(40, 41);
		createLow(42, 43);
		createHigh(44, 45);
		createHigh(46, 47);
	} else if (unit == 2 && state == 1 && all == 0) {
		createLow(40, 41);
		createLow(42, 43);
		createHigh(44, 45);
		createLow(46, 47);
	} else if (unit == 3 && state == 0 && all == 0) {
		createLow(40, 41);
		createHigh(42, 43);
		createLow(44, 45);
		createHigh(46, 47);
	} else if (unit == 3 && state == 1 && all == 0) {
		createLow(40, 41);
		createHigh(42, 43);
		createLow(44, 45);
		createLow(46, 47);
	} else if (unit == 4 && state == 0 && all == 0) {
		createHigh(40, 41);
		createLow(42, 43);
		createLow(44, 45);
		createHigh(46, 47);
	} else if (unit == 4 && state == 1 && all == 0) {
		createHigh(40, 41);
		createLow(42, 43);
		createLow(44, 45);
		createLow(46, 47);
	} else if (unit == 0 && state == 0 && all == 1) {
		createHigh(40, 41);
		createHigh(42, 43);
		createHigh(44, 45);
		createLow(46, 47);
	} else if (unit == 0 && state == 1 && all == 1) {
		createHigh(40, 41);
		createHigh(42, 43);
		createLow(44, 45);
		createHigh(46, 47);
	} else {
		logprintf(LOG_ERR, "eurodomest_switch: incorrect combination of arguments");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static void createFooter(void) {
	eurodomest_switch->raw[48] = AVG_SHORT_PULSE_LENGTH;
	eurodomest_switch->raw[49] = AVG_LONG_PULSE_LENGTH;
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	int all = 0;
	int learn = -1;
	double itmp = -1;

	if (json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if (json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if (json_find_number(code, "all", &itmp)	== 0)
		all = (int)round(itmp);
	if (json_find_number(code, "off", &itmp) == 0)
		state = 0;
	else if (json_find_number(code, "on", &itmp) == 0)
		state = 1;
	if (json_find_number(code, "learn", &itmp) == 0)
		learn = 1;

	if (all > 0 && learn > -1) {
		logprintf(LOG_ERR, "eurodomest_switch: all and learn cannot be combined");
		return EXIT_FAILURE;
	} else if (id == -1 || (unit == -1 && all == 0) || state == -1) {
		logprintf(LOG_ERR, "eurodomest_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if (id > 1048575 || id < 0) {
		logprintf(LOG_ERR, "eurodomest_switch: invalid id range");
		return EXIT_FAILURE;
	} else if ((unit > 4 || unit < 1) && all == 0) {
		logprintf(LOG_ERR, "eurodomest_switch: invalid unit range");
		return EXIT_FAILURE;
	} else {
		if (unit == -1 && all == 1) {
			unit = 0;
		}
		createMessage(id, unit, state, all, learn);
		createId(id);
		if (createUnitAndStateAndAll(unit, state, all) == EXIT_FAILURE)
			return EXIT_FAILURE;
		createFooter();
		eurodomest_switch->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}


static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -a --all\t\t\tsend command to all devices with this id\n");
	printf("\t -l --learn\t\t\tsend multiple streams so switch can learn\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void eurodomestSwitchInit(void) {

	protocol_register(&eurodomest_switch);
	protocol_set_id(eurodomest_switch, "eurodomest_switch");
	protocol_device_add(eurodomest_switch, "eurodomest_switch", "Eurodomest Switches");
	eurodomest_switch->devtype = SWITCH;
	eurodomest_switch->hwtype = RF433;
	eurodomest_switch->minrawlen = RAW_LENGTH;
	eurodomest_switch->maxrawlen = RAW_LENGTH;
	eurodomest_switch->maxgaplen = MAX_LONG_PULSE_LENGTH;
	eurodomest_switch->mingaplen = MIN_LONG_PULSE_LENGTH;

	options_add(&eurodomest_switch->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&eurodomest_switch->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&eurodomest_switch->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([1-4])$");
	options_add(&eurodomest_switch->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1}|[0-9]{2}|[0-9]{3}|[0-9]{4}|[0-9]{5}|[0-9]{6}|[0-9]{7})$");
	options_add(&eurodomest_switch->options, "a", "all", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);
	options_add(&eurodomest_switch->options, "l", "learn", OPTION_NO_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);

	options_add(&eurodomest_switch->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]$");
	options_add(&eurodomest_switch->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]$");

	eurodomest_switch->parseCode = &parseCode;
	eurodomest_switch->createCode = &createCode;
	eurodomest_switch->printHelp = &printHelp;
	eurodomest_switch->validate = &validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "eurodomest_switch";
	module->version = "1.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	eurodomestSwitchInit();
}
#endif
