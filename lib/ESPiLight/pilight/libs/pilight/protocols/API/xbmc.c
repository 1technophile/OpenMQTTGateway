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
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#ifdef _WIN32
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
#include <pthread.h>
#include <stdint.h>
#include <math.h>

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/network.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../../core/socket.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "xbmc.h"

typedef struct data_t {
	char *server;
	int port;
	int sockfd;
	struct data_t *next;
} data_t;

static pthread_mutex_t xbmclock;
static pthread_mutexattr_t xbmcattr;

static struct data_t *data;
static unsigned short loop = 1;
static unsigned short threads = 0;

static void createMessage(char *server, int port, char *action, char *media) {
	xbmc->message = json_mkobject();
	JsonNode *code = json_mkobject();
	json_append_member(code, "action", json_mkstring(action));
	json_append_member(code, "media", json_mkstring(media));
	json_append_member(code, "server", json_mkstring(server));
	json_append_member(code, "port", json_mknumber(port, 0));

	json_append_member(xbmc->message, "message", code);
	json_append_member(xbmc->message, "origin", json_mkstring("receiver"));
	json_append_member(xbmc->message, "protocol", json_mkstring(xbmc->id));

	if(pilight.broadcast != NULL) {
		pilight.broadcast(xbmc->id, xbmc->message, PROTOCOL);
	}
	json_delete(xbmc->message);
	xbmc->message = NULL;
}

static void *thread(void *param) {
	struct protocol_threads_t *node = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)node->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	struct sockaddr_in serv_addr;
	struct data_t *xnode = MALLOC(sizeof(struct data_t));

	char recvBuff[BUFFER_SIZE], action[10], media[15];
	char *m = NULL, *t = NULL;
	char shut[] = "shutdown";
	char home[] = "home";
	char none[] = "none";
	int nrloops = 0, bytes = 0, n = 0, has_server = 0;
	int has_port = 0, reset = 1, maxfd = 0, retry = 0;
	fd_set fdsread;
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	if(xnode == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	/* Clear the server address */
	memset(&serv_addr, '\0', sizeof(serv_addr));
	memset(&recvBuff, '\0', BUFFER_SIZE);
	memset(&action, '\0', 10);
	memset(&media, '\0', 15);

	threads++;

	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);

			while(jchild1) {
				if(strcmp(jchild1->key, "server") == 0) {
					if((xnode->server = MALLOC(strlen(jchild1->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(xnode->server, jchild1->string_);
					has_server = 1;
				}
				if(strcmp(jchild1->key, "port") == 0) {
					xnode->port = (int)round(jchild1->number_);
					has_port = 1;
				}
				jchild1 = jchild1->next;
			}
			if(has_server == 1 && has_port == 1) {
				xnode->sockfd = -1;
				xnode->next = data;
				data = xnode;
			} else {
				if(has_server == 1) {
					FREE(xnode->server);
				}
				FREE(xnode);
				xnode = NULL;
			}
			jchild = jchild->next;
		}
	}

	if(xnode == NULL) {
		return (void *)NULL;;
	}

#ifdef _WIN32
		WSADATA wsa;

		if(WSAStartup(0x202, &wsa) != 0) {
			logprintf(LOG_ERR, "could not initialize new socket");
			return (void *)NULL;
		}
#endif

	while(loop) {

		if(reset == 1) {
			createMessage(xnode->server, xnode->port, shut, none);
			reset = 0;
		}

		if(xnode->sockfd > -1) {
			close(xnode->sockfd);
			xnode->sockfd = -1;
		}

		/* Try to open a new socket */
		if((xnode->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			logprintf(LOG_NOTICE, "could not create XBMC/Kodi socket");
			break;
		}

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons((unsigned short)xnode->port);

		inet_pton(AF_INET, xnode->server, &serv_addr.sin_addr);

		retry = 0;
		/* Connect to the server */
		switch(socket_timeout_connect(xnode->sockfd, (struct sockaddr *)&serv_addr, 3)) {
			case -1:
				logprintf(LOG_NOTICE, "could not connect to XBMC/Kodi server @%s", xnode->server);
				protocol_thread_wait(node, 3, &nrloops);
				retry = 1;
			break;
			case -2:
				logprintf(LOG_NOTICE, "XBMC/Kodi connection timeout @%s", xnode->server);
				protocol_thread_wait(node, 3, &nrloops);
				retry = 1;
			break;
			case -3:
				logprintf(LOG_NOTICE, "Error in XBMC/Kodi socket connection @%s", xnode->server);
				protocol_thread_wait(node, 3, &nrloops);
				retry = 1;
			break;
			default:
				createMessage(xnode->server, xnode->port, home, none);
				reset = 1;
			break;
		}
		if(retry == 1) {
			continue;
		}

		struct data_t *xtmp = data;
		while(xtmp) {
			if(xtmp->sockfd > -1) {
				if(maxfd < xtmp->sockfd) {
					maxfd = xtmp->sockfd;
				}
			}
			xtmp = xtmp->next;
		}

		while(loop) {
			pthread_mutex_lock(&xbmclock);
			FD_ZERO(&fdsread);
			FD_SET((unsigned long)xnode->sockfd, &fdsread);

			do {
				n = select(maxfd+1, &fdsread, NULL, NULL, &timeout);
			} while(n == -1 && errno == EINTR && loop);

			if(loop == 0) {
				pthread_mutex_unlock(&xbmclock);
				break;
			}

			if(n == -1) {
				pthread_mutex_unlock(&xbmclock);
				break;
			} else if(n == 0) {
				usleep(100000);
			} else if(n > 0) {
				if(FD_ISSET((unsigned long)xnode->sockfd, &fdsread)) {
					bytes = (int)recv(xnode->sockfd, recvBuff, BUFFER_SIZE, 0);
					if(bytes <= 0) {
						pthread_mutex_unlock(&xbmclock);
						break;
					} else {
						if(json_validate(recvBuff) == true) {
							JsonNode *joutput = json_decode(recvBuff);
							JsonNode *params = NULL;
							JsonNode *data = NULL;
							JsonNode *item = NULL;

							if(json_find_string(joutput, "method", &m) == 0) {
								if(strcmp(m, "GUI.OnScreensaverActivated") == 0) {
									strcpy(media, "screensaver");
									strcpy(action, "active");
								} else if(strcmp(m, "GUI.OnScreensaverDeactivated") == 0) {
									strcpy(media, "screensaver");
									strcpy(action, "inactive");
								} else {
									if((params = json_find_member(joutput, "params")) != NULL) {
										if((data = json_find_member(params, "data")) != NULL) {
											if((item = json_find_member(data, "item")) != NULL) {
												if(json_find_string(item, "type", &t) == 0) {
													xbmc->message = json_mkobject();

													strcpy(media, t);
													if(strcmp(m, "Player.OnPlay") == 0) {
														strcpy(action, "play");
													} else if(strcmp(m, "Player.OnStop") == 0) {
														strcpy(action, home);
														strcpy(media, none);
													} else if(strcmp(m, "Player.OnPause") == 0) {
														strcpy(action, "pause");
													}
												}
											}
										}
									}
								}
								if(strlen(media) > 0 && strlen(action) > 0) {
									createMessage(xnode->server, xnode->port, action, media);
									reset = 1;
								}
							}
							json_delete(joutput);
						}
						memset(recvBuff, '\0', BUFFER_SIZE);
						memset(&action, '\0', 10);
						memset(&media, '\0', 15);
					}
				}
			}
			pthread_mutex_unlock(&xbmclock);
		}
	}
	pthread_mutex_unlock(&xbmclock);

	threads--;
	return (void *)NULL;
}

static struct threadqueue_t *initDev(JsonNode *jdevice) {
	loop = 1;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(xbmc, json);
	return threads_register("xbmc", &thread, (void *)node, 0);
}

static void threadGC(void) {
	loop = 0;
	struct data_t *xtmp = NULL;

	while(data) {
		xtmp = data;
		if(xtmp->sockfd > -1) {
			close(xtmp->sockfd);
			xtmp->sockfd = -1;
		}
		FREE(xtmp->server);
		data = data->next;
		FREE(xtmp);
	}

	protocol_thread_stop(xbmc);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(xbmc);
}

static int checkValues(JsonNode *code) {
	char *action = NULL;
	char *media = NULL;

	if(json_find_string(code, "action", &action) == 0 &&
	   json_find_string(code, "media", &media) == 0) {
		if(strcmp(media, "none") == 0) {
			if(!(strcmp(action, "shutdown") == 0 || strcmp(action, "home") == 0)) {
				return 1;
			} else {
				return 0;
			}
		} else if(strcmp(media, "episode") == 0
		   || strcmp(media, "movie") == 0
			 || strcmp(media, "movies") == 0
		   || strcmp(media, "song") == 0) {
			if(!(strcmp(action, "play") == 0 || strcmp(action, "pause") == 0)) {
				return 1;
			} else {
				return 0;
			}
		} else if(strcmp(media, "screensaver") == 0) {
			if(!(strcmp(action, "active") == 0 || strcmp(action, "inactive") == 0)) {
				return 1;
			} else {
				return 0;
			}
		} else {
			return 1;
		}
	} else {
		return 1;
	}
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void xbmcInit(void) {
	pthread_mutexattr_init(&xbmcattr);
	pthread_mutexattr_settype(&xbmcattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&xbmclock, &xbmcattr);

	protocol_register(&xbmc);
	protocol_set_id(xbmc, "xbmc");
	protocol_device_add(xbmc, "xbmc", "XBMC API");
	protocol_device_add(xbmc, "kodi", "Kodi API");
	xbmc->devtype = XBMC;
	xbmc->hwtype = API;
	xbmc->multipleId = 0;

	options_add(&xbmc->options, 'a', "action", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&xbmc->options, 'm', "media", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&xbmc->options, 's', "server", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, NULL);
	options_add(&xbmc->options, 'p', "port", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);

	options_add(&xbmc->options, 0, "show-media", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&xbmc->options, 0, "show-action", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	xbmc->initDev=&initDev;
	xbmc->threadGC=&threadGC;
	xbmc->checkValues=&checkValues;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "xbmc";
	module->version = "1.8";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	xbmcInit();
}
#endif
