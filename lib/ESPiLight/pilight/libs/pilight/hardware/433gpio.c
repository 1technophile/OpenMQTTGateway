/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <wiringx.h>

#include "../../libuv/uv.h"
#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/dso.h"
#include "../core/log.h"
#include "../core/json.h"
#include "../core/eventpool.h"
#ifdef PILIGHT_REWRITE
#include "hardware.h"
#else
#include "../config/hardware.h"
#include "../config/settings.h"
#endif
#include "433gpio.h"

static int gpio_433_in = -1;
static int gpio_433_out = -1;
static int pollpri = UV_PRIORITIZED;

#if defined(__arm__) || defined(__mips__) || defined(PILIGHT_UNITTEST)
typedef struct timestamp_t {
	unsigned long first;
	unsigned long second;
} timestamp_t;

typedef struct data_t {
	int rbuffer[1024];
	int rptr;
} data_t;

static struct data_t data;
static struct timestamp_t timestamp;

static void *reason_received_pulsetrain_free(void *param) {
	struct reason_received_pulsetrain_t *data = param;
	FREE(data);
	return NULL;
}

static void *reason_send_code_success_free(void *param) {
	struct reason_send_code_success_free *data = param;
	FREE(data);
	return NULL;
}

static void poll_cb(uv_poll_t *req, int status, int events) {
	int duration = 0;
	int fd = req->io_watcher.fd;

	if(events & pollpri) {
		uint8_t c = 0;

		(void)read(fd, &c, 1);
		lseek(fd, 0, SEEK_SET);

		struct timeval tv;
		gettimeofday(&tv, NULL);
		timestamp.first = timestamp.second;
		timestamp.second = 1000000 * (unsigned int)tv.tv_sec + (unsigned int)tv.tv_usec;

		duration = (int)((int)timestamp.second-(int)timestamp.first);

		if(duration > 0) {
			data.rbuffer[data.rptr++] = duration;
			if(data.rptr > MAXPULSESTREAMLENGTH-1) {
				data.rptr = 0;
			}
			if(duration > gpio433->mingaplen) {
				/* Let's do a little filtering here as well */
				if(data.rptr >= gpio433->minrawlen && data.rptr <= gpio433->maxrawlen) {
					struct reason_received_pulsetrain_t *data1 = MALLOC(sizeof(struct reason_received_pulsetrain_t));
					if(data1 == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					data1->length = data.rptr;
					memcpy(data1->pulses, data.rbuffer, data.rptr*sizeof(int));
					data1->hardware = gpio433->id;

					eventpool_trigger(REASON_RECEIVED_PULSETRAIN, reason_received_pulsetrain_free, data1);
				}
				data.rptr = 0;
			}
		}
	};
	if(events & UV_DISCONNECT) {
		FREE(req); /*LCOV_EXCL_LINE*/
	}
	return;
}

static void *gpio433Send(int reason, void *param) {
	struct reason_send_code_t *data1 = param;
	int *code = data1->pulses;
	int rawlen = data1->rawlen;
	int repeats = data1->txrpt;

	int r = 0, x = 0;
	if(gpio_433_out >= 0) {
		for(r=0;r<repeats;r++) {
			for(x=0;x<rawlen;x+=2) {
				digitalWrite(gpio_433_out, 1);
				usleep((__useconds_t)code[x]);
				digitalWrite(gpio_433_out, 0);
				if(x+1 < rawlen) {
					usleep((__useconds_t)code[x+1]);
				}
			}
		}
		digitalWrite(gpio_433_out, 0);
	}

	struct reason_code_sent_success_t *data2 = MALLOC(sizeof(struct reason_code_sent_success_t));
	strcpy(data2->message, data1->message);
	strcpy(data2->uuid, data1->uuid);
	eventpool_trigger(REASON_CODE_SEND_SUCCESS, reason_send_code_success_free, data2);
	return NULL;
}
#endif

static unsigned short int gpio433HwInit(void) {
#if defined(__arm__) || defined(__mips__) || defined(PILIGHT_UNITTEST)

	/* Make sure the pilight sender gets
	   the highest priority available */
#ifdef _WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#else
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = 80;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched);
#endif

	uv_poll_t *poll_req = NULL;
	char *platform = GPIO_PLATFORM;

#ifdef PILIGHT_REWRITE
	if(settings_select_string(ORIGIN_MASTER, "gpio-platform", &platform) != 0 || strcmp(platform, "none") == 0) {
		logprintf(LOG_ERR, "no gpio-platform configured");
		return EXIT_FAILURE;
	}
	if(wiringXSetup(platform, _logprintf) < 0) {
		return EXIT_FAILURE;
	}
#else
	if(settings_find_string("gpio-platform", &platform) != 0 || strcmp(platform, "none") == 0) {
		logprintf(LOG_ERR, "no gpio-platform configured");
		return EXIT_FAILURE;
	}
	if(wiringXSetup(platform, logprintf1) < 0) {
		return EXIT_FAILURE;
	}
#endif
	if(gpio_433_out >= 0) {
		if(wiringXValidGPIO(gpio_433_out) != 0) {
			logprintf(LOG_ERR, "invalid sender pin: %d", gpio_433_out);
			return EXIT_FAILURE;
		}
		pinMode(gpio_433_out, PINMODE_OUTPUT);
	}
	if(gpio_433_in >= 0) {
		if(wiringXValidGPIO(gpio_433_in) != 0) {
			logprintf(LOG_ERR, "invalid receiver pin: %d", gpio_433_in);
			return EXIT_FAILURE;
		}
		if(wiringXISR(gpio_433_in, ISR_MODE_BOTH) < 0) {
			logprintf(LOG_ERR, "unable to register interrupt for pin %d", gpio_433_in);
			return EXIT_FAILURE;
		}
	}
	if(gpio_433_in >= 0) {
		if((poll_req = MALLOC(sizeof(uv_poll_t))) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		int fd = wiringXSelectableFd(gpio_433_in);
		memset(data.rbuffer, '\0', sizeof(data.rbuffer));
		data.rptr = 0;

		uv_poll_init(uv_default_loop(), poll_req, fd);

#ifdef PILIGHT_UNITTEST
		char *dev = getenv("PILIGHT_433GPIO_READ");
		if(dev == NULL) {
			pollpri = UV_PRIORITIZED; /*LCOV_EXCL_LINE*/
		} else {
			pollpri = UV_READABLE;
		}
#endif
		uv_poll_start(poll_req, pollpri, poll_cb);
	}

	eventpool_callback(REASON_SEND_CODE, gpio433Send);

	return EXIT_SUCCESS;
#else
	logprintf(LOG_ERR, "the 433gpio module is not supported on this hardware", gpio_433_in);
	return EXIT_FAILURE;
#endif
}

static unsigned short gpio433Settings(struct JsonNode *json) {
	if(strcmp(json->key, "receiver") == 0) {
		if(json->tag == JSON_NUMBER) {
			gpio_433_in = (int)json->number_;
		} else {
			return EXIT_FAILURE;
		}
	}
	if(strcmp(json->key, "sender") == 0) {
		if(json->tag == JSON_NUMBER) {
			gpio_433_out = (int)json->number_;
		} else {
			return EXIT_FAILURE;
		}
	}
	if(gpio_433_out > -1 && gpio_433_in > -1 && gpio_433_in == gpio_433_out) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void gpio433Init(void) {
	hardware_register(&gpio433);
	hardware_set_id(gpio433, "433gpio");

	options_add(&gpio433->options, 'r', "receiver", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9-]+$");
	options_add(&gpio433->options, 's', "sender", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9-]+$");

	gpio433->minrawlen = 1000;
	gpio433->maxrawlen = 0;
	gpio433->mingaplen = 5100;
	gpio433->maxgaplen = 10000;

	gpio433->hwtype=RF433;
	gpio433->comtype=COMOOK;
	gpio433->init=&gpio433HwInit;
	gpio433->settings=&gpio433Settings;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "433gpio";
	module->version = "2.0";
	module->reqversion = "8.0";
	module->reqcommit = NULL;
}

void init(void) {
	gpio433Init();
}
#endif
