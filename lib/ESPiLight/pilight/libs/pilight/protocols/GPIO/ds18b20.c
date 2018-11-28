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
#include <math.h>
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
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "ds18b20.h"

static unsigned short loop = 1;
static unsigned short threads = 0;
static char source_path[21];

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

static void *ds18b20Parse(void *param) {
	struct protocol_threads_t *node = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)node->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;

#ifndef _WIN32
	struct dirent *file = NULL;
	struct stat st;

	DIR *d = NULL;
	FILE *fp = NULL;
	char crcVar[5];
	int w1valid = 0;
	double w1temp = 0.0;
	size_t bytes = 0;
#endif
	char **id = NULL, *stmp = NULL, *content = NULL;
	char *ds18b20_sensor = NULL;
	int nrid = 0, interval = 10, nrloops = 0, y = 0;
	double temp_offset = 0.0, itmp = 0.0;

	threads++;

	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			if(json_find_string(jchild, "id", &stmp) == 0) {
				if((id = REALLOC(id, (sizeof(char *)*(size_t)(nrid+1)))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if((id[nrid] = MALLOC(strlen(stmp)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(id[nrid], stmp);
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
#ifndef _WIN32
			pthread_mutex_lock(&lock);
			for(y=0;y<nrid;y++) {
				if((ds18b20_sensor = REALLOC(ds18b20_sensor, strlen(source_path)+strlen(id[y])+5)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				sprintf(ds18b20_sensor, "%s28-%s/", source_path, id[y]);
				if((d = opendir(ds18b20_sensor))) {
					while((file = readdir(d)) != NULL) {
						if(file->d_type == DT_REG) {
							if(strcmp(file->d_name, "w1_slave") == 0) {
								size_t w1slavelen = strlen(ds18b20_sensor)+10;
								char ds18b20_w1slave[w1slavelen];
								memset(ds18b20_w1slave, '\0', w1slavelen);
								strncpy(ds18b20_w1slave, ds18b20_sensor, strlen(ds18b20_sensor));
								strcat(ds18b20_w1slave, "w1_slave");

								if(!(fp = fopen(ds18b20_w1slave, "rb"))) {
									logprintf(LOG_ERR, "cannot read w1 file: %s", ds18b20_w1slave);
									break;
								}

								fstat(fileno(fp), &st);
								bytes = (size_t)st.st_size;

								if((content = REALLOC(content, bytes+1)) == NULL) {
									fprintf(stderr, "out of memory\n");
									fclose(fp);
									break;
								}
								memset(content, '\0', bytes+1);

								if(fread(content, sizeof(char), bytes, fp) == -1) {
									logprintf(LOG_ERR, "cannot read config file: %s", ds18b20_w1slave);
									fclose(fp);
									break;
								}
								fclose(fp);
								w1valid = 0;

								char **array = NULL;
								unsigned int n = explode(content, "\n", &array);
								if(n > 0) {
									sscanf(array[0], "%*x %*x %*x %*x %*x %*x %*x %*x %*x : crc=%*x %s", crcVar);
									if(strncmp(crcVar, "YES", 3) == 0 && n > 1) {
										w1valid = 1;
										sscanf(array[1], "%*x %*x %*x %*x %*x %*x %*x %*x %*x t=%lf", &w1temp);
										w1temp = (w1temp/1000)+temp_offset;
									}
								}
								array_free(&array, n);

								if(w1valid) {
									ds18b20->message = json_mkobject();

									JsonNode *code = json_mkobject();

									json_append_member(code, "id", json_mkstring(id[y]));
									json_append_member(code, "temperature", json_mknumber(w1temp, 3));

									json_append_member(ds18b20->message, "message", code);
									json_append_member(ds18b20->message, "origin", json_mkstring("receiver"));
									json_append_member(ds18b20->message, "protocol", json_mkstring(ds18b20->id));

									if(pilight.broadcast != NULL) {
										pilight.broadcast(ds18b20->id, ds18b20->message, PROTOCOL);
									}
									json_delete(ds18b20->message);
									ds18b20->message = NULL;
								}
							}
						}
					}
					closedir(d);
				} else {
					logprintf(LOG_ERR, "1-wire device %s does not exist", ds18b20_sensor);
				}
			}
#endif
			pthread_mutex_unlock(&lock);
		}
	}
	pthread_mutex_unlock(&lock);

	if(ds18b20_sensor) {
		FREE(ds18b20_sensor);
	}
	if(content) {
		FREE(content);
	}
	for(y=0;y<nrid;y++) {
		FREE(id[y]);
	}
	FREE(id);
	threads--;
	return (void *)NULL;
}

static struct threadqueue_t *initDev(JsonNode *jdevice) {
	loop = 1;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(ds18b20, json);
	return threads_register("ds18b20", &ds18b20Parse, (void *)node, 0);
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(ds18b20);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(ds18b20);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void ds18b20Init(void) {
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);

	protocol_register(&ds18b20);
	protocol_set_id(ds18b20, "ds18b20");
	protocol_device_add(ds18b20, "ds18b20", "1-wire Temperature Sensor");
	ds18b20->devtype = WEATHER;
	ds18b20->hwtype = SENSOR;

	options_add(&ds18b20->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&ds18b20->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-z0-9]{12}$");

	// options_add(&ds18b20->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)3, "[0-9]");
	options_add(&ds18b20->options, 0, "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&ds18b20->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)3, "[0-9]");
	options_add(&ds18b20->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&ds18b20->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)10, "[0-9]");

	memset(source_path, '\0', 21);
	strcpy(source_path, "/sys/bus/w1/devices/");

	ds18b20->initDev=&initDev;
	ds18b20->threadGC=&threadGC;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "ds18b20";
	module->version = "2.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	ds18b20Init();
}
#endif
