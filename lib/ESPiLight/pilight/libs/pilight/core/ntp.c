/*
	Copyright (C) 2014 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <assert.h>
#ifdef _WIN32
	#if _WIN32_WINNT < 0x0501
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define MSG_NOSIGNAL 0
#else
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif
#include <stdint.h>
#include <time.h>
#include <signal.h>

#include "../../libuv/uv.h"
#include "network.h"
#include "ntp.h"
#include "socket.h"
#include "pilight.h"
#include "eventpool.h"
#include "common.h"
#include "log.h"
#ifdef PILIGHT_REWRITE
	#include "../storage/storage.h"
#else
	#include "../config/settings.h"
#endif

typedef struct l_fp {
	union {
		unsigned int Xl_ui;
		int Xl_i;
	} Ul_i;
	union {
		unsigned int Xl_uf;
		int Xl_f;
	} Ul_f;
} l_fp;

typedef struct pkt {
	int	li_vn_mode;
	int rootdelay;
	int rootdispersion;
	int refid;
	struct l_fp ref;
	struct l_fp org;
	struct l_fp rec;
	/* Make sure the pkg is 48 bits */
	double tmp;
} pkt;

typedef struct data_t {
	void (*callback)(int, time_t);
	uv_udp_t *stream;
	uv_timer_t *timer;
} data_t;

static int nr = 0;
static int started = 0;
static int synced = 1;
static int init = 0;
static int diff = 0;
static uv_timer_t *timer_req = NULL;

static void close_cb(uv_handle_t *handle) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	FREE(handle);
}

static void ntp_timeout(uv_timer_t *param) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct data_t *data = param->data;
	void (*callback)(int, time_t) = data->callback;
	if(callback != NULL) {
		callback(-1, 0);
	}

	if(!uv_is_closing((uv_handle_t *)param)) {
		uv_close((uv_handle_t *)param, close_cb);
	}
	if(!uv_is_closing((uv_handle_t *)data->stream)) {
		uv_close((uv_handle_t *)data->stream, close_cb);
	}
	if(data != NULL) {
		FREE(data);
	}

	logprintf(LOG_INFO, "could not sync with ntp server: %s", ntp_servers.server[nr].host);
}

static void restart(uv_timer_t *req) {
	nr = 0;
	ntpsync();
}

static void on_read(uv_udp_t *stream, ssize_t len, const uv_buf_t *buf, const struct sockaddr *addr, unsigned int port) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct pkt msg;
	struct data_t *data = stream->data;

	if(len == 48) {
		memcpy(&msg, buf->base, 48);

		if(msg.refid != 0) {
			(msg.rec).Ul_i.Xl_ui = ntohl((msg.rec).Ul_i.Xl_ui);
			(msg.rec).Ul_f.Xl_f = (int)ntohl((unsigned int)(msg.rec).Ul_f.Xl_f);

			unsigned int adj = 2208988800u;
			unsigned long ntptime = (time_t)(msg.rec.Ul_i.Xl_ui - adj);
			diff = (int)(time(NULL) - ntptime);

			synced = 0;
			if(data->callback != NULL) {
				data->callback(0, (time_t)(msg.rec.Ul_i.Xl_ui - adj));
			}
			logprintf(LOG_INFO, "time offset found of %d seconds", diff);
			uv_timer_start(timer_req, restart, 86400*1000, 0);

			if(!uv_is_closing((uv_handle_t *)stream)) {
				uv_close((uv_handle_t *)stream, close_cb);
			}
			if(!uv_is_closing((uv_handle_t *)data->timer)) {
				uv_close((uv_handle_t *)data->timer, close_cb);
			}
			if(data != NULL) {
				FREE(data);
			}
		}
		free(buf->base);
		return;
	}
}

static void alloc(uv_handle_t *handle, size_t len, uv_buf_t *buf) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	buf->len = len;
	buf->base = malloc(len);
	memset(buf->base, 0, len);
}

static void on_send(uv_udp_send_t *req, int status) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	FREE(req);
}

static void loop(uv_timer_t *req) {
	/*
	 * Make sure we are called from the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	if(uv_thread_equal(&pth_main_id, &pth_cur_id) == 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "ntpsync can only be started from the main thread");
		return;
		/*LCOV_EXCL_STOP*/
	}

	if(init == 0) {
		if((timer_req = MALLOC(sizeof(uv_timer_t))) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		uv_timer_init(uv_default_loop(), timer_req);
		init = 1;
	}

	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
	struct data_t *data = NULL;
	uv_udp_t *client_req = NULL;
	uv_udp_send_t *send_req = NULL;
	uv_timer_t *timeout_req = NULL;
	char *ip = NULL;
	char buffer[BUFFER_SIZE];
	int r = 0, type = 0;

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "WSAStartup");
		exit(EXIT_FAILURE);
	}
#endif

	type = host2ip(ntp_servers.server[nr].host, &ip);
	switch(type) {
		case AF_INET: {
			memset(&addr4, '\0', sizeof(struct sockaddr_in));
			r = uv_ip4_addr(ip, ntp_servers.server[nr].port, &addr4);
			if(r != 0) {
				/*LCOV_EXCL_START*/
				logprintf(LOG_ERR, "uv_ip4_addr: %s", uv_strerror(r));
				FREE(ip);
				return;
				/*LCOV_EXCL_END*/
			}
		} break;
		case AF_INET6: {
			memset(&addr6, '\0', sizeof(struct sockaddr_in6));
			r = uv_ip6_addr(ip, ntp_servers.server[nr].port, &addr6);
			if(r != 0) {
				/*LCOV_EXCL_START*/
				logprintf(LOG_ERR, "uv_ip6_addr: %s", uv_strerror(r));
				FREE(ip);
				return;
				/*LCOV_EXCL_END*/
			}
		} break;
		default: {
			/*LCOV_EXCL_START*/
			logprintf(LOG_ERR, "host2ip");
			FREE(ip);
			/*LCOV_EXCL_END*/
		} break;
	}
	FREE(ip);

	if((client_req = MALLOC(sizeof(uv_udp_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	r = uv_udp_init(uv_default_loop(), client_req);
	if(r != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_udp_init: %s", uv_strerror(r));
		FREE(client_req);
		return;
		/*LCOV_EXCL_STOP*/
	}

	if((send_req = MALLOC(sizeof(uv_udp_send_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	if((timeout_req = MALLOC(sizeof(uv_timer_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	if((data = MALLOC(sizeof(struct data_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	memset(data, 0, sizeof(struct data_t));
	data->callback = ntp_servers.callback;
	data->stream = client_req;
	data->timer = timeout_req;

	struct pkt msg;
	memset(&msg, '\0', sizeof(struct pkt));
	msg.li_vn_mode = 227;

	memcpy(&buffer, &msg, sizeof(struct pkt));
	uv_buf_t buf = uv_buf_init(buffer, sizeof(struct pkt));

	client_req->data = data;

	switch(type) {
		case AF_INET: {
			r = uv_udp_send(send_req, client_req, &buf, 1, (const struct sockaddr *)&addr4, on_send);
		} break;
		case AF_INET6: {
			r = uv_udp_send(send_req, client_req, &buf, 1, (const struct sockaddr *)&addr6, on_send);
		} break;
		default: {
		} break;
	}
	if(r != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_udp_send: %s", uv_strerror(r));
		FREE(send_req);
		FREE(client_req);
		return;
		/*LCOV_EXCL_STOP*/
	}

	timeout_req->data = data;

	uv_timer_init(uv_default_loop(), timeout_req);
	uv_timer_start(timeout_req, ntp_timeout, 1000, 0);

	r = uv_udp_recv_start(client_req, alloc, on_read);
	if(r != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_udp_recv_start: %s", uv_strerror(r));
		FREE(send_req);
		FREE(client_req);
		return;
		/*LCOV_EXCL_STOP*/
	}
	logprintf(LOG_DEBUG, "syncing with ntp-server %s", ntp_servers.server[nr].host);

	if(nr+1 < ntp_servers.nrservers) {
		uv_timer_start(timer_req, loop, 1.5*1000, 0);
		nr++;
	} else {
		nr = 0;
		uv_timer_start(timer_req, loop, 10*1000, 0);
	}

	return;
}

void ntpsync(void) {
	if(started == 1) {
		return;
	}
	started = 1;
	loop(NULL);
}

void ntp_gc(void) {
	nr = 0;
	started = 0;
	synced = 1;
	init = 0;
	diff = 0;
}

int getntpdiff(void) {
	return diff;
}

int isntpsynced(void) {
	return synced;
}
