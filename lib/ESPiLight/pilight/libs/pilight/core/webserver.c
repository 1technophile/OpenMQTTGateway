/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
 * This webserver contains various code originally written by Sergey Lyubka
 * (from Cesenta) for Mongoose (https://github.com/cesanta/mongoose).
 * The code was either copied, adapted and/or stripped so it could be integrated
 * with the pilight eventpool and mbed TLS SSL library.
 *
 * The following functions are therefor licensed under GPLv2.
 *
 * send_websocket_handshake_if_requested
 * send_websocket_handshake
 * http_parse_request
 * _urldecode
 * parse_http_headers
 * remove_double_dots_and_double_slashes
 * is_valid_http_method
 * *skip
 * websocket_write
 * authorize_input
 * http_get_header
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#ifdef _WIN32
#else
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <pwd.h>
	#include <arpa/inet.h>
	#define __USE_UNIX98
	#include <pthread.h>
	#include <unistd.h>
	#include <sys/time.h>
#endif

#ifdef PILIGHT_REWRITE
	#include "../storage/storage.h"
#else
	#include "../config/devices.h"
	#include "../config/settings.h"
	#include "../config/registry.h"
#endif

#include "eventpool.h"
#include "sha256cache.h"
#include "pilight.h"
#include "network.h"
#include "gc.h"
#include "log.h"
#include "json.h"
#include "webserver.h"
#include "socket.h"
#include "ssdp.h"
#include "common.h"

#include <mbedtls/sha1.h>

#ifdef WEBSERVER_HTTPS
#include <mbedtls/error.h>
#include <mbedtls/pk.h>
#include <mbedtls/version.h>
#if MBEDTLS_VERSION_MAJOR <= 2 && \
    MBEDTLS_VERSION_MINOR <= 3
	#include <mbedtls/net.h>
#else
	#include <mbedtls/net_sockets.h>
#endif
#include <mbedtls/x509_crt.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cache.h>

#include "ssl.h"

static int https_port = WEBSERVER_HTTPS_PORT;
#endif

static uv_poll_t *poll_http_req = NULL;
static uv_poll_t *poll_https_req = NULL;
static uv_async_t *async_req = NULL;

static int http_port = WEBSERVER_HTTP_PORT;
static int websockets = WEBGUI_WEBSOCKETS;
static int cache = 1;
static char *authentication_username = NULL;
static char *authentication_password = NULL;
static unsigned short loop = 1;
static char *root = NULL;
static unsigned short root_free = 0;

typedef struct broadcast_list_t {
	char *out;
	ssize_t len;
	int fd;

	struct broadcast_list_t *next;
} broadcast_list_t;

static struct broadcast_list_t *broadcast_list = NULL;

enum mg_result {
	MG_FALSE,
	MG_TRUE,
	MG_MORE
};

enum {
	WEBSOCKET_OPCODE_CONTINUATION = 0x0,
	WEBSOCKET_OPCODE_TEXT = 0x1,
	WEBSOCKET_OPCODE_BINARY = 0x2,
	WEBSOCKET_OPCODE_CONNECTION_CLOSE = 0x8,
	WEBSOCKET_OPCODE_PING = 0x9,
	WEBSOCKET_OPCODE_PONG = 0xa
};

typedef struct webserver_clients_t {
	uv_poll_t *req;
	int is_websocket;

	struct webserver_clients_t *next;
} webserver_clients_t;

typedef struct fcache_t {
	char *name;
	unsigned long size;
	unsigned char *bytes;
	struct fcache_t *next;
} fcaches_t;

static struct fcache_t *fcache;

#ifdef _WIN32
	static uv_mutex_t webserver_lock;
#else
	static pthread_mutex_t webserver_lock;
	static pthread_mutexattr_t webserver_attr;
#endif
static int lock_init = 0;
static struct webserver_clients_t *webserver_clients = NULL;

static void poll_close_cb(uv_poll_t *req);

static void *reason_socket_received_free(void *param) {
	struct reason_socket_received_t *data = param;
	FREE(data->buffer);
	FREE(data);
	return NULL;
}

static void *reason_webserver_connected_free(void *param) {
	struct reason_webserver_connected_t *data = param;
	FREE(data);
	return NULL;
}

int webserver_gc(void) {
	struct webserver_clients_t *node = NULL;

	loop = 0;

	if(root_free == 1) {
		FREE(root);
	}

#ifdef _WIN32
	uv_mutex_lock(&webserver_lock);
#else
	pthread_mutex_lock(&webserver_lock);
#endif
	{
		while(webserver_clients) {
			node = webserver_clients->next;
			poll_close_cb(webserver_clients->req);
			webserver_clients = node;
		}
	}

	{
		struct broadcast_list_t *tmp = NULL;
		while(broadcast_list) {
			tmp = broadcast_list;
			broadcast_list = broadcast_list->next;
			FREE(tmp);
		}
	}
#ifdef _WIN32
	uv_mutex_unlock(&webserver_lock);
#else
	pthread_mutex_unlock(&webserver_lock);
#endif

	{
		struct fcache_t *tmp = fcache;
		while(fcache) {
			tmp = fcache;
			FREE(tmp->name);
			FREE(tmp->bytes);
			fcache = fcache->next;
			FREE(tmp);
		}
	}

	if(poll_http_req != NULL) {
		poll_close_cb(poll_http_req);
	}
	if(poll_https_req != NULL) {
		poll_close_cb(poll_https_req);
	}

	authentication_username = NULL;
	authentication_password = NULL;

	sha256cache_gc();
	logprintf(LOG_DEBUG, "garbage collected webserver library");
	return 1;
}

void webserver_create_header(char **p, const char *message, char *mimetype, unsigned long len) {
	*p += sprintf((char *)*p,
		"HTTP/1.0 %s\r\n"
		"Server: pilight\r\n"
		"Keep-Alive: timeout=15, max=100\r\n"
		"Content-Type: %s\r\n",
		message, mimetype);
	*p += sprintf((char *)*p,
		"Content-Length: %lu\r\n\r\n",
		len);
}

static void create_404_header(const char *in, char **p) {
	char mimetype[] = "text/html";
	webserver_create_header(p, "404 Not Found", mimetype, (unsigned long)(202+strlen((const char *)in)));
	*p += sprintf((char *)*p, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\x0d\x0a"
		"<html><head>\x0d\x0a"
		"<title>404 Not Found</title>\x0d\x0a"
		"</head><body>\x0d\x0a"
		"<h1>Not Found</h1>\x0d\x0a"
		"<p>The requested URL %s was not found on this server.</p>\x0d\x0a"
		"</body></html>",
		(const char *)in);
}

const char *http_get_header(struct connection_t *conn, const char *s) {
	int i = 0;

	for(i = 0; i < conn->num_headers; i++) {
		if(strcmp(s, conn->http_headers[i].name) == 0) {
			return conn->http_headers[i].value;
		}
	}

	return NULL;
}

void send_auth_request(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;
	char *a = "HTTP/1.1 401 Unauthorized\r\n"
		"WWW-Authenticate: Basic realm=\"pilight\"\r\n\r\n";
	conn->status_code = 401;

	iobuf_append(&custom_poll_data->send_iobuf, a, strlen(a));
	uv_custom_write(req);
}

int authorize_input(uv_poll_t *req, char *username, char *password) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;
	const char *hdr = NULL;
	char **array = NULL, *decoded = NULL;
	int n = 0;

	if(conn == NULL) {
		return MG_FALSE;
	}

	if((hdr = http_get_header(conn, "Authorization")) == NULL ||
	   (strncmp(hdr, "Basic ", 6) != 0 &&
			strncmp(hdr, "basic ", 6) != 0)) {
		return MG_FALSE;
	}

	char *user = MALLOC(strlen(hdr)+1);
	if(user == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy(user, &hdr[6]);

	if((decoded = base64decode(user, strlen(user), NULL)) == NULL) {
		FREE(user);
		return MG_FALSE;
	}

	if((n = explode(decoded, ":", &array)) == 2) {
		if(strlen(array[1]) < 64) {
			if((array[1] = REALLOC(array[1], 65)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
		}

		if(sha256cache_get_hash(array[1]) == NULL) {
			sha256cache_add(array[1]);
		}

		if(strcmp(sha256cache_get_hash(array[1]), password) == 0 && strcmp(array[0], username) == 0) {
			array_free(&array, n);
			FREE(user);
			FREE(decoded);
			return MG_TRUE;
		}
	}
	FREE(user);
	FREE(decoded);
	array_free(&array, n);

	return MG_FALSE;
}

static int auth_handler(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	if(authentication_username != NULL && authentication_password != NULL) {
		return authorize_input(req, authentication_username, authentication_password);
	} else {
		return MG_TRUE;
	}
}

size_t websocket_write(uv_poll_t *req, int opcode, const char *data, unsigned long long data_len) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	unsigned char *copy = NULL;
	size_t copy_len = 0;
	int index = 2;

	if((copy = REALLOC(copy, data_len + 10)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(copy, '\0', data_len + 10);

	copy[0] = 0x80 + (opcode & 0x0f);
	if(data_len <= 125) {
		copy[1] = data_len;
	} else if(data_len < 65535) {
		copy[1] = 126;
		copy[2] = (data_len >> 8) & 255;
		copy[3] = (data_len) & 255;
		index = 4;
	} else {
		copy[1] = 127;
		copy[2] = (data_len >> 56) & 255;
		copy[3] = (data_len >> 48) & 255;
		copy[4] = (data_len >> 40) & 255;
		copy[5] = (data_len >> 32) & 255;
		copy[6] = (data_len >> 24) & 255;
		copy[7] = (data_len >> 16) & 255;
		copy[8] = (data_len >> 8) & 255;
		copy[9] = (data_len) & 255;
		index = 10;
	}
	if(data != NULL) {
		memcpy(&copy[index], data, data_len);
	}
	copy_len = data_len + index;

	if(copy_len > 0) {
		iobuf_append(&custom_poll_data->send_iobuf, (char *)copy, copy_len);
		uv_custom_write(req);
	}

	FREE(copy);
	return data_len;
}

static void *webserver_send(int reason, void *param) {
	struct reason_socket_send_t *data = param;
	struct connection_t c;
	memset(&c, 0, sizeof(struct connection_t));

	if(strcmp(data->type, "websocket") == 0) {
#ifdef _WIN32
		uv_mutex_lock(&webserver_lock);
#else
		pthread_mutex_lock(&webserver_lock);
#endif
		struct broadcast_list_t *node = MALLOC(sizeof(struct broadcast_list_t));
		if(node == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		memset(node, 0, sizeof(struct broadcast_list_t));
		node->fd = data->fd;
		if((node->out = STRDUP(data->buffer)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		node->len = strlen(data->buffer);

		struct broadcast_list_t *tmp = broadcast_list;
		if(tmp != NULL) {
			while(tmp->next != NULL) {
				tmp = tmp->next;
			}
			tmp->next = node;
			node = tmp;
		} else {
			node->next = broadcast_list;
			broadcast_list = node;
		}

#ifdef _WIN32
		uv_mutex_unlock(&webserver_lock);
#else
		pthread_mutex_unlock(&webserver_lock);
#endif
		uv_async_send(async_req);
	}

	return NULL;
}

static void write_chunk(uv_poll_t *req, char *buf, int len) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;

	char chunk_size[50];
	int n = snprintf(chunk_size, sizeof(chunk_size), "%X\r\n", len);

	iobuf_append(&custom_poll_data->send_iobuf, chunk_size, n);
	iobuf_append(&custom_poll_data->send_iobuf, buf, len);
	iobuf_append(&custom_poll_data->send_iobuf, "\r\n", 2);
}

static size_t send_data(uv_poll_t *req, char *mimetype, void *data, unsigned long data_len) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	char header[1024], *p = header;
	memset(header, '\0', 1024);

	webserver_create_header(&p, "200 OK", mimetype, data_len);
	iobuf_append(&custom_poll_data->send_iobuf, header, (int)(p-header));
	iobuf_append(&custom_poll_data->send_iobuf, data, (int)data_len);

	return 0;
}

static size_t send_chunked_data(uv_poll_t *req, void *data, unsigned long data_len) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;

	if(conn->flags == 0) {
		char a[255], *p = a;
		memset(p, '\0', 255);
		int i = snprintf(p, 255,
			"HTTP/1.1 200 OK\r\nKeep-Alive: timeout=15, max=100\r\n"\
			"Content-Type: %s\r\nTransfer-Encoding: chunked\r\n\r\n",
			conn->mimetype
		);

		iobuf_append(&custom_poll_data->send_iobuf, p, i);
		conn->flags = 1;
	}
	write_chunk(req, data, data_len);

	return 0;
}

static int parse_rest(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;

	char **array = NULL, **array1 = NULL;
	struct JsonNode *jobject = json_mkobject();
	struct JsonNode *jcode = json_mkobject();
	struct JsonNode *jvalues = json_mkobject();
	int a = 0, b = 0, c = 0, has_protocol = 0;

	if(strcmp(conn->request_method, "POST") == 0) {
		conn->query_string = conn->content;
	}
	if(strcmp(conn->request_method, "GET") == 0 && conn->query_string == NULL) {
		conn->query_string = conn->content;
	}
	if(conn->query_string != NULL) {
#ifdef PILIGHT_REWRITE
		char *dev = NULL;
#else
		struct devices_t *dev = NULL;
#endif
		int len = urldecode(conn->query_string, NULL);
		if(len == -1) {
			char *z = "{\"message\":\"failed\",\"error\":\"cannot decode url\"}";
			send_data(req, "application/json", z, strlen(z));
			return MG_TRUE;
		}

		char *decoded = MALLOC(len+1);
		if(decoded == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}

		if(urldecode(conn->query_string, decoded) == -1) {
			char *z = "{\"message\":\"failed\",\"error\":\"cannot decode url\"}";
			send_data(req, "application/json", z, strlen(z));
			FREE(decoded);
			return MG_TRUE;
		}

		char state[16], *p = NULL;
		a = explode((char *)decoded, "&", &array), b = 0, c = 0;
		memset(state, 0, 16);

		if(a > 1) {
			int type = 0; //0 = SEND, 1 = CONTROL, 2 = REGISTRY
			json_append_member(jobject, "code", jcode);

			if(strcmp(conn->uri, "/send") == 0) {
				type = 0;
				json_append_member(jobject, "action", json_mkstring("send"));
			} else if(strcmp(conn->uri, "/control") == 0) {
				type = 1;
				json_append_member(jobject, "action", json_mkstring("control"));
			} else if(strcmp(conn->uri, "/registry") == 0) {
				type = 2;
				json_append_member(jobject, "action", json_mkstring("registry"));
			}

			for(b=0;b<a;b++) {
				c = explode(array[b], "=", &array1);
				if(c == 2) {
					if(strcmp(array1[0], "protocol") == 0) {
						struct JsonNode *jprotocol = json_mkarray();
						json_append_element(jprotocol, json_mkstring(array1[1]));
						json_append_member(jcode, "protocol", jprotocol);
						has_protocol = 1;
					} else if(strcmp(array1[0], "state") == 0) {
						strcpy(state, array1[1]);
					} else if(strcmp(array1[0], "device") == 0) {
#ifdef PILIGHT_REWRITE
						dev = array1[1];
						if(devices_select(ORIGIN_WEBSERVER, array1[1], NULL) != 0) {
#else
						if(devices_get(array1[1], &dev) != 0) {
#endif
							char *z = "{\"message\":\"failed\",\"error\":\"device does not exist\"}";
							send_data(req, "application/json", z, strlen(z));
							goto clear;
						}
					} else if(strncmp(array1[0], "values", 6) == 0) {
						char name[255], *ptr = name;
						if(sscanf(array1[0], "values[%254[a-z]]", ptr) != 1) {
							char *z = "{\"message\":\"failed\",\"error\":\"values should be passed like this \'values[dimlevel]=10\'\"}";
							send_data(req, "application/json", z, strlen(z));
							goto clear;
						} else {
							if(isNumeric(array1[1]) == 0) {
								json_append_member(jvalues, name, json_mknumber(atof(array1[1]), nrDecimals(array1[1])));
							} else {
								json_append_member(jvalues, name, json_mkstring(array1[1]));
							}
						}
					} else if(isNumeric(array1[1]) == 0) {
						json_append_member(jcode, array1[0], json_mknumber(atof(array1[1]), nrDecimals(array1[1])));
					} else {
						json_append_member(jcode, array1[0], json_mkstring(array1[1]));
					}
				}
			}
			if(type == 0) {
				if(has_protocol == 0) {
					char *z = "{\"message\":\"failed\",\"error\":\"no protocol was sent\"}";
					send_data(req, "application/json", z, strlen(z));
					goto clear;
				}
				if(pilight.send != NULL) {
					if(pilight.send(jobject, ORIGIN_WEBSERVER) == 0) {
						char *z = "{\"message\":\"success\"}";
						send_data(req, "application/json", z, strlen(z));
						goto clear;
					}
				}
			} else if(type == 1) {
				if(pilight.control != NULL) {
					if(dev == NULL) {
						char *z = "{\"message\":\"failed\",\"error\":\"no device was sent\"}";
						send_data(req, "application/json", z, strlen(z));
						goto clear;
					} else {
						if(strlen(state) > 0) {
							p = state;
						}
						if(pilight.control(dev, p, json_first_child(jvalues), ORIGIN_WEBSERVER) == 0) {
							char *z = "{\"message\":\"success\"}";
							send_data(req, "application/json", z, strlen(z));
							goto clear;
						}
					}
				}
			} else if(type == 2) {
				struct JsonNode *value = NULL;
				char *type = NULL;
				char *key = NULL;
				char *sval = NULL;
				double nval = 0.0;
				int dec = 0;
				if(json_find_string(jcode, "type", &type) != 0) {
					char *z = "{\"message\":\"failed\"}";
					send_data(req, "application/json", z, strlen(z));
					goto clear;
				} else {
					if(strcmp(type, "set") == 0) {
						if(json_find_string(jcode, "key", &key) != 0) {
							char *z = "{\"message\":\"failed\"}";
							send_data(req, "application/json", z, strlen(z));
							goto clear;
						} else if((value = json_find_member(jcode, "value")) == NULL) {
							char *z = "{\"message\":\"failed\"}";
							send_data(req, "application/json", z, strlen(z));
							goto clear;
						} else {
							if(value->tag == JSON_NUMBER) {
								if(registry_set_number(key, value->number_, value->decimals_) == 0) {
									char *z = "{\"message\":\"success\"}";
									send_data(req, "application/json", z, strlen(z));
									goto clear;
								}
							} else if(value->tag == JSON_STRING) {
								if(registry_set_string(key, value->string_) == 0) {
									char *z = "{\"message\":\"success\"}";
									send_data(req, "application/json", z, strlen(z));
									goto clear;
								}
							} else {
								char *z = "{\"message\":\"failed\"}";
								send_data(req, "application/json", z, strlen(z));
								goto clear;
							}
						}
					} else if(strcmp(type, "remove") == 0) {
						if(json_find_string(jcode, "key", &key) != 0) {
							char *z = "{\"message\":\"failed\"}";
							send_data(req, "application/json", z, strlen(z));
							goto clear;
						} else {
							if(registry_remove_value(key) == 0) {
								char *z = "{\"message\":\"success\"}";
								send_data(req, "application/json", z, strlen(z));
								goto clear;
							} else {
								char *z = "{\"message\":\"failed\"}";
								send_data(req, "application/json", z, strlen(z));
								goto clear;
							}
						}
					} else if(strcmp(type, "get") == 0) {
						if(json_find_string(jcode, "key", &key) != 0) {
							char *z = "{\"message\":\"failed\"}";
							send_data(req, "application/json", z, strlen(z));
							goto clear;
						} else {
							if(registry_get_number(key, &nval, &dec) == 0) {
								struct JsonNode *jsend = json_mkobject();
								json_append_member(jsend, "message", json_mkstring("registry"));
								json_append_member(jsend, "value", json_mknumber(nval, dec));
								json_append_member(jsend, "key", json_mkstring(key));
								char *output = json_stringify(jsend, NULL);
								send_data(req, "application/json", output, strlen(output));
								json_free(output);
								json_delete(jsend);
								goto clear;
							} else if(registry_get_string(key, &sval) == 0) {
								struct JsonNode *jsend = json_mkobject();
								json_append_member(jsend, "message", json_mkstring("registry"));
								json_append_member(jsend, "value", json_mkstring(sval));
								json_append_member(jsend, "key", json_mkstring(key));
								char *output = json_stringify(jsend, NULL);
								send_data(req, "application/json", output, strlen(output));
								json_free(output);
								json_delete(jsend);
								goto clear;
							} else {
								char *z = "{\"message\":\"failed\"}";
								send_data(req, "application/json", z, strlen(z));
								goto clear;
							}
						}
					}
				}
			}
			json_delete(jvalues);
			json_delete(jobject);
		}
		FREE(decoded);
		array_free(&array, a);
	}
	char *z = "{\"message\":\"failed\"}";
	send_data(req, "application/json", z, strlen(z));
	return MG_TRUE;

clear:
	array_free(&array1, c);
	array_free(&array, a);
	json_delete(jvalues);
	json_delete(jobject);

	return MG_TRUE;
}

static void close_cb(uv_handle_t *handle) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	FREE(handle);
}

static int file_read_cb(int fd, uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;
	struct timeval timeout;
	fd_set readset;
	char buffer[WEBSERVER_CHUNK_SIZE];

	memset(&buffer, '\0', WEBSERVER_CHUNK_SIZE);
	memset(&readset, 0, sizeof(fd_set));

	FD_SET(fd, &readset);

	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;

	int ret = select(fd+1, &readset, NULL, NULL, &timeout);
	if(ret >= 0 && errno == EINTR) {
		conn->fd = -1;
		close(fd);
		return -1;
	} else if((ret == -1 && errno == EINTR) || ret == 0) {
		return 0;
	}

	int bytes = read(fd, buffer, WEBSERVER_CHUNK_SIZE);

	if(bytes > 0) {
		send_chunked_data(req, &buffer, bytes);
		if(bytes < WEBSERVER_CHUNK_SIZE) {
			iobuf_append(&custom_poll_data->send_iobuf, "0\r\n\r\n", 5);
			uv_custom_close(req);
		}
		uv_custom_write(req);

		return 0;
	} else if(bytes == 0) {
		if(conn->flags == 1) {
			iobuf_append(&custom_poll_data->send_iobuf, "0\r\n\r\n", 5);
			uv_custom_close(req);
		}
		return 0;
	} else if(bytes < 0) {
		logprintf(LOG_ERR, "read: %s", strerror(errno));
		conn->fd = -1;
		close(fd);
		return -1;
	}
	return 0;
}

static int request_handler(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;

	char *ext = NULL;
	char buffer[4096], *p = buffer;

	memset(buffer, '\0', 4096);

	if(conn->is_websocket == 0) {
		if(conn->uri != NULL && strlen(conn->uri) > 0) {
			if(strcmp(conn->uri, "/send") == 0 || strcmp(conn->uri, "/control") == 0 || strcmp(conn->uri, "/registry") == 0) {
				return parse_rest(req);
			} else if(strcmp(conn->uri, "/config") == 0) {
				char media[15];
				int internal = CONFIG_USER;
				strcpy(media, "web");
				if(conn->query_string != NULL) {
					sscanf(conn->query_string, "media=%14s%*[ \n\r]", media);
					if(strstr(conn->query_string, "internal") != NULL) {
						internal = CONFIG_INTERNAL;
					}
				}
				struct JsonNode *jsend = config_print(internal, media);
				if(jsend != NULL) {
					char *output = json_stringify(jsend, NULL);
					send_data(req, "application/json", output, strlen(output));
					json_delete(jsend);
					json_free(output);
				}
				jsend = NULL;
				return MG_TRUE;
			} else if(strcmp(conn->uri, "/values") == 0) {
				char media[15];
				strcpy(media, "web");
				if(conn->query_string != NULL) {
					sscanf(conn->query_string, "media=%14s%*[ \n\r]", media);
				}
#ifdef PILIGHT_REWRITE
				struct JsonNode *jsend = values_print(media);
#else
				JsonNode *jsend = devices_values(media);
#endif
				if(jsend != NULL) {
					char *output = json_stringify(jsend, NULL);
					send_data(req, "application/json", output, strlen(output));
					json_delete(jsend);
					json_free(output);
				}
				jsend = NULL;
				return MG_TRUE;
			} else if(strstr(conn->uri, "/") != NULL && strcmp(&conn->uri[(rstrstr(conn->uri, "/")-conn->uri)], "/") == 0) {
				char indexes[2][11] = {"index.html","index.htm"};

				unsigned q = 0;
				/* Check if the root is terminated by a slash. If not, then add it */
				for(q=0;q<(sizeof(indexes)/sizeof(indexes[0]));q++) {
					size_t l = strlen(root)+strlen(conn->uri)+strlen(indexes[q])+4;
					if((conn->request = REALLOC(conn->request, l)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					memset(conn->request, '\0', l);
					if(root[strlen(root)-1] == '/') {
#ifdef __FreeBSD__
						sprintf(conn->request, "%s/%s%s", root, conn->uri, indexes[q]);
#else
						sprintf(conn->request, "%s%s%s", root, conn->uri, indexes[q]);
#endif
					} else {
						sprintf(conn->request, "%s/%s%s", root, conn->uri, indexes[q]);
					}
					if(access(conn->request, F_OK) == 0) {
						break;
					}
				}
			} else if(root != NULL && conn->uri != NULL) {
				size_t wlen = strlen(root)+strlen(conn->uri)+2;
				if((conn->request = MALLOC(wlen)) == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				memset(conn->request, '\0', wlen);
				/* If a file was requested add it to the webserver path to create the absolute path */
				if(root[strlen(root)-1] == '/') {
					if(conn->uri[0] == '/')
						sprintf(conn->request, "%s%s", root, conn->uri);
					else
						sprintf(conn->request, "%s/%s", root, conn->uri);
				} else {
					if(conn->uri[0] == '/')
						sprintf(conn->request, "%s/%s", root, conn->uri);
					else
						sprintf(conn->request, "%s/%s", root, conn->uri);
				}
			}
			if(conn->request == NULL) {
				return MG_FALSE;
			}

			char *dot = NULL;
			/* Retrieve the extension of the requested file and create a mimetype accordingly */
			dot = strrchr(conn->request, '.');
			if(dot == NULL || dot == conn->request) {
				strcpy(conn->mimetype, "text/plain");
			} else {
				if((ext = REALLOC(ext, strlen(dot)+1)) == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				memset(ext, '\0', strlen(dot)+1);
				strcpy(ext, dot+1);

				if(strcmp(ext, "html") == 0) {
					strcpy(conn->mimetype, "text/html");
				} else if(strcmp(ext, "xml") == 0) {
					strcpy(conn->mimetype, "text/xml");
				} else if(strcmp(ext, "png") == 0) {
					strcpy(conn->mimetype, "image/png");
				} else if(strcmp(ext, "gif") == 0) {
					strcpy(conn->mimetype, "image/gif");
				} else if(strcmp(ext, "ico") == 0) {
					strcpy(conn->mimetype, "image/x-icon");
				} else if(strcmp(ext, "jpg") == 0) {
					strcpy(conn->mimetype, "image/jpg");
				} else if(strcmp(ext, "css") == 0) {
					strcpy(conn->mimetype, "text/css");
				} else if(strcmp(ext, "js") == 0) {
					strcpy(conn->mimetype, "text/javascript");
				} else if(strcmp(ext, "php") == 0) {
					strcpy(conn->mimetype, "application/x-httpd-php");
				} else {
					strcpy(conn->mimetype, "text/plain");
				}
			}
			FREE(ext);

			memset(buffer, '\0', 4096);
			p = buffer;

			if(access(conn->request, F_OK) != 0) {
				goto filenotfound;
			}

			const char *cl = NULL;
			if((cl = http_get_header(conn, "Content-Length"))) {
				if(atoi(cl) > MAX_UPLOAD_FILESIZE) {
					char line[1024] = {'\0'};
					sprintf(line, "Webserver Warning: POST Content-Length of %d bytes exceeds the limit of %d bytes in Unknown on line 0", MAX_UPLOAD_FILESIZE, atoi(cl));
					webserver_create_header(&p, "200 OK", "text/plain", strlen(line));
					iobuf_append(&custom_poll_data->send_iobuf, buffer, (int)(p-buffer));
					iobuf_append(&custom_poll_data->send_iobuf, line, (int)strlen(line));

					FREE(conn->request);
					conn->request = NULL;
					return MG_TRUE;
				}
			}

			int match = 0;
			if(cache == 1) {
				struct fcache_t *tmp = fcache;
				while(tmp) {
					if(strcmp(tmp->name, conn->request) == 0) {
						match = 1;
						break;
					}
					tmp = tmp->next;
				}
				if(match == 1) {
					send_chunked_data(req, tmp->bytes, tmp->size);
					iobuf_append(&custom_poll_data->send_iobuf, "0\r\n\r\n", 5);
					return MG_TRUE;
				}
			}
			if(match == 0) {
				if((conn->file_fd = open(conn->request, O_RDONLY)) < 0) {
					logprintf(LOG_ERR, "open: %s", strerror(errno));
					goto filenotfound;
				}
#ifdef _WIN32
				unsigned long on = 1;
				ioctlsocket(conn->file_fd, FIONBIO, &on);
#else
				long arg = fcntl(conn->file_fd, F_GETFL, NULL);
				fcntl(conn->file_fd, F_SETFL, arg | O_NONBLOCK);
#endif

				if(file_read_cb(conn->file_fd, req) == 0) {
					return MG_MORE;
				} else {
					return MG_TRUE;
				}
			}

			return MG_MORE;
		}
	} else if(websockets == WEBGUI_WEBSOCKETS) {
		char *input = MALLOC(conn->content_len+1);
		if(input == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		memset(input, '\0', conn->content_len+1);

		strncpy(input, conn->content, conn->content_len);
		input[conn->content_len] = '\0';

		if(json_validate(input) == true) {
			struct reason_socket_received_t *data = MALLOC(sizeof(struct reason_socket_received_t));
			if(data == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			data->fd = conn->fd;
			if((data->buffer = MALLOC(strlen(input)+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			strcpy(data->buffer, input);
			strcpy(data->type, "websocket");

			eventpool_trigger(REASON_SOCKET_RECEIVED, reason_socket_received_free, data);
		}
		FREE(input);
		return MG_TRUE;
	}

filenotfound:
	logprintf(LOG_WARNING, "(webserver) could not read %s", conn->request);
	create_404_header(conn->uri, &p);
	FREE(conn->request);

	iobuf_append(&custom_poll_data->send_iobuf, buffer, (int)(p-buffer));

	return MG_TRUE;
}

static void webserver_process(uv_async_t *handle) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

#ifdef _WIN32
	uv_mutex_lock(&webserver_lock);
#else
	pthread_mutex_lock(&webserver_lock);
#endif

	struct webserver_clients_t *clients = webserver_clients;
	struct broadcast_list_t *tmp = NULL;
	while(broadcast_list) {
		tmp = broadcast_list;

		while(clients) {
			if(tmp->fd > 0) {
				int fd = 0, r = 0;

				if((r = uv_fileno((uv_handle_t *)clients->req, (uv_os_fd_t *)&fd)) != 0) {
					/*LCOV_EXCL_START*/
					logprintf(LOG_ERR, "uv_fileno: %s", uv_strerror(r));
					continue;
					/*LCOV_EXCL_STOP*/
				}

				if(fd == tmp->fd) {
					websocket_write(clients->req, WEBSOCKET_OPCODE_TEXT, tmp->out, tmp->len);
				}
			} else if(clients->is_websocket == 1) {
				websocket_write(clients->req, 1, tmp->out, tmp->len);
			}
			clients = clients->next;
		}
		if(tmp->len > 0) {
			FREE(tmp->out);
		}
		broadcast_list = broadcast_list->next;
		FREE(tmp);
	}
	if(broadcast_list != NULL) {
		uv_async_send(async_req);
	}
#ifdef _WIN32
	uv_mutex_unlock(&webserver_lock);
#else
	pthread_mutex_unlock(&webserver_lock);
#endif
}

static void *broadcast(int reason, void *param) {
	if(loop == 0) {
		return NULL;
	}

#ifdef _WIN32
	uv_mutex_lock(&webserver_lock);
#else
	pthread_mutex_lock(&webserver_lock);
#endif
	struct broadcast_list_t *node = MALLOC(sizeof(struct broadcast_list_t));
	if(node == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(node, 0, sizeof(struct broadcast_list_t));
	if((node->out = MALLOC(1024)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	int i = 0;
	switch(reason) {
		case REASON_CONFIG_UPDATE: {
			struct reason_config_update_t *data = param;
			node->len += snprintf(node->out, 1024,
				"{\"origin\":\"update\",\"type\":%d,\"devices\":[",
				data->type
			);
			for(i=0;i<data->nrdev;i++) {
				node->len += snprintf(&node->out[node->len], 1024-node->len, "\"%s\",", data->devices[i]);
			}
			node->len += snprintf(&node->out[node->len-1], 1024-node->len, "],\"values\":{");
			node->len -= 1;
			for(i=0;i<data->nrval;i++) {
				if(data->values[i].type == JSON_NUMBER) {
					node->len += snprintf(&node->out[node->len], 1024-node->len,
						"\"%s\":%.*f,",
						data->values[i].name, data->values[i].decimals, data->values[i].number_
					);
				} else if(data->values[i].type == JSON_STRING) {
					node->len += snprintf(&node->out[node->len], 1024-node->len,
						"\"%s\":\"%s\",",
						data->values[i].name, data->values[i].string_
					);
				}
			}
			node->len += snprintf(&node->out[node->len-1], 1024-node->len, "}}");
			node->len -= 1;
		} break;
		case REASON_BROADCAST_CORE:
			strncpy(node->out, (char *)param, 1024);
			node->len = strlen(node->out);
		break;
		default:
			FREE(node);
		return NULL;
	}

	struct broadcast_list_t *tmp = broadcast_list;
	if(tmp != NULL) {
		while(tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = node;
		node = tmp;
	} else {
		node->next = broadcast_list;
		broadcast_list = node;
	}

#ifdef _WIN32
	uv_mutex_unlock(&webserver_lock);
#else
	pthread_mutex_unlock(&webserver_lock);
#endif
	uv_async_send(async_req);

	return NULL;
}

static char *skip(char **buf, const char *delimiters) {
  char *p, *begin_word, *end_word, *end_delimiters;

  begin_word = *buf;
  end_word = begin_word + strcspn(begin_word, delimiters);
  end_delimiters = end_word + strspn(end_word, delimiters);

  for(p = end_word; p < end_delimiters; p++) {
    *p = '\0';
  }

  *buf = end_delimiters;

  return begin_word;
}

static int is_valid_http_method(const char *s) {
  return !strcmp(s, "GET") || !strcmp(s, "POST") || !strcmp(s, "HEAD") ||
    !strcmp(s, "CONNECT") || !strcmp(s, "PUT") || !strcmp(s, "DELETE") ||
    !strcmp(s, "OPTIONS") || !strcmp(s, "PROPFIND") || !strcmp(s, "MKCOL");
}

static void remove_double_dots_and_double_slashes(char *s) {
	char *p = s;

	while(*s != '\0') {
		*p++ = *s++;
		if(s[-1] == '/' || s[-1] == '\\') {
		// Skip all following slashes, backslashes and double-dots
			while(s[0] != '\0') {
				if(s[0] == '/' || s[0] == '\\') {
					s++;
				} else if(s[0] == '.' && s[1] == '.') {
					s += 2;
				} else {
					break;
				}
			}
		}
	}
	*p = '\0';
}

static void parse_http_headers(char **buf, struct connection_t *c) {
	size_t i;
	c->num_headers = 0;
	for(i = 0; i < 1024; i++) {
		c->http_headers[i].name = skip(buf, ": ");
		c->http_headers[i].value = skip(buf, "\r\n");
		if(c->http_headers[i].name[0] == '\0')
			break;
		c->num_headers = i + 1;
	}
}

static int _urldecode(const char *src, int src_len, char *dst, int dst_len, int is_form_url_encoded) {
	int i, j, a, b;

#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

	for(i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
		if(src[i] == '%' && i < src_len - 2 &&
			isxdigit(*(const unsigned char *)(src + i + 1)) &&
			isxdigit(*(const unsigned char *)(src + i + 2))) {
			a = tolower(*(const unsigned char *)(src + i + 1));
			b = tolower(*(const unsigned char *)(src + i + 2));
			dst[j] = (char)((HEXTOI(a) << 4) | HEXTOI(b));
			i += 2;
		} else if(is_form_url_encoded && src[i] == '+') {
			dst[j] = ' ';
		} else {
			dst[j] = src[i];
		}
	}

	dst[j] = '\0'; // Null-terminate the destination

	return i >= src_len ? j : -1;
}

int http_parse_request(char *buffer, struct connection_t *c) {
	int is_request = 0, n = 0;

	while(*buffer != '\0' && isspace(*(unsigned char *)buffer)) {
		buffer++;
	}

	c->request_method = skip(&buffer, " ");
	c->uri = skip(&buffer, " ");
	c->http_version = skip(&buffer, "\r\n");

	is_request = is_valid_http_method(c->request_method);
	if((is_request && memcmp(c->http_version, "HTTP/", 5) != 0) ||
		(!is_request && memcmp(c->request_method, "HTTP/", 5) != 0)) {
	} else {
		if(is_request == 1) {
			c->http_version += 5;
		} else {
			c->status_code = atoi(c->uri);
		}
		parse_http_headers(&buffer, c);

		if((c->query_string = strchr(c->uri, '?')) != NULL) {
			*(char *)c->query_string++ = '\0';
		}
		n = (int)strlen(c->uri);
		_urldecode(c->uri, n, (char *)c->uri, n + 1, 0);

		if(*c->uri == '/' || *c->uri == '.') {
			remove_double_dots_and_double_slashes((char *)c->uri);
		}
	}

	return 0;
}

static void webserver_client_add(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

#ifdef _WIN32
	uv_mutex_lock(&webserver_lock);
#else
	pthread_mutex_lock(&webserver_lock);
#endif
	struct webserver_clients_t *node = MALLOC(sizeof(struct webserver_clients_t));
	if(node == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	node->req = req;
	node->is_websocket = 0;

	node->next = webserver_clients;
	webserver_clients = node;
#ifdef _WIN32
	uv_mutex_unlock(&webserver_lock);
#else
	pthread_mutex_unlock(&webserver_lock);
#endif
}

static void webserver_client_remove(uv_poll_t *req) {
#ifdef _WIN32
	uv_mutex_lock(&webserver_lock);
#else
	pthread_mutex_lock(&webserver_lock);
#endif
	struct webserver_clients_t *currP, *prevP;

	prevP = NULL;

	for(currP = webserver_clients; currP != NULL; prevP = currP, currP = currP->next) {
		if(currP->req == req) {
			if(prevP == NULL) {
				webserver_clients = currP->next;
			} else {
				prevP->next = currP->next;
			}

			FREE(currP);
			break;
		}
	}
#ifdef _WIN32
	uv_mutex_unlock(&webserver_lock);
#else
	pthread_mutex_unlock(&webserver_lock);
#endif
}

static void send_websocket_handshake(uv_poll_t *req, const char *key) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	static const char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char sha[20];
	char buf[500], *b64_sha = NULL, *p = (char *)sha;
	mbedtls_sha1_context ctx;

	memset(&sha, '\0', 20);

	snprintf(buf, sizeof(buf), "%s%s", key, magic);
	mbedtls_sha1_init(&ctx);
	mbedtls_sha1_starts(&ctx);
	mbedtls_sha1_update(&ctx, (unsigned char *)buf, strlen(buf));
	mbedtls_sha1_finish(&ctx, sha);
	b64_sha = base64encode(p, 20);
	int i = sprintf(buf,
              "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
              "Connection: Upgrade\r\n"
              "Upgrade: websocket\r\n"
              "Sec-WebSocket-Accept: %s\r\n\r\n", b64_sha);

	iobuf_append(&custom_poll_data->send_iobuf, buf, i);
	uv_custom_write(req);
	mbedtls_sha1_free(&ctx);
	FREE(b64_sha);
}

static void send_websocket_handshake_if_requested(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;

	const char *ver = http_get_header(conn, "Sec-WebSocket-Version");
	const char *key = http_get_header(conn, "Sec-WebSocket-Key");
	if(ver != NULL && key != NULL) {
		conn->is_websocket = 1;

		struct webserver_clients_t *tmp = webserver_clients;
		while(tmp) {
			if(tmp->req == req) {
				tmp->is_websocket = 1;
				break;
			}
			tmp = tmp->next;
		}
		send_websocket_handshake(req, key);
	}
}

int websocket_read(uv_poll_t *req, unsigned char *buf, ssize_t buf_len) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;

	unsigned int i = 0, j = 0;
	unsigned char mask[4];
	unsigned int packet_length = 0;
	unsigned int length_code = 0;
	int index_first_mask = 0;
	int index_first_data_byte = 0;
	int opcode = buf[0] & 0xF;

	memset(&mask, '\0', 4);
	length_code = ((unsigned char)buf[1]) & 0x7F;
	index_first_mask = 2;
	if(length_code == 126) {
		index_first_mask = 4;
	} else if(length_code == 127) {
		index_first_mask = 10;
	}
	memcpy(mask, &buf[index_first_mask], 4);
	index_first_data_byte = index_first_mask + 4;
	packet_length = buf_len - index_first_data_byte;
	for(i = index_first_data_byte, j = 0; i < buf_len; i++, j++) {
		buf[j] = buf[i] ^ mask[j % 4];
	}
	buf[packet_length] = '\0';

	switch(opcode) {
		case WEBSOCKET_OPCODE_PONG: {
			struct timeval tv;
			gettimeofday(&tv, NULL);
			conn->ping = 1000000 * (unsigned int)tv.tv_sec + (unsigned int)tv.tv_usec;
		} break;
		case WEBSOCKET_OPCODE_PING: {
			websocket_write(req, WEBSOCKET_OPCODE_PONG, NULL, 0);
		} break;
		case WEBSOCKET_OPCODE_CONNECTION_CLOSE:
			websocket_write(req, WEBSOCKET_OPCODE_CONNECTION_CLOSE, NULL, 0);
			return -1;
		break;
		case WEBSOCKET_OPCODE_TEXT:
			conn->content_len = packet_length;
			conn->content = (char *)buf;
			return 0;
		break;
	}

	return packet_length;
}

// static void *adhoc_mode(int reason, void *param) {
	// if(https_server != NULL) {
		// eventpool_fd_remove(https_server);
		// logprintf(LOG_INFO, "shut down secure webserver due to adhoc mode");
		// https_server = NULL;
	// }
	// if(http_server != NULL) {
		// eventpool_fd_remove(http_server);
		// logprintf(LOG_INFO, "shut down regular webserver due to adhoc mode");
		// http_server = NULL;
	// }

	// return NULL;
// }

// void *webserver_restart(int reason, void *param) {
	// if(https_server != NULL) {
		// eventpool_fd_remove(https_server);
		// https_server = NULL;
	// }
	// if(http_server != NULL) {
		// eventpool_fd_remove(http_server);
		// http_server = NULL;
	// }

// #ifdef WEBSERVER_HTTPS
	// if(https_port > 0) {
		// if(ssl_server_init_status() == 0) {
			// https_server = eventpool_socket_add("webserver https", NULL, https_port, AF_INET, SOCK_STREAM, 0, EVENTPOOL_TYPE_SOCKET_SERVER, server_callback, client_send, NULL);
		// } else {
			// logprintf(LOG_NOTICE, "could not initialize mbedtls library, secure webserver not started");
		// }
	// }
// #endif

	// if(http_port > 0) {
		// http_server = eventpool_socket_add("webserver http", NULL, http_port, AF_INET, SOCK_STREAM, 0, EVENTPOOL_TYPE_SOCKET_SERVER, server_callback, client_send, NULL);
	// }

	// return NULL;
// }

static void poll_close_cb(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *conn = custom_poll_data->data;
	int fd = -1, r = 0;

	if((r = uv_fileno((uv_handle_t *)req, (uv_os_fd_t *)&fd)) != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_fileno: %s", uv_strerror(r));
		/*LCOV_EXCL_STOP*/
	}

	if(conn != NULL && conn->is_websocket == 1) {
		websocket_write(req, WEBSOCKET_OPCODE_CONNECTION_CLOSE, NULL, 0);
	}

	if(fd > -1) {
#ifdef _WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		shutdown(fd, SHUT_RDWR);
		close(fd);
#endif
	}

	if(conn != NULL) {
		if(conn->fd > 0) {
#ifdef _WIN32
			closesocket(conn->fd);
#else
			close(conn->fd);
#endif
			conn->fd = -1;
		}
		if(conn->request != NULL) {
			FREE(conn->request);
		}
	}

	webserver_client_remove(req);

	if(!uv_is_closing((uv_handle_t *)req)) {
		uv_close((uv_handle_t *)req, close_cb);
	}

	if(custom_poll_data->data != NULL) {
		FREE(custom_poll_data->data);
	}

	if(req->data != NULL) {
		uv_custom_poll_free(custom_poll_data);
		req->data = NULL;
	}

}

static void client_write_cb(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *c = (struct connection_t *)custom_poll_data->data;

	if(c->file_fd >= 0) {
		if(file_read_cb(c->file_fd, req) != 0) {
			uv_custom_close(req);
		}
	}
}

static void client_read_cb(uv_poll_t *req, ssize_t *nread, char *buf) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct connection_t *c = (struct connection_t *)custom_poll_data->data;
	int is_websocket = 0;

	if(c != NULL && c->is_websocket == 1) {
		is_websocket = 1;
	}

	if(*nread > 0 && is_websocket == 0) {
		buf[*nread-1] = '\0';
	}

	if(*nread > 0) {
		if(c->is_websocket == 0) {
			http_parse_request(buf, c);
			send_websocket_handshake_if_requested(req);

			if(auth_handler(req) == MG_FALSE) {
				send_auth_request(req);
				uv_custom_close(req);
				return;
			}
		} else {
			unsigned char *p = (unsigned char *)buf;
			if(websocket_read(req, p, *nread) == -1) {
				uv_custom_close(req);
				return;
			}
		}
		int x = request_handler(req);
		if(x == MG_TRUE) {
			if(c->is_websocket == 1) {
				uv_custom_read(req);
				*nread = 0;
				return;
			}

			uv_custom_close(req);
			return;
		}
	}
}

static void server_read_cb(uv_poll_t *req, ssize_t *nread, char *buf) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct sockaddr_in servaddr;
	struct uv_custom_poll_t *server_poll_data = req->data;
	struct uv_custom_poll_t *custom_poll_data = NULL;
	socklen_t socklen = sizeof(servaddr);
	char buffer[BUFFER_SIZE];
	int client = 0, r = 0, fd = 0;

	memset(buffer, '\0', BUFFER_SIZE);

	if((r = uv_fileno((uv_handle_t *)req, (uv_os_fd_t *)&fd)) != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_fileno: %s", uv_strerror(r));
		return;
		/*LCOV_EXCL_STOP*/
	}

	if((client = accept(fd, (struct sockaddr *)&servaddr, (socklen_t *)&socklen)) < 0) {
		logprintf(LOG_NOTICE, "accept: %s", strerror(errno));
		return;
	}

	memset(&buffer, '\0', INET_ADDRSTRLEN+1);
	uv_inet_ntop(AF_INET, (void *)&(servaddr.sin_addr), buffer, INET_ADDRSTRLEN+1);

	if(whitelist_check(buffer) != 0) {
		logprintf(LOG_INFO, "rejected client, ip: %s, port: %d", buffer, ntohs(servaddr.sin_port));
#ifdef _WIN32
		closesocket(client);
#else
		close(client);
#endif
		return;
	} else {
		logprintf(LOG_DEBUG, "new client, ip: %s, port: %d", buffer, ntohs(servaddr.sin_port));
		logprintf(LOG_DEBUG, "client fd: %d", client);
	}

	struct reason_webserver_connected_t *data = MALLOC(sizeof(struct reason_webserver_connected_t));
	if(data == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	data->fd = client;

	eventpool_trigger(REASON_WEBSERVER_CONNECTED, reason_webserver_connected_free, data);

#ifdef _WIN32
	unsigned long on = 1;
	ioctlsocket(client, FIONBIO, &on);
#else
	long arg = fcntl(client, F_GETFL, NULL);
	fcntl(client, F_SETFL, arg | O_NONBLOCK);
#endif

	uv_poll_t *poll_req = NULL;
	if((poll_req = MALLOC(sizeof(uv_poll_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	uv_custom_poll_init(&custom_poll_data, poll_req, NULL);
	custom_poll_data->read_cb = client_read_cb;
	custom_poll_data->write_cb = client_write_cb;
	custom_poll_data->close_cb = poll_close_cb;

	r = uv_poll_init_socket(uv_default_loop(), poll_req, client);
	if(r != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_poll_init_socket: %s", uv_strerror(r));
#ifdef _WIN32
		closesocket(fd);
#else
		close(fd);
#endif
		if(custom_poll_data != NULL) {
			uv_custom_poll_free(custom_poll_data);
			poll_req->data = NULL;
		}
		FREE(poll_req);
		return;
		/*LCOV_EXCL_STOP*/
	}

	struct connection_t *c = MALLOC(sizeof(struct connection_t));
	if(c == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(c, 0, sizeof(struct connection_t));

	custom_poll_data->data = (void *)c;
	custom_poll_data->is_server = 0;
	c->request = NULL;
	c->is_websocket = 0;
	c->fd = client;
	c->flags = 0;
	c->ping = 0;
	c->file_fd = -1;
#ifdef WEBSERVER_HTTPS
	c->is_ssl = custom_poll_data->is_ssl = server_poll_data->is_ssl;
	custom_poll_data->is_server = 1;
#endif

	webserver_client_add(poll_req);
	uv_custom_read(req);
	uv_custom_read(poll_req);
}

static void server_write_cb(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct sockaddr_in servaddr;
	socklen_t socklen = sizeof(servaddr);
	int r = 0, fd = 0;

	if((r = uv_fileno((uv_handle_t *)req, (uv_os_fd_t *)&fd)) != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_fileno: %s", uv_strerror(r));
		return;
		/*LCOV_EXCL_STOP*/
	}

	static struct linger linger = { 0, 0 };
#ifdef _WIN32
	unsigned int lsize = sizeof(struct linger);
#else
	socklen_t lsize = sizeof(struct linger);
#endif
	setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);

	if(getsockname(fd, (struct sockaddr *)&servaddr, (socklen_t *)&socklen) == -1) {
		logprintf(LOG_ERR, "getsockname"); /*LCOV_EXCL_LINE*/
	} else {
#ifdef WEBSERVER_HTTPS
		if(ntohs(servaddr.sin_port) == https_port) {
			logprintf(LOG_INFO, "secured webserver started on port: %d (fd %d)", https_port, fd);
		} else {
#endif
			logprintf(LOG_INFO, "regular webserver started on port: %d (fd %d)", ntohs(servaddr.sin_port), fd);
#ifdef WEBSERVER_HTTPS
		}
#endif
	}

	uv_custom_read(req);
}

static int webserver_init(int port, int is_ssl) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct sockaddr_in addr;
	struct uv_custom_poll_t *custom_poll_data = NULL;
	int r = 0, sockfd = 0, socklen = sizeof(addr);

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "WSAStartup");
		exit(EXIT_FAILURE);
	}
#endif

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "socket: %s", strerror(errno));
		return -1;
		/*LCOV_EXCL_STOP*/
	}

	unsigned long on = 1;

	if((r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int))) < 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "setsockopt: %s", strerror(errno));
#ifdef _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
		return -1;
		/*LCOV_EXCL_STOP*/
	}

	memset((char *)&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if((r = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
		int x = 0;
		if((x = getpeername(sockfd, (struct sockaddr *)&addr, (socklen_t *)&socklen)) == 0) {
			logprintf(LOG_ERR, "cannot bind to webserver port %d, address already in use?", ntohs(addr.sin_port));
		} else {
			logprintf(LOG_ERR, "cannot bind to webserver port, address already in use?");
		}
#ifdef _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
		return -1;
	}

	if((listen(sockfd, 0)) < 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "listen: %s", strerror(errno));
#ifdef _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
		return -1;
		/*LCOV_EXCL_STOP*/
	}

	uv_poll_t *poll_req = NULL;

	if(is_ssl == 1) {
		if((poll_https_req = MALLOC(sizeof(uv_poll_t))) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		poll_req = poll_https_req;
	}	else {
		if((poll_http_req = MALLOC(sizeof(uv_poll_t))) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		poll_req = poll_http_req;
	}
	uv_custom_poll_init(&custom_poll_data, poll_req, NULL);
	custom_poll_data->is_ssl = is_ssl;
	custom_poll_data->is_server = 1;

	custom_poll_data->write_cb = server_write_cb;
	custom_poll_data->read_cb = server_read_cb;

	r = uv_poll_init_socket(uv_default_loop(), poll_req, sockfd);
	if(r != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_poll_init_socket: %s", uv_strerror(r));
#ifdef _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
		if(custom_poll_data != NULL) {
			uv_custom_poll_free(custom_poll_data);
			poll_req->data = NULL;
		};
		FREE(poll_req);
		return -1;
		/*LCOV_EXCL_STOP*/
	}

	server_write_cb(poll_req);

	return 0;
}

int webserver_start(void) {
	const uv_thread_t pth_cur_id = uv_thread_self();
	if(uv_thread_equal(&pth_main_id, &pth_cur_id) == 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "webserver_start can only be started from the main thread");
		return -1;
		/*LCOV_EXCL_STOP*/
	}

	if((async_req = MALLOC(sizeof(uv_async_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	uv_async_init(uv_default_loop(), async_req, webserver_process);

	// double itmp = 0.0;
	// int webserver_enabled = 0;
	loop = 1;

	if(lock_init == 0) {
		lock_init = 1;
#ifdef _WIN32
		uv_mutex_init(&webserver_lock);
#else
		pthread_mutexattr_init(&webserver_attr);
		pthread_mutexattr_settype(&webserver_attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&webserver_lock, &webserver_attr);
#endif
	}

#ifdef PILIGHT_REWRITE	
	if(settings_select_number(ORIGIN_WEBSERVER, "webserver-http-port", &itmp) == 0) { http_port = (int)itmp; }
	if(settings_select_number(ORIGIN_WEBSERVER, "webgui-websockets", &itmp) == 0) { websockets = (int)itmp; }
	if(settings_select_number(ORIGIN_WEBSERVER, "webserver-cache", &itmp) == 0) { cache = (int)itmp; }
	if(settings_select_number(ORIGIN_WEBSERVER, "webserver-enable", &itmp) == 0) { webserver_enabled = (int)itmp; }
#ifdef WEBSERVER_HTTPS
	if(settings_select_number(ORIGIN_WEBSERVER, "webserver-https-port", &itmp) == 0) { https_port = (int)itmp; }
#endif

	if(settings_select_string(ORIGIN_WEBSERVER, "webserver-root", &root) != 0) {
		/*LCOV_EXCL_START*/
		if((root = MALLOC(strlen(WEBSERVER_ROOT)+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strcpy(root, WEBSERVER_ROOT);
		root_free = 1;
		/*LCOV_EXCL_STOP*/
	}

	settings_select_string_element(ORIGIN_WEBSERVER, "webserver-authentication", 0, &authentication_username);
	settings_select_string_element(ORIGIN_WEBSERVER, "webserver-authentication", 1, &authentication_password);
#else
	/* Check on what port the webserver needs to run */
	settings_find_number("webserver-http-port", &http_port);

#ifdef WEBSERVER_HTTPS
	settings_find_number("webserver-https-port", &https_port);
#endif

	if(settings_find_string("webserver-root", &root) != 0) {
		/* If no webserver port was set, use the default webserver port */
		if((root = MALLOC(strlen(WEBSERVER_ROOT)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(root, WEBSERVER_ROOT);
		root_free = 1;
	}
	settings_find_number("webgui-websockets", &websockets);

	/* Do we turn on webserver caching. This means that all requested files are
	   loaded into the memory so they aren't read from the FS anymore */
	settings_find_number("webserver-cache", &cache);
	settings_find_string("webserver-authentication-password", &authentication_password);
	settings_find_string("webserver-authentication-username", &authentication_username);

#endif

	eventpool_callback(REASON_CONFIG_UPDATE, broadcast);
	eventpool_callback(REASON_BROADCAST_CORE, broadcast);
	// eventpool_callback(REASON_ADHOC_CONNECTED, adhoc_mode);
	// eventpool_callback(REASON_ADHOC_DISCONNECTED, webserver_restart);
	eventpool_callback(REASON_SOCKET_SEND, webserver_send);

#ifdef WEBSERVER_HTTPS
	if(https_port > 0 /*&& webserver_enabled == 1*/) {
		if(ssl_server_init_status() == 0) {
			webserver_init(https_port, 1);
		} else {
			logprintf(LOG_NOTICE, "could not initialize mbedtls library, secure webserver not started");
		}
	}
#endif

	if(http_port > 0 /*&& webserver_enabled == 1*/) {
		webserver_init(http_port, 0);
	}

	return 0;
}
