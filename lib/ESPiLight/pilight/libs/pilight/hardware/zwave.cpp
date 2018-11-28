/*
	Copyright (C) 2013 - 2015 CurlyMo

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

	Big thanks to the Domoticz developers for inspiration.
*/

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <libgen.h>
#include <list>

#ifdef _WIN32
	#define STRICT
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <termios.h>
#endif

extern "C" {

#include "../config/hardware.h"
#include "../core/pilight.h"
#include "../core/json.h"
#include "../core/log.h"
#include "../core/json.h"
#include "../core/dso.h"
#include "../config/settings.h"
#include "zwave.h"

}

#include "../../openzwave/Options.h"
#include "../../openzwave/Manager.h"
#include "../../openzwave/Notification.h"
#include "../../openzwave/Node.h"
#include "../../openzwave/Driver.h"
#include "../../openzwave/platform/Log.h"
#include "../../openzwave/value_classes/Value.h"

static uint32 gHomeId = 0;

#define DRIVER_STATUS_INIT_SUCCESS 							0
#define DRIVER_STATUS_INIT_FAILED 							1
#define DRIVER_STATUS_NODES_QUERIED 						2
#define DRIVER_STATUS_NODES_QUERIED_SOME_DEAD 	3

static pthread_mutex_t init_lock;
static pthread_cond_t init_signal;
static pthread_mutexattr_t init_attr;

static pthread_mutex_t values_lock;
static pthread_mutexattr_t values_attr;

static unsigned short init_lock_init = 0;
static unsigned short values_lock_init = 0;

static int pending_command = 0;
static unsigned short driver_status = -1;
static char com[255];
static int threads = 0;
static int isinit = 0;

#ifdef _WIN32
static unsigned short nrports = 16;
static char comports[16][10]={"COM1",  "COM2",  "COM3",  "COM4",
                       "COM5",  "COM6",  "COM7",  "COM8",
                       "COM9",  "COM10", "COM11", "COM12",
                       "COM13", "COM14", "COM15", "COM16"};
#else
static unsigned short nrports = 44;
static char comports[44][16]={"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3","/dev/ttyS4","/dev/ttyS5",
                       "/dev/ttyS6","/dev/ttyS7","/dev/ttyS8","/dev/ttyS9","/dev/ttyS10","/dev/ttyS11",
                       "/dev/ttyS12","/dev/ttyS13","/dev/ttyS14","/dev/ttyS15","/dev/ttyUSB0",
                       "/dev/ttyUSB1","/dev/ttyUSB2","/dev/ttyUSB3","/dev/ttyUSB4","/dev/ttyUSB5",
                       "/dev/ttyAMA0","/dev/ttyAMA1","/dev/ttyACM0","/dev/ttyACM1",
                       "/dev/rfcomm0","/dev/rfcomm1","/dev/ircomm0","/dev/ircomm1",
                       "/dev/cuau0","/dev/cuau1","/dev/cuau2","/dev/cuau3",
                       "/dev/cuaU0","/dev/cuaU1","/dev/cuaU2","/dev/cuaU3",
											 "/dev/ttymxc0", "/dev/ttymxc1", "/dev/ttymxc2",
											 "/dev/ttymxc3", "/dev/ttymxc4", "/dev/ttymxc5"};
#endif

void UpdateNodeValue(int nodeId, OpenZWave::ValueID const v) {
	pthread_mutex_lock(&values_lock);
	struct JsonNode *json = json_mkobject();
	struct JsonNode *jcode = json_mkobject();
	struct zwave_values_t *tmp = NULL;
	int match = 0;
	std::string label = OpenZWave::Manager::Get()->GetValueLabel(v);

	tmp = zwave_values;
	while(tmp) {
		if(tmp->nodeId == nodeId && tmp->valueId == v.GetIndex()) {
			match = 1;
			break;
		}
		tmp = tmp->next;
	}
	
	if(match == 0) {
		if((tmp = (struct zwave_values_t *)MALLOC(sizeof(struct zwave_values_t))) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		tmp->nodeId = nodeId;
		tmp->cmdId = v.GetCommandClassId();
		tmp->valueId = v.GetIndex();
		tmp->instance = v.GetInstance();
		tmp->genre = v.GetGenre();
		tmp->valueType = v.GetType();
		if((tmp->label = (char *)MALLOC(strlen(label.c_str())+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(tmp->label, label.c_str());
		tmp->next = NULL;
	}

	json_append_member(json, "message", jcode);
	json_append_member(json, "origin", json_mkstring("receiver"));
	json_append_member(jcode, "homeId", json_mknumber(gHomeId, 0));
	json_append_member(jcode, "nodeId", json_mknumber(nodeId, 0));
	json_append_member(jcode, "cmdId", json_mknumber(v.GetCommandClassId(), 0));
	json_append_member(jcode, "label", json_mkstring(label.c_str()));

	std::string str;
	OpenZWave::Manager::Get()->GetValueAsString(v, &str);
	
	switch(v.GetType()) {
		case OpenZWave::ValueID::ValueType_Button:
		case OpenZWave::ValueID::ValueType_Bool:
			if(strcmp(str.c_str(), "False") == 0) {
				tmp->number_ = 0;
				json_append_member(jcode, "value", json_mknumber(0, 0));
			} else if(strcmp(str.c_str(), "True") == 0) {
				tmp->number_ = 1;
				json_append_member(jcode, "value", json_mknumber(1, 0));
			}
		break;
		case OpenZWave::ValueID::ValueType_String:
		case OpenZWave::ValueID::ValueType_Raw:
			json_append_member(jcode, "value", json_mkstring(str.c_str()));
		break;
		case OpenZWave::ValueID::ValueType_Byte:
		case OpenZWave::ValueID::ValueType_Decimal:
		case OpenZWave::ValueID::ValueType_Int:
		case OpenZWave::ValueID::ValueType_Short:
			json_append_member(jcode, "value", json_mknumber(atoi(str.c_str()), 0));
		break;
		case OpenZWave::ValueID::ValueType_List: {
			struct JsonNode *jvalues = json_mkarray();
			std::vector<std::string> values;
			OpenZWave::Manager::Get()->GetValueListSelection(v, &str);
			OpenZWave::Manager::Get()->GetValueListItems(v, &values);
			for(std::vector<std::string>::iterator it = values.begin(); it != values.end(); ++it) {
				std::string t = *it;
				json_append_element(jvalues, json_mkstring(t.c_str()));
			}
			json_append_member(jcode, "value", json_mkstring(str.c_str()));
			json_append_member(jcode, "values", jvalues);
		} break;
		case OpenZWave::ValueID::ValueType_Schedule:
		default:
		break;
	}

	if(pilight.receive != NULL) {
		pilight.receive(json, ZWAVE);
	}

	if(match == 0) {
		tmp->next = zwave_values;
		zwave_values = tmp;
	}

	OpenZWave::Manager::Get()->SetChangeVerified(v, true);
	json_delete(json);
	pthread_mutex_unlock(&values_lock);
}

void DeleteNodeValue(int nodeId, OpenZWave::ValueID const v) {
	pthread_mutex_lock(&values_lock);
	struct zwave_values_t *currP, *prevP;

	prevP = NULL;

	for(currP = zwave_values; currP != NULL; prevP = currP, currP = currP->next) {

		if(currP->nodeId == nodeId && currP->valueId == v.GetIndex()) {
			if(prevP == NULL) {
				zwave_values = currP->next;
			} else {
				prevP->next = currP->next;
			}

			FREE(currP);

			break;
		}
	}
	pthread_mutex_unlock(&values_lock);
}

void OnNotification(OpenZWave::Notification const *_notification, void *_context) {
	switch(_notification->GetType()) {
		case OpenZWave::Notification::Type_ValueAdded: {
			OpenZWave::ValueID v = _notification->GetValueID();
			UpdateNodeValue(_notification->GetNodeId(), v);
			if(pilight.debuglevel == 1) {
				printf("z-wave value added\n");
			}
		} break;
		case OpenZWave::Notification::Type_ValueRemoved: {
			OpenZWave::ValueID v = _notification->GetValueID();
			DeleteNodeValue(_notification->GetNodeId(), v);
			if(pilight.debuglevel == 1) {
				printf("z-wave value removed\n");
			}
		} break;
		case OpenZWave::Notification::Type_ValueChanged: {
			OpenZWave::ValueID v = _notification->GetValueID();
			UpdateNodeValue(_notification->GetNodeId(), v);
			if(pilight.debuglevel == 1) {
				printf("z-wave value changed\n");
			}
		} break;
		case OpenZWave::Notification::Type_Group:
		case OpenZWave::Notification::Type_NodeNew:
		break;
		case OpenZWave::Notification::Type_NodeAdded: {
			/* Add new node to the nodes list */
			if(pilight.debuglevel == 1) {
				printf("z-wave node added\n");
			}
		} break;
		case OpenZWave::Notification::Type_NodeRemoved: {
			// Remove a node from our list
			if(pilight.debuglevel == 1) {
				printf("z-wave node removed\n");
			}
		} break;
		case OpenZWave::Notification::Type_NodeProtocolInfo:
		case OpenZWave::Notification::Type_NodeNaming:
		case OpenZWave::Notification::Type_NodeEvent:
		case OpenZWave::Notification::Type_PollingDisabled:
		case OpenZWave::Notification::Type_PollingEnabled:
		case OpenZWave::Notification::Type_SceneEvent:
		case OpenZWave::Notification::Type_CreateButton:
		case OpenZWave::Notification::Type_DeleteButton:
		case OpenZWave::Notification::Type_ButtonOn:
		case OpenZWave::Notification::Type_ButtonOff:
		break;
		case OpenZWave::Notification::Type_DriverReady:
			gHomeId = _notification->GetHomeId();
			logprintf(LOG_INFO, "[Z-Wave] homeID is %lu", gHomeId);
			if(pilight.debuglevel == 1) {
				printf("z-wave driver ready\n");
			}
			driver_status = DRIVER_STATUS_INIT_SUCCESS;
			pthread_cond_signal(&init_signal);
		break;
		case OpenZWave::Notification::Type_DriverFailed:
			if(pilight.debuglevel == 1) {
				printf("z-wave driver failed\n");
			}
			driver_status = DRIVER_STATUS_INIT_FAILED;
			pthread_cond_signal(&init_signal);
		break;
		case OpenZWave::Notification::Type_DriverReset:
		break;
		case OpenZWave::Notification::Type_EssentialNodeQueriesComplete:
		case OpenZWave::Notification::Type_NodeQueriesComplete:
		case OpenZWave::Notification::Type_AwakeNodesQueried:
		break;
		case OpenZWave::Notification::Type_AllNodesQueriedSomeDead:
			if(pilight.debuglevel == 1) {
				printf("z-wave all nodes partially queried\n");
			}
			driver_status = DRIVER_STATUS_NODES_QUERIED_SOME_DEAD;
			pthread_cond_signal(&init_signal);
		break;
		case OpenZWave::Notification::Type_AllNodesQueried:
			if(pilight.debuglevel == 1) {
				printf("z-wave all nodes queried\n");
			}
			/* Nodes are ready to take instructions */
			driver_status = DRIVER_STATUS_NODES_QUERIED;
			pthread_cond_signal(&init_signal);
		break;
		case OpenZWave::Notification::Type_Notification: {
			if(pilight.debuglevel == 1) {
				printf("z-wave all notification\n");
			}
			switch(_notification->GetNotification()) {
				case OpenZWave::Notification::Code_MsgComplete:
					if(pilight.debuglevel == 1) {
						printf("- MsgComplete\n");
					}
				break;
				case OpenZWave::Notification::Code_Timeout:
					/* No connection could be made with the z-wave module */
					if(pilight.debuglevel == 1) {
						printf("- Timeout\n");
					}
				break;
				case OpenZWave::Notification::Code_NoOperation:
					if(pilight.debuglevel == 1) {
						printf("- NoOperation\n");
					}
				break;
				case OpenZWave::Notification::Code_Awake:
					if(pilight.debuglevel == 1) {
						printf("- Awake\n");
					}
				break;
				case OpenZWave::Notification::Code_Sleep:
					if(pilight.debuglevel == 1) {
						printf("- Sleep\n");
					}
				break;
				case OpenZWave::Notification::Code_Dead:
					if(pilight.debuglevel == 1) {
						printf("- Dead\n");
					}
				break;
				case OpenZWave::Notification::Code_Alive:
					if(pilight.debuglevel == 1) {
						printf("- Alive\n");
					}
				break;
				default:
					// logprintf(LOG_DEBUG, "- %d\n", _notification->GetNotification());
				break;
			}
		} break;
		case OpenZWave::Notification::Type_DriverRemoved:
			if(pilight.debuglevel == 1) {
				printf("z-wave driver removed\n");
			}
		default:
		break;
	}
}

static unsigned short zwaveHwInit(void) {
	char *logfile = NULL;
	int free_log_file = 0;
	if(settings_find_string("log-file", &logfile) != 0) {
		if((logfile = (char *)MALLOC(strlen(LOG_FILE)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		free_log_file = 1;
	}

	char *logpath = NULL;
	atomiclock();
	/* basename isn't thread safe */
	char *filename = basename(logfile);
	atomicunlock();	

	pthread_mutexattr_init(&init_attr);
	pthread_mutexattr_settype(&init_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&init_lock, &init_attr);
	pthread_cond_init(&init_signal, NULL);
	init_lock_init = 1;

	pthread_mutexattr_init(&values_attr);
	pthread_mutexattr_settype(&values_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&values_lock, &values_attr);
	values_lock_init = 1;	
	
	pthread_mutex_lock(&init_lock);

	size_t i = (strlen(logfile)-strlen(filename));
	if((logpath = (char *)REALLOC(logpath, i+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	memset(logpath, '\0', i+1);
	strncpy(logpath, logfile, i);	

	OpenZWave::Options::Create(logpath, logpath, "--SaveConfiguration=true");
	OpenZWave::Options::Get()->AddOptionString("UserPath", logpath, false);
	OpenZWave::Options::Get()->AddOptionString("logFilename", filename, false);

	FREE(logpath);
	if(free_log_file == 1) {
		FREE(logfile);
	}
	
	OpenZWave::Options::Get()->AddOptionBool("logging", true);
	if(pilight.debuglevel == 1) {
		OpenZWave::Options::Get()->AddOptionInt("SaveLogLevel", OpenZWave::LogLevel_Info);
	} else {
		OpenZWave::Options::Get()->AddOptionInt("SaveLogLevel", OpenZWave::LogLevel_Warning);
	}
	OpenZWave::Options::Get()->AddOptionInt("PollInterval", 500);
	OpenZWave::Options::Get()->AddOptionBool("IntervalBetweenPolls", true);
	OpenZWave::Options::Get()->AddOptionBool("AppendLogFile", true);
	OpenZWave::Options::Get()->AddOptionBool("ConsoleOutput", true);
	OpenZWave::Options::Get()->AddOptionBool("NotifyTransactions", true);
	OpenZWave::Options::Get()->Lock();
	OpenZWave::Manager::Create();
	OpenZWave::Manager::Get()->AddWatcher(OnNotification, NULL);
	OpenZWave::Manager::Get()->AddDriver(com);
	isinit = 1;

	pthread_cond_wait(&init_signal, &init_lock);

	if(driver_status == DRIVER_STATUS_INIT_SUCCESS) {
		logprintf(LOG_INFO, "[Z-Wave]: z-wave initialized, querying all nodes");
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

static unsigned short zwaveHwDeinit(void) {
	pthread_mutex_unlock(&init_lock);
	pthread_cond_signal(&init_signal);

	logprintf(LOG_NOTICE, "[Z-Wave]: closing and freeing z-wave modules, please wait...");
	if(isinit == 1) {
		OpenZWave::Manager::Get()->RemoveWatcher(OnNotification, NULL);
		sleep(1);	
		OpenZWave::Manager::Get()->RemoveDriver(com);
		OpenZWave::Manager::Destroy();
		OpenZWave::Options::Destroy();	
	}

	struct zwave_values_t *tmp = NULL;
	while(zwave_values) {
		tmp = zwave_values;
		FREE(tmp->label);
		zwave_values = zwave_values->next;
		FREE(tmp);
	}
	if(zwave_values != NULL) {
		FREE(zwave_values);
	}
	
	while(threads > 0) {
		usleep(10);
	}

	return EXIT_SUCCESS;
}

static OpenZWave::ValueID::ValueGenre getGenreFromEnum(int genre) {
	switch(genre) {
		default:
		case OpenZWave::ValueID::ValueGenre_Basic:
			return OpenZWave::ValueID::ValueGenre_Basic;
		break;
		case OpenZWave::ValueID::ValueGenre_User:
			return OpenZWave::ValueID::ValueGenre_User;
		break;
		case OpenZWave::ValueID::ValueGenre_Config:
			return OpenZWave::ValueID::ValueGenre_Config;
		break;
		case OpenZWave::ValueID::ValueGenre_System:
			return OpenZWave::ValueID::ValueGenre_System;
		break;
		case OpenZWave::ValueID::ValueGenre_Count:
			return OpenZWave::ValueID::ValueGenre_Count;
		break;
	}
}

static OpenZWave::ValueID::ValueType getValueTypeFromEnum(int type) {
	switch(type) {
		default:
		case OpenZWave::ValueID::ValueType_Bool:
			return OpenZWave::ValueID::ValueType_Bool;
		break;
		case OpenZWave::ValueID::ValueType_Byte:
			return OpenZWave::ValueID::ValueType_Byte;
		break;
		case OpenZWave::ValueID::ValueType_Decimal:
			return OpenZWave::ValueID::ValueType_Decimal;
		break;
		case OpenZWave::ValueID::ValueType_Int:
			return OpenZWave::ValueID::ValueType_Int;
		break;
		case OpenZWave::ValueID::ValueType_List:
			return OpenZWave::ValueID::ValueType_List;
		break;
		case OpenZWave::ValueID::ValueType_Schedule:
			return OpenZWave::ValueID::ValueType_Schedule;
		break;
		case OpenZWave::ValueID::ValueType_Short:
			return OpenZWave::ValueID::ValueType_Short;
		break;
		case OpenZWave::ValueID::ValueType_String:
			return OpenZWave::ValueID::ValueType_String;
		break;
		case OpenZWave::ValueID::ValueType_Button:
			return OpenZWave::ValueID::ValueType_Button;
		break;
		case OpenZWave::ValueID::ValueType_Raw:
			return OpenZWave::ValueID::ValueType_Raw;
		break;
	}
}

void zwaveSetValue(int nodeId, int cmdId, char *label, char *value) {
	pthread_mutex_lock(&values_lock);
	struct zwave_values_t *tmp = zwave_values;
	std::string v = value;

	while(tmp) {
		if(tmp->nodeId == nodeId && tmp->cmdId == cmdId &&
			((label != NULL && tmp->label != NULL && strcmp(tmp->label, label) == 0)
			|| label == NULL)) {
			OpenZWave::ValueID vID(gHomeId, tmp->nodeId, getGenreFromEnum(tmp->genre), tmp->cmdId, tmp->instance, tmp->valueId, getValueTypeFromEnum(tmp->valueType));			
			OpenZWave::Manager::Get()->SetValue(vID, v);
		}
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&values_lock);
}

static int zwaveSend(struct JsonNode *code) {
	return EXIT_SUCCESS;
}

static void OnDeviceStatusUpdate(OpenZWave::Driver::ControllerState cs, OpenZWave::Driver::ControllerError err, void *_context) {
	switch(cs) {
		case OpenZWave::Driver::ControllerState_Normal:
			pending_command = 0;
			logprintf(LOG_INFO, "[Z-Wave]: No Command in progress");
		break;
		case OpenZWave::Driver::ControllerState_Starting:
			logprintf(LOG_INFO, "[Z-Wave]: Starting controller command");
		break;
		case OpenZWave::Driver::ControllerState_Cancel:
			logprintf(LOG_INFO, "[Z-Wave]: The command was canceled");
			pending_command = 0;
		break;
		case OpenZWave::Driver::ControllerState_Error:
			logprintf(LOG_INFO, "[Z-Wave]: Command invocation had error(s) and was aborted");
			pending_command = 0;
		break;
		case OpenZWave::Driver::ControllerState_Waiting:
			logprintf(LOG_INFO, "[Z-Wave]: Controller is waiting for a user action");
			pending_command = 1;
		break;
		case OpenZWave::Driver::ControllerState_Sleeping:
			logprintf(LOG_INFO, "[Z-Wave]: Controller command is on a sleep queue wait for device");
		break;
		case OpenZWave::Driver::ControllerState_InProgress:
			logprintf(LOG_INFO, "[Z-Wave]: The controller is communicating with the other device to carry out the command");
		break;
		case OpenZWave::Driver::ControllerState_Completed:
			logprintf(LOG_INFO, "[Z-Wave]: The command has completed successfully");
			pending_command = 0;
		break;
		case OpenZWave::Driver::ControllerState_Failed:
			logprintf(LOG_INFO, "[Z-Wave]: The command has failed");
			pending_command = 0;
		break;
		case OpenZWave::Driver::ControllerState_NodeOK:
			logprintf(LOG_INFO, "[Z-Wave]: Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node is OK");
		break;
		case OpenZWave::Driver::ControllerState_NodeFailed:
			logprintf(LOG_INFO, "[Z-Wave]: Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node has failed");
		break;
		default:
			logprintf(LOG_INFO, "[Z-Wave]: Unknown Device Response!");
			pending_command = 0;
		break;
	}
}

void zwaveStartInclusion(void) {
	if(pending_command == 0) {
		// OpenZWave::Manager::Get()->AddNode(gHomeId);
		OpenZWave::Manager::Get()->BeginControllerCommand(gHomeId, OpenZWave::Driver::ControllerCommand_AddDevice, OnDeviceStatusUpdate, NULL, true);
	}
}

void zwaveStartExclusion(void) {
	if(pending_command == 0) {
		// OpenZWave::Manager::Get()->RemoveNode(gHomeId);
		OpenZWave::Manager::Get()->BeginControllerCommand(gHomeId, OpenZWave::Driver::ControllerCommand_RemoveDevice, OnDeviceStatusUpdate, NULL, true);
	}
}

void zwaveStopCommand(void) {
	if(pending_command == 1) {
		OpenZWave::Manager::Get()->CancelControllerCommand(gHomeId);
	}
}

void zwaveSoftReset(void) {
	OpenZWave::Manager::Get()->SoftReset(gHomeId);
}

void zwaveHardReset(void) {
	// OpenZWave::Manager::Get()->ResetController(gHomeId);
}

void zwaveGetConfigParam(int nodeId, int paramId) {
	OpenZWave::Manager::Get()->RequestConfigParam(gHomeId, nodeId, paramId);
}

void zwaveSetConfigParam(int nodeId, int paramId, int valueId, int len) {
	OpenZWave::Manager::Get()->SetConfigParam(gHomeId, nodeId, paramId, valueId, len);
}

static void *zwaveReceive(void *param) {
	threads++;
	
	while(init_lock_init == 0 && zwave->stop == 0) {
		usleep(10);
	}

	sleep(1);

	if(zwave->stop == 0) {
			pthread_cond_wait(&init_signal, &init_lock);

		if(driver_status == DRIVER_STATUS_NODES_QUERIED) {
			logprintf(LOG_INFO, "[Z-Wave]: all nodes queried");
		} else if(driver_status == DRIVER_STATUS_NODES_QUERIED_SOME_DEAD) {
			logprintf(LOG_INFO, "[Z-Wave]: nodes partially queried");
		}
		
		while(zwave->stop == 0) {
			sleep(3);
		}
	}
	threads--;
	return (void *)NULL;
}

static unsigned short zwaveSettings(JsonNode *json) {
	int i = 0;
	if(strcmp(json->key, "comport") == 0) {
		if(json->tag == JSON_STRING) {
			for(i=0;i<nrports;i++) {
				if(strcmp(comports[i], json->string_) == 0) {
					strcpy(com, json->string_);
					return EXIT_SUCCESS;
				}
			}
		}
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void zwaveInit(void) {
	hardware_register(&zwave);
	hardware_set_id(zwave, "zwave");

	options_add(&zwave->options, 'p', "comport", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	zwave->hwtype=ZWAVE;
	zwave->comtype=COMAPI;
	zwave->init=&zwaveHwInit;
	zwave->deinit=&zwaveHwDeinit;
	zwave->sendAPI=&zwaveSend;
	zwave->receiveAPI=&zwaveReceive;
	zwave->settings=&zwaveSettings;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "zwave";
	module->version = "1.0";
	module->reqversion = "6.0";
	module->reqcommit = "40";
}

void init(void) {
	zwaveInit();
}
#endif
