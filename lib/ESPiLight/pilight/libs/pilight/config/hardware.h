/*
	Copyright (C) 2013 - 2014 CurlyMo

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

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

typedef enum {
	HWINTERNAL = -1,
	NONE = 0,
	RF433,
	RF868,
	RFIR,
	ZWAVE,
	SENSOR,
	HWRELAY,
	API
} hwtype_t;

typedef enum {
	COMNONE = 0,
	COMOOK,
	COMPLSTRAIN,
	COMAPI
} communication_t;

#include <pthread.h>
#include "../core/options.h"
#include "../core/json.h"
#include "../core/config.h"
#include "defines.h"

struct config_t *config_hardware;

typedef struct rawcode_t {
	int pulses[MAXPULSESTREAMLENGTH];
	int length;
} rawcode_t;

typedef struct hardware_t {
	char *id;
	unsigned short wait;
	unsigned short stop;
	pthread_mutex_t lock;
	pthread_cond_t signal;
	pthread_mutexattr_t attr;
	unsigned short running;
	hwtype_t hwtype;
	communication_t comtype;
	struct options_t *options;

	int minrawlen;
	int maxrawlen;
	int mingaplen;
	int maxgaplen;

	unsigned short (*init)(void);
	unsigned short (*deinit)(void);
	union {
		int (*receiveOOK)(void);
		void *(*receiveAPI)(void *param);
		int (*receivePulseTrain)(struct rawcode_t *r);
	};
	union {
		int (*sendOOK)(int *code, int rawlen, int repeats);
		int (*sendAPI)(struct JsonNode *code);
	};
	int (*gc)(void);
	unsigned short (*settings)(JsonNode *json);
	struct hardware_t *next;
} hardware_t;

typedef struct conf_hardware_t {
	hardware_t *hardware;
	struct conf_hardware_t *next;
} conf_hardware_t;

extern struct hardware_t *hardware;
extern struct conf_hardware_t *conf_hardware;

void hardware_init(void);
void hardware_register(struct hardware_t **hw);
void hardware_set_id(struct hardware_t *hw, const char *id);

#endif
