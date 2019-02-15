/*
	Copyright (C) 2014, 2018 CurlyMo & DonBernos & Michael Behrisch

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
#include "tfa2017.h"

#define MIN_PULSE_LENGTH	250
#define AVG_PULSE		750
#define MIN_RAW_LENGTH		200
#define MAX_RAW_LENGTH		400
#define MESSAGE_LENGTH		48

typedef struct settings_t {
	double id;
	double temp;
	double humi;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static int validate(void) {
	if(tfa2017->rawlen >= MIN_RAW_LENGTH && tfa2017->rawlen <= MAX_RAW_LENGTH) {
		if(tfa2017->raw[tfa2017->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}
	return -1;
}

static void parseCode(void) {
	int i = 0, x = 0, short_pulse = 0, prev = 0, long_pulse = 0;
	int s = 0, start[3], m = 0, binary[MAX_RAW_LENGTH];
	int msg[MESSAGE_LENGTH], channel = 0;
	double humidity = 0.0, temperature = 0.0;

	if(tfa2017->rawlen > MAX_RAW_LENGTH) {
		logprintf(LOG_ERR, "tfa2017: parsecode - invalid parameter passed %d", tfa2017->rawlen);
		return;
	}
	for(x=0;x<tfa2017->rawlen;x++) {
		if(tfa2017->raw[x] > AVG_PULSE) {
			binary[i++] = 0;
			if(short_pulse>0) {
				prev = short_pulse;
				short_pulse = 0;
			}
			long_pulse++;
		} else {
			short_pulse++;
			if(short_pulse%2 == 0) {
				binary[i++] = 1;
			}
			long_pulse = 0;
		}
		if(long_pulse == 4 && (prev == 20 || (x > 7 && prev == x-1))) {
			if (s==0 || i > start[s-1]+MESSAGE_LENGTH) {
				start[s++] = i-2;
				prev = 0;
			}
		}
	}
	// The protocol sends the message three times in a row.
	// If we find two identical ones, we consider it valid.
	if(s < 2) {
		return;
	}
	if(i > start[1]+MESSAGE_LENGTH && memcmp(&binary[start[0]], &binary[start[1]], MESSAGE_LENGTH) == 0) {
		m=start[0];
	} else if(s > 2 && i > start[2]+MESSAGE_LENGTH &&
				(memcmp(&binary[start[0]], &binary[start[2]], MESSAGE_LENGTH) == 0 ||
					memcmp(&binary[start[1]], &binary[start[2]], MESSAGE_LENGTH) == 0)) {
		m=start[2];
	} else {
		return;
	}

	// decode manchester
	prev = 1;
	for(x=0;x<MESSAGE_LENGTH;x++) {
		if(binary[x+m] == 0) {
			prev = !prev;
		}
		msg[x] = prev;
	}
	// According to http://www.osengr.org/WxShield/Downloads/Weather-Sensor-RF-Protocols.pdf
	// the first byte is a fixed id (0x45), the second is a rolling code which changes on
	// battery replacement (both are not used here).
	// Of the next four bits the first is unused, the next three encode the channel.
	channel = binToDecRev(msg, 17, 19)+1;
	// The next twelve bits encode the temperature T
	// in tenth of degree Fahrenheit with an offset of 40.
	// The following is a simplification of F=T/10-40 and C=(F-32)*5/9.
	temperature = (double)binToDecRev(msg, 20, 31)/18.-40.;
	// The next byte has the relative humidity in percent.
	humidity = (double)binToDecRev(msg, 32, 39);
	// The last byte contains a checksum which is not used here.

	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-channel) < EPSILON){
			temperature += tmp->temp;
			humidity += tmp->humi;
			break;
		}
		tmp = tmp->next;
	}
	if(humidity < 0 || humidity > 100) {
		return;
	}

	tfa2017->message = json_mkobject();
	json_append_member(tfa2017->message, "id", json_mknumber(channel, 0));
	json_append_member(tfa2017->message, "temperature", json_mknumber(temperature, 2));
	json_append_member(tfa2017->message, "humidity", json_mknumber(humidity, 2));
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
void tfa2017Init(void) {
	protocol_register(&tfa2017);
	protocol_set_id(tfa2017, "tfa2017");
	protocol_device_add(tfa2017, "tfa2017", "TFA 30.X Temp Hum Sensor Revision 09/2017");
	tfa2017->devtype = WEATHER;
	tfa2017->hwtype = RF433;
	tfa2017->minrawlen = MIN_RAW_LENGTH;
	tfa2017->maxrawlen = MAX_RAW_LENGTH;
	tfa2017->maxgaplen = AVG_PULSE*PULSE_DIV;
	tfa2017->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&tfa2017->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&tfa2017->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");
	options_add(&tfa2017->options, "h", "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");

	// options_add(&tfa2017->options, "0", "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&tfa2017->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&tfa2017->options, "0", "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&tfa2017->options, "0", "humidity-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tfa2017->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&tfa2017->options, "0", "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&tfa2017->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	tfa2017->parseCode=&parseCode;
	tfa2017->checkValues=&checkValues;
	tfa2017->validate=&validate;
	tfa2017->gc=&gc;
}

#ifdef MODULAR
void compatibility(const char **version, const char **commit) {
	module->name = "tfa2017";
	module->version = "1.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	tfa2017Init();
}
#endif
