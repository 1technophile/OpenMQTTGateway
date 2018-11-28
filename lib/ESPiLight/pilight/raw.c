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
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <string.h>
#ifndef _WIN32
#include <wiringx.h>
#endif

#include "libs/pilight/core/threads.h"
#include "libs/pilight/core/pilight.h"
#include "libs/pilight/core/network.h"
#include "libs/pilight/core/config.h"
#include "libs/pilight/core/options.h"
#include "libs/pilight/core/log.h"
#include "libs/pilight/core/datetime.h"
#include "libs/pilight/core/ssdp.h"
#include "libs/pilight/core/socket.h"
#include "libs/pilight/core/threads.h"
#include "libs/pilight/core/dso.h"
#include "libs/pilight/core/gc.h"

#include "libs/pilight/protocols/protocol.h"

#include "libs/pilight/events/events.h"

#include "libs/pilight/config/hardware.h"

#ifndef PILIGHT_DEVELOPMENT
static uv_signal_t *signal_req = NULL;
#endif

static unsigned short main_loop = 1;
static unsigned short linefeed = 0;

int main_gc(void) {
	log_shell_disable();
	main_loop = 0;

	datetime_gc();
	ssdp_gc();
#ifdef EVENTS
	events_gc();
#endif
	options_gc();
	socket_gc();

#ifndef PILIGHT_DEVELOPMENT
	eventpool_gc();
#endif
	config_gc();
	protocol_gc();
	whitelist_free();
	threads_gc();

#ifndef _WIN32
	wiringXGC();
#endif
	dso_gc();
	log_gc();
	gc_clear();

	FREE(progname);
	xfree();

#ifdef _WIN32
	WSACleanup();
#endif

	return EXIT_SUCCESS;
}

void *receiveOOK(void *param) {
	int duration = 0, iLoop = 0;
	long stream_duration = 0;

	struct hardware_t *hw = (hardware_t *)param;
	while(main_loop && hw->receiveOOK) {
		duration = hw->receiveOOK();
		stream_duration += duration;
		iLoop++;
		if(duration > 0) {
			if(linefeed == 1) {
				if(duration > 5100) {
					printf(" %d -#: %d -d: %ld\n%s: ",duration, iLoop, stream_duration, hw->id);
					iLoop = 0;
				} else {
					printf(" %d", duration);
				}
			} else {
				printf("%s: %d\n", hw->id, duration);
			}
		}
	};
	return NULL;
}

void *receivePulseTrain(void *param) {
	struct rawcode_t r;
	int i = 0;

	struct hardware_t *hw = (hardware_t *)param;
	while(main_loop && hw->receivePulseTrain) {
		hw->receivePulseTrain(&r);
		if(r.length == -1) {
			main_gc();
			break;
		} else if(r.length > 0) {
			for(i=0;i<r.length;i++) {
				if(linefeed == 1) {
					printf(" %d", r.pulses[i]);
					if(r.pulses[i] > 5100) {
						printf(" -# %d\n %s:", i, hw->id);
					}
				} else {
					printf("%s: %d\n", hw->id, r.pulses[i]);
				}
			}
		}
	};
	return NULL;
}

static void *receivePulseTrain1(int reason, void *param) {
	struct reason_received_pulsetrain_t *data = param;
	int i = 0;

	if(data->hardware != NULL && data->pulses != NULL && data->length > 0) {
#ifndef PILIGHT_REWRITE
		char *id = NULL;
		struct conf_hardware_t *tmp_confhw = conf_hardware;
		while(tmp_confhw) {
			if(strcmp(tmp_confhw->hardware->id, data->hardware) == 0) {
				id = tmp_confhw->hardware->id;
			}
			tmp_confhw = tmp_confhw->next;
		}
#endif
#ifdef PILIGHT_REWRITE
		struct hardware_t *hw = NULL;
		if(hardware_select_struct(ORIGIN_MASTER, data->hardware, &hw) == 0) {
#endif
			if(data->length > 0) {
				for(i=0;i<data->length;i++) {
					if(linefeed == 1) {
						printf(" %d", data->pulses[i]);
						if(data->pulses[i] > 5100) {
							printf(" -# %d\n %s:", i, id);
						}
					} else {
						printf("%s: %d\n", id, data->pulses[i]);
					}
				}
			}
#ifdef PILIGHT_REWRITE
		}
#endif
	}

	return (void *)NULL;
}

#ifndef PILIGHT_DEVELOPMENT
static void signal_cb(uv_signal_t *handle, int signum) {
	uv_stop(uv_default_loop());
	main_gc();
}

static void close_cb(uv_handle_t *handle) {
	FREE(handle);
}

static void walk_cb(uv_handle_t *handle, void *arg) {
	if(!uv_is_closing(handle)) {
		uv_close(handle, close_cb);
	}
}

static void main_loop1(int onclose) {
	if(onclose == 1) {
		signal_cb(NULL, SIGINT);
	}
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_walk(uv_default_loop(), walk_cb, NULL);
	uv_run(uv_default_loop(), UV_RUN_ONCE);

	if(onclose == 1) {
		while(uv_loop_close(uv_default_loop()) == UV_EBUSY) {
			usleep(10);
		}
	}
}
#endif

int main(int argc, char **argv) {
#ifndef PILIGHT_DEVELOPMENT
	const uv_thread_t pth_cur_id = uv_thread_self();
	memcpy((void *)&pth_main_id, &pth_cur_id, sizeof(uv_thread_t));
#endif

	// memtrack();

	atomicinit();
	struct options_t *options = NULL;
	char *args = NULL;
	char *configtmp = MALLOC(strlen(CONFIG_FILE)+1);
	pid_t pid = 0;

	strcpy(configtmp, CONFIG_FILE);

	gc_attach(main_gc);

	/* Catch all exit signals for gc */
	gc_catch();

	if((progname = MALLOC(12)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(progname, "pilight-raw");

#ifndef PILIGHT_DEVELOPMENT
	if((signal_req = MALLOC(sizeof(uv_signal_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	uv_signal_init(uv_default_loop(), signal_req);
	uv_signal_start(signal_req, signal_cb, SIGINT);
#endif

#ifndef _WIN32
	if(geteuid() != 0) {
		printf("%s requires root privileges in order to run\n", progname);
		FREE(progname);
		exit(EXIT_FAILURE);
	}
#endif

	log_shell_enable();
	log_file_disable();
	log_level_set(LOG_NOTICE);

	options_add(&options, 'H', "help", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'V', "version", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'C', "config", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'L', "linefeed", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);

	while (1) {
		int c;
		c = options_parse(&options, argc, argv, 1, &args);
		if(c == -1)
			break;
		if(c == -2)
			c = 'H';
		switch (c) {
			case 'H':
				printf("Usage: %s [options]\n", progname);
				printf("\t -H --help\t\tdisplay usage summary\n");
				printf("\t -V --version\t\tdisplay version\n");
 				printf("\t -L --linefeed\t\tstructure raw printout\n");
 				printf("\t -C --config\t\tconfig file\n");
				goto close;
			break;
			case 'L':
				linefeed = 1;
			break;
			case 'V':
				printf("%s v%s\n", progname, PILIGHT_VERSION);
				goto close;
			break;
			case 'C':
				configtmp = REALLOC(configtmp, strlen(args)+1);
				strcpy(configtmp, args);
			break;
			default:
				printf("Usage: %s [options]\n", progname);
				goto close;
			break;
		}
	}
	options_delete(options);

#ifdef _WIN32
	if((pid = check_instances(L"pilight-raw")) != -1) {
		logprintf(LOG_NOTICE, "pilight-raw is already running");
		goto close;
	}
#endif

	if((pid = isrunning("pilight-daemon")) != -1) {
		logprintf(LOG_NOTICE, "pilight-daemon instance found (%d)", (int)pid);
		goto close;
	}

	if((pid = isrunning("pilight-debug")) != -1) {
		logprintf(LOG_NOTICE, "pilight-debug instance found (%d)", (int)pid);
		goto close;
	}

	if(config_set_file(configtmp) == EXIT_FAILURE) {
		FREE(configtmp);
		return EXIT_FAILURE;
	}

#ifndef PILIGHT_DEVELOPMENT
	eventpool_init(EVENTPOOL_THREADED);
#endif
	protocol_init();
	config_init();
	if(config_read() != EXIT_SUCCESS) {
		FREE(configtmp);
		goto close;
	}
	FREE(configtmp);

#ifndef PILIGHT_DEVELOPMENT
	eventpool_callback(REASON_RECEIVED_PULSETRAIN, receivePulseTrain1);
#endif

	/* Start threads library that keeps track of all threads used */
	threads_start();

	struct conf_hardware_t *tmp_confhw = conf_hardware;
	while(tmp_confhw) {
		if(tmp_confhw->hardware->init) {
			if(tmp_confhw->hardware->comtype == COMOOK) {
				tmp_confhw->hardware->maxrawlen = 1024;
				tmp_confhw->hardware->minrawlen = 0;
				tmp_confhw->hardware->maxgaplen = 99999;
				tmp_confhw->hardware->mingaplen = 0;
			}
			if(tmp_confhw->hardware->init() == EXIT_FAILURE) {
				logprintf(LOG_ERR, "could not initialize %s hardware mode", tmp_confhw->hardware->id);
				goto close;
			}
			if(tmp_confhw->hardware->comtype == COMOOK) {
#ifdef PILIGHT_DEVELOPMENT
					// threads_register(tmp_confhw->hardware->id, &receiveOOK, (void *)tmp_confhw->hardware, 0);
#endif
			} else if(tmp_confhw->hardware->comtype == COMPLSTRAIN) {
				threads_register(tmp_confhw->hardware->id, &receivePulseTrain, (void *)tmp_confhw->hardware, 0);
			}
		}
		tmp_confhw = tmp_confhw->next;
	}

#ifdef PILIGHT_DEVELOPMENT
	while(main_loop) {
		sleep(1);
	}
#else
	main_loop1(0);
#endif

close:
	if(args != NULL) {
		FREE(args);
	}
#ifdef PILIGHT_DEVELOPMENT
	if(main_loop == 1) {
		main_gc();
	}
#else
	main_loop1(1);
	main_gc();
#endif
	return (EXIT_FAILURE);
}
