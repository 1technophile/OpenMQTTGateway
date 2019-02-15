/*
	Copyright (C) 2016 Puuu

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
#include "tcm.h"

#define PULSE_MULTIPLIER	8
#define MIN_PULSE_LENGTH	220
#define MAX_PULSE_LENGTH	250
#define AVG_PULSE_LENGTH	235
#define RAW_LENGTH		74

/*
	protocol description: http://forum.arduino.cc/index.php?topic=136836.0 (german)
	Bit
	1-8	sensor id, will change on battery exchange
	9	low battery indicator
	12	1 indicates TX button press on sensor
	17-24	humidity
	25-36	temperature (signed int12)
*/

typedef struct settings_t {
	double id;
	double temp;
	double humi;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(tcm->rawlen == RAW_LENGTH) {
		if(tcm->raw[tcm->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   tcm->raw[tcm->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void parseCode(void) {
	double humi_offset = 0.0, temp_offset = 0.0;
	double temperature = 0.0, humidity = 0.0;
	int binary[RAW_LENGTH/2];
	int id = 0, button = 0, battery = 0;
	int i = 0, x = 0;

	if(tcm->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "tcm: parsecode - invalid parameter passed %d", tcm->rawlen);
		return;
	}

	for(x=1;x<tcm->rawlen-2;x+=2) {
		if(tcm->raw[x] > AVG_PULSE_LENGTH*PULSE_MULTIPLIER) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	id = binToDecRev(binary, 0, 7);
	battery = !binary[8];
	button = binary[11];

	humidity = binToDecRev(binary, 16, 23);

	temperature = binToSignedRev(binary, 24, 35);

	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON) {
			humi_offset = tmp->humi;
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	temperature += temp_offset;
	humidity += humi_offset;

	tcm->message = json_mkobject();
	json_append_member(tcm->message, "id", json_mknumber(id, 0));
	json_append_member(tcm->message, "temperature", json_mknumber(temperature/10, 1));
	json_append_member(tcm->message, "humidity", json_mknumber(humidity, 0));
	json_append_member(tcm->message, "battery", json_mknumber(battery, 0));
	json_append_member(tcm->message, "button", json_mknumber(button, 0));
}

static int checkValues(struct JsonNode *jvalues) {
	struct JsonNode *jid = NULL;

	if((jid = json_find_member(jvalues, "id"))) {
		struct settings_t *snode = NULL;
		struct JsonNode *jchild = NULL;
		struct JsonNode *jchild1 = NULL;
		double id = -1;
		int match = 0;

		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);
			while(jchild1) {
				if(strcmp(jchild1->key, "id") == 0) {
					id = jchild1->number_;
				}
				jchild1 = jchild1->next;
			}
			jchild = jchild->next;
		}

		struct settings_t *tmp = settings;
		while(tmp) {
			if(fabs(tmp->id-id) < EPSILON) {
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
void tcmInit(void) {
	protocol_register(&tcm);
	protocol_set_id(tcm, "tcm");
	protocol_device_add(tcm, "tcm", "TCM 218943 weather stations");
	tcm->devtype = WEATHER;
	tcm->hwtype = RF433;
	tcm->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	tcm->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;
	tcm->minrawlen = RAW_LENGTH;
	tcm->maxrawlen = RAW_LENGTH;

	options_add(&tcm->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&tcm->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");
	options_add(&tcm->options, "h", "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "[0-9]");
	options_add(&tcm->options, "b", "battery", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[01]$");

	options_add(&tcm->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tcm->options, "0", "humidity-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tcm->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&tcm->options, "0", "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&tcm->options, "0", "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&tcm->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&tcm->options, "0", "show-battery", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	tcm->parseCode=&parseCode;
	tcm->checkValues=&checkValues;
	tcm->validate=&validate;
	tcm->gc=&gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "tcm";
	module->version = "1.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	tcmInit();
}
#endif
