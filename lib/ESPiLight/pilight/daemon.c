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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
	#include <ws2tcpip.h>
	#include <winsvc.h>
	#include <shellapi.h>
	#define MSG_NOSIGNAL 0
#else
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
#endif
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#ifndef _WIN32
#include <wiringx.h>
#endif

#include "libs/pilight/core/pilight.h"
#include "libs/pilight/core/threads.h"
#include "libs/pilight/core/datetime.h"
#include "libs/pilight/core/common.h"
#include "libs/pilight/core/network.h"
#include "libs/pilight/core/gc.h"
#include "libs/pilight/core/log.h"
#include "libs/pilight/core/ssl.h"
#include "libs/pilight/core/options.h"
#include "libs/pilight/core/socket.h"
#include "libs/pilight/core/json.h"
#include "libs/pilight/core/ssdp.h"
#include "libs/pilight/core/dso.h"
#include "libs/pilight/core/firmware.h"
#include "libs/pilight/core/proc.h"
#include "libs/pilight/core/ntp.h"
#include "libs/pilight/core/config.h"
#include "libs/pilight/lua/lua.h"

#ifdef EVENTS
	#include "libs/pilight/events/events.h"
#endif

#ifdef WEBSERVER
	#ifdef WEBSERVER_HTTPS
		#include <mbedtls/md5.h>
	#endif
	#include "libs/pilight/core/webserver.h"
#endif

#include "libs/pilight/config/hardware.h"
#include "libs/pilight/config/registry.h"
#include "libs/pilight/config/devices.h"
#include "libs/pilight/config/settings.h"
#include "libs/pilight/config/gui.h"

static uv_signal_t **signal_req = NULL;
static int signals[5] = { SIGINT, SIGQUIT, SIGTERM, SIGABRT, SIGTSTP };
static uv_timer_t *timer_abort_req = NULL;
static uv_timer_t *timer_stats_req = NULL;

#ifdef _WIN32
static char server_name[40];
#endif

typedef struct clients_t {
	char uuid[UUID_LENGTH];
	int id;
	int receiver;
	int config;
	int core;
	int stats;
	int forward;
	char media[8];
	double cpu;
	double ram;
	struct clients_t *next;
} clients_t;

static struct clients_t *clients = NULL;

typedef struct sendqueue_t {
	unsigned int id;
	char *protoname;
	char *settings;
	char *message;
	enum origin_t origin;
	struct protocol_t *protopt;
	int code[MAXPULSESTREAMLENGTH];
	int length;
	char uuid[UUID_LENGTH];
	struct sendqueue_t *next;
} sendqueue_t;

static struct sendqueue_t *sendqueue;
static struct sendqueue_t *sendqueue_head;

typedef struct recvqueue_t {
	int raw[MAXPULSESTREAMLENGTH];
	int rawlen;
	int hwtype;
	int plslen;
	struct recvqueue_t *next;
} recvqueue_t;

static struct recvqueue_t *recvqueue;
static struct recvqueue_t *recvqueue_head;

static pthread_mutex_t config_lock;
static pthread_mutexattr_t config_attr;

static pthread_mutex_t sendqueue_lock;
static pthread_cond_t sendqueue_signal;
static pthread_mutexattr_t sendqueue_attr;
static unsigned short sendqueue_init = 0;

static int sendqueue_number = 0;
static int recvqueue_number = 0;

static pthread_mutex_t recvqueue_lock;
static pthread_cond_t recvqueue_signal;
static pthread_mutexattr_t recvqueue_attr;
static unsigned short recvqueue_init = 0;

typedef struct bcqueue_t {
	struct JsonNode *jmessage;
	char *protoname;
	enum origin_t origin;
	struct bcqueue_t *next;
} bcqueue_t;

static struct bcqueue_t *bcqueue;
static struct bcqueue_t *bcqueue_head;

static pthread_mutex_t bcqueue_lock;
static pthread_cond_t bcqueue_signal;
static pthread_mutexattr_t bcqueue_attr;
static unsigned short bcqueue_init = 0;

static int bcqueue_number = 0;

static struct protocol_t *procProtocol;

/* The pid_file and pid of this daemon */
#ifndef _WIN32
static char *pid_file;
static unsigned short pid_file_free = 0;
#endif
static pid_t pid;
/* Daemonize or not */
static int nodaemon = 0;
/* Run tracktracer */
static int stacktracer = 0;
/* Run thread profiler */
static int threadprofiler = 0;
/* Are we already running */
static int running = 1;
/* Are we currently sending code */
static int sending = 0;
/* Socket identifier to the server if we are running as client */
static int sockfd = 0;
/* Thread pointers */
static pthread_t logpth;
/* While loop conditions */
static unsigned short main_loop = 1;
/* Reset repeats after a certain amount of time */
static struct timeval tv;
/* Are we running standalone */
static int standalone = 0;
/* Do we need to connect to a master server:port? */
static char *master_server = NULL;
static unsigned short master_port = 0;

static int adhoc_pending = 0;
static char *configtmp = NULL;
static int verbosity = LOG_INFO;
struct socket_callback_t socket_callback;

#ifdef _WIN32
	static int console = 0;
	static int oldverbosity = LOG_NOTICE;
#endif


#ifdef WEBSERVER
/* Do we enable the webserver */
static int webserver_enable = WEBSERVER_ENABLE;
static int webgui_websockets = WEBGUI_WEBSOCKETS;
/* On what port does the webserver run */
static int webserver_http_port = WEBSERVER_HTTP_PORT;
/* The webroot of pilight */
static char *webserver_root = NULL;
static int webserver_root_free = 0;
#endif

static void *reason_forward_free(void *param) {
	FREE(param);
	return NULL;
}

static void *reason_send_code_free(void *param) {
	struct reason_send_code_t *data = param;
	FREE(data);
	return NULL;
}

static void *reason_broadcast_core_free(void *param) {
	char *code = param;
	FREE(code);
	return NULL;
}

static void *reason_socket_send_free(void *param) {
	struct reason_socket_send_t *data = param;
	FREE(data->buffer);
	FREE(data);
	return NULL;
}

static void client_remove(int id) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct clients_t *currP, *prevP;

	prevP = NULL;

	for(currP = clients; currP != NULL; prevP = currP, currP = currP->next) {

		if(currP->id == id) {
			if(prevP == NULL) {
				clients = currP->next;
			} else {
				prevP->next = currP->next;
			}

			FREE(currP);
			break;
		}
	}
}

static void broadcast_queue(char *protoname, struct JsonNode *json, enum origin_t origin) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(main_loop == 1) {
		pthread_mutex_lock(&bcqueue_lock);
		if(bcqueue_number <= 1024) {
			struct bcqueue_t *bnode = MALLOC(sizeof(struct bcqueue_t));
			if(bnode == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}

			char *jstr = json_stringify(json, NULL);
			bnode->jmessage = json_decode(jstr);
			if(json_find_member(bnode->jmessage, "uuid") == NULL && strlen(pilight_uuid) > 0) {
				json_append_member(bnode->jmessage, "uuid", json_mkstring(pilight_uuid));
			}
			json_free(jstr);

			if((bnode->protoname = MALLOC(strlen(protoname)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(bnode->protoname, protoname);

			bnode->origin = origin;

			if(bcqueue_number == 0) {
				bcqueue = bnode;
				bcqueue_head = bnode;
			} else {
				bcqueue_head->next = bnode;
				bcqueue_head = bnode;
			}

			bcqueue_number++;
		} else {
			logprintf(LOG_ERR, "broadcast queue full");
		}
		pthread_mutex_unlock(&bcqueue_lock);
		pthread_cond_signal(&bcqueue_signal);
	}
}

void *broadcast(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int broadcasted = 0/*, free_conf = 1*/;

	pthread_mutex_lock(&bcqueue_lock);
	while(main_loop) {
		if(bcqueue_number > 0) {
			pthread_mutex_lock(&bcqueue_lock);

			logprintf(LOG_STACK, "%s::unlocked", __FUNCTION__);

			broadcasted = 0;
			struct JsonNode *jret = NULL;
			char *origin = NULL;

			if(json_find_string(bcqueue->jmessage, "origin", &origin) == 0) {
				if(strcmp(origin, "core") == 0) {
					double tmp = 0;
					json_find_number(bcqueue->jmessage, "type", &tmp);
					char *conf = json_stringify(bcqueue->jmessage, NULL);
					struct clients_t *tmp_clients = clients;
					while(tmp_clients) {
						if(((int)tmp < 0 && tmp_clients->core == 1) ||
						   ((int)tmp >= 0 && tmp_clients->config == 1) ||
							 ((int)tmp == PROCESS && tmp_clients->stats == 1)) {
							socket_write(tmp_clients->id, conf);
							broadcasted = 1;
						}
						tmp_clients = tmp_clients->next;
					}

					if(pilight.runmode == ADHOC && sockfd > 0) {
						struct JsonNode *jupdate = json_decode(conf);
						json_append_member(jupdate, "action", json_mkstring("update"));
						char *ret = json_stringify(jupdate, NULL);
						socket_write(sockfd, ret);
						broadcasted = 1;
						json_delete(jupdate);
						json_free(ret);
					}
					if(broadcasted == 1) {
						logprintf(LOG_DEBUG, "broadcasted: %s", conf);
					}
					eventpool_trigger(REASON_BROADCAST_CORE, reason_broadcast_core_free, conf);
				} else {
					/* Update the config */
					if(devices_update(bcqueue->protoname, bcqueue->jmessage, bcqueue->origin, &jret) == 0) {
						char *tmp = json_stringify(jret, NULL);
						struct clients_t *tmp_clients = clients;
						unsigned short match1 = 0, match2 = 0;

						while(tmp_clients) {
							if(tmp_clients->config == 1) {
								struct JsonNode *jtmp = json_decode(tmp);
								struct JsonNode *jdevices = json_find_member(jtmp, "devices");
								if(jdevices != NULL) {
									match1 = 0;
									struct JsonNode *jchilds = json_first_child(jdevices);
									struct gui_values_t *gui_values = NULL;
									while(jchilds) {
										match2 = 0;
										if(jchilds->tag == JSON_STRING) {
											if((gui_values = gui_media(jchilds->string_)) != NULL) {
												while(gui_values) {
													if(gui_values->type == JSON_STRING) {
														if(strcmp(gui_values->string_, tmp_clients->media) == 0 ||
															 strcmp(gui_values->string_, "all") == 0 ||
															 strcmp(tmp_clients->media, "all") == 0) {
																match1 = 1;
																match2 = 1;
														}
													}
													gui_values = gui_values->next;
												}
											} else {
												match1 = 1;
												match2 = 1;
											}
										}
										if(match2 == 0) {
											json_remove_from_parent(jchilds);
										}
										struct JsonNode *jtmp1 = jchilds;
										jchilds = jchilds->next;
										if(match2 == 0) {
											json_delete(jtmp1);
										}
									}
								}
								if(match1 == 1) {
									char *conf = json_stringify(jtmp, NULL);
									socket_write(tmp_clients->id, conf);
									logprintf(LOG_DEBUG, "broadcasted: %s", conf);
									json_free(conf);
								}
								json_delete(jtmp);
							}
							tmp_clients = tmp_clients->next;
						}
						eventpool_trigger(REASON_BROADCAST_CORE, reason_broadcast_core_free, tmp);

						// json_free(tmp);
						json_delete(jret);
					}

					/* The settings objects inside the broadcast queue is only of interest for the
					   internal pilight functions. For the outside world we only communicate the
					   message part of the queue so we remove the settings */
					char *internal = json_stringify(bcqueue->jmessage, NULL);

					struct JsonNode *jsettings = NULL;
					if((jsettings = json_find_member(bcqueue->jmessage, "settings"))) {
						json_remove_from_parent(jsettings);
						json_delete(jsettings);
					}
					struct JsonNode *tmp = json_find_member(bcqueue->jmessage, "action");
					if(tmp != NULL && tmp->tag == JSON_STRING && strcmp(tmp->string_, "update") == 0) {
						json_remove_from_parent(tmp);
						json_delete(tmp);
					}

					char *out = json_stringify(bcqueue->jmessage, NULL);
					if(strcmp(bcqueue->protoname, "pilight_firmware") == 0) {
						struct JsonNode *code = NULL;
						if((code = json_find_member(bcqueue->jmessage, "message")) != NULL) {
							json_find_number(code, "version", &firmware.version);
							json_find_number(code, "lpf", &firmware.lpf);
							json_find_number(code, "hpf", &firmware.hpf);
							if(firmware.version > 0 && firmware.lpf > 0 && firmware.hpf > 0) {
								registry_set_number("pilight.firmware.version", firmware.version, 0);
								registry_set_number("pilight.firmware.lpf", firmware.lpf, 0);
								registry_set_number("pilight.firmware.hpf", firmware.hpf, 0);

								struct JsonNode *jmessage = json_mkobject();
								struct JsonNode *jcode = json_mkobject();
								json_append_member(jcode, "version", json_mknumber(firmware.version, 0));
								json_append_member(jcode, "lpf", json_mknumber(firmware.lpf, 0));
								json_append_member(jcode, "hpf", json_mknumber(firmware.hpf, 0));
								json_append_member(jmessage, "values", jcode);
								json_append_member(jmessage, "origin", json_mkstring("core"));
								json_append_member(jmessage, "type", json_mknumber(FIRMWARE, 0));
								char pname[17];
								strcpy(pname, "pilight-firmware");
								pilight.broadcast(pname, jmessage, FW);
								json_delete(jmessage);
								jmessage = NULL;
							}
						}
					}
					broadcasted = 0;

					struct JsonNode *childs = json_first_child(bcqueue->jmessage);
					int nrchilds = 0;
					while(childs) {
						nrchilds++;
						childs = childs->next;
					}

					/* Write the message to all receivers */
					struct clients_t *tmp_clients = clients;
					while(tmp_clients) {
						if(tmp_clients->receiver == 1 && tmp_clients->forward == 0) {
								if(strcmp(out, "{}") != 0 && nrchilds > 1) {
									socket_write(tmp_clients->id, out);
									broadcasted = 1;
								}
						}
						tmp_clients = tmp_clients->next;
					}

					if(pilight.runmode == ADHOC && sockfd > 0) {
						struct JsonNode *jupdate = json_decode(internal);
						json_append_member(jupdate, "action", json_mkstring("update"));
						char *ret = json_stringify(jupdate, NULL);
						socket_write(sockfd, ret);
						broadcasted = 1;
						json_delete(jupdate);
						json_free(ret);
					}
					if((broadcasted == 1 || nodaemon == 1) && (strcmp(out, "{}") != 0 && nrchilds > 1)) {
						logprintf(LOG_DEBUG, "broadcasted: %s", out);
					}
					json_free(internal);
					// json_free(out);
					eventpool_trigger(REASON_BROADCAST_CORE, reason_broadcast_core_free, out);
				}
			}
			struct bcqueue_t *tmp = bcqueue;
			FREE(tmp->protoname);
			json_delete(tmp->jmessage);
			bcqueue = bcqueue->next;
			FREE(tmp);
			bcqueue_number--;
			pthread_mutex_unlock(&bcqueue_lock);
		} else {
			pthread_cond_wait(&bcqueue_signal, &bcqueue_lock);
		}
	}
	return (void *)NULL;
}

static void receive_queue(int *raw, int rawlen, int plslen, int hwtype) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int i = 0;

	if(main_loop == 1) {
		pthread_mutex_lock(&recvqueue_lock);
		if(recvqueue_number <= 1024) {
			struct recvqueue_t *rnode = MALLOC(sizeof(struct recvqueue_t));
			if(rnode == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			for(i=0;i<rawlen;i++) {
				rnode->raw[i] = raw[i];
			}
			rnode->rawlen = rawlen;
			rnode->plslen = plslen;
			rnode->hwtype = hwtype;

			if(recvqueue_number == 0) {
				recvqueue = rnode;
				recvqueue_head = rnode;
			} else {
				recvqueue_head->next = rnode;
				recvqueue_head = rnode;
			}

			recvqueue_number++;
		} else {
			logprintf(LOG_ERR, "receiver queue full");
		}
		pthread_mutex_unlock(&recvqueue_lock);
		pthread_cond_signal(&recvqueue_signal);
	}
}

static void receiver_create_message(protocol_t *protocol) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(protocol->message != NULL) {
		char *valid = json_stringify(protocol->message, NULL);
		json_delete(protocol->message);
		if(valid != NULL && json_validate(valid) == true) {
			struct JsonNode *jmessage = json_mkobject();

			json_append_member(jmessage, "message", json_decode(valid));
			json_append_member(jmessage, "origin", json_mkstring("receiver"));
			json_append_member(jmessage, "protocol", json_mkstring(protocol->id));
			if(strlen(pilight_uuid) > 0) {
				json_append_member(jmessage, "uuid", json_mkstring(pilight_uuid));
			}
			if(protocol->repeats > -1) {
				json_append_member(jmessage, "repeats", json_mknumber(protocol->repeats, 0));
			}
			char *output = json_stringify(jmessage, NULL);
			struct JsonNode *json = json_decode(output);
			broadcast_queue(protocol->id, json, RECEIVER);
			json_free(output);
			json_delete(json);
			json = NULL;
			json_delete(jmessage);
		}
		json_free(valid);
	}
	protocol->message = NULL;
}

static void receive_parse_api(struct JsonNode *code, int hwtype) {
	struct protocol_t *protocol = NULL;
	struct protocols_t *pnode = protocols;

	while(pnode != NULL) {
		protocol = pnode->listener;

		if(protocol->hwtype == hwtype && protocol->parseCommand != NULL) {
			protocol->parseCommand(code);
			receiver_create_message(protocol);
		}		
		pnode = pnode->next;
	}
}

void *receive_parse_code(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	pthread_mutex_lock(&recvqueue_lock);
	while(main_loop) {
		if(recvqueue_number > 0) {
			pthread_mutex_lock(&recvqueue_lock);

			logprintf(LOG_STACK, "%s::unlocked", __FUNCTION__);

			struct protocol_t *protocol = NULL;
			struct protocols_t *pnode = protocols;

			while(pnode != NULL && main_loop) {
				protocol = pnode->listener;

				if((protocol->hwtype == recvqueue->hwtype || protocol->hwtype == -1 || recvqueue->hwtype == -1) &&
				   (protocol->parseCode != NULL && protocol->validate != NULL)) {

					if(recvqueue->rawlen < MAXPULSESTREAMLENGTH) {
						protocol->raw = recvqueue->raw;
					}
					protocol->rawlen = recvqueue->rawlen;

					if(protocol->validate() == 0) {
						logprintf(LOG_DEBUG, "possible %s protocol", protocol->id);
						gettimeofday(&tv, NULL);
						if(protocol->first > 0) {
							protocol->first = protocol->second;
						}
						protocol->second = 1000000 * (unsigned int)tv.tv_sec + (unsigned int)tv.tv_usec;
						if(protocol->first == 0) {
							protocol->first = protocol->second;
						}

						/* Reset # of repeats after a certain delay */
						if(((int)protocol->second-(int)protocol->first) > 500000) {
							protocol->repeats = 0;
						}

						protocol->repeats++;
						if(protocol->parseCode != NULL) {
							logprintf(LOG_DEBUG, "recevied pulse length of %d", recvqueue->plslen);
							logprintf(LOG_DEBUG, "caught minimum # of repeats %d of %s", protocol->repeats, protocol->id);
							logprintf(LOG_DEBUG, "called %s parseRaw()", protocol->id);
							protocol->parseCode();
							receiver_create_message(protocol);
						}
					}
				}
				pnode = pnode->next;
			}

			struct recvqueue_t *tmp = recvqueue;
			recvqueue = recvqueue->next;
			FREE(tmp);
			recvqueue_number--;
			pthread_mutex_unlock(&recvqueue_lock);
		} else {
			pthread_cond_wait(&recvqueue_signal, &recvqueue_lock);
		}
	}
	return (void *)NULL;
}

void *send_code(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int i = 0;

	/* Make sure the pilight sender gets
	   the highest priority available */
#ifdef _WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#else
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = 80;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched);
#endif

	pthread_mutex_lock(&sendqueue_lock);

	while(main_loop) {
		if(sendqueue_number > 0) {
			pthread_mutex_lock(&sendqueue_lock);

			logprintf(LOG_STACK, "%s::unlocked", __FUNCTION__);

			sending = 1;

			struct protocol_t *protocol = sendqueue->protopt;
			struct hardware_t *hw = NULL;

			struct JsonNode *message = NULL;

			if(sendqueue->message != NULL && strcmp(sendqueue->message, "{}") != 0) {
				if(json_validate(sendqueue->message) == true) {
					if(message == NULL) {
						message = json_mkobject();
					}
					json_append_member(message, "origin", json_mkstring("sender"));
					json_append_member(message, "protocol", json_mkstring(protocol->id));
					json_append_member(message, "message", json_decode(sendqueue->message));
					if(strlen(sendqueue->uuid) > 0) {
						json_append_member(message, "uuid", json_mkstring(sendqueue->uuid));
					}
					json_append_member(message, "repeat", json_mknumber(1, 0));
				}
			}
			if(sendqueue->settings != NULL && strcmp(sendqueue->settings, "{}") != 0) {
				if(json_validate(sendqueue->settings) == true) {
					if(message == NULL) {
						message = json_mkobject();
					}
					json_append_member(message, "settings", json_decode(sendqueue->settings));
				}
			}

			struct conf_hardware_t *tmp_confhw = conf_hardware;
			while(tmp_confhw) {
				if(protocol->hwtype == tmp_confhw->hardware->hwtype) {
					hw = tmp_confhw->hardware;
					break;
				}
				tmp_confhw = tmp_confhw->next;
			}
			if(hw != NULL) {
#ifdef PILIGHT_DEVELOPMENT
				if((hw->comtype == COMOOK || hw->comtype == COMPLSTRAIN) && hw->sendOOK != NULL) {
					if(hw->receiveOOK != NULL || hw->receivePulseTrain != NULL) {
						hw->wait = 1;
						pthread_mutex_unlock(&hw->lock);
						pthread_cond_signal(&hw->signal);
					}
#else
				if((hw->comtype == COMOOK) || (hw->comtype == COMPLSTRAIN && hw->sendOOK != NULL)) {
#endif
					logprintf(LOG_DEBUG, "**** RAW CODE ****");
					if(log_level_get() >= LOG_DEBUG) {
						for(i=0;i<sendqueue->length;i++) {
							printf("%d ", sendqueue->code[i]);
						}
						printf("\n");
					}
					logprintf(LOG_DEBUG, "**** RAW CODE ****");

					/*
					 * Rewrite start
					 */
					struct reason_send_code_t *data = MALLOC(sizeof(struct reason_send_code_t));
					if(data == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					data->origin = ORIGIN_SENDER;
					memset(&data->message, 0, 255);
					// snprintf(data->message, 1024, "{\"message\":%s}", message);
					data->rawlen = sendqueue->length;
					memcpy(data->pulses, sendqueue->code, data->rawlen*sizeof(int));
					data->txrpt = protocol->txrpt;
					strncpy(data->protocol, protocol->id, 255);
					data->hwtype = hw->hwtype;

					memset(data->uuid, 0, UUID_LENGTH+1);
					eventpool_trigger(REASON_SEND_CODE, reason_send_code_free, data);
					/*
					 * Rewrite end
					 */

#ifdef PILIGHT_DEVELOPMENT
					if(hw->sendOOK(sendqueue->code, sendqueue->length, protocol->txrpt) == 0) {
						logprintf(LOG_DEBUG, "successfully send %s code", protocol->id);
					} else {
						logprintf(LOG_ERR, "failed to send code");
					}
#endif
					if(strcmp(protocol->id, "raw") == 0) {
						int plslen = sendqueue->code[sendqueue->length-1]/PULSE_DIV;
						receive_queue(sendqueue->code, sendqueue->length, plslen, -1);
					}
#ifdef PILIGHT_DEVELOPMENT
					if(hw->receiveOOK != NULL || hw->receivePulseTrain != NULL) {
						hw->wait = 0;
						pthread_mutex_unlock(&hw->lock);
						pthread_cond_signal(&hw->signal);
					}
#endif
				} else if(hw->comtype == COMAPI && hw->sendAPI != NULL) {
					if(message != NULL) {				
						if(hw->sendAPI(message) == 0) {
							logprintf(LOG_DEBUG, "successfully send %s command", protocol->id);
						} else {
							logprintf(LOG_ERR, "failed to send command");
						}
					}
				}
			} else {
				if(strcmp(protocol->id, "raw") == 0) {
					int plslen = sendqueue->code[sendqueue->length-1]/PULSE_DIV;
					receive_queue(sendqueue->code, sendqueue->length, plslen, -1);
				}
			}
			if(message != NULL) {
				broadcast_queue(sendqueue->protoname, message, sendqueue->origin);
				json_delete(message);
				message = NULL;
			}

			struct sendqueue_t *tmp = sendqueue;
			if(tmp->message != NULL) {
				FREE(tmp->message);
			}
			if(tmp->settings != NULL) {
				FREE(tmp->settings);
			}
			FREE(tmp->protoname);
			sendqueue = sendqueue->next;
			FREE(tmp);
			sendqueue_number--;
			sending = 0;
			pthread_mutex_unlock(&sendqueue_lock);
		} else {
			pthread_cond_wait(&sendqueue_signal, &sendqueue_lock);
		}
	}
	return (void *)NULL;
}

/* Send a specific code */
static int send_queue(struct JsonNode *json, enum origin_t origin) {
	pthread_mutex_lock(&sendqueue_lock);
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int match = 0, raw[MAXPULSESTREAMLENGTH-1];
	struct timeval tcurrent;
	struct clients_t *tmp_clients = NULL;
	char *uuid = NULL, *buffer = NULL;
	/* Hold the final protocol struct */
	struct protocol_t *protocol = NULL;

#ifdef _WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#else
	/* Make sure the pilight sender gets
	   the highest priority available */
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = 80;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched);
#endif

	struct JsonNode *jcode = NULL;
	struct JsonNode *jprotocols = NULL;
	struct JsonNode *jprotocol = NULL;

	buffer = json_stringify(json, NULL);
	tmp_clients = clients;
	while(tmp_clients) {
		if(tmp_clients->forward == 1) {
			socket_write(tmp_clients->id, buffer);
		}
		tmp_clients = tmp_clients->next;
	}
	json_free(buffer);

	if((jcode = json_find_member(json, "code")) == NULL) {
		logprintf(LOG_ERR, "sender did not send any codes");
		json_delete(jcode);
		pthread_mutex_unlock(&sendqueue_lock);
		return -1;
	} else if((jprotocols = json_find_member(jcode, "protocol")) == NULL) {
		logprintf(LOG_ERR, "sender did not provide a protocol name");
		json_delete(jcode);
		pthread_mutex_unlock(&sendqueue_lock);
		return -1;
	} else {
		json_find_string(jcode, "uuid", &uuid);
		/* If we matched a protocol and are not already sending, continue */
		if(uuid == NULL || (uuid != NULL && strcmp(uuid, pilight_uuid) == 0)) {
			jprotocol = json_first_child(jprotocols);
			while(jprotocol && match == 0) {
				match = 0;
				if(jprotocol->tag == JSON_STRING) {
					struct protocols_t *pnode = protocols;
					/* Retrieve the used protocol */
					while(pnode) {
						protocol = pnode->listener;
						/* Check if the protocol exists */
						if(protocol_device_exists(protocol, jprotocol->string_) == 0 && match == 0) {
							match = 1;
							break;
						}
						pnode = pnode->next;
					}
				}
				jprotocol = jprotocol->next;
			}
			memset(raw, 0, sizeof(raw));
			protocol->raw = raw;
			if(match == 1 && protocol->createCode != NULL) {
				/* Let the protocol create his code */
				if(protocol->createCode(jcode) == 0 && main_loop == 1) {
					if(sendqueue_number <= 1024) {
						struct sendqueue_t *mnode = MALLOC(sizeof(struct sendqueue_t));
						if(mnode == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						gettimeofday(&tcurrent, NULL);
						mnode->origin = origin;
						mnode->id = 1000000 * (unsigned int)tcurrent.tv_sec + (unsigned int)tcurrent.tv_usec;
						mnode->message = NULL;
						if(protocol->message != NULL) {
							char *jsonstr = json_stringify(protocol->message, NULL);
							json_delete(protocol->message);
							if(json_validate(jsonstr) == true) {
								if((mnode->message = MALLOC(strlen(jsonstr)+1)) == NULL) {
									fprintf(stderr, "out of memory\n");
									exit(EXIT_FAILURE);
								}
								strcpy(mnode->message, jsonstr);
							}
							json_free(jsonstr);
							protocol->message = NULL;
						}

						mnode->length = protocol->rawlen;
						memcpy(mnode->code, protocol->raw, sizeof(int)*protocol->rawlen);

						if((mnode->protoname = MALLOC(strlen(protocol->id)+1)) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						strcpy(mnode->protoname, protocol->id);
						mnode->protopt = protocol;

						struct options_t *tmp_options = protocol->options;
						char *stmp = NULL;
						struct JsonNode *jsettings = json_mkobject();
						struct JsonNode *jtmp = NULL;
						while(tmp_options) {
							if(tmp_options->conftype == DEVICES_SETTING) {
								if(tmp_options->vartype == JSON_NUMBER &&
								  (jtmp = json_find_member(jcode, tmp_options->name)) != NULL &&
								   jtmp->tag == JSON_NUMBER) {
									json_append_member(jsettings, tmp_options->name, json_mknumber(jtmp->number_, jtmp->decimals_));
								} else if(tmp_options->vartype == JSON_STRING && json_find_string(jcode, tmp_options->name, &stmp) == 0) {
									json_append_member(jsettings, tmp_options->name, json_mkstring(stmp));
								}
							}
							tmp_options = tmp_options->next;
						}
						char *strsett = json_stringify(jsettings, NULL);
						if((mnode->settings = MALLOC(strlen(strsett)+1)) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						strcpy(mnode->settings, strsett);
						json_free(strsett);
						json_delete(jsettings);

						if(uuid != NULL) {
							strcpy(mnode->uuid, uuid);
						} else {
							memset(mnode->uuid, '\0', UUID_LENGTH);
						}
						if(sendqueue_number == 0) {
							sendqueue = mnode;
							sendqueue_head = mnode;
						} else {
							sendqueue_head->next = mnode;
							sendqueue_head = mnode;
						}
						sendqueue_number++;
					} else {
						logprintf(LOG_ERR, "send queue full");
						pthread_mutex_unlock(&sendqueue_lock);
						return -1;
					}
					pthread_mutex_unlock(&sendqueue_lock);
					pthread_cond_signal(&sendqueue_signal);
					return 0;
				} else {
					pthread_mutex_unlock(&sendqueue_lock);
					return -1;
				}
			}
		} else {
			pthread_mutex_unlock(&sendqueue_lock);
			return 0;
		}
	}
	pthread_mutex_unlock(&sendqueue_lock);
	return -1;
}

#ifdef WEBSERVER
static void client_webserver_parse_code(int i, char buffer[BUFFER_SIZE]) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int sd = socket_get_clients(i);
	int x = 0;
	FILE *f;
	char *p = NULL;
	char buff[BUFFER_SIZE];
	char *cache = NULL;
	char *path = NULL, buf[INET_ADDRSTRLEN+1];
	struct stat sb;
	struct sockaddr_in sockin;
	socklen_t len = sizeof(sockin);

	if(getsockname(sd, (struct sockaddr *)&sockin, &len) == -1) {
		logprintf(LOG_ERR, "could not determine server ip address");
	} else {
		if(strstr(buffer, " HTTP/") == NULL) {
			return;
		}
		p = buff;
		if(strstr(buffer, "/logo.png") != NULL) {
			if((path = MALLOC(strlen(webserver_root)+strlen("logo.png")+2)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			sprintf(path, "%s/logo.png", webserver_root);
			if((f = fopen(path, "rb"))) {
				fstat(fileno(f), &sb);
				webserver_create_header(&p, "200 OK", "image/png", (unsigned int)sb.st_size);
				send(sd, (const char *)buff, (size_t)(p-buff), MSG_NOSIGNAL);
				x = 0;
				if((cache = MALLOC(BUFFER_SIZE)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				memset(cache, '\0', BUFFER_SIZE);
				while(!feof(f)) {
					x = (int)fread(cache, 1, BUFFER_SIZE, f);
					send(sd, (const char *)cache, (size_t)x, MSG_NOSIGNAL);
				}
				fclose(f);
				FREE(cache);
			} else {
				logprintf(LOG_WARNING, "pilight logo not found");
			}
			FREE(path);
		} else {
		    /* Catch all webserver page to inform users on which port the webserver runs */
			webserver_create_header(&p, "200 OK", "text/html", (unsigned int)BUFFER_SIZE);
			send(sd, (const char *)buff, (size_t)(p-buff), MSG_NOSIGNAL);
			if(webserver_enable == 1) {
				if((cache = MALLOC(BUFFER_SIZE)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				memset(cache, '\0', BUFFER_SIZE);
				memset(&buf, '\0', INET_ADDRSTRLEN+1);
				inet_ntop(AF_INET, (void *)&(sockin.sin_addr), buf, INET_ADDRSTRLEN+1);
				sprintf(cache, "<html><head><title>pilight</title></head>"
							   "<body><center><img src=\"logo.png\"><br />"
							   "<p style=\"color: #0099ff; font-weight: 800px;"
							   "font-family: Verdana; font-size: 20px;\">"
							   "The pilight webgui is located at "
							   "<a style=\"text-decoration: none; color: #0099ff;"
							   "font-weight: 800px; font-family: Verdana; font-size:"
							   "20px;\" href=\"http://%s:%d\">http://%s:%d</a></p>"
							   "</center></body></html>",
							   buf, webserver_http_port, buf, webserver_http_port);
				send(sd, (const char *)cache, strlen(cache), MSG_NOSIGNAL);
				FREE(cache);
			} else {
				send(sd, "<body><center><img src=\"logo.png\"></center></body></html>", 57, MSG_NOSIGNAL);
			}
		}
	}
}
#endif

static int control_device(struct devices_t *dev, char *state, struct JsonNode *values, enum origin_t origin) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct devices_settings_t *sett = NULL;
	struct devices_values_t *val = NULL;
	struct options_t *opt = NULL;
	struct protocols_t *tmp_protocols = NULL;

	struct JsonNode *code = json_mkobject();
	struct JsonNode *json = json_mkobject();
	struct JsonNode *jprotocols = json_mkarray();

	/* Check all protocol options */
	tmp_protocols = dev->protocols;
	while(tmp_protocols) {
		json_append_element(jprotocols, json_mkstring(tmp_protocols->name));
		if((opt = tmp_protocols->listener->options)) {
			while(opt) {
				sett = dev->settings;
				while(sett) {
					/* Retrieve the device id's */
					if(strcmp(sett->name, "id") == 0) {
						val = sett->values;
						while(val) {
							if((opt->conftype == DEVICES_ID)
							   && strcmp(val->name, opt->name) == 0
							   && json_find_member(code, opt->name) == NULL) {
								if(val->type == JSON_STRING) {
									json_append_member(code, val->name, json_mkstring(val->string_));
								} else if(val->type == JSON_NUMBER) {
									json_append_member(code, val->name, json_mknumber(val->number_, val->decimals));
								}
							}
							val = val->next;
						}
					}
					if(strcmp(sett->name, opt->name) == 0
					   && opt->conftype == DEVICES_SETTING) {
						val = sett->values;
						if(json_find_member(code, opt->name) == NULL) {
							if(val->type == JSON_STRING) {
								json_append_member(code, opt->name, json_mkstring(val->string_));
							} else if(val->type == JSON_NUMBER) {
								json_append_member(code, opt->name, json_mknumber(val->number_, val->decimals));
							}
						}
					}
					sett = sett->next;
				}
				opt = opt->next;
			}
			while(values) {
				opt = tmp_protocols->listener->options;
				while(opt) {
					if((opt->conftype == DEVICES_VALUE || opt->conftype == DEVICES_OPTIONAL)
					   && strcmp(values->key, opt->name) == 0
					   && json_find_member(code, opt->name) == NULL) {
						if(values->tag == JSON_STRING) {
							json_append_member(code, values->key, json_mkstring(values->string_));
						} else if(values->tag == JSON_NUMBER) {
							json_append_member(code, values->key, json_mknumber(values->number_, values->decimals_));
						}
					}
					opt = opt->next;
				}
				values = values->next;
			}
		}
		/* Send the new device state */
		if((opt = tmp_protocols->listener->options) && state != NULL) {
			while(opt) {
				if(json_find_member(code, opt->name) == NULL) {
					if(opt->conftype == DEVICES_STATE && opt->argtype == OPTION_NO_VALUE && strcmp(opt->name, state) == 0) {
						json_append_member(code, opt->name, json_mknumber(1, 0));
						break;
					} else if(opt->conftype == DEVICES_STATE && opt->argtype == OPTION_HAS_VALUE) {
						json_append_member(code, opt->name, json_mkstring(state));
						break;
					}
				}
				opt = opt->next;
			}
		}
		tmp_protocols = tmp_protocols->next;
	}

	/* Construct the right json object */
	json_append_member(code, "protocol", jprotocols);
	if(dev->dev_uuid != NULL && dev->cst_uuid == 1) {
	/*if(dev->dev_uuid != NULL && (dev->protocols->listener->hwtype == SENSOR
	   || dev->protocols->listener->hwtype == HWRELAY)) {*/
		json_append_member(code, "uuid", json_mkstring(dev->dev_uuid));
	}
	json_append_member(json, "code", code);
	json_append_member(json, "action", json_mkstring("send"));

	if(send_queue(json, origin) == 0) {
		json_delete(json);
		return 0;
	}

	json_delete(json);
	return -1;
}

/* Parse the incoming buffer from the client */
static void socket_parse_data(int i, char *buffer) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct sockaddr_in address;
	struct JsonNode *json = NULL;
	struct JsonNode *options = NULL;
	struct clients_t *tmp_clients = NULL;
	struct clients_t *client = NULL;
	int sd = -1;
	int addrlen = sizeof(address);
	char *action = NULL, *media = NULL, *status = NULL;
	int error = 0, exists = 0;

	if(pilight.runmode == ADHOC) {
		sd = sockfd;
	} else {
		sd = socket_get_clients(i);
		getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	}

	if(strcmp(buffer, "HEART") == 0) {
		socket_write(sd, "BEAT");
	} else {
		if(pilight.runmode != ADHOC) {
			logprintf(LOG_DEBUG, "socket recv: %s", buffer);
		}
		/* Serve static webserver page. This is the only request that is
		   expected not to be a json object */
#ifdef WEBSERVER
		if(strstr(buffer, " HTTP/")) {
			client_webserver_parse_code(i, buffer);
			socket_close(sd);
		} else if(json_validate(buffer) == true) {
#else
		if(json_validate(buffer) == true) {
#endif
			json = json_decode(buffer);
			if((json_find_string(json, "action", &action)) == 0) {
				tmp_clients = clients;
				while(tmp_clients) {
					if(tmp_clients->id == sd) {
						exists = 1;
						client = tmp_clients;
						break;
					}
					tmp_clients = tmp_clients->next;
				}
				if(strcmp(action, "identify") == 0) {
					/* Check if client doesn't already exist */
					if(exists == 0) {
						if((client = MALLOC(sizeof(struct clients_t))) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						client->core = 0;
						client->config = 0;
						client->receiver = 0;
						client->forward = 0;
						client->stats = 0;
						client->cpu = 0;
						client->ram = 0;
						strcpy(client->media, "all");
						client->next = NULL;
						client->id = sd;
						memset(client->uuid, '\0', sizeof(client->uuid));
					}
					if(json_find_string(json, "media", &media) == 0) {
						if(strcmp(media, "all") == 0 || strcmp(media, "mobile") == 0 ||
						   strcmp(media, "desktop") == 0 || strcmp(media, "web") == 0) {
								strcpy(client->media, media);
							 }
					} else {
						strcpy(client->media, "all");
					}
					char *t = NULL;
					if(json_find_string(json, "uuid", &t) == 0) {
						strcpy(client->uuid, t);
					}
					if((options = json_find_member(json, "options")) != NULL) {
						struct JsonNode *childs = json_first_child(options);
						while(childs) {
							if(strcmp(childs->key, "core") == 0 &&
							   childs->tag == JSON_NUMBER) {
								if((int)childs->number_ == 1) {
									client->core = 1;
								} else {
									client->core = 0;
								}
							} else if(strcmp(childs->key, "stats") == 0 &&
							   childs->tag == JSON_NUMBER) {
								if((int)childs->number_ == 1) {
									client->stats = 1;
								} else {
									client->stats = 0;
								}
							} else if(strcmp(childs->key, "receiver") == 0 &&
							   childs->tag == JSON_NUMBER) {
								if((int)childs->number_ == 1) {
									client->receiver = 1;
								} else {
									client->receiver = 0;
								}
							} else if(strcmp(childs->key, "config") == 0 &&
							   childs->tag == JSON_NUMBER) {
								if((int)childs->number_ == 1) {
									client->config = 1;
								} else {
									client->config = 0;
								}
							} else if(strcmp(childs->key, "forward") == 0 &&
							   childs->tag == JSON_NUMBER) {
								if((int)childs->number_ == 1) {
									client->forward = 1;
								} else {
									client->forward = 0;
								}
							} else {
							   error = 1;
							   break;
							}
							childs = childs->next;
						}
					}
					if(exists == 0) {
						if(error == 1) {
							FREE(client);
						} else {
							tmp_clients = clients;
							if(tmp_clients) {
								while(tmp_clients->next != NULL) {
									tmp_clients = tmp_clients->next;
								}
								tmp_clients->next = client;
							} else {
								client->next = clients;
								clients = client;
							}
						}
					}
					socket_write(sd, "{\"status\":\"success\"}");
				} else if(strcmp(action, "send") == 0) {
					if(send_queue(json, SENDER) == 0) {
						socket_write(sd, "{\"status\":\"success\"}");
					} else {
						socket_write(sd, "{\"status\":\"failed\"}");
					}
				} else if(strcmp(action, "control") == 0) {
					struct JsonNode *code = NULL;
					struct devices_t *dev = NULL;
					char *device = NULL;
					if((code = json_find_member(json, "code")) == NULL || code->tag != JSON_OBJECT) {
						logprintf(LOG_ERR, "client did not send any codes");
					} else {
						/* Check if a location and device are given */
						if(json_find_string(code, "device", &device) != 0) {
							logprintf(LOG_ERR, "client did not send a device");
						/* Check if the device and location exists in the config file */
						} else if(devices_get(device, &dev) == 0) {
							char *state = NULL;
							struct JsonNode *values = NULL;

							json_find_string(code, "state", &state);
							if((values = json_find_member(code, "values")) != NULL) {
								values = json_first_child(values);
							}

							if(control_device(dev, state, values, SENDER) == 0) {
								socket_write(sd, "{\"status\":\"success\"}");
							} else {
								socket_write(sd, "{\"status\":\"failed\"}");
							}
						} else {
							logprintf(LOG_ERR, "the device \"%s\" does not exist", device);
						}
					}
				} else if(strcmp(action, "registry") == 0) {
					struct JsonNode *value = NULL;
					char *type = NULL;
					char *key = NULL;
					char *sval = NULL;
					double nval = 0.0;
					int dec = 0;
					if(json_find_string(json, "type", &type) != 0) {
						logprintf(LOG_ERR, "client did not send a type of action");
					} else {
						if(strcmp(type, "set") == 0) {
							if(json_find_string(json, "key", &key) != 0) {
								logprintf(LOG_ERR, "client did not send a registry key");
								socket_write(sd, "{\"status\":\"failed\"}");
							} else if((value = json_find_member(json, "value")) == NULL) {
								logprintf(LOG_ERR, "client did not send a registry value");
								socket_write(sd, "{\"status\":\"failed\"}");
							} else {
								if(value->tag == JSON_NUMBER) {
									if(registry_set_number(key, value->number_, value->decimals_) == 0) {
										socket_write(sd, "{\"status\":\"success\"}");
									} else {
										socket_write(sd, "{\"status\":\"failed\"}");
									}
								} else if(value->tag == JSON_STRING) {
									if(registry_set_string(key, value->string_) == 0) {
										socket_write(sd, "{\"status\":\"success\"}");
									} else {
										socket_write(sd, "{\"status\":\"failed\"}");
									}
								} else {
									logprintf(LOG_ERR, "registry value can only be a string or number");
									socket_write(sd, "{\"status\":\"failed\"}");
								}
							}
						} else if(strcmp(type, "remove") == 0) {
							if(json_find_string(json, "key", &key) != 0) {
								logprintf(LOG_ERR, "client did not send a registry key");
								socket_write(sd, "{\"status\":\"failed\"}");
							} else {
								if(registry_remove_value(key) == 0) {
									socket_write(sd, "{\"status\":\"success\"}");
								} else {
									socket_write(sd, "{\"status\":\"failed\"}");
								}
							}
						} else if(strcmp(type, "get") == 0) {
							if(json_find_string(json, "key", &key) != 0) {
								logprintf(LOG_ERR, "client did not send a registry key");
								socket_write(sd, "{\"status\":\"failed\"}");
							} else {
								if(registry_get_number(key, &nval, &dec) == 0) {
									struct JsonNode *jsend = json_mkobject();
									json_append_member(jsend, "message", json_mkstring("registry"));
									json_append_member(jsend, "value", json_mknumber(nval, dec));
									json_append_member(jsend, "key", json_mkstring(key));
									char *output = json_stringify(jsend, NULL);
									socket_write(sd, output);
									json_free(output);
									json_delete(jsend);
								} else if(registry_get_string(key, &sval) == 0) {
									struct JsonNode *jsend = json_mkobject();
									json_append_member(jsend, "message", json_mkstring("registry"));
									json_append_member(jsend, "value", json_mkstring(sval));
									json_append_member(jsend, "key", json_mkstring(key));
									char *output = json_stringify(jsend, NULL);
									socket_write(sd, output);
									json_free(output);
									json_delete(jsend);
								} else {
									logprintf(LOG_ERR, "registry key '%s' doesn't exists", key);
									socket_write(sd, "{\"status\":\"failed\"}");
								}
							}
						}
					}
				} else if(strcmp(action, "request config") == 0) {
					struct JsonNode *jsend = json_mkobject();
					struct JsonNode *jconfig = NULL;
					if(client->forward == 1) {
						jconfig = config_print(CONFIG_FORWARD, client->media);
					} else {
						jconfig = config_print(CONFIG_INTERNAL, client->media);
					}
					json_append_member(jsend, "message", json_mkstring("config"));
					json_append_member(jsend, "config", jconfig);
					char *output = json_stringify(jsend, NULL);
					str_replace("%", "%%", &output);
					socket_write(sd, output);
					json_free(output);
					json_delete(jsend);
				} else if(strcmp(action, "request values") == 0) {
					struct JsonNode *jsend = json_mkobject();
					struct JsonNode *jvalues = devices_values(client->media);
					json_append_member(jsend, "message", json_mkstring("values"));
					json_append_member(jsend, "values", jvalues);
					char *output = json_stringify(jsend, NULL);
					socket_write(sd, output);
					json_free(output);
					json_delete(jsend);
					/* send version packet */ 
					struct JsonNode *jsend_version = json_mkobject();
					json_append_member(jsend_version, "version", json_mkstring(PILIGHT_VERSION));
					char *output_version = json_stringify(jsend_version, NULL);
					socket_write(sd, output_version);
					json_free(output_version);
					json_delete(jsend_version);

				/*
				 * Parse received codes from nodes
				 */
				} else if(strcmp(action, "update") == 0) {
					struct JsonNode *jvalues = NULL;
					char *pname = NULL;
					if((jvalues = json_find_member(json, "values")) != NULL) {
						exists = 0;
						tmp_clients = clients;
						while(tmp_clients) {
							if(tmp_clients->id == sd) {
								exists = 1;
								client = tmp_clients;
								break;
							}
							tmp_clients = tmp_clients->next;
						}
						if(exists) {
							json_find_number(jvalues, "ram", &client->ram);
							json_find_number(jvalues, "cpu", &client->cpu);
						}
					}
					if(json_find_string(json, "protocol", &pname) == 0) {
						broadcast_queue(pname, json, MASTER);
					}
				} else {
					error = 1;
				}
			} else if((json_find_string(json, "status", &status)) == 0) {
				tmp_clients = clients;
				while(tmp_clients) {
					if(tmp_clients->id == sd) {
						exists = 1;
						client = tmp_clients;
						break;
					}
					tmp_clients = tmp_clients->next;
				}
				if(strcmp(status, "success") == 0) {
					logprintf(LOG_DEBUG, "client \"%s\" successfully executed our latest request", client->uuid);
				} else if(strcmp(status, "failed") == 0) {
					logprintf(LOG_DEBUG, "client \"%s\" failed executing our latest request", client->uuid);
				}
			} else {
				error = 1;
			}
			json_delete(json);
		}
	}
	if(error == 1) {
		client_remove(sd);
		socket_close(sd);
	}
}

/* Rewrite code start */

static int socket_parse_responses(char *buffer, char *media, char **respons) {
	struct JsonNode *json = NULL;
	char *action = NULL, *status = NULL;

	if(strcmp(buffer, "HEART") == 0) {
		if((*respons = MALLOC(strlen("BEAT")+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strcpy(*respons, "BEAT");
		return 0;
	}	else {
		if(pilight.runmode != ADHOC) {
			logprintf(LOG_DEBUG, "socket recv: %s", buffer);
		}

		if(json_validate(buffer) == true) {
			json = json_decode(buffer);
			if((json_find_string(json, "status", &status)) == 0) {
				if(strcmp(status, "success") == 0) {
					if((*respons = MALLOC(strlen("{\"status\":\"success\"}")+1)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					strcpy(*respons, "{\"status\":\"success\"}");
					json_delete(json);
					return 0;
				}
			}
			if((json_find_string(json, "action", &action)) == 0) {
				if(strcmp(action, "send") == 0) {
					if(send_queue(json, ORIGIN_SENDER) == 0) {
						if((*respons = MALLOC(strlen("{\"status\":\"success\"}")+1)) == NULL) {
							OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
						}
						strcpy(*respons, "{\"status\":\"success\"}");
						json_delete(json);
						return 0;
					} else {
						if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
							OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
						}
						strcpy(*respons, "{\"status\":\"failed\"}");
						json_delete(json);
						return 0;
					}
				} else if(strcmp(action, "control") == 0) {
					struct JsonNode *code = NULL;
					char *device = NULL;
					struct devices_t *dev = NULL;
					if((code = json_find_member(json, "code")) == NULL || code->tag != JSON_OBJECT) {
						logprintf(LOG_ERR, "client did not send any codes");
						json_delete(json);
						return -1;
					} else {
						/* Check if a location and device are given */
						if(json_find_string(code, "device", &device) != 0) {
							logprintf(LOG_ERR, "client did not send a device");
							json_delete(json);
							return -1;
						/* Check if the device and location exists in the config file */
						} else if(devices_get(device, &dev) == 0) {
							char *state = NULL;
							struct JsonNode *values = NULL;

							json_find_string(code, "state", &state);
							if((values = json_find_member(code, "values")) != NULL) {
								values = json_first_child(values);
							}

							if(control_device(dev, state, values, ORIGIN_SENDER) == 0) {
								if((*respons = MALLOC(strlen("{\"status\":\"success\"}")+1)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								strcpy(*respons, "{\"status\":\"success\"}");
								json_delete(json);
								return 0;
							} else {
								if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								strcpy(*respons, "{\"status\":\"failed\"}");
								json_delete(json);
								return 0;
							}
						} else {
							logprintf(LOG_ERR, "the device \"%s\" does not exist", device);
							json_delete(json);
							return -1;
						}
					}
				} else if(strcmp(action, "registry") == 0) {
					struct JsonNode *value = NULL;
					char *type = NULL;
					char *key = NULL;
					char *sval = NULL;
					double nval = 0.0;
					int dec = 0;
					if(json_find_string(json, "type", &type) != 0) {
						logprintf(LOG_ERR, "client did not send a type of action");
						json_delete(json);
						return -1;
					} else {
						if(strcmp(type, "set") == 0) {
							if(json_find_string(json, "key", &key) != 0) {
								logprintf(LOG_ERR, "client did not send a registry key");
								if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								strcpy(*respons, "{\"status\":\"failed\"}");
								json_delete(json);
								return 0;
							} else if((value = json_find_member(json, "value")) == NULL) {
								logprintf(LOG_ERR, "client did not send a registry value");
								if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								strcpy(*respons, "{\"status\":\"failed\"}");
								json_delete(json);
								return 0;
							} else {
								if(value->tag == JSON_NUMBER) {
#ifdef PILIGHT_REWRITE
									if(registry_update(ORIGIN_MASTER, key, json_mknumber(value->number_, value->decimals_)) == 0) {
#else
									if(registry_set_number(key, value->number_, value->decimals_) == 0) {
#endif
										if((*respons = MALLOC(strlen("{\"status\":\"success\"}")+1)) == NULL) {
											OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
										}
										strcpy(*respons, "{\"status\":\"success\"}");
										json_delete(json);
										return 0;
									} else {
										if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
											OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
										}
										strcpy(*respons, "{\"status\":\"failed\"}");
										json_delete(json);
										return 0;
									}
								} else if(value->tag == JSON_STRING) {
#ifdef PILIGHT_REWRITE
									if(registry_update(ORIGIN_MASTER, key, json_mkstring(value->string_)) == 0) {
#else
									if(registry_set_string(key, value->string_) == 0) {
#endif
										if((*respons = MALLOC(strlen("{\"status\":\"success\"}")+1)) == NULL) {
											OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
										}
										strcpy(*respons, "{\"status\":\"success\"}");
										json_delete(json);
										return 0;
									} else {
										if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
											OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
										}
										strcpy(*respons, "{\"status\":\"failed\"}");
										json_delete(json);
										return 0;
									}
								} else {
									logprintf(LOG_ERR, "registry value can only be a string or number");
									if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
										OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
									}
									strcpy(*respons, "{\"status\":\"failed\"}");
									json_delete(json);
									return 0;
								}
							}
						} else if(strcmp(type, "remove") == 0) {
							if(json_find_string(json, "key", &key) != 0) {
								logprintf(LOG_ERR, "client did not send a registry key");
								if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								strcpy(*respons, "{\"status\":\"failed\"}");
								json_delete(json);
								return 0;
							} else {
#ifdef PILIGHT_REWRITE
								if(registry_delete(ORIGIN_MASTER, key) == 0) {
#else
								if(registry_remove_value(key) == 0) {
#endif
									if((*respons = MALLOC(strlen("{\"status\":\"success\"}")+1)) == NULL) {
										OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
									}
									strcpy(*respons, "{\"status\":\"success\"}");
									json_delete(json);
									return 0;
								} else {
									logprintf(LOG_ERR, "registry value can only be a string or number");
									if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
										OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
									}
									strcpy(*respons, "{\"status\":\"failed\"}");
									json_delete(json);
									return 0;
								}
							}
						} else if(strcmp(type, "get") == 0) {
							if(json_find_string(json, "key", &key) != 0) {
								logprintf(LOG_ERR, "client did not send a registry key");
								if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								strcpy(*respons, "{\"status\":\"failed\"}");
								json_delete(json);
								return 0;
							} else {
#ifdef PILIGHT_REWRITE
								if(registry_select_number(ORIGIN_MASTER, key, &nval, &dec) == 0) {
#else
								if(registry_get_number(key, &nval, &dec) == 0) {
#endif
									struct JsonNode *jsend = json_mkobject();
									json_append_member(jsend, "message", json_mkstring("registry"));
									json_append_member(jsend, "value", json_mknumber(nval, dec));
									json_append_member(jsend, "key", json_mkstring(key));
									char *output = json_stringify(jsend, NULL);
									if((*respons = MALLOC(strlen(output)+1)) == NULL) {
										OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
									}
									strcpy(*respons, output);
									json_free(output);
									json_delete(jsend);
									json_delete(json);
									return 0;
#ifdef PILIGHT_REWRITE
								} else if(registry_select_string(ORIGIN_MASTER, key, &sval) == 0) {
#else
								} else if(registry_get_string(key, &sval) == 0) {
#endif
									struct JsonNode *jsend = json_mkobject();
									json_append_member(jsend, "message", json_mkstring("registry"));
									json_append_member(jsend, "value", json_mkstring(sval));
									json_append_member(jsend, "key", json_mkstring(key));
									char *output = json_stringify(jsend, NULL);
									if((*respons = MALLOC(strlen(output)+1)) == NULL) {
										OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
									}
									strcpy(*respons, output);
									json_free(output);
									json_delete(jsend);
									json_delete(json);
									return 0;
								} else {
									logprintf(LOG_ERR, "registry key '%s' does not exist", key);
									if((*respons = MALLOC(strlen("{\"status\":\"failed\"}")+1)) == NULL) {
										OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
									}
									strcpy(*respons, "{\"status\":\"failed\"}");
									json_delete(json);
									return 0;
								}
							}
						}
					}
				} else if(strcmp(action, "request config") == 0) {
					struct JsonNode *jsend = json_mkobject();
					struct JsonNode *jconfig = NULL;
					jconfig = config_print(CONFIG_INTERNAL, media);
					json_append_member(jsend, "message", json_mkstring("config"));
					json_append_member(jsend, "config", jconfig);
					char *output = json_stringify(jsend, NULL);
					str_replace("%", "%%", &output);
					if((*respons = MALLOC(strlen(output)+1)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					strcpy(*respons, output);
					json_free(output);
					json_delete(jsend);
					json_delete(json);
					return 0;
				} else if(strcmp(action, "request values") == 0) {
					struct JsonNode *jsend = json_mkobject();
#ifdef PILIGHT_REWRITE
					struct JsonNode *jvalues = values_print(media);
#else
					JsonNode *jvalues = devices_values(media);
#endif
					json_append_member(jsend, "message", json_mkstring("values"));
					json_append_member(jsend, "values", jvalues);
					char *output = json_stringify(jsend, NULL);
					if((*respons = MALLOC(strlen(output)+1)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					strcpy(*respons, output);
					json_free(output);
					json_delete(jsend);
					json_delete(json);
					return 0;
				}
			} else {
				json_delete(json);
				return -1;
			}
		}
	}
	return -1;
}

static void *socket_parse_data1(int reason, void *param) {
	struct reason_socket_received_t *data = param;

	struct sockaddr_in address;
	socklen_t socklen = sizeof(address);

	struct JsonNode *json = NULL;
	struct JsonNode *options = NULL;
	struct clients_t *tmp_clients = NULL;
	struct clients_t *client = NULL;
	char *action = NULL, *media = NULL, *status = NULL, *respons = NULL;
	char all[] = "all";
	int error = 0, exists = 0, sd = -1;

	if(strlen(data->type) == 0) {
		logprintf(LOG_ERR, "socket data misses a socket type");
	}

	if(data->buffer == NULL) {
		logprintf(LOG_ERR, "socket message incorrectly formatted");
		return NULL;
	}

	sd = data->fd;
	getpeername(sd, (struct sockaddr*)&address, &socklen);

	tmp_clients = clients;
	while(tmp_clients) {
		if(tmp_clients->id == sd) {
			exists = 1;
			client = tmp_clients;
			break;
		}
		tmp_clients = tmp_clients->next;
	}
	if(exists == 0) {
		media = all;
	} else {
		media = client->media;
	}

	if(json_validate(data->buffer) == true) {
		json = json_decode(data->buffer);
		if((json_find_string(json, "action", &action)) == 0) {
			if(strcmp(action, "identify") == 0) {
				/* Check if client doesn't already exist */
				if(exists == 0) {
					if((client = MALLOC(sizeof(struct clients_t))) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					client->core = 0;
					client->config = 0;
					client->receiver = 0;
					client->forward = 0;
					client->stats = 0;
					client->cpu = 0;
					strcpy(client->media, "all");
					client->next = NULL;
					client->id = sd;
					memset(client->uuid, '\0', sizeof(client->uuid));
				}
				if(json_find_string(json, "media", &media) == 0) {
					if(strcmp(media, "all") == 0 || strcmp(media, "mobile") == 0 ||
						 strcmp(media, "desktop") == 0 || strcmp(media, "web") == 0) {
						strcpy(client->media, media);
					}
				} else {
					strcpy(client->media, "all");
				}
				char *t = NULL;
				if(json_find_string(json, "uuid", &t) == 0) {
					strcpy(client->uuid, t);
				}
				if((options = json_find_member(json, "options")) != NULL) {
					struct JsonNode *childs = json_first_child(options);
					while(childs) {
						if(strcmp(childs->key, "core") == 0 &&
							 childs->tag == JSON_NUMBER) {
							if((int)childs->number_ == 1) {
								client->core = 1;
							} else {
								client->core = 0;
							}
						} else if(strcmp(childs->key, "stats") == 0 &&
							 childs->tag == JSON_NUMBER) {
							if((int)childs->number_ == 1) {
								client->stats = 1;
							} else {
								client->stats = 0;
							}
						} else if(strcmp(childs->key, "receiver") == 0 &&
							 childs->tag == JSON_NUMBER) {
							if((int)childs->number_ == 1) {
								client->receiver = 1;
							} else {
								client->receiver = 0;
							}
						} else if(strcmp(childs->key, "config") == 0 &&
							 childs->tag == JSON_NUMBER) {
							if((int)childs->number_ == 1) {
								client->config = 1;
							} else {
								client->config = 0;
							}
						} else if(strcmp(childs->key, "forward") == 0 &&
							 childs->tag == JSON_NUMBER) {
							if((int)childs->number_ == 1) {
								client->forward = 1;
							} else {
								client->forward = 0;
							}
						} else {
						 error = 1;
						 break;
						}
						childs = childs->next;
					}
				}
				if(exists == 0) {
					if(error == -1) {
						FREE(client);
					} else {
						tmp_clients = clients;
						if(tmp_clients) {
							while(tmp_clients->next != NULL) {
								tmp_clients = tmp_clients->next;
							}
							tmp_clients->next = client;
						} else {
							client->next = clients;
							clients = client;
						}
						socket_write(sd, "{\"status\":\"success\"}");
						json_delete(json);
						return NULL;
					}
				}
				json_delete(json);
				return NULL;
			/*
			 * Parse received codes from nodes
			 */
			} else if(strcmp(action, "update") == 0) {
				if(exists == 1) {
					struct JsonNode *jvalues = NULL;
					char *pname = NULL;
					if((jvalues = json_find_member(json, "values")) != NULL) {
						exists = 0;
						tmp_clients = clients;
						while(tmp_clients) {
							if(tmp_clients->id == sd) {
								exists = 1;
								client = tmp_clients;
								break;
							}
							tmp_clients = tmp_clients->next;
						}
						if(exists == 1) {
							json_find_number(jvalues, "cpu", &client->cpu);
						}
					}
					if(json_find_string(json, "protocol", &pname) == 0) {
						char *out = MALLOC(strlen(data->buffer)+1);
						if(out == NULL) {
							OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
						}
						strcpy(out, data->buffer);
						eventpool_trigger(REASON_FORWARD, reason_forward_free, out);
						json_delete(json);
						return NULL;
					}
				}
				json_delete(json);
				return NULL;
			} else if((json_find_string(json, "status", &status)) == 0) {
				tmp_clients = clients;
				while(tmp_clients) {
					if(tmp_clients->id == sd) {
						exists = 1;
						client = tmp_clients;
						break;
					}
					tmp_clients = tmp_clients->next;
				}
				if(strcmp(status, "success") == 0) {
					logprintf(LOG_DEBUG, "client \"%s\" successfully executed our latest request", client->uuid);
					json_delete(json);
					return NULL;
				} else if(strcmp(status, "failed") == 0) {
					logprintf(LOG_DEBUG, "client \"%s\" failed executing our latest request", client->uuid);
					json_delete(json);
					return NULL;
				}

				json_delete(json);
				return NULL;
			}
		}
	}

	if(socket_parse_responses(data->buffer, media, &respons) == 0) {
		struct reason_socket_send_t *data1 = MALLOC(sizeof(struct reason_socket_send_t));
		if(data1 == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		data1->fd = sd;
		if((data1->buffer = MALLOC(strlen(respons)+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/;
		}
		strcpy(data1->buffer, respons);
		strncpy(data1->type, data->type, 255);

		eventpool_trigger(REASON_SOCKET_SEND, reason_socket_send_free, data1);

		FREE(respons);
		json_delete(json);
		return NULL;
	} else {
		logprintf(LOG_ERR, "could not parse response to: %s", data->buffer);
		client_remove(sd);
		socket_close(sd);
		json_delete(json);
		return NULL;
	}
	json_delete(json);
	return NULL;
}
/* Rewrite code end */

static void socket_client_disconnected(int i) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	client_remove(socket_get_clients(i));
}

void *receivePulseTrain(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct rawcode_t r;
	r.length = 0;
	int plslen = 0;
#ifdef _WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#else
	/* Make sure the pilight receiving gets
	   the highest priority available */
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = 70;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched);
#endif

	struct hardware_t *hw = (hardware_t *)param;
	pthread_mutex_lock(&hw->lock);
	hw->running = 1;

	while(main_loop == 1 && hw->receivePulseTrain != NULL && hw->stop == 0) {
		if(hw->wait == 0) {
			pthread_mutex_lock(&hw->lock);
			logprintf(LOG_STACK, "%s::unlocked", __FUNCTION__);

			hw->receivePulseTrain(&r);
			plslen = r.pulses[r.length-1]/PULSE_DIV;
			if(r.length > 0) {
				receive_queue(r.pulses, r.length, plslen, hw->hwtype);
			} else if(r.length == -1) {
				hw->init();
				sleep(1);
			}

			pthread_mutex_unlock(&hw->lock);
		} else {
			pthread_cond_wait(&hw->signal, &hw->lock);
		}
	}
	hw->running = 0;
	return (void *)NULL;
}

#ifdef PILIGHT_DEVELOPMENT
// void *receiveOOK(void *param) {
	// logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	// struct rawcode_t r;
	// r.length = 0;
	// int plslen = 0, duration = 0;
	// struct timeval tp;
	// struct timespec ts;

// #ifdef _WIN32
	// SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
// #else
	// /* Make sure the pilight receiving gets
	   // the highest priority available */
	// struct sched_param sched;
	// memset(&sched, 0, sizeof(sched));
	// sched.sched_priority = 70;
	// pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched);
// #endif

	// struct hardware_t *hw = (hardware_t *)param;
	// pthread_mutex_lock(&hw->lock);
	// hw->running = 1;

	// while(main_loop == 1 && hw->receiveOOK != NULL && hw->stop == 0) {
		// if(hw->wait == 0) {
			// pthread_mutex_lock(&hw->lock);
			// logprintf(LOG_STACK, "%s::unlocked", __FUNCTION__);
			// duration = hw->receiveOOK();
			// if(duration > 0) {
				// r.pulses[r.length++] = duration;
				// if(r.length > MAXPULSESTREAMLENGTH-1) {
					// r.length = 0;
				// }
				// if(duration > hw->mingaplen) {
					// if(duration < hw->maxgaplen) {
						// plslen = duration/PULSE_DIV;
					// }
					// /* Let's do a little filtering here as well */
					// if(r.length >= hw->minrawlen && r.length <= hw->maxrawlen) {
						// receive_queue(r.pulses, r.length, plslen, hw->hwtype);
					// }
					// r.length = 0;
				// }
			// /* Hardware failure */
			// } else if(duration == -1) {
				// pthread_mutex_unlock(&hw->lock);
				// gettimeofday(&tp, NULL);
				// ts.tv_sec = tp.tv_sec;
				// ts.tv_nsec = tp.tv_usec * 1000;
				// ts.tv_sec += 1;
				// pthread_mutex_lock(&hw->lock);
				// pthread_cond_timedwait(&hw->signal, &hw->lock, &ts);
			// }
			// pthread_mutex_unlock(&hw->lock);
		// } else {
			// pthread_cond_wait(&hw->signal, &hw->lock);
		// }
	// }
	// hw->running = 0;
	// return (void *)NULL;
// }
#endif

static void *receivePulseTrain1(int reason, void *param) {
	struct reason_received_pulsetrain_t *data = param;
	int plslen = 0;

	if(data->hardware != NULL && data->pulses != NULL && data->length > 0) {
#ifndef PILIGHT_REWRITE
		int hwtype = 0;
		struct conf_hardware_t *tmp_confhw = conf_hardware;
		while(tmp_confhw) {
			if(strcmp(tmp_confhw->hardware->id, data->hardware) == 0) {
				hwtype = tmp_confhw->hardware->hwtype;
			}
			tmp_confhw = tmp_confhw->next;
		}
#endif
#ifdef PILIGHT_REWRITE
		struct hardware_t *hw = NULL;
		if(hardware_select_struct(ORIGIN_MASTER, data->hardware, &hw) == 0) {
#endif
			plslen = data->pulses[data->length-1]/PULSE_DIV;
			if(data->length > 0) {
#ifdef PILIGHT_REWRITE
				receive_parse_code(data->pulses, data->length, plslen, hw->hwtype);
#else
				receive_queue(data->pulses, data->length, plslen, hwtype);
#endif
			}
#ifdef PILIGHT_REWRITE
		}
#endif
	}

	return (void *)NULL;
}

void *clientize(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	adhoc_pending = 1;
	struct ssdp_list_t *ssdp_list = NULL;
	struct JsonNode *json = NULL;
	struct JsonNode *joptions = NULL;
	struct JsonNode *jchilds = NULL;
	struct JsonNode *tmp = NULL;
  char *recvBuff = NULL, *output = NULL;
	char *message = NULL, *action = NULL;
	char *origin = NULL, *protocol = NULL;
	int client_loop = 0, config_synced = 0;

	while(main_loop) {

		if(client_loop == 1) {
			logprintf(LOG_NOTICE, "connection to main pilight daemon lost");
			logprintf(LOG_NOTICE, "trying to reconnect...");
			sleep(1);
		}

		client_loop = 1;
		config_synced = 0;

		ssdp_list = NULL;
		if(master_server != NULL && master_port > 0) {
			if((sockfd = socket_connect(master_server, master_port)) == -1) {
				logprintf(LOG_ERR, "could not connect to pilight-daemon");
				continue;
			}
		} else if(ssdp_seek(&ssdp_list) == -1) {
			logprintf(LOG_NOTICE, "no pilight ssdp connections found");
			continue;
		} else {
			if((sockfd = socket_connect(ssdp_list->ip, ssdp_list->port)) == -1) {
				logprintf(LOG_ERR, "could not connect to pilight-daemon");
				continue;
			}
		}
		if(ssdp_list) {
			ssdp_free(ssdp_list);
		}

		json = json_mkobject();
		joptions = json_mkobject();
		json_append_member(json, "action", json_mkstring("identify"));
		json_append_member(joptions, "receiver", json_mknumber(1, 0));
		json_append_member(joptions, "forward", json_mknumber(1, 0));
		json_append_member(joptions, "config", json_mknumber(1, 0));
		json_append_member(json, "uuid", json_mkstring(pilight_uuid));
		json_append_member(json, "options", joptions);
		output = json_stringify(json, NULL);
		if(socket_write(sockfd, output) != (strlen(output)+strlen(EOSS))) {
			json_free(output);
			json_delete(json);
			continue;
		}
		json_free(output);
		json_delete(json);

		if(socket_read(sockfd, &recvBuff, 1) != 0
		   || strcmp(recvBuff, "{\"status\":\"success\"}") != 0) {
			continue;
		}
		logprintf(LOG_DEBUG, "socket recv: %s", recvBuff);

		json = json_mkobject();
		json_append_member(json, "action", json_mkstring("request config"));
		output = json_stringify(json, NULL);
		if(socket_write(sockfd, output) != (strlen(output)+strlen(EOSS))) {
			json_free(output);
			json_delete(json);
			continue;
		}
		json_free(output);
		json_delete(json);

		if(socket_read(sockfd, &recvBuff, 0) == 0) {
			logprintf(LOG_DEBUG, "socket recv: %s", recvBuff);
			if(json_validate(recvBuff) == true) {
				json = json_decode(recvBuff);
				if(json_find_string(json, "message", &message) == 0) {
					if(strcmp(message, "config") == 0) {
						struct JsonNode *jconfig = NULL;
						if((jconfig = json_find_member(json, "config")) != NULL) {

							pthread_mutex_lock(&config_lock);
							gui_gc();
							devices_gc();
#ifdef EVENTS
							rules_gc();
#endif
							registry_gc();
							pthread_mutex_unlock(&config_lock);

							int match = 1;
							while(match) {
								jchilds = json_first_child(jconfig);
								match = 0;
								while(jchilds) {
									tmp = jchilds;
									if(strcmp(tmp->key, "devices") != 0) {
										json_remove_from_parent(tmp);
										match = 1;
									}
									jchilds = jchilds->next;
									if(match == 1) {
										json_delete(tmp);
									}
								}
							}
							if(config_parse(jconfig) == EXIT_SUCCESS) {
								logprintf(LOG_DEBUG, "loaded master configuration");
								config_synced = 1;
							} else {
								logprintf(LOG_WARNING, "failed to load master configuration");
							}
						}
					}
				}
				json_delete(json);
			}
		}

		while(client_loop && config_synced) {
			if(sockfd <= 0) {
				break;
			}
			if(main_loop == 0) {
				client_loop = 0;
				break;
			}

			int n = socket_read(sockfd, &recvBuff, 1);
			if(n == -1) {
				sockfd = 0;
				break;
			} else if(n == 1) {
				continue;
			}

			logprintf(LOG_DEBUG, "socket recv: %s", recvBuff);
			char **array = NULL;
			unsigned int z = explode(recvBuff, "\n", &array), q = 0;
			for(q=0;q<z;q++) {
				if(json_validate(array[q]) == true) {
					json = json_decode(array[q]);
					if(json_find_string(json, "action", &action) == 0) {
						if(strcmp(action, "send") == 0 ||
						   strcmp(action, "control") == 0) {
							socket_parse_data(sockfd, array[q]);
						}
					} else if(json_find_string(json, "origin", &origin) == 0 &&
							json_find_string(json, "protocol", &protocol) == 0) {
							if(strcmp(origin, "receiver") == 0 ||
								 strcmp(origin, "sender") == 0) {
								broadcast_queue(protocol, json, NODE);
						}
					}
					json_delete(json);
				}
			}
			array_free(&array, z);
		}
	}

	if(recvBuff != NULL) {
		FREE(recvBuff);
	}

	adhoc_pending = 0;
	return NULL;
}

#ifndef _WIN32
static void save_pid(pid_t npid) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int f = 0;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	if((f = open(pid_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) != -1) {
		lseek(f, 0, SEEK_SET);
		sprintf(buffer, "%d", npid);
		ssize_t i = write(f, buffer, strlen(buffer));
		if(i != strlen(buffer)) {
			logprintf(LOG_WARNING, "could not store pid in %s", pid_file);
		}
	}
	close(f);
}

static void daemonize(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	log_file_enable();
	log_shell_disable();
	/* Get the pid of the fork */

	pid_t npid = fork();
	switch(npid) {
		case 0:
		break;
		case -1:
			logprintf(LOG_ERR, "could not daemonize program");
			exit(1);
		break;
		default:
			save_pid(npid);
			logprintf(LOG_INFO, "daemon started with pid: %d", npid);
			exit(0);
		break;
	}
}
#endif

static void close_cb(uv_handle_t *handle) {
	FREE(handle);
}

static void walk_cb(uv_handle_t *handle, void *arg) {
	if(!uv_is_closing(handle)) {
		uv_close(handle, close_cb);
	}
}

/* Garbage collector of main program */
int main_gc(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	running = 0;
	pilight.running = 0;
	main_loop = 0;

	while(adhoc_pending == 1) {
		sleep(1);
	}

	/* If we are running in node mode, the clientize
	   thread is waiting for a response from the main
	   daemon. This means we can't gracefull stop that
	   thread. However, by sending a HEART message the
	   main daemon will response with a BEAT. This allows
	   us to stop the socket_read function and properly
	   stop the clientize thread. */
	if(pilight.runmode == ADHOC) {
		socket_write(sockfd, "HEART");
	}

	pilight.runmode = STANDALONE;

	while(sending) {
		usleep(1000);
	}

#ifdef EVENTS
	events_gc();
#endif

	if(recvqueue_init == 1) {
		pthread_mutex_unlock(&recvqueue_lock);
		pthread_cond_signal(&recvqueue_signal);
		usleep(1000);
	}

	if(sendqueue_init == 1) {
		pthread_mutex_unlock(&sendqueue_lock);
		pthread_cond_signal(&sendqueue_signal);
	}

	if(bcqueue_init == 1) {
		pthread_mutex_unlock(&bcqueue_lock);
		pthread_cond_signal(&bcqueue_signal);
	}

	struct clients_t *tmp_clients;
	while(clients) {
		tmp_clients = clients;
		clients = clients->next;
		FREE(tmp_clients);
	}
	if(clients != NULL) {
		FREE(clients);
	}

#ifndef _WIN32
	if(running == 0) {
		/* Remove the stale pid file */
		if(access(pid_file, F_OK) != -1) {
			if(remove(pid_file) != -1) {
				logprintf(LOG_INFO, "removed stale pid_file %s", pid_file);
			} else {
				logprintf(LOG_ERR, "could not remove stale pid file %s", pid_file);
			}
		}
	}

	if(pid_file_free) {
		FREE(pid_file);
	}
#endif
	eventpool_gc();
#ifdef WEBSERVER
	if(webserver_enable == 1) {
		webserver_gc();
	}
	if(webserver_root_free == 1) {
		FREE(webserver_root);
	}
#endif

	if(master_server != NULL) {
		FREE(master_server);
	}

	datetime_gc();
	ssdp_gc();
	options_gc();
	socket_gc();

	pthread_mutex_lock(&config_lock);
	config_gc();
	pthread_mutex_unlock(&config_lock);

	protocol_gc();
	ntp_gc();
	whitelist_free();
	threads_gc();
#ifndef _WIN32
	wiringXGC();
#endif
	dso_gc();
	log_gc();
	ssl_gc();
	plua_gc();
	if(configtmp != NULL) {
		FREE(configtmp);
	}

	uv_stop(uv_default_loop());
	gc_clear();
	FREE(progname);
	xfree();

#ifdef _WIN32
	WSACleanup();
	if(console == 1) {
		FreeConsole();
	}
#endif

	FREE(signal_req);

	return 0;
}

static void procProtocolInit(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	protocol_register(&procProtocol);
	protocol_set_id(procProtocol, "process");
	protocol_device_add(procProtocol, "process", "pilight proc. API");
	procProtocol->devtype = PROCESS;
	procProtocol->hwtype = API;
	procProtocol->multipleId = 0;
	procProtocol->config = 0;

	options_add(&procProtocol->options, 'c', "cpu", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&procProtocol->options, 'r', "ram", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
}

#ifndef _WIN32
#pragma GCC diagnostic push  // require GCC 4.6
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
void registerVersion(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	registry_remove_value("pilight.version");
	registry_set_string("pilight.version.current", (char *)PILIGHT_VERSION);
}
#ifndef _WIN32
#pragma GCC diagnostic pop   // require GCC 4.6
#endif

#ifdef _WIN32
void closeconsole(void) {
	log_shell_disable();
	verbosity = oldverbosity;
	FreeConsole();
	console = 0;
}

BOOL CtrlHandler(DWORD fdwCtrlType) {
	closeconsole();
	return TRUE;
}

void openconsole(void) {
	DWORD lpMode;
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	HWND hWnd = GetConsoleWindow();
	if(hWnd != NULL) {
		GetConsoleMode(hWnd, &lpMode);
		SetConsoleMode(hWnd, lpMode & ~ENABLE_PROCESSED_INPUT);
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
		HMENU hMenu = GetSystemMenu(hWnd, FALSE);
		if(hMenu != NULL) {
			RemoveMenu(hMenu, SC_CLOSE, MF_GRAYED);
			RemoveMenu(hMenu, SC_MINIMIZE, MF_GRAYED);
			RemoveMenu(hMenu, SC_MAXIMIZE, MF_GRAYED);
		}

		console = 1;
		oldverbosity = verbosity;
		verbosity = LOG_DEBUG;
		log_level_set(verbosity);
		log_shell_enable();
	}
}
#endif

static void pilight_abort(uv_timer_t *timer_req) {
	exit(EXIT_FAILURE);
}

static void pilight_stats(uv_timer_t *timer_req) {
	int watchdog = 1, stats = 1;
	// double itmp = 0.0;
	settings_find_number("watchdog-enable", &watchdog);
	settings_find_number("stats-enable", &stats);

	if(pilight.runmode == STANDALONE) {
		registerVersion();
	}

	if(stats == 1) {
		double cpu = 0.0;
		cpu = getCPUUsage();
		if(watchdog == 1 && (cpu > 90)) {
			logprintf(LOG_CRIT, "cpu usage too high %f%%, will abort when this persists", cpu);
		} else {
			if(watchdog == 1 && stats == 1 && timer_abort_req != NULL) {
				uv_timer_stop(timer_abort_req);
				uv_timer_start(timer_abort_req, pilight_abort, 9000, -1);
			}

			procProtocol->message = json_mkobject();
			struct JsonNode *code = json_mkobject();
			json_append_member(code, "cpu", json_mknumber(cpu, 16));
			logprintf(LOG_DEBUG, "cpu: %f%%", cpu);
			json_append_member(procProtocol->message, "values", code);
			json_append_member(procProtocol->message, "origin", json_mkstring("core"));
			json_append_member(procProtocol->message, "type", json_mknumber(PROCESS, 0));
			struct clients_t *tmp_clients = clients;
			while(tmp_clients) {
				if(tmp_clients->cpu > 0 && tmp_clients->ram > 0) {
					logprintf(LOG_DEBUG, "- client: %s cpu: %f%%",
								tmp_clients->uuid, tmp_clients->cpu);
				}
				tmp_clients = tmp_clients->next;
			}
			pilight.broadcast(procProtocol->id, procProtocol->message, STATS);
			json_delete(procProtocol->message);
			procProtocol->message = NULL;
		}
	}
	return;
}

static void signal_cb(uv_signal_t *handle, int signum) {
	logprintf(LOG_INFO, "Interrupt signal received. Please wait while pilight is shutting down");

	if(config_get_file() != NULL) {
		if(pilight.runmode == STANDALONE) {
			config_write(1, "all");
		}
	}

	main_gc();	
	uv_stop(uv_default_loop());
}

int start_pilight(int argc, char **argv) {
	const uv_thread_t pth_cur_id = uv_thread_self();
	memcpy((void *)&pth_main_id, &pth_cur_id, sizeof(uv_thread_t));

	struct options_t *options = NULL;
	struct ssdp_list_t *ssdp_list = NULL;

	char buffer[BUFFER_SIZE];
	int verbosity_changed = 0, show_default = 0, show_version = 0, show_help = 0;
#ifndef _WIN32
	int f = 0;
#endif
	char *stmp = NULL, *args = NULL, *p = NULL;
	int port = 0;

	if((progname = MALLOC(16)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(progname, "pilight-daemon");

	{
		int nr = sizeof(signals)/sizeof(signals[0]), i = 0;
		if((signal_req = MALLOC(sizeof(uv_signal_t *)*nr)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		for(i=0;i<nr;i++) {
			if((signal_req[i] = MALLOC(sizeof(uv_signal_t))) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}

			uv_signal_init(uv_default_loop(), signal_req[i]);
			uv_signal_start(signal_req[i], signal_cb, signals[i]);
		}
	}

	if((configtmp = MALLOC(strlen(CONFIG_FILE)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(configtmp, CONFIG_FILE);

	options_add(&options, 'H', "help", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'V', "version", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'F', "foreground", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'C', "config", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 'S', "server", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
	options_add(&options, 'P', "port", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, "[0-9]{1,4}");
	options_add(&options, 'D', "debug", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 256, "stacktracer", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 257, "threadprofiler", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);
	options_add(&options, 258, "debuglevel", OPTION_HAS_VALUE, 0, JSON_NULL, NULL, "[0-2]{1}");
	// options_add(&options, 258, "memory-tracer", OPTION_NO_VALUE, 0, JSON_NULL, NULL, NULL);

	while(1) {
		int c = options_parse(&options, argc, argv, 1, &args);
		if(c == -1) {
			break;
		}
		if(c == -2) {
			show_help = 1;
			break;
		}
		switch(c) {
			case 'H':
				show_help = 1;
			break;
			case 'V':
				show_version = 1;
			break;
			case 'C':
				configtmp = REALLOC(configtmp, strlen(args)+1);
				strcpy(configtmp, args);
			break;
			case 'S':
				if((master_server = MALLOC(strlen(args)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(master_server, args);
			break;
			case 'P':
				master_port = (unsigned short)atoi(args);
			break;
			case 'D':
				nodaemon = 1;
				verbosity = LOG_DEBUG;
				verbosity_changed = 1;
			break;
			case 'F':
				nodaemon = 1;
			break;
			case 257:
				threadprofiler = 1;
				verbosity = LOG_ERR;
				verbosity_changed = 1;
				nodaemon = 1;
			break;
			case 256:
				verbosity = LOG_STACK;
				verbosity_changed = 1;
				stacktracer = 1;
				nodaemon = 1;
			break;
			case 258:
				pilight.debuglevel = atoi(args);
				nodaemon = 1;
				verbosity = LOG_DEBUG;
				verbosity_changed = 1;
			break;
			default:
				show_default = 1;
			break;
		}
	}
	options_delete(options);
	if(show_help == 1) {
		char help[1024];
		char tabs[5];
#ifdef _WIN32
		strcpy(tabs, "\t");
#else
		strcpy(tabs, "\t\t");
#endif
		sprintf(help, "Usage: %s [options]\n"
									"\t -H --help\t\t\tdisplay usage summary\n"
									"\t -V --version\t%sdisplay version\n"
									"\t -C --config\t%sconfig file\n"
									"\t -S --server=x.x.x.x\t\tconnect to server address\n"
									"\t -P --port=xxxx\t%sconnect to server port\n"
									"\t -F --foreground\t\tdo not daemonize\n"
									"\t -D --debug\t%sdo not daemonize and\n"
									"\t\t\t%sshow debug information\n"
									"\t    --stacktracer\t\tshow internal function calls\n"
									"\t    --threadprofiler\t\tshow per thread cpu usage\n"
									"\t    --debuglevel\t\tshow additional development info\n",
									progname, tabs, tabs, tabs, tabs, tabs);
#ifdef _WIN32
		MessageBox(NULL, help, "pilight :: info", MB_OK);
#else
		printf("%s", help);
#if defined(__arm__) || defined(__mips__)
		printf("\n\tThe following GPIO platforms are supported:\n");
		char **out = NULL;
		int z = 0, i = wiringXSupportedPlatforms(&out);

		printf("\t- none\n");
		for(z=0;z<i;z++) {
			printf("\t- %s\n", out[z]);
			free(out[z]);
		}
		free(out);
		printf("\n");
#endif
#endif
		goto clear;
	}
	if(show_version == 1) {
#ifdef HASH
	#ifdef _WIN32
			char version[50];
			snprintf(version, 50, "%s version %s", progname, HASH);
			MessageBox(NULL, version, "pilight :: info", MB_OK);
	#else
			printf("%s version %s\n", progname, HASH);
	#endif
#else
	#ifdef _WIN32
			char version[50];
			snprintf(version, 50, "%s version %s", progname, PILIGHT_VERSION);
			MessageBox(NULL, version, "pilight :: info", MB_OK);
	#else
			printf("%s version %s\n", progname, PILIGHT_VERSION);
	#endif
#endif
		goto clear;
	}
	if(show_default == 1) {
#ifdef _WIN32
		char def[50];
		snprintf(def, 50, "Usage: %s [options]\n", progname);
		MessageBox(NULL, def, "pilight :: info", MB_OK);
#else
		printf("Usage: %s [options]\n", progname);
#endif
		goto clear;
	}

#ifdef _WIN32
	if(nodaemon == 1) {
		openconsole();
		console = 1;
	}
#endif

	datetime_init();
	atomicinit();
	procProtocolInit();

#ifndef _WIN32
	if(geteuid() != 0) {
		printf("%s requires root privileges in order to run\n", progname);
		FREE(progname);
		exit(EXIT_FAILURE);
	}
#endif

	// /* Run main garbage collector when quiting the daemon */
	// gc_attach(main_gc);

	// /* Catch all exit signals for gc */
	// gc_catch();

	int nrdevs = 0, x = 0;
	char **devs = NULL;
	if((nrdevs = inetdevs(&devs)) > 0) {
		for(x=0;x<nrdevs;x++) {
			if((p = genuuid(devs[x])) == NULL) {
				logprintf(LOG_ERR, "could not generate the device uuid");
			} else {
				strcpy(pilight_uuid, p);
				FREE(p);
				break;
			}
		}
	}
	array_free(&devs, nrdevs);

	firmware.version = 0;
	firmware.lpf = 0;
	firmware.hpf = 0;

	log_file_enable();
	log_shell_disable();

	memset(buffer, '\0', BUFFER_SIZE);

	if(nodaemon == 1) {
		log_shell_enable();
	}

#ifdef _WIN32
	if((pid = check_instances(L"pilight-daemon")) != -1) {
		logprintf(LOG_NOTICE, "pilight is already running");
		goto clear;
	}
#endif

	if((pid = isrunning("pilight-raw")) != -1) {
		logprintf(LOG_NOTICE, "pilight-raw instance found (%d)", (int)pid);
		goto clear;
	}

	if((pid = isrunning("pilight-debug")) != -1) {
		logprintf(LOG_NOTICE, "pilight-debug instance found (%d)", (int)pid);
		goto clear;
	}

	if(config_set_file(configtmp) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	eventpool_init(EVENTPOOL_THREADED);
	protocol_init();
	config_init();

	/* Export certain daemon function to global usage */
	pilight.broadcast = &broadcast_queue;
	pilight.send = &send_queue;
	pilight.control = &control_device;
	pilight.receive = &receive_parse_api;

/* Rewrite */
	eventpool_callback(REASON_SOCKET_RECEIVED, socket_parse_data1);
	eventpool_callback(REASON_RECEIVED_PULSETRAIN, receivePulseTrain1);

	if(config_read() != EXIT_SUCCESS) {
		goto clear;
	}

	registerVersion();

	// let verbosity level from command line take precedence over config file
	if(verbosity_changed == 0) {
		settings_find_number("log-level", &verbosity);
	}

	log_level_set(verbosity);

	if(settings_find_string("log-file", &stmp) == 0) {
		if(log_file_set(stmp) == EXIT_FAILURE) {
			goto clear;
		}
	}

#ifdef WEBSERVER
	#ifdef WEBSERVER_HTTPS
	char *pemfile = NULL;
	int pem_free = 0;
	if(settings_find_string("pem-file", &pemfile) != 0) {
		if((pemfile = REALLOC(pemfile, strlen(PEM_FILE)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(pemfile, PEM_FILE);
		pem_free = 1;
	}	
	
	char *content = NULL;
	unsigned char md5sum[17];
	char md5conv[33];
	int i = 0;
	p = (char *)md5sum;
	if(file_exists(pemfile) != 0) {
		logprintf(LOG_ERR, "missing webserver SSL private key %s", pemfile);
		if(pem_free == 1) {
			FREE(pemfile);
		}
		goto clear;
	}
	if(file_get_contents(pemfile, &content) == 0) {
		mbedtls_md5((const unsigned char *)content, strlen((char *)content), (unsigned char *)p);
		for(i = 0; i < 32; i+=2) {
			sprintf(&md5conv[i], "%02x", md5sum[i/2] );
		}
		if(strcmp(md5conv, PILIGHT_PEM_MD5) == 0) {
			registry_set_number("webserver.ssl.certificate.secure", 0, 0);
		} else {
			registry_set_number("webserver.ssl.certificate.secure", 1, 0);
		}
		registry_set_string("webserver.ssl.certificate.location", pemfile);
		FREE(content);
	}

	if(pem_free == 1) {
		FREE(pemfile);
	}	
	#endif

	settings_find_number("webserver-enable", &webserver_enable);
	settings_find_number("webserver-http-port", &webserver_http_port);
	if(settings_find_string("webserver-root", &webserver_root) != 0) {
		if((webserver_root = REALLOC(webserver_root, strlen(WEBSERVER_ROOT)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(webserver_root, WEBSERVER_ROOT);
		webserver_root_free = 1;
	}
#endif

#ifndef _WIN32
	if(settings_find_string("pid-file", &pid_file) != 0) {
		if((pid_file = REALLOC(pid_file, strlen(PID_FILE)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(pid_file, PID_FILE);
		pid_file_free = 1;
	}

	if((f = open(pid_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) != -1) {
		if(read(f, buffer, BUFFER_SIZE) != -1) {
			//If the file is empty, create a new process
			strcat(buffer, "\0");
			if(!atoi(buffer)) {
				running = 0;
			} else {
				//Check if the process is running
				kill(atoi(buffer), 0);
				//If not, create a new process
				if(errno == ESRCH) {
					running = 0;
				}
			}
		}
	} else {
		logprintf(LOG_ERR, "could not open / create pid_file %s", pid_file);
		goto clear;
	}
	close(f);
#endif

#ifdef HASH
	logprintf(LOG_INFO, "version %s", HASH);
#else
	logprintf(LOG_INFO, "version %s", PILIGHT_VERSION);
#endif

	if(nodaemon == 1 || running == 1) {
		log_file_disable();
		log_shell_enable();
	}

#ifndef _WIN32
	if(running == 1) {
		nodaemon = 1;
		logprintf(LOG_NOTICE, "already active (pid %d)", atoi(buffer));
		log_level_set(LOG_NOTICE);
		log_shell_disable();
		goto clear;
	}
#endif

	struct conf_hardware_t *tmp_confhw = conf_hardware;
	while(tmp_confhw) {
		if(tmp_confhw->hardware->init) {
			if(tmp_confhw->hardware->comtype == COMOOK) {
				struct protocols_t *tmp = protocols;
				while(tmp) {
					if(tmp->listener->hwtype == tmp_confhw->hardware->hwtype) {
						if(tmp->listener->maxrawlen > tmp_confhw->hardware->maxrawlen) {
							tmp_confhw->hardware->maxrawlen = tmp->listener->maxrawlen;
						}
						if(tmp->listener->minrawlen > 0 && tmp->listener->minrawlen < tmp_confhw->hardware->minrawlen) {
							tmp_confhw->hardware->minrawlen = tmp->listener->minrawlen;
						}
						if(tmp->listener->maxgaplen > tmp_confhw->hardware->maxgaplen) {
							tmp_confhw->hardware->maxgaplen = tmp->listener->maxgaplen;
						}
						if(tmp->listener->mingaplen > 0 && tmp->listener->mingaplen < tmp_confhw->hardware->mingaplen) {
							tmp_confhw->hardware->mingaplen = tmp->listener->mingaplen;
						}
						if(tmp->listener->rawlen > 0) {
							logprintf(LOG_EMERG, "%s: setting \"rawlen\" length is not allowed, use the \"minrawlen\" and \"maxrawlen\" instead", tmp->listener->id);
							goto clear;
						}
					}
					tmp = tmp->next;
				}
			}
		}
		tmp_confhw = tmp_confhw->next;
	}

	settings_find_number("port", &port);
	settings_find_number("standalone", &standalone);

	pilight.runmode = STANDALONE;
	if(standalone == 0 || (master_server != NULL && master_port > 0)) {
		if(master_server != NULL && master_port > 0) {
			if((sockfd = socket_connect(master_server, master_port)) == -1) {
				logprintf(LOG_NOTICE, "pilight daemon not found @%s, waiting for it to come online", master_server);
			} else {
				logprintf(LOG_INFO, "a pilight daemon was found, clientizing");
			}
			pilight.runmode = ADHOC;
		} else if(ssdp_seek(&ssdp_list) == -1) {
			logprintf(LOG_INFO, "no pilight daemon found, daemonizing");
		} else {
			logprintf(LOG_NOTICE, "a pilight daemon was found @%s, clientizing", ssdp_list->ip);
			pilight.runmode = ADHOC;
		}
		if(ssdp_list) {
			ssdp_free(ssdp_list);
		}
	}

	ssl_init();
	if(pilight.runmode == STANDALONE) {
		socket_start((unsigned short)port);
		if(standalone == 0) {
			ssdp_start();
		}
	}

#ifndef _WIN32
	if(nodaemon == 0) {
		daemonize();
	} else {
		save_pid(getpid());
	}
#endif

	pilight.running = 1;

	/* Threads are unable to survive forks properly.
	 * Therefor, we queue all messages until we're
	 * able to fork properly.
	 */
	threads_create(&logpth, NULL, &logloop, (void *)NULL);

	pthread_mutexattr_init(&sendqueue_attr);
	pthread_mutexattr_settype(&sendqueue_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&sendqueue_lock, &sendqueue_attr);
	pthread_cond_init(&sendqueue_signal, NULL);
	sendqueue_init = 1;

	pthread_mutexattr_init(&config_attr);
	pthread_mutexattr_settype(&config_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&config_lock, &config_attr);

	pthread_mutexattr_init(&recvqueue_attr);
	pthread_mutexattr_settype(&recvqueue_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&recvqueue_lock, &recvqueue_attr);
	pthread_cond_init(&recvqueue_signal, NULL);
	recvqueue_init = 1;

	pthread_mutexattr_init(&bcqueue_attr);
	pthread_mutexattr_settype(&bcqueue_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&bcqueue_lock, &bcqueue_attr);
	pthread_cond_init(&bcqueue_signal, NULL);
	bcqueue_init = 1;

	/* Run certain daemon functions from the socket library */
	socket_callback.client_disconnected_callback = &socket_client_disconnected;
	socket_callback.client_connected_callback = NULL;
	socket_callback.client_data_callback = &socket_parse_data;

	/* Start threads library that keeps track of all threads used */
	threads_start();

	/* The daemon running in client mode, register a seperate thread that
	   communicates with the server */
	if(pilight.runmode == ADHOC) {
		threads_register("node", &clientize, (void *)NULL, 0);
	} else {
		/* Register a seperate thread for the socket server */
		threads_register("socket", &socket_wait, (void *)&socket_callback, 0);
		if(standalone == 0) {
			threads_register("ssdp", &ssdp_wait, (void *)NULL, 0);
		}
	}
	threads_register("sender", &send_code, (void *)NULL, 0);
	threads_register("broadcaster", &broadcast, (void *)NULL, 0);

	tmp_confhw = conf_hardware;
	while(tmp_confhw && main_loop) {
		if(tmp_confhw->hardware->init) {
			if(tmp_confhw->hardware->init() == EXIT_FAILURE) {
				if(main_loop == 1) {
					logprintf(LOG_ERR, "could not initialize %s hardware module", tmp_confhw->hardware->id);
					goto clear;
				} else {
					break;
				}
			}
			if(main_loop == 1) {
				tmp_confhw->hardware->wait = 0;
				tmp_confhw->hardware->stop = 0;
				if(tmp_confhw->hardware->comtype == COMOOK) {
#ifdef PILIGHT_DEVELOPMENT
					// threads_register(tmp_confhw->hardware->id, &receiveOOK, (void *)tmp_confhw->hardware, 0);
#endif
				} else if(tmp_confhw->hardware->comtype == COMPLSTRAIN) {
					threads_register(tmp_confhw->hardware->id, &receivePulseTrain, (void *)tmp_confhw->hardware, 0);
				} else if(tmp_confhw->hardware->comtype == COMAPI) {
					threads_register(tmp_confhw->hardware->id, tmp_confhw->hardware->receiveAPI, (void *)tmp_confhw->hardware, 0);
				}
			} else {
				break;
			}
		}
		if(main_loop == 0) {
			break;
		}
		tmp_confhw = tmp_confhw->next;
	}
	if(main_loop == 0) {
		goto clear;
	}

	threads_register("receive parser", &receive_parse_code, (void *)NULL, 0);

#ifdef EVENTS
	if(pilight.runmode == STANDALONE) {
		/* Register a seperate thread in which the daemon communicates the events library */
		threads_register("events client", &events_clientize, (void *)NULL, 0);
		threads_register("events loop", &events_loop, (void *)NULL, 0);
	}
#endif

#ifdef WEBSERVER
	settings_find_number("webgui-websockets", &webgui_websockets);

	/* Register a seperate thread for the webserver */
	if(webserver_enable == 1 && pilight.runmode == STANDALONE) {
		/* Register a seperate thread in which the webserver communicates the main daemon */
#ifdef PILIGHT_DEVELOPMENT
		if(webgui_websockets == 1) {
			threads_register("webserver broadcast", &webserver_broadcast, (void *)NULL, 0);
		}
#endif
	} else {
		webserver_enable = 0;
	}
#ifndef PILIGHT_DEVELOPMENT
	if(webserver_enable == 1) {
		webserver_start();
	}
#endif
#endif

	{
		char name[25], *server = NULL;
		unsigned int nrservers = 0;
		while(1) {
			sprintf(name, "ntpserver%d", nrservers);
			if(settings_find_string(name, &server) == 0) {
				strcpy(ntp_servers.server[nrservers].host, server);
				ntp_servers.server[x].port = 123;
				nrservers++;
			} else {
				break;
			}
		}
		ntp_servers.nrservers = nrservers;
		ntp_servers.callback = NULL;
		if(nrservers > 0) {
			ntpsync();
		}
	}

// #ifdef _WIN32
	// threads_register("stats", &pilight_stats, NULL, 0);
// #endif

	timer_stats_req = MALLOC(sizeof(uv_timer_t));
	if(timer_stats_req == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	uv_timer_init(uv_default_loop(), timer_stats_req);
	uv_timer_start(timer_stats_req, pilight_stats, 1000, 3000);
	return EXIT_SUCCESS;

clear:
	if(nodaemon == 0) {
		log_level_set(LOG_NOTICE);
		log_shell_disable();
	}
	if(main_loop == 1) {
		main_gc();
	}
	uv_stop(uv_default_loop());
	return EXIT_FAILURE;
}

#ifdef _WIN32
#define ID_TRAYICON 100
#define ID_QUIT 101
#define ID_SEPARATOR 102
#define ID_INSTALL_SERVICE 103
#define ID_REMOVE_SERVICE 104
#define ID_START 105
#define ID_STOP 106
#define ID_WEBGUI 107
#define ID_CONSOLE 108
#define ID_ICON 200
static NOTIFYICONDATA TrayIcon;

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  char buf[200], *service_argv[] = {__argv[0], NULL};
  POINT pt;
  HMENU hMenu;

  switch(msg) {
    case WM_CREATE:
			if(start_pilight(__argc, __argv) == EXIT_FAILURE) {
				if(main_loop == 1) {
					main_gc();
				}
				exit(EXIT_FAILURE);
			}
			running = 1;
		break;
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
        case ID_QUIT:
					if(main_loop == 1) {
						main_gc();
					}
          Shell_NotifyIcon(NIM_DELETE, &TrayIcon);
          PostQuitMessage(0);
				break;
				case ID_STOP:
					if(running == 1) {
						if(main_loop == 1) {
							main_gc();
						}
						running = 0;
						MessageBox(NULL, "pilight stopped", "pilight :: notice", MB_OK);
					}
				break;
				case ID_CONSOLE:
					if(console == 0) {
						openconsole();
					} else {
						closeconsole();
					}
				break;
				case ID_START:
					if(running == 0) {
						main_loop = 1;
						if(start_pilight(1, service_argv) == EXIT_FAILURE) {
							if(main_loop == 1) {
								main_gc();
								MessageBox(NULL, "pilight failed to start", "pilight :: error", MB_OK);
							}
						}
						running = 1;
						MessageBox(NULL, "pilight started", "pilight :: notice", MB_OK);
					}
				break;
				case ID_WEBGUI:
					if(webserver_http_port == 80) {
						snprintf(buf, sizeof(buf), "http://localhost");
					} else {
						snprintf(buf, sizeof(buf), "http://localhost:%d", webserver_http_port);
					}
					ShellExecute(NULL, "open", buf, NULL, NULL, SW_SHOWNORMAL);
				break;
      }
		break;
    case WM_USER:
      switch(lParam) {
        case WM_RBUTTONUP:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
          hMenu = CreatePopupMenu();

          AppendMenu(hMenu, MF_STRING | MF_GRAYED, ID_SEPARATOR, server_name);
					AppendMenu(hMenu, MF_SEPARATOR, ID_SEPARATOR, "");
          AppendMenu(hMenu, MF_STRING | MF_GRAYED, ID_SEPARATOR, ((pilight.runmode == ADHOC) ? "Ad-Hoc mode" : "Standalone mode"));
					AppendMenu(hMenu, MF_SEPARATOR, ID_SEPARATOR, "");
					if(pilight.runmode == STANDALONE) {
						AppendMenu(hMenu, MF_STRING | (running ? 0 : MF_GRAYED), ID_WEBGUI, "Open webGUI");
					}
					if(console == 0) {
						AppendMenu(hMenu, MF_STRING, ID_CONSOLE, "Open console");
					} else {
						AppendMenu(hMenu, MF_STRING, ID_CONSOLE, "Close console");
					}
          AppendMenu(hMenu, MF_SEPARATOR, ID_SEPARATOR, "");

					// snprintf(buf, sizeof(buf), "pilight is %s", running ? "running" : "stopped");

					// AppendMenu(hMenu, MF_STRING | MF_GRAYED, ID_SEPARATOR, buf);
          // AppendMenu(hMenu, MF_SEPARATOR, ID_SEPARATOR, "");

					// AppendMenu(hMenu, MF_STRING | (running ? 0 : MF_GRAYED), ID_STOP, "Stop pilight");
					// AppendMenu(hMenu, MF_STRING | (running ? MF_GRAYED : 0), ID_START, "Start pilight");

          AppendMenu(hMenu, MF_STRING, ID_QUIT, "Exit");

          GetCursorPos(&pt);
          SetForegroundWindow(hWnd);
          TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hWnd, NULL);
          PostMessage(hWnd, WM_NULL, 0, 0);
          DestroyMenu(hMenu);
				break;
      }
		break;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdline, int show) {
	pilight.running = 0;
	pilight.debuglevel = 0;

	HWND hWnd;
  WNDCLASS cls;
  MSG msg;
#ifdef HASH
	snprintf(server_name, sizeof(server_name), "pilight-daemon %s", HASH);
#else
	snprintf(server_name, sizeof(server_name), "pilight-daemon %s", PILIGHT_VERSION);
#endif
  memset(&cls, 0, sizeof(cls));
  cls.lpfnWndProc = (WNDPROC)WindowProc;
  cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  cls.lpszClassName = server_name;

  RegisterClass(&cls);
  hWnd = CreateWindow(cls.lpszClassName, server_name, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
  ShowWindow(hWnd, SW_HIDE);

  TrayIcon.cbSize = sizeof(TrayIcon);
  TrayIcon.uID = ID_TRAYICON;
  TrayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  TrayIcon.hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON), IMAGE_ICON, 16, 16, 0);
  TrayIcon.hWnd = hWnd;
  snprintf(TrayIcon.szTip, sizeof(TrayIcon.szTip), "%s", server_name);

  TrayIcon.uCallbackMessage = WM_USER;
  Shell_NotifyIcon(NIM_ADD, &TrayIcon);

  while(GetMessage(&msg, hWnd, 0, 0) && main_loop == 1) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
	return 0;
}
#else
int main(int argc, char **argv) {
	pilight.running = 0;
	pilight.debuglevel = 0;

	int ret = start_pilight(argc, argv);
	if(ret == EXIT_SUCCESS) {
		uv_run(uv_default_loop(), UV_RUN_DEFAULT);
		uv_walk(uv_default_loop(), walk_cb, NULL);
		uv_run(uv_default_loop(), UV_RUN_ONCE);

		while(uv_loop_close(uv_default_loop()) == UV_EBUSY) {
			uv_run(uv_default_loop(), UV_RUN_ONCE);
		}
		// pilight_stats((void *)NULL);
	}
	return ret;
}
#endif
