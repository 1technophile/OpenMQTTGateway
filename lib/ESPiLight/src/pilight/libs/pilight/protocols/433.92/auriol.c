/*
	Copyright (C) 2014 Bram1337 & CurlyMo

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
#include "auriol.h"

#define PULSE_MULTIPLIER	13
#define MIN_PULSE_LENGTH	264
#define MAX_PULSE_LENGTH	274
#define AVG_PULSE_LENGTH	269
#define RAW_LENGTH				66

typedef struct settings_t {
	double id;
	double temp;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(auriol->rawlen == RAW_LENGTH) {
		if(auriol->raw[auriol->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   auriol->raw[auriol->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void parseCode(void) {
	int i = 0, x = 0, binary[RAW_LENGTH/2];
	int channel = 0, id = 0, battery = 0;
	double temp_offset = 0.0, temperature = 0.0;

	if(auriol->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "auriol: parsecode - invalid parameter passed %d", auriol->rawlen);
		return;
	}

	for(x=1;x<auriol->rawlen-2;x+=2) {
		if(auriol->raw[x] > AVG_PULSE_LENGTH*PULSE_MULTIPLIER) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	id = binToDecRev(binary, 0, 7);
	battery = binary[8];
	channel = 1 + binToDecRev(binary, 10, 11); // channel as id
	temperature = (double)binToSignedRev(binary, 12, 23)/10;
	// checksum = (double)binToDecRev(binary, 24, 31); been unable to deciper it
	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON) {
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	temperature += temp_offset;

	if(channel != 4) {
		auriol->message = json_mkobject();
		json_append_member(auriol->message, "id", json_mknumber(id, 0));
		json_append_member(auriol->message, "temperature", json_mknumber(temperature, 1));
		json_append_member(auriol->message, "battery", json_mknumber(battery, 0));
		json_append_member(auriol->message, "channel", json_mknumber(channel, 0));
	}
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

			json_find_number(jvalues, "temperature-offset", &snode->temp);

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
void auriolInit(void) {

	protocol_register(&auriol);
	protocol_set_id(auriol, "auriol");
	protocol_device_add(auriol, "auriol", "Auriol Weather Stations");
	auriol->devtype = WEATHER;
	auriol->hwtype = RF433;
	auriol->minrawlen = RAW_LENGTH;
	auriol->maxrawlen = RAW_LENGTH;
	auriol->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	auriol->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&auriol->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");
	options_add(&auriol->options, "c", "channel", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[1-3]");
	options_add(&auriol->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&auriol->options, "b", "battery", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[01]$");

	// options_add(&auriol->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&auriol->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&auriol->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&auriol->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&auriol->options, "0", "show-battery", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	auriol->parseCode=&parseCode;
	auriol->checkValues=&checkValues;
	auriol->validate=&validate;
	auriol->gc=&gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "auriol";
	module->version = "2.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	auriolInit();
}
#endif
