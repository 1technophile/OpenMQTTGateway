/*
	Copyright (C) 2014 CurlyMo

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

#ifndef _PROC_H_
#define _PROC_H_

#include <time.h>

/* CPU usage */
typedef struct cpu_usage_t {
	double sec_start;
	double sec_stop;
	double sec_diff;
	double cpu_old;
	double cpu_new;
	double cpu_per;
	struct timespec ts;
	clock_t starts;
} cpu_usage_t;

double getCPUUsage(void);
double getRAMUsage(void);
void getThreadCPUUsage(pthread_t pth, struct cpu_usage_t *cpu_usage);

#endif
