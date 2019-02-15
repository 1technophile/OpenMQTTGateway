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
#include "alecto_wsd17.h"

#define PULSE_MULTIPLIER	14
#define MIN_PULSE_LENGTH	265
#define AVG_PULSE_LENGTH	270
#define MAX_PULSE_LENGTH	275
#define RAW_LENGTH				74

typedef struct settings_t {
	double id;
	double temp;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(alecto_wsd17->rawlen == RAW_LENGTH) {
		if(alecto_wsd17->raw[alecto_wsd17->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   alecto_wsd17->raw[alecto_wsd17->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void parseCode(void) {
	int i = 0, x = 0, id = 0, binary[RAW_LENGTH/2];
	double temp_offset = 0.0, temperature = 0.0;

	if(alecto_wsd17->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "alecto_wsd17: parsecode - invalid parameter passed %d", alecto_wsd17->rawlen);
		return;
	}

	for(x=1;x<alecto_wsd17->rawlen-1;x+=2) {
		if(alecto_wsd17->raw[x] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	id = binToDecRev(binary, 0, 11);
	temperature = binToDecRev(binary, 16, 27);

	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON) {
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	temperature += temp_offset;

	alecto_wsd17->message = json_mkobject();
	json_append_member(alecto_wsd17->message, "id", json_mknumber(id, 0));
	json_append_member(alecto_wsd17->message, "temperature", json_mknumber(temperature/10, 1));
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
void alectoWSD17Init(void) {
	protocol_register(&alecto_wsd17);
	protocol_set_id(alecto_wsd17, "alecto_wsd17");
	protocol_device_add(alecto_wsd17, "alecto_wsd17", "Alecto WSD-17 Weather Stations");
	alecto_wsd17->devtype = WEATHER;
	alecto_wsd17->hwtype = RF433;
	alecto_wsd17->minrawlen = RAW_LENGTH;
	alecto_wsd17->maxrawlen = RAW_LENGTH;
	alecto_wsd17->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	alecto_wsd17->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&alecto_wsd17->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&alecto_wsd17->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");

	// options_add(&alecto_wsd17->options, "0", "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&alecto_wsd17->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&alecto_wsd17->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&alecto_wsd17->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	alecto_wsd17->parseCode=&parseCode;
	alecto_wsd17->checkValues=&checkValues;
	alecto_wsd17->validate=&validate;
	alecto_wsd17->gc=&gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "alecto_wsd17";
	module->version = "1.1";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	alectoWSD17Init();
}
#endif
