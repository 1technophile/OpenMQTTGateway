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

#ifndef _DEVICES_H_
#define _DEVICES_H_

#include <pthread.h>

typedef struct devices_settings_t devices_settings_t;
typedef struct devices_values_t devices_values_t;
typedef struct devices_t devices_t;

#include "../core/pilight.h"
#include "../core/threads.h"
#include "../core/config.h"
#include "../protocols/protocol.h"
#include "../events/action.h"

/*
|------------------|
|    devices_t     |
|------------------|
| id               |
| name		         |
| protocols	       | --> protocols_t <protocol.h>
| settings	       | ---
|------------------|   |
				       |
|------------------|   |
|devices_settings_t| <--
|------------------|
| name             |
| values	         | ---
|------------------|   |
                       |
|------------------|   |
| devices_values_t | <--
|------------------|
| value            |
| type		         |
|------------------|
*/

struct devices_values_t {
	union {
		char *string_;
		double number_;
	};
	int decimals;
	char *name;
	int type;
	struct devices_values_t *next;
};

struct devices_settings_t {
	char *name;
	struct devices_values_t *values;
	struct devices_settings_t *next;
};

struct devices_t {
	char *id;
	char dev_uuid[22];
	char ori_uuid[22];
	int cst_uuid;
	int nrthreads;
	time_t timestamp;
#ifdef EVENTS
	int lastrule;
	int prevrule;
	enum origin_t lastorigin;
	enum origin_t prevorigin;
	struct event_action_thread_t *action_thread;
#endif
	struct protocols_t *protocols;
	struct devices_settings_t *settings;
	struct threadqueue_t **protocol_threads;
	struct devices_t *next;
};

extern struct config_t *config_devices;

int devices_update(char *protoname, JsonNode *message, enum origin_t origin, JsonNode **out);
int devices_get(char *sid, struct devices_t **dev);
int devices_valid_state(char *sid, char *state);
int devices_valid_value(char *sid, char *name, char *value);
struct JsonNode *devices_values(const char *media);
void devices_init(void);
int devices_gc(void);

#endif
