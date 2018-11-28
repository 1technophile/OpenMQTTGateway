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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <libgen.h>
#include <dirent.h>
#ifndef _WIN32
	#include <dlfcn.h>
#endif

#include "../core/threads.h"
#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/options.h"
#include "../core/dso.h"
#include "../core/log.h"
#include "../config/settings.h"

#include "action.h"
#include "actions/action_header.h"

#ifndef _WIN32
void event_action_remove(char *name) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct event_actions_t *currP, *prevP;

	prevP = NULL;

	for(currP = event_actions; currP != NULL; prevP = currP, currP = currP->next) {

		if(strcmp(currP->name, name) == 0) {
			if(prevP == NULL) {
				event_actions = currP->next;
			} else {
				prevP->next = currP->next;
			}

			logprintf(LOG_DEBUG, "removed event action %s", currP->name);
			FREE(currP->name);
			FREE(currP);

			break;
		}
	}
}
#endif

void event_action_init(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	#include "actions/action_init.h"

#ifndef _WIN32
	void *handle = NULL;
	void (*init)(void);
	void (*compatibility)(struct module_t *module);
	char path[PATH_MAX];
	struct module_t module;
	char pilight_version[strlen(PILIGHT_VERSION)+1];
	char pilight_commit[3];
	char *action_root = NULL;
	int check1 = 0, check2 = 0, valid = 1, action_root_free = 0;
	strcpy(pilight_version, PILIGHT_VERSION);

	struct dirent *file = NULL;
	DIR *d = NULL;
	struct stat s;

	memset(pilight_commit, '\0', 3);

	if(settings_find_string("actions-root", &action_root) != 0) {
		/* If no action root was set, use the default action root */
		if((action_root = MALLOC(strlen(ACTION_ROOT)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(action_root, ACTION_ROOT);
		action_root_free = 1;
	}
	size_t len = strlen(action_root);
	if(action_root[len-1] != '/') {
		strcat(action_root, "/");
	}

	if((d = opendir(action_root))) {
		while((file = readdir(d)) != NULL) {
			memset(path, '\0', PATH_MAX);
			sprintf(path, "%s%s", action_root, file->d_name);
			if(stat(path, &s) == 0) {
				/* Check if file */
				if(S_ISREG(s.st_mode)) {
					if(strstr(file->d_name, ".so") != NULL) {
						valid = 1;

						if((handle = dso_load(path)) != NULL) {
							init = dso_function(handle, "init");
							compatibility = dso_function(handle, "compatibility");
							if(init && compatibility) {
								compatibility(&module);
								if(module.name != NULL && module.version != NULL && module.reqversion != NULL) {
									char ver[strlen(module.reqversion)+1];
									strcpy(ver, module.reqversion);

									if((check1 = vercmp(ver, pilight_version)) > 0) {
										valid = 0;
									}

									if(check1 == 0 && module.reqcommit != NULL) {
										char com[strlen(module.reqcommit)+1];
										strcpy(com, module.reqcommit);
										sscanf(HASH, "v%*[0-9].%*[0-9]-%[0-9]-%*[0-9a-zA-Z\n\r]", pilight_commit);

										if(strlen(pilight_commit) > 0 && (check2 = vercmp(com, pilight_commit)) > 0) {
											valid = 0;
										}
									}
									if(valid == 1) {
										char tmp[strlen(module.name)+1];
										strcpy(tmp, module.name);
										event_action_remove(tmp);
										init();
										logprintf(LOG_DEBUG, "loaded event action %s v%s", file->d_name, module.version);
									} else {
										if(module.reqcommit != NULL) {
											logprintf(LOG_ERR, "event action %s requires at least pilight v%s (commit %s)", file->d_name, module.reqversion, module.reqcommit);
										} else {
											logprintf(LOG_ERR, "event action %s requires at least pilight v%s", file->d_name, module.reqversion);
										}
									}
								} else {
									logprintf(LOG_ERR, "invalid module %s", file->d_name);
								}
							}
						}
					}
				}
			}
		}
		closedir(d);
	}
	if(action_root_free) {
		FREE(action_root);
	}
#endif
}

void event_action_register(struct event_actions_t **act, const char *name) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if((*act = MALLOC(sizeof(struct event_actions_t))) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	if(((*act)->name = MALLOC(strlen(name)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy((*act)->name, name);

	(*act)->options = NULL;
	(*act)->run = NULL;
	(*act)->nrthreads = 0;
	(*act)->checkArguments = NULL;

	(*act)->next = event_actions;
	event_actions = (*act);
}

int event_action_gc(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct event_actions_t *tmp_action = NULL;
	while(event_actions) {
		tmp_action = event_actions;
		if(tmp_action->nrthreads > 0) {
			logprintf(LOG_DEBUG, "waiting for \"%s\" threads to finish", tmp_action->name);
		}
		FREE(tmp_action->name);
		options_delete(tmp_action->options);
		event_actions = event_actions->next;
		FREE(tmp_action);
	}
	if(event_actions != NULL) {
		FREE(event_actions);
	}

	logprintf(LOG_DEBUG, "garbage collected event action library");
	return 0;
}

void event_action_thread_init(struct devices_t *dev) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if((dev->action_thread = MALLOC(sizeof(struct event_action_thread_t))) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	pthread_mutexattr_init(&dev->action_thread->attr);
	pthread_mutexattr_settype(&dev->action_thread->attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&dev->action_thread->mutex, &dev->action_thread->attr);
	pthread_cond_init(&dev->action_thread->cond, NULL);
	dev->action_thread->running = 0;
	dev->action_thread->obj = NULL;
	dev->action_thread->action = NULL;
	dev->action_thread->loop = 0;
	dev->action_thread->initialized = 0;
	memset(&dev->action_thread->pth, '\0', sizeof(pthread_t));
}

void event_action_thread_start(struct devices_t *dev, char *name, void *(*func)(void *), struct rules_actions_t *obj) {
	struct event_action_thread_t *thread = dev->action_thread;

	if(thread->running == 1) {
		logprintf(LOG_DEBUG, "aborting previous \"%s\" action for device \"%s\"", thread->action, dev->id);
	}

	thread->loop = 0;

	pthread_mutex_unlock(&thread->mutex);
	pthread_cond_signal(&thread->cond);

	while(thread->running > 0) {
		usleep(10);
	}

	if(thread->initialized == 1) {
		pthread_join(thread->pth, NULL);
		thread->initialized = 0;
	}

	// if(thread->param != NULL) {
		// json_delete(thread->param);
		// thread->param = NULL;
	// }

	// if(param != NULL) {
		// char *json = json_stringify(param, NULL);
		// thread->param = json_decode(json);
		// json_free(json);
	// }

	thread->obj = obj;
	thread->device = dev;
	thread->loop = 1;
	thread->action = REALLOC(thread->action, strlen(name)+1);
	strcpy(thread->action, name);

	thread->initialized = 1;
	threads_create(&thread->pth, NULL, func, (void *)thread);
}

int event_action_thread_wait(struct devices_t *dev, int interval) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct timeval tp;
	struct timespec ts;

	pthread_mutex_unlock(&dev->action_thread->mutex);

	gettimeofday(&tp, NULL);
	ts.tv_sec = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;
	ts.tv_sec += interval;

	pthread_mutex_lock(&dev->action_thread->mutex);

	return pthread_cond_timedwait(&dev->action_thread->cond, &dev->action_thread->mutex, &ts);
}

void event_action_thread_stop(struct devices_t *dev) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct event_action_thread_t *thread = NULL;

	if(dev != NULL) {
		thread = dev->action_thread;
		if(thread->running == 1) {
			logprintf(LOG_DEBUG, "aborting running \"%s\" action for device \"%s\"", thread->action, dev->id);

			thread->loop = 0;
			pthread_mutex_unlock(&thread->mutex);
			pthread_cond_signal(&thread->cond);

			while(thread->running > 0) {
				usleep(10);
			}
		}
		if(thread->initialized == 1) {
			pthread_join(thread->pth, NULL);
			thread->initialized = 0;
		}
	}
}

void event_action_thread_free(struct devices_t *dev) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct event_action_thread_t *thread = NULL;

	if(dev != NULL && dev->action_thread != NULL) {
		thread = dev->action_thread;
		if(thread->running == 1) {
			logprintf(LOG_DEBUG, "aborting running \"%s\" action for device \"%s\"", thread->action, dev->id);

			thread->loop = 0;
			pthread_mutex_unlock(&thread->mutex);
			pthread_cond_signal(&thread->cond);
			while(thread->running > 0) {
				usleep(10);
			}
		}
		if(thread->action != NULL) {
			FREE(thread->action);
		}
		// if(thread->param != NULL) {
			// json_delete(thread->param);
			// thread->param = NULL;
		// }
		if(thread->initialized == 1) {
			pthread_join(thread->pth, NULL);
			thread->initialized = 0;
		}
		FREE(dev->action_thread);
	}
}

void event_action_started(struct event_action_thread_t *thread) {
	logprintf(LOG_INFO, "started \"%s\" action for device \"%s\"", thread->action, thread->device->id);
	pthread_mutex_lock(&thread->mutex);
	thread->running = 1;
	pthread_mutex_unlock(&thread->mutex);
}

void event_action_stopped(struct event_action_thread_t *thread) {
	logprintf(LOG_INFO, "stopped \"%s\" action for device \"%s\"", thread->action, thread->device->id);
	pthread_mutex_lock(&thread->mutex);
	thread->running = 0;
	pthread_mutex_unlock(&thread->mutex);
}
