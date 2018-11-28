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
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
#endif
#include <pthread.h>

#ifndef PILIGHT_DEVELOPMENT
#include <assert.h>
#endif

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/datetime.h" // Full path because we also have a datetime protocol
#include "../../core/log.h"
#include "../../core/http.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "openweathermap.h"

#define INTERVAL 	600

#ifdef PILIGHT_DEVELOPMENT
typedef struct settings_t {
	char *country;
	char *location;
	protocol_threads_t *thread;
	time_t update;
	int ointerval;
	int interval;
	
	struct settings_t *next;
} settings_t;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

static struct settings_t *settings;
static unsigned short loop = 1;
static unsigned short threads = 0;

static void callback(int code, char *data, int size, char *type, void *userdata) {
	struct JsonNode *jdata = NULL;
	struct JsonNode *jmain = NULL;
	struct JsonNode *jsys = NULL;
	struct JsonNode *node = NULL;
	double temp = 0, sunrise = 0, sunset = 0, humi = 0;
	struct settings_t *wnode = userdata;

	time_t timenow = 0;
	struct tm tm;

	if(code == 200) {
		if(strstr(type, "application/json") != NULL) {
			if(json_validate(data) == true) {
				if((jdata = json_decode(data)) != NULL) {
					if((jmain = json_find_member(jdata, "main")) != NULL
						 && (jsys = json_find_member(jdata, "sys")) != NULL) {
						if((node = json_find_member(jmain, "temp")) == NULL) {
							printf("api.openweathermap.org json has no temp key");
						} else if(json_find_number(jmain, "humidity", &humi) != 0) {
							printf("api.openweathermap.org json has no humidity key");
						} else if(json_find_number(jsys, "sunrise", &sunrise) != 0) {
							printf("api.openweathermap.org json has no sunrise key");
						} else if(json_find_number(jsys, "sunset", &sunset) != 0) {
							printf("api.openweathermap.org json has no sunset key");
						} else {
							if(node->tag != JSON_NUMBER) {
								printf("api.openweathermap.org json has no temp key");
							} else {
								temp = node->number_-273.15;

								timenow = time(NULL);
								struct tm current;
								memset(&current, '\0', sizeof(struct tm));
	#ifdef _WIN32
								localtime(&timenow);
	#else
								localtime_r(&timenow, &current);
	#endif

								int month = current.tm_mon+1;
								int mday = current.tm_mday;
								int year = current.tm_year+1900;

								time_t midnight = (datetime2ts(year, month, mday, 23, 59, 59)+1);

								openweathermap->message = json_mkobject();

								JsonNode *code = json_mkobject();

								json_append_member(code, "location", json_mkstring(wnode->location));
								json_append_member(code, "country", json_mkstring(wnode->country));
								json_append_member(code, "temperature", json_mknumber(temp, 2));
								json_append_member(code, "humidity", json_mknumber(humi, 2));
								json_append_member(code, "update", json_mknumber(0, 0));

								time_t a = (time_t)sunrise;
								memset(&tm, '\0', sizeof(struct tm));
	#ifdef _WIN32
								localtime(&a);
	#else
								localtime_r(&a, &tm);
	#endif
								json_append_member(code, "sunrise", json_mknumber((double)((tm.tm_hour*100)+tm.tm_min)/100, 2));

								a = (time_t)sunset;
								memset(&tm, '\0', sizeof(struct tm));
	#ifdef _WIN32
								localtime(&a);
	#else
								localtime_r(&a, &tm);
	#endif
								json_append_member(code, "sunset", json_mknumber((double)((tm.tm_hour*100)+tm.tm_min)/100, 2));
								if(timenow > (int)round(sunrise) && timenow < (int)round(sunset)) {
									json_append_member(code, "sun", json_mkstring("rise"));
								} else {
									json_append_member(code, "sun", json_mkstring("set"));
								}

								json_append_member(openweathermap->message, "message", code);
								json_append_member(openweathermap->message, "origin", json_mkstring("receiver"));
								json_append_member(openweathermap->message, "protocol", json_mkstring(openweathermap->id));

								if(pilight.broadcast != NULL) {
									pilight.broadcast(openweathermap->id, openweathermap->message, PROTOCOL);
								}
								json_delete(openweathermap->message);
								openweathermap->message = NULL;

								/* Send message when sun rises */
								if((int)round(sunrise) > timenow) {
									if(((int)round(sunrise)-timenow) < wnode->ointerval) {
										wnode->interval = (int)((int)round(sunrise)-timenow);
									}
								/* Send message when sun sets */
								} else if((int)round(sunset) > timenow) {
									if(((int)round(sunset)-timenow) < wnode->ointerval) {
										wnode->interval = (int)((int)round(sunset)-timenow);
									}
								/* Update all values when a new day arrives */
								} else {
									if((midnight-timenow) < wnode->ointerval) {
										wnode->interval = (int)(midnight-timenow);
									}
								}

								wnode->update = time(NULL);
							}
						}
					} else {
						logprintf(LOG_NOTICE, "api.openweathermap.org json has no current_observation key");
					}
					json_delete(jdata);
				} else {
					logprintf(LOG_NOTICE, "api.openweathermap.org json could not be parsed");
				}
			}  else {
				logprintf(LOG_NOTICE, "api.openweathermap.org response was not in a valid json format");
			}
		} else {
			logprintf(LOG_NOTICE, "api.openweathermap.org response was not in a valid json format");
		}
	} else {
		logprintf(LOG_NOTICE, "could not reach api.openweathermap.org");
	}
	// if(data) {
		// FREE(data);
	// }
}

static void *openweathermapParse(void *param) {
	struct protocol_threads_t *thread = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)thread->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	struct settings_t *wnode = MALLOC(sizeof(struct settings_t));

	wnode->ointerval = 86400;
	wnode->interval = 86400;

	int event = 0;
	int firstrun = 1, nrloops = 0, timeout = 0;
	double itmp = 0.0;

	char url[1024];

	threads++;

	if(wnode == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	int has_country = 0, has_location = 0;
	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			has_country = 0, has_location = 0;
			jchild1 = json_first_child(jchild);

			while(jchild1) {
				if(strcmp(jchild1->key, "location") == 0) {
					has_location = 1;
					if((wnode->location = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(wnode->location, jchild1->string_);
				}
				if(strcmp(jchild1->key, "country") == 0) {
					has_country = 1;
					if((wnode->country = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(wnode->country, jchild1->string_);
				}
				jchild1 = jchild1->next;
			}
			if(has_country == 1 && has_location == 1) {
				wnode->thread = thread;
				wnode->next = settings;
				settings = wnode;
			} else {
				if(has_country == 1) {
					FREE(wnode->country);
				}
				if(has_location == 1) {
					FREE(wnode->location);
				}
				FREE(wnode);
				wnode = NULL;
			}
			jchild = jchild->next;
		}
	}

	if(wnode == NULL) {
		return 0;
	}

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		wnode->interval = (int)round(itmp);
	wnode->ointerval = wnode->interval;

	while(loop) {
		event = protocol_thread_wait(thread, INTERVAL, &nrloops);
		pthread_mutex_lock(&lock);
		if(loop == 0) {
			break;
		}
		timeout += INTERVAL;
		if(timeout >= wnode->interval || event != ETIMEDOUT || firstrun == 1) {
			timeout = 0;
			wnode->interval = wnode->ointerval;

			sprintf(url, "http://api.openweathermap.org/data/2.5/weather?q=%s,%s&APPID=8db24c4ac56251371c7ea87fd3115493", wnode->location, wnode->country);
			http_get_content(url, callback, wnode);
		} else {
			openweathermap->message = json_mkobject();
			JsonNode *code = json_mkobject();
			json_append_member(code, "location", json_mkstring(wnode->location));
			json_append_member(code, "country", json_mkstring(wnode->country));
			json_append_member(code, "update", json_mknumber(1, 0));
			json_append_member(openweathermap->message, "message", code);
			json_append_member(openweathermap->message, "origin", json_mkstring("receiver"));
			json_append_member(openweathermap->message, "protocol", json_mkstring(openweathermap->id));
			if(pilight.broadcast != NULL) {
				pilight.broadcast(openweathermap->id, openweathermap->message, PROTOCOL);
			}
			json_delete(openweathermap->message);
			openweathermap->message = NULL;
		}
		firstrun = 0;

		pthread_mutex_unlock(&lock);
	}
	pthread_mutex_unlock(&lock);

	threads--;
	return (void *)NULL;
}
#else
static int min_interval = INTERVAL;
static int time_override = -1;
static char url[1024] = "http://api.openweathermap.org/data/2.5/weather?q=%s,%s&APPID=%s";

typedef struct data_t {
	char *name;
	char *country;
	char *location;
	char *api;
	uv_timer_t *enable_timer_req;
	uv_timer_t *update_timer_req;

	int interval;
	double temp_offset;

	time_t update;
	struct data_t *next;
} data_t;

static struct data_t *data = NULL;

static void *update(void *param);
static void *enable(void *param);

#ifdef PILIGHT_REWRITE
static void *reason_code_received_free(void *param) {
	struct reason_code_received_t *data = param;
	FREE(data);
	return NULL;
}
#endif

static void callback(int code, char *data, int size, char *type, void *userdata) {
	struct JsonNode *jdata = NULL;
	struct JsonNode *jmain = NULL;
	struct JsonNode *jsys = NULL;
	struct JsonNode *node = NULL;
	struct data_t *settings = userdata;
	time_t timenow = 0;
	struct tm tm_rise, tm_set;
	double sunset = 0.0, sunrise = 0.0, temp = 0.0, humi = 0.0;

	memset(&tm_rise, 0, sizeof(struct tm));
	memset(&tm_set, 0, sizeof(struct tm));

	if(code == 200) {
		if(strstr(type, "application/json") != NULL) {
			if(json_validate(data) == true) {
				if((jdata = json_decode(data)) != NULL) {
					if((jmain = json_find_member(jdata, "main")) != NULL
						 && (jsys = json_find_member(jdata, "sys")) != NULL) {
						if((node = json_find_member(jmain, "temp")) == NULL) {
							logprintf(LOG_NOTICE, "api.openweathermap.org json has no temp key");
						} else if(json_find_number(jmain, "humidity", &humi) != 0) {
							logprintf(LOG_NOTICE, "api.openweathermap.org json has no humidity key");
						} else if(json_find_number(jsys, "sunrise", &sunrise) != 0) {
							logprintf(LOG_NOTICE, "api.openweathermap.org json has no sunrise key");
						} else if(json_find_number(jsys, "sunset", &sunset) != 0) {
							logprintf(LOG_NOTICE, "api.openweathermap.org json has no sunset key");
						} else {
							if(node->tag != JSON_NUMBER) {
								logprintf(LOG_NOTICE, "api.openweathermap.org json has no temp key");
							} else {
								temp = node->number_-273.15;

								if(time_override > -1) {
									timenow = time_override;
								} else {
									timenow = time(NULL);
								}

								struct tm current;
								memset(&current, '\0', sizeof(struct tm));
#ifdef _WIN32
								struct tm *tmp = gmtime(&timenow);
								memcpy(&current, tmp, sizeof(struct tm));
#else
								gmtime_r(&timenow, &current);
#endif

								int month = current.tm_mon+1;
								int mday = current.tm_mday;
								int year = current.tm_year+1900;

								time_t midnight = (datetime2ts(year, month, mday, 23, 59, 59)+1);

								char *_sun = NULL;

								time_t a = (time_t)sunrise;
								memset(&tm_rise, '\0', sizeof(struct tm));
#ifdef _WIN32
								tmp = gmtime(&a);
								memcpy(&tm_rise, tmp, sizeof(struct tm));
#else
								gmtime_r(&a, &tm_rise);
#endif
								a = (time_t)sunset;
								memset(&tm_set, '\0', sizeof(struct tm));
#ifdef _WIN32
								tmp = gmtime(&a);
								memcpy(&tm_set, tmp, sizeof(struct tm));
#else
								gmtime_r(&a, &tm_set);
#endif
								if(timenow > (int)round(sunrise) && timenow < (int)round(sunset)) {
									_sun = "rise";
								} else {
									_sun = "set";
								}

#ifdef PILIGHT_REWRITE
								struct reason_code_received_t *data = MALLOC(sizeof(struct reason_code_received_t));
								if(data == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								};
								snprintf(data->message, 1024,
									"{\"api\":\"%s\",\"location\":\"%s\",\"country\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"update\":0,\"sunrise\":%.2f,\"sunset\":%.2f,\"sun\":\"%s\"}",
									settings->api, settings->location, settings->country, temp, humi, ((double)((tm_rise.tm_hour*100)+tm_rise.tm_min)/100), ((double)((tm_set.tm_hour*100)+tm_set.tm_min)/100), _sun
								);
								strncpy(data->origin, "receiver", 255);
								data->protocol = openweathermap->id;
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
										"{\"location\":\"%s\",\"country\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"update\":0,\"sunrise\":%.2f,\"sunset\":%.2f,\"sun\":\"%s\"}"\
									"}",
									openweathermap->id, settings->location, settings->country, temp, humi, ((double)((tm_rise.tm_hour*100)+tm_rise.tm_min)/100), ((double)((tm_set.tm_hour*100)+tm_set.tm_min)/100), _sun
								);

								openweathermap->message = json_decode(message);

								if(pilight.broadcast != NULL) {
									pilight.broadcast(openweathermap->id, openweathermap->message, PROTOCOL);
								}
								json_delete(openweathermap->message);
								openweathermap->message = NULL;
#endif
								/* Send message when sun rises */
								if((int)round(sunrise) > timenow) {
									if(((int)round(sunrise)-timenow) < settings->interval) {
										settings->interval = (int)((int)round(sunrise)-timenow);
									}
								/* Send message when sun sets */
								} else if((int)round(sunset) > timenow) {
									if(((int)round(sunset)-timenow) < settings->interval) {
										settings->interval = (int)((int)round(sunset)-timenow);
									}
								/* Update all values when a new day arrives */
								} else {
									if((midnight-timenow) < settings->interval) {
										settings->interval = (int)(midnight-timenow);
									}
								}

								if(time_override > -1) {
									settings->update = time_override;
								} else {
									settings->update = time(NULL);
								}

								/*
								 * Update all values on next event as described above
								 */
								assert(settings->interval > 0);
								uv_timer_start(settings->update_timer_req, (void (*)(uv_timer_t *))update, settings->interval*1000, 0);
								/*
								 * Allow updating the values customly after INTERVAL seconds
								 */
								assert(min_interval > 0);
								uv_timer_start(settings->enable_timer_req, (void (*)(uv_timer_t *))enable, min_interval*1000, 0);
							}
						}
					} else {
						logprintf(LOG_NOTICE, "api.openweathermap.org json has no current_observation key");
					}
					json_delete(jdata);
				} else {
					logprintf(LOG_NOTICE, "api.openweathermap.org json could not be parsed");
				}
			}  else {
				logprintf(LOG_NOTICE, "api.openweathermap.org response was not in a valid json format");
			}
		} else {
			logprintf(LOG_NOTICE, "api.openweathermap.org response was not in a valid json format");
		}
	} else {
		logprintf(LOG_NOTICE, "could not reach api.openweathermap.org");
	}
	return;
}

static void thread_free(uv_work_t *req, int status) {
	FREE(req);
}

static void thread(uv_work_t *req) {
	struct data_t *settings = req->data;
	char parsed[1024];
	char *enc = urlencode(settings->location);

	memset(parsed, '\0', 1024);
	snprintf(parsed, 1024, url, enc, settings->country, settings->api);
	FREE(enc);

	http_get_content(parsed, callback, settings);
	return;
}

static void *update(void *param) {
	uv_timer_t *timer_req = param;
	struct data_t *settings = timer_req->data;

	uv_work_t *work_req = NULL;
	if((work_req = MALLOC(sizeof(uv_work_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	work_req->data = settings;
	uv_queue_work(uv_default_loop(), work_req, "openweathermap", thread, thread_free);
	if(time_override > -1) {
		settings->update = time_override;
	} else {
		settings->update = time(NULL);
	}
	return NULL;
}

static void *enable(void *param) {
	uv_timer_t *timer_req = param;
	struct data_t *settings = timer_req->data;

#ifdef PILIGHT_REWRITE
	struct reason_code_received_t *data = MALLOC(sizeof(struct reason_code_received_t));
	if(data == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	};
	snprintf(data->message, 1024,
		"{\"api\":\"%s\",\"location\":\"%s\",\"country\":\"%s\",\"update\":1}",
		settings->api, settings->location, settings->country
	);
	strncpy(data->origin, "receiver", 255);
	data->protocol = openweathermap->id;
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
			"{\"location\":\"%s\",\"country\":\"%s\",\"update\":1}"\
		"}",
		openweathermap->id, settings->location, settings->country
	);

	openweathermap->message = json_decode(message);

	if(pilight.broadcast != NULL) {
		pilight.broadcast(openweathermap->id, openweathermap->message, PROTOCOL);
	}
	json_delete(openweathermap->message);
	openweathermap->message = NULL;
#endif
	return NULL;
}
#endif

static struct threadqueue_t *initDev(struct JsonNode *jdevice) {
#ifdef PILIGHT_DEVELOPMENT
	loop = 1;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(openweathermap, json);
	return threads_register("openweathermap", &openweathermapParse, (void *)node, 0);
#else
#ifdef PILIGHT_REWRITE
	struct JsonNode *jdevice = NULL;
#endif
	struct JsonNode *jprotocols = NULL;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	struct data_t *node = NULL;
	double itmp = 0;
	int match = 0;

#ifdef PILIGHT_REWRITE
	if(param == NULL) {
		return NULL;
	}
#endif

	if(!(openweathermap->masterOnly == 0 || pilight.runmode == STANDALONE)) {
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
			if(strcmp(openweathermap->id, jchild->string_) == 0) {
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

	node->interval = min_interval;
	node->country = NULL;
	node->location = NULL;
	node->update = 0;
	node->temp_offset = 0;

	int has_country = 0, has_location = 0, has_api = 0;
	if((jid = json_find_member(jdevice, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			has_country = 0, has_location = 0, has_api = 0;
			jchild1 = json_first_child(jchild);

			while(jchild1) {
				if(strcmp(jchild1->key, "api") == 0) {
					has_api = 1;
					if((node->api = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					strcpy(node->api, jchild1->string_);
				}
				if(strcmp(jchild1->key, "location") == 0) {
					has_location = 1;
					if((node->location = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					strcpy(node->location, jchild1->string_);
				}
				if(strcmp(jchild1->key, "country") == 0) {
					has_country = 1;
					if((node->country = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					strcpy(node->country, jchild1->string_);
				}
				jchild1 = jchild1->next;
			}
			if(has_country == 1 && has_location == 1 && has_api == 1) {
				node->next = data;
				data = node;
			} else {
				if(has_country == 1) {
					FREE(node->country);
				}
				if(has_location == 1) {
					FREE(node->location);
				}
				FREE(node);
				node = NULL;
			}
			jchild = jchild->next;
		}
	}

	if(node == NULL) {
		return NULL;
	}
	if(json_find_number(jdevice, "poll-interval", &itmp) == 0)
		node->interval = (int)round(itmp);
	json_find_number(jdevice, "temperature-offset", &node->temp_offset);

	if((node->name = MALLOC(strlen(jdevice->key)+1)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy(node->name, jdevice->key);

	if((node->enable_timer_req = MALLOC(sizeof(uv_timer_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	if((node->update_timer_req = MALLOC(sizeof(uv_timer_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	node->enable_timer_req->data = node;
	node->update_timer_req->data = node;

	uv_timer_init(uv_default_loop(), node->enable_timer_req);
	uv_timer_init(uv_default_loop(), node->update_timer_req);

	assert(node->interval > 0);
	uv_timer_start(node->enable_timer_req, (void (*)(uv_timer_t *))enable, node->interval*1000, 0);
	/*
	 * Do an update when this device is added after 3 seconds
	 * to make sure pilight is actually running.
	 */
	uv_timer_start(node->update_timer_req, (void (*)(uv_timer_t *))update, 3000, 0);
	return NULL;
#endif
}

static int checkValues(JsonNode *code) {
	double interval = INTERVAL;

	json_find_number(code, "poll-interval", &interval);

	if((int)round(interval) < INTERVAL) {
		logprintf(LOG_ERR, "openweathermap poll-interval cannot be lower than %d", INTERVAL);
		return 1;
	}

	return 0;
}

static int createCode(JsonNode *code) {
#ifdef PILIGHT_DEVELOPMENT
	struct settings_t *wtmp = settings;
	char *country = NULL;
	char *location = NULL;
	double itmp = 0;
	time_t currenttime = 0;

	if(json_find_number(code, "min-interval", &itmp) == 0) {
		logprintf(LOG_ERR, "you can't override the min-interval setting");
		return EXIT_FAILURE;
	}

	if(json_find_string(code, "country", &country) == 0 &&
	   json_find_string(code, "location", &location) == 0 &&
	   json_find_number(code, "update", &itmp) == 0) {
		currenttime = time(NULL);
		while(wtmp) {
			if(strcmp(wtmp->country, country) == 0
			   && strcmp(wtmp->location, location) == 0) {
				if((currenttime-wtmp->update) > INTERVAL) {
					pthread_mutex_unlock(&wtmp->thread->mutex);
					pthread_cond_signal(&wtmp->thread->cond);
					wtmp->update = time(NULL);
				}
			}
			wtmp = wtmp->next;
		}
	} else {
		logprintf(LOG_ERR, "openweathermap: insufficient number of arguments");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
#else
	struct data_t *tmp = data;
	char *country = NULL;
	char *location = NULL;
	char *api = NULL;
	double itmp = 0;

	if(json_find_number(code, "min-interval", &itmp) == 0) {
		logprintf(LOG_ERR, "you can't override the min-interval setting");
		return EXIT_FAILURE;
	}

	if(json_find_string(code, "country", &country) == 0 &&
	   json_find_string(code, "location", &location) == 0 &&
		 json_find_string(code, "api", &api) == 0 &&
	   json_find_number(code, "update", &itmp) == 0) {

		time_t currenttime = time(NULL);
		if(time_override > -1) {
			currenttime = time_override;
		}

		while(tmp) {
			if(strcmp(tmp->country, country) == 0
			   && strcmp(tmp->location, location) == 0
				 && strcmp(tmp->api, api) == 0) {
				if((currenttime-tmp->update) > min_interval) {

					uv_work_t *work_req = NULL;
					if((work_req = MALLOC(sizeof(uv_work_t))) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					work_req->data = tmp;
					uv_queue_work(uv_default_loop(), work_req, "openweathermap", thread, thread_free);

					if(time_override > -1) {
						tmp->update = time_override;
					} else {
						tmp->update = time(NULL);
					}
				}
			}
			tmp = tmp->next;
		}
	} else {
		logprintf(LOG_ERR, "openweathermap: insufficient number of arguments");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
#endif
}

static void threadGC(void) {
#ifdef PILIGHT_DEVELOPMENT
	loop = 0;
	protocol_thread_stop(openweathermap);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(openweathermap);


	struct settings_t *wtmp = NULL;
	while(settings) {
		wtmp = settings;
		FREE(settings->country);
		FREE(settings->location);
		settings = settings->next;
		FREE(wtmp);
	}
	if(settings != NULL) {
		FREE(settings);
	}
#else
	struct data_t *tmp = NULL;
	while(data) {
		tmp = data;
#ifndef PILIGHT_REWRITE
		uv_timer_stop(tmp->update_timer_req);
		uv_timer_stop(tmp->enable_timer_req);
#endif
		FREE(tmp->country);
		FREE(tmp->location);
		FREE(tmp->name);
		FREE(tmp->api);
		data = data->next;
		FREE(tmp);
	}
	if(data != NULL) {
		FREE(data);
	}
#endif
}

static void printHelp(void) {
	printf("\t -c --country=country\t\tupdate an entry with this country\n");
	printf("\t -l --location=location\t\tupdate an entry with this location\n");
	printf("\t -a --api=api\t\t\tupdate an entry with this api code\n");
	printf("\t -u --update\t\t\tupdate the defined weather entry\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void openweathermapInit(void) {
#ifdef PILIGHT_DEVELOPMENT
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
#endif

	protocol_register(&openweathermap);
	protocol_set_id(openweathermap, "openweathermap");
	protocol_device_add(openweathermap, "openweathermap", "Open Weather Map API");
	openweathermap->devtype = WEATHER;
	openweathermap->hwtype = API;
	openweathermap->multipleId = 0;
	openweathermap->masterOnly = 1;

	options_add(&openweathermap->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&openweathermap->options, 'h', "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&openweathermap->options, 'l', "location", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^([a-zA-Z-]|[[:space:]])+$");
	options_add(&openweathermap->options, 'c', "country", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-zA-Z]+$");
	options_add(&openweathermap->options, 'a', "api", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-zA-Z0-9]+$");
	options_add(&openweathermap->options, 'x', "sunrise", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{3,4}$");
	options_add(&openweathermap->options, 'y', "sunset", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{3,4}$");
	options_add(&openweathermap->options, 's', "sun", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&openweathermap->options, 'u', "update", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	// options_add(&openweathermap->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&openweathermap->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&openweathermap->options, 0, "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&openweathermap->options, 0, "sunriseset-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&openweathermap->options, 0, "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&openweathermap->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&openweathermap->options, 0, "show-sunriseset", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&openweathermap->options, 0, "show-update", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&openweathermap->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)86400, "[0-9]");

	openweathermap->createCode=&createCode;
	openweathermap->initDev=&initDev;
	openweathermap->checkValues=&checkValues;
	openweathermap->threadGC=&threadGC;
	openweathermap->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "openweathermap";
	module->version = "1.12";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	openweathermapInit();
}
#endif
