/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
#include "generic_dimmer.h"

static void createMessage(int id, int state, int dimlevel) {
	generic_dimmer->message = json_mkobject();
	json_append_member(generic_dimmer->message, "id", json_mknumber(id, 0));
	if(dimlevel >= 0) {
		state = 1;
		json_append_member(generic_dimmer->message, "dimlevel", json_mknumber(dimlevel, 0));
	}
	if(state == 1) {
		json_append_member(generic_dimmer->message, "state", json_mkstring("on"));
	} else {
		json_append_member(generic_dimmer->message, "state", json_mkstring("off"));
	}
}

/*
static int checkValues(struct JsonNode *code) {
	int dimlevel = -1;
	int max = 15;
	int min = 0;
	double itmp = -1;

	if(json_find_number(code, "dimlevel-maximum", &itmp) == 0)
		max = (int)round(itmp);
	if(json_find_number(code, "dimlevel-minimum", &itmp) == 0)
		min = (int)round(itmp);
	if(json_find_number(code, "dimlevel", &itmp) == 0)
		dimlevel = (int)round(itmp);

	if(min > max) {
		return 1;
	}

	if(dimlevel != -1) {
		if(dimlevel < min || dimlevel > max) {
			return 1;
		} else {
			return 0;
		}
	}
	return 0;
}
*/

static int createCode(struct JsonNode *code) {
	int id = -1;
	int state = -1;
	int dimlevel = -1;
	/*
	int max = -1;
	int min = -1;
	*/
	double itmp = -1;

	/*
	if(json_find_number(code, "dimlevel-maximum", &itmp) == 0)
		max = (int)round(itmp);
	if(json_find_number(code, "dimlevel-minimum", &itmp) == 0)
		min = (int)round(itmp);
	*/

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "dimlevel", &itmp) == 0)
		dimlevel = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(id == -1 || (dimlevel == -1 && state == -1)) {
		logprintf(LOG_ERR, "generic_dimmer: insufficient number of arguments");
		return EXIT_FAILURE;
	/*
	} else if(dimlevel != -1 && ((max != -1 && dimlevel > max) || (min != -1 && dimlevel < min))) {
		logprintf(LOG_ERR, "generic_dimmer: invalid dimlevel range");
		return EXIT_FAILURE;
	*/
	} else if(dimlevel >= 0 && state == 0) {
		logprintf(LOG_ERR, "generic_dimmer: dimlevel and state cannot be combined");
		return EXIT_FAILURE;
	} else {
		if(dimlevel >= 0) {
			state = -1;
		}
		createMessage(id, state, dimlevel);
	}
	return EXIT_SUCCESS;

}

static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -d --dimlevel=dimlevel\t\tsend a specific dimlevel\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void genericDimmerInit(void) {

	protocol_register(&generic_dimmer);
	protocol_set_id(generic_dimmer, "generic_dimmer");
	protocol_device_add(generic_dimmer, "generic_dimmer", "Generic Dimmers");
	generic_dimmer->devtype = DIMMER;

	options_add(&generic_dimmer->options, 'd', "dimlevel", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^([0-9]{1,})$");
	options_add(&generic_dimmer->options, 't', "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&generic_dimmer->options, 'f', "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&generic_dimmer->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,})$");

	options_add(&generic_dimmer->options, 0, "dimlevel-minimum", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^([0-9]{1}|[1][0-5])$");
	options_add(&generic_dimmer->options, 0, "dimlevel-maximum", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)15, "^([0-9]{1}|[1][0-5])$");

	options_add(&generic_dimmer->options, 0, "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&generic_dimmer->options, 0, "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	generic_dimmer->printHelp=&printHelp;
	generic_dimmer->createCode=&createCode;
	/*
	generic_dimmer->checkValues=&checkValues;
	*/
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "generic_dimmer";
	module->version = "2.0";
	module->reqversion = "7.0";
	module->reqcommit = "94";
}

void init(void) {
	genericDimmerInit();
}
#endif
