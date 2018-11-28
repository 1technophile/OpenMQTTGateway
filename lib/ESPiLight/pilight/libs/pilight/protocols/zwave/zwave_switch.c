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
#include "zwave_switch.h"

static void createStateMessage(unsigned int homeId, int nodeId, int state) {
	zwave_switch->message = json_mkobject();
	json_append_member(zwave_switch->message, "homeId", json_mknumber(homeId, 0));
	json_append_member(zwave_switch->message, "nodeId", json_mknumber(nodeId, 0));
	if(state == 1) {
		json_append_member(zwave_switch->message, "state", json_mkstring("on"));
	} else {
		json_append_member(zwave_switch->message, "state", json_mkstring("off"));
	}
}

static void createConfigMessage(unsigned int homeId, int nodeId, char *label, int value) {
	zwave_switch->message = json_mkobject();
	json_append_member(zwave_switch->message, "homeId", json_mknumber(homeId, 0));
	json_append_member(zwave_switch->message, "nodeId", json_mknumber(nodeId, 0));
	json_append_member(zwave_switch->message, "label", json_mkstring(label));
	json_append_member(zwave_switch->message, "value", json_mknumber(value, 0));
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
	if(cmdId == COMMAND_CLASS_SWITCH_BINARY) {
		createStateMessage(homeId, nodeId, value);
	}
	if(cmdId == COMMAND_CLASS_CONFIGURATION && label != NULL) {
		createConfigMessage(homeId, nodeId, label, value);
	}
}

static int createCode(struct JsonNode *code) {
	unsigned int homeId = 0;
	int nodeId = 0;
	char state = 0;
	double itmp = 0.0;

	if(json_find_number(code, "homeId", &itmp) == 0)
		homeId = (unsigned int)round(itmp);
	if(json_find_number(code, "nodeId", &itmp) == 0)
		nodeId = (int)round(itmp);

	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(state == 0) {
		zwaveSetValue(nodeId, COMMAND_CLASS_SWITCH_BINARY, NULL, "false");
	} else {
		zwaveSetValue(nodeId, COMMAND_CLASS_SWITCH_BINARY, NULL, "true");
	}
	
	createStateMessage(homeId, nodeId, state);		

	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -h --homeid=id\t\t\tcontrol a device with this home id\n");
	printf("\t -i --unitid=id\t\t\tcontrol a device with this unit id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void zwaveSwitchInit(void) {

	protocol_register(&zwave_switch);
	protocol_set_id(zwave_switch, "zwave_switch");
	protocol_device_add(zwave_switch, "zwave_switch", "Z-Wave Switches");
	zwave_switch->devtype = SWITCH;
	zwave_switch->hwtype = ZWAVE;

	options_add(&zwave_switch->options, 'h', "homeId", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&zwave_switch->options, 'i', "nodeId", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&zwave_switch->options, 't', "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&zwave_switch->options, 'f', "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	zwave_switch->createCode=&createCode;
	zwave_switch->parseCommand=&parseCommand;
	zwave_switch->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "zwave_switch";
	module->version = "1.0";
	module->reqversion = "7.0";
	module->reqcommit = "10";
}

void init(void) {
	zwaveSwitchInit();
}
#endif
