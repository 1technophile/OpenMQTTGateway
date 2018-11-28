/*
	Copyright (C) 2014 CurlyMo & hstroh

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
#include <sys/time.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <wiringx.h>
#endif
#include <pthread.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../../core/threads.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "../../core/json.h"
#include "../../config/settings.h"
#include "../protocol.h"
#include "bmp180.h"

#if !defined(__FreeBSD__) && !defined(_WIN32)

typedef struct settings_t {
	char **id;
	int nrid;
	char path[PATH_MAX];
	int *fd;
	// calibration values (stored in each BMP180/085)
	short *ac1;
	short *ac2;
	short *ac3;
	unsigned short *ac4;
	unsigned short *ac5;
	unsigned short *ac6;
	short *b1;
	short *b2;
	short *mb;
	short *mc;
	short *md;
} settings_t;

static unsigned short loop = 1;
static int threads = 0;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

// helper function with built-in result conversion
static int readReg16(int fd, int reg) {
	int res = wiringXI2CReadReg16(fd, reg);
	// convert result to 16 bits and swap bytes
	return ((res << 8) & 0xFF00) | ((res >> 8) & 0xFF);
}

static void *thread(void *param) {
	struct protocol_threads_t *node = (struct protocol_threads_t *) param;
	struct JsonNode *json = (struct JsonNode *) node->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct settings_t *bmp180data = MALLOC(sizeof(struct settings_t));
	int y = 0, interval = 10, nrloops = 0;
	char *stmp = NULL;
	double itmp = -1, temp_offset = 0, pressure_offset = 0;
	unsigned char oversampling = 1;

	if(bmp180data == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	bmp180data->nrid = 0;
	bmp180data->id = NULL;
	bmp180data->fd = 0;
	bmp180data->ac1 = 0;
	bmp180data->ac2 = 0;
	bmp180data->ac3 = 0;
	bmp180data->ac4 = 0;
	bmp180data->ac5 = 0;
	bmp180data->ac6 = 0;
	bmp180data->b1 = 0;
	bmp180data->b2 = 0;
	bmp180data->mb = 0;
	bmp180data->mc = 0;
	bmp180data->md = 0;

	threads++;

	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			if(json_find_string(jchild, "id", &stmp) == 0) {
				if((bmp180data->id = REALLOC(bmp180data->id, (sizeof(char *) * (size_t)(bmp180data->nrid + 1)))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if((bmp180data->id[bmp180data->nrid] = MALLOC(strlen(stmp) + 1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(bmp180data->id[bmp180data->nrid], stmp);
				bmp180data->nrid++;
			}
			if(json_find_string(jchild, "i2c-path", &stmp) == 0) {
				strcpy(bmp180data->path, stmp);
			}
			jchild = jchild->next;
		}
	}

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		interval = (int) round(itmp);
	json_find_number(json, "temperature-offset", &temp_offset);
	json_find_number(json, "pressure-offset", &pressure_offset);
	if(json_find_number(json, "oversampling", &itmp) == 0) {
		oversampling = (unsigned char) itmp;
	}

	// resize the memory blocks pointed to by the different pointers
	size_t sz = (size_t) (bmp180data->nrid + 1);
	unsigned long int sizeShort = sizeof(short) * sz;
	unsigned long int sizeUShort = sizeof(unsigned short) * sz;
	bmp180data->fd = REALLOC(bmp180data->fd, (sizeof(int) * sz));
	bmp180data->ac1 = REALLOC(bmp180data->ac1, sizeShort);
	bmp180data->ac2 = REALLOC(bmp180data->ac2, sizeShort);
	bmp180data->ac3 = REALLOC(bmp180data->ac3, sizeShort);
	bmp180data->ac4 = REALLOC(bmp180data->ac4, sizeUShort);
	bmp180data->ac5 = REALLOC(bmp180data->ac5, sizeUShort);
	bmp180data->ac6 = REALLOC(bmp180data->ac6, sizeUShort);
	bmp180data->b1 = REALLOC(bmp180data->b1, sizeShort);
	bmp180data->b2 = REALLOC(bmp180data->b2, sizeShort);
	bmp180data->mb = REALLOC(bmp180data->mb, sizeShort);
	bmp180data->mc = REALLOC(bmp180data->mc, sizeShort);
	bmp180data->md = REALLOC(bmp180data->md, sizeShort);
	if(bmp180data->ac1 == NULL || bmp180data->ac2 == NULL || bmp180data->ac3 == NULL || bmp180data->ac4 == NULL ||
		bmp180data->ac5 == NULL || bmp180data->ac6 == NULL || bmp180data->b1 == NULL || bmp180data->b2 == NULL ||
		bmp180data->mb == NULL || bmp180data->mc == NULL || bmp180data->md == NULL || bmp180data->fd == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	for(y = 0; y < bmp180data->nrid; y++) {
		// setup i2c
		bmp180data->fd[y] = wiringXI2CSetup(bmp180data->path, (int)strtol(bmp180data->id[y], NULL, 16));
		if(bmp180data->fd[y] > 0) {
			// read 0xD0 to check chip id: must equal 0x55 for BMP085/180
			int id = wiringXI2CReadReg8(bmp180data->fd[y], 0xD0);
			if(id != 0x55) {
				logprintf(LOG_ERR, "wrong device detected");
				exit(EXIT_FAILURE);
			}

			// read 0xD1 to check chip version: must equal 0x01 for BMP085 or 0x02 for BMP180
			int version = wiringXI2CReadReg8(bmp180data->fd[y], 0xD1);
			if(version != 0x01 && version != 0x02) {
				logprintf(LOG_ERR, "wrong device detected");
				exit(EXIT_FAILURE);
			}

			// read calibration coefficients from register addresses
			bmp180data->ac1[y] = (short) readReg16(bmp180data->fd[y], 0xAA);
			bmp180data->ac2[y] = (short) readReg16(bmp180data->fd[y], 0xAC);
			bmp180data->ac3[y] = (short) readReg16(bmp180data->fd[y], 0xAE);
			bmp180data->ac4[y] = (unsigned short) readReg16(bmp180data->fd[y], 0xB0);
			bmp180data->ac5[y] = (unsigned short) readReg16(bmp180data->fd[y], 0xB2);
			bmp180data->ac6[y] = (unsigned short) readReg16(bmp180data->fd[y], 0xB4);
			bmp180data->b1[y] = (short) readReg16(bmp180data->fd[y], 0xB6);
			bmp180data->b2[y] = (short) readReg16(bmp180data->fd[y], 0xB8);
			bmp180data->mb[y] = (short) readReg16(bmp180data->fd[y], 0xBA);
			bmp180data->mc[y] = (short) readReg16(bmp180data->fd[y], 0xBC);
			bmp180data->md[y] = (short) readReg16(bmp180data->fd[y], 0xBE);

			// check communication: no result must equal 0 or 0xFFFF (=65535)
			if (bmp180data->ac1[y] == 0 || bmp180data->ac1[y] == 0xFFFF ||
					bmp180data->ac2[y] == 0 || bmp180data->ac2[y] == 0xFFFF ||
					bmp180data->ac3[y] == 0 || bmp180data->ac3[y] == 0xFFFF ||
					bmp180data->ac4[y] == 0 || bmp180data->ac4[y] == 0xFFFF ||
					bmp180data->ac5[y] == 0 || bmp180data->ac5[y] == 0xFFFF ||
					bmp180data->ac6[y] == 0 || bmp180data->ac6[y] == 0xFFFF ||
					bmp180data->b1[y] == 0 || bmp180data->b1[y] == 0xFFFF ||
					bmp180data->b2[y] == 0 || bmp180data->b2[y] == 0xFFFF ||
					bmp180data->mb[y] == 0 || bmp180data->mb[y] == 0xFFFF ||
					bmp180data->mc[y] == 0 || bmp180data->mc[y] == 0xFFFF ||
					bmp180data->md[y] == 0 || bmp180data->md[y] == 0xFFFF) {
				logprintf(LOG_ERR, "data communication error");
				exit(EXIT_FAILURE);
			}
		}
	}

	while (loop) {
		if (protocol_thread_wait(node, interval, &nrloops) == ETIMEDOUT) {
			pthread_mutex_lock(&lock);
			for (y = 0; y < bmp180data->nrid; y++) {
				if (bmp180data->fd[y] > 0) {
					// uncompensated temperature value
					unsigned short ut = 0;

					// write 0x2E into Register 0xF4 to request a temperature reading.
					wiringXI2CWriteReg8(bmp180data->fd[y], 0xF4, 0x2E);

					// wait at least 4.5ms: we suspend execution for 5000 microseconds.
					usleep(5000);

					// read the two byte result from address 0xF6.
					ut = (unsigned short) readReg16(bmp180data->fd[y], 0xF6);

					// calculate temperature (in units of 0.1 deg C) given uncompensated value
					int x1, x2;
					x1 = (((int) ut - (int) bmp180data->ac6[y])) * (int) bmp180data->ac5[y] >> 15;
					x2 = ((int) bmp180data->mc[y] << 11) / (x1 + bmp180data->md[y]);
					int b5 = x1 + x2;
					int temp = ((b5 + 8) >> 4);

					// uncompensated pressure value
					unsigned int up = 0;

					// write 0x34+(BMP085_OVERSAMPLING_SETTING<<6) into register 0xF4
					// request a pressure reading with specified oversampling setting
					wiringXI2CWriteReg8(bmp180data->fd[y], 0xF4,
							0x34 + (oversampling << 6));

					// wait for conversion, delay time dependent on oversampling setting
					unsigned int delay = (unsigned int) ((2 + (3 << oversampling)) * 1000);
					usleep(delay);

					// read the three byte result (block data): 0xF6 = MSB, 0xF7 = LSB and 0xF8 = XLSB
					int msb = wiringXI2CReadReg8(bmp180data->fd[y], 0xF6);
					int lsb = wiringXI2CReadReg8(bmp180data->fd[y], 0xF7);
					int xlsb = wiringXI2CReadReg8(bmp180data->fd[y], 0xF8);
					up = (((unsigned int) msb << 16) | ((unsigned int) lsb << 8) | (unsigned int) xlsb)
							>> (8 - oversampling);

					// calculate pressure (in Pa) given uncompensated value
					int x3, b3, b6, pressure;
					unsigned int b4, b7;

					// calculate B6
					b6 = b5 - 4000;

					// calculate B3
					x1 = (bmp180data->b2[y] * (b6 * b6) >> 12) >> 11;
					x2 = (bmp180data->ac2[y] * b6) >> 11;
					x3 = x1 + x2;
					b3 = (((bmp180data->ac1[y] * 4 + x3) << oversampling) + 2) >> 2;

					// calculate B4
					x1 = (bmp180data->ac3[y] * b6) >> 13;
					x2 = (bmp180data->b1[y] * ((b6 * b6) >> 12)) >> 16;
					x3 = ((x1 + x2) + 2) >> 2;
					b4 = (bmp180data->ac4[y] * (unsigned int) (x3 + 32768)) >> 15;

					// calculate B7
					b7 = ((up - (unsigned int) b3) * ((unsigned int) 50000 >> oversampling));

					// calculate pressure in Pa
					pressure = b7 < 0x80000000 ? (int) ((b7 << 1) / b4) : (int) ((b7 / b4) << 1);
					x1 = (pressure >> 8) * (pressure >> 8);
					x1 = (x1 * 3038) >> 16;
					x2 = (-7357 * pressure) >> 16;
					pressure += (x1 + x2 + 3791) >> 4;

					bmp180->message = json_mkobject();
					JsonNode *code = json_mkobject();
					json_append_member(code, "id", json_mkstring(bmp180data->id[y]));
					json_append_member(code, "temperature", json_mknumber(((double) temp / 10) + temp_offset, 1)); // in deg C
					json_append_member(code, "pressure", json_mknumber(((double) pressure / 100) + pressure_offset, 1)); // in hPa

					json_append_member(bmp180->message, "message", code);
					json_append_member(bmp180->message, "origin", json_mkstring("receiver"));
					json_append_member(bmp180->message, "protocol", json_mkstring(bmp180->id));

					if(pilight.broadcast != NULL) {
						pilight.broadcast(bmp180->id, bmp180->message, PROTOCOL);
					}
					json_delete(bmp180->message);
					bmp180->message = NULL;
				} else {
					logprintf(LOG_NOTICE, "error connecting to bmp180");
					logprintf(LOG_DEBUG, "(probably i2c bus error from wiringXI2CSetup)");
					logprintf(LOG_DEBUG, "(maybe wrong id? use i2cdetect to find out)");
					protocol_thread_wait(node, 1, &nrloops);
				}
			}
			pthread_mutex_unlock(&lock);
		}
	}

	if (bmp180data->id) {
		for (y = 0; y < bmp180data->nrid; y++) {
			FREE(bmp180data->id[y]);
		}
		FREE(bmp180data->id);
	}
	if (bmp180data->ac1) {
		FREE(bmp180data->ac1);
	}
	if (bmp180data->ac2) {
		FREE(bmp180data->ac2);
	}
	if (bmp180data->ac3) {
		FREE(bmp180data->ac3);
	}
	if (bmp180data->ac4) {
		FREE(bmp180data->ac4);
	}
	if (bmp180data->ac5) {
		FREE(bmp180data->ac5);
	}
	if (bmp180data->ac6) {
		FREE(bmp180data->ac6);
	}
	if (bmp180data->b1) {
		FREE(bmp180data->b1);
	}
	if (bmp180data->b2) {
		FREE(bmp180data->b2);
	}
	if (bmp180data->mb) {
		FREE(bmp180data->mb);
	}
	if (bmp180data->mc) {
		FREE(bmp180data->mc);
	}
	if (bmp180data->md) {
		FREE(bmp180data->md);
	}
	if (bmp180data->fd) {
		for (y = 0; y < bmp180data->nrid; y++) {
			if (bmp180data->fd[y] > 0) {
				close(bmp180data->fd[y]);
			}
		}
		FREE(bmp180data->fd);
	}
	FREE(bmp180data);
	threads--;

	return (void *) NULL;
}

static struct threadqueue_t *initDev(JsonNode *jdevice) {
	char *platform = GPIO_PLATFORM;
	if(settings_find_string("gpio-platform", &platform) != 0 || strcmp(platform, "none") == 0) {
		logprintf(LOG_ERR, "gpio_switch: no gpio-platform configured");
		exit(EXIT_FAILURE);
	}
	if(wiringXSetup(platform, logprintf1) == 0) {
		loop = 1;
		char *output = json_stringify(jdevice, NULL);
		JsonNode *json = json_decode(output);
		json_free(output);

		struct protocol_threads_t *node = protocol_thread_init(bmp180, json);
		return threads_register("bmp180", &thread, (void *) node, 0);
	} else {
		return NULL;
	}
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(bmp180);
	while (threads > 0) {
		usleep(10);
	}
	protocol_thread_free(bmp180);
}
#endif

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void bmp180Init(void) {
#if !defined(__FreeBSD__) && !defined(_WIN32)
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
#endif

	protocol_register(&bmp180);
	protocol_set_id(bmp180, "bmp180");
	protocol_device_add(bmp180, "bmp180", "I2C Barometric Pressure and Temperature Sensor");
	protocol_device_add(bmp180, "bmp085", "I2C Barometric Pressure and Temperature Sensor");
	bmp180->devtype = WEATHER;
	bmp180->hwtype = SENSOR;

	options_add(&bmp180->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "0x[0-9a-f]{2}");
	options_add(&bmp180->options, 'o', "oversampling", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *) 1, "^[0123]$");
	options_add(&bmp180->options, 'p', "pressure", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, (void *) 0, "^[0-9]{1,3}$");
	options_add(&bmp180->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, (void *) 0, "^[0-9]{1,3}$");
	options_add(&bmp180->options, 'd', "i2c-path", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^/dev/i2c-[0-9]{1,2}$");

	options_add(&bmp180->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *) 10, "[0-9]");
	options_add(&bmp180->options, 0, "pressure-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *) 0, "[0-9]");
	options_add(&bmp180->options, 0, "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *) 0, "[0-9]");
	options_add(&bmp180->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *) 1, "[0-9]");
	options_add(&bmp180->options, 0, "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *) 1, "[0-9]");
	options_add(&bmp180->options, 0, "pressure-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *) 1, "[0-9]");
	options_add(&bmp180->options, 0, "show-pressure", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *) 1, "^[10]{1}$");
	options_add(&bmp180->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *) 1, "^[10]{1}$");

#if !defined(__FreeBSD__) && !defined(_WIN32)
	bmp180->initDev = &initDev;
	bmp180->threadGC = &threadGC;
#endif
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "bmp180";
	module->version = "2.2";
	module->reqversion = "7.0";
	module->reqcommit = "186";
}

void init(void) {
	bmp180Init();
}
#endif
