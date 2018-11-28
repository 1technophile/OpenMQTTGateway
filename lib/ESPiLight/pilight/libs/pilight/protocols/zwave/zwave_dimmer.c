/*
	Copyright (C) 2015 CurlyMo

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
#
#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../../hardware/zwave.h"
#include "../protocol.h"
#include "zwave_dimmer.h"

static void createDimMessage(unsigned int homeId, int nodeId, int dimlevel) {
	zwave_dimmer->message = json_mkobject();
	json_append_member(zwave_dimmer->message, "homeId", json_mknumber(homeId, 0));
	json_append_member(zwave_dimmer->message, "nodeId", json_mknumber(nodeId, 0));
	if(dimlevel > 0) {
		json_append_member(zwave_dimmer->message, "state", json_mkstring("on"));
	} else {
		json_append_member(zwave_dimmer->message, "state", json_mkstring("off"));
	}
	json_append_member(zwave_dimmer->message, "dimlevel", json_mknumber(dimlevel, 0));
}

static void createConfigMessage(unsigned int homeId, int nodeId, char *label, int value) {
	zwave_dimmer->message = json_mkobject();
	json_append_member(zwave_dimmer->message, "homeId", json_mknumber(homeId, 0));
	json_append_member(zwave_dimmer->message, "nodeId", json_mknumber(nodeId, 0));
	json_append_member(zwave_dimmer->message, "label", json_mkstring(label));
	json_append_member(zwave_dimmer->message, "value", json_mknumber(value, 0));
}

static void parseCommand(struct JsonNode *code) {
	struct JsonNode *message = NULL;
	char *label = NULL;
	double itmp = 0.0;
	int nodeId = 0, value = 0, cmdId = 0;
	unsigned int homeId = 0;

	if((message = json_find_member(code, "message")) != NULL) {
		if(json_find_number(message, "nodeId", &itmp) == 0) {
			nodeId = (int)round(itmp);
		} else {
			return;
		}
		if(json_find_number(message, "homeId", &itmp) == 0) {
			homeId = (unsigned int)round(itmp);
		} else {
			return;
		}
		if(json_find_number(message, "cmdId", &itmp) == 0) {
			cmdId = (int)round(itmp);
		} else {
			return;
		}
		if(json_find_number(message, "value", &itmp) == 0) {
			value = (int)round(itmp);
		} else {
			return;
		}
		json_find_string(message, "label", &label);
	}

	if(cmdId == COMMAND_CLASS_SWITCH_MULTILEVEL) {
		createDimMessage(homeId, nodeId, value);
	}
	if(cmdId == COMMAND_CLASS_CONFIGURATION && label != NULL) {
		createConfigMessage(homeId, nodeId, label, value);
	}
}

static int createCode(struct JsonNode *code) {
	char out[255];
	unsigned int homeId = 0;
	int nodeId = 0;
	int dimlevel = -1;
	char state = 0;
	double itmp = 0.0;

	if(json_find_number(code, "homeId", &itmp) == 0)
		homeId = (unsigned int)round(itmp);
	if(json_find_number(code, "nodeId", &itmp) == 0)
		nodeId = (int)round(itmp);
	if(json_find_number(code, "dimlevel", &itmp) == 0)
		dimlevel = (int)round(itmp);	

	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(dimlevel > -1) {
		state = 1;
		if(dimlevel > 100) {
			dimlevel = 99;
		}
	} else if(state == 0) {
		dimlevel = 0;
	} else if(state == 1) {
		dimlevel = 255;
	}

	if(dimlevel > -1) {
		snprintf(out, 255, "%d", dimlevel);
		zwaveSetValue(nodeId, COMMAND_CLASS_SWITCH_MULTILEVEL, "Level", out);
		createDimMessage(homeId, nodeId, dimlevel);
	}

	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -h --homeid=id\t\t\tcontrol a device with this home id\n");
	printf("\t -i --unitid=id\t\t\tcontrol a device with this unit id\n");
	printf("\t -d --dimlevel=dimlevel\t\tsend a specific dimlevel\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void zwaveDimmerInit(void) {

	protocol_register(&zwave_dimmer);
	protocol_set_id(zwave_dimmer, "zwave_dimmer");
	protocol_device_add(zwave_dimmer, "zwave_dimmer", "Z-Wave Dimmers");
	zwave_dimmer->devtype = DIMMER;
	zwave_dimmer->hwtype = ZWAVE;

	options_add(&zwave_dimmer->options, 'h', "homeId", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&zwave_dimmer->options, 'i', "nodeId", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&zwave_dimmer->options, 't', "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&zwave_dimmer->options, 'f', "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&zwave_dimmer->options, 'd', "dimlevel", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	options_add(&zwave_dimmer->options, 0, "dimlevel-minimum", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, NULL);
	options_add(&zwave_dimmer->options, 0, "dimlevel-maximum", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)99, NULL);	
	
	zwave_dimmer->createCode=&createCode;
	zwave_dimmer->parseCommand=&parseCommand;
	zwave_dimmer->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "zwave_dimmer";
	module->version = "1.0";
	module->reqversion = "7.0";
	module->reqcommit = "10";
}

void init(void) {
	zwaveDimmerInit();
}
#endif
