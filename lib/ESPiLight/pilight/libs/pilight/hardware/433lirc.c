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
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/dso.h"
#include "../core/log.h"
#include "../core/json.h"
#include "../core/gc.h"
#include "../config/hardware.h"
#include "433lirc.h"

#define FREQ433				433920
#define FREQ38				38000

static int lirc_433_initialized = 0;
static int lirc_433_setfreq = 0;
static int lirc_433_fd = 0;
static char *lirc_433_socket = NULL;

static unsigned short lirc433HwInit(void) {
	unsigned int freq = 0;
	int fd = 0, i = 0, count = 0;
	int c = 0;

	if(strcmp(lirc_433_socket, "/var/lirc/lircd") == 0) {
		logprintf(LOG_ERR, "refusing to connect to lircd socket");
		return EXIT_FAILURE;
	}

	if(lirc_433_initialized == 0) {
		if((lirc_433_fd = open(lirc_433_socket, O_RDWR)) < 0) {
			logprintf(LOG_ERR, "could not open %s", lirc_433_socket);
			return EXIT_FAILURE;
		} else {
			/* Only set the frequency once */
			if(lirc_433_setfreq == 0) {
				freq = FREQ433;
				/* Set the lirc_rpi frequency to 433.92Mhz */
				if(ioctl(lirc_433_fd, _IOW('i', 0x00000013, unsigned long), &freq) == -1) {
					logprintf(LOG_ERR, "could not set lirc_rpi send frequency");
					exit(EXIT_FAILURE);
				}
				fd = open(lirc_433_socket, O_RDWR);
				ioctl(fd, FIONREAD, &count);
				for(i=0; i<count; ++i) {
					read(lirc_433_fd, &c, sizeof(c));
				}
				close(fd);
				lirc_433_setfreq = 1;
			}
			logprintf(LOG_DEBUG, "initialized lirc_rpi lirc");
			lirc_433_initialized = 1;
		}
	}
	return EXIT_SUCCESS;
}

static unsigned short lirc433HwDeinit(void) {
	unsigned int freq = 0;

	if(lirc_433_initialized == 1) {

		freq = FREQ38;

		if(lirc_433_fd > 0) {
			/* Restore the lirc_rpi frequency to its default value */
			if(ioctl(lirc_433_fd, _IOW('i', 0x00000013, unsigned long), &freq) == -1) {
				logprintf(LOG_ERR, "could not restore default freq of the lirc_rpi lirc");
				exit(EXIT_FAILURE);
			} else {
				logprintf(LOG_DEBUG, "default freq of the lirc_rpi lirc set");
			}

			close(lirc_433_fd);
			lirc_433_fd = 0;
		}

		logprintf(LOG_DEBUG, "deinitialized lirc_rpi lirc");
		lirc_433_initialized = 0;
	}

	return EXIT_SUCCESS;
}

static int lirc433Send(int *code, int rawlen, int repeats) {
	/* Create a single code with all repeats included */
	int code_len = (rawlen*repeats)+1;
	size_t send_len = (size_t)(code_len * (int)sizeof(int));
	int longCode[code_len], i = 0, x = 0;
	size_t y = 0;
	ssize_t n = 0;
	memset(longCode, 0, send_len);

	for(i=0;i<repeats;i++) {
		for(x=0;x<rawlen;x++) {
			longCode[x+(rawlen*i)]=code[x];
		}
	}
	longCode[code_len] = 0;

	while(longCode[y]) {
		y++;
	}
	y++;
	y *= sizeof(int);
	n = write(lirc_433_fd, longCode, y);

	if(n == y) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

static int lirc433Receive(void) {
	struct pollfd polls;
	int data = 0;
	int x = 0;
	int ms = 10;
	polls.fd = lirc_433_fd;
	polls.events = POLLIN;

	x = poll(&polls, 1, ms);

	if(x > 0) {
		(void)read(lirc_433_fd, &data, sizeof(data));
		lseek(lirc_433_fd, 0, SEEK_SET);
		return (data & 0x00FFFFFF);
	} else {
		return -1;
	}
}

static unsigned short lirc433Settings(JsonNode *json) {
	if(strcmp(json->key, "socket") == 0) {
		if(json->tag == JSON_STRING) {
			if((lirc_433_socket = MALLOC(strlen(json->string_)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(lirc_433_socket, json->string_);
		} else {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}


static int lirc433gc(void) {
	if(lirc_433_socket != NULL) {
		FREE(lirc_433_socket);
	}

	return 1;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void lirc433Init(void) {

	hardware_register(&lirc433);
	hardware_set_id(lirc433, "433lirc");

	options_add(&lirc433->options, 's', "socket", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, "^/dev/([a-z]+)[0-9]+$");

	lirc433->minrawlen = 1000;
	lirc433->maxrawlen = 0;
	lirc433->mingaplen = 5100;
	lirc433->maxgaplen = 10000;	
	
	lirc433->hwtype=RF433;
	lirc433->comtype=COMOOK;
	lirc433->init=&lirc433HwInit;
	lirc433->deinit=&lirc433HwDeinit;
	lirc433->sendOOK=&lirc433Send;
	lirc433->receiveOOK=&lirc433Receive;
	lirc433->settings=&lirc433Settings;
	lirc433->gc=&lirc433gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "433lirc";
	module->version = "1.4";
	module->reqversion = "7.0";
	module->reqcommit = "10";
}

void init(void) {
	lirc433Init();
}
#endif
