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
#include "libs/pilight/core/gc.h"
#include "libs/pilight/core/common.h"
#include "libs/pilight/core/log.h"
#include "libs/pilight/core/options.h"
#include "libs/pilight/core/firmware.h"

#include "libs/pilight/events/events.h"

int main(int argc, char **argv) {
	// memtrack();

	atomicinit();
	log_shell_enable();
	log_file_disable();

	struct options_t *options = NULL;
	char *configtmp = MALLOC(strlen(CONFIG_FILE)+1);
	char *args = NULL;
	char *fwfile = NULL;
	char comport[255];

	memset(&comport, '\0', 255);

	strcpy(configtmp, CONFIG_FILE);

	if((progname = MALLOC(15)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(progname, "pilight-flash");

	options_add(&options, 'H', "help", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'V', "version", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'C', "config", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'f', "file", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'p', "comport", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);

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
				printf("\t -p --comport\t\tserial COM port\n");
				printf("\t -f --file=firmware\tfirmware file\n");
				goto close;
			break;
			case 'V':
				printf("%s v%s\n", progname, PILIGHT_VERSION);
				goto close;
			break;
			case 'C':
				configtmp = REALLOC(configtmp, strlen(args)+1);
				strcpy(configtmp, args);
			break;
			case 'p':
				strcpy(comport, args);
			break;
			case 'f':
				if(access(args, F_OK) != -1) {
					fwfile = REALLOC(fwfile, strlen(args)+1);
					strcpy(fwfile, args);
				} else {
					fprintf(stderr, "%s: the firmware file %s does not exist\n", progname, args);
					goto close;
				}
			break;
			default:
				printf("Usage: %s -f pilight_firmware_tX5_v3.hex\n", progname);
				goto close;
			break;
		}
	}

	if(config_set_file(configtmp) == EXIT_FAILURE) {
		goto close;
	}

	protocol_init();
	config_init();
	if(config_read() != EXIT_SUCCESS) {
		FREE(configtmp);
		goto close;
	}

#ifdef _WIN32
	if(fwfile == NULL || strlen(fwfile) == 0 || strlen(comport) == 0) {
		printf("Usage: %s -f pilight_firmware_XXX_vX.hex -p comX\n", progname);
		goto close;
	}
#else
	if(fwfile == NULL || strlen(fwfile) == 0) {
		printf("Usage: %s -f pilight_firmware_XXX_vX.hex\n", progname);
		goto close;
	}
#endif

	log_level_set(LOG_DEBUG);
	firmware.version = 0;
	logprintf(LOG_INFO, "**** START UPD. FW ****");
	firmware_getmp(comport);

	if(firmware_update(fwfile, comport) != 0) {
		logprintf(LOG_INFO, "**** FAILED UPD. FW ****");
	} else {
		logprintf(LOG_INFO, "**** DONE UPD. FW ****");
	}

close:
	log_shell_disable();
	options_delete(options);
	if(fwfile != NULL) {
		FREE(fwfile);
	}
	if(configtmp != NULL) {
		FREE(configtmp);
	}
	log_shell_disable();
	log_level_set(LOG_ERR);
	config_gc();
	protocol_gc();
	options_gc();
#ifdef EVENTS
	events_gc();
#endif
#ifndef _WIN32
	wiringXGC();
#endif
	log_gc();
	threads_gc();
	gc_clear();
	FREE(progname);
	xfree();
	return (EXIT_SUCCESS);
}
