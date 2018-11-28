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
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
#endif
#include <pthread.h>

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/dso.h"
#include "../core/options.h"
#include "../core/log.h"

#include "../config/settings.h"

#include "protocol.h"
#include "protocol_header.h"

struct protocols_t *protocols;

#ifndef _WIN32
void protocol_remove(char *name) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct protocols_t *currP, *prevP;

	prevP = NULL;

	for(currP = protocols; currP != NULL; prevP = currP, currP = currP->next) {

		if(strcmp(currP->listener->id, name) == 0) {
			if(prevP == NULL) {
				protocols = currP->next;
			} else {
				prevP->next = currP->next;
			}

			struct protocol_devices_t *dtmp;
			logprintf(LOG_DEBUG, "removed protocol %s", currP->listener->id);
			if(currP->listener->threadGC) {
				currP->listener->threadGC();
				logprintf(LOG_DEBUG, "stopped protocol threads");
			}
			if(currP->listener->gc) {
				currP->listener->gc();
				logprintf(LOG_DEBUG, "ran garbage collector");
			}
			FREE(currP->listener->id);
			options_delete(currP->listener->options);
			if(currP->listener->devices) {
				while(currP->listener->devices) {
					dtmp = currP->listener->devices;
					FREE(dtmp->id);
					FREE(dtmp->desc);
					currP->listener->devices = currP->listener->devices->next;
					FREE(dtmp);
				}
			}
			FREE(currP->listener->devices);
			FREE(currP->listener);
			FREE(currP);

			break;
		}
	}
}
#endif

void protocol_init(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	#include "protocol_init.h"

#ifndef _WIN32
	void *handle = NULL;
	void (*init)(void);
	void (*compatibility)(struct module_t *module);
	char path[PATH_MAX];
	struct module_t module;
	char pilight_version[strlen(PILIGHT_VERSION)+1];
	char pilight_commit[3];
	char *protocol_root = NULL;
	int check1 = 0, check2 = 0, valid = 1, protocol_root_free = 0;
	strcpy(pilight_version, PILIGHT_VERSION);

	struct dirent *file = NULL;
	DIR *d = NULL;
	struct stat s;

	memset(pilight_commit, '\0', 3);

	if(settings_find_string("protocol-root", &protocol_root) != 0) {
		/* If no protocol root was set, use the default protocol root */
		if((protocol_root = MALLOC(strlen(PROTOCOL_ROOT)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(protocol_root, PROTOCOL_ROOT);
		protocol_root_free = 1;
	}
	size_t len = strlen(protocol_root);
	if(protocol_root[len-1] != '/') {
		strcat(protocol_root, "/");
	}

	if((d = opendir(protocol_root))) {
		while((file = readdir(d)) != NULL) {
			memset(path, '\0', PATH_MAX);
			sprintf(path, "%s%s", protocol_root, file->d_name);
			if(stat(path, &s) == 0) {
				/* Check if file */
				if(S_ISREG(s.st_mode)) {
					if(strstr(file->d_name, ".so") != NULL) {
						valid = 1;
						if((handle = dso_load(path)) != NULL) {

							init = dso_function(handle, "init");
							compatibility = dso_function(handle, "compatibility");
							if(init != NULL && compatibility != NULL) {
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
										protocol_remove(tmp);
										init();
										logprintf(LOG_DEBUG, "loaded protocol %s v%s", file->d_name, module.version);
									} else {
										if(module.reqcommit != NULL) {
											logprintf(LOG_ERR, "protocol %s requires at least pilight v%s (commit %s)", file->d_name, module.reqversion, module.reqcommit);
										} else {
											logprintf(LOG_ERR, "protocol %s requires at least pilight v%s", file->d_name, module.reqversion);
										}
									}
								} else {
									logprintf(LOG_ERR, "invalid module %s", file->d_name);
								}
							}
						}
					}
				}
			} else {
				perror("stat");
			}
		}
		closedir(d);
	}
	if(protocol_root_free) {
		FREE(protocol_root);
	}
#endif
}

void protocol_register(protocol_t **proto) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if((*proto = MALLOC(sizeof(struct protocol_t))) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	(*proto)->options = NULL;
	(*proto)->devices = NULL;

	(*proto)->rawlen = 0;
	(*proto)->minrawlen = 0;
	(*proto)->maxrawlen = 0;
	(*proto)->mingaplen = 0;
	(*proto)->maxgaplen = 0;
	(*proto)->txrpt = 10;
	(*proto)->rxrpt = 1;
	(*proto)->hwtype = NONE;
	(*proto)->multipleId = 1;
	(*proto)->config = 1;
	(*proto)->masterOnly = 0;
	(*proto)->parseCode = NULL;
	(*proto)->parseCommand = NULL;
	(*proto)->createCode = NULL;
	(*proto)->checkValues = NULL;
	(*proto)->initDev = NULL;
	(*proto)->printHelp = NULL;
	(*proto)->threadGC = NULL;
	(*proto)->gc = NULL;
	(*proto)->message = NULL;
	(*proto)->threads = NULL;

	(*proto)->repeats = 0;
	(*proto)->first = 0;
	(*proto)->second = 0;

	(*proto)->raw = NULL;

	struct protocols_t *pnode = MALLOC(sizeof(struct protocols_t));
	if(pnode == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	pnode->listener = *proto;
	pnode->next = protocols;
	protocols = pnode;
}

struct protocol_threads_t *protocol_thread_init(protocol_t *proto, struct JsonNode *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct protocol_threads_t *node = MALLOC(sizeof(struct protocol_threads_t));
	if(node == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	node->param = param;
	pthread_mutexattr_init(&node->attr);
	pthread_mutexattr_settype(&node->attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&node->mutex, &node->attr);
	pthread_cond_init(&node->cond, NULL);
	node->next = proto->threads;
	proto->threads = node;

	return node;
}

int protocol_thread_wait(struct protocol_threads_t *node, int interval, int *nrloops) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct timeval tp;
	struct timespec ts;

	pthread_mutex_unlock(&node->mutex);

	gettimeofday(&tp, NULL);
	ts.tv_sec = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;

	if(*nrloops == 0) {
		ts.tv_sec += 1;
		*nrloops = 1;
	} else {
		ts.tv_sec += interval;
	}

	pthread_mutex_lock(&node->mutex);

	return pthread_cond_timedwait(&node->cond, &node->mutex, &ts);
}

void protocol_thread_stop(protocol_t *proto) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(proto != NULL && proto->threads != NULL ) {
		struct protocol_threads_t *tmp = proto->threads;
		while(tmp) {
			pthread_mutex_unlock(&tmp->mutex);
			pthread_cond_signal(&tmp->cond);
			tmp = tmp->next;
		}
	}
}

void protocol_thread_free(protocol_t *proto) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(proto != NULL && proto->threads != NULL) {
		struct protocol_threads_t *tmp;
		while(proto->threads) {
			tmp = proto->threads;
			if(proto->threads->param != NULL) {
				json_delete(proto->threads->param);
			}
			proto->threads = proto->threads->next;
			FREE(tmp);
		}
		if(proto->threads != NULL) {
			FREE(proto->threads);
		}
	}
}

void protocol_set_id(protocol_t *proto, const char *id) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if((proto->id = MALLOC(strlen(id)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(proto->id, id);
}

void protocol_device_add(protocol_t *proto, const char *id, const char *desc) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct protocol_devices_t *dnode = MALLOC(sizeof(struct protocol_devices_t));
	if(dnode == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	if((dnode->id = MALLOC(strlen(id)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(dnode->id, id);
	if((dnode->desc = MALLOC(strlen(desc)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(dnode->desc, desc);
	dnode->next	= proto->devices;
	proto->devices = dnode;
}

int protocol_device_exists(protocol_t *proto, const char *id) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct protocol_devices_t *temp = proto->devices;

	while(temp) {
		if(strcmp(temp->id, id) == 0) {
			return 0;
		}
		temp = temp->next;
	}
	if(temp != NULL) {
		FREE(temp);
	}

	return 1;
}

int protocol_gc(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct protocols_t *ptmp;
	struct protocol_devices_t *dtmp;

	while(protocols) {
		ptmp = protocols;
		logprintf(LOG_DEBUG, "protocol %s", ptmp->listener->id);
		if(ptmp->listener->threadGC != NULL) {
			ptmp->listener->threadGC();
			logprintf(LOG_DEBUG, "stopped protocol threads");
		}
		if(ptmp->listener->gc) {
			ptmp->listener->gc();
			logprintf(LOG_DEBUG, "ran garbage collector");
		}
		FREE(ptmp->listener->id);
		options_delete(ptmp->listener->options);
		if(ptmp->listener->devices) {
			while(ptmp->listener->devices) {
				dtmp = ptmp->listener->devices;
				FREE(dtmp->id);
				FREE(dtmp->desc);
				ptmp->listener->devices = ptmp->listener->devices->next;
				if(dtmp != NULL) {
					FREE(dtmp);
				}
			}
		}
		if(ptmp->listener->devices != NULL) {
			FREE(ptmp->listener->devices);
		}
		FREE(ptmp->listener);
		protocols = protocols->next;
		FREE(ptmp);
	}
	if(protocols != NULL) {
		FREE(protocols);
	}

	logprintf(LOG_DEBUG, "garbage collected protocol library");
	return EXIT_SUCCESS;
}
