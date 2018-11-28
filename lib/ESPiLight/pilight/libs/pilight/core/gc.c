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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <errno.h>
#ifdef _WIN32
	#include <windows.h>
#else
	#include <execinfo.h>
	#define UNW_LOCAL_ONLY
	#ifndef __mips__
		#include <libunwind.h>
	#endif
#endif
#include <signal.h>

#include "pilight.h"
#include "gc.h"
#include "json.h"
#include "log.h"
#include "mem.h"
#include "common.h"
#include "config.h"

static unsigned short gc_enable = 1;
static int (*gc)(void) = NULL;

void gc_handler(int sig) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);
#if !defined(_WIN32) && !defined(__mips__)
	char name[256];
	unw_cursor_t cursor; unw_context_t uc;
	unw_word_t ip, sp, offp;
#endif

	switch(sig) {
		case SIGSEGV:
#ifndef _WIN32
		case SIGBUS:
#endif
		case SIGILL:
		case SIGABRT:
		case SIGFPE: {
#if !defined(_WIN32) && !defined(__mips__)
			log_shell_disable();
			void *stack[50];
			int n = backtrace(stack, 50);
			printf("-- STACKTRACE (%d FRAMES) --\n", n);
			logerror("-- STACKTRACE (%d FRAMES) --", n);

			unw_getcontext(&uc);
			unw_init_local(&cursor, &uc);

			while(unw_step(&cursor) > 0) {
				name[0] = '\0';
				unw_get_proc_name(&cursor, name, 256, &offp);
				unw_get_reg(&cursor, UNW_REG_IP, &ip);
				unw_get_reg(&cursor, UNW_REG_SP, &sp);

				printf("%-30s ip = %10p, sp = %10p\n", name, (void *)ip, (void *)sp);
				logerror("%-30s ip = %10p, sp = %10p", name, (void *)ip, (void *)sp);
			}
#endif
		}
		break;
		default:;
	}
	if(sig == SIGSEGV) {
		fprintf(stderr, "segmentation fault\n");
		exit(EXIT_FAILURE);
#ifndef _WIN32
	} else if(sig == SIGBUS) {
		fprintf(stderr, "buserror\n");
		exit(EXIT_FAILURE);
#endif
	}
#ifdef _WIN32
	if(((sig == SIGINT || sig == SIGTERM) && gc_enable == 1) ||
		(!(sig == SIGINT || sig == SIGTERM) && gc_enable == 0)) {
#else
	if(((sig == SIGINT || sig == SIGTERM || sig == SIGTSTP) && gc_enable == 1) ||
		(!(sig == SIGINT || sig == SIGTERM || sig == SIGTSTP) && gc_enable == 0)) {
#endif
		if(sig == SIGINT) {
			logprintf(LOG_DEBUG, "received interrupt signal, stopping pilight...");
		} else if(sig == SIGTERM) {
			logprintf(LOG_DEBUG, "received terminate signal, stopping pilight...");
		} else {
			logprintf(LOG_DEBUG, "received stop signal, stopping pilight...");
		}
		if(config_get_file() != NULL && gc_enable == 1) {
			gc_enable = 0;
			if(pilight.runmode == STANDALONE) {
				config_write(1, "all");
			}
		}
		gc_enable = 0;
		gc_run();
	}
}

/* Add function to gc */
void gc_attach(int (*fp)(void)) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);
	if(gc != NULL) {
		logprintf(LOG_ERR, "multiple calls to gc_attach", __FUNCTION__);
	}
	gc = fp;
}

void gc_clear(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);
	gc = NULL;
}

/* Run the GC manually */
int gc_run(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(gc != NULL) {
		if(gc() == 0) {
			return EXIT_SUCCESS;
		} else {
			return EXIT_FAILURE;
		}
	} else {
		return EXIT_SUCCESS;
	}
}

/* Initialize the catch all gc */
void gc_catch(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

#ifdef _WIN32
	// signal(SIGABRT, gc_handler);
	// signal(SIGFPE, gc_handler);
	// signal(SIGILL, gc_handler);
	// signal(SIGINT, gc_handler);
	// signal(SIGSEGV, gc_handler);
	// signal(SIGTERM, gc_handler);
#else
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = gc_handler;
	sigemptyset(&act.sa_mask);
	sigaction(SIGINT,  &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	sigaction(SIGABRT, &act, NULL);
	sigaction(SIGTSTP, &act, NULL);

	sigaction(SIGBUS,  &act, NULL);
	sigaction(SIGILL,  &act, NULL);
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGFPE,  &act, NULL);
#endif
}
