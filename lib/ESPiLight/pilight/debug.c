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
#include "libs/pilight/core/log.h"
#include "libs/pilight/core/options.h"
#include "libs/pilight/core/threads.h"
#include "libs/pilight/core/datetime.h"
#include "libs/pilight/core/ssdp.h"
#include "libs/pilight/core/socket.h"
#include "libs/pilight/core/gc.h"
#include "libs/pilight/core/dso.h"

#include "libs/pilight/events/events.h"

#include "libs/pilight/config/hardware.h"

static unsigned short main_loop = 1;
static unsigned short inner_loop = 1;

#ifndef PILIGHT_DEVELOPMENT
static uv_signal_t *signal_req = NULL;
static int doSkip = 0;
#endif

static int normalize(int i, int pulselen) {
	double x;
	x=(double)i/pulselen;

	return (int)(round(x));
}

int main_gc(void) {
	log_shell_disable();
	main_loop = 0;
	inner_loop = 0;

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

	return EXIT_SUCCESS;
}

void *receivePulseTrain(void *param) {
	int i = 0;

	int pulselen = 0;
	int pulse = 0;

	struct rawcode_t r;
	struct tm tm;
	time_t now = 0;

	struct hardware_t *hw = (hardware_t *)param;

	while(main_loop) {
		memset(&r.pulses, 0, sizeof(r.pulses));
		memset(&tm, '\0', sizeof(struct tm));
		pulse = 0;
		inner_loop = 1;

		i = 0;
		time(&now);

		hw->receivePulseTrain(&r);
		if(r.length == -1) {
			main_gc();
			break;
		} else if(r.length > 0) {
			pulselen = r.pulses[r.length-1]/PULSE_DIV;

			if(pulselen > 25) {
				for(i=3;i<r.length;i++) {
					if((r.pulses[i]/pulselen) >= 2) {
						pulse=r.pulses[i];
						break;
					}
				}

				if(normalize(pulse, pulselen) > 0 && r.length > 25) {
					/* Print everything */
					printf("--[RESULTS]--\n");
					printf("\n");
#ifdef _WIN32
					localtime(&now);
#else
					localtime_r(&now, &tm);
#endif

#ifdef _WIN32
					printf("time:\t\t%s\n", asctime(&tm));
#else
					char buf[128];
					char *p = buf;
					memset(&buf, '\0', sizeof(buf));
					asctime_r(&tm, p);
					printf("time:\t\t%s", buf);
#endif
					printf("hardware:\t%s\n", hw->id);
					printf("pulse:\t\t%d\n", normalize(pulse, pulselen));
					printf("rawlen:\t\t%d\n", r.length);
					printf("pulselen:\t%d\n", pulselen);
					printf("\n");
					printf("Raw code:\n");
					for(i=0;i<r.length;i++) {
						printf("%d ", r.pulses[i]);
					}
					printf("\n");
				}
			}
		}
	}
	return (void *)NULL;
}

void *receivePulseTrain1(int reason, void *param) {
	doSkip ^= 1;
	if(doSkip == 1) {
		return NULL;
	}

	struct reason_received_pulsetrain_t *data = param;

	int pulselen = 0;
	int pulse = 0;

	struct tm tm;
	time_t now = 0;

#ifdef PILIGHT_DEVELOPMENT
	struct hardware_t *hw = NULL;
#endif
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
#else
		if(hardware_select_struct(ORIGIN_MASTER, data->hardware, &hw) == 0) {
#endif
			memset(&tm, '\0', sizeof(struct tm));
			pulse = 0;
			inner_loop = 1;

			i = 0;
			time(&now);

			if(data->length > 0) {
				pulselen = data->pulses[data->length-1]/PULSE_DIV;
				if(pulselen > 25) {
					for(i=3;i<data->length;i++) {
						if((data->pulses[i]/pulselen) >= 2) {
							pulse=data->pulses[i];
							break;
						}
					}

					if(normalize(pulse, pulselen) > 0 && data->length > 25) {
						/* Print everything */
						printf("--[RESULTS]--\n");
						printf("\n");
#ifdef _WIN32
						memcpy(&tm, localtime(&now), sizeof(struct tm));
#else
						localtime_r(&now, &tm);
#endif

#ifdef _WIN32
						printf("time:\t\t%s", asctime(&tm));
#else
						char buf[128];
						char *p = buf;
						memset(&buf, '\0', sizeof(buf));
	#ifdef __sun
						asctime_r(&tm, p, sizeof(buf));
	#else
						asctime_r(&tm, p);
	#endif
						printf("time:\t\t%s", buf);
#endif
#ifdef PILIGHT_DEVELOPMENT
						printf("hardware:\t%s\n", hw->id);
#else
						printf("hardware:\t%s\n", id);
#endif
						printf("pulse:\t\t%d\n", normalize(pulse, pulselen));
						printf("rawlen:\t\t%d\n", data->length);
						printf("pulselen:\t%d\n", pulselen);
						printf("\n");
						printf("Raw code:\n");
						for(i=0;i<data->length;i++) {
							printf("%d ", data->pulses[i]);
						}
						printf("\n");
					}
				}
			}
#ifdef PILIGHT_DEVELOPMENT
		}
#endif
	}
	return (void *)NULL;
}

void *receiveOOK(void *param) {
	int duration = 0;
	int i = 0;
	unsigned int y = 0;

	int recording = 1;
	int pulselen = 0;
	int bit = 0;
	int raw[MAXPULSESTREAMLENGTH] = {0};
	int pRaw[MAXPULSESTREAMLENGTH] = {0};
	int footer = 0;
	int pulse = 0;
	int rawLength = 0;
	int plsdec = 1;

	struct tm tm;
	time_t now = 0, later = 0;

	struct hardware_t *hw = (hardware_t *)param;

	while(main_loop) {
		memset(&raw, '\0', sizeof(raw));
		memset(&pRaw, '\0', sizeof(pRaw));
		memset(&tm, '\0', sizeof(struct tm));
		recording = 1;
		bit = 0;
		footer = 0;
		pulse = 0;
		rawLength = 0;
		inner_loop = 1;
		pulselen = 0;

		duration = 0;
		i = 0;
		y = 0;
		time(&now);

		while(inner_loop == 1 && hw->receiveOOK != NULL) {
			duration = hw->receiveOOK();
			time(&later);
			if(difftime(later, now) > 1) {
				inner_loop = 0;
			}
			/* If we are recording, keep recording until the next footer has been matched */
			if(recording == 1) {
				if(bit < MAXPULSESTREAMLENGTH) {
					raw[bit++] = duration;
				} else {
					bit = 0;
					recording = 0;
				}
			}

			/* First try to catch code that seems to be a footer.
				 If a real footer has been recognized, start using that as the new footer */
			if((duration > 5100 && footer == 0) || ((footer-(footer*0.3)<duration) && (footer+(footer*0.3)>duration))) {
				recording = 1;
				pulselen = (int)duration/PULSE_DIV;

				/* Check if we are recording similar codes */
				for(i=0;i<(bit-1);i++) {
					if(!(((pRaw[i]-(pRaw[i]*0.3)) < raw[i]) && ((pRaw[i]+(pRaw[i]*0.3)) > raw[i]))) {
						y=0;
						recording=0;
					}
					pRaw[i]=raw[i];
				}
				y++;

				/* Continue if we have 2 matches */
				if(y>=1) {
					/* If we are certain we are recording similar codes. Save the raw code length. */
					if(footer>0) {
						if(rawLength == 0)
							rawLength=bit;
					}

					if(pulselen > 1000) {
						plsdec = 10;
					}
					/* Try to catch the footer, and the low and high values */
					for(i=0;i<bit;i++) {
						if((i+1)<bit && i > 2 && footer > 0) {
							if((raw[i]/(pulselen/plsdec)) >= 2) {
								pulse=raw[i];
							}
						}
						if(duration > 5100) {
							footer=raw[i];
						}
					}

					/* If we have gathered all data, stop with the loop */
					if(footer > 0 && pulse > 0 && rawLength > 0) {
						inner_loop = 0;
					}
				}
				bit=0;
			}

			fflush(stdout);
		}

		if(normalize(pulse, (pulselen/plsdec)) > 0 && rawLength > 25) {
			/* Print everything */
			printf("--[RESULTS]--\n");
			printf("\n");
#ifdef _WIN32
			localtime(&now);
#else
			localtime_r(&now, &tm);
#endif

#ifdef _WIN32
			printf("time:\t\t%s\n", asctime(&tm));
#else
			char buf[128];
			char *p = buf;
			memset(&buf, '\0', sizeof(buf));
			asctime_r(&tm, p);
			printf("time:\t\t%s", buf);
#endif
			printf("hardware:\t%s\n", hw->id);
			printf("pulse:\t\t%d\n", normalize(pulse, (pulselen/plsdec)));
			printf("rawlen:\t\t%d\n", rawLength);
			printf("pulselen:\t%d\n", pulselen);
			printf("\n");
			printf("Raw code:\n");
			for(i=0;i<rawLength;i++) {
				printf("%d ",normalize(raw[i], (pulselen/plsdec))*(pulselen/plsdec));
			}
			printf("\n");
		}
	}

	main_loop = 0;

	return NULL;
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

	memtrack();

	atomicinit();

	gc_attach(main_gc);

	/* Catch all exit signals for gc */
	gc_catch();

	if((progname = MALLOC(15)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(progname, "pilight-debug");

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

	struct options_t *options = NULL;

	char *args = NULL;
	pid_t pid = 0;

	char configtmp[] = CONFIG_FILE;
	config_set_file(configtmp);

	options_add(&options, 'H', "help", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'V', "version", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'C', "config", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);

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
				printf("\t -C --config\t\tconfig file\n");
				goto clear;
			break;
			case 'V':
				printf("%s v%s\n", progname, PILIGHT_VERSION);
				goto clear;
			break;
			case 'C':
				if(config_set_file(args) == EXIT_FAILURE) {
					goto clear;
				}
			break;
			default:
				printf("Usage: %s [options]\n", progname);
				goto clear;
			break;
		}
	}
	options_delete(options);

#ifdef _WIN32
	if((pid = check_instances(L"pilight-debug")) != -1) {
		logprintf(LOG_NOTICE, "pilight-debug is already running");
		goto clear;
	}
#endif

	if((pid = isrunning("pilight-daemon")) != -1) {
		logprintf(LOG_NOTICE, "pilight-daemon instance found (%d)", (int)pid);
		goto clear;
	}

	if((pid = isrunning("pilight-raw")) != -1) {
		logprintf(LOG_NOTICE, "pilight-raw instance found (%d)", (int)pid);
		goto clear;
	}

#ifndef PILIGHT_DEVELOPMENT
	eventpool_init(EVENTPOOL_THREADED);
#endif
	protocol_init();
	config_init();

	if(config_read() != EXIT_SUCCESS) {
		goto clear;
	}

#ifndef PILIGHT_DEVELOPMENT
	eventpool_callback(REASON_RECEIVED_PULSETRAIN, receivePulseTrain1);
#endif

	/* Start threads library that keeps track of all threads used */
	threads_start();

	struct conf_hardware_t *tmp_confhw = conf_hardware;
	while(tmp_confhw) {
		if(tmp_confhw->hardware->init) {
			if(tmp_confhw->hardware->comtype == COMOOK) {
				tmp_confhw->hardware->maxrawlen = MAXPULSESTREAMLENGTH;
				tmp_confhw->hardware->minrawlen = 25;
				tmp_confhw->hardware->maxgaplen = 34000;
				tmp_confhw->hardware->mingaplen = 5100;
			}
			if(tmp_confhw->hardware->init() == EXIT_FAILURE) {
				logprintf(LOG_ERR, "could not initialize %s hardware mode", tmp_confhw->hardware->id);
				goto clear;
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

	printf("Press and hold one of the buttons on your remote or wait until\n");
	printf("another device such as a weather station has sent new codes\n");
	printf("The debugger will automatically reset itself after one second of\n");
	printf("failed leads. It will keep running until you explicitly stop it.\n");
	printf("This is done by pressing both the [CTRL] and C buttons on your keyboard.\n");

#ifdef PILIGHT_DEVELOPMENT
	while(main_loop) {
		sleep(1);
	}
#else
	main_loop1(0);
#endif

clear:
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
