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
	#include <arpa/inet.h>
	#include <unistd.h>
#endif
#include <mbedtls/ssl.h>

#include "ssl.h"
#include "eventpool.h"
#include "../../libuv/uv.h"

#include "network.h"
#include "log.h"
#include "mail.h"

#define USERAGENT			"pilight"

#define UNSUPPORTED		0
#define AUTHPLAIN			1
#define STARTTLS			2

#define SMTP_STEP_RECV_WELCOME	0
#define SMTP_STEP_SEND_HELLO		1
#define SMTP_STEP_RECV_HELLO		2
#define SMTP_STEP_SEND_STARTTLS	3
#define SMTP_STEP_RECV_STARTTLS	4
#define SMTP_STEP_SEND_AUTH			5
#define SMTP_STEP_RECV_AUTH			6
#define SMTP_STEP_SEND_FROM			7
#define SMTP_STEP_RECV_FROM			8
#define SMTP_STEP_SEND_TO				9
#define SMTP_STEP_RECV_TO				10
#define SMTP_STEP_SEND_DATA			11
#define SMTP_STEP_RECV_DATA			12
#define SMTP_STEP_SEND_BODY			13
#define SMTP_STEP_RECV_BODY			14
#define SMTP_STEP_SEND_RESET		15
#define SMTP_STEP_RECV_RESET		16
#define SMTP_STEP_SEND_QUIT			17
#define SMTP_STEP_RECV_QUIT			18
#define SMTP_STEP_END						99

typedef struct request_t {
	const char *host;
  const char *login;
	const char *pass;
	unsigned short port;

	char *content;
	int content_len;
	int step;
	int authtype;
	int reading;
	int sending;
	int bytes_read;
	void (*callback)(int, struct mail_t *);

	struct mail_t *mail;

	mbedtls_ssl_context ssl;
} request_t;

static void close_cb(uv_handle_t *handle) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	FREE(handle);
}

static void abort_cb(uv_poll_t *req) {
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

	if(request != NULL && request->callback != NULL) {
		request->callback(-1, request->mail);
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

	uv_poll_stop(req);
	if(!uv_is_closing((uv_handle_t *)req)) {
		uv_close((uv_handle_t *)req, close_cb);
	}

	if(custom_poll_data != NULL) {
		uv_custom_poll_free(custom_poll_data);
	}

	if(request != NULL) {
		FREE(request);
	}
}

static void custom_close_cb(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	abort_cb(req);
}

static void read_cb(uv_poll_t *req, ssize_t *nread, char *buf) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct request_t *request = custom_poll_data->data;
	char buffer[BUFFER_SIZE], testme[256], ch = 0;
	int val = 0, n = 0;
	memset(&buffer, '\0', BUFFER_SIZE);

	if(*nread == -1) {
		/*
		 * We are disconnected
		 */
		abort_cb(req);
		return;
	}

	if(*nread > 0) {
		buf[*nread-1] = '\0';
	}

	switch(request->step) {
		case SMTP_STEP_RECV_WELCOME: {
			if(strncmp(buf, "220", 3) != 0) {
				uv_custom_close(req);
				return;
			}
			request->bytes_read = 0;
			*nread = 0;
			request->step = SMTP_STEP_SEND_HELLO;
			uv_custom_write(req);
		} break;
		case SMTP_STEP_RECV_HELLO: {
			char **array = NULL;
			int i = 0;
			n = explode(buf, "\r\n", &array);
			for(i=0;i<n;i++) {
				if(sscanf(array[i], "%d%c%[^\r\n]", &val, &ch, testme) == 0) {
					continue;
				}
				if(val == 250) {
					if(strncmp(testme, "AUTH", 4) == 0) {
						if(strstr(testme, "PLAIN") != NULL || strstr(testme, "plain") != NULL) {
							request->authtype = AUTHPLAIN;
						}
					}
					if(strncmp(testme, "STARTTLS", 8) == 0) {
						request->authtype = STARTTLS;
					}
				}

				if(val == 501 && ch == 32) {
					array_free(&array, n);
					uv_custom_close(req);
					logprintf(LOG_NOTICE, "SMTP: EHLO error");
					return;
				}
				if(val == 250 && ch == 32) {
					request->reading = 0;
					request->bytes_read = 0;
					if(request->authtype == UNSUPPORTED) {
						logprintf(LOG_NOTICE, "SMTP: no supported authentication method");
						array_free(&array, n);
						uv_custom_close(req);
						return;
					}
					if(request->authtype == STARTTLS) {
						request->step = SMTP_STEP_SEND_STARTTLS;
					} else {
						request->step = SMTP_STEP_SEND_AUTH;
					}
					uv_custom_write(req);
					break;
				}
			}
			array_free(&array, n);
			if(request->reading == 1) {
				uv_custom_read(req);
				return;
			}
			*nread = 0;
		} break;
		case SMTP_STEP_RECV_AUTH: {
			if(strncmp(buf, "235", 3) == 0) {
				request->step = SMTP_STEP_SEND_FROM;
			}
			if(strncmp(buf, "451", 3) == 0) {
				logprintf(LOG_NOTICE, "SMTP: protocol violation while authenticating");
				uv_custom_close(req);
				return;
			}
			if(strncmp(buf, "501", 3) == 0) {
				logprintf(LOG_NOTICE, "SMTP: improperly base64 encoded user/password");
				uv_custom_close(req);
				return;
			}
			if(strncmp(buf, "535", 3) == 0) {
				logprintf(LOG_NOTICE, "SMTP: authentication failed: wrong user/password");
				uv_custom_close(req);
				return;
			}
			*nread = 0;
			request->step = SMTP_STEP_SEND_FROM;
			uv_custom_write(req);
		} break;
		case SMTP_STEP_RECV_FROM: {
			if(strncmp(buf, "250", 3) != 0) {
				uv_custom_close(req);
				return;
			}
			request->bytes_read = 0;
			*nread = 0;
			request->step = SMTP_STEP_SEND_TO;
			uv_custom_write(req);
		} break;
		case SMTP_STEP_RECV_TO: {
			if(strncmp(buf, "250", 3) != 0) {
				uv_custom_close(req);
				return;
			}
			request->bytes_read = 0;
			*nread = 0;
			request->step = SMTP_STEP_SEND_DATA;
			uv_custom_write(req);
		} break;
		case SMTP_STEP_RECV_DATA: {
			if(strncmp(buf, "354", 3) != 0) {
				uv_custom_close(req);
				return;
			}
			request->bytes_read = 0;
			*nread = 0;
			request->step = SMTP_STEP_SEND_BODY;
			uv_custom_write(req);
		} break;
		case SMTP_STEP_RECV_BODY: {
			if(strncmp(buf, "250", 3) != 0) {
				uv_custom_close(req);
				return;
			}
			request->bytes_read = 0;
			*nread = 0;
			request->step = SMTP_STEP_SEND_RESET;
			uv_custom_write(req);
		} break;
		case SMTP_STEP_RECV_RESET: {
			if(strncmp(buf, "250", 3) != 0) {
				uv_custom_close(req);
				return;
			}
			request->bytes_read = 0;
			*nread = 0;
			request->step = SMTP_STEP_SEND_QUIT;
			uv_custom_write(req);
		} break;
		case SMTP_STEP_RECV_QUIT: {
			logprintf(LOG_INFO, "SMTP: successfully send mail");
			if(request->callback != NULL) {
				request->callback(0, request->mail);
			}
			request->callback = NULL;
			uv_custom_close(req);
			// if(!uv_is_closing((uv_handle_t *)req)) {
				// uv_close((uv_handle_t *)req, close_cb);
			// }
			// uv_custom_poll_free(custom_poll_data);
			// FREE(request);
			return;
		} break;
		case SMTP_STEP_RECV_STARTTLS: {
			if(strncmp(buf, "220", 3) == 0) {
				custom_poll_data->is_ssl = 1;

				request->bytes_read = 0;
				*nread = 0;
				request->step = SMTP_STEP_SEND_HELLO;
				uv_custom_write(req);
			}
		} break;
	}
}

static void push_data(uv_poll_t *req, int step) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct request_t *request = custom_poll_data->data;

	iobuf_append(&custom_poll_data->send_iobuf, (void *)request->content, request->content_len);

	if(request->content != NULL) {
		FREE(request->content);
	}
	request->content_len = 0;
	request->step = step;
	request->reading = 1;

	uv_custom_write(req);
	uv_custom_read(req);
}

static void write_cb(uv_poll_t *req) {
	/*
	 * Make sure we execute in the main thread
	 */
	const uv_thread_t pth_cur_id = uv_thread_self();
	assert(uv_thread_equal(&pth_main_id, &pth_cur_id));

	struct uv_custom_poll_t *custom_poll_data = req->data;
	struct request_t *request = custom_poll_data->data;

	switch(request->step) {
		case SMTP_STEP_RECV_WELCOME: {
			push_data(req, SMTP_STEP_RECV_WELCOME);
		} break;
		case SMTP_STEP_SEND_HELLO: {
			request->content_len = strlen("EHLO \r\n")+strlen(USERAGENT)+1;
			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			request->content_len = (size_t)snprintf(request->content, request->content_len, "EHLO %s\r\n", USERAGENT);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/

			push_data(req, SMTP_STEP_RECV_HELLO);
		} break;
		case SMTP_STEP_SEND_AUTH: {
			request->content_len = strlen(request->login)+strlen(request->pass)+2;
			char *authstr = MALLOC(request->content_len), *hash = NULL;
			if(authstr == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			memset(authstr, '\0', request->content_len);
			strncpy(&authstr[1], request->login, strlen(request->login));
			strncpy(&authstr[2+strlen(request->login)], request->pass, request->content_len-strlen(request->login)-2);
			hash = base64encode(authstr, request->content_len);
			FREE(authstr);

			request->content_len = strlen("AUTH PLAIN \r\n")+strlen(hash)+1;
			if((request->content = REALLOC(request->content, request->content_len+4)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			memset(request->content, '\0', request->content_len);
			request->content_len = snprintf(request->content, request->content_len, "AUTH PLAIN %s\r\n", hash);
			FREE(hash);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: AUTH PLAIN XXX\n");
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_AUTH);
		} break;
		case SMTP_STEP_SEND_FROM: {
			request->content_len = strlen("MAIL FROM: <>\r\n")+strlen(request->mail->from)+1;
			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			request->content_len = (size_t)snprintf(request->content, request->content_len, "MAIL FROM: <%s>\r\n", request->mail->from);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_FROM);
		} break;
		case SMTP_STEP_SEND_TO: {
			request->content_len = strlen("RCPT TO: <>\r\n")+strlen(request->mail->to)+1;
			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			request->content_len = (size_t)snprintf(request->content, request->content_len, "RCPT TO: <%s>\r\n", request->mail->to);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_TO);
		} break;
		case SMTP_STEP_SEND_DATA: {
			request->content_len = strlen("DATA\r\n");
			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			strncpy(request->content, "DATA\r\n", request->content_len);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_DATA);
		} break;
		case SMTP_STEP_SEND_BODY: {
			request->content_len = 255;
			request->content_len += strlen(request->mail->to);
			request->content_len += strlen(request->mail->from);
			request->content_len += strlen(request->mail->subject);
			request->content_len += strlen(request->mail->message);

			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			request->content_len = (size_t)snprintf(request->content, request->content_len,
				"Subject: %s\r\n"
				"From: <%s>\r\n"
				"To: <%s>\r\n"
				"Content-Type: text/plain\r\n"
				"Mime-Version: 1.0\r\n"
				"X-Mailer: Emoticode smtp_send\r\n"
				"Content-Transfer-Encoding: 7bit\r\n\r\n"
				"%s"
				"\r\n.\r\n",
				request->mail->subject, request->mail->from, request->mail->to, request->mail->message);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_BODY);
		} break;
		case SMTP_STEP_SEND_RESET: {
			request->content_len = strlen("RSET\r\n");
			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			strncpy(request->content, "RSET\r\n", request->content_len);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_RESET);
		} break;
		case SMTP_STEP_SEND_QUIT: {
			request->content_len = strlen("QUIT\r\n");
			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			strncpy(request->content, "QUIT\r\n", request->content_len);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_QUIT);
		} break;
		case SMTP_STEP_SEND_STARTTLS: {
			request->content_len = strlen("STARTTLS\r\n");
			if((request->content = REALLOC(request->content, request->content_len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			strncpy(request->content, "STARTTLS\r\n", request->content_len);
			/*LCOV_EXCL_START*/
			if(pilight.debuglevel >= 2) {
				fprintf(stderr, "SMTP: %s\n", request->content);
			}
			/*LCOV_EXCL_STOP*/
			push_data(req, SMTP_STEP_RECV_STARTTLS);
		} break;
	}
}

int sendmail(char *host, char *login, char *pass, unsigned short port, int is_ssl, struct mail_t *mail, void (*callback)(int, struct mail_t *)) {
	struct request_t *request = NULL;
	struct uv_custom_poll_t *custom_poll_data = NULL;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
	char *ip = NULL;
	int type = 0, sockfd = 0, r = 0;

	if(mail->from == NULL) {
		logprintf(LOG_ERR, "SMTP: sender not set");
		return -1;
	}
	if(mail->subject == NULL) {
		logprintf(LOG_ERR, "SMTP: subject not set");
		return -1;
	}
	if(mail->message == NULL) {
		logprintf(LOG_ERR, "SMTP: message not set");
		return -1;
	}
	if(mail->to == NULL) {
		logprintf(LOG_ERR, "SMTP: recipient not set");
		return -1;
	}
	if(strcmp(mail->message, ".") == 0) {
		logprintf(LOG_ERR, "SMTP: message cannot be a single .");
		return -1;
	}

	if((request = MALLOC(sizeof(struct request_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(request, 0, sizeof(struct request_t));
	request->host = host;
	request->login = login;
	request->pass = pass;
	request->port = port;
	request->mail = mail;
	request->authtype = UNSUPPORTED;
	request->callback = callback;

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "WSAStartup");
		exit(EXIT_FAILURE);
	}
#endif

	type = host2ip(host, &ip);
	switch(type) {
		case AF_INET: {
			memset(&addr4, '\0', sizeof(struct sockaddr_in));
			r = uv_ip4_addr(ip, port, &addr4);
			if(r != 0) {
				/*LCOV_EXCL_START*/
				logprintf(LOG_ERR, "uv_ip4_addr: %s", uv_strerror(r));
				goto free;
				/*LCOV_EXCL_END*/
			}
		} break;
		case AF_INET6: {
			memset(&addr6, '\0', sizeof(struct sockaddr_in6));
			r = uv_ip6_addr(ip, port, &addr6);
			if(r != 0) {
				/*LCOV_EXCL_START*/
				logprintf(LOG_ERR, "uv_ip6_addr: %s", uv_strerror(r));
				goto free;
				/*LCOV_EXCL_END*/
			}
		} break;
		default: {
			/*LCOV_EXCL_START*/
			logprintf(LOG_ERR, "host2ip");
			goto free;
			/*LCOV_EXCL_END*/
		} break;
	}
	FREE(ip);

	if((sockfd = socket(type, SOCK_STREAM, 0)) < 0){
		logprintf(LOG_ERR, "socket: %s", strerror(errno)); /*LCOV_EXCL_LINE*/
		goto free;
	}

#ifdef _WIN32
	unsigned long on = 1;
	ioctlsocket(sockfd, FIONBIO, &on);
#else
	long arg = fcntl(sockfd, F_GETFL, NULL);
	fcntl(sockfd, F_SETFL, arg | O_NONBLOCK);
#endif

	switch(type) {
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
			goto free;
			/*LCOV_EXCL_STOP*/
		}
	}

	uv_poll_t *poll_req = NULL;
	if((poll_req = MALLOC(sizeof(uv_poll_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	uv_custom_poll_init(&custom_poll_data, poll_req, (void *)request);

	custom_poll_data->is_ssl = is_ssl;

	if(custom_poll_data->is_ssl == 1 && ssl_client_init_status() == -1) {
		logprintf(LOG_ERR, "secure e-mails require a properly initialized SSL library");
		abort_cb(poll_req);
		return -1;
	}
	custom_poll_data->write_cb = write_cb;
	custom_poll_data->read_cb = read_cb;
	custom_poll_data->close_cb = custom_close_cb;

	r = uv_poll_init_socket(uv_default_loop(), poll_req, sockfd);
	if(r != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_ERR, "uv_poll_init_socket: %s", uv_strerror(r));
		FREE(poll_req);
		goto free;
		/*LCOV_EXCL_STOP*/
	}

	request->step = SMTP_STEP_RECV_WELCOME;
	uv_custom_write(poll_req);

	return 0;

free:
	if(request != NULL && request->callback != NULL) {
		request->callback(-1, request->mail);
	}
	FREE(request);
	if(custom_poll_data != NULL) {
		uv_custom_poll_free(custom_poll_data);
	}
	if(sockfd > 0) {
#ifdef _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
	}
	return -1;
}
