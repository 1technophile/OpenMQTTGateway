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
#include "zwave_ctrl.h"

static void createCommandMessage(int state) {
	zwave_ctrl->message = json_mkobject();
	if(state == 1) {
		json_append_member(zwave_ctrl->message, "command", json_mkstring("inclusion"));
	} else if(state == 0) {
		json_append_member(zwave_ctrl->message, "command", json_mkstring("exclusion"));
	} else if(state == 2) {
		json_append_member(zwave_ctrl->message, "command", json_mkstring("stop"));
	} else if(state == 3) {
		json_append_member(zwave_ctrl->message, "command", json_mkstring("soft-reset"));
	}
}

static void createParameterMessage(int nodeid, int parameter, int value) {
	zwave_ctrl->message = json_mkobject();
	json_append_member(zwave_ctrl->message, "nodeid", json_mknumber(nodeid, 0));
	json_append_member(zwave_ctrl->message, "parameter", json_mknumber(parameter, 0));
	if(value >= 0) {
		json_append_member(zwave_ctrl->message, "value", json_mknumber(value, 0));
	}
}

static int createCode(struct JsonNode *code) {
	printf("%s\n", json_stringify(code, "\t"));
	int state = -1;
	int parameter = -1;
	int value = -1;
	int nodeid = -1;
	double itmp = 0.0;

	if(json_find_number(code, "inclusion", &itmp) == 0)
		state=1;
	else if(json_find_number(code, "exclusion", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "stop", &itmp) == 0)
		state=2;
	else if(json_find_number(code, "soft-reset", &itmp) == 0)
		state=3;

	if(json_find_number(code, "parameter", &itmp) == 0)
		parameter = (int)itmp;
	if(json_find_number(code, "nodeid", &itmp) == 0)
		nodeid = (int)itmp;
	if(json_find_number(code, "value", &itmp) == 0)
		value = (int)itmp;

	if(parameter >= 0 && nodeid == -1) {
		logprintf(LOG_ERR, "zwave_ctrl: parameter needs a nodeid");
		return EXIT_FAILURE;
	}
	if(nodeid >= 0 && parameter == -1) {
		logprintf(LOG_ERR, "zwave_ctrl: nodeid needs a parameter");
		return EXIT_FAILURE;
	}
	if(value >= 0 && (parameter == -1 || nodeid == -1)) {
		logprintf(LOG_ERR, "zwave_ctrl: value needs a parameter and nodeid");
		return EXIT_FAILURE;
	}
	if(state > -1 && (parameter > -1 || nodeid > -1)) {
		logprintf(LOG_ERR, "zwave_ctrl: state and parameter and node cannot be combined");
		return EXIT_FAILURE;
	}

	if(strstr(progname, "daemon") != NULL) {	
		if(parameter >= 0 && nodeid >= 0) {
			if(value >= 0) {
				int x = snprintf(NULL, 0, "%d", value);
				zwaveSetConfigParam(nodeid, parameter, value, x);
			} else {
				zwaveGetConfigParam(nodeid, parameter);
			}
		} 
		if(state == 1) {
			zwaveStartInclusion();
		} else if(state == 0) {
			zwaveStartExclusion();
		} else if(state == 2) {
			zwaveStopCommand();
		} else if(state == 3) {
			zwaveSoftReset();
		}
	}
	if(parameter >= 0 && nodeid >= 0) {
		createParameterMessage(nodeid, parameter, value);
	} else if(state > 0) {
		createCommandMessage(state);		
	}

	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -i --inclusion\t\t\tsend an inclusion command\n");
	printf("\t -e --exclusion\t\t\tsend an exclusion command\n");
	printf("\t -s --stop\t\t\tstop previous command\n");
	printf("\t -r --soft-reset\t\tsend a soft-reset command\n");
	printf("\t -c --parameter\t\tset or retrieve parameter\n");
	printf("\t -v --value=X\t\tset parameter to this value\n");
	printf("\t -n --nodeid=X\t\tset parameter to this value\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void zwaveCtrlInit(void) {

	protocol_register(&zwave_ctrl);
	protocol_set_id(zwave_ctrl, "zwave_ctrl");
	protocol_device_add(zwave_ctrl, "zwave_ctrl", "Z-Wave Controller");
	zwave_ctrl->devtype = SWITCH;
	zwave_ctrl->hwtype = ZWAVE;
	zwave_ctrl->config = 0;

	options_add(&zwave_ctrl->options, 'i', "inclusion", OPTION_NO_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&zwave_ctrl->options, 'e', "exclusion", OPTION_NO_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&zwave_ctrl->options, 's', "stop", OPTION_NO_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&zwave_ctrl->options, 'r', "soft-reset", OPTION_NO_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&zwave_ctrl->options, 'c', "parameter", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&zwave_ctrl->options, 'v', "value", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&zwave_ctrl->options, 'n', "nodeid", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	zwave_ctrl->createCode=&createCode;
	zwave_ctrl->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "zwave_ctrl";
	module->version = "1.0";
	module->reqversion = "7.0";
	module->reqcommit = "10";
}

void init(void) {
	zwaveCtrlInit();
}
#endif
