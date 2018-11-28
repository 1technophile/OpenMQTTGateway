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

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_DEPRECATE
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <unistd.h>
	#include <pthread.h>
#else
	#include <windows.h>
	#define strdup(a) _strdup(a)
#endif

#include "mem.h"

static unsigned short memdbg = 0;
static int lockinit = 0;
static unsigned long openallocs = 0;
static unsigned long totalnrallocs = 0;
#ifdef _WIN32
	HANDLE lock;
#else
	static pthread_mutex_t lock;
	static pthread_mutexattr_t attr;
#endif

struct mallocs_t {
	void *p;
	unsigned long size;
	int line;
	char file[255];
	struct mallocs_t *next;
} mallocs_t;

static struct mallocs_t *mallocs = NULL;

void memtrack(void) {
	memdbg = 1;

	if(lockinit == 0) {
		lockinit = 1;
#ifdef _WIN32
		lock = CreateMutex(NULL, FALSE, NULL);
#else
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&lock, &attr);
#endif
	}
}

int xfree(void) {
	if(memdbg == 1) {
#ifdef _WIN32
		DWORD dwWaitResult = WaitForSingleObject(lock, INFINITE);
#else
		pthread_mutex_lock(&lock);
#endif
		unsigned long totalsize = 0;
		struct mallocs_t *tmp;
		while(mallocs) {
			tmp = mallocs;
			totalsize += mallocs->size;
			fprintf(stderr, "WARNING: unfreed pointer (%p) in %s at line #%d\n", mallocs->p, tmp->file, tmp->line);
			free(mallocs->p);
			mallocs = mallocs->next;
			free(tmp);
		}
		free(mallocs);
#ifdef _WIN32
		ReleaseMutex(lock);
#else
		pthread_mutex_unlock(&lock);
#endif

		/*fprintf(stderr, "%s: leaked %lu bytes from pilight libraries and programs.\n",
										(totalsize > 0) ? "ERROR" : "NOTICE", totalsize);
		fprintf(stderr, "NOTICE: memory allocations total: %lu, still open: %lu\n", totalnrallocs, openallocs);*/
		totalnrallocs = 0;
		memdbg = 2;
	}
	return openallocs;
}

void *__malloc(unsigned long a, const char *file, int line) {
	if(memdbg == 1) {
		struct mallocs_t *node = malloc(sizeof(mallocs_t));
		if((node->p = malloc(a)) == NULL) {
			fprintf(stderr, "out of memory in %s at line #%d\n", file, line);
			free(node);
			xfree();
			exit(EXIT_FAILURE);
		}
#ifdef _WIN32
		InterlockedIncrement(&openallocs);
		InterlockedIncrement(&totalnrallocs);
#else
		__sync_add_and_fetch(&openallocs, 1);
		__sync_add_and_fetch(&totalnrallocs, 1);
#endif
		node->size = a;
		node->line = line;
		strcpy(node->file, file);

#ifdef _WIN32
		DWORD dwWaitResult = WaitForSingleObject(lock, INFINITE);
#else
		pthread_mutex_lock(&lock);
#endif
		node->next = mallocs;
		mallocs = node;
#ifdef _WIN32
		ReleaseMutex(lock);
#else
		pthread_mutex_unlock(&lock);
#endif

		return node->p;
	} else {
		return malloc(a);
	}
}

char *___strdup(char *a, const char *file, int line) {
	char *d = __malloc(strlen(a) + 1, file, line);
	if(d == NULL) {
		return NULL;
	}
	strcpy(d, a);
	return d;
}

void *__realloc(void *a, unsigned long b, const char *file, int line) {
	if(memdbg == 1) {
		if(a == NULL) {
			return __malloc(b, file, line);
		} else {
#ifdef _WIN32
			DWORD dwWaitResult = WaitForSingleObject(lock, INFINITE);
#else
			pthread_mutex_lock(&lock);
#endif
			struct mallocs_t *tmp = mallocs;
			while(tmp) {
				if(tmp->p == a) {
					tmp->line = line;
					strcpy(tmp->file, file);
					tmp->size = b;
					if((a = realloc(a, b)) == NULL) {
						fprintf(stderr, "out of memory in %s at line #%d\n", file, line);
#ifdef _WIN32
						ReleaseMutex(lock);
#else
						pthread_mutex_unlock(&lock);
#endif
						xfree();
						exit(EXIT_FAILURE);
					}
					tmp->p = a;
					break;
				}
				tmp = tmp->next;
			}
#ifdef _WIN32
			ReleaseMutex(lock);
#else
			pthread_mutex_unlock(&lock);
#endif
			if(tmp == NULL) {
				fprintf(stderr, "ERROR: calling realloc on an unknown pointer in %s at line #%d\n", file, line);
				return __malloc(b, file, line);
			} else if(tmp != NULL && tmp->p != NULL) {
				return a;
			} else {
				return __malloc(b, file, line);
			}
		}
	} else {
		return realloc(a, b);
	}
}

void *__calloc(unsigned long a, unsigned long b, const char *file, int line) {
	if(memdbg == 1) {
		struct mallocs_t *node = malloc(sizeof(mallocs_t));
		if((node->p = malloc(a*b)) == NULL) {
			fprintf(stderr, "out of memory in %s at line #%d\n", file, line);
			free(node);
			xfree();
			exit(EXIT_FAILURE);
		}
#ifdef _WIN32
		InterlockedIncrement(&openallocs);
		InterlockedIncrement(&totalnrallocs);
#else
		__sync_add_and_fetch(&openallocs, 1);
		__sync_add_and_fetch(&totalnrallocs, 1);
#endif
		memset(node->p, '\0', a*b);
		node->size = a*b;
		node->line = line;
		strcpy(node->file, file);

#ifdef _WIN32
		DWORD dwWaitResult = WaitForSingleObject(lock, INFINITE);
#else
		pthread_mutex_lock(&lock);
#endif
		node->next = mallocs;
		mallocs = node;
#ifdef _WIN32
		ReleaseMutex(lock);
#else
		pthread_mutex_unlock(&lock);
#endif

		return node->p;
	} else {
		return calloc(a, b);
	}
}

void __free(void *a, const char *file, int line) {
	if(memdbg == 2) {
		fprintf(stderr, "WARNING: calling free after xfree was called in %s at line #%d\n", file, line);
	}
	if(memdbg == 1) {
		if(a == NULL) {
			fprintf(stderr, "WARNING: calling free on already freed pointer in %s at line #%d\n", file, line);
		} else {
#ifdef _WIN32
			DWORD dwWaitResult = WaitForSingleObject(lock, INFINITE);
#else
			pthread_mutex_lock(&lock);
#endif
			struct mallocs_t *currP, *prevP;
			int match = 0;

			prevP = NULL;

			for(currP = mallocs; currP != NULL; prevP = currP, currP = currP->next) {
				if(currP->p == a) {
					match = 1;
					if(prevP == NULL) {
						mallocs = currP->next;
					} else {
						prevP->next = currP->next;
					}
					free(currP->p);
					free(currP);

					break;
				}
			}
#ifdef _WIN32
			ReleaseMutex(lock);
			InterlockedDecrement(&openallocs);
#else
			__sync_add_and_fetch(&openallocs, -1);
			pthread_mutex_unlock(&lock);
#endif

			if(match == 0) {
				fprintf(stderr, "ERROR: trying to free an unknown pointer in %s at line #%d\n", file, line);
				free(a);
			}
		}
	} else {
		free(a);
	}
}
