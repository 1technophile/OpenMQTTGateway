/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
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
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <poll.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>

#include "../../libuv/uv.h"
#include "pilight.h"
#include "socket.h"
#include "log.h"
#include "network.h"
#include "webserver.h"
#include "ssl.h"

#define USERAGENT			"pilight"
#define HTTP_POST			1
#define HTTP_GET			0

#define STEP_WRITE					0
#define STEP_READ						1

typedef struct http_clients_t {
	uv_poll_t *req;
	int fd;
	struct uv_custom_poll_t *data;
	struct http_clients_t *next;
} http_clients_t;

#ifdef _WIN32
	static uv_mutex_t http_lock;
#else
	static pthread_mutex_t http_lock;
	static pthread_mutexattr_t http_attr;
#endif

struct http_clients_t *http_clients = NULL;
static int http_lock_init = 0;

typedef struct request_t {
	char *host;
  char *uri;
  char *query_string;
	char *auth64;
	int port;
	int is_ssl;
	int status_code;
	int reading;
	void *userdata;

	int steps;
	int request_method;

  char *content;
	char mimetype[255];
	int chunked;
	int chunksize;
	int chunkread;
	int gotheader;
  size_t content_len;
	size_t bytes_read;

	void (*callback)(int code, char *data, int size, char *type, void *userdata);
} request_t;

int done = 0;

static void http_client_close(uv_poll_t *req);

static void free_request(struct request_t *request) {
	if(request->host != NULL) {
		FREE(request->host);
	}
	if(request->uri != NULL) {
		FREE(request->uri);
	}
	if(request->content != NULL) {
		FREE(request->content);
	}
	if(request->auth64 != NULL) {
		FREE(request->auth64);
	}
	FREE(request);
}

int http_gc(void) {
	struct http_clients_t *node = NULL;

#ifdef _WIN32
	uv_mutex_lock(&http_lock);
#else
	pthread_mutex_lock(&http_lock);
#endif
	while(http_clients) {
		node = http_clients;

		if(http_clients->fd > -1) {
#ifdef _WIN32
			shutdown(http_clients->fd, SD_BOTH);
			closesocket(http_clients->fd);
#else
			shutdown(http_clients->fd, SHUT_RDWR);
			close(http_clients->fd);
#endif
		}

		struct uv_custom_poll_t *custom_poll_data = http_clients->data;
		struct request_t *request = custom_poll_data->data;
		if(request != NULL) {
			free_request(request);
		}

		if(custom_poll_data != NULL) {
			uv_custom_poll_free(custom_poll_data);
		}

		http_clients = http_clients->next;
		FREE(node);
	}

#ifdef _WIN32
	uv_mutex_unlock(&http_lock);
#else
	pthread_mutex_unlock(&http_lock);
#endif

	logprintf(LOG_DEBUG, "garbage collected http library");
	return 1;
}

static void http_client_add(uv_poll_t *req, struct uv_custom_poll_t *data) {
#ifdef _WIN32
	uv_mutex_lock(&http_lock);
#else
	pthread_mutex_lock(&http_lock);
#endif

	int fd = -1, r = 0;

	if((r = uv_fileno((uv_handle_t *)req, (uv_os_fd_t *)&fd)) != 0) {
		logprintf(LOG_ERR, "uv_fileno: %s", uv_strerror(r)); /*LCOV_EXCL_LINE*/
	}

	struct http_clients_t *node = MALLOC(sizeof(struct http_clients_t));
	if(node == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	node->req = req;
	node->data = data;
	node->fd = fd;

	node->next = http_clients;
	http_clients = node;
#ifdef _WIN32
	uv_mutex_unlock(&http_lock);
#else
	pthread_mutex_unlock(&http_lock);
#endif
}

static void http_client_remove(uv_poll_t *req) {
#ifdef _WIN32
	uv_mutex_lock(&http_lock);
#else
	pthread_mutex_lock(&http_lock);
#endif
	struct http_clients_t *currP, *prevP;

	prevP = NULL;

	for(currP = http_clients; currP != NULL; prevP = currP, currP = currP->next) {
		if(currP->req == req) {
			if(prevP == NULL) {
				http_clients = currP->next;
			} else {
				prevP->next = currP->next;
			}

			FREE(currP);
			break;
		}
	}
#ifdef _WIN32
	uv_mutex_unlock(&http_lock);
#else
	pthread_mutex_unlock(&http_lock);
#endif
}

static int prepare_request(struct request_t **request, int method, char *url, const char *contype, char *post, void (*callback)(int, char *, int, char *, void *), void *userdata) {
	char *tok = NULL, *auth = NULL;
	int plen = 0, len = 0, tlen = 0;

	if(((*request) = MALLOC(sizeof(struct request_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset((*request), 0, sizeof(struct request_t));

	/* Check which port we need to use based on the http(s) protocol */
	if(strncmp(url, "http://", 7) == 0) {
		(*request)->port = 80;
		plen = 8;
		(*request)->is_ssl = 0;
	} else if(strncmp(url, "https://", 8) == 0) {
		(*request)->port = 443;
		(*request)->is_ssl = 1;
		plen = 9;
		if(ssl_client_init_status() == -1) {
			logprintf(LOG_ERR, "HTTPS URL's require a properly initialized SSL library");
			return -1;
		}
	} else {
		logprintf(LOG_ERR, "A URL should start with either http:// or https://");
		FREE((*request));
		return -1;
	}

	/* Split the url into a host and page part */
	len = strlen(url);
	if((tok = strstr(&url[plen], "/"))) {
		tlen = (size_t)(tok-url)-plen+1;
		if(((*request)->host = MALLOC(tlen+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strncpy((*request)->host, &url[plen-1], tlen);
		(*request)->host[tlen] = '\0';
		if(((*request)->uri = MALLOC(len-tlen)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strncpy((*request)->uri, &url[tlen+(plen-1)], (len-tlen)-1);
	} else {
		tlen = strlen(url)-(plen-1);
		if(((*request)->host = MALLOC(tlen+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strncpy((*request)->host, &url[(plen-1)], tlen);
		(*request)->host[tlen] = '\0';
		if(((*request)->uri = MALLOC(2)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strncpy((*request)->uri, "/", 1);
	}
	if((tok = strstr((*request)->host, "@"))) {
		size_t pglen = strlen((*request)->uri);
		tlen = (size_t)(tok-(*request)->host);
		if((auth = MALLOC(tlen+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strncpy(auth, &(*request)->host[0], tlen);
		auth[tlen] = '\0';
		strncpy(&(*request)->host[0], &url[plen+tlen], len-(plen+tlen+pglen));
		(*request)->host[len-(plen+tlen+pglen)] = '\0';
		(*request)->auth64 = base64encode(auth, strlen(auth));
		FREE(auth);
	}
	if(method == HTTP_POST) {
		strncpy((*request)->mimetype, contype, sizeof((*request)->mimetype));
		if(((*request)->content = REALLOC((*request)->content, strlen(post)+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strcpy((*request)->content, post);
		(*request)->content_len = strlen(post);
	}

	len = strlen((*request)->host);
	int i = 0;
	while(i < len) {
		if((tok = strstr(&(*request)->host[i], ":"))) {
			size_t pglen = strlen((*request)->host);
			tlen = (size_t)(tok-(*request)->host)+1;
			if((*request)->host[tlen] == ':') {
				i = tlen;
				while(i < len && (*request)->host[i++] == ':');
				continue;
			}
			if(isNumeric(&(*request)->host[tlen]) == 0) {
				(*request)->port = atoi(&(*request)->host[tlen]);
			} else {
				logprintf(LOG_ERR, "A custom URL port must be numeric");
				FREE((*request)->host);
				FREE((*request)->uri);
				FREE((*request));
				return -1;
			}
			if(pglen == tlen) {
				logprintf(LOG_ERR, "A custom URL port is missing");
				FREE((*request)->host);
				FREE((*request)->uri);
				FREE((*request));
				return -1;
			}
			(*request)->host[tlen-1] = '\0';
			break;
		}
		i++;
	}
	len = strlen((*request)->host)-1;
	if((*request)->host[0] == '[') {
		memmove(&(*request)->host[0], &(*request)->host[1], len);
		(*request)->host[len] = '\0';
	}
	if((*request)->host[len-1] == ']') {
		(*request)->host[len-1] = '\0';
	}

	(*request)->userdata = userdata;
	(*request)->callback = callback;
	(*request)->request_method = method;

	return 0;
}

static void append_to_header(char **header, char *data, ...) {
	va_list ap, apcpy;
	int pos = 0, bytes = 0;
	if(*header != NULL) {
		pos = strlen(*header);
	}
	// va_copy(apcpy, ap);
	va_start(apcpy, data);
#ifdef _WIN32
	bytes = _vscprintf(data, apcpy);
#else
	bytes = vsnprintf(NULL, 0, data, apcpy);
#endif
	if(bytes == -1) {
		/*
		 * FIXME
		 */
		// fprintf(stderr, "ERROR: improperly formatted logprintf message %s\n", data);
	} else {
		va_end(apcpy);
		if((*header = REALLOC(*header, pos+bytes+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		va_start(ap, data);
		vsprintf(&(*header)[pos], data, ap);
		va_end(ap);
	}
}

static void process_chunk(char **buf, ssize_t *size, struct request_t *request) {
	char *p = NULL;
	int pos = 0,  toread = 0;

	while(1) {
		toread = *size;
		if(strncmp(*buf, "0\r\n\r\n", 5) == 0) {
			request->reading = 0;
			break;
		}

		if(request->chunksize == request->chunkread) {
			if((p = strstr(*buf, "\r\n")) != NULL) {
				pos = p-*buf;
				(*buf)[pos+1] = '\0';
				request->chunksize = strtoul(*buf, NULL, 16);
				memmove(&(*buf)[0], &(*buf)[pos+2], toread-(pos+2));
				toread -= (pos+2);
				*size -= (pos+2);
				(*buf)[toread] = '\0';
			}
		}

		if((request->chunksize-request->chunkread) < *size) {
			toread = request->chunksize-request->chunkread;
		}

		if((request->content = REALLOC(request->content, request->bytes_read+toread+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}

		/*
		 * Prevent uninitialised values in valgrind
		 */
		memset(&request->content[request->bytes_read], 0, toread+1);
		memcpy(&request->content[request->bytes_read], *buf, toread);
		request->bytes_read += toread;
		request->chunkread += toread;

		memmove(&(*buf)[0], &(*buf)[toread], *size-toread);

		*size -= toread;
		(*buf)[*size] = '\0';

		if(request->chunkread <= request->chunksize) {
			if(strncmp(*buf, "\r\n", 2) == 0) {
				memmove(&(*buf)[0], &(*buf)[2], *size-2);
				*size -= 2;
				toread -= 2;
				(*buf)[*size] = '\0';
			}
			if(request->chunkread == request->chunksize) {
				request->chunksize = 0;
				request->chunkread = 0;
			}
		}

		if(request->chunksize == 0 && strstr(*buf, "\r\n") == NULL) {
			break;
		} else if(*size > 0 && strncmp(*buf, "0\r\n\r\n", 5) != 0) {
			request->reading = 1;
		} else {
			break;
		}
	}
}

static void close_cb(uv_handle_t *handle) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	FREE(handle);
}

static void http_client_close(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct request_t *request = custom_poll_data->data;
	int fd = -1, r = 0;

	if((r = uv_fileno((uv_handle_t *)req, (uv_os_fd_t *)&fd)) != 0) {
		logprintf(LOG_ERR, "uv_fileno: %s", uv_strerror(r)); /*LCOV_EXCL_LINE*/
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

	http_client_remove(req);

	if(!uv_is_closing((uv_handle_t *)req)) {
		uv_poll_stop(req);
		uv_close((uv_handle_t *)req, close_cb);
	}

	if(request != NULL) {
		free_request(request);
		custom_poll_data->data = NULL;
	}

	if(custom_poll_data != NULL) {
		uv_custom_poll_free(custom_poll_data);
		req->data = NULL;
	}
}

static void poll_close_cb(uv_poll_t *req) {
	http_client_close(req);
}

static void read_cb(uv_poll_t *req, ssize_t *nread, char *buf) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct request_t *request = custom_poll_data->data;
	char *header = NULL, *p = NULL;
	const char *a = NULL, *b = NULL;
	int pos = 0;

	if(*nread > 0) {
		buf[*nread] = '\0';
	}

	/*
	 * FIXME: Use io read buf instead of new buffer
	 */
	if(*nread > 0) {
		if(request->gotheader == 0 && (p = strstr(buf, "\r\n\r\n")) != NULL) {
			pos = p-buf;
			if((header = MALLOC(pos+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			strncpy(header, buf, pos);
			header[pos] = '\0';
			request->gotheader = 1;
			*nread -= pos+4;
			memmove(buf, &buf[pos+4], *nread);
			buf[*nread] = '\0';

			struct connection_t c;
			char *location = NULL;
			const char *p = location;
			http_parse_request(header, &c);
			if(c.status_code == 301 && (p = http_get_header(&c, "Location")) != NULL) {
				if(request->callback != NULL) {
					request->callback(c.status_code, location, strlen(location), NULL, request->userdata);
				}
				FREE(header);
				return;
			}
			request->status_code = c.status_code;
			if((a = http_get_header(&c, "Content-Type")) != NULL || (b = http_get_header(&c, "Content-type")) != NULL) {
				int len = 0, i = 0;
				if(a != NULL) {
					strcpy(request->mimetype, http_get_header(&c, "Content-Type"));
				} else if(b != NULL) {
					strcpy(request->mimetype, http_get_header(&c, "Content-type"));
				}
				len = strlen(request->mimetype);
				for(i=0;i<len;i++) {
					if(request->mimetype[i] == ';') {
						request->mimetype[i] = '\0';
						break;
					}
				}
			}
			if(http_get_header(&c, "Content-Length") != NULL) {
				request->content_len = atoi(http_get_header(&c, "Content-Length"));
			}
			if(http_get_header(&c, "Transfer-Encoding") != NULL) {
				if(strcmp(http_get_header(&c, "Transfer-Encoding"), "chunked") == 0) {
					request->chunked = 1;
				}
			}
			FREE(header);
			if(*nread == 0) {
				if(request->callback != NULL) {
					request->callback(request->status_code, "", request->content_len, request->mimetype, request->userdata);
					goto close;
				}
			}
		}
		if(request->chunked == 1) {
			process_chunk(&buf, &(*nread), request);
		} else if(*nread > 0) {
			if((request->content = REALLOC(request->content, request->bytes_read+(*nread)+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			/*
			 * Prevent uninitialised values in valgrind
			 */
			memset(&request->content[request->bytes_read], 0, *nread+1);
			memcpy(&request->content[request->bytes_read], buf, *nread);
			request->bytes_read += *nread;
			*nread = 0;
		}

		if(strcmp(buf, "0\r\n\r\n") == 0) {
			request->reading = 0;
			request->chunked = 0;
			request->content[request->bytes_read] = '\0';
			request->content_len = request->bytes_read;
		}

		if(request->chunked == 1 || request->bytes_read < request->content_len) {
			request->reading = 1;
		} else if(request->content != NULL) {
			request->content[request->content_len] = '\0';
			if(request->callback != NULL) {
				request->callback(request->status_code, request->content, request->content_len, request->mimetype, request->userdata);
				goto close;
			}
		}
	}

	if(request->reading == 1) {
		uv_custom_read(req);
		return;
	}

close:
	uv_custom_close(req);
}

static void write_cb(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct request_t *request = custom_poll_data->data;
	char *header = NULL;

	switch(request->steps) {
		case STEP_WRITE: {
			if(request->request_method == HTTP_POST) {
				append_to_header(&header, "POST %s HTTP/1.0\r\n", request->uri);
				append_to_header(&header, "Host: %s\r\n", request->host);
				if(request->auth64 != NULL) {
					append_to_header(&header, "Authorization: Basic %s\r\n", request->auth64);
				}
				append_to_header(&header, "User-Agent: %s\r\n", USERAGENT);
				append_to_header(&header, "Content-Type: %s\r\n", request->mimetype);
				append_to_header(&header, "Content-Length: %lu\r\n\r\n", request->content_len);
				append_to_header(&header, "%s", request->content);
			} else if(request->request_method == HTTP_GET) {
				append_to_header(&header, "GET %s HTTP/1.1\r\n", request->uri);
				append_to_header(&header, "Host: %s\r\n", request->host);
				if(request->auth64 != NULL) {
					append_to_header(&header, "Authorization: Basic %s\r\n", request->auth64);
				}
				append_to_header(&header, "User-Agent: %s\r\n", USERAGENT);
				append_to_header(&header, "Connection: close\r\n\r\n");
			}
			iobuf_append(&custom_poll_data->send_iobuf, (void *)header, strlen(header));

			uv_custom_write(req);
			FREE(header);
			request->steps = STEP_READ;
		} break;
		case STEP_READ:
			uv_custom_read(req);
			return;
		break;
	}
}

char *http_process(int type, char *url, const char *conttype, char *post, void (*callback)(int, char *, int, char *, void *), void *userdata) {
	struct request_t *request = NULL;
	struct uv_custom_poll_t *custom_poll_data = NULL;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
	char *ip = NULL;
	int r = 0, sockfd = 0;

	if(http_lock_init == 0) {
		http_lock_init = 1;
#ifdef _WIN32
		uv_mutex_init(&http_lock);
#else
		pthread_mutexattr_init(&http_attr);
		pthread_mutexattr_settype(&http_attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&http_lock, &http_attr);
#endif
	}

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "WSAStartup");
		exit(EXIT_FAILURE);
	}
#endif

	memset(&addr4, 0, sizeof(addr4));
	memset(&addr6, 0, sizeof(addr6));
	if(prepare_request(&request, type, url, conttype, post, callback, userdata) == 0) {
		int inet = host2ip(request->host, &ip);
		switch(inet) {
			case AF_INET: {
				memset(&addr4, '\0', sizeof(struct sockaddr_in));
				r = uv_ip4_addr(ip, request->port, &addr4);
				if(r != 0) {
					/*LCOV_EXCL_START*/
					logprintf(LOG_ERR, "uv_ip4_addr: %s", uv_strerror(r));
					goto freeuv;
					/*LCOV_EXCL_END*/
				}
			} break;
			case AF_INET6: {
				memset(&addr6, '\0', sizeof(struct sockaddr_in6));
				r = uv_ip6_addr(ip, request->port, &addr6);
				if(r != 0) {
					/*LCOV_EXCL_START*/
					logprintf(LOG_ERR, "uv_ip6_addr: %s", uv_strerror(r));
					goto freeuv;
					/*LCOV_EXCL_END*/
				}
			} break;
			default: {
				/*LCOV_EXCL_START*/
				logprintf(LOG_ERR, "host2ip");
				goto freeuv;
				/*LCOV_EXCL_END*/
			} break;
		}
		FREE(ip);

		/*
		 * Partly bypass libuv in case of ssl connections
		 */
		if((sockfd = socket(inet, SOCK_STREAM, 0)) < 0){
			/*LCOV_EXCL_START*/
			logprintf(LOG_ERR, "socket: %s", strerror(errno));
			goto freeuv;
			/*LCOV_EXCL_STOP*/
		}

#ifdef _WIN32
		unsigned long on = 1;
		ioctlsocket(sockfd, FIONBIO, &on);
#else
		long arg = fcntl(sockfd, F_GETFL, NULL);
		fcntl(sockfd, F_SETFL, arg | O_NONBLOCK);
#endif

		switch(inet) {
			case AF_INET: {
				r = connect(sockfd, (struct sockaddr *)&addr4, sizeof(addr4));
			} break;
			case AF_INET6: {
				r = connect(sockfd, (struct sockaddr *)&addr6, sizeof(addr6));
			} break;
			default: {
			} break;
		}

		if(r < 0) {
#ifdef _WIN32
			if(!(WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEISCONN)) {
#else
			if(!(errno == EINPROGRESS || errno == EISCONN)) {
#endif
				/*LCOV_EXCL_START*/
				logprintf(LOG_ERR, "connect: %s", strerror(errno));
				goto freeuv;
				/*LCOV_EXCL_STOP*/
			}
		}

		uv_poll_t *poll_req = NULL;
		if((poll_req = MALLOC(sizeof(uv_poll_t))) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		uv_custom_poll_init(&custom_poll_data, poll_req, (void *)request);
		custom_poll_data->is_ssl = request->is_ssl;
		custom_poll_data->write_cb = write_cb;
		custom_poll_data->read_cb = read_cb;
		custom_poll_data->close_cb = poll_close_cb;

		r = uv_poll_init_socket(uv_default_loop(), poll_req, sockfd);
		if(r != 0) {
			/*LCOV_EXCL_START*/
			logprintf(LOG_ERR, "uv_poll_init_socket: %s", uv_strerror(r));
			FREE(poll_req);
			goto freeuv;
			/*LCOV_EXCL_STOP*/
		}

		http_client_add(poll_req, custom_poll_data);
		request->steps = STEP_WRITE;
		uv_custom_write(poll_req);		
	}

	return NULL;

freeuv:
	request->callback(404, NULL, 0, 0, request->userdata);
	FREE(request->uri);
	FREE(request->host);
	FREE(request);

	if(sockfd > 0) {
#ifdef _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
	}
	return NULL;
}

char *http_get_content(char *url, void (*callback)(int, char *, int, char *, void *), void *userdata) {
	return http_process(HTTP_GET, url, NULL, NULL, callback, userdata);
}

char *http_post_content(char *url, const char *conttype, char *post, void (*callback)(int, char *, int, char *, void *), void *userdata) {
	return http_process(HTTP_POST, url, conttype, post, callback, userdata);
}
