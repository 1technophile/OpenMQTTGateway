/*
	Copyright (C) 2014 CurlyMo & DonBernos

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

// adapted for TFA 30.3125, 30.3120.90, 30.3120.30 and 30.3121 by DonBernos

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
#include "tfa30.h"
//
// Protocol characteristics: High signal length between 300 and 1500 ms
//
#define PULSE_MULTIPLIER	16
#define MIN_PULSE_LENGTH	734 // min zero signal after sequence (..x 34)us
#define AVG_PULSE_LENGTH	882 //
#define MAX_PULSE_LENGTH	1030 // max zero signal after sequence(..x 34)us
#define ZERO_PULSE  			1300
#define ONE_PULSE   			300
#define AVG_PULSE   			800       //limit puls length for binary analyses
#define MIN_RAW_LENGTH		80
#define MAX_RAW_LENGTH		88
#define RAW_LENGTH				88

typedef struct settings_t {
	double id;
	double temp;
	double humi;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(tfa30->rawlen >= MIN_RAW_LENGTH && tfa30->rawlen <= MAX_RAW_LENGTH) {
		if(tfa30->raw[tfa30->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}
	return -1;
}

static void parseCode(void) {
	int i = 0, x = 0, type = 0, id = 0, binary[MAX_RAW_LENGTH/2];
	double temp_offset = 0.0, humi_offset = 0.0;
	double humidity = 0.0, temperature = 0.0;
	int n0 = 0, n1 = 0, n2 = 0, n3 = 0, n3b = 0;
	int n4 = 0, n5 = 0, n6 = 0, n7 = 0, n8 = 0;
	int n9 = 0, n10 = 0;
	int y = 0;
	int checksum = 1;

	if(tfa30->rawlen>MAX_RAW_LENGTH) {
		logprintf(LOG_ERR, "tfa30: parsecode - invalid parameter passed %d", tfa30->rawlen);
		return;
	}

	if(tfa30->rawlen == 80) {         // create first nibble for raw length 80
		for(y=0;y<4;y+=1) {
			binary[i++] = 0;
		}
	}

	for(x=0;x<tfa30->rawlen;x+=2) {
		if(tfa30->raw[x] > AVG_PULSE) {
			binary[i++] = 0;
		} else {
			binary[i++] = 1;
		}
	}

 	n10=binToDecRev(binary, 40, 43);
 	n9=binToDecRev(binary, 36, 39);
	n8=binToDecRev(binary, 32, 35);
	n7=binToDecRev(binary, 28, 31);
	n6=binToDecRev(binary, 24, 27);
	n5=binToDecRev(binary, 20, 23);
	n4=binToDecRev(binary, 16, 19);
	n3b=binToDecRev(binary, 12, 18);
	n3=binToDecRev(binary, 12, 15);
	n2=binToDecRev(binary, 8, 11);
	n1=binToDecRev(binary, 4, 7);
	n0=binToDecRev(binary, 0, 3);

	id = n3b;

	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON){
			humi_offset = tmp->humi;
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	// Temp
	if((n1 == 0xa) & (n2 == 0x0)) {
		type = 0x1;
		checksum = (n0+n1+n2+n3+n4+n5+n6+n7+n8+n9) & 0xf;
		if(n10 != checksum) {
			type=0x5;
			return;
		}
	// Hum
	} else if((n1 == 0xa) & (n2 == 0xe)) {
		type = 0x2;
		checksum = (n0+n1+n2+n3+n4+n5+n6+n7+n8+n9) & 0xf;
		if(n10 != checksum){
			type=0x5;
			return;
		}
	// nothing valid
	} else 	{
		type = 0x5;
		return;
	}

	tfa30->message = json_mkobject();
	switch(type) {
		case 1:
			temperature = (double)(n5-5)*10 + n6 + n7/10.0;
			temperature += temp_offset;

			json_append_member(tfa30->message, "id", json_mknumber(id, 0));
			json_append_member(tfa30->message, "temperature", json_mknumber(temperature, 1));
		break;
		case 2:
			humidity = (double)(n5)*10 + n6;
			humidity += humi_offset;

			json_append_member(tfa30->message, "id", json_mknumber(id, 0));
			json_append_member(tfa30->message, "humidity", json_mknumber(humidity, 1));
		break;
		default:
			json_delete(tfa30->message);
			tfa30->message = NULL;
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
void tfa30Init(void) {

	protocol_register(&tfa30);
	protocol_set_id(tfa30, "tfa30");
	protocol_device_add(tfa30, "tfa30", "TFA 30.X Temp Hum Sensor");
	tfa30->devtype = WEATHER;
	tfa30->hwtype = RF433;
	tfa30->minrawlen = MIN_RAW_LENGTH;
	tfa30->maxrawlen = MAX_RAW_LENGTH;
	tfa30->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	tfa30->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&tfa30->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&tfa30->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");
	options_add(&tfa30->options, "h", "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");

	// options_add(&tfa30->options, "0", "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&tfa30->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&tfa30->options, "0", "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&tfa30->options, "0", "humidity-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tfa30->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tfa30->options, "0", "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&tfa30->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");


	tfa30->parseCode=&parseCode;
	tfa30->checkValues=&checkValues;
	tfa30->validate=&validate;
	tfa30->gc=&gc;
}

#ifdef MODULAR
void compatibility(const char **version, const char **commit) {
	module->name = "tfa30";
	module->version = "1.1";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	tfa30Init();
}
#endif
