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
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <sys/un.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif
#include <stdint.h>
#include <math.h>

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/socket.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "lirc.h"

#ifndef _WIN32
static char socket_path[BUFFER_SIZE];
static int sockfd = -1;

static unsigned short loop = 1;
static unsigned short threads = 0;
static unsigned short initialized = 0;


// Windows
// #define UNIX_PATH_MAX 108

// struct sockaddr_un {
	// uint16_t sun_family;
	// char sun_path[UNIX_PATH_MAX];
// };

static void *thread(void *param) {
	struct protocol_threads_t *node = (struct protocol_threads_t *)param;
	struct sockaddr_un addr;
	struct timeval timeout;

	int nrloops = 0, n = -1, bytes = 0, retry = 0;
	char recvBuff[BUFFER_SIZE];
	fd_set fdsread;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	/* Clear the server address */
	memset(&addr, '\0', sizeof(addr));
	memset(&recvBuff, '\0', BUFFER_SIZE);

	threads++;

	while(loop) {
		if(sockfd > -1) {
			close(sockfd);
			sockfd = -1;
		}

		if(path_exists(socket_path) == EXIT_SUCCESS) {
			/* Try to open a new socket */
			if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
				logprintf(LOG_NOTICE, "could not create Lirc socket");
				break;
			}

			addr.sun_family=AF_UNIX;
			strcpy(addr.sun_path, socket_path);

			retry = 0;
			/* Connect to the server */
			switch(socket_timeout_connect(sockfd, (struct sockaddr *)&addr, 3)) {
				case -1:
					logprintf(LOG_NOTICE, "could not connect to Lirc socket @%s", socket_path);
					protocol_thread_wait(node, 3, &nrloops);
					retry = 1;
				break;
				case -2:
					logprintf(LOG_NOTICE, "Lirc socket timeout @%s", socket_path);
					protocol_thread_wait(node, 3, &nrloops);
					retry = 1;
				break;
				case -3:
					logprintf(LOG_NOTICE, "Error in Lirc socket connection @%s", socket_path);
					protocol_thread_wait(node, 3, &nrloops);
					retry = 1;
				break;
				default:
				break;
			}
			if(retry == 1) {
				continue;
			}

			while(loop) {
				FD_ZERO(&fdsread);
				FD_SET((unsigned long)sockfd, &fdsread);

				do {
					n = select(sockfd+1, &fdsread, NULL, NULL, &timeout);
				} while(n == -1 && errno == EINTR && loop);

				if(loop == 0) {
					break;
				}

				if(path_exists(socket_path) == EXIT_FAILURE) {
					break;
				}

				if(n == -1) {
					break;
				} else if(n == 0) {
					usleep(100000);
				} else if(n > 0) {
					if(FD_ISSET((unsigned long)sockfd, &fdsread)) {
						bytes = (int)recv(sockfd, recvBuff, BUFFER_SIZE, 0);
						if(bytes <= 0) {
							break;
						} else {
							char **nlarray = NULL;
							unsigned int e = explode(recvBuff, "\n", &nlarray), t = 0;
							for(t=0;t<e;t++) {
								int x = 0, nrspace = 0;
								for(x=0;x<strlen(nlarray[t]);x++) {
									if(nlarray[t][x] == ' ') {
										nrspace++;
									}
								}
								if(nrspace >= 3) {
									char **array = NULL;
									unsigned int q = explode(nlarray[t], " ", &array);
									if(q == 4) {
										char *code1 = array[0];
										char *rep = array[1];
										char *btn = array[2];
										char *remote = array[3];
										char *y = NULL;
										if((y = strstr(remote, "\n")) != NULL) {
											size_t pos = (size_t)(y-remote);
											remote[pos] = '\0';
										}
										int r = strtol(rep, NULL, 16);
										lirc->message = json_mkobject();
										JsonNode *code = json_mkobject();
										json_append_member(code, "code", json_mkstring(code1));
										json_append_member(code, "repeat", json_mknumber(r, 0));
										json_append_member(code, "button", json_mkstring(btn));
										json_append_member(code, "remote", json_mkstring(remote));

										json_append_member(lirc->message, "message", code);
										json_append_member(lirc->message, "origin", json_mkstring("receiver"));
										json_append_member(lirc->message, "protocol", json_mkstring(lirc->id));

										if(pilight.broadcast != NULL) {
											pilight.broadcast(lirc->id, lirc->message, PROTOCOL);
										}
										json_delete(lirc->message);
										lirc->message = NULL;
									}
									array_free(&array, q);
								}
							}
							array_free(&nlarray, t);
							memset(recvBuff, '\0', BUFFER_SIZE);
						}
					}
				}
			}
		} else {
			sleep(1);
		}
	}

	if(sockfd > -1) {
		close(sockfd);
		sockfd = -1;
	}

	threads--;
	return (void *)NULL;
}

struct threadqueue_t *initDev(JsonNode *jdevice) {
	loop = 1;

	if(initialized == 0) {
		initialized = 1;
		struct protocol_threads_t *node = protocol_thread_init(lirc, NULL);
		return threads_register("lirc", &thread, (void *)node, 0);
	} else {
		return NULL;
	}
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(lirc);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(lirc);
	initialized = 0;
}
#endif

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void lircInit(void) {
	protocol_register(&lirc);
	protocol_set_id(lirc, "lirc");
	protocol_device_add(lirc, "lirc", "Lirc API");
	lirc->devtype = LIRC;
	lirc->hwtype = API;

	options_add(&lirc->options, 'c', "code", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&lirc->options, 'a', "repeat", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "[0-9]");
	options_add(&lirc->options, 'b', "button", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&lirc->options, 'r', "remote", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, NULL);

#ifndef _WIN32
	lirc->initDev=&initDev;
	lirc->threadGC=&threadGC;

	memset(socket_path, '\0', BUFFER_SIZE);
	strcpy(socket_path, "/dev/lircd");
#endif
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "lirc";
	module->version = "1.10";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	lircInit();
}
#endif
