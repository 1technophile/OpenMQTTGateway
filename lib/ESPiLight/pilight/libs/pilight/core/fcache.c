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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fcache.h"
#include "common.h"
#include "mem.h"
#include "log.h"
#include "gc.h"

int fcache_gc(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct fcache_t *tmp = fcache;
	while(fcache) {
		tmp = fcache;
		FREE(tmp->name);
		FREE(tmp->bytes);
		fcache = fcache->next;
		FREE(tmp);
	}
	if(fcache != NULL) {
		FREE(fcache);
	}

	logprintf(LOG_DEBUG, "garbage collected fcache library");
	return 1;
}

void fcache_remove_node(struct fcache_t **cache, char *name) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct fcache_t *currP, *prevP;

	prevP = NULL;

	for(currP = *cache; currP != NULL; prevP = currP, currP = currP->next) {

		if(strcmp(currP->name, name) == 0) {
			if(prevP == NULL) {
				*cache = currP->next;
			} else {
				prevP->next = currP->next;
			}

			FREE(currP->name);
			FREE(currP->bytes);
			FREE(currP);

			break;
		}
	}
}

int fcache_rm(char *filename) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	fcache_remove_node(&fcache, filename);
	logprintf(LOG_DEBUG, "removed %s from cache", filename);
	return 1;
}

int fcache_add(char *filename) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	size_t filesize = 0;
	FILE *fp = NULL;

	logprintf(LOG_NOTICE, "caching %s", filename);

/*
 * dir stat doens't work on Windows if path has a trailing slash
 */
#ifdef _WIN32
	size_t i = strlen(filename);
	if(filename[i-1] == '\\' || filename[i-1] == '/') {
		filename[i-1] = '\0';
	}
#endif

	if((fp = fopen(filename, "rb")) == NULL) {
		logprintf(LOG_NOTICE, "failed to open %s", filename);
		return -1;
	} else {
		struct fcache_t *node = MALLOC(sizeof(struct fcache_t));
		if(node == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		fseek(fp, 0, SEEK_END);
		filesize = (size_t)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if((node->bytes = MALLOC(filesize + 1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		memset(node->bytes, '\0', filesize + 1);
		if(fread(node->bytes, 1, filesize, fp) != filesize) {
			logprintf(LOG_NOTICE, "error reading %s", filename);
			FREE(node->bytes);
			FREE(node);
			fclose(fp);
			return -1;
		}
		node->size = (int)filesize;
		if((node->name = MALLOC(strlen(filename)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(node->name, filename);
		node->next = fcache;
		fcache = node;
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return -1;
}

short fcache_get_size(char *filename, int *out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct fcache_t *ftmp = fcache;
	while(ftmp) {
		if(strcmp(ftmp->name, filename) == 0) {
			*out = ftmp->size;
			return 0;
		}
		ftmp = ftmp->next;
	}
	return -1;
}

unsigned char *fcache_get_bytes(char *filename) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct fcache_t *ftmp = fcache;
	while(ftmp) {
		if(strcmp(ftmp->name, filename) == 0) {
			return ftmp->bytes;
		}
		ftmp = ftmp->next;
	}
	return NULL;
}
