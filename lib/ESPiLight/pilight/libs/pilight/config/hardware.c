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
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#ifndef _WIN32
	#include <wiringx.h>
	#include <regex.h>
	#include <sys/ioctl.h>
	#include <dlfcn.h>
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <pthread.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <dirent.h>

#include "../core/threads.h"
#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/log.h"
#include "../core/json.h"
#include "../core/dso.h"
#include "settings.h"
#include "hardware.h"


static char *hwfile = NULL;
struct hardware_t *hardware;
struct conf_hardware_t *conf_hardware;

#include "../hardware/hardware_header.h"

#ifndef _WIN32
static void hardware_remove(char *name) {
	struct hardware_t *currP, *prevP;

	prevP = NULL;

	for(currP = hardware; currP != NULL; prevP = currP, currP = currP->next) {

		if(strcmp(currP->id, name) == 0) {
			if(prevP == NULL) {
				hardware = currP->next;
			} else {
				prevP->next = currP->next;
			}

			logprintf(LOG_DEBUG, "removed config hardware module %s", currP->id);
			FREE(currP->id);
			options_delete(currP->options);
			FREE(currP);

			break;
		}
	}
}
#endif

void hardware_register(struct hardware_t **hw) {
	if((*hw = MALLOC(sizeof(struct hardware_t))) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	(*hw)->options = NULL;
	(*hw)->wait = 0;
	(*hw)->stop = 0;
	(*hw)->running = 0;
	(*hw)->minrawlen = 0;
	(*hw)->maxrawlen = 0;
	(*hw)->mingaplen = 0;
	(*hw)->maxgaplen = 0;

	(*hw)->init = NULL;
	(*hw)->deinit = NULL;
	(*hw)->receiveOOK = NULL;
	(*hw)->receivePulseTrain = NULL;
	(*hw)->receiveAPI = NULL;
	(*hw)->sendOOK = NULL;
	(*hw)->sendAPI = NULL;
	(*hw)->gc = NULL;
	(*hw)->settings = NULL;

	pthread_mutexattr_init(&(*hw)->attr);
	pthread_mutexattr_settype(&(*hw)->attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&(*hw)->lock, &(*hw)->attr);
	pthread_cond_init(&(*hw)->signal, NULL);

	(*hw)->next = hardware;
	hardware = (*hw);
}

void hardware_set_id(hardware_t *hw, const char *id) {
	if((hw->id = MALLOC(strlen(id)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(hw->id, id);
}

static int hardware_gc(void) {
	struct hardware_t *htmp = hardware;
	struct conf_hardware_t *ctmp = NULL;

	while(hardware) {
		htmp = hardware;
		htmp->stop = 1;
		pthread_mutex_unlock(&htmp->lock);
		pthread_cond_signal(&htmp->signal);
		while(htmp->running == 1) {
			usleep(10);
		}
		thread_stop(htmp->id);
		if(htmp->deinit != NULL) {
			htmp->deinit();
		}
		if(htmp->gc != NULL) {
			htmp->gc();
		}
		FREE(htmp->id);
		options_delete(htmp->options);
		hardware = hardware->next;
		FREE(htmp);
	}
	if(hardware != NULL) {
		FREE(hardware);
	}

	while(conf_hardware) {
		ctmp = conf_hardware;
		conf_hardware = conf_hardware->next;
		FREE(ctmp);
	}

	if(hwfile != NULL) {
		FREE(hwfile);
	}

	logprintf(LOG_DEBUG, "garbage collected config hardware library");
	return EXIT_SUCCESS;
}

static JsonNode *hardware_sync(int level, const char *display) {
	struct conf_hardware_t *tmp = conf_hardware;
	struct JsonNode *root = json_mkobject();
	while(tmp) {
		struct JsonNode *module = json_mkobject();
		struct options_t *options = tmp->hardware->options;
		while(options) {
			if(options->vartype == JSON_NUMBER) {
				json_append_member(module, options->name, json_mknumber(options->number_, 0));
			} else if(options->vartype == JSON_STRING) {
				json_append_member(module, options->name, json_mkstring(options->string_));
			}
			options = options->next;
		}
		json_append_member(root, tmp->hardware->id, module);
		tmp = tmp->next;
	}

	return root;
}

static int hardware_parse(JsonNode *root) {
	struct conf_hardware_t *hnode = NULL;
	struct conf_hardware_t *tmp_confhw = NULL;
	struct options_t *hw_options = NULL;
	struct hardware_t *tmp_hardware = NULL;
	struct hardware_t *hw = NULL;

	JsonNode *jvalues = NULL;
	JsonNode *jchilds = json_first_child(root);

	int i = 0, have_error = 0, match = 0;

	while(jchilds) {
		i++;
		/* A hardware module can only be a JSON object */
		if(jchilds->tag != JSON_OBJECT) {
			logprintf(LOG_ERR, "config hardware module #%d \"%s\", invalid format", i, jchilds->key);
			have_error = 1;
			goto clear;
		} else {
			/* Check if defined hardware module exists */
			tmp_hardware = hardware;
			match = 0;
			while(tmp_hardware) {
				if(strcmp(tmp_hardware->id, jchilds->key) == 0) {
					hw = tmp_hardware;
					match = 1;
					break;
				}
				tmp_hardware = tmp_hardware->next;
			}
			if(match == 0) {
				logprintf(LOG_ERR, "config hardware module #%d \"%s\" does not exist", i, jchilds->key);
				have_error = 1;
				goto clear;
			}

			/* Check for duplicate hardware modules */
			tmp_confhw = conf_hardware;
			while(tmp_confhw) {
				/* Only allow one module of the same name */
				if(strcmp(tmp_confhw->hardware->id, jchilds->key) == 0) {
					logprintf(LOG_ERR, "config hardware module #%d \"%s\", duplicate", i, jchilds->key);
					have_error = 1;
					goto clear;
				}
				/* And only allow one module covering the same frequency */
				if(tmp_confhw->hardware->hwtype == hw->hwtype) {
					logprintf(LOG_ERR, "config hardware module #%d \"%s\", duplicate freq.", i, jchilds->key);
					have_error = 1;
					goto clear;
				}
				tmp_confhw = tmp_confhw->next;
			}

			/* Check if all options required by the hardware module are present */
			hw_options = hw->options;
			while(hw_options) {
				match = 0;
				jvalues = json_first_child(jchilds);
				while(jvalues) {
					if(jvalues->tag == JSON_NUMBER || jvalues->tag == JSON_STRING) {
						if(strcmp(jvalues->key, hw_options->name) == 0 && hw_options->argtype == OPTION_HAS_VALUE) {
							match = 1;
							break;
						}
					}
					jvalues = jvalues->next;
				}
				if(match == 0) {
					logprintf(LOG_ERR, "config hardware module #%d \"%s\", setting \"%s\" missing", i, jchilds->key, hw_options->name);
					have_error = 1;
					goto clear;
				} else {
					/* Check if setting contains a valid value */
#if !defined(__FreeBSD__) && !defined(_WIN32)
					regex_t regex;
					int reti;
					char *stmp = NULL;

					if(jvalues->tag == JSON_NUMBER) {
						if((stmp = REALLOC(stmp, sizeof(jvalues->number_))) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						sprintf(stmp, "%d", (int)jvalues->number_);
					} else if(jvalues->tag == JSON_STRING) {
						if((stmp = REALLOC(stmp, strlen(jvalues->string_)+1)) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						strcpy(stmp, jvalues->string_);
					}
					if(hw_options->mask != NULL) {
						reti = regcomp(&regex, hw_options->mask, REG_EXTENDED);
						if(reti) {
							logprintf(LOG_ERR, "could not compile regex");
							exit(EXIT_FAILURE);
						}
						reti = regexec(&regex, stmp, 0, NULL, 0);
						if(reti == REG_NOMATCH || reti != 0) {
							logprintf(LOG_ERR, "config hardware module #%d \"%s\", setting \"%s\" invalid", i, jchilds->key, hw_options->name);
							have_error = 1;
							regfree(&regex);
							goto clear;
						}
						regfree(&regex);
					}
					FREE(stmp);
#endif
				}
				hw_options = hw_options->next;
			}

			/* Check for any settings that are not valid for this hardware module */
			jvalues = json_first_child(jchilds);
			while(jvalues) {
				match = 0;
				if(jvalues->tag == JSON_NUMBER || jvalues->tag == JSON_STRING) {
					hw_options = hw->options;
					while(hw_options) {
						if(strcmp(jvalues->key, hw_options->name) == 0 && jvalues->tag == hw_options->vartype) {
							if(hw_options->vartype == JSON_NUMBER) {
								options_set_number(&hw_options, hw_options->id, jvalues->number_);
							} else if(hw_options->vartype == JSON_STRING) {
								options_set_string(&hw_options, hw_options->id, jvalues->string_);
							}
							match = 1;
							break;
						}
						hw_options = hw_options->next;
					}
					if(match == 0) {
						logprintf(LOG_ERR, "config hardware module #%d \"%s\", setting \"%s\" invalid", i, jchilds->key, jvalues->key);
						have_error = 1;
						goto clear;
					}
				}
				jvalues = jvalues->next;
			}

			if(hw->settings != NULL) {
				/* Sync all settings with the hardware module */
				jvalues = json_first_child(jchilds);
				while(jvalues) {
					if(hw->settings(jvalues) == EXIT_FAILURE) {
						logprintf(LOG_ERR, "config hardware module #%d \"%s\", setting \"%s\" invalid", i, jchilds->key, jvalues->key);
						have_error = 1;
						goto clear;
					}
					jvalues = jvalues->next;
				}
			}

			if((hnode = MALLOC(sizeof(struct conf_hardware_t))) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			hnode->hardware = hw;
			hnode->next = conf_hardware;
			conf_hardware = hnode;
		}
		jchilds = jchilds->next;
	}

	if(tmp_confhw != NULL) {
		FREE(tmp_confhw);
	}
clear:
	return have_error;
}

void hardware_init(void) {
	/* Request hardware json object in main configuration */
	config_register(&config_hardware, "hardware");
	config_hardware->readorder = 4;
	config_hardware->writeorder = 4;
	config_hardware->parse=&hardware_parse;
	config_hardware->sync=&hardware_sync;
	config_hardware->gc=&hardware_gc;

	#include "../hardware/hardware_init.h"

#ifndef _WIN32
	void *handle = NULL;
	void (*init)(void);
	void (*compatibility)(struct module_t *module);
	char path[PATH_MAX];
	struct module_t module;
	char pilight_version[strlen(PILIGHT_VERSION)+1];
	char pilight_commit[3];
	char *hardware_root = NULL;
	int check1 = 0, check2 = 0, valid = 1, hardware_root_free = 0;
	strcpy(pilight_version, PILIGHT_VERSION);

	struct dirent *file = NULL;
	DIR *d = NULL;
	struct stat s;

	memset(pilight_commit, '\0', 3);

	if(settings_find_string("hardware-root", &hardware_root) != 0) {
		/* If no hardware root was set, use the default hardware root */
		if((hardware_root = MALLOC(strlen(HARDWARE_ROOT)+2)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(hardware_root, HARDWARE_ROOT);
		hardware_root_free = 1;
	}
	size_t len = strlen(hardware_root);

	if(hardware_root[len-1] != '/') {
		strcat(hardware_root, "/");
	}

	if((d = opendir(hardware_root))) {
		while((file = readdir(d)) != NULL) {
			memset(path, '\0', PATH_MAX);
			sprintf(path, "%s%s", hardware_root, file->d_name);
			if(stat(path, &s) == 0) {
				/* Check if file */
				if(S_ISREG(s.st_mode)) {
					if(strstr(file->d_name, ".so") != NULL) {
						valid = 1;

						if((handle = dso_load(path)) != NULL) {
							init = dso_function(handle, "init");
							compatibility = dso_function(handle, "compatibility");
							if(init != NULL && compatibility != NULL ) {
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
										hardware_remove(tmp);
										init();
										logprintf(LOG_DEBUG, "loaded config hardware module %s v%s", file->d_name, module.version);
									} else {
										if(module.reqcommit != NULL) {
											logprintf(LOG_ERR, "config hardware module %s requires at least pilight v%s (commit %s)", file->d_name, module.reqversion, module.reqcommit);
										} else {
											logprintf(LOG_ERR, "config hardware module %s requires at least pilight v%s", file->d_name, module.reqversion);
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
	if(hardware_root_free) {
		FREE(hardware_root);
	}
#endif
}
