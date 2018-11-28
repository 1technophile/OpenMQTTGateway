/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mbedtls/sha256.h>

#include "sha256cache.h"
#include "common.h"
#include "mem.h"
#include "log.h"
#include "gc.h"

int sha256cache_gc(void) {
	struct sha256cache_t *tmp = sha256cache;
	while(sha256cache) {
		tmp = sha256cache;
		FREE(tmp->name);
		sha256cache = sha256cache->next;
		FREE(tmp);
	}
	if(sha256cache != NULL) {
		FREE(sha256cache);
	}

	logprintf(LOG_DEBUG, "garbage collected sha256cache library");
	return 1;
}

void sha256cache_remove_node(struct sha256cache_t **cache, char *name) {
	struct sha256cache_t *currP, *prevP;

	prevP = NULL;

	for(currP = *cache; currP != NULL; prevP = currP, currP = currP->next) {

		if(strcmp(currP->name, name) == 0) {
			if(prevP == NULL) {
				*cache = currP->next;
			} else {
				prevP->next = currP->next;
			}

			FREE(currP->name);
			FREE(currP);

			break;
		}
	}
}

int sha256cache_rm(char *name) {
	sha256cache_remove_node(&sha256cache, name);

	logprintf(LOG_DEBUG, "removed %s from cache", name);
	return 0;
}

int sha256cache_add(char *name) {
	logprintf(LOG_INFO, "cached new sha256 hash");

	unsigned char output[33];
	char *password = NULL;
	int i = 0, x = 0, len = 65;
	mbedtls_sha256_context ctx;

	if(strlen(name) < 64) {
		len = 65;
	} else {
		len = strlen(name)+1;
	}

	if((password = MALLOC(len)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strncpy(password, name, len);

	struct sha256cache_t *node = MALLOC(sizeof(struct sha256cache_t));
	if(node == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	if((node->name = MALLOC(strlen(name)+1)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy(node->name, name);

	for(i=0;i<SHA256_ITERATIONS;i++) {
		mbedtls_sha256_init(&ctx);
		mbedtls_sha256_starts(&ctx, 0);
		mbedtls_sha256_update(&ctx, (unsigned char *)password, strlen((char *)password));
		mbedtls_sha256_finish(&ctx, output);
		for(x=0;x<64;x+=2) {
			sprintf(&password[x], "%02x", output[x/2]);
		}
		mbedtls_sha256_free(&ctx);
	}

	for(i=0;i<64;i+=2) {
		sprintf(&node->hash[i], "%02x", output[i/2]);
	}

	node->next = sha256cache;
	sha256cache = node;
	FREE(password);
	return 0;
}

char *sha256cache_get_hash(char *name) {
	struct sha256cache_t *ftmp = sha256cache;
	while(ftmp) {
		if(strcmp(ftmp->name, name) == 0) {
			return ftmp->hash;
		}
		ftmp = ftmp->next;
	}
	return NULL;
}
