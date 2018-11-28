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
#include <assert.h>
#include <sys/stat.h>
#include <signal.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define MSG_NOSIGNAL 0
#else
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/socket.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/ntp.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "../../core/datetime.h"
#include "datetime.h"

#ifdef PILIGHT_DEVELOPMENT
static unsigned short loop = 1;
static unsigned short threads = 0;
static char *format = NULL;

static pthread_mutex_t lock;

static void *thread(void *param) {
	char UTC[] = "UTC";
	struct protocol_threads_t *thread = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)thread->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	struct tm tm;
	char *tz = NULL;
	time_t t;
	int counter = 0;
	double longitude = 0.0, latitude = 0.0;

	threads++;

	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);
			while(jchild1) {
				if(strcmp(jchild1->key, "longitude") == 0) {
					longitude = jchild1->number_;
				}
				if(strcmp(jchild1->key, "latitude") == 0) {
					latitude = jchild1->number_;
				}
				jchild1 = jchild1->next;
			}
			jchild = jchild->next;
		}
	}

	if((tz = coord2tz(longitude, latitude)) == NULL) {
		logprintf(LOG_INFO, "datetime #%d, could not determine timezone", threads);
		tz = UTC;
	} else {
		logprintf(LOG_INFO, "datetime #%d %.6f:%.6f seems to be in timezone: %s", threads, longitude, latitude, tz);
	}

	t = time(NULL);
	if(isntpsynced() == 0) {
		t -= getntpdiff();
	}

	while(loop) {
		pthread_mutex_lock(&lock);
		t = time(NULL);
		t -= getntpdiff();

		/* Get UTC time */
		if(localtime_l(t, &tm, tz) == 0) {
			int year = tm.tm_year+1900;
			int month = tm.tm_mon+1;
			int day = tm.tm_mday;
			int hour = tm.tm_hour;
			int minute = tm.tm_min;
			int second = tm.tm_sec;
			int weekday = tm.tm_wday+1;
			int dst = tm.tm_isdst;

			datetime->message = json_mkobject();

			JsonNode *code = json_mkobject();
			json_append_member(code, "longitude", json_mknumber(longitude, 6));
			json_append_member(code, "latitude", json_mknumber(latitude, 6));
			json_append_member(code, "year", json_mknumber(year, 0));
			json_append_member(code, "month", json_mknumber(month, 0));
			json_append_member(code, "day", json_mknumber(day, 0));
			json_append_member(code, "weekday", json_mknumber(weekday, 0));
			json_append_member(code, "hour", json_mknumber(hour, 0));
			json_append_member(code, "minute", json_mknumber(minute, 0));
			json_append_member(code, "second", json_mknumber(second, 0));
			json_append_member(code, "dst", json_mknumber(dst, 0));

			json_append_member(datetime->message, "message", code);
			json_append_member(datetime->message, "origin", json_mkstring("receiver"));
			json_append_member(datetime->message, "protocol", json_mkstring(datetime->id));

			if(pilight.broadcast != NULL) {
				pilight.broadcast(datetime->id, datetime->message, PROTOCOL);
			}

			json_delete(datetime->message);
			datetime->message = NULL;
			counter++;
		}
		pthread_mutex_unlock(&lock);
		sleep(1);
	}
	pthread_mutex_unlock(&lock);

	threads--;
	return (void *)NULL;
}
#else

static char UTC[] = "UTC";
static char *format = NULL;
static int time_override = -1;

typedef struct data_t {
	char *name;
	int id;
	int interval;
	uv_timer_t *timer_req;

	double longitude;
	double latitude;

	char *tz;

	struct data_t *next;
} data_t;

static struct data_t *data = NULL;

#ifdef PILIGHT_REWRITE
static void *reason_code_received_free(void *param) {
	struct reason_code_received_t *data = param;
	FREE(data);
	return NULL;
}

static void *adaptDevice(int reason, void *param) {
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jtime = NULL;

	if(param == NULL) {
		return NULL;
	}

	if((jdevice = json_first_child(param)) == NULL) {
		return NULL;
	}

	if(strcmp(jdevice->key, "datetime") != 0) {
		return NULL;
	}

	if((jtime = json_find_member(jdevice, "time-override")) != NULL) {
		if(jtime->tag == JSON_NUMBER) {
			time_override = jtime->number_;
		}
	}

	return NULL;
}
#endif

static void *thread(void *param) {
	uv_timer_t *timer_req = param;
	struct data_t *settings = timer_req->data;
	struct tm tm;
	time_t t;

	if(time_override > -1) {
		t = time_override;
	} else {
		t = time(NULL);
		if(isntpsynced() == 0) {
			t -= getntpdiff();
		}
	}

	/* Get UTC time */
	if(localtime_l(t, &tm, settings->tz) == 0) {
		int year = tm.tm_year+1900;
		int month = tm.tm_mon+1;
		int day = tm.tm_mday;
		int hour = tm.tm_hour;
		int minute = tm.tm_min;
		int second = tm.tm_sec;
		int weekday = tm.tm_wday+1;
		int dst = tm.tm_isdst;

#ifdef PILIGHT_REWRITE
		struct reason_code_received_t *data = MALLOC(sizeof(struct reason_code_received_t));
		if(data == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		snprintf(data->message, 1024,
			"{\"longitude\":%.6f,\"latitude\":%.6f,\"year\":%d,\"month\":%d,\"day\":%d,\"weekday\":%d,\"hour\":%d,\"minute\":%d,\"second\":%d,\"dst\":%d}",
			settings->longitude, settings->latitude, year, month, day, weekday, hour, minute, second, dst
		);
		strncpy(data->origin, "receiver", 255);
		data->protocol = datetime->id;
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
			"{\"longitude\":%.6f,\"latitude\":%.6f,\"year\":%d,\"month\":%d,\"day\":%d,\"weekday\":%d,\"hour\":%d,\"minute\":%d,\"second\":%d,\"dst\":%d}"\
		"}",
		datetime->id, settings->longitude, settings->latitude, year, month, day, weekday, hour, minute, second, dst
	);

	datetime->message = json_decode(message);

	if(pilight.broadcast != NULL) {
		pilight.broadcast(datetime->id, datetime->message, PROTOCOL);
	}
	json_delete(datetime->message);
	datetime->message = NULL;
#endif
	}

	return (void *)NULL;
}
#endif
static struct threadqueue_t *initDev(JsonNode *jdevice) {
#ifdef PILIGHT_DEVELOPMENT
	loop = 1;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(datetime, json);
	return threads_register("datetime", &thread, (void *)node, 0);
#else
#ifdef PILIGHT_REWRITE
	struct JsonNode *jdevice = NULL;
#endif
	struct JsonNode *jprotocols = NULL;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	struct data_t *node = NULL;
	int match = 0;

#ifdef PILIGHT_REWRITE
	if(param == NULL) {
		return NULL;
	}
#endif

	if(!(datetime->masterOnly == 0 || pilight.runmode == STANDALONE)) {
		return NULL;
	}

#ifdef PILIGHT_REWRITE
	if((jdevice = json_first_child(param)) == NULL) {
		return NULL;
	}
#endif
	if((jprotocols = json_find_member(jdevice, "protocol")) != NULL) {
		jchild = json_first_child(jprotocols);
		while(jchild) {
			if(strcmp(datetime->id, jchild->string_) == 0) {
				match = 1;
				break;
			}
			jchild = jchild->next;
		}
	}

	if(match == 0) {
		return NULL;
	}

	if((node = MALLOC(sizeof(struct data_t)))== NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(node, '\0', sizeof(struct data_t));
	node->interval = 1;

	if((jid = json_find_member(jdevice, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);
			while(jchild1) {
				if(strcmp(jchild1->key, "longitude") == 0) {
					node->longitude = jchild1->number_;
				}
				if(strcmp(jchild1->key, "latitude") == 0) {
					node->latitude = jchild1->number_;
				}
				jchild1 = jchild1->next;
			}
			jchild = jchild->next;
		}
	}

	if((node->tz = coord2tz(node->longitude, node->latitude)) == NULL) {
		logprintf(LOG_INFO, "datetime %s, could not determine timezone, defaulting to UTC", jdevice->key);
		node->tz = UTC;
	} else {
		logprintf(LOG_INFO, "datetime %s %.6f:%.6f seems to be in timezone: %s", jdevice->key, node->longitude, node->latitude, node->tz);
	}

	if((node->name = MALLOC(strlen(jdevice->key)+1)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy(node->name, jdevice->key);

	node->next = data;
	node->timer_req = NULL;
	data = node;

	if((node->timer_req = MALLOC(sizeof(uv_timer_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	node->timer_req->data = node;
	uv_timer_init(uv_default_loop(), node->timer_req);

	assert(node->interval > 0);
	uv_timer_start(node->timer_req, (void (*)(uv_timer_t *))thread, node->interval*1000, node->interval*1000);
#endif
	return NULL;
}

static void threadGC(void) {
#ifdef PILIGHT_DEVELOPMENT
	loop = 0;

	protocol_thread_stop(datetime);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(datetime);
#else
	struct data_t *tmp = data;
	while(tmp) {
#ifndef PILIGHT_REWRITE
		uv_timer_stop(tmp->timer_req);
#endif
		tmp = tmp->next;
	}
#endif
}

static void gc(void) {
#ifndef PILIGHT_DEVELOPMENT
	struct data_t *tmp = NULL;
	while(data) {
		tmp = data;
#ifndef PILIGHT_REWRITE
		uv_timer_stop(tmp->timer_req);
#endif
		FREE(tmp->name);
		data = data->next;
		FREE(tmp);
	}
	if(data != NULL) {
		FREE(data);
	}
#endif
	FREE(format);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void datetimeInit(void) {

	if((format = MALLOC(20)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(format, 0, 20);
	strncpy(format, "HH:mm:ss YYYY-MM-DD", 19);

	protocol_register(&datetime);
	protocol_set_id(datetime, "datetime");
	protocol_device_add(datetime, "datetime", "Date and Time protocol");
	datetime->devtype = DATETIME;
	datetime->hwtype = API;
	datetime->multipleId = 0;
	datetime->masterOnly = 1;

	options_add(&datetime->options, 'o', "longitude", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&datetime->options, 'a', "latitude", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&datetime->options, 'y', "year", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{3,4}$");
	options_add(&datetime->options, 'm', "month", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{3,4}$");
	options_add(&datetime->options, 'd', "day", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&datetime->options, 'w', "weekday", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&datetime->options, 'h', "hour", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&datetime->options, 'i', "minute", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&datetime->options, 's', "second", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&datetime->options, 'z', "dst", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	options_add(&datetime->options, 0, "show-datetime", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&datetime->options, 0, "format", OPTION_HAS_VALUE, GUI_SETTING, JSON_STRING, (void *)format, NULL);

	datetime->initDev=&initDev;
	datetime->threadGC=&threadGC;
	datetime->gc=&gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "datetime";
	module->version = "2.6";
	module->reqversion = "6.0";
	module->reqcommit = "115";
}

void init(void) {
	datetimeInit();
}
#endif
