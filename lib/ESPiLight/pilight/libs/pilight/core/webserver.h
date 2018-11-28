/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#ifdef WEBSERVER_HTTPS
#include <mbedtls/ssl.h>
#endif

#include "../libs/libuv/uv.h"

typedef struct connection_t {
	int fd;
	char *request;
  const char *request_method;
  const char *uri;
  const char *http_version;
  const char *query_string;

  int num_headers;
  struct headers {
    const char *name;
    const char *value;
  } http_headers[30];

  char *content;
  size_t content_len;
  char mimetype[255];

  int is_websocket;
	int ping;
  int status_code;
	void *connection_param;
  void *callback_param;
  unsigned int flags;
	unsigned short timer;

	int file_fd;

	char buffer[WEBSERVER_CHUNK_SIZE];

#ifdef WEBSERVER_HTTPS
	int is_ssl;
	int handshake;
	mbedtls_ssl_context ssl;
#endif
} connection_t;

int webserver_gc(void);
int webserver_start(void);
void *webserver_broadcast(void *);
void webserver_create_header(char **, const char *, char *, unsigned long);
int http_parse_request(char *, struct connection_t *);
const char *http_get_header(struct connection_t *, const char *);
void *webserver_clientize(void *param);

#endif