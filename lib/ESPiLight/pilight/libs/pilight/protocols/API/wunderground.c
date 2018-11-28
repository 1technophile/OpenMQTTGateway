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
#include <assert.h>
#include <sys/stat.h>
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
#include "../../core/datetime.h" // Full path because we also have a datetime protocol
#include "../../core/log.h"
#include "../../core/http.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "wunderground.h"

#define INTERVAL	900

#ifdef PILIGHT_DEVELOPMENT
typedef struct settings_t {
	char *api;
	char *country;
	char *location;

	struct JsonNode *node;

	char *stmp;
	int interval;
	int ointerval;

	time_t update;
	protocol_threads_t *thread;
	struct settings_t *next;
} settings_t;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

static struct settings_t *settings;
static unsigned short loop = 1;
static unsigned short threads = 0;

static void callback1(int code, char *data, int size, char *type, void *userdata) {
	struct settings_t *wnode = userdata;

	struct JsonNode *jdata1 = NULL;
	struct JsonNode *jsun = NULL;
	struct JsonNode *jsunr = NULL;
	struct JsonNode *jsuns = NULL;

	char *shour = NULL, *smin = NULL;
	char *rhour = NULL, *rmin = NULL;
	
	int humi = 0, temp = 0;

	if(code == 200) {
		if(strcmp(type, "application/json") == 0) {
			if(json_validate(data) == true) {
				if((jdata1 = json_decode(data)) != NULL) {
					if((jsun = json_find_member(jdata1, "sun_phase")) != NULL) {
						if((jsunr = json_find_member(jsun, "sunrise")) != NULL
							 && (jsuns = json_find_member(jsun, "sunset")) != NULL) {
							if(json_find_string(jsuns, "hour", &shour) != 0) {
								printf("api.wunderground.com json has no sunset hour key");
							} else if(json_find_string(jsuns, "minute", &smin) != 0) {
								printf("api.wunderground.com json has no sunset minute key");
							} else if(json_find_string(jsunr, "hour", &rhour) != 0) {
								printf("api.wunderground.com json has no sunrise hour key");
							} else if(json_find_string(jsunr, "minute", &rmin) != 0) {
								printf("api.wunderground.com json has no sunrise minute key");
							} else {
								temp = wnode->node->number_;
								sscanf(wnode->stmp, "%d%%", &humi);

								time_t timenow;
								timenow = time(NULL);

								struct tm current;
								memset(&current, '\0', sizeof(struct tm));
								/*
								 * Retrieving the current day is fine with
								 * the UTC timezone, because we don't do
								 * anything with the hours, minutes or seconds.
								 * We just need to know what day, month, and year
								 * we are in.
								 */
#ifdef _WIN32
								struct tm *ptm;
								ptm = gmtime(&timenow);
								memcpy(&current, ptm, sizeof(struct tm));
#else
								gmtime_r(&timenow, &current);
#endif
								int month = current.tm_mon+1;
								int mday = current.tm_mday;
								int year = current.tm_year+1900;

								time_t midnight = (datetime2ts(year, month, mday, 23, 59, 59)+1);
								time_t sunset = 0;
								time_t sunrise = 0;

								wunderground->message = json_mkobject();

								JsonNode *code = json_mkobject();

								json_append_member(code, "api", json_mkstring(wnode->api));
								json_append_member(code, "location", json_mkstring(wnode->location));
								json_append_member(code, "country", json_mkstring(wnode->country));
								json_append_member(code, "temperature", json_mknumber((double)temp, 2));
								json_append_member(code, "humidity", json_mknumber((double)humi, 0));
								json_append_member(code, "update", json_mknumber(0, 0));
								sunrise = datetime2ts(year, month, mday, atoi(rhour), atoi(rmin), 0);
								json_append_member(code, "sunrise", json_mknumber((double)((atoi(rhour)*100)+atoi(rmin))/100, 2));
								sunset = datetime2ts(year, month, mday, atoi(shour), atoi(smin), 0);
								json_append_member(code, "sunset", json_mknumber((double)((atoi(shour)*100)+atoi(smin))/100, 2));
								if(timenow > sunrise && timenow < sunset) {
									json_append_member(code, "sun", json_mkstring("rise"));
								} else {
									json_append_member(code, "sun", json_mkstring("set"));
								}

								json_append_member(wunderground->message, "message", code);
								json_append_member(wunderground->message, "origin", json_mkstring("receiver"));
								json_append_member(wunderground->message, "protocol", json_mkstring(wunderground->id));

								if(pilight.broadcast != NULL) {
									pilight.broadcast(wunderground->id, wunderground->message, PROTOCOL);
								}
								json_delete(wunderground->message);
								wunderground->message = NULL;
								/* Send message when sun rises */
								if(sunrise > timenow) {
									if((sunrise-timenow) < wnode->ointerval) {
										wnode->interval = (int)(sunrise-timenow);
									}
								/* Send message when sun sets */
								} else if(sunset > timenow) {
									if((sunset-timenow) < wnode->ointerval) {
										wnode->interval = (int)(sunset-timenow);
									}
								/* Update all values when a new day arrives */
								} else {
									if((midnight-timenow) < wnode->ointerval) {
										wnode->interval = (int)(midnight-timenow);
									}
								}

								wnode->update = time(NULL);
							}
						} else {
							logprintf(LOG_NOTICE, "api.wunderground.com json has no sunset and/or sunrise key");
						}
					} else {
						logprintf(LOG_NOTICE, "api.wunderground.com json has no sun_phase key");
					}
					json_delete(jdata1);
				} else {
					logprintf(LOG_NOTICE, "api.wunderground.com json could not be parsed");
				}
			}  else {
				logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
			}
		} else {
			logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
		}
	} else {
		logprintf(LOG_NOTICE, "could not reach api.wundergrond.com");
	}
}

static void callback(int code, char *data, int size, char *type, void *userdata) {
	struct settings_t *wnode = userdata;
	char url[1024];

	struct JsonNode *jdata = NULL;
	struct JsonNode *jobs = NULL;
	// double temp = 0;

	if(code == 200) {
		if(strstr(type, "application/json") != NULL) {
			if(json_validate(data) == true) {
				if((jdata = json_decode(data)) != NULL) {
					if((jobs = json_find_member(jdata, "current_observation")) != NULL) {
						if((wnode->node = json_find_member(jobs, "temp_c")) == NULL) {
							printf("api.wunderground.com json has no temp_c key");
						} else if(json_find_string(jobs, "relative_humidity", &wnode->stmp) != 0) {
							printf("api.wunderground.com json has no relative_humidity key");
						} else {
							if(wnode->node->tag != JSON_NUMBER) {
								printf("api.wunderground.com json has no temp_c key");
							} else {
								sprintf(url, "http://api.wunderground.com/api/%s/astronomy/q/%s/%s.json", wnode->api, wnode->country, wnode->location);
								http_get_content(url, callback1, wnode);
							}
						}
					} else {
						logprintf(LOG_NOTICE, "api.wunderground.com json has no current_observation key");
					}
					json_delete(jdata);
				} else {
					logprintf(LOG_NOTICE, "api.wunderground.com json could not be parsed");
				}
			} else {
				logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
			}
		} else {
			logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
		}
	} else {
		logprintf(LOG_NOTICE, "could not reach api.wundergrond.com");
	}
}

static void *thread(void *param) {
	struct protocol_threads_t *thread = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)thread->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	struct settings_t *wnode = MALLOC(sizeof(struct settings_t));

	wnode->interval = 86400;
	wnode->ointerval = 86400;
	wnode->stmp = NULL;

	int event = 0;
	int firstrun = 1, nrloops = 0, timeout = 0;

	char url[1024];
	double itmp = -1;

	if(wnode == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	threads++;

	int has_country = 0, has_api = 0, has_location = 0;
	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			has_country = 0, has_api = 0, has_location = 0;
			jchild1 = json_first_child(jchild);

			while(jchild1) {
				if(strcmp(jchild1->key, "api") == 0) {
					has_api = 1;
					if((wnode->api = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(wnode->api, jchild1->string_);
				}
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
			if(has_country == 1 && has_api == 1 && has_location == 1) {
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
				if(has_api == 1) {
					FREE(wnode->api);
				}
				FREE(wnode);
				wnode = NULL;
			}
			jchild = jchild->next;
		}
	}

	if(!wnode) {
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

			sprintf(url, "http://api.wunderground.com/api/%s/geolookup/conditions/q/%s/%s.json", wnode->api, wnode->country, wnode->location);
			http_get_content(url, callback, wnode);
		} else {
			wunderground->message = json_mkobject();
			JsonNode *code = json_mkobject();
			json_append_member(code, "api", json_mkstring(wnode->api));
			json_append_member(code, "location", json_mkstring(wnode->location));
			json_append_member(code, "country", json_mkstring(wnode->country));
			json_append_member(code, "update", json_mknumber(1, 0));
			json_append_member(wunderground->message, "message", code);
			json_append_member(wunderground->message, "origin", json_mkstring("receiver"));
			json_append_member(wunderground->message, "protocol", json_mkstring(wunderground->id));
			if(pilight.broadcast != NULL) {
				pilight.broadcast(wunderground->id, wunderground->message, PROTOCOL);
			}
			json_delete(wunderground->message);
			wunderground->message = NULL;
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
static char url[2][1024] = {
	"http://api.wunderground.com/api/%s/geolookup/conditions/q/%s/%s.json",
	"http://api.wunderground.com/api/%s/astronomy/q/%s/%s.json"
};

typedef struct data_t {
	char *country;
	char *location;
	char *key;
	char *api;
	char tz[128];
	uv_timer_t *enable_timer_req;
	uv_timer_t *update_timer_req;

	double temp;
	int humi;

	int interval;
	double temp_offset;

	unsigned long sunrisesetid;
	unsigned long updateid;
	unsigned long initid;

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

static void callback2(int code, char *data, int size, char *type, void *userdata) {
	struct data_t *settings = userdata;
	struct JsonNode *jdata = NULL;
	struct JsonNode *jsun = NULL;
	struct JsonNode *jsunr = NULL;
	struct JsonNode *jsuns = NULL;
	char *shour = NULL, *smin = NULL;
	char *rhour = NULL, *rmin = NULL;
	struct tm tm;

	memset(&tm, 0, sizeof(struct tm));

	if(code == 200) {
		if(strstr(type, "application/json") != NULL) {
			if(json_validate(data) == true) {
				if((jdata = json_decode(data)) != NULL) {
					if((jsun = json_find_member(jdata, "sun_phase")) != NULL) {
						if((jsunr = json_find_member(jsun, "sunrise")) != NULL
							 && (jsuns = json_find_member(jsun, "sunset")) != NULL) {
							if(json_find_string(jsuns, "hour", &shour) != 0) {
								logprintf(LOG_NOTICE, "api.wunderground.com json has no sunset hour key");
							} else if(json_find_string(jsuns, "minute", &smin) != 0) {
								logprintf(LOG_NOTICE, "api.wunderground.com json has no sunset minute key");
							} else if(json_find_string(jsunr, "hour", &rhour) != 0) {
								logprintf(LOG_NOTICE, "api.wunderground.com json has no sunrise hour key");
							} else if(json_find_string(jsunr, "minute", &rmin) != 0) {
								logprintf(LOG_NOTICE, "api.wunderground.com json has no sunrise minute key");
							} else {

								time_t timenow;
								if(time_override > -1) {
									timenow = time_override;
								} else {
									timenow = time(NULL);
								}

								struct tm current;
								memset(&current, '\0', sizeof(struct tm));
								/*
								 * Retrieving the current day is fine with
								 * the UTC timezone, because we don't do
								 * anything with the hours, minutes or seconds.
								 * We just need to know what day, month, and year
								 * we are in.
								 */
#ifdef _WIN32
								struct tm *ptm;
								ptm = gmtime(&timenow);
								memcpy(&current, ptm, sizeof(struct tm));
#else
								gmtime_r(&timenow, &current);
#endif

								int month = current.tm_mon+1;
								int mday = current.tm_mday;
								int year = current.tm_year+1900;
								char *_sun = NULL;

								time_t midnight = (datetime2ts(year, month, mday, 23, 59, 59)+1);
								time_t sunset = datetime2ts(year, month, mday, atoi(shour), atoi(smin), 0);
								time_t sunrise = datetime2ts(year, month, mday, atoi(rhour), atoi(rmin), 0);

								if(timenow > sunrise && timenow < sunset) {
									_sun = "rise";
								} else {
									_sun = "set";
								}

#ifdef PILIGHT_REWRITE
								struct reason_code_received_t *data = MALLOC(sizeof(struct reason_code_received_t));
								if(data == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								snprintf(data->message, 1024,
									"{\"api\":\"%s\",\"location\":\"%s\",\"country\":\"%s\",\"temperature\":%.2f,\"humidity\":%d,\"update\":0,\"sunrise\":%.2f,\"sunset\":%.2f,\"sun\":\"%s\"}",
									settings->api, settings->location, settings->country, settings->temp, settings->humi,
									((double)((atoi(rhour)*100)+atoi(rmin))/100), ((double)((atoi(shour)*100)+atoi(smin))/100), _sun
								);
								strncpy(data->origin, "receiver", 255);
								data->protocol = wunderground->id;
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
										"{\"api\":\"%s\",\"location\":\"%s\",\"country\":\"%s\",\"temperature\":%.2f,\"humidity\":%d,\"update\":0,\"sunrise\":%.2f,\"sunset\":%.2f,\"sun\":\"%s\"}"\
									"}",
									wunderground->id, settings->api, settings->location, settings->country, settings->temp, settings->humi,
									((double)((atoi(rhour)*100)+atoi(rmin))/100), ((double)((atoi(shour)*100)+atoi(smin))/100), _sun
								);

								wunderground->message = json_decode(message);

								if(pilight.broadcast != NULL) {
									pilight.broadcast(wunderground->id, wunderground->message, PROTOCOL);
								}
								json_delete(wunderground->message);
								wunderground->message = NULL;
#endif
								/* Send message when sun rises */
								if(sunrise > timenow) {
									if((sunrise-timenow) < settings->interval) {
										settings->interval = (int)(sunrise-timenow);
									}
								/* Send message when sun sets */
								} else if(sunset > timenow) {
									if((sunset-timenow) < settings->interval) {
										settings->interval = (int)(sunset-timenow);
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
						} else {
							logprintf(LOG_NOTICE, "api.wunderground.com json has no sunset and/or sunrise key");
						}
					} else {
						logprintf(LOG_NOTICE, "api.wunderground.com json has no sun_phase key");
					}
					json_delete(jdata);
				} else {
					logprintf(LOG_NOTICE, "api.wunderground.com json could not be parsed");
				}
			}  else {
				logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
			}
		} else {
			logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
		}
	} else {
		logprintf(LOG_NOTICE, "could not reach api.wunderground.com");
	}
	return;
}

static void callback1(int code, char *data, int size, char *type, void *userdata) {
	struct data_t *settings = userdata;
	struct JsonNode *jdata = NULL;
	struct JsonNode *jobs = NULL;
	struct JsonNode *jloc = NULL;
	struct JsonNode *node = NULL;
	char *stmp = NULL;

	if(code == 200) {
		if(strstr(type, "application/json") != NULL) {
			if(json_validate(data) == true) {
				if((jdata = json_decode(data)) != NULL) {
					if((jloc = json_find_member(jdata, "location")) != NULL) {
						if((node = json_find_member(jloc, "tz_long")) == NULL) {
							logprintf(LOG_NOTICE, "api.wunderground.com json has no tz_long key");
						} else {
							if(node->tag != JSON_STRING) {
								logprintf(LOG_NOTICE, "api.wunderground.com json has no tz_long key");
							} else {
								strcpy(settings->tz, node->string_);
								if((jobs = json_find_member(jdata, "current_observation")) != NULL) {
									if((node = json_find_member(jobs, "temp_c")) == NULL) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no temp_c key");
									} else if(json_find_string(jobs, "relative_humidity", &stmp) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no relative_humidity key");
									} else {
										if(node->tag != JSON_NUMBER) {
											logprintf(LOG_NOTICE, "api.wunderground.com json has no temp_c key");
										} else {
											settings->temp = node->number_;
											sscanf(stmp, "%d%%", &settings->humi);

											char parsed[1024];
											char *enc = urlencode(settings->location);
											memset(parsed, '\0', 1024);
											snprintf(parsed, 1024, url[1], settings->api, settings->country, enc);
											FREE(enc);

											http_get_content(parsed, callback2, userdata);
										}
									}
								} else {
									logprintf(LOG_NOTICE, "api.wunderground.com json has no current_observation key");
								}
							}
						}
					} else {
						logprintf(LOG_NOTICE, "api.wunderground.com json has no location key");
					}
					json_delete(jdata);
				} else {
					logprintf(LOG_NOTICE, "api.wunderground.com json could not be parsed");
				}
			} else {
				logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
			}
		} else {
			logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
		}
	} else {
		logprintf(LOG_NOTICE, "could not reach api.wunderground.com");
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
	snprintf(parsed, 1024, url[0], settings->api, settings->country, enc);
	FREE(enc);

	http_get_content(parsed, callback1, settings);

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
	uv_queue_work(uv_default_loop(), work_req, "wunderground", thread, thread_free);
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
	data->protocol = wunderground->id;
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
			"{\"api\":\"%s\",\"location\":\"%s\",\"country\":\"%s\",\"update\":1}"\
		"}",
		wunderground->id, settings->api, settings->location, settings->country
	);

	wunderground->message = json_decode(message);

	if(pilight.broadcast != NULL) {
		pilight.broadcast(wunderground->id, wunderground->message, PROTOCOL);
	}
	json_delete(wunderground->message);
	wunderground->message = NULL;
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

	struct protocol_threads_t *node = protocol_thread_init(wunderground, json);
	return threads_register("wunderground", &thread, (void *)node, 0);
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

	if(!(wunderground->masterOnly == 0 || pilight.runmode == STANDALONE)) {
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
			if(strcmp(wunderground->id, jchild->string_) == 0) {
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

	node->interval = INTERVAL;
	node->country = NULL;
	node->location = NULL;
	node->update = 0;
	node->temp_offset = 0;

	int has_country = 0, has_location = 0, has_api = 0;
	if((jid = json_find_member(jdevice, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			has_country = 0, has_location = 0;
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

	if((node->key = MALLOC(strlen(jdevice->key)+1)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy(node->key, jdevice->key);
	node->temp = 0.0;
	node->humi = 0;

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
#endif
	return NULL;
}

static int checkValues(JsonNode *code) {
	double interval = INTERVAL;

	json_find_number(code, "poll-interval", &interval);

	if((int)round(interval) < INTERVAL) {
		logprintf(LOG_ERR, "wunderground poll-interval cannot be lower than %d", INTERVAL);
		return 1;
	}

	return 0;
}

static int createCode(JsonNode *code) {
#ifdef PILIGHT_DEVELOPMENT
	struct settings_t *wtmp = settings;
	char *country = NULL;
	char *location = NULL;
	char *api = NULL;
	double itmp = 0;
	time_t currenttime = 0;

	if(json_find_number(code, "min-interval", &itmp) == 0) {
		logprintf(LOG_ERR, "you can't override the min-interval setting");
		return EXIT_FAILURE;
	}

	if(json_find_string(code, "country", &country) == 0 &&
	   json_find_string(code, "location", &location) == 0 &&
	   json_find_string(code, "api", &api) == 0 &&
	   json_find_number(code, "update", &itmp) == 0) {

		while(wtmp) {
			if(strcmp(wtmp->country, country) == 0
			   && strcmp(wtmp->location, location) == 0
			   && strcmp(wtmp->api, api) == 0) {
				if((currenttime-wtmp->update) > INTERVAL) {
					pthread_mutex_unlock(&wtmp->thread->mutex);
					pthread_cond_signal(&wtmp->thread->cond);
					wtmp->update = time(NULL);
				}
			}
			wtmp = wtmp->next;
		}
	} else {
		logprintf(LOG_ERR, "wunderground: insufficient number of arguments");
		return EXIT_FAILURE;
	}
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

		while(tmp) {
			if(strcmp(tmp->country, country) == 0
			   && strcmp(tmp->location, location) == 0
			   && strcmp(tmp->api, api) == 0) {

				time_t currenttime = time(NULL);
				if(time_override > -1) {
					currenttime = time_override;
				}

				if((currenttime-tmp->update) > INTERVAL) {
					uv_work_t *work_req = NULL;
					if((work_req = MALLOC(sizeof(uv_work_t))) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					work_req->data = tmp;
					uv_queue_work(uv_default_loop(), work_req, "wunderground", thread, thread_free);

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
		logprintf(LOG_ERR, "wunderground: insufficient number of arguments");
		return EXIT_FAILURE;
	}
#endif
	return EXIT_SUCCESS;
}

static void threadGC(void) {
#ifdef PILIGHT_DEVELOPMENT
	loop = 0;
	protocol_thread_stop(wunderground);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(wunderground);

	struct settings_t *wtmp = NULL;
	while(settings) {
		wtmp = settings;
		FREE(settings->api);
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
		uv_timer_stop(tmp->enable_timer_req);
		uv_timer_stop(tmp->update_timer_req);
#endif
		FREE(tmp->api);
		FREE(tmp->country);
		FREE(tmp->location);
		FREE(tmp->key);
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
void wundergroundInit(void) {
#ifdef PILIGHT_DEVELOPMENT
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
#endif
	protocol_register(&wunderground);
	protocol_set_id(wunderground, "wunderground");
	protocol_device_add(wunderground, "wunderground", "Weather Underground API");
	wunderground->devtype = WEATHER;
	wunderground->hwtype = API;
	wunderground->multipleId = 0;
	wunderground->masterOnly = 1;

	options_add(&wunderground->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&wunderground->options, 'h', "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&wunderground->options, 'a', "api", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-z0-9]+$");
	options_add(&wunderground->options, 'l', "location", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-z_]+$");
	options_add(&wunderground->options, 'c', "country", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-z]+$");
	options_add(&wunderground->options, 'x', "sunrise", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{3,4}$");
	options_add(&wunderground->options, 'y', "sunset", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{3,4}$");
	options_add(&wunderground->options, 's', "sun", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&wunderground->options, 'u', "update", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	// options_add(&wunderground->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&wunderground->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&wunderground->options, 0, "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&wunderground->options, 0, "sunriseset-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&wunderground->options, 0, "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&wunderground->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&wunderground->options, 0, "show-sunriseset", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&wunderground->options, 0, "show-update", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&wunderground->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)86400, "[0-9]");

	wunderground->createCode=&createCode;
	wunderground->initDev=&initDev;
	wunderground->checkValues=&checkValues;
	wunderground->threadGC=&threadGC;
	wunderground->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "wunderground";
	module->version = "1.13";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	wundergroundInit();
}
#endif
