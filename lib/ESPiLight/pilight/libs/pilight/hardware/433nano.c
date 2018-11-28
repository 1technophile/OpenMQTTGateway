/*
	Copyright (C) 2013 - 2015 CurlyMo

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

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#ifdef _WIN32
	#define STRICT
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <termios.h>
#endif

#include "../core/pilight.h"
#include "../core/json.h"
#include "../core/log.h"
#include "../core/dso.h"
#include "../core/firmware.h"
#include "../config/registry.h"
#include "../config/hardware.h"
#include "433nano.h"

typedef struct timestamp_t {
	unsigned long first;
	unsigned long second;
} timestamp_t;

timestamp_t timestamp;

/* What is the minimum rawlenth to consider a pulse stream valid */
static int minrawlen = 1000;
/* What is the maximum rawlenth to consider a pulse stream valid */
static int maxrawlen = 0;
/* What is the minimum rawlenth to consider a pulse stream valid */
static int maxgaplen = 5100;
/* What is the maximum rawlenth to consider a pulse stream valid */
static int mingaplen = 10000;

#ifdef _WIN32
static HANDLE serial_433_fd;
#else
static int serial_433_fd = 0;
static int nano_433_initialized = 0;
#endif

static char com[255];
static unsigned short loop = 1;
static unsigned short running = 0;
static unsigned short threads = 0;
static unsigned short sendSync = 0;
static pthread_t pth;

void *syncFW(void *param) {

	threads++;

#ifdef _WIN32
	DWORD n;
#else
	int n;
#endif
	char send[MAXPULSESTREAMLENGTH+1];
	int len = 0;

	memset(&send, '\0', sizeof(send));

	struct protocols_t *tmp = protocols;
	while(tmp) {
		if(tmp->listener->hwtype == RF433) {
			if(tmp->listener->maxrawlen > maxrawlen) {
				maxrawlen = tmp->listener->maxrawlen;
			}
			if(tmp->listener->minrawlen > 0 && tmp->listener->minrawlen < minrawlen) {
				minrawlen = tmp->listener->minrawlen;
			}
			if(tmp->listener->maxgaplen > maxgaplen) {
				maxgaplen = tmp->listener->maxgaplen;
			}
			if(tmp->listener->mingaplen > 0 && tmp->listener->mingaplen < mingaplen) {
				mingaplen = tmp->listener->mingaplen;
			}
		}
		tmp = tmp->next;
	}

	/* Let's wait for Nano to accept instructions */
	while(sendSync == 0 && loop == 1) {
		usleep(10);
	}

	len = sprintf(send, "s:%d,%d,%d,%d@", minrawlen, maxrawlen, mingaplen, maxgaplen);

#ifdef _WIN32
	WriteFile(serial_433_fd, &send, len, &n, NULL);
#else
	n = write(serial_433_fd, send, len);
#endif

	if(n != len) {
		logprintf(LOG_NOTICE, "could not sync FW values");
	}

	threads--;

	return NULL;
}

#ifndef _WIN32
/* http://stackoverflow.com/a/6947758 */
static int serial_interface_attribs(int fd, speed_t speed, tcflag_t parity) {
	struct termios tty;

	memset(&tty, 0, sizeof tty);
	if(tcgetattr(fd, &tty) != 0) {
		logprintf(LOG_ERR, "error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & (unsigned int)~CSIZE) | CS8;     // 8-bit chars
	tty.c_iflag &= (unsigned int)~IGNBRK;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 5;
	tty.c_iflag &= ~((unsigned int)IXON | (unsigned int)IXOFF | (unsigned int)IXANY);
	tty.c_cflag |= ((unsigned int)CLOCAL | (unsigned int)CREAD);
	tty.c_cflag &= ~((unsigned int)PARENB | (unsigned int)PARODD);
	tty.c_cflag |= parity;
	tty.c_cflag &= (unsigned int)~CSTOPB;
	tty.c_cflag &= (unsigned int)~CRTSCTS;

	if(tcsetattr(fd, TCSANOW, &tty) != 0) {
		logprintf(LOG_ERR, "error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}
#endif

static unsigned short int nano433HwInit(void) {
#ifdef _WIN32
	COMMTIMEOUTS timeouts;
	DCB port;
	char tmp[255];
	memset(tmp, '\0', 255);

	snprintf(tmp, 255, "\\\\.\\%s", com);

	if((int)(serial_433_fd = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) < 0) {
		logprintf(LOG_NOTICE, "cannot open port %s", com);
		return EXIT_FAILURE;
	}
	logprintf(LOG_INFO, "connected to port %s", com);

	memset(&port, '\0', sizeof(port));

	port.DCBlength = sizeof(port);
	if(GetCommState(serial_433_fd, &port) == FALSE) {
		logprintf(LOG_ERR, "cannot get comm state for port %s", com);
		return EXIT_FAILURE;
	}

	if(BuildCommDCB("baud=57600 parity=n data=8 stop=1", &port) == FALSE) {
		logprintf(LOG_ERR, "cannot build comm DCB for port %s", com);
		return EXIT_FAILURE;
	}

	if(SetCommState(serial_433_fd, &port) == FALSE) {
		logprintf(LOG_ERR, "cannot set port settings for port %s", com);
		return EXIT_FAILURE;
	}

	timeouts.ReadIntervalTimeout = 1000;
	timeouts.ReadTotalTimeoutMultiplier = 1000;
	timeouts.ReadTotalTimeoutConstant = 1000;
	timeouts.WriteTotalTimeoutMultiplier = 1000;
	timeouts.WriteTotalTimeoutConstant = 1000;

	if(SetCommTimeouts(serial_433_fd, &timeouts) == FALSE) {
		logprintf(LOG_ERR, "error setting port %s time-outs.", com);
		return EXIT_FAILURE;
	}
#else
	if((serial_433_fd = open(com, O_RDWR | O_SYNC)) >= 0) {
		serial_interface_attribs(serial_433_fd, B57600, 0);
		nano_433_initialized = 1;
	} else {
		logprintf(LOG_NOTICE, "could not open port %s", com);
		return EXIT_FAILURE;
	}
#endif

	pthread_create(&pth, NULL, &syncFW, (void *)NULL);
	pthread_detach(pth);

	return EXIT_SUCCESS;
}

static unsigned short nano433HwDeinit(void) {
	loop = 0;
	while(running > 0 && threads > 0) {
		usleep(10);
	}
#ifdef _WIN32
	CloseHandle(serial_433_fd);
#else
	if(nano_433_initialized == 1) {
		close(serial_433_fd);
		nano_433_initialized = 0;
	}
#endif
	return EXIT_SUCCESS;
}

static int nano433Send(int *code, int rawlen, int repeats) {
	unsigned int i = 0, x = 0, y = 0, len = 0, nrpulses = 0;
	int pulses[10], match = 0;
	char c[16], send[MAXPULSESTREAMLENGTH+1];
#ifdef _WIN32
	DWORD n;
#else
	int n = 0;
#endif

	memset(send, 0, MAXPULSESTREAMLENGTH);
	strncpy(&send[0], "c:", 2);
	len += 2;

	for(i=0;i<rawlen;i++) {
		match = -1;
		for(x=0;x<nrpulses;x++) {
			if(pulses[x] == code[i]) {
				match = (int)x;
				break;
			}
		}
		if(match == -1) {
			pulses[nrpulses] = code[i];
			match = (int)nrpulses;
			nrpulses++;
		}
		if(match < 10) {
			send[len++] = (char)(((int)'0')+match);
		} else {
			logprintf(LOG_ERR, "too many distinct pulses for pilight usb nano to send");
			return EXIT_FAILURE;
		}
	}

	strncpy(&send[len], ";p:", 3);
	len += 3;
	for(i=0;i<nrpulses;i++) {
		y = (unsigned int)snprintf(c, sizeof(c), "%d", pulses[i]);
		strncpy(&send[len], c, y);
		len += y;
		if(i+1 < nrpulses) {
			strncpy(&send[len++], ",", 1);
		}
	}
	strncpy(&send[len], ";r:", 3);
	len += 3;
	y = (unsigned int)snprintf(c, sizeof(c), "%d", repeats);
	strncpy(&send[len], c, y);
	len += y;
	strncpy(&send[len], "@", 3);
	len += 3;

#ifdef _WIN32
	WriteFile(serial_433_fd, &send, len, &n, NULL);
#else
	n = write(serial_433_fd, send, len);
#endif

	struct timeval tv;
	gettimeofday(&tv, NULL);
	timestamp.first = timestamp.second;
	timestamp.second = 1000000 * (unsigned int)tv.tv_sec + (unsigned int)tv.tv_usec;

	if(((int)timestamp.second-(int)timestamp.first) < 1000000) {
		sleep(1);
	}

	if(n == len) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

static int nano433Receive(struct rawcode_t *r) {
	char buffer[1024], c[1];
	int start = 0, bytes = 0, startv = 0;
	int s = 0, nrpulses = 0, y = 0;
	int startp = 0, pulses[10];
	size_t x = 0;
#ifdef _WIN32
	DWORD n;
#else
	int n = 0;
#endif

	r->length = 0;
	memset(r->pulses, 0, MAXPULSESTREAMLENGTH);

	running = 1;

	while(loop) {
#ifdef _WIN32
		if(WriteFile(serial_433_fd, "ping", 0, &n, NULL) == 0) {
			logprintf(LOG_INFO, "lost connection to %s", com);
			CloseHandle(serial_433_fd);
			r->length = -1;
			return -1;
		}
		ReadFile(serial_433_fd, c, 1, &n, NULL);
#else
		n = read(serial_433_fd, c, 1);
#endif
		if(n > 0) {
			if(c[0] == '\n') {
				sendSync = 1;
				break;
			} else {
				if(c[0] == 'v') {
					startv = 1;
					start = 1;
					bytes = 0;
				}
				if(c[0] == 'c') {
					start = 1;
					bytes = 0;
				}
				if(c[0] == 'p') {
					startp = bytes+2;
					buffer[bytes-1] = '\0';
				}
				if(c[0] == '@') {
					buffer[bytes] = '\0';
					if(startv == 1) {
						start = 0;
						startv = 0;
						char **array = NULL;
						int c = explode(&buffer[2], ",", &array);
						if(c == 7) {
							if(!(minrawlen == atoi(array[0]) && maxrawlen == atoi(array[1]) &&
							     mingaplen == atoi(array[2]) && maxgaplen == atoi(array[3]))) {
								logprintf(LOG_WARNING, "could not sync FW values");
							}
							firmware.version = atof(array[4]);
							firmware.lpf = atof(array[5]);
							firmware.hpf = atof(array[6]);

							if(firmware.version > 0 && firmware.lpf > 0 && firmware.hpf > 0) {
								registry_set_number("pilight.firmware.version", firmware.version, 0);
								registry_set_number("pilight.firmware.lpf", firmware.lpf, 0);
								registry_set_number("pilight.firmware.hpf", firmware.hpf, 0);

								struct JsonNode *jmessage = json_mkobject();
								struct JsonNode *jcode = json_mkobject();
								json_append_member(jcode, "version", json_mknumber(firmware.version, 0));
								json_append_member(jcode, "lpf", json_mknumber(firmware.lpf, 0));
								json_append_member(jcode, "hpf", json_mknumber(firmware.hpf, 0));
								json_append_member(jmessage, "values", jcode);
								json_append_member(jmessage, "origin", json_mkstring("core"));
								json_append_member(jmessage, "type", json_mknumber(FIRMWARE, 0));
								char pname[17];
								strcpy(pname, "pilight-firmware");
								if(pilight.broadcast != NULL) {
									pilight.broadcast(pname, jmessage, FW);
								}
								json_delete(jmessage);
								jmessage = NULL;
							}
						}
						array_free(&array, c);
					} else {
						x = strlen(&buffer[startp]);
						s = startp;
						nrpulses = 0;
						for(y = startp; y < startp + (int)x; y++) {
							if(buffer[y] == ',') {
								buffer[y] = '\0';
								pulses[nrpulses++] = atoi(&buffer[s]);
								s = y+1;
							}
						}
						pulses[nrpulses++] = atoi(&buffer[s]);
						x = strlen(&buffer[2]);
						for(y = 2; y < 2 + x; y++) {
							r->pulses[r->length++] = pulses[0];
							r->pulses[r->length++] = pulses[buffer[y] - '0'];
						}
						bytes = 0;
						return 0;
					}
				}
				if(start == 1) {
					buffer[bytes++] = c[0];
				}
			}
		}
	}

	running = 0;

  return -1;
}

static unsigned short nano433Settings(JsonNode *json) {
	if(strcmp(json->key, "comport") == 0) {
		if(json->tag == JSON_STRING) {
			strcpy(com, json->string_);
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void nano433Init(void) {
	hardware_register(&nano433);
	hardware_set_id(nano433, "433nano");

	options_add(&nano433->options, 'p', "comport", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	nano433->hwtype=RF433;
	nano433->comtype=COMPLSTRAIN;
	nano433->init=&nano433HwInit;
	nano433->deinit=&nano433HwDeinit;
	nano433->sendOOK=&nano433Send;
	nano433->receivePulseTrain=&nano433Receive;
	nano433->settings=&nano433Settings;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "433nano";
	module->version = "1.2";
	module->reqversion = "7.0";
	module->reqcommit = "10";
}

void init(void) {
	nano433Init();
}
#endif
