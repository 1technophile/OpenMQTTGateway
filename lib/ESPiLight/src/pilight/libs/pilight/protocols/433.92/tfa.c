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
#include "tfa.h"

#define PULSE_MULTIPLIER	13
#define MIN_PULSE_LENGTH	220
#define MAX_PULSE_LENGTH	280	// FREETEC NC7104-675, Globaltronics GT-WT-01
#define AVG_PULSE_LENGTH	235
#define RAW_LENGTH			88
#define MIN_RAW_LENGTH		76	// SOENS, NC7104-675, GT-WT-01
#define MED_RAW_LENGTH		86	// TFA
#define MAX_RAW_LENGTH		88	// DOSTMAN 32.3200

typedef struct settings_t {
	double id;
	double channel;
	double temp;
	double humi;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(tfa->rawlen == MIN_RAW_LENGTH || tfa->rawlen == MED_RAW_LENGTH || tfa->rawlen == MAX_RAW_LENGTH) {
		if(tfa->raw[tfa->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   tfa->raw[tfa->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}
	return -1;
}

static void parseCode(void) {
	int binary[RAW_LENGTH/2];
	int temp1 = 0, temp2 = 0, temp3 = 0;
	int humi1 = 0, humi2 = 0;
	int id = 0, battery = 0, crc = 0;
	int channel = 0;
	int i = 0, x = 0, xLoop = 1;
	double humi_offset = 0.0, temp_offset = 0.0;
	double temperature = 0.0, humidity = 0.0;

	if (tfa->rawlen == MIN_RAW_LENGTH) {
		xLoop = 1;  // SOENS has 8 static pulses - binary: 1001
	}

	if (tfa->rawlen == MAX_RAW_LENGTH) {
		xLoop = 3;  // Skip the two Header Pulses of DOSTMAN 32.3200
	}

	if(tfa->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "tfa: parsecode - invalid parameter passed %d", tfa->rawlen);
		return;
	}

	for(x=xLoop;x<tfa->rawlen-2;x+=2) {
		if(tfa->raw[x] > AVG_PULSE_LENGTH*PULSE_MULTIPLIER) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	if(tfa->rawlen == MED_RAW_LENGTH || tfa->rawlen == MAX_RAW_LENGTH) {
		for(i=0;i<34;i++) {
			if(binary[i] != (crc&1)) {
				crc = (crc>>1) ^ 12;
			} else {
				crc = (crc>>1);
			}
		}
		crc ^= binToDec(binary, 34, 37);
		if (crc != binToDec(binary, 38, 41)) {
			return; // incorrect checksum
		}

		id = binToDecRev(binary, 2, 9);
		channel = binToDecRev(binary, 12, 13) + 1;

		temp1 = binToDecRev(binary, 14, 17);
		temp2 = binToDecRev(binary, 18, 21);
		temp3 = binToDecRev(binary, 22, 25);

		// Convert from °F to °C,  a zero value is equivalent to -90.00 °F with an exp of 10, we enlarge that to 2 digit
		temperature = (double)(((((temp1 + temp2*16 + temp3*256) * 10) - 9000 - 3200) * 5) / 9);

		humi1 = binToDecRev(binary, 26, 29);
		humi2 = binToDecRev(binary, 30, 33);
		humidity = (double)(humi1 + humi2*16);

		if(binToDecRev(binary, 35, 35) == 1) {
			battery = 0;
		} else {
			battery = 1;
		}

	} else {

	// must be MIN_RAW_LENGTH, we can omit it here, as validate has checked that condition already
	// SOENS has binary 1001 in the first 4 bits, if not we discard further processing of the protocol
		id = binToDecRev(binary, 0, 3);
		if(id == 9) {

			id = binToDecRev(binary, 4, 11);		// 12 - 0, 13 - Tx Button
			channel = binToDecRev(binary, 14, 15) + 1;

			temp1 = binToSignedRev(binary, 16, 27);
			temperature = (double)(temp1*10);

			humi1 = binToDecRev(binary, 28, 35);
			humidity = (double)humi1;

			if(binToDecRev(binary, 36, 36) == 1) {
				battery = 0;
			} else {
				battery = 1;
			}
		} else {
			return;
		}
	}

	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON && fabs(tmp->channel-channel) < EPSILON) {
			humi_offset = tmp->humi;
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	temperature += temp_offset;
	humidity += humi_offset;

	tfa->message = json_mkobject();
	json_append_member(tfa->message, "id", json_mknumber(id, 0));
	json_append_member(tfa->message, "temperature", json_mknumber(temperature/100, 2));
	json_append_member(tfa->message, "humidity", json_mknumber(humidity, 2));
	json_append_member(tfa->message, "battery", json_mknumber(battery, 0));
	json_append_member(tfa->message, "channel", json_mknumber(channel, 0));
}

static int checkValues(struct JsonNode *jvalues) {
	struct JsonNode *jid = NULL;

	if((jid = json_find_member(jvalues, "id"))) {
		struct settings_t *snode = NULL;
		struct JsonNode *jchild = NULL;
		struct JsonNode *jchild1 = NULL;
		double channel = -1, id = -1;
		int match = 0;

		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);
			while(jchild1) {
				if(strcmp(jchild1->key, "channel") == 0) {
					channel = jchild1->number_;
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
			if(fabs(tmp->id-id) < EPSILON && fabs(tmp->channel-channel) < EPSILON) {
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
			snode->channel = channel;
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
void tfaInit(void) {
	protocol_register(&tfa);
	protocol_set_id(tfa, "tfa");
	protocol_device_add(tfa, "tfa", "TFA weather stations");
	protocol_device_add(tfa, "conrad_weather", "Conrad Weather Stations");
	protocol_device_add(tfa, "soens", "SOENS Weather Stations");
	protocol_device_add(tfa, "NC7104", "Freetec NC7104-675 Weather Station");
	protocol_device_add(tfa, "GT-WT-01", "Globaltronics GT-WT-01 Weather Station");
	tfa->devtype = WEATHER;
	tfa->hwtype = RF433;
	tfa->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	tfa->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;
	tfa->minrawlen = MIN_RAW_LENGTH;
	tfa->maxrawlen = MAX_RAW_LENGTH;

	options_add(&tfa->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&tfa->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");
	options_add(&tfa->options, "c", "channel", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");
	options_add(&tfa->options, "h", "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "[0-9]");
	options_add(&tfa->options, "b", "battery", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[01]$");

	// options_add(&tfa->options, "0", "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&tfa->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tfa->options, "0", "humidity-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tfa->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&tfa->options, "0", "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&tfa->options, "0", "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&tfa->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&tfa->options, "0", "show-battery", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	tfa->parseCode=&parseCode;
	tfa->checkValues=&checkValues;
	tfa->validate=&validate;
	tfa->gc=&gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "tfa";
	module->version = "1.4";
	module->reqversion = "7.0";
	module->reqcommit = "84";
}

void init(void) {
	tfaInit();
}
#endif
