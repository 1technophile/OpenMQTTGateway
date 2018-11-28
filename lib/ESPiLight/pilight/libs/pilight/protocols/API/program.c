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
#include <signal.h>
#include <math.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <sys/wait.h>
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
#include "program.h"

#ifndef _WIN32
static unsigned short loop = 1;
static unsigned short threads = 0;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

typedef struct settings_t {
	char *name;
	char *arguments;
	char *program;
	char *start;
	char *stop;
	int wait;
	int currentstate;
	int laststate;
	pthread_t pth;
	int hasthread;
	protocol_threads_t *thread;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static void *thread(void *param) {
	struct protocol_threads_t *pnode = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)pnode->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	char *prog = NULL, *args = NULL, *stopcmd = NULL, *startcmd = NULL;

	int interval = 1, nrloops = 0;
	int pid = 0;
	double itmp = 0;

	threads++;

	json_find_string(json, "program", &prog);
	json_find_string(json, "arguments", &args);
	json_find_string(json, "stop-command", &stopcmd);
	json_find_string(json, "start-command", &startcmd);

	struct settings_t *lnode = MALLOC(sizeof(struct settings_t));
	lnode->wait = 0;
	lnode->hasthread = 0;
	memset(&lnode->pth, '\0', sizeof(pthread_t));

	if(args != NULL && strlen(args) > 0) {
		if((lnode->arguments = MALLOC(strlen(args)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->arguments, args);
	} else {
		lnode->arguments = NULL;
	}

	if(prog != NULL) {
		if((lnode->program = MALLOC(strlen(prog)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->program, prog);
	} else {
		lnode->program = NULL;
	}

	if(stopcmd != NULL) {
		if((lnode->stop = MALLOC(strlen(stopcmd)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->stop, stopcmd);
	} else {
		lnode->stop = NULL;
	}

	if(startcmd != NULL) {
		if((lnode->start = MALLOC(strlen(startcmd)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->start, startcmd);
	} else {
		lnode->start = NULL;
	}

	lnode->name = NULL;
	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);
			while(jchild1) {
				if(strcmp(jchild1->key, "name") == 0) {
					if((lnode->name = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(lnode->name, jchild1->string_);
				}
				jchild1 = jchild1->next;
			}
			jchild = jchild->next;
		}
	}

	lnode->thread = pnode;
	lnode->laststate = -1;

	lnode->next = settings;
	settings = lnode;

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		interval = (int)round(itmp);

	while(loop) {
		if(protocol_thread_wait(pnode, interval, &nrloops) == ETIMEDOUT) {
			pthread_mutex_lock(&lock);
			if(lnode->wait == 0) {
				struct JsonNode *message = json_mkobject();

				JsonNode *code = json_mkobject();
				json_append_member(code, "name", json_mkstring(lnode->name));

				if((pid = (int)findproc(lnode->program, lnode->arguments, 0)) > 0) {
					lnode->currentstate = 1;
					json_append_member(code, "state", json_mkstring("running"));
					json_append_member(code, "pid", json_mknumber((int)pid, 0));
				} else {
					lnode->currentstate = 0;
					json_append_member(code, "state", json_mkstring("stopped"));
					json_append_member(code, "pid", json_mknumber(0, 0));
				}
				json_append_member(message, "message", code);
				json_append_member(message, "origin", json_mkstring("receiver"));
				json_append_member(message, "protocol", json_mkstring(program->id));

				if(lnode->currentstate != lnode->laststate) {
					lnode->laststate = lnode->currentstate;
					if(pilight.broadcast != NULL) {
						pilight.broadcast(program->id, message, PROTOCOL);
					}
				}
				json_delete(message);
				message = NULL;
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

	struct protocol_threads_t *node = protocol_thread_init(program, json);
	return threads_register("program", &thread, (void *)node, 0);
}

static void *execute(void *param) {
	struct settings_t *p = (struct settings_t *)param;
	int pid = 0;
	int result = 0;

	if((pid = (int)findproc(p->program, p->arguments, 0)) > 0) {
		result = system(p->stop);
	} else {
		result = system(p->start);
	}

	/* Check of the user wanted to stop pilight */
	if(WIFSIGNALED(result)) {
		int ppid = 0;
		/* Find the pilight daemon pid */
		if((ppid = (int)findproc(progname, NULL, 0)) > 0) {
			/* Send a sigint to ourself */
			kill(ppid, SIGINT);
		}
	}

	p->wait = 0;
	memset(&p->pth, '\0', sizeof(pthread_t));
	p->hasthread = 0;
	p->laststate = -1;

	pthread_mutex_unlock(&p->thread->mutex);
	pthread_cond_signal(&p->thread->cond);

	return NULL;
}

static int createCode(JsonNode *code) {
	char *name = NULL;
	double itmp = -1;
	int state = -1;
	int pid = 0;

	if(json_find_string(code, "name", &name) == 0) {
		if(strstr(progname, "daemon") != NULL) {
			struct settings_t *tmp = settings;
			while(tmp) {
				if(strcmp(tmp->name, name) == 0) {
					if(tmp->wait == 0) {
						if(tmp->name != NULL && tmp->stop != NULL && tmp->start != NULL) {

							if(json_find_number(code, "running", &itmp) == 0)
								state = 1;
							else if(json_find_number(code, "stopped", &itmp) == 0)
								state = 0;

							if((pid = (int)findproc(tmp->program, tmp->arguments, 0)) > 0 && state == 1) {
								logprintf(LOG_INFO, "program \"%s\" already running", tmp->name);
							} else if(pid == -1 && state == 0) {
								logprintf(LOG_INFO, "program \"%s\" already stopped", tmp->name);
								break;
							} else {
								if(state > -1) {
									program->message = json_mkobject();
									JsonNode *code1 = json_mkobject();
									json_append_member(code1, "name", json_mkstring(name));

									if(state == 1) {
										json_append_member(code1, "state", json_mkstring("running"));
									} else {
										json_append_member(code1, "state", json_mkstring("stopped"));
									}

									json_append_member(program->message, "message", code1);
									json_append_member(program->message, "origin", json_mkstring("receiver"));
									json_append_member(program->message, "protocol", json_mkstring(program->id));

									if(pilight.broadcast != NULL) {
										pilight.broadcast(program->id, program->message, PROTOCOL);
									}
									json_delete(program->message);
									program->message = NULL;
								}

								tmp->wait = 1;
								threads_create(&tmp->pth, NULL, execute, (void *)tmp);
								tmp->hasthread = 1;
								pthread_detach(tmp->pth);

								program->message = json_mkobject();
								json_append_member(program->message, "name", json_mkstring(name));
								json_append_member(program->message, "state", json_mkstring("pending"));
							}
						} else {
							logprintf(LOG_NOTICE, "program \"%s\" cannot be controlled", tmp->name);
						}
					} else {
						logprintf(LOG_NOTICE, "please wait for program \"%s\" to finish it's state change", tmp->name);
					}
					break;
				}
				tmp = tmp->next;
			}
		} else {
			program->message = json_mkobject();

			if(json_find_number(code, "running", &itmp) == 0)
				json_append_member(program->message, "state", json_mkstring("running"));
			else if(json_find_number(code, "stopped", &itmp) == 0)
				json_append_member(program->message, "state", json_mkstring("stopped"));

			json_append_member(program->message, "name", json_mkstring(name));
			json_append_member(program->message, "state", json_mkstring("pending"));
		}
	} else {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(program);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(program);

	struct settings_t *tmp;
	while(settings) {
		tmp = settings;
		if(tmp->stop) FREE(tmp->stop);
		if(tmp->start) FREE(tmp->start);
		if(tmp->name) FREE(tmp->name);
		if(tmp->arguments) FREE(tmp->arguments);
		if(tmp->program) FREE(tmp->program);
		if(tmp->hasthread > 0) pthread_cancel(tmp->pth);
		settings = settings->next;
		FREE(tmp);
	}
	if(settings != NULL) {
		FREE(settings);
	}
}

static void printHelp(void) {
	printf("\t -t --running\t\t\tstart the program\n");
	printf("\t -f --stopped\t\t\tstop the program\n");
	printf("\t -n --name=name\t\t\tname of the program\n");
}
#endif

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void programInit(void) {
#ifndef _WIN32
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
#endif

	protocol_register(&program);
	protocol_set_id(program, "program");
	protocol_device_add(program, "program", "Start / Stop / State of a program");
	program->devtype = PENDINGSW;
	program->hwtype = API;
	program->multipleId = 0;

	options_add(&program->options, 'n', "name", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, NULL);
	options_add(&program->options, 'x', "start-command", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&program->options, 'y', "stop-command", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&program->options, 'p', "program", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&program->options, 'i', "pid", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&program->options, 'a', "arguments", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&program->options, 't', "running", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&program->options, 'd', "pending", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&program->options, 'f', "stopped", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&program->options, 0, "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&program->options, 0, "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&program->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");

#ifndef _WIN32
	program->createCode=&createCode;
	program->printHelp=&printHelp;
	program->initDev=&initDev;
	program->threadGC=&threadGC;
#endif
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "program";
	module->version = "1.6";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	programInit();
}
#endif
