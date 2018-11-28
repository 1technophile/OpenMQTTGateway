/*
	Copyright (C) 2013 - 2014 CurlyMo

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
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifndef _WIN32
#include <wiringx.h>
#endif

#include "libs/pilight/core/threads.h"
#include "libs/pilight/core/pilight.h"
#include "libs/pilight/core/common.h"
#include "libs/pilight/core/config.h"
#include "libs/pilight/core/log.h"
#include "libs/pilight/core/options.h"
#include "libs/pilight/core/socket.h"
#include "libs/pilight/core/json.h"
#include "libs/pilight/core/ssdp.h"
#include "libs/pilight/core/dso.h"
#include "libs/pilight/core/gc.h"

#include "libs/pilight/config/devices.h"

#include "libs/pilight/protocols/protocol.h"

#include "libs/pilight/events/events.h"

int main(int argc, char **argv) {
	// memtrack();

	atomicinit();
	struct options_t *options = NULL;
	struct ssdp_list_t *ssdp_list = NULL;
	struct devices_t *dev = NULL;
	struct JsonNode *json = NULL;
	struct JsonNode *tmp = NULL;
	char *recvBuff = NULL, *message = NULL, *output = NULL;
	char *device = NULL, *state = NULL, *values = NULL;
	char *server = NULL;
	int has_values = 0, sockfd = 0, hasconfarg = 0;
	unsigned short port = 0, showhelp = 0, showversion = 0;

	log_file_disable();
	log_shell_enable();
	log_level_set(LOG_NOTICE);

	if((progname = MALLOC(16)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(progname, "pilight-control");

	/* Define all CLI arguments of this program */
	options_add(&options, 'H', "help", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'V', "version", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'd', "device", OPTION_HAS_VALUE, 0,  JSON_NULL, NULL, NULL);
	options_add(&options, 's', "state", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'v', "values", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'S', "server", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
	options_add(&options, 'P', "port", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, "[0-9]{1,4}");
	options_add(&options, 'C', "config", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);

	/* Store all CLI arguments for later usage
	   and also check if the CLI arguments where
	   used correctly by the user. This will also
	   fill all necessary values in the options struct */
	while(1) {
		int c;
		c = options_parse(&options, argc, argv, 1, &optarg);
		if(c == -1)
			break;
		if(c == -2) {
			showhelp = 1;
			break;
		}
		switch(c) {
			case 'H':
				showhelp = 1;
			break;
			case 'V':
				showversion = 1;
			break;
			case 'd':
				if((device = REALLOC(device, strlen(optarg)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(device, optarg);
			break;
			case 's':
				if((state = REALLOC(state, strlen(optarg)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(state, optarg);
			break;
			case 'v':
				if((values = REALLOC(values, strlen(optarg)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(values, optarg);
			break;
			case 'C':
				if(config_set_file(optarg) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				hasconfarg = 1;
			break;
			case 'S':
				if(!(server = REALLOC(server, strlen(optarg)+1))) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(server, optarg);
			break;
			case 'P':
				port = (unsigned short)atoi(optarg);
			break;
			default:
				printf("Usage: %s -l location -d device -s state\n", progname);
				goto close;
			break;
		}
	}
	options_delete(options);

	if(showversion == 1) {
		printf("%s v%s\n", progname, PILIGHT_VERSION);
		goto close;
	}
	if(showhelp == 1) {
		printf("\t -H --help\t\t\tdisplay this message\n");
		printf("\t -V --version\t\t\tdisplay version\n");
		printf("\t -S --server=x.x.x.x\t\tconnect to server address\n");
		printf("\t -C --config\t\t\tconfig file\n");
		printf("\t -P --port=xxxx\t\t\tconnect to server port\n");
		printf("\t -d --device=device\t\tthe device that you want to control\n");
		printf("\t -s --state=state\t\tthe new state of the device\n");
		printf("\t -v --values=values\t\tspecific comma separated values, e.g.:\n");
		printf("\t\t\t\t\t-v dimlevel=10\n");
		goto close;
	}
	if(device == NULL || state == NULL ||
	   strlen(device) == 0 || strlen(state) == 0) {
		printf("Usage: %s -d device -s state\n", progname);
		goto close;
	}

	if(server && port > 0) {
		if((sockfd = socket_connect(server, port)) == -1) {
			logprintf(LOG_ERR, "could not connect to pilight-daemon");
			goto close;
		}
	} else if(ssdp_seek(&ssdp_list) == -1) {
		logprintf(LOG_NOTICE, "no pilight ssdp connections found");
		goto close;
	} else {
		if((sockfd = socket_connect(ssdp_list->ip, ssdp_list->port)) == -1) {
			logprintf(LOG_ERR, "could not connect to pilight-daemon");
			goto close;
		}
	}
	if(ssdp_list) {
		ssdp_free(ssdp_list);
	}

	protocol_init();
	config_init();
	if(hasconfarg == 1) {
		if(config_read() != EXIT_SUCCESS) {
			goto close;
		}
	}

	socket_write(sockfd, "{\"action\":\"identify\"}");
	if(socket_read(sockfd, &recvBuff, 0) != 0
	   || strcmp(recvBuff, "{\"status\":\"success\"}") != 0) {
		goto close;
	}

	json = json_mkobject();
	json_append_member(json, "action", json_mkstring("request config"));
	output = json_stringify(json, NULL);
	socket_write(sockfd, output);
	json_free(output);
	json_delete(json);

	if(socket_read(sockfd, &recvBuff, 0) == 0) {
		if(json_validate(recvBuff) == true) {
			json = json_decode(recvBuff);
			if(json_find_string(json, "message", &message) == 0) {
				if(strcmp(message, "config") == 0) {
					struct JsonNode *jconfig = NULL;
					if((jconfig = json_find_member(json, "config")) != NULL) {
						int match = 1;
						while(match) {
							struct JsonNode *jchilds = json_first_child(jconfig);
							match = 0;
							while(jchilds) {
								if(strcmp(jchilds->key, "devices") != 0) {
									json_remove_from_parent(jchilds);
									tmp = jchilds;
									match = 1;
								}
								jchilds = jchilds->next;
								if(tmp != NULL) {
									json_delete(tmp);
								}
								tmp = NULL;
							}
						}
						config_parse(jconfig);
						if(devices_get(device, &dev) == 0) {
							JsonNode *joutput = json_mkobject();
							JsonNode *jcode = json_mkobject();
							JsonNode *jvalues = json_mkobject();
							json_append_member(jcode, "device", json_mkstring(device));

							if(values != NULL) {
								char *sptr = NULL;
								char *ptr1 = strtok_r(values, ",", &sptr);
								while(ptr1) {
									char **array = NULL;
									int n = explode(ptr1, "=", &array), q = 0;
									if(n == 2) {
										char *name = MALLOC(strlen(array[0])+1);
										if(name == NULL) {
											logprintf(LOG_ERR, "out of memory\n");
											exit(EXIT_FAILURE);
										}
										strcpy(name, array[q]);
										char *val = MALLOC(strlen(array[q+1])+1);
										if(val == NULL) {
											logprintf(LOG_ERR, "out of memory\n");
											exit(EXIT_FAILURE);
										}
										strcpy(val, array[1]);
										if(devices_valid_value(device, name, val) == 0) {
											if(isNumeric(val) == EXIT_SUCCESS) {
												json_append_member(jvalues, name, json_mknumber(atof(val), nrDecimals(val)));
											} else {
												json_append_member(jvalues, name, json_mkstring(val));
											}
											has_values = 1;
											FREE(name);
											FREE(values);
											array_free(&array, n);
										} else {
											logprintf(LOG_ERR, "\"%s\" is an invalid value for device \"%s\"", name, device);
											array_free(&array, n);
											FREE(name);
											FREE(values);
											json_delete(json);
											goto close;
										}								
									} else {
										array_free(&array, n);
										logprintf(LOG_ERR, "\"%s\" requires a name=value format", ptr1);
										break;
									}
									ptr1 = strtok_r(NULL, ",", &sptr);
								}
							}

							if(devices_valid_state(device, state) == 0) {
								json_append_member(jcode, "state", json_mkstring(state));
							} else {
								logprintf(LOG_ERR, "\"%s\" is an invalid state for device \"%s\"", state, device);
								json_delete(json);
								goto close;
							}

							if(has_values == 1) {
								json_append_member(jcode, "values", jvalues);
							} else {
								json_delete(jvalues);
							}
							json_append_member(joutput, "action", json_mkstring("control"));
							json_append_member(joutput, "code", jcode);
							output = json_stringify(joutput, NULL);
							socket_write(sockfd, output);
							json_free(output);
							json_delete(joutput);
							if(socket_read(sockfd, &recvBuff, 0) != 0
							   || strcmp(recvBuff, "{\"status\":\"success\"}") != 0) {
								logprintf(LOG_ERR, "failed to control %s", device);
							}
						} else {
							logprintf(LOG_ERR, "the device \"%s\" does not exist", device);
							json_delete(json);
							goto close;
						}
					}
				}
			}
			json_delete(json);
		}
	}
close:
	if(recvBuff) {
		FREE(recvBuff);
	}
	if(sockfd > 0) {
		socket_close(sockfd);
	}
	if(server != NULL) {
		FREE(server);
	}
	if(device != NULL) {
		FREE(device);
	}
	if(state != NULL) {
		FREE(state);
	}
	if(values != NULL) {
		FREE(values);
	}
	log_shell_disable();
	socket_gc();
	config_gc();
	protocol_gc();
	options_gc();
#ifdef EVENTS
	events_gc();
#endif
	dso_gc();
	log_gc();
	threads_gc();
	gc_clear();
	FREE(progname);
	xfree();

#ifdef _WIN32
	WSACleanup();
#endif

	return EXIT_SUCCESS;
}
