/*
	Copyright (C) 2013 CurlyMo

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
#include <assert.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
#endif
#include <pthread.h>

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "cpu_temp.h"

#ifndef _WIN32
#ifdef PILIGHT_DEVELOPMENT
static unsigned short loop = 1;
static unsigned short threads = 0;
static char cpu_path[] = "/sys/class/thermal/thermal_zone0/temp";

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

static void *thread(void *param) {
	struct protocol_threads_t *node = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)node->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct stat st;

	FILE *fp = NULL;
	double itmp = 0;
	int *id = MALLOC(sizeof(int));
	char *content = NULL;
	int nrloops = 0, nrid = 0, y = 0, interval = 0;
	double temp_offset = 0.0;
	size_t bytes = 0;

	threads++;

	if(id == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			if(json_find_number(jchild, "id", &itmp) == 0) {
				id = REALLOC(id, (sizeof(int)*(size_t)(nrid+1)));
				id[nrid] = (int)round(itmp);
				nrid++;
			}
			jchild = jchild->next;
		}
	}

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		interval = (int)round(itmp);
	json_find_number(json, "temperature-offset", &temp_offset);

	while(loop) {
		if(protocol_thread_wait(node, interval, &nrloops) == ETIMEDOUT) {
			pthread_mutex_lock(&lock);
			for(y=0;y<nrid;y++) {
				if((fp = fopen(cpu_path, "rb"))) {
					fstat(fileno(fp), &st);
					bytes = (size_t)st.st_size;

					if((content = REALLOC(content, bytes+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					memset(content, '\0', bytes+1);

					if(fread(content, sizeof(char), bytes, fp) == -1) {
						logprintf(LOG_NOTICE, "cannot read file: %s", cpu_path);
						fclose(fp);
						break;
					} else {
						fclose(fp);
						double temp = atof(content)+temp_offset;
						FREE(content);

						cpuTemp->message = json_mkobject();
						JsonNode *code = json_mkobject();
						json_append_member(code, "id", json_mknumber(id[y], 0));
						json_append_member(code, "temperature", json_mknumber((temp/1000), 3));

						json_append_member(cpuTemp->message, "message", code);
						json_append_member(cpuTemp->message, "origin", json_mkstring("receiver"));
						json_append_member(cpuTemp->message, "protocol", json_mkstring(cpuTemp->id));

						if(pilight.broadcast != NULL) {
							pilight.broadcast(cpuTemp->id, cpuTemp->message, PROTOCOL);
						}
						json_delete(cpuTemp->message);
						cpuTemp->message = NULL;
					}
				} else {
					logprintf(LOG_NOTICE, "CPU sysfs \"%s\" does not exist", cpu_path);
				}
			}
			pthread_mutex_unlock(&lock);
		}
	}
	pthread_mutex_unlock(&lock);

	FREE(id);
	threads--;
	return (void *)NULL;
}
#else

static char cpu_path[PATH_MAX] = "/sys/class/thermal/thermal_zone0/temp";

typedef struct data_t {
	char *name;
	int id;
	int interval;
	uv_timer_t *timer_req;
	double temp_offset;
	struct data_t *next;
} data_t;

static struct data_t *data = NULL;

#ifdef PILIGHT_REWRITE
static void *reason_code_received_free(void *param) {
	struct reason_code_received_t *data = param;
	FREE(data);
	return NULL;
}
#endif

static void *thread(void *param) {
	uv_timer_t *timer_req = param;
	struct data_t *settings = timer_req->data;
	FILE *fp = NULL;
	struct stat st;
	char *content = NULL;
	size_t bytes = 0;

	if((fp = fopen(cpu_path, "rb"))) {
		fstat(fileno(fp), &st);
		bytes = (size_t)st.st_size;
		if((content = REALLOC(content, bytes+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		memset(content, '\0', bytes+1);

		if(fread(content, sizeof(char), bytes, fp) == -1) {
			logprintf(LOG_NOTICE, "cannot read file: %s", cpu_path);
			fclose(fp);
			return NULL;
		} else {
			fclose(fp);
			double temp = atof(content)+settings->temp_offset;
			FREE(content);

#ifdef PILIGHT_DEVELOPMENT
			struct reason_code_received_t *data = MALLOC(sizeof(struct reason_code_received_t));
			if(data == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			snprintf(data->message, 1024, "{\"id\":%d,\"temperature\":%.3f}", settings->id, (temp/1000));
			strcpy(data->origin, "receiver");
			data->protocol = cpuTemp->id;
			if(strlen(pilight_uuid) > 0) {
				data->uuid = pilight_uuid;
			} else {
				data->uuid = NULL;
			}
			data->repeat = 1;
			eventpool_trigger(REASON_CODE_RECEIVED, reason_code_received_free, data);
#else
			char message[1024];
			snprintf(message, 1024,
				"{\"origin\":\"receiver\",\"protocol\":\"%s\",\"message\":"\
					"{\"id\":%d,\"temperature\":%.3f}"\
				"}",
				cpuTemp->id, settings->id, (temp/1000)
			);

			cpuTemp->message = json_decode(message);

			if(pilight.broadcast != NULL) {
				pilight.broadcast(cpuTemp->id, cpuTemp->message, PROTOCOL);
			}
			json_delete(cpuTemp->message);
			cpuTemp->message = NULL;
#endif
		}
	} else {
		logprintf(LOG_NOTICE, "CPU sysfs \"%s\" does not exists", cpu_path);
	}

	return (void *)NULL;
}
#endif

static struct threadqueue_t *initDev(struct JsonNode *jdevice) {
#ifdef PILIGHT_DEVELOPMENT
	loop = 1;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(cpuTemp, json);
	return threads_register("cpu_temp", &thread, (void *)node, 0);
#else
#ifdef PILIGHT_REWRITE
	struct JsonNode *jdevice = NULL;
#endif
	struct JsonNode *jprotocols = NULL;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct data_t *node = NULL;
	FILE *fp = NULL;
	double itmp = 0;
	int match = 0;

#ifdef PILIGHT_REWRITE
	if(param == NULL) {
		return NULL;
	}

	if((jdevice = json_first_child(param)) == NULL) {
		return NULL;
	}
#endif
	if((jprotocols = json_find_member(jdevice, "protocol")) != NULL) {
		jchild = json_first_child(jprotocols);
		while(jchild) {
			if(strcmp(cpuTemp->id, jchild->string_) == 0) {
				match = 1;
				break;
		}
			jchild = jchild->next;
		}
	}

	if(match == 0) {
		return NULL;
	}

	if(!(fp = fopen(cpu_path, "rb"))) {
		logprintf(LOG_NOTICE, "CPU sysfs \"%s\" does not exists", cpu_path);
		return NULL;
	} else {
		fclose(fp);
	}

	if((node = MALLOC(sizeof(struct data_t)))== NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(node, '\0', sizeof(struct data_t));
	node->interval = 10;
	node->temp_offset = 0;

	if((jid = json_find_member(jdevice, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			if(json_find_number(jchild, "id", &itmp) == 0) {
				node->id = (int)round(itmp);
			}
			jchild = jchild->next;
		}
	}

	if(json_find_number(jdevice, "poll-interval", &itmp) == 0)
		node->interval = (int)round(itmp);
	json_find_number(jdevice, "temperature-offset", &node->temp_offset);

	if((node->name = MALLOC(strlen(jdevice->key)+1)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy(node->name, jdevice->key);
	node->timer_req = NULL;
	node->next = data;
	data = node;

	if((node->timer_req = MALLOC(sizeof(uv_timer_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	node->timer_req->data = node;
	uv_timer_init(uv_default_loop(), node->timer_req);
	assert(node->interval > 0);
	uv_timer_start(node->timer_req, (void (*)(uv_timer_t *))thread, node->interval*1000, node->interval*1000);

	return NULL;
#endif
}

static void threadGC(void) {
#ifdef PILIGHT_DEVELOPMENT
	loop = 0;
	protocol_thread_stop(cpuTemp);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(cpuTemp);
#else
	struct data_t *tmp = NULL;
	while(data) {
		tmp = data;
#ifndef PILIGHT_REWRITE
		uv_timer_stop(data->timer_req);
#endif
		FREE(tmp->name);
		data = data->next;
		FREE(tmp);
	}
	if(data != NULL) {
		FREE(data);
	}
#endif
}
#endif

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void cpuTempInit(void) {
#ifndef _WIN32
	#ifdef PILIGHT_DEVELOPMENT
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&lock, &attr);
	#endif
#endif

	protocol_register(&cpuTemp);
	protocol_set_id(cpuTemp, "cpu_temp");
	protocol_device_add(cpuTemp, "cpu_temp", "CPU temperature sensor");
	cpuTemp->devtype = WEATHER;
	cpuTemp->hwtype = SENSOR;

	options_add(&cpuTemp->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&cpuTemp->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "[0-9]");

	// options_add(&cpuTemp->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)3, "[0-9]");
	options_add(&cpuTemp->options, 0, "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&cpuTemp->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)3, "[0-9]");
	options_add(&cpuTemp->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&cpuTemp->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)10, "[0-9]");

#ifndef _WIN32
	cpuTemp->initDev=&initDev;
	cpuTemp->threadGC=&threadGC;
#endif
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "cpu_temp";
	module->version = "1.6";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	cpuTempInit();
}
#endif
