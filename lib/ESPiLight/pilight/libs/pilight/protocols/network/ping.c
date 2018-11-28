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
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
#endif
#include <pthread.h>

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/ping.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "ping.h"

static unsigned short loop = 1;
static unsigned short threads = 0;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

#define CONNECTED				1
#define DISCONNECTED 		0

static void *thread(void *param) {
	struct protocol_threads_t *node = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)node->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	char *ip = NULL;
	char *pstate = NULL;
	double itmp = 0.0;
	int state = 0, nrloops = 0, interval = 1;

	threads++;

	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			if(json_find_string(jchild, "ip", &ip) == 0) {
				break;
			}
			jchild = jchild->next;
		}
	}

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		interval = (int)round(itmp);

   if(json_find_string(json, "state", &pstate) == 0) {
      if(strcmp(pstate,"connected") == 0) state = CONNECTED;
      if(strcmp(pstate,"disconnected") == 0) state = DISCONNECTED;
      }

	while(loop) {
		if(protocol_thread_wait(node, interval, &nrloops) == ETIMEDOUT) {
			pthread_mutex_lock(&lock);
			if(ping(ip) == 0) {
				if(state == DISCONNECTED) {
					state = CONNECTED;
					pping->message = json_mkobject();
					JsonNode *code = json_mkobject();
					json_append_member(code, "ip", json_mkstring(ip));
					json_append_member(code, "state", json_mkstring("connected"));

					json_append_member(pping->message, "message", code);
					json_append_member(pping->message, "origin", json_mkstring("receiver"));
					json_append_member(pping->message, "protocol", json_mkstring(pping->id));

					if(pilight.broadcast != NULL) {
						pilight.broadcast(pping->id, pping->message, PROTOCOL);
					}
					json_delete(pping->message);
					pping->message = NULL;
				}
			} else if(state == CONNECTED) {
				state = DISCONNECTED;

				pping->message = json_mkobject();
				JsonNode *code = json_mkobject();
				json_append_member(code, "ip", json_mkstring(ip));
				json_append_member(code, "state", json_mkstring("disconnected"));

				json_append_member(pping->message, "message", code);
				json_append_member(pping->message, "origin", json_mkstring("receiver"));
				json_append_member(pping->message, "protocol", json_mkstring(pping->id));

				if(pilight.broadcast != NULL) {
					pilight.broadcast(pping->id, pping->message, PROTOCOL);
				}
				json_delete(pping->message);
				pping->message = NULL;
			}
			pthread_mutex_unlock(&lock);
		}
	}
	pthread_mutex_unlock(&lock);

	threads--;
	return (void *)NULL;
}

static struct threadqueue_t *initDev(JsonNode *jdevice) {
	loop = 1;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(pping, json);
	return threads_register("ping", &thread, (void *)node, 0);
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(pping);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(pping);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void pingInit(void) {
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);

	protocol_register(&pping);
	protocol_set_id(pping, "ping");
	protocol_device_add(pping, "ping", "Ping network devices");
	pping->devtype = PING;
	pping->hwtype = API;
	pping->multipleId = 0;
	pping->masterOnly = 1;

	options_add(&pping->options, 'c', "connected", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&pping->options, 'd', "disconnected", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&pping->options, 'i', "ip", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");

	options_add(&pping->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)10, "[0-9]");

	pping->initDev=&initDev;
	pping->threadGC=&threadGC;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "ping";
	module->version = "2.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	pingInit();
}
#endif
