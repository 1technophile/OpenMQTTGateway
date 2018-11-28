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

#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include <time.h>

typedef struct socket_callback_t {
    void (*client_connected_callback)(int);
    void (*client_disconnected_callback)(int);
    void (*client_data_callback)(int, char*);
} socket_callback_t;

/* Start the socket server */
int socket_start(unsigned short port);
int socket_connect(char *address, unsigned short port);
int socket_timeout_connect(int sockfd, struct sockaddr *serv_addr, int usec);
void socket_close(int i);
int socket_write(int sockfd, const char *msg, ...);
int socket_read(int sockfd, char **out, time_t timeout);
void *socket_wait(void *param);
int socket_gc(void);
unsigned int socket_get_port(void);
int socket_get_fd(void);
int socket_get_clients(int i);

#endif
