/*
	Copyright (C) 2013 CurlyMo

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
#include "generic_weather.h"

static void createMessage(int id, double temperature, double humidity, int battery) {
	generic_weather->message = json_mkobject();
	json_append_member(generic_weather->message, "id", json_mknumber(id, 0));
	if(temperature > -998.0) {
		json_append_member(generic_weather->message, "temperature", json_mknumber(temperature, 2));
	}
	if(humidity > -1.0) {
		json_append_member(generic_weather->message, "humidity", json_mknumber(humidity, 2));
	}
	if(battery > -1) {
		json_append_member(generic_weather->message, "battery", json_mknumber(battery, 0));
	}
}

static int createCode(JsonNode *code) {
	double itmp = 0;
	int id = -999;
	double temp = -999.0;
	double humi = -1.0;
	int batt = -1;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "temperature", &itmp) == 0)
		temp = itmp;
	if(json_find_number(code, "humidity", &itmp) == 0)
		humi = itmp;
	if(json_find_number(code, "battery", &itmp) == 0)
		batt = (int)round(itmp);

	if(id == -999 && temp < -998.0 && humi < 0.0 && batt == -1) {
		logprintf(LOG_ERR, "generic_weather: insufficient number of arguments");
		return EXIT_FAILURE;
	} else {
		createMessage(id, temp, humi, batt);
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -t --temperature=temperature\tset the temperature\n");
	printf("\t -h --humidity=humidity\t\tset the humidity\n");
	printf("\t -b --battery=battery\t\tset the battery level\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void genericWeatherInit(void) {

	protocol_register(&generic_weather);
	protocol_set_id(generic_weather, "generic_weather");
	protocol_device_add(generic_weather, "generic_weather", "Generic Weather Stations");
	generic_weather->devtype = WEATHER;

	options_add(&generic_weather->options, 'h', "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "[0-9]");
	options_add(&generic_weather->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "[0-9]");
	options_add(&generic_weather->options, 'b', "battery", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[01]$");
	options_add(&generic_weather->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");

	// options_add(&generic_weather->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&generic_weather->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&generic_weather->options, 0, "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&generic_weather->options, 0, "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&generic_weather->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&generic_weather->options, 0, "show-battery", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	generic_weather->printHelp=&printHelp;
	generic_weather->createCode=&createCode;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "generic_weather";
	module->version = "1.22";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	genericWeatherInit();
}
#endif
