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
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef _WIN32
	#if _WIN32_WINNT < 0x0501
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define MSG_NOSIGNAL 0
	#ifndef EINPROGRESS
		#define EINPROGRESS WSAEINPROGRESS
	#endif
#else
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif

#include "pilight.h"
#include "network.h"
#include "log.h"
#include "gc.h"
#include "socket.h"
#include "../config/settings.h"

static char recvBuff[BUFFER_SIZE];
static char *waitMessage = NULL;
static unsigned short socket_loop = 1;
static unsigned int socket_port = 0;
static int socket_loopback = 0;
static int socket_server = 0;
static int socket_clients[MAX_CLIENTS];

int socket_gc(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int x = 0;

	socket_loop = 0;
	/* Wakeup all our select statement so the socket_wait and
		 socket_read functions can actually close and the
		 all threads using sockets can end gracefully */

	for(x=1;x<MAX_CLIENTS;x++) {
		if(socket_clients[x] > 0) {
			send(socket_clients[x], "1", 1, MSG_NOSIGNAL);
		}
	}

	if(socket_loopback > 0) {
		send(socket_loopback, "1", 1, MSG_NOSIGNAL);
		socket_close(socket_loopback);
	}

	if(waitMessage != NULL) {
		FREE(waitMessage);
	}

	logprintf(LOG_DEBUG, "garbage collected socket library");
	return EXIT_SUCCESS;
}

/* Start the socket server */
int socket_start(unsigned short port) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int opt = 1;

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "could not initialize new socket");
		exit(EXIT_FAILURE);
	}
#endif

	memset(&address, '\0', sizeof(struct sockaddr_in));
	memset(socket_clients, 0, sizeof(socket_clients));

	//create a master socket
	if((socket_server = socket(AF_INET, SOCK_STREAM, 0)) == 0)  {
		logprintf(LOG_ERR, "could not create new socket");
		exit(EXIT_FAILURE);
	}

	//set master socket to allow multiple connections , this is just a good habit, it will work without this
	if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
		logprintf(LOG_ERR, "could not set proper socket options");
		exit(EXIT_FAILURE);
	}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	if(port <= 0) {
		address.sin_port = 0;
	} else {
		address.sin_port = htons(port);
	}

	//bind the socket to localhost
	if(bind(socket_server, (struct sockaddr *)&address, sizeof(address)) < 0) {
		logprintf(LOG_ERR, "cannot bind to socket port %d, address already in use?", address);
		exit(EXIT_FAILURE);
	}

	int x = 0;
	//try to specify maximum of 3 pending connections for the master socket
	if((x = listen(socket_server, 3)) < 0) {
		logprintf(LOG_ERR, "failed to listen to socket");
		exit(EXIT_FAILURE);
	}

	static struct linger linger = { 0, 0 };
#ifdef _WIN32
	unsigned int lsize = sizeof(struct linger);
#else
 	socklen_t lsize = sizeof(struct linger);
#endif
	setsockopt(socket_server, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);

	if(getsockname(socket_server, (struct sockaddr *)&address, (socklen_t *)&addrlen) == -1)
		perror("getsockname");
	else
		socket_port = ntohs(address.sin_port);

	/* Make a loopback socket we can close when pilight needs to be stopped
	   or else the select statement will wait forever for an activity */
	char localhost[16] = "127.0.0.1";
	socket_loopback = socket_connect(localhost, (unsigned short)socket_port);
	socket_clients[0] = socket_loopback;
	logprintf(LOG_INFO, "daemon listening to port: %d", socket_port);

	return 0;
}

int socket_timeout_connect(int sockfd, struct sockaddr *serv_addr, int sec) {
	struct timeval tv;
	fd_set fdset;
	int valopt;
	socklen_t lon = 0;

#ifdef _WIN32
	unsigned long on = 1;
	unsigned long off = 0;
	ioctlsocket(sockfd, FIONBIO, &on);
#else
	long arg = fcntl(sockfd, F_GETFL, NULL);
	fcntl(sockfd, F_SETFL, arg | O_NONBLOCK);
#endif

	if(connect(sockfd, serv_addr, sizeof(struct sockaddr)) < 0) {
		if(errno == EINPROGRESS) {
			tv.tv_sec = sec;
			tv.tv_usec = 0;
			FD_ZERO(&fdset);
			FD_SET(sockfd, &fdset);
			if(select(sockfd+1, NULL, &fdset, NULL, &tv) > 0) {
				 lon = sizeof(int);
				 getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
				 if(valopt > 0) {
					return -3;
				 }
			} else {
				return -2;
			}
		} else {
			return -1;
		}
	}

#ifdef _WIN32
	ioctlsocket(sockfd, FIONBIO, &off);
#else
	arg = fcntl(sockfd, F_GETFL, NULL);
	fcntl(sockfd, F_SETFL, arg & ~O_NONBLOCK);
#endif

	return 0;
}

unsigned int socket_get_port(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	return socket_port;
}

int socket_get_fd(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	return socket_server;
}

int socket_get_clients(int i) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	return socket_clients[i];
}

int socket_connect(char *address, unsigned short port) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct sockaddr_in serv_addr;
	int sockfd;
	fd_set fdset;
	struct timeval tv;

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "could not initialize new socket");
		exit(EXIT_FAILURE);
	}
#endif

	/* Try to open a new socket */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		logprintf(LOG_ERR, "could not create socket");
		return -1;
	}

	/* Clear the server address */
	memset(&serv_addr, '\0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, address, &serv_addr.sin_addr);

	/* Connect to the server */
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != -1) {
#ifdef _WIN32
		unsigned long on = 1;
		ioctlsocket(sockfd, FIONBIO, &on);
#else
		fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

		FD_ZERO(&fdset);
		FD_SET((long unsigned int)sockfd, &fdset);
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		if(select(sockfd+1, NULL, &fdset, NULL, &tv) == 1) {
			int error = -1;
			socklen_t len = sizeof(error);

#ifdef _WIN32
			if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *)&error, &len) != SOCKET_ERROR) {
				if(error == 0) {
					return sockfd;
				} else {
					return -1;
				}
			} else {
				return -1;
			}
#else
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);

			if(error == 0) {
				return sockfd;
			} else {
				close(sockfd);
				return -1;
			}
#endif
		} else {
			close(sockfd);
			return -1;
		}
	} else {
		close(sockfd);
		return -1;
	}
}

void socket_close(int sockfd) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int i = 0;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char buf[INET_ADDRSTRLEN+1];

	if(sockfd > 0) {
		if(getpeername(sockfd, (struct sockaddr*)&address, (socklen_t*)&addrlen) == 0) {
			memset(&buf, '\0', INET_ADDRSTRLEN+1);
			inet_ntop(AF_INET, (void *)&(address.sin_addr), buf, INET_ADDRSTRLEN+1);
			logprintf(LOG_DEBUG, "client disconnected, ip %s, port %d", buf, ntohs(address.sin_port));
		}

		for(i=0;i<MAX_CLIENTS;i++) {
			if(socket_clients[i] == sockfd) {
				socket_clients[i] = 0;
				break;
			}
		}
		shutdown(sockfd, 2);
		close(sockfd);
	}
}

int socket_write(int sockfd, const char *msg, ...) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	va_list ap;
	int bytes = -1;
	int ptr = 0, n = 0, x = BUFFER_SIZE, len = (int)strlen(EOSS);
	char *sendBuff = NULL;
	if(strlen(msg) > 0 && sockfd > 0) {

		va_start(ap, msg);
#ifdef _WIN32
		n = _vscprintf(msg, ap);
#else
		n = vsnprintf(NULL, 0, msg, ap);
#endif
		if(n == -1) {
			logprintf(LOG_ERR, "improperly formatted string: %s", msg);
			return -1;
		}
		n += (int)len;
		va_end(ap);

		if((sendBuff = MALLOC((size_t)n)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		memset(sendBuff, '\0', (size_t)n);

		va_start(ap, msg);
		vsprintf(sendBuff, msg, ap);
		va_end(ap);

		memcpy(&sendBuff[n-len], EOSS, (size_t)len);

		while(ptr < n) {
			if((n-ptr) < BUFFER_SIZE) {
				x = (n-ptr);
			} else {
				x = BUFFER_SIZE;
			}
			if((bytes = (int)send(sockfd, &sendBuff[ptr], (size_t)x, MSG_NOSIGNAL)) == -1) {
				/* Change the delimiter into regular newlines */
				sendBuff[n-(len-1)] = '\0';
				sendBuff[n-(len)] = '\n';
				logprintf(LOG_DEBUG, "socket write failed: %s", sendBuff);
				FREE(sendBuff);
				return -1;
			}
			ptr += bytes;
		}

		if(strncmp(&sendBuff[0], "BEAT", 4) != 0) {
			/* Change the delimiter into regular newlines */
			sendBuff[n-(len-1)] = '\0';
			sendBuff[n-(len)] = '\n';
			logprintf(LOG_DEBUG, "socket write succeeded: %s", sendBuff);
		}
		FREE(sendBuff);
	}
	return n;
}

void socket_rm_client(int i, struct socket_callback_t *socket_callback) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int sd = socket_clients[i];
	char buf[INET_ADDRSTRLEN+1];

	//Somebody disconnected, get his details and print
	getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	memset(&buf, '\0', INET_ADDRSTRLEN+1);
	inet_ntop(AF_INET, (void *)&(address.sin_addr), buf, INET_ADDRSTRLEN+1);
	logprintf(LOG_DEBUG, "client disconnected, ip %s, port %d", buf, ntohs(address.sin_port));
	if(socket_callback->client_disconnected_callback)
		socket_callback->client_disconnected_callback(i);
	//Close the socket and mark as 0 in list for reuse
	shutdown(sd, 2);
	close(sd);
	socket_clients[i] = 0;
}

int socket_read(int sockfd, char **message, time_t timeout) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct timeval tv;
	int bytes = 0;
	size_t msglen = 0;
	int ptr = 0, n = 0, len = (int)strlen(EOSS);
	fd_set fdsread;
#ifdef _WIN32
	unsigned long on = 1;
	ioctlsocket(sockfd, FIONBIO, &on);
#else
 	fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

	if(timeout > 0) {
		tv.tv_sec = timeout;
    tv.tv_usec = 0;
	}

	while(socket_loop && sockfd > 0) {
		FD_ZERO(&fdsread);
		FD_SET((unsigned long)sockfd, &fdsread);

		do {
			if(timeout > 0) {
				n = select(sockfd+1, &fdsread, NULL, NULL, &tv);
			} else {
				n = select(sockfd+1, &fdsread, NULL, NULL, 0);
			}
		} while(n == -1 && errno == EINTR && socket_loop);
		if(timeout > 0 && n == 0) {
			return 1;
		}
		/* Immediatly stop loop if the select was waken up by the garbage collector */
		if(socket_loop == 0) {
			break;
		}
		if(n == -1) {
			return -1;
		} else if(n > 0) {
			if(FD_ISSET((unsigned long)sockfd, &fdsread)) {
				bytes = (int)recv(sockfd, recvBuff, BUFFER_SIZE, 0);

				if(bytes <= 0) {
					return -1;
				} else {
					ptr+=bytes;
					if((*message = REALLOC(*message, (size_t)ptr+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					memset(&(*message)[(ptr-bytes)], '\0', (size_t)bytes+1);
					memcpy(&(*message)[(ptr-bytes)], recvBuff, (size_t)bytes);
					msglen = strlen(*message);
				}
				if(*message && msglen > 0) {
					/* When a stream is larger then the buffer size, it has to contain
					   the pilight delimiter to know when the stream ends. If the stream
					   is shorter then the buffer size, we know we received the full stream */
					int l = 0;
					if(((l = strncmp(&(*message)[ptr-(len)], EOSS, (unsigned int)(len))) == 0) || ptr < BUFFER_SIZE) {
						/* If the socket contains buffered TCP messages, separate them by
						   changing the delimiters into newlines */
						if(ptr > msglen) {
							int i = 0;
							for(i=0;i<ptr;i++) {
								if(i+(len-1) < ptr && strncmp(&(*message)[i], EOSS, (size_t)len) == 0) {
									memmove(&(*message)[i], message[i+(len-1)], (size_t)(ptr-(i+(len-1))));
									ptr-=(len-1);
									(*message)[i] = '\n';
								}
							}
							(*message)[ptr] = '\0';
						} else {
							if(l == 0) {
								(*message)[ptr-(len)] = '\0'; // remove delimiter
							} else {
								(*message)[ptr] = '\0';
							}
							if(strcmp(*message, "1") == 0 || strcmp(*message, "BEAT") == 0) {
								return -1;
							}
						}
						return 0;
					}
				}
			}
		}
	}

	return -1;
}

void *socket_wait(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct socket_callback_t *socket_callback = (struct socket_callback_t *)param;

	char buf[INET_ADDRSTRLEN+1];
	int activity;
	int i, sd;
	int max_sd;
	struct sockaddr_in address;
	int socket_client = 0;
	int addrlen = sizeof(address);
	fd_set readfds;
#ifdef _WIN32
	unsigned long on = 1;
#endif

	while(socket_loop) {
		do {
			//clear the socket set
			FD_ZERO(&readfds);

			//add master socket to set
			FD_SET((unsigned long)socket_get_fd(), &readfds);
			max_sd = socket_get_fd();

			//add child sockets to set
			for(i=0;i<MAX_CLIENTS;i++) {
				//socket descriptor
				sd = socket_clients[i];
				//if valid socket descriptor then add to read list
				if(sd > 0)
					FD_SET((unsigned long)sd, &readfds);

				//highest file descriptor number, need it for the select function
				if(sd > max_sd)
					max_sd = sd;
			}
			//wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
			activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		} while(activity == -1 && errno == EINTR && socket_loop);

		/* Immediatly stop loop if the select was waken up by the garbage collector */
		if(socket_loop == 0) {
			break;
		}
		//If something happened on the master socket, then its an incoming connection
		if(FD_ISSET((unsigned long)socket_get_fd(), &readfds)) {
			if((socket_client = accept(socket_get_fd(), (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
				logprintf(LOG_ERR, "failed to accept client");
				exit(EXIT_FAILURE);
			}
			memset(&buf, '\0', INET_ADDRSTRLEN+1);
			inet_ntop(AF_INET, (void *)&(address.sin_addr), buf, INET_ADDRSTRLEN+1);
			if(whitelist_check(buf) != 0) {
				logprintf(LOG_INFO, "rejected client, ip: %s, port: %d", buf, ntohs(address.sin_port));
				shutdown(socket_client, 2);
				close(socket_client);
			} else {
				//inform user of socket number - used in send and receive commands
				logprintf(LOG_INFO, "new client, ip: %s, port: %d", buf, ntohs(address.sin_port));
				logprintf(LOG_DEBUG, "client fd: %d", socket_client);
				//send new connection accept message
				//socket_write(socket_client, "{\"message\":\"accept connection\"}");

				static struct linger linger = { 0, 0 };
				socklen_t lsize = sizeof(struct linger);
				setsockopt(socket_client, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);
#ifdef _WIN32
				int flags = ioctlsocket(socket_client, FIONBIO, &on);
#else
 				int flags = fcntl(socket_client, F_GETFL, 0);
#endif
 				if(flags != -1) {
#ifdef _WIN32
					ioctlsocket(socket_client, FIONBIO, &on);
#else
 					fcntl(socket_client, F_SETFL, flags | O_NONBLOCK);
#endif
				}

				//add new socket to array of sockets
				for(i=0;i<MAX_CLIENTS;i++) {
					//if position is empty
					if(socket_clients[i] == 0) {
						socket_clients[i] = socket_client;
						if(socket_callback->client_connected_callback)
							socket_callback->client_connected_callback(i);
						logprintf(LOG_DEBUG, "client id: %d", i);
						break;
					}
				}
			}
		}

		//else its some IO operation on some other socket :)
		for(i=1;i<MAX_CLIENTS;i++) {
			sd = socket_clients[i];
			if(FD_ISSET((unsigned long)socket_clients[i], &readfds)) {
				FD_CLR((unsigned long)socket_clients[i], &readfds);
				if(socket_read(sd, &waitMessage, 0) == 0) {
					if(socket_callback->client_data_callback) {
						size_t l = strlen(waitMessage);
						if(l > 0) {
							if(strstr(waitMessage, "\n") != NULL) {
								char **array = NULL;
								unsigned int n = explode(waitMessage, "\n", &array), q = 0;
								for(q=0;q<n;q++) {
									socket_callback->client_data_callback(i, array[q]);
								}
								array_free(&array, n);
							} else {
								socket_callback->client_data_callback(i, waitMessage);
							}
						}
					}
				} else {
					socket_rm_client(i, socket_callback);
					i--;
				}
			}
		}
	}

#ifdef _WIN32
	return 0;
#else
 	return NULL;
#endif
}
