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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#ifndef _WIN32
#include <wiringx.h>
#endif

#include "irq.h"
#include "gc.h"
#include "log.h"
#include "mem.h"

typedef struct timestamp_t {
	unsigned long first;
	unsigned long second;
} timestamp_t;

timestamp_t timestamp;

/* Attaches an interrupt handler to a specific GPIO pin
   Whenever an rising, falling or changing interrupt occurs
   the function given as the last argument will be called */
int irq_read(int gpio) {
#ifndef _WIN32
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int x = waitForInterrupt(gpio, 1000);
	if(x > 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		timestamp.first = timestamp.second;
		timestamp.second = 1000000 * (unsigned int)tv.tv_sec + (unsigned int)tv.tv_usec;
		return (int)timestamp.second-(int)timestamp.first;
	}
	return x;
#else
	return -1;
#endif
}
