/*
	Copyright (C) 2015 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define MSG_NOSIGNAL 0
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/time.h>
#endif
#include <sys/stat.h>
#include <ctype.h>
#include <mbedtls/sha256.h>

#include "libs/libuv/uv.h"
#include "libs/pilight/core/log.h"
#include "libs/pilight/core/common.h"
#include "libs/pilight/core/options.h"

static uv_signal_t *signal_req = NULL;

int main_gc(void) {
	log_shell_disable();

	eventpool_gc();
	options_gc();
	log_gc();
	FREE(progname);

	return EXIT_SUCCESS;
}

void signal_cb(uv_signal_t *handle, int signum) {
	uv_stop(uv_default_loop());
	main_gc();
}

void close_cb(uv_handle_t *handle) {
	FREE(handle);
}

static void walk_cb(uv_handle_t *handle, void *arg) {
	uv_close(handle, close_cb);
}

int main(int argc, char **argv) {
	const uv_thread_t pth_cur_id = uv_thread_self();
	memcpy((void *)&pth_main_id, &pth_cur_id, sizeof(uv_thread_t));

	pilight.process = PROCESS_CLIENT;

#ifdef PILIGHT_REWRITE
	uv_replace_allocator(_MALLOC, _REALLOC, _CALLOC, _FREE);

	log_init();
#endif
	log_shell_enable();
	log_file_disable();
	log_level_set(LOG_NOTICE);

	struct options_t *options = NULL;
  unsigned char output[33];
	char converted[65], *password = NULL, *args = NULL;
	mbedtls_sha256_context ctx;
	int i = 0, x = 0;

	if((progname = MALLOC(15)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy(progname, "pilight-sha256");

	if((signal_req = malloc(sizeof(uv_signal_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	uv_signal_init(uv_default_loop(), signal_req);
	uv_signal_start(signal_req, signal_cb, SIGINT);	

	options_add(&options, 'H', "help", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'V', "version", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'p', "password", OPTION_HAS_VALUE, 0, JSON_STRING, NULL, NULL);

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
				printf("\t -p --password=password\tpassword to encrypt\n");
				goto close;
			break;
			case 'V':
				printf("%s v%s\n", progname, PILIGHT_VERSION);
				goto close;
			break;
			case 'p':
				if((password = MALLOC(strlen(args)+1)) == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				strcpy(password, args);
			break;
			default:
				printf("Usage: %s [options]\n", progname);
				goto close;
			break;
		}
	}
	options_delete(options);

	if(password == NULL) {
		printf("Usage: %s [options]\n", progname);
		printf("\t -H --help\t\tdisplay usage summary\n");
		printf("\t -V --version\t\tdisplay version\n");
		printf("\t -p --password=password\tpassword to encrypt\n");
		goto close;
	}

	if(strlen(password) < 64) {
		if((password = REALLOC(password, 65)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
	}

	for(i=0;i<SHA256_ITERATIONS;i++) {
		mbedtls_sha256_init(&ctx);
		mbedtls_sha256_starts(&ctx, 0);
		mbedtls_sha256_update(&ctx, (unsigned char *)password, strlen((char *)password));
		mbedtls_sha256_finish(&ctx, output);
		for(x=0;x<64;x+=2) {
			sprintf(&password[x], "%02x", output[x/2] );
		}
		mbedtls_sha256_free(&ctx);
	}

	for(x=0;x<64;x+=2) {
		sprintf(&converted[x], "%02x", output[x/2] );
	}

	printf("%s\n", converted);
	mbedtls_sha256_free(&ctx);

close:
	signal_cb(NULL, SIGINT);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);	
	uv_walk(uv_default_loop(), walk_cb, NULL);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	while(uv_loop_close(uv_default_loop()) == UV_EBUSY) {
		usleep(10);
	}

	if(password != NULL) {
		FREE(password);
	}

	main_gc();

	return (EXIT_FAILURE);
}
