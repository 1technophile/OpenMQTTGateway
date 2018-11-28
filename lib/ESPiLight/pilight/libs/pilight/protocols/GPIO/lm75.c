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
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <wiringx.h>
#endif
#include <pthread.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../../core/threads.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "../../core/json.h"
#include "../../config/settings.h"
#include "../protocol.h"
#include "lm75.h"

#if !defined(__FreeBSD__) && !defined(_WIN32)
typedef struct settings_t {
	char **id;
	char path[PATH_MAX];
	int nrid;
	int *fd;
} settings_t;

static unsigned short loop = 1;
static int threads = 0;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

static void *thread(void *param) {
	struct protocol_threads_t *node = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)node->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct settings_t *lm75data = MALLOC(sizeof(struct settings_t));
	int y = 0, interval = 10, nrloops = 0;
	char *stmp = NULL;
	double itmp = -1, temp_offset = 0.0;

	if(lm75data == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	lm75data->nrid = 0;
	lm75data->id = NULL;
	lm75data->fd = 0;

	threads++;

	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			if(json_find_string(jchild, "id", &stmp) == 0) {
				if((lm75data->id = REALLOC(lm75data->id, (sizeof(char *)*(size_t)(lm75data->nrid+1)))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if((lm75data->id[lm75data->nrid] = MALLOC(strlen(stmp)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(lm75data->id[lm75data->nrid], stmp);
				lm75data->nrid++;
			}
			if(json_find_string(jchild, "i2c-path", &stmp) == 0) {
				strcpy(lm75data->path, stmp);
			}
			jchild = jchild->next;
		}
	}

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		interval = (int)round(itmp);
	json_find_number(json, "temperature-offset", &temp_offset);

	if((lm75data->fd = REALLOC(lm75data->fd, (sizeof(int)*(size_t)(lm75data->nrid+1)))) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	for(y=0;y<lm75data->nrid;y++) {
		lm75data->fd[y] = wiringXI2CSetup(lm75data->path, (int)strtol(lm75data->id[y], NULL, 16));
	}

	while(loop) {
		if(protocol_thread_wait(node, interval, &nrloops) == ETIMEDOUT) {
			pthread_mutex_lock(&lock);
			for(y=0;y<lm75data->nrid;y++) {
				if(lm75data->fd[y] > 0) {
					int raw = wiringXI2CReadReg16(lm75data->fd[y], 0x00);
					float temp = ((float)((raw&0x00ff)+((raw>>15)?0:0.5))*10);

					lm75->message = json_mkobject();
					JsonNode *code = json_mkobject();
					json_append_member(code, "id", json_mkstring(lm75data->id[y]));
					json_append_member(code, "temperature", json_mknumber((temp+temp_offset)/10, 1));

					json_append_member(lm75->message, "message", code);
					json_append_member(lm75->message, "origin", json_mkstring("receiver"));
					json_append_member(lm75->message, "protocol", json_mkstring(lm75->id));

					if(pilight.broadcast != NULL) {
						pilight.broadcast(lm75->id, lm75->message, PROTOCOL);
					}
					json_delete(lm75->message);
					lm75->message = NULL;
				} else {
					logprintf(LOG_NOTICE, "error connecting to lm75");
					logprintf(LOG_DEBUG, "(probably i2c bus error from wiringXI2CSetup)");
					logprintf(LOG_DEBUG, "(maybe wrong id? use i2cdetect to find out)");
					protocol_thread_wait(node, 1, &nrloops);
				}
			}
			pthread_mutex_unlock(&lock);
		}
	}
	pthread_mutex_unlock(&lock);

	if(lm75data->id) {
		for(y=0;y<lm75data->nrid;y++) {
			FREE(lm75data->id[y]);
		}
		FREE(lm75data->id);
	}
	if(lm75data->fd) {
		for(y=0;y<lm75data->nrid;y++) {
			if(lm75data->fd[y] > 0) {
				close(lm75data->fd[y]);
			}
		}
		FREE(lm75data->fd);
	}
	FREE(lm75data);
	threads--;

	return (void *)NULL;
}

static struct threadqueue_t *initDev(JsonNode *jdevice) {
	char *platform = GPIO_PLATFORM;
	if(settings_find_string("gpio-platform", &platform) != 0 || strcmp(platform, "none") == 0) {
		logprintf(LOG_ERR, "lm75: no gpio-platform configured");
		exit(EXIT_FAILURE);
	}
	if(wiringXSetup(platform, logprintf1) == 0) {
		loop = 1;
		char *output = json_stringify(jdevice, NULL);
		JsonNode *json = json_decode(output);
		json_free(output);

		struct protocol_threads_t *node = protocol_thread_init(lm75, json);
		return threads_register("lm75", &thread, (void *)node, 0);
	} else {
		return NULL;
	}
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(lm75);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(lm75);
}
#endif

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void lm75Init(void) {
#if !defined(__FreeBSD__) && !defined(_WIN32)
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
#endif

	protocol_register(&lm75);
	protocol_set_id(lm75, "lm75");
	protocol_device_add(lm75, "lm75", "TI I2C Temperature Sensor");
	lm75->devtype = WEATHER;
	lm75->hwtype = SENSOR;

	options_add(&lm75->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&lm75->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "0x[0-9a-f]{2}");
	options_add(&lm75->options, 'd', "i2c-path", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^/dev/i2c-[0-9]{1,2}$");

	// options_add(&lm75->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&lm75->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&lm75->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

#if !defined(__FreeBSD__) && !defined(_WIN32)
	lm75->initDev=&initDev;
	lm75->threadGC=&threadGC;
#endif
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "lm75";
	module->version = "2.2";
	module->reqversion = "7.0";
	module->reqcommit = "186";
}

void init(void) {
	lm75Init();
}
#endif
