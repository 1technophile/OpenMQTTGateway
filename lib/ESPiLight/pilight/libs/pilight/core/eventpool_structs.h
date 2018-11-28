/*
	Copyright (C) 2015 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _EVENTPOOL_STRUCTS_H_
#define _EVENTPOOL_STRUCTS_H_

#include "defines.h"
#include "eventpool_structs.h"

typedef struct reason_log_t {
	char *buffer;
} reason_log_t;

typedef struct reason_ssdp_received_t {
	char name[17];
	char uuid[UUID_LENGTH+1];
	char ip[INET_ADDRSTRLEN+1];
	int port;
} reason_ssdp_received_t;

typedef struct reason_received_pulsetrain_t {
	int length;
	int pulses[MAXPULSESTREAMLENGTH+1];
	char *hardware;
} reason_received_pulsetrain_t;

typedef struct reason_code_received_t {
	char message[1025];
	char origin[256];
	char *protocol;
	char *uuid;

	int repeat;
} reason_code_received_t;

typedef struct reason_code_sent_t {
	char message[1025];
	char protocol[256];
	char origin[256];
	char uuid[UUID_LENGTH+1];

	int type;
	int repeat;
} reason_code_sent_t;

typedef struct reason_code_sent_fail_t {
	char message[1025];
	char uuid[UUID_LENGTH+1];
} reason_code_sent_failed_t;

typedef struct reason_code_sent_success_t {
	char message[1025];
	char uuid[UUID_LENGTH+1];
} reason_code_sent_success_t;

typedef struct reason_socket_disconnected_t {
	int fd;
} reason_socket_disconnected_t;

typedef struct reason_socket_connected_t {
	int fd;
} reason_socket_connected_t;

typedef struct reason_ssdp_disconnected_t {
	int fd;
} reason_ssdp_disconnected_t;

typedef struct reason_webserver_connected_t {
	int fd;
} reason_webserver_connected_t;

typedef struct reason_socket_received_t {
	int fd;
	char *buffer;
	char type[256];
	int endpoint;
} reason_socket_received_t;

typedef struct reason_socket_send_t {
	int fd;
	char *buffer;
	char type[256];
} reason_socket_send_t;

typedef struct reason_send_code_t {
	int origin;
	char message[1025];
	int rawlen;
	int txrpt;
	int pulses[MAXPULSESTREAMLENGTH+1];
	char protocol[256];
	int hwtype;
	char uuid[UUID_LENGTH+1];
} reason_send_code_t;

typedef struct reason_config_update_t {
	char origin[256];
	int type;
	time_t timestamp;
	int nrdev;
	char devices[256][256];
	int nrval;
	struct {
		char name[256];
		union {
			char string_[256];
			double number_;
		};
		int decimals;
		int type;
	} values[256];
	char *uuid;
} reason_config_update_t;

#define reason_config_updated_t reason_config_update_t

typedef struct reason_control_device_t {
	char *dev;
	char *state;
	struct JsonNode *values;
} reason_control_device_t;

typedef struct reason_arp_device_t {
	char mac[18];
	char ip[INET_ADDRSTRLEN];
} reason_arp_device_t;

#endif
