/*
	Copyright (C) 2013 CurlyMo

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

#ifndef _THREADS_H_
#define _THREADS_H_

#ifdef _WIN32
	#include "windows.h"
#endif
#include <pthread.h>
#include "proc.h"

struct threadqueue_t {
	unsigned int ts;
	pthread_t pth;
#ifdef _WIN32
	HANDLE handle;
#endif
	int force;
	char *id;
	void *param;
	unsigned int running;
	struct cpu_usage_t cpu_usage;
	void *(*function)(void *param);
	struct threadqueue_t *next;
} threadqueue_t;

struct threadqueue_t *threads_register(const char *id, void *(*function)(void* param), void *param, int force);
void threads_create(pthread_t *pth, const pthread_attr_t *attr,  void *(*start_routine) (void *), void *arg);
void threads_start(void);
void thread_stop(char *id);
void threads_cpu_usage(int print);
int threads_gc(void);
void thread_signal(char *id, int signal);

#endif
