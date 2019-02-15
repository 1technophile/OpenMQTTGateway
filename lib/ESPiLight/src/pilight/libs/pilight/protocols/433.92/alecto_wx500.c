/*
	Copyright (C) 2014 CurlyMo & Tommybear1979

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
#include "alecto_wx500.h"
//
// Protocol characteristics: SYNC bit: 526/8942, Logical 0: 526/1578, Logical 1: 526/3419
//
#define PULSE_MULTIPLIER	16
#define MIN_PULSE_LENGTH	235
#define AVG_PULSE_LENGTH	255
#define MAX_PULSE_LENGTH	275
#define ZERO_PULSE				2104
#define ONE_PULSE					3945
#define AVG_PULSE					(ZERO_PULSE+ONE_PULSE)/2
#define RAW_LENGTH				74

typedef struct settings_t {
	double id;
	double temp;
	double humi;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(alecto_wx500->rawlen == RAW_LENGTH) {
		if(alecto_wx500->raw[alecto_wx500->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   alecto_wx500->raw[alecto_wx500->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}
	return -1;
}

static void parseCode(void) {
	int i = 0, x = 0, type = 0, id = 0, binary[RAW_LENGTH/2];
	double temp_offset = 0.0, humi_offset = 0.0;
	double humidity = 0.0, temperature = 0.0;
	int winddir = 0, windavg = 0, windgust = 0;
	int /*rain = 0, */battery = 0;
	int n0 = 0, n1 = 0, n2 = 0, n3 = 0;
	int n4 = 0, n5 = 0, n6 = 0, n7 = 0, n8 = 0;
	int checksum = 1;

	if(alecto_wx500->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "alecto_wx500: parsecode - invalid parameter passed %d", alecto_wx500->rawlen);
		return;
	}

	for(x=1;x<alecto_wx500->rawlen;x+=2) {
		if(alecto_wx500->raw[x] > AVG_PULSE) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	n8=binToDec(binary, 32, 35);
	n7=binToDec(binary, 28, 31);
	n6=binToDec(binary, 24, 27);
	n5=binToDec(binary, 20, 23);
	n4=binToDec(binary, 16, 19);
	n3=binToDec(binary, 12, 15);
	n2=binToDec(binary, 8, 11);
	n1=binToDec(binary, 4, 7);
	n0=binToDec(binary, 0, 3);

	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON){
			humi_offset = tmp->humi;
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	if((n2 & 0x6) != 0x6) {
		type = 0x1;
		checksum = (0xf-n0-n1-n2-n3-n4-n5-n6-n7) & 0xf;
		if(n8 != checksum) {
			type=0x5;
			return;
		}
	//Wind average * 0.2
	} else if(n3 == 0x1) {
		type = 0x2;
		checksum = (0xf-n0-n1-n2-n3-n4-n5-n6-n7) & 0xf;
		if(n8 != checksum){
			type=0x5;
			return;
		}
	//Wind direction & gust
	} else if((n3 & 0x7) == 0x7) {
		type = 0x3;
		checksum = (0xf-n0-n1-n2-n3-n4-n5-n6-n7) & 0xf;
		if(n8 != checksum) {
			type=0x5;
			return;
		}
	//Rain
	} else if(n3 == 0x3)	{
		type = 0x4;
		checksum = (0x7+n0+n1+n2+n3+n4+n5+n6+n7) & 0xf;
		if(n8 != checksum){
			type = 0x5;
			return;
		}
	//Catch
	} else 	{
		type = 0x5;
		return;
	}

	alecto_wx500->message = json_mkobject();
	switch(type) {
		case 1:
			id = binToDec(binary, 0, 7);
			temperature = (double)(binToSigned(binary, 12, 23)) / 10.0;
			humidity = (binToDec(binary, 28, 31) * 10) + binToDec(binary, 24,27);
			battery = !binary[8];

			temperature += temp_offset;
			humidity += humi_offset;

			json_append_member(alecto_wx500->message, "id", json_mknumber(id, 0));
			json_append_member(alecto_wx500->message, "temperature", json_mknumber(temperature, 1));
			json_append_member(alecto_wx500->message, "humidity", json_mknumber(humidity, 1));
			json_append_member(alecto_wx500->message, "battery", json_mknumber(battery, 0));
		break;
		case 2:
			id = binToDec(binary, 0, 7);
			windavg = binToDec(binary, 24, 31) * 2;
			battery = !binary[8];

			json_append_member(alecto_wx500->message, "id", json_mknumber(id, 0));
			json_append_member(alecto_wx500->message, "windavg", json_mknumber((double)windavg/10, 1));
			json_append_member(alecto_wx500->message, "battery", json_mknumber(battery, 0));
		break;
		case 3:
			id = binToDec(binary, 0, 7);
			winddir = binToDec(binary, 15, 23);
			windgust = binToDec(binary, 24, 31) * 2;
			battery = !binary[8];

			json_append_member(alecto_wx500->message, "id", json_mknumber(id, 0));
			json_append_member(alecto_wx500->message, "winddir", json_mknumber((double)winddir, 0));
			json_append_member(alecto_wx500->message, "windgust", json_mknumber((double)windgust/10, 1));
			json_append_member(alecto_wx500->message, "battery", json_mknumber(battery, 0));
		break;
		case 4:
			id = binToDec(binary, 0, 7);
			/*rain = binToDec(binary, 16, 30) * 5;*/
			battery = !binary[8];
			//json_append_member(alecto_wx500->message, "rain", json_mknumber((double)rain/10, 1));
			json_append_member(alecto_wx500->message, "id", json_mknumber(id, 0));
			json_append_member(alecto_wx500->message, "battery", json_mknumber(battery, 0));
		break;
		default:
			type=0x5;
			json_delete(alecto_wx500->message);
			alecto_wx500->message = NULL;
			return;
		break;
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
void alectoWX500Init(void) {

	protocol_register(&alecto_wx500);
	protocol_set_id(alecto_wx500, "alecto_wx500");
	protocol_device_add(alecto_wx500, "alecto_wx500", "Alecto WX500 Weather Stations");
	protocol_device_add(alecto_wx500, "auriol_h13726", "Auriol H13726 Weather Stations");
	protocol_device_add(alecto_wx500, "ventus_wsxxx", "Ventus WSXXX Weather Stations");
	protocol_device_add(alecto_wx500, "hama_ews1500", "Hama EWS1500 Weather Stations");
	protocol_device_add(alecto_wx500, "meteoscan_w1XX", "Meteoscan W1XXX Weather Stations");
	protocol_device_add(alecto_wx500, "balance_rf_ws105", "Balance RF-WS105 Weather Stations");
	alecto_wx500->devtype = WEATHER;
	alecto_wx500->hwtype = RF433;
	alecto_wx500->minrawlen = RAW_LENGTH;
	alecto_wx500->maxrawlen = RAW_LENGTH;
	alecto_wx500->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	alecto_wx500->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&alecto_wx500->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&alecto_wx500->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");
	options_add(&alecto_wx500->options, "b", "battery", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "[0-9]");
	options_add(&alecto_wx500->options, "h", "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&alecto_wx500->options, "w", "windavg", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&alecto_wx500->options, "d", "winddir", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&alecto_wx500->options, "g", "windgust", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	//options_add(&alecto_wx500->options, "r", "rain", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");

	// options_add(&alecto_wx500->options, "0", "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&alecto_wx500->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&alecto_wx500->options, "0", "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&alecto_wx500->options, "0", "wind-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&alecto_wx500->options, "0", "humidity-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&alecto_wx500->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&alecto_wx500->options, "0", "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&alecto_wx500->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&alecto_wx500->options, "0", "show-battery", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&alecto_wx500->options, "0", "show-wind", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	//options_add(&alecto_wx500->options, "0", "show-rain", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	alecto_wx500->parseCode=&parseCode;
	alecto_wx500->checkValues=&checkValues;
	alecto_wx500->validate=&validate;
	alecto_wx500->gc=&gc;
}

#ifdef MODULAR
void compatibility(const char **version, const char **commit) {
	module->name = "alecto_wx500";
	module->version = "1.2";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	alectoWX500Init();
}
#endif
