/*
	Copyright (C) 2015 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pilight.h"
#include "ssl.h"
#include "mem.h"
#include "log.h"

#include "../config/settings.h"

static int client_success = 0;
static int server_success = 0;

int ssl_client_init_status(void) {
	return client_success;
}

int ssl_server_init_status(void) {
	return server_success;
}

void ssl_init(void) {
	char *pemfile = NULL, buffer[BUFFER_SIZE];
	int ret = 0, pem_free = 0;

#ifdef PILIGHT_REWRITE
	if(settings_select_string(ORIGIN_WEBSERVER, "pem-file", &pemfile) != 0) {
#else
	if(settings_find_string("pem-file", &pemfile) != 0) {
#endif
		if((pemfile = REALLOC(pemfile, strlen(PEM_FILE)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(pemfile, PEM_FILE);
		pem_free = 1;
	}
	if(file_exists(pemfile) != 0) {
		logprintf(LOG_NOTICE, "pemfile does not exists: %s", pemfile);
		server_success = -1;
	}

	if(server_success == 0) {
		mbedtls_ssl_config_init(&ssl_server_conf);
	}

	mbedtls_ssl_config_init(&ssl_client_conf);
	mbedtls_entropy_init(&ssl_entropy);
	mbedtls_ctr_drbg_init(&ssl_ctr_drbg);
	mbedtls_ssl_cache_init(&ssl_cache);

	if((ret = mbedtls_ssl_config_defaults(&ssl_client_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		/*LCOV_EXCL_START*/
		mbedtls_strerror(ret, (char *)&buffer, BUFFER_SIZE);
		logprintf(LOG_ERR, "mbedtls_ctr_drbg_seed failed: %s", buffer);
		client_success = -1;
		/*LCOV_EXCL_STOP*/
	}

	if((ret = mbedtls_ssl_config_defaults(&ssl_server_conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		mbedtls_strerror(ret, (char *)&buffer, BUFFER_SIZE);
		logprintf(LOG_ERR, "mbedtls_ssl_config_defaults failed: %s", buffer);
		server_success = -1;
	}

	if(client_success == 0 && server_success == 0) {
	  if((ret = mbedtls_ctr_drbg_seed(&ssl_ctr_drbg, mbedtls_entropy_func, &ssl_entropy, (const unsigned char *)"pilight", strlen("pilight"))) != 0) {
			/*LCOV_EXCL_START*/
			mbedtls_strerror(ret, (char *)&buffer, BUFFER_SIZE);
			logprintf(LOG_ERR, "mbedtls_ctr_drbg_seed failed: %s", buffer);
			server_success = -1;
			client_success = -1;
			/*LCOV_EXCL_STOP*/
		}
	}

	mbedtls_ssl_conf_rng(&ssl_client_conf, mbedtls_ctr_drbg_random, &ssl_ctr_drbg);
	mbedtls_ssl_conf_rng(&ssl_server_conf, mbedtls_ctr_drbg_random, &ssl_ctr_drbg);
	mbedtls_ssl_conf_authmode(&ssl_client_conf, MBEDTLS_SSL_VERIFY_NONE);
	mbedtls_ssl_conf_session_cache(&ssl_server_conf, &ssl_cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);
	mbedtls_ssl_conf_session_cache(&ssl_client_conf, &ssl_cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);

	memset(&ssl_server_crt, 0, sizeof(mbedtls_x509_crt));
	mbedtls_x509_crt_init(&ssl_server_crt);

	if(server_success == 0) {
		if((ret = mbedtls_x509_crt_parse_file(&ssl_server_crt, pemfile)) != 0) {
			mbedtls_strerror(ret, (char *)&buffer, BUFFER_SIZE);
			logprintf(LOG_NOTICE, "mbedtls_x509_crt_parse_file failed: %s", buffer);
			server_success = -1;
		}
	}

	mbedtls_pk_init(&ssl_pk_key);
	if(server_success == 0) {
		if((ret = mbedtls_pk_parse_keyfile(&ssl_pk_key, pemfile, NULL)) != 0) {
			mbedtls_strerror(ret, (char *)&buffer, BUFFER_SIZE);
			logprintf(LOG_NOTICE, "mbedtls_pk_parse_keyfile failed: %s", buffer);
			server_success = -1;
		}
	}

	mbedtls_ssl_conf_ca_chain(&ssl_server_conf, ssl_server_crt.next, NULL);

	if(server_success == 0) {
		if((ret = mbedtls_ssl_conf_own_cert(&ssl_server_conf, &ssl_server_crt, &ssl_pk_key)) != 0) {
			/*LCOV_EXCL_START*/
			mbedtls_strerror(ret, (char *)&buffer, BUFFER_SIZE);
			logprintf(LOG_ERR, "mbedtls_ssl_conf_own_cert failed: %s", buffer);
			server_success = -1;
			/*LCOV_EXCL_STOP*/
		}
	}

	if(pem_free == 1) {
		FREE(pemfile);
	}
}

void ssl_gc(void) {
	client_success = 0;
	server_success = 0;

	mbedtls_entropy_free(&ssl_entropy);
	mbedtls_ssl_config_free(&ssl_server_conf);
	mbedtls_ssl_config_free(&ssl_client_conf);
	mbedtls_x509_crt_free(&ssl_server_crt);
	mbedtls_pk_free(&ssl_pk_key);
	mbedtls_ssl_cache_free(&ssl_cache);
}
