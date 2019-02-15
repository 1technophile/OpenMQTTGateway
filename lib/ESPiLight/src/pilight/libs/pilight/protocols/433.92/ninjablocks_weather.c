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
#include "ninjablocks_weather.h"

#define PULSE_MULTIPLIER	2
#define MIN_PULSE_LENGTH	2115
#define MAX_PULSE_LENGTH	2125
#define AVG_PULSE_LENGTH	2120
#define MIN_RAW_LENGTH		41
#define MAX_RAW_LENGTH		70
#define RAW_LENGTH			50

#define PULSE_NINJA_WEATHER_SHORT	1000
#define PULSE_NINJA_WEATHER_LONG		2000
#define PULSE_NINJA_WEATHER_FOOTER	AVG_PULSE_LENGTH	// 72080/PULSE_DIV
#define PULSE_NINJA_WEATHER_LOWER	750	// SHORT*0,75
#define PULSE_NINJA_WEATHER_UPPER	1250	// SHORT * 1,25

typedef struct settings_t {
	double id;
	double unit;
	double temp;
	double humi;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(ninjablocks_weather->rawlen >= MIN_RAW_LENGTH && ninjablocks_weather->rawlen <= MAX_RAW_LENGTH) {
		if(ninjablocks_weather->raw[ninjablocks_weather->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   ninjablocks_weather->raw[ninjablocks_weather->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, double temperature, double humidity) {
	ninjablocks_weather->message = json_mkobject();
	json_append_member(ninjablocks_weather->message, "id", json_mknumber(id, 0));
	json_append_member(ninjablocks_weather->message, "unit", json_mknumber(unit, 0));
	json_append_member(ninjablocks_weather->message, "temperature", json_mknumber(temperature/100, 2));
	json_append_member(ninjablocks_weather->message, "humidity", json_mknumber(humidity, 0));
}

static void parseCode(void) {
	int x = 0, pRaw = 0, binary[MAX_RAW_LENGTH/2];
	int iParity = 1, iParityData = -1;	// init for even parity
	int iHeaderSync = 12;				// 1100
	int iDataSync = 6;					// 110
	double temp_offset = 0.0;
	double humi_offset = 0.0;

	if(ninjablocks_weather->rawlen>MAX_RAW_LENGTH) {
		logprintf(LOG_ERR, "ninjablocks_weather: parsecode - invalid parameter passed %d", ninjablocks_weather->rawlen);
		return;
	}

	// Decode Biphase Mark Coded Differential Manchester (BMCDM) pulse stream into binary
	for(x=0; x<=(MAX_RAW_LENGTH/2); x++) {
		if(ninjablocks_weather->raw[pRaw] > PULSE_NINJA_WEATHER_LOWER &&
		  ninjablocks_weather->raw[pRaw] < PULSE_NINJA_WEATHER_UPPER) {
			binary[x] = 1;
			iParityData = iParity;
			iParity = -iParity;
			pRaw++;
		} else {
			binary[x] = 0;
		}
		pRaw++;
	}
	if(iParityData < 0) {
		iParityData = 0;
	}

	// Binary record: 0-3 sync0, 4-7 unit, 8-9 id, 10-12 sync1, 13-19 humidity, 20-34 temperature, 35 even par, 36 footer
	int headerSync = binToDecRev(binary, 0,3);
	int unit = binToDecRev(binary, 4,7);
	int id = binToDecRev(binary, 8,9);
	int dataSync = binToDecRev(binary, 10,12);
	double humidity = binToDecRev(binary, 13,19);	// %
	double temperature = binToDecRev(binary, 20,34);
	// ((temp * (100 / 128)) - 5000) * 10 °C, 2 digits
	temperature = ((int)((double)(temperature * 0.78125)) - 5000);

	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON && fabs(tmp->unit-unit) < EPSILON) {
			humi_offset = tmp->humi;
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	temperature += temp_offset;
	humidity += humi_offset;

	if(iParityData == 0 && (iHeaderSync == headerSync || dataSync == iDataSync)) {
		createMessage(id, unit, temperature, humidity);
	}
}

static int checkValues(struct JsonNode *jvalues) {
	struct JsonNode *jid = NULL;

	if((jid = json_find_member(jvalues, "id"))) {
		struct settings_t *snode = NULL;
		struct JsonNode *jchild = NULL;
		struct JsonNode *jchild1 = NULL;
		double unit = -1, id = -1;
		int match = 0;

		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);
			while(jchild1) {
				if(strcmp(jchild1->key, "unit") == 0) {
					unit = jchild1->number_;
				}
				if(strcmp(jchild1->key, "id") == 0) {
					id = jchild1->number_;
				}
				jchild1 = jchild1->next;
			}
			jchild = jchild->next;
		}

		struct settings_t *tmp = settings;
		while(tmp) {
			if(fabs(tmp->id-id) < EPSILON && fabs(tmp->unit-unit) < EPSILON) {
				match = 1;
				break;
			}
			tmp = tmp->next;
		}

		if(match == 0) {
			if((snode = MALLOC(sizeof(struct settings_t))) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			snode->id = id;
			snode->unit = unit;
			snode->temp = 0;
			snode->humi = 0;

			json_find_number(jvalues, "temperature-offset", &snode->temp);
			json_find_number(jvalues, "humidity-offset", &snode->humi);

			snode->next = settings;
			settings = snode;
		}
	}
	return 0;
}

static void gc(void) {
	struct settings_t *tmp = NULL;
	while(settings) {
		tmp = settings;
		settings = settings->next;
		FREE(tmp);
	}
	if(settings != NULL) {
		FREE(settings);
	}
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void ninjablocksWeatherInit(void) {

	protocol_register(&ninjablocks_weather);
	protocol_set_id(ninjablocks_weather, "ninjablocks_weather");
	protocol_device_add(ninjablocks_weather, "ninjablocks_weather", "Ninjablocks Weather Sensors");
	ninjablocks_weather->devtype = WEATHER;
	ninjablocks_weather->hwtype = RF433;
	ninjablocks_weather->minrawlen = MIN_RAW_LENGTH;
	ninjablocks_weather->maxrawlen = MAX_RAW_LENGTH;
	ninjablocks_weather->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;
	ninjablocks_weather->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;

	// sync-id[4]; Homecode[4], Channel Code[2], Sync[3], Humidity[7], Temperature[15], Footer [1]
	options_add(&ninjablocks_weather->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]|1[0-5])$");
	options_add(&ninjablocks_weather->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-3])$");
	options_add(&ninjablocks_weather->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&ninjablocks_weather->options, "h", "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");

	// options_add(&ninjablocks_weather->options, "0", "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&ninjablocks_weather->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&ninjablocks_weather->options, "0", "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&ninjablocks_weather->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&ninjablocks_weather->options, "0", "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&ninjablocks_weather->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	ninjablocks_weather->parseCode=&parseCode;
	ninjablocks_weather->checkValues=&checkValues;
	ninjablocks_weather->validate=&validate;
	ninjablocks_weather->gc=&gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "ninjablocks_weather";
	module->version = "1.2";
	module->reqversion = "7.0";
	module->reqcommit = "84";
}

void init(void) {
	ninjablocksWeatherInit();
}
#endif

