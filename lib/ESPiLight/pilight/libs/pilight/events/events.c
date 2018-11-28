/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
#endif

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/config.h"
#include "../core/log.h"
#include "../core/cast.h"
#include "../core/options.h"
#include "../core/json.h"
#include "../core/ssdp.h"
#include "../core/socket.h"
#include "../datatypes/stack.h"

#include "../lua/lua.h"

#include "../protocols/protocol.h"

#include "../config/rules.h"
#include "../config/settings.h"
#include "../config/devices.h"

#include "events.h"

#include "operator.h"
#include "function.h"
#include "action.h"

typedef struct lexer_t {
	int pos;
	int ppos;
	int len;
	char *current_char;
	struct token_t *current_token;
	char *text;
} lexer_t;

typedef struct token_t {
	int type;
	int pos;
	char *value;
} token_t;

typedef struct tree_t {
	struct token_t *token;
	struct tree_t **child;
	int nrchildren;
} tree_t;

// static struct token_string {
	// char *name;
// } token_string[] = {
	// { "TOPERATOR" },
	// { "TFUNCTION" },
	// { "TSTRING" },
	// { "TINTEGER" },
	// { "TEOF" },
	// { "LPAREN" },
	// { "RPAREN" },
	// { "TCOMMA" },
	// { "TIF" },
	// { "TELSE" },
	// { "TTHEN" },
	// { "TEND" },
	// { "TACTION" }
// };

typedef enum {
	TOPERATOR = 0,
	TFUNCTION = 1,
	TSTRING = 2,
	TINTEGER = 3,
	TEOF = 4,
	LPAREN = 5,
	RPAREN = 6,
	TCOMMA = 7,
	TIF = 8,
	TELSE = 9,
	TTHEN = 10,
	TEND = 11,
	TACTION = 12
} token_types;

#define ORIGIN_RULE RULE
#define ORIGIN_ACTION ACTION

static unsigned short loop = 1;
static char *dummy = "a";
static char true_[2];
static char false_[2];
static char dot_[2];

static char *recvBuff = NULL;
static int sockfd = 0;

static pthread_mutex_t events_lock;
static pthread_cond_t events_signal;
static pthread_mutexattr_t events_attr;
static unsigned short eventslock_init = 0;

typedef struct eventsqueue_t {
	struct JsonNode *jconfig;
	struct eventsqueue_t *next;
} eventsqueue_t;

static struct eventsqueue_t *eventsqueue;
static struct eventsqueue_t *eventsqueue_head;
static int eventsqueue_number = 0;
static int running = 0;

static int get_precedence(char *symbol) {
	struct plua_module_t *modules = plua_get_modules();
	int len = 0, x = 0;
	while(modules) {
		if(modules->type == OPERATOR) {
			len = strlen(modules->name);
			if(strnicmp(symbol, modules->name, len) == 0) {
				event_operator_associativity(modules->name, &x);
				return x;
			}
		}
		modules = modules->next;
	}
	return -1;
}

static int get_associativity(char *symbol) {
	struct plua_module_t *modules = plua_get_modules();
	int len = 0, x = 0;
	while(modules) {
		if(modules->type == OPERATOR) {
			len = strlen(modules->name);
			if(strnicmp(symbol, modules->name, len) == 0) {
				event_operator_precedence(modules->name, &x);
				return x;
			}
		}
		modules = modules->next;
	}
	return -1;
}

struct event_actions_t *get_action(char *symbol, int size) {
	struct event_actions_t *tmp_actions = NULL;
	int len = 0;

	tmp_actions = event_actions;
	while(tmp_actions) {
		len = strlen(tmp_actions->name);
		if((size == -1 && strnicmp(symbol, tmp_actions->name, len) == 0) ||
			(size == len && strnicmp(symbol, tmp_actions->name, len) == 0)) {
			return tmp_actions;
		}
		tmp_actions = tmp_actions->next;
	}
	return NULL;
}

static int is_function(char *symbol, int size) {
	struct plua_module_t *modules = plua_get_modules();
	int len = 0;
	while(modules) {
		if(modules->type == FUNCTION) {
			len = strlen(modules->name);
			if((size == -1 && strnicmp(symbol, modules->name, len) == 0) ||
				(size == len && strnicmp(symbol, modules->name, len) == 0)) {
				return len;
			}
		}
		modules = modules->next;
	}
	return -1;
}

static int is_operator(char *symbol, int size) {
	struct plua_module_t *modules = plua_get_modules();
	int len = 0;

	while(modules) {
		if(modules->type == OPERATOR) {
			len = strlen(modules->name);
			if((size == -1 && strnicmp(symbol, modules->name, len) == 0) ||
				(size == len && strnicmp(symbol, modules->name, len) == 0)) {
				return len;
			}
		}
		modules = modules->next;
	}
	return -1;
}

void events_tree_gc(struct tree_t *tree) {
	int i = 0;
	if(tree == NULL) {
		return;
	}
	for(i=0;i<tree->nrchildren;i++) {
		events_tree_gc(tree->child[i]);
	}
	if(tree->token != NULL) {
		FREE(tree->token->value);
		FREE(tree->token);
	}
	if(tree->nrchildren > 0) {
		FREE(tree->child);
	}
	FREE(tree);
}

int events_gc(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	loop = 0;

	if(eventslock_init == 1) {
		pthread_mutex_unlock(&events_lock);
		pthread_cond_signal(&events_signal);
	}

	while(running == 1) {
		usleep(10);
	}

	event_operator_gc();
	event_action_gc();
	event_function_gc();
	logprintf(LOG_DEBUG, "garbage collected events library");
	return 1;
}

static int print_error(struct lexer_t *lexer, struct tree_t *p, struct tree_t *node, struct token_t *token, int err, char *expected, int pos, int trunc) {
	char *p_elipses = "...";
	char *s_elipses = "...";
	char *s_tmp = "";
	char *p_tmp = "";
	int length = lexer->ppos+25;

	trunc -= 15;

	if(trunc < 0) {
		trunc = 0;
	}

	if(err == -1) {
		while(trunc > 0 && lexer->text[trunc] != ' ') {
			trunc--;
		}
		if(trunc > 0) {
			p_tmp = p_elipses;
		}
		if(length < strlen(&lexer->text[trunc])) {
			while(length < strlen(&lexer->text[trunc]) && lexer->text[length+trunc] != ' ') {
				length++;
			}
			s_tmp = s_elipses;
		}
		if(expected != NULL) {
			logprintf(LOG_ERR,
				"\n%s%.*s %s\n%*s^ unexpected symbol, expected %s",
				p_tmp, length, &lexer->text[trunc], s_tmp, pos+strlen(p_tmp)-trunc, " ", expected);
		} else {
			logprintf(LOG_ERR,
				"\n%s%.*s %s\n%*s^ unexpected symbol",
				p_tmp, length, &lexer->text[trunc], s_tmp, pos+strlen(p_tmp)-trunc, " ");
		}
		err = -2;
	}
	if(node != NULL) {
		events_tree_gc(node);
		node = NULL;
	}
	if(lexer->current_token != NULL) {
		FREE(lexer->current_token->value);
		FREE(lexer->current_token);
	}
	if(token != NULL) {
		FREE(token->value);
		FREE(token);
	}
	if(p != NULL) {
		events_tree_gc(p);
	}

	return err;
}

/*
 * TESTME: Check if right devices are cached.
 */
void event_cache_device(struct rules_t *obj, char *device) {
	int exists = 0;
	int o = 0;

	if(obj != NULL) {
		for(o=0;o<obj->nrdevices;o++) {
			if(strcmp(obj->devices[o], device) == 0) {
				exists = 1;
				break;
			}
		}
		if(exists == 0) {
			/* Store all devices that are present in this rule */
			if((obj->devices = REALLOC(obj->devices, sizeof(char *)*(unsigned int)(obj->nrdevices+1))) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			if((obj->devices[obj->nrdevices] = MALLOC(strlen(device)+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			strcpy(obj->devices[obj->nrdevices], device);
			obj->nrdevices++;
		}
	}
}

/*
 * This functions checks if the defined event variable
 * is part of one of devices in the config. If it is,
 * replace the variable with the actual value
 *
 * Return codes:
 * -1: An error was found and abort rule parsing
 * 0: Found variable and filled varcont
 * 1: Did not find variable and did not fill varcont
 */
int event_lookup_variable(char *var, struct rules_t *obj, struct varcont_t *varcont, unsigned short validate, enum origin_t origin) {
	int recvtype = 0;
	// int cached = 0;
	if(strcmp(true_, "1") != 0) {
		strcpy(true_, "1");
	}
	if(strcmp(dot_, ".") != 0) {
		strcpy(dot_, ".");
	}
	if(strcmp(false_, "0") != 0) {
		strcpy(false_, "0");
	}
	if(strcmp(var, "1") == 0 || strcmp(var, "0") == 0 ||
		 strcmp(var, "true") == 0 ||
		 strcmp(var, "false") == 0) {
		if(strcmp(var, "true") == 0) {
			varcont->number_ = 1;
		} else if(strcmp(var, "false") == 0) {
			varcont->number_ = 0;
		} else {
			varcont->number_ = atof(var);
		}
		varcont->decimals_ = 0;
		varcont->type_ = JSON_NUMBER;

		return 0;
	}

	int len = (int)strlen(var);
	int i = 0;
	int nrdots = 0;
	for(i=0;i<len;i++) {
		if(var[i] == '.') {
			nrdots++;
		}
	}

	if(nrdots == 1) {
		char **array = NULL;
		unsigned int n = explode(var, ".", &array);

		if(n < 2) {
			varcont->string_ = dot_;
			array_free(&array, n);
			varcont->type_ = JSON_STRING;
			return 0;
		}

		char *device = array[0];
		char *name = array[1];

		recvtype = 0;
		struct protocols_t *tmp_protocols = protocols;
#ifdef PILIGHT_REWRITE
		if(devices_select(ORIGIN_MASTER, device, NULL) == 0) {
			recvtype = 1;
		}
#else
		struct devices_t *dev = NULL;
		if(devices_get(device, &dev) == 0) {
			recvtype = 1;
		}
#endif
		if(recvtype == 0) {
			while(tmp_protocols) {
				if(strcmp(tmp_protocols->listener->id, device) == 0) {
					recvtype = 2;
					break;
				}
				tmp_protocols = tmp_protocols->next;
			}
		}

		unsigned int match1 = 0, match2 = 0, has_state = 0;

		if(recvtype == 2) {
			if(validate == 1) {
				if(origin == ORIGIN_RULE) {
					event_cache_device(obj, device);
				}
				if(strcmp(name, "repeats") != 0 && strcmp(name, "uuid") != 0) {
					struct options_t *options = tmp_protocols->listener->options;
					while(options) {
						if(options->conftype == DEVICES_STATE) {
							has_state = 1;
						}
						if(strcmp(options->name, name) == 0) {
							match2 = 1;
							if(options->vartype == JSON_NUMBER) {
								varcont->number_ = 0;
								varcont->decimals_ = 0;
								varcont->type_ = JSON_NUMBER;
							}
							if(options->vartype == JSON_STRING) {
								varcont->string_ = dummy;
								varcont->type_ = JSON_STRING;
							}
						}
						options = options->next;
					}
					if(match2 == 0 && ((!(strcmp(name, "state") == 0 && has_state == 1)) || (strcmp(name, "state") != 0))) {
						logprintf(LOG_ERR, "rule #%d invalid: protocol \"%s\" has no field \"%s\"", obj->nr, device, name);
						varcont->string_ = NULL;
						varcont->number_ = 0;
						varcont->decimals_ = 0;
						array_free(&array, n);
						return -1;
					}
				} else if(!(strcmp(name, "repeats") == 0 || strcmp(name, "uuid") == 0)) {
					logprintf(LOG_ERR, "rule #%d invalid: protocol \"%s\" has no field \"%s\"", obj->nr, device, name);
					varcont->string_ = NULL;
					varcont->number_ = 0;
					varcont->decimals_ = 0;
					array_free(&array, n);
					return -1;
				}
			}
			struct JsonNode *jmessage = NULL, *jnode = NULL;
			if(obj->jtrigger != NULL) {
				if(((jnode = json_find_member(obj->jtrigger, name)) != NULL) ||
					 ((jmessage = json_find_member(obj->jtrigger, "message")) != NULL &&
					 (jnode = json_find_member(jmessage, name)) != NULL)) {
					if(jnode->tag == JSON_STRING) {
						varcont->string_ = jnode->string_;
						varcont->type_ = JSON_STRING;
						array_free(&array, n);
						return 0;
					} else if(jnode->tag == JSON_NUMBER) {
						varcont->number_ = jnode->number_;
						varcont->decimals_ = jnode->decimals_;
						varcont->type_ = JSON_NUMBER;
						array_free(&array, n);
						return 0;
					}
				}
			}
			array_free(&array, n);
			return 0;
		} else if(recvtype == 1) {
			if(validate == 1) {
				if(origin == ORIGIN_RULE) {
					event_cache_device(obj, device);
				}
#ifdef PILIGHT_REWRITE
				struct protocol_t *tmp = NULL;
				i = 0;

				while(devices_select_protocol(ORIGIN_MASTER, device, i++, &tmp) == 0) {
					struct options_t *opt = tmp->options;
					while(opt) {
						if(opt->conftype == DEVICES_STATE && strcmp("state", name) == 0) {
							match1 = 1;
							match2 = 1;
							break;
						} else if(strcmp(opt->name, name) == 0) {
							match1 = 1;
							if(opt->conftype == DEVICES_VALUE || opt->conftype == DEVICES_STATE || opt->conftype == DEVICES_SETTING) {
								match2 = 1;
								break;
							}
						}
						opt = opt->next;
					}
				}
#else
				struct protocols_t *tmp = dev->protocols;
				while(tmp) {
					struct options_t *opt = tmp->listener->options;
					while(opt) {
						if(opt->conftype == DEVICES_STATE && strcmp("state", name) == 0) {
							match1 = 1;
							match2 = 1;
							break;
						} else if(strcmp(opt->name, name) == 0) {
							match1 = 1;
							if(opt->conftype == DEVICES_VALUE || opt->conftype == DEVICES_STATE || opt->conftype == DEVICES_SETTING) {
								match2 = 1;
								break;
							}
						}
						opt = opt->next;
					}
					tmp = tmp->next;
				}
#endif
				if(match1 == 0) {
					logprintf(LOG_ERR, "rule #%d invalid: device \"%s\" has no variable \"%s\"", obj->nr, device, name);
				} else if(match2 == 0) {
					logprintf(LOG_ERR, "rule #%d invalid: variable \"%s\" of device \"%s\" cannot be used in event rules", obj->nr, name, device);
				}
				if(match1 == 0 || match2 == 0) {
					varcont->string_ = NULL;
					varcont->number_ = 0;
					varcont->decimals_ = 0;
					array_free(&array, n);
					return -1;
				}
			}
			struct varcont_t val;
			i = 0;

#ifdef PILIGHT_REWRITE
			char *setting = NULL;
			while(devices_select_settings(ORIGIN_MASTER, device, i++, &setting, &val) == 0) {
				if(strcmp(setting, name) == 0) {
					if(val.type_ == JSON_STRING) {
						/* Cache values for faster future lookup */
						// if(obj != NULL) {
							// event_store_val_ptr(obj, device, name, tmp_settings);
						// }
						varcont->string_ = val.string_;
						varcont->type_ = JSON_STRING;
						array_free(&array, n);
						return 0;
					} else if(val.type_ == JSON_NUMBER) {
						/* Cache values for faster future lookup */
						// if(obj != NULL) {
							// event_store_val_ptr(obj, device, name, tmp_settings);
						// }
						varcont->number_ = val.number_;
						varcont->decimals_ = val.decimals_;
						varcont->type_ = JSON_NUMBER;
						array_free(&array, n);
						return 0;
					}
				}
			}
#else
			struct devices_settings_t *tmp_settings = dev->settings;
			while(tmp_settings) {
				if(strcmp(tmp_settings->name, name) == 0) {
					val.type_ = tmp_settings->values->type;
					if(val.type_ == JSON_STRING) {
						/* Cache values for faster future lookup */
						// if(obj != NULL) {
							// event_store_val_ptr(obj, device, name, tmp_settings);
						// }
						varcont->string_ = tmp_settings->values->string_;
						varcont->type_ = JSON_STRING;
						array_free(&array, n);
						return 0;
					} else if(val.type_ == JSON_NUMBER) {
						/* Cache values for faster future lookup */
						// if(obj != NULL) {
							// event_store_val_ptr(obj, device, name, tmp_settings);
						// }
						varcont->number_ = tmp_settings->values->number_;
						varcont->decimals_ = tmp_settings->values->decimals;
						varcont->type_ = JSON_NUMBER;
						array_free(&array, n);
						return 0;
					}
				}
				tmp_settings = tmp_settings->next;
			}
#endif
			logprintf(LOG_ERR, "rule #%d invalid: device \"%s\" has no variable \"%s\"", obj->nr, device, name);
			varcont->string_ = NULL;
			varcont->number_ = 0;
			varcont->decimals_ = 0;
			array_free(&array, n);
			return -1;
		}
		/*
		 * '21.00' comparison should be allowed and not be seen as config device
		 */
		/*else {
			logprintf(LOG_ERR, "rule #%d invalid: device \"%s\" does not exist in the config", obj->nr, device);
			varcont->string_ = NULL;
			varcont->number_ = 0;
			varcont->decimals_ = 0;
			array_free(&array, n);
			return -1;
		}*/
		array_free(&array, n);
	} else if(nrdots > 2) {
		logprintf(LOG_ERR, "rule #%d invalid: variable \"%s\" is invalid", obj->nr, var);
		varcont->string_ = NULL;
		varcont->number_ = 0;
		varcont->decimals_ = 0;
		return -1;
	}

	if(isNumeric(var) == 0) {
		varcont->number_ = atof(var);
		varcont->decimals_ = nrDecimals(var);
		varcont->type_ = JSON_NUMBER;
	} else {
		if((varcont->string_ = STRDUP(var)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		varcont->free_ = 1;
		varcont->type_ = JSON_STRING;
	}
	return 0;
}


static int lexer_parse_integer(struct lexer_t *lexer, struct stack_dt *t) {
	if(isdigit(lexer->current_char[0])) {
		while(lexer->pos <= lexer->len && isdigit(lexer->current_char[0])) {
			dt_stack_push(t, sizeof(char *), lexer->current_char);
			lexer->current_char = &lexer->text[lexer->pos++];
		}
		return 0;
	} else {
		return -1;
	}
}

static int lexer_parse_quoted_string(struct lexer_t *lexer, struct stack_dt *t) {
	int x = lexer->current_char[0];
	if(lexer->pos < lexer->len) {
		lexer->current_char = &lexer->text[lexer->pos++];

		while(lexer->pos < lexer->len) {
			if(lexer->pos > 0 && strnicmp(&lexer->text[lexer->pos-1], "\\'", 2) == 0) {
				/*
				 * Skip escape
				 */
				lexer->current_char = &lexer->text[lexer->pos++];
				dt_stack_push(t, sizeof(char *), lexer->current_char);
				lexer->current_char = &lexer->text[lexer->pos++];
				continue;
			}
			if(lexer->pos > 0 && strnicmp(&lexer->text[lexer->pos-1], "\\\"", 2) == 0) {
				/*
				 * Skip escape
				 */
				lexer->current_char = &lexer->text[lexer->pos++];
				dt_stack_push(t, sizeof(char *), lexer->current_char);
				lexer->current_char = &lexer->text[lexer->pos++];
				continue;
			}
			if(lexer->current_char[0] == x) {
				break;
			}
			dt_stack_push(t, sizeof(char *), lexer->current_char);
			lexer->current_char = &lexer->text[lexer->pos++];
		}
		if(lexer->current_char[0] != '"' && lexer->current_char[0] != '\'') {
			dt_stack_free(t, NULL);
			return print_error(lexer, NULL, NULL, NULL, -1, "a ending quote", lexer->pos, lexer->ppos-1);
		} else if(lexer->pos <= lexer->len) {
			lexer->current_char = &lexer->text[lexer->pos++];
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}

static int lexer_parse_string(struct lexer_t *lexer, struct stack_dt *t) {
	while(lexer->pos <= lexer->len &&
				lexer->current_char[0] != ' ' &&
				lexer->current_char[0] != ',' &&
				lexer->current_char[0] != ')') {
		dt_stack_push(t, sizeof(char *), lexer->current_char);
		lexer->current_char = &lexer->text[lexer->pos++];
	}
	return 0;
}

static int lexer_parse_space(struct lexer_t *lexer) {
	while(lexer->pos <= lexer->len && lexer->current_char[0] == ' ') {
		lexer->current_char = &lexer->text[lexer->pos++];
	}
	return 0;
}

static char *stack_to_char(struct stack_dt *stack) {
	char *c = NULL;
	char *str = MALLOC(dt_stack_top(stack)+1);
	int i = 0;

	if(str == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	while((c = dt_stack_pop(stack, 0)) != NULL) {
		str[i++] = c[0];
	}
	str[i] = '\0';
	dt_stack_free(stack, NULL);
	return str;
}

static char *min(char *numbers[], int n){
	char *m = NULL;
	int	i = 0;

	for(i=0;i<n;i++) {
		if(numbers[i] != NULL) {
			m = numbers[i];
			break;
		}
	}
	if(m == NULL) {
		return NULL;
	}

	for(i=1;i<n;i++) {
		if(numbers[i] != NULL && m > numbers[i]) {
			m = numbers[i];
		}
	}

	return m;
}

static int lexer_next_token(struct lexer_t *lexer) {
	struct stack_dt *t = MALLOC(sizeof(struct stack_dt));
	int type = TEOF, ret = 0;

	if(t == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	memset(t, 0, sizeof(struct stack_dt));

	while(lexer->pos <= lexer->len) {
		lexer->ppos = lexer->pos;
		if(lexer->current_char[0] == ' ') {
			if(lexer_parse_space(lexer) == -1) {
				return -1;
			}
		}

		int x = 0;
		if(lexer->current_char[0] == '\'' || lexer->current_char[0] == '"') {
			ret = lexer_parse_quoted_string(lexer, t);
			type = TSTRING;
		} else if(isdigit(lexer->current_char[0])) {
			ret = lexer_parse_integer(lexer, t);
			type = TINTEGER;
		} else if(strnicmp(lexer->current_char, "if", 2) == 0) {
			dt_stack_push(t, sizeof(char *), &lexer->current_char[0]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[1]);
			type = TIF;
			lexer->pos += 2;
			lexer->current_char = &lexer->text[lexer->pos-1];
		} else if(strnicmp(lexer->current_char, "else", 4) == 0) {
			dt_stack_push(t, sizeof(char *), &lexer->current_char[0]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[1]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[2]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[3]);
			type = TELSE;
			lexer->pos += 4;
			lexer->current_char = &lexer->text[lexer->pos-1];
		} else if(strnicmp(lexer->current_char, "then", 4) == 0) {
			dt_stack_push(t, sizeof(char *), &lexer->current_char[0]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[1]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[2]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[3]);
			type = TTHEN;
			lexer->pos += 4;
			lexer->current_char = &lexer->text[lexer->pos-1];
		} else if(strnicmp(lexer->current_char, "end", 3) == 0) {
			dt_stack_push(t, sizeof(char *), &lexer->current_char[0]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[1]);
			dt_stack_push(t, sizeof(char *), &lexer->current_char[2]);
			type = TEND;
			lexer->pos += 3;
			lexer->current_char = &lexer->text[lexer->pos-1];
		} else if(lexer->current_char[0] == ',') {
			dt_stack_push(t, sizeof(char *), lexer->current_char);
			type = TCOMMA;
			lexer->current_char = &lexer->text[lexer->pos++];
		} else if(lexer->current_char[0] == '(') {
			dt_stack_push(t, sizeof(char *), lexer->current_char);
			type = LPAREN;
			lexer->current_char = &lexer->text[lexer->pos++];
		} else if(lexer->current_char[0] == ')') {
			dt_stack_push(t, sizeof(char *), lexer->current_char);
			type = RPAREN;
			lexer->current_char = &lexer->text[lexer->pos++];
		} else {
			char *pos[3] = { NULL }, *m = NULL;
			int len = lexer->len-(lexer->pos-1), len1 = 0;
			pos[0] = strstr(lexer->current_char, " ");
			pos[1] = strstr(lexer->current_char, "(");
			pos[2] = strstr(lexer->current_char, ",");
			if((m = min(pos, 3)) != NULL) {
				len = m-lexer->current_char;
			}
			struct event_actions_t *action = NULL;
			if((len1 = is_function(lexer->current_char, len)) > 0) {
				for(x=0;x<len1;x++) {
					dt_stack_push(t, sizeof(char *), &lexer->current_char[x]);
				}
				type = TFUNCTION;
				lexer->current_char = &lexer->text[lexer->pos+(len1-1)];
				lexer->pos += len1;
			} else if((len1 = is_operator(lexer->current_char, len)) > 0) {
				for(x=0;x<len1;x++) {
					dt_stack_push(t, sizeof(char *), &lexer->current_char[x]);
				}
				type = TOPERATOR;
				lexer->current_char = &lexer->text[lexer->pos+(len1-1)];
				lexer->pos += len1;
			} else if((action = get_action(lexer->current_char, len)) != NULL) {
				len1 = strlen(action->name);
				for(x=0;x<len1;x++) {
					dt_stack_push(t, sizeof(char *), &lexer->current_char[x]);
				}
				type = TACTION;
				lexer->current_char = &lexer->text[lexer->pos+(len1-1)];
				lexer->pos += len1;
			}
		}
		if(type == TEOF) {
			ret = lexer_parse_string(lexer, t);
			type = TSTRING;
		}
		if(type != TEOF && ret == 0) {
			if((lexer->current_token = MALLOC(sizeof(struct token_t))) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			lexer->current_token->value = stack_to_char(t);
			lexer->current_token->type = type;
			lexer->current_token->pos = lexer->pos;
			return 0;
		} else {
			return ret;
		}
	}
	if(t != NULL) {
		FREE(t);
	}

	lexer->current_token = NULL;
	return 0;
}

static struct tree_t *ast_parent(struct token_t *token) {
	struct tree_t *tree = MALLOC(sizeof(struct tree_t));
	if(tree == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	tree->child = NULL;
	tree->nrchildren = 0;
	tree->token = token;
	return tree;
}

static struct tree_t *ast_child(struct tree_t *p, struct tree_t *c) {
	if((p->child = REALLOC(p->child, sizeof(struct tree_t)*(p->nrchildren+1))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	p->child[p->nrchildren] = c;
	p->nrchildren++;
	return p;
}

static int lexer_peek(struct lexer_t *lexer, int skip, int type, char *val) {
	int pos = lexer->pos, i = 0, ret = -1;
	struct token_t *tmp = lexer->current_token;
	char *c = lexer->current_char;

	if(skip > 0) {
		for(i=0;i<skip;i++) {
			if((ret = lexer_next_token(lexer)) < 0) {
				goto bad;
			}
			if(i < skip-1 && lexer->current_token != NULL) {
				FREE(lexer->current_token->value);
				FREE(lexer->current_token);
			}
		}
	}

	if(type == TEOF && lexer->current_token == NULL) {
		goto good;
	} else if(lexer->current_token != NULL && lexer->current_token->type == type) {
		if(val != NULL) {
			if(stricmp(lexer->current_token->value, val) == 0) {
				goto good;
			} else {
				ret = -1;
				goto bad;
			}
		} else {
			goto good;
		}
	} else {
		ret = -1;
		goto bad;
	}

good:
	if(skip > 0 && lexer->current_token != NULL) {
		FREE(lexer->current_token->value);
		FREE(lexer->current_token);
	}
	lexer->pos = pos;
	lexer->current_token = tmp;
	lexer->current_char = c;
	return 0;
bad:
	if(skip > 0 && lexer->current_token != NULL) {
		FREE(lexer->current_token->value);
		FREE(lexer->current_token);
	}
	lexer->pos = pos;
	lexer->current_token = tmp;
	lexer->current_char = c;

	return ret;
}

static int lexer_eat(struct lexer_t *lexer, int type, struct token_t **token_out) {
	struct token_t *token = lexer->current_token;
	int ret = -1;

	if((type == TEOF && lexer->current_token == NULL) ||
	   (lexer->current_token != NULL && lexer->current_token->type == type)) {
		if((ret = lexer_next_token(lexer)) < 0) {
			*token_out = NULL;
			return ret;
		}
	} else {
		if(lexer->current_token != NULL) {
			FREE(lexer->current_token->value);
			FREE(lexer->current_token);
		}
		*token_out = NULL;
		return -1;
	}
	*token_out = token; 
	return 0;
}

static int lexer_expr(struct lexer_t *lexer, struct tree_t *tree, struct tree_t **tree_out);
static int lexer_term(struct lexer_t *lexer, struct tree_t *tree, int precedence, struct tree_t **tree_out);

static int lexer_factor(struct lexer_t *lexer, struct tree_t *tree_in, struct tree_t **tree_out) {
	struct token_t *token = lexer->current_token;
	struct token_t *token_ret = NULL;
	char *expected = NULL;
	int pos = 0, err = -2;

	if(token != NULL) {
		switch(token->type) {
			case TSTRING: {
				if((err = lexer_eat(lexer, TSTRING, &token_ret)) < 0) {
					goto error;
				}
				*tree_out = ast_parent(token);
				return 0;
			} break;
			case TINTEGER: {
				if((err = lexer_eat(lexer, TINTEGER, &token_ret)) < 0) {
					goto error;
				}
				*tree_out = ast_parent(token);
				return 0;
			} break;
			case LPAREN: {
				if((err = lexer_eat(lexer, LPAREN, &token_ret)) < 0) {
					goto error;
				}
				FREE(token_ret->value);
				FREE(token_ret);
				if((err = lexer_expr(lexer, tree_in, tree_out)) < 0) {
					goto error;
				}
				pos = (*tree_out)->token->pos+1;
				if((err = lexer_eat(lexer, RPAREN, &token_ret)) < 0) {
					char *tmp = "a closing parenthesis";
					expected = tmp;
					pos += 1;
					goto error;
				}
				FREE(token_ret->value);
				FREE(token_ret);
				return 0;
			} break;
		}
	}

	*tree_out = NULL;
	return -1;

error:
	return print_error(lexer, NULL, *tree_out, token_ret, err, expected, pos, lexer->ppos-1);
}

static int lexer_parse_function(struct lexer_t *lexer, struct tree_t *tree_in, struct tree_t **tree_out) {
	struct tree_t *node = NULL;
	struct token_t *token = lexer->current_token, *token_ret = NULL;
	struct tree_t *p = ast_parent(token);
	char *expected = NULL;
	int pos = 0, err = -1;

	if((err = lexer_eat(lexer, TFUNCTION, &token_ret)) < 0) {
		*tree_out = NULL;
		return err;
	}
	if((err = lexer_eat(lexer, LPAREN, &token_ret)) < 0) {
		*tree_out = NULL;
		return err;
	}
	FREE(token_ret->value);
	FREE(token_ret);

	if((err = lexer_term(lexer, tree_in, 0, &node)) == 0) {
		ast_child(p, node);
	} else {
		events_tree_gc(p);
		*tree_out = NULL;
		return err;
	}
	pos = node->token->pos+1;
	while(1) {
		if((err = lexer_eat(lexer, TCOMMA, &token_ret)) < 0) {
			char *tmp = "a comma or closing parenthesis";
			pos -= strlen(node->token->value);
			lexer->ppos -= 2;
			expected = tmp;
			events_tree_gc(p);
			err = -1;
			goto error;
		}
		FREE(token_ret->value);
		FREE(token_ret);
		if(lexer_term(lexer, tree_in, 0, &node) == 0) {
			ast_child(p, node);
		}
		if(lexer_peek(lexer, 0, RPAREN, NULL) == 0) {
			break;
		}
		pos = node->token->pos+1;
	}

	if(lexer->current_token == NULL) {
		char *tmp = "a closing parenthesis";
		expected = tmp;
		err = -1;
		goto error;
	}

	if((err = lexer_eat(lexer, RPAREN, &token_ret)) < 0) {
		char *tmp = "a closing parenthesis";
		expected = tmp;
		pos = lexer->pos;
		err = -1;
		goto error;
	}
	FREE(token_ret->value);
	FREE(token_ret);

	*tree_out = p;
	return 0;

error:
	return print_error(lexer, NULL, NULL, NULL, err, expected, pos, lexer->ppos-1);
}

static void print_ast(struct tree_t *tree);

static int lexer_parse_action(struct lexer_t *lexer, struct tree_t *tree_in, struct tree_t **tree_out) {
	struct event_actions_t *action = NULL;
	struct options_t *options = NULL;
	struct tree_t *node = NULL;
	struct tree_t *p = NULL;
	struct tree_t *p1 = NULL;
	struct tree_t *tree_ret = NULL;
	struct token_t *token = lexer->current_token, *token_ret = NULL;
	char *expected = NULL;
	int len = strlen(token->value), match = 0, pos = 0, err = -1;

	p = ast_parent(token);

	action = get_action(token->value, len);
	if(action != NULL) {
		if((err = lexer_eat(lexer, TACTION, &token_ret)) < 0) {
			*tree_out = NULL;
			return -1;
		}
		/*
		 * In case we enter a recursive action action, we
		 * want to be able to free the string leading to
		 * that situation. E.g.:
		 *
		 * ... THEN switch DEVICE switch
		 *
		 * We want to be able to free the 'DEVICE' string.
		 */
		if(lexer_peek(lexer, 0, TSTRING, NULL) < 0) {
			if(lexer_peek(lexer, 0, TACTION, NULL) == 0) {
				char *tmp = "a string but got an action";
				expected = tmp;
			} else {
				char *tmp = "an action argument";
				expected = tmp;
			}
			pos = token->pos;
			err = -1;
			goto error;
		}
		while(1) {
			pos = lexer->pos;
			if(lexer_peek(lexer, 0, TEOF, NULL) == 0 ||
				 lexer_peek(lexer, 0, TELSE, NULL) == 0 ||
				 lexer_peek(lexer, 0, TEND, NULL) == 0) {
				break;
			}
			if(lexer_peek(lexer, 0, TACTION, NULL) == 0) {
				if(lexer_term(lexer, tree_in, 0, &tree_ret) == 0) {
					ast_child(p, tree_ret);
				}
				break;
			}
			if((err = lexer_eat(lexer, TSTRING, &token_ret)) < 0) {
				char *tmp = "an action argument";
				if(token_ret != NULL) {
					pos -= strlen(token_ret->value)+1;
				}
				lexer->ppos -= lexer->ppos-pos;
				expected = tmp;
				goto error;
			} else {
				match = 0;
				options = action->options;
				while(options != NULL) {
					if(stricmp(token_ret->value, options->name) == 0) {
						match = 1;
						break;
					}
					options = options->next;
				}
				if(match == 0) {
					char *tmp = "an action argument";
					pos -= strlen(token_ret->value)+1;
					lexer->ppos -= lexer->ppos-pos;
					expected = tmp;
					FREE(token_ret->value);
					FREE(token_ret);
					err = -1;
					goto error;
				}
				p1 = ast_parent(token_ret);
				ast_child(p, p1);
				/*
				 * In case we a arguments is combined like this
				 * THEN switch BETWEEN on AND off
				 */
				if(lexer_peek(lexer, 1, TOPERATOR, "and") == 0) {
					while(lexer_peek(lexer, 1, TOPERATOR, "and") == 0) {
						if(lexer_peek(lexer, 0, TINTEGER, NULL) == 0) {
							if((err = lexer_eat(lexer, TINTEGER, &token_ret)) < 0) {
								char *tmp = "a string,  number or function";
								expected = tmp;
								err = -1;
								goto error;
							}
							ast_child(p1, ast_parent(token_ret));
						} else if(lexer_peek(lexer, 0, TSTRING, NULL) == 0) {
							if((err = lexer_eat(lexer, TSTRING, &token_ret)) < 0) {
								char *tmp = "a string,  number or function";
								expected = tmp;
								err = -1;
								goto error;
							}
							ast_child(p1, ast_parent(token_ret));
						}
						if((err = lexer_eat(lexer, TOPERATOR, &token_ret)) < 0) {
							char *tmp = "an 'and' operator";
							expected = tmp;
							err = -1;
							goto error;
						}
						FREE(token_ret->value);
						FREE(token_ret);
					}
					if(lexer_peek(lexer, 0, TINTEGER, NULL) == 0) {
						if((err = lexer_eat(lexer, TINTEGER, &token_ret)) < 0) {
							char *tmp = "a string, number or function";
							expected = tmp;
							goto error;
						}
						ast_child(p1, ast_parent(token_ret));
					} else if(lexer_peek(lexer, 0, TSTRING, NULL) == 0) {
						if((err = lexer_eat(lexer, TSTRING, &token_ret)) < 0) {
							char *tmp = "a string, number or function";
							expected = tmp;
							err = -1;
							goto error;
						}
						ast_child(p1, ast_parent(token_ret));
					}
				} else {
					if(lexer_peek(lexer, 0, TACTION, NULL) == 0) {
						char *tmp = "a string, number, or function, got an action";
						expected = tmp;
						lexer->ppos -= strlen(lexer->current_token->value)+1;
						err = -1;
						goto error;
					}
					if((err = lexer_term(lexer, tree_in, 0, &node)) < 0) {
						char *tmp = "a string, number or function";
						expected = tmp;
						goto error;
					}
					ast_child(p1, node);
				}
			}
		}
	} else {
		*tree_out = NULL;
		return -1;
	}

	*tree_out = p;
	return 0;

error:
		return print_error(lexer, p, NULL, NULL, err, expected, pos, lexer->ppos-1);
}

static int lexer_parse_if(struct lexer_t *lexer, struct tree_t *tree, struct tree_t **tree_out) {
	struct tree_t *node = NULL;
	struct token_t *token = lexer->current_token, *token_ret = NULL;
	struct tree_t *p = ast_parent(token);
	char *expected = NULL;
	int pos = 0, err = -1;

	if((err = lexer_eat(lexer, TIF, &token_ret)) < 0) {
		*tree_out = NULL;
		return -1;
	}
	if((err = lexer_term(lexer, tree, 0, &node)) == 0) {
		ast_child(p, node);
	} else {
		char *tmp = "a condition";
		expected = tmp;
		pos = lexer->pos;
		goto error;
	}

	token = lexer->current_token;
	pos = token->pos;

	if((err = lexer_eat(lexer, TTHEN, &token_ret)) < 0) {
		char *tmp = "a condition";
		expected = tmp;
		err = -1;
		goto error;
	}
	pos = token->pos;
	FREE(token->value);
	FREE(token);
	if(lexer_peek(lexer, 0, TACTION, NULL) < 0 && lexer_peek(lexer, 0, TIF, NULL) < 0) {
		if((err = lexer_peek(lexer, 0, TACTION, NULL)) < 0) {
			char *tmp = "an action";
			expected = tmp;
			err = -1;
			goto error;
		}
	}
	if((err = lexer_term(lexer, tree, 0, &node)) == 0) {
		ast_child(p, node);
	} else {
		char *tmp = "an action";
		expected = tmp;
		goto error;
	}
	if(lexer_peek(lexer, 0, TELSE, NULL) == 0) {
		token = lexer->current_token;
		if((err = lexer_eat(lexer, TELSE, &token_ret)) < 0) {
			*tree_out = NULL;
			return -1;
		}
		FREE(token->value);
		FREE(token);
		if(lexer_term(lexer, tree, 0, &node) == 0) {
			ast_child(p, node);
		}
		if(lexer_peek(lexer, 0, TACTION, NULL) < 0 && lexer_peek(lexer, 0, TIF, NULL) < 0 &&
			lexer_peek(lexer, 0, TEND, NULL) < 0 && lexer_peek(lexer, 0, TEOF, NULL) < 0) {
			if((err = lexer_peek(lexer, 0, TACTION, NULL)) < 0) {
				char *tmp = "an action";
				expected = tmp;
				err = -1;
				goto error;
			}
		}
	}
	if(lexer_peek(lexer, 0, TEND, NULL) == 0) {
		token = lexer->current_token;
		if((err = lexer_eat(lexer, TEND, &token_ret)) < 0) {
			*tree_out = NULL;
			return -1;
		}
		FREE(token->value);
		FREE(token);
	} else if(lexer_peek(lexer, 0, TEOF, NULL) == 0) {
		if((err = lexer_eat(lexer, TEOF, &token_ret)) < 0) {
			*tree_out = NULL;
			return -1;
		}
	} else {
		token = lexer->current_token;
		if((err = lexer_eat(lexer, TEND, &token_ret)) < 0) {
			*tree_out = NULL;
			return -1;
		}
		FREE(token->value);
		FREE(token);
	}

	*tree_out = p;
	return 0;

error:
	return print_error(lexer, p, NULL, NULL, err, expected, pos, lexer->ppos-1);
}

/*
 * Always annoying when you reinvented an already existing
 * algorithm without knowing it :)
 *
 * An implementation of precedence climbing
 */
static int lexer_term(struct lexer_t *lexer, struct tree_t *tree_in, int precedence, struct tree_t **tree_out) {
	struct token_t *token = NULL, *token_ret = NULL;
	struct tree_t *node = NULL, *tree_ret = NULL;
	struct tree_t *p = NULL;
	char *expected = NULL;
	int pos = 0, err = -1;

	int match = 0, x = 0;
	struct plua_module_t *modules = plua_get_modules();
	while(modules) {
		if(modules->type == OPERATOR) {
			event_operator_associativity(modules->name, &x);
			if(x > precedence) {
				match = 1;
				break;
			}
		}
		modules = modules->next;
	}

	if(match == 0) {
		if((err = lexer_factor(lexer, tree_in, &tree_ret)) < 0) {
			*tree_out = NULL;
			return err;
		} else {
			*tree_out = tree_ret;
			return 0;
		}
	}

	err = lexer_term(lexer, tree_in, precedence+1, &node);
	if(lexer->current_token != NULL) {
		switch(lexer->current_token->type) {
			case TIF: {
				if((err = lexer_parse_if(lexer, tree_in, &tree_ret)) < 0) {
					*tree_out = NULL;
					char *tmp = "an action";
					expected = tmp;
					pos = lexer->pos;
					goto error;
				}
				*tree_out = tree_ret;
				return 0;
			} break;
			case TACTION: {
				if(node != NULL) {
					if(node->token->value != NULL) {
						FREE(node->token->value);
					}
					if(node->token != NULL) {
						FREE(node->token);
					}
					FREE(node);
					node = NULL;
				}
				if((err = lexer_parse_action(lexer, tree_in, &tree_ret)) < 0) {
					*tree_out = NULL;
					goto error;
				}
				*tree_out = tree_ret;
				return 0;
			} break;
			case TFUNCTION: {
				if((err = lexer_parse_function(lexer, tree_in, &tree_ret)) < 0) {
					*tree_out = NULL;
					char *tmp = "an action";
					expected = tmp;
					pos = lexer->pos;
					goto error;
				}
				if(lexer_peek(lexer, 0, TFUNCTION, NULL) == 0) {
					*tree_out = NULL;
					char *tmp = "an operator";
					expected = tmp;
					pos = lexer->pos-strlen(lexer->current_token->value)-1;
					events_tree_gc(tree_ret);
					err = -1;
					goto error;
				}
				if(node != NULL) {
					ast_child(node, tree_ret);
					*tree_out = node;
				} else {
					*tree_out = tree_ret;
				}
				return 0;
			} break;
			case TOPERATOR: {
				int tpos = lexer->ppos;
				int z = get_precedence(lexer->current_token->value);
				int y = get_associativity(lexer->current_token->value);
				if((z == precedence && y == 1) || z > precedence) {
					if(lexer->current_token->type == TEOF) {
						*tree_out = node;
						goto error;
					}
					token = lexer->current_token;
					if((err = lexer_eat(lexer, TOPERATOR, &token_ret)) < 0) {
						*tree_out = NULL;
						return -1;
					}
					if(z == -1) {
						if((err = lexer_term(lexer, tree_in, precedence, &tree_ret)) < 0) {
							char *tmp = "an operand";
							expected = tmp;
							lexer->ppos = tpos;
							pos = token->pos;
							goto error;
						}
						if(tree_ret->token->type == TACTION) {
							char *tmp = "an operand, got an action";
							expected = tmp;
							lexer->ppos = tpos;
							pos = token->pos;
							err = -1;
							events_tree_gc(tree_ret);
							goto error;
						}
						p = ast_parent(token);
						ast_child(p, node);
						ast_child(p, tree_ret);
						*tree_out = p;
						return 0;
					} else {
						if((err = lexer_term(lexer, tree_in, precedence+1, &tree_ret)) < 0) {
							char *tmp = "an operand";
							expected = tmp;
							pos = token->pos;
							lexer->ppos = tpos;
							goto error;
						}
						if(tree_ret->token->type == TACTION) {
							char *tmp = "an operand, got an action";
							expected = tmp;
							pos = token->pos;
							lexer->ppos = tpos;
							err = -1;
							events_tree_gc(tree_ret);
							goto error;
						}
						p = ast_parent(token);
						ast_child(p, node);
						ast_child(p, tree_ret);
						*tree_out = p;
						return 0;
					}
				}
			} break;
		}
	}

	if((*tree_out = node) == NULL) {
		return err;
	}
	return 0;

error:
	return print_error(lexer, p, node, token, err, expected, pos, lexer->ppos-1);
}

static int lexer_expr(struct lexer_t *lexer, struct tree_t *tree, struct tree_t **out) {
	return lexer_term(lexer, tree, 0, out);
}

static int lexer_parse(struct lexer_t *lexer, struct tree_t **tree_out) {
	struct tree_t *tree = MALLOC(sizeof(struct tree_t));
	int err = -1;

	if(tree == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(tree, 0, sizeof(struct tree_t));

	if((err = lexer_expr(lexer, tree, tree_out)) < 0) {
		if(err == -1) {
			err = print_error(lexer, tree, NULL, NULL, err, NULL, lexer->pos+1, lexer->ppos-1);
		} else {
			events_tree_gc(tree);
		}
		return err;
	} else {
		if(lexer->current_token != NULL && lexer->current_token->type != TEOF) {
			FREE(tree);
			return -1;
		}

		FREE(tree);
	}
	return 0;
}

static void print_ast(struct tree_t *tree) {
	int i = 0;
	if(tree == NULL) {
		return;
	}
	for(i=0;i<tree->nrchildren;i++) {
		if(tree->child[i]->token != NULL) {
			printf("\"%p\" [label=\"%s\"];\n", tree->token->value, tree->token->value);
			printf("\"%p\" [label=\"%s\"];\n", tree->child[i]->token->value, tree->child[i]->token->value);
			printf("\"%p\" -> ", tree->token->value);
			printf("\"%p\";\n", tree->child[i]->token->value);
		}
		print_ast(tree->child[i]);
	}

	if(tree->nrchildren == 0 && tree->token != NULL) {
		printf("\"%p\" [label=\"%s\"];\n", tree->token->value, tree->token->value);
		printf("\"%p\";\n", tree->token->value);
	}
}

static int interpret(struct tree_t *tree, struct rules_t *obj, unsigned short validate, struct varcont_t *v);

static void varcont_free(struct varcont_t *v) {
	if(v->free_ == 1) {
		if(v->type_ == JSON_STRING) {
			FREE(v->string_);
		}
		v->free_ = 0;
	}
}

static int run_function(struct tree_t *tree, struct rules_t *obj, unsigned short validate, struct varcont_t *v) {
	struct event_function_args_t *args = NULL;
	struct varcont_t v_res, v1;
	int i = 0;

	for(i=0;i<tree->nrchildren;i++) {
		memset(&v_res, '\0', sizeof(struct varcont_t));
		memset(&v1, '\0', sizeof(struct varcont_t));

		if(interpret(tree->child[i], obj, validate, &v_res) == -1) {
			v = NULL;
			return -1;
		}
		if(v_res.type_ == JSON_STRING) {
			if(event_lookup_variable(v_res.string_, obj, &v1, validate, ORIGIN_RULE) == -1) {
				varcont_free(&v1);
				varcont_free(&v_res);
				return -1;
			} else {
				args = event_function_add_argument(&v1, args);
				varcont_free(&v1);
				varcont_free(&v_res);
			}
		} else {
			args = event_function_add_argument(&v_res, args);
		}
	}

	memset(&v_res, '\0', sizeof(struct varcont_t));
	if(event_function_callback(tree->token->value, args, v) == -1) {
		return -1;
	}

	return 0;
}

static int run_action(struct tree_t *tree, struct rules_t *obj, unsigned short validate, struct varcont_t *v_out) {
	struct JsonNode *jobject = json_mkobject();
	struct JsonNode *jparam = NULL;
	struct JsonNode *jvalues = NULL;
	struct varcont_t v_res, v_res1, v1;
	struct rules_actions_t *node = NULL;
	struct event_actions_t *action = NULL;
	char *key = NULL;
	int i = 0, x = 0, match = 0;

	action = get_action(tree->token->value, strlen(tree->token->value));

	for(i=0;i<tree->nrchildren;i++) {
		if(tree->child[i]->token->type == TACTION) {
			if(interpret(tree->child[i], obj, validate, &v_res) == -1) {
				v_out = NULL;
				return -1;
			}
			continue;
		}
		memset(&v_res1, '\0', sizeof(struct varcont_t));
		memset(&v_res, '\0', sizeof(struct varcont_t));
		memset(&v1, '\0', sizeof(struct varcont_t));

		jparam = json_mkobject();
		jvalues = json_mkarray();
		json_append_member(jparam, "order", json_mknumber(i+1, 0));
		json_append_member(jparam, "value", jvalues);
		if(interpret(tree->child[i], obj, validate, &v_res) == -1) {
			v_out = NULL;
			return -1;
		}
		key = v_res.string_;

		for(x=0;x<tree->child[i]->nrchildren;x++) {
			if(interpret(tree->child[i]->child[x], obj, validate, &v_res1) == -1) {
				v_out = NULL;
				return -1;
			}

			switch(v_res1.type_) {
				case JSON_STRING: {
					if(event_lookup_variable(v_res1.string_, obj, &v1, validate, ORIGIN_RULE) == -1) {
						varcont_free(&v1);
						varcont_free(&v_res);
						varcont_free(&v_res1);
						v_out = NULL;
						return -1;
					} else {
						switch(v1.type_) {
							case JSON_NUMBER: {
								json_append_element(jvalues, json_mknumber(v1.number_, v1.decimals_));
							} break;
							case JSON_STRING: {
								json_append_element(jvalues, json_mkstring(v1.string_));
							} break;
							case JSON_BOOL: {
								json_append_element(jvalues, json_mkbool(v1.bool_));
							} break;
						}
						varcont_free(&v1);
					}
				} break;
				case JSON_NUMBER: {
					json_append_element(jvalues, json_mknumber(v_res1.number_, v_res1.decimals_));
				} break;
				case JSON_BOOL: {
					json_append_element(jvalues, json_mkbool(v_res1.bool_));
				} break;
			}
			varcont_free(&v_res1);
		}
		json_append_member(jobject, key, jparam);
		varcont_free(&v_res);
	}

	node = obj->actions;
	while(node) {
		if(node->ptr == tree) {
			match = 1;
			break;
		}
		node = node->next;
	}
	if(match == 0) {
		if((node = MALLOC(sizeof(struct rules_actions_t))) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		node->ptr = tree;
		node->rule = obj;
		node->arguments = NULL;
		node->next = obj->actions;
		obj->actions = node;
	}

	if(node->arguments != NULL) {
		json_delete(node->arguments);
		node->arguments = NULL;
	}

	node->arguments = jobject;
	if(action != NULL) {
		if(validate == 1) {
			if(action->checkArguments != NULL) {
				if(action->checkArguments(node) == -1) {
					return -1;
				}
			}
		} else {
			if(action->run != NULL) {
				if(action->run(node) == -1) {
					return -1;
				}
			}
		}
	}

	v_out->bool_ = 1;
	v_out->type_ = JSON_BOOL;
	return 0;
}

static int interpret(struct tree_t *tree, struct rules_t *obj, unsigned short validate, struct varcont_t *v_out) {
	struct varcont_t v_res;
	memset(&v_res, 0, sizeof(struct varcont_t));
	if(tree == NULL) {
		return -1;
	}

	switch(tree->token->type) {
		case TACTION: {
			if(run_action(tree, obj, validate, v_out) == -1) {
				return -1;
			}
			return 0;
		} break;
		case TFUNCTION: {
			if(run_function(tree, obj, validate, v_out) == -1) {
				return -1;
			}
			return 0;
		} break;
		case TSTRING: {
			v_out->string_ = tree->token->value;
			v_out->type_ = JSON_STRING;
			return 0;
		} break;
		case TINTEGER: {
			v_out->number_ = atof(tree->token->value);
			v_out->decimals_ = nrDecimals(tree->token->value);
			v_out->type_ = JSON_NUMBER;
			return 0;
		} break;
		case TIF: {
			if(interpret(tree->child[0], obj, validate, &v_res) == -1) {
				varcont_free(&v_res);
				return -1;
			} else {
				if(v_res.type_ != JSON_BOOL) {
					logprintf(LOG_ERR, "If condition did not result in a boolean expression");
					return -1;
				}
				if(v_res.bool_ == 1) {
					varcont_free(&v_res);
					if(interpret(tree->child[1], obj, validate, &v_res) == -1) {
						varcont_free(&v_res);
						return -1;
					}
					memcpy(v_out, &v_res, sizeof(struct varcont_t));
					varcont_free(&v_res);
					return 0;
				} else if(tree->nrchildren == 3) {
					varcont_free(&v_res);
					if(interpret(tree->child[2], obj, validate, &v_res) == -1) {
						varcont_free(&v_res);
						return -1;
					}
					memcpy(v_out, &v_res, sizeof(struct varcont_t));
					varcont_free(&v_res);
					return 0;
				} else {
					v_out->bool_ = 1;
					v_out->type_ = JSON_BOOL;
					varcont_free(&v_res);
					return 0;
				}
			}
			varcont_free(&v_res);
			return -1;
		} break;
		case TOPERATOR: {
			struct varcont_t v1, v2, v3, v4;

			memset(&v1, '\0', sizeof(struct varcont_t));
			memset(&v2, '\0', sizeof(struct varcont_t));
			memset(&v3, '\0', sizeof(struct varcont_t));
			memset(&v4, '\0', sizeof(struct varcont_t));

			if(event_operator_exists(tree->token->value) == 0) {
				if(interpret(tree->child[0], obj, validate, &v1) == -1) {
					varcont_free(&v1);
					return -1;
				}
				if(interpret(tree->child[1], obj, validate, &v2) == -1) {
					varcont_free(&v2);
					return -1;
				}
				if(v1.type_ == JSON_STRING) {
					if(event_lookup_variable(v1.string_, obj, &v3, validate, ORIGIN_RULE) == -1) {
						varcont_free(&v1);
						return -1;
					} else {
						varcont_free(&v1);
						switch(v3.type_) {
							case JSON_STRING: {
								if((v1.string_ = STRDUP(v3.string_)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								v1.free_ = 1;
								v1.type_ = JSON_STRING;
							} break;
							case JSON_NUMBER: {
								v1.number_ = v3.number_;
								v1.decimals_ = v3.decimals_;
								v1.type_ = JSON_NUMBER;
							} break;
							case JSON_BOOL: {
								v1.bool_ = v3.bool_;
								v1.type_ = JSON_BOOL;
							} break;
						}
						varcont_free(&v3);
					}
				}
				if(v2.type_ == JSON_STRING) {
					if(event_lookup_variable(v2.string_, obj, &v4, validate, ORIGIN_RULE) == -1) {
						varcont_free(&v2);
						return -1;
					} else {
						varcont_free(&v2);
						switch(v4.type_) {
							case JSON_STRING: {
								if((v2.string_ = STRDUP(v4.string_)) == NULL) {
									OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
								}
								v2.free_ = 1;
								v2.type_ = JSON_STRING;
							} break;
							case JSON_NUMBER: {
								v2.number_ = v4.number_;
								v2.decimals_ = v4.decimals_;
								v2.type_ = JSON_NUMBER;
							} break;
							case JSON_BOOL: {
								v2.bool_ = v4.bool_;
								v2.type_ = JSON_BOOL;
							} break;
						}
						varcont_free(&v4);
					}
				}
				if(event_operator_callback(tree->token->value, &v1, &v2, v_out) != 0) {
					logprintf(LOG_ERR, "rule #%d: an unexpected error occurred while parsing", obj->nr);
					varcont_free(&v1);
					varcont_free(&v2);
					return -1;
				}
				varcont_free(&v1);
				varcont_free(&v2);
				return 0;
			} else {
				return -1;
			}
		} break;
	}

	v_out->bool_ = 0;
	v_out->type_ = JSON_BOOL;
	return 0;
}

int event_parse_rule(char *rule, struct rules_t *obj, int depth, unsigned short validate) {
	struct varcont_t v_res;

	if(validate == 1) {
		struct lexer_t *lexer = MALLOC(sizeof(struct lexer_t));
		if(lexer == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		memset(lexer, 0, sizeof(struct lexer_t));

		if((lexer->text = STRDUP(rule)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		lexer->pos = 0;
		lexer->len = strlen(lexer->text);
		lexer->current_char = &lexer->text[lexer->pos++];

		if(lexer_next_token(lexer) == -1) {
			FREE(lexer->text);
			FREE(lexer);
			return -1;
		}
		if(lexer_parse(lexer, &obj->tree) == -1) {
			FREE(lexer->text);
			FREE(lexer);
			return -1;
		}
		if(pilight.debuglevel >= 1) {
			logprintf(LOG_DEBUG, "%s", lexer->text);
			print_ast(obj->tree);
		}
		FREE(lexer->text);
		FREE(lexer);
	}

	if(interpret(obj->tree, obj, validate, &v_res) == -1) {
		return -1;
	}
	if(v_res.type_ == JSON_BOOL) {
		return v_res.bool_;
	}
	return -1;
}

void *events_loop(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(eventslock_init == 0) {
		pthread_mutexattr_init(&events_attr);
		pthread_mutexattr_settype(&events_attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&events_lock, &events_attr);
		pthread_cond_init(&events_signal, NULL);
		eventslock_init = 1;
	}

	struct devices_t *dev = NULL;
	struct JsonNode *jdevices = NULL, *jchilds = NULL;
	struct rules_t *tmp_rules = NULL;
	char *str = NULL, *origin = NULL, *protocol = NULL;
	unsigned short match = 0;
	unsigned int i = 0;

	pthread_mutex_lock(&events_lock);
	while(loop) {
		if(eventsqueue_number > 0) {
			pthread_mutex_lock(&events_lock);

			logprintf(LOG_STACK, "%s::unlocked", __FUNCTION__);

			running = 1;

			jdevices = json_find_member(eventsqueue->jconfig, "devices");
			tmp_rules = rules_get();
			while(tmp_rules) {
				if(tmp_rules->active == 1) {
					if(eventsqueue->jconfig != NULL) {
						char *conf = json_stringify(eventsqueue->jconfig, NULL);
						tmp_rules->jtrigger = json_decode(conf);
						json_free(conf);
					}

					match = 0;
					if((str = MALLOC(strlen(tmp_rules->rule)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(str, tmp_rules->rule);
					if(json_find_string(eventsqueue->jconfig, "origin", &origin) == 0 &&
					   json_find_string(eventsqueue->jconfig, "protocol", &protocol) == 0) {
						if(strcmp(origin, "sender") == 0 || strcmp(origin, "receiver") == 0) {
							for(i=0;i<tmp_rules->nrdevices;i++) {
								if(strcmp(tmp_rules->devices[i], protocol) == 0) {
									match = 1;
									break;
								}
							}
						}
					}
					/* Only run those events that affect the updates devices */
					if(jdevices != NULL && match == 0) {
						jchilds = json_first_child(jdevices);
						while(jchilds) {
							for(i=0;i<tmp_rules->nrdevices;i++) {
								if(jchilds->tag == JSON_STRING &&
								   strcmp(jchilds->string_, tmp_rules->devices[i]) == 0) {
									if(devices_get(jchilds->string_, &dev) == 0) {
										if(dev->lastrule == tmp_rules->nr &&
											 tmp_rules->nr == dev->prevrule &&
											 dev->lastrule == dev->prevrule) {
											logprintf(LOG_ERR, "skipped rule #%d because of an infinite loop triggered by device %s", tmp_rules->nr, jchilds->string_);
										} else {
											match = 1;
										}
									} else {
										match = 1;
									}
									break;
								}
							}
							jchilds = jchilds->next;
						}
					}
					if(match == 1 && tmp_rules->status == 0) {
#ifndef WIN32
						clock_gettime(CLOCK_MONOTONIC, &tmp_rules->timestamp.first);
#endif
						if(event_parse_rule(str, tmp_rules, 0, 0) == 0) {
							if(tmp_rules->status == 1) {
								logprintf(LOG_INFO, "executed rule: %s", tmp_rules->name);
							}
						}
#ifndef WIN32
						clock_gettime(CLOCK_MONOTONIC, &tmp_rules->timestamp.second);
						logprintf(LOG_DEBUG, "rule #%d %s was parsed in %.6f seconds", tmp_rules->nr, tmp_rules->name,
							((double)tmp_rules->timestamp.second.tv_sec + 1.0e-9*tmp_rules->timestamp.second.tv_nsec) -
							((double)tmp_rules->timestamp.first.tv_sec + 1.0e-9*tmp_rules->timestamp.first.tv_nsec));
#endif
						tmp_rules->status = 0;
					}
					FREE(str);
					if(tmp_rules->jtrigger != NULL) {
						json_delete(tmp_rules->jtrigger);
						tmp_rules->jtrigger = NULL;
					}
				}
				tmp_rules = tmp_rules->next;
			}
			struct eventsqueue_t *tmp = eventsqueue;
			json_delete(tmp->jconfig);
			eventsqueue = eventsqueue->next;
			FREE(tmp);
			eventsqueue_number--;
			pthread_mutex_unlock(&events_lock);
		} else {
			running = 0;
			pthread_cond_wait(&events_signal, &events_lock);
		}
	}
	return (void *)NULL;
}

int events_running(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	return (running == 1) ? 0 : -1;
}
static void events_queue(char *message) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(eventslock_init == 1) {
		pthread_mutex_lock(&events_lock);
	}
	if(eventsqueue_number < 1024) {
		struct eventsqueue_t *enode = MALLOC(sizeof(eventsqueue_t));
		if(enode == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		enode->jconfig = json_decode(message);

		if(eventsqueue_number == 0) {
			eventsqueue = enode;
			eventsqueue_head = enode;
		} else {
			eventsqueue_head->next = enode;
			eventsqueue_head = enode;
		}

		eventsqueue_number++;
	} else {
		logprintf(LOG_ERR, "event queue full");
	}
	if(eventslock_init == 1) {
		pthread_mutex_unlock(&events_lock);
		pthread_cond_signal(&events_signal);
	}
}

void *events_clientize(void *param) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct JsonNode *jclient = NULL;
	struct JsonNode *joptions = NULL;
	struct ssdp_list_t *ssdp_list = NULL;
	char *out = NULL;
	int standalone = 0;
	int client_loop = 0;
	settings_find_number("standalone", &standalone);

	while(loop) {

		if(client_loop == 1) {
			sleep(1);
		}
		client_loop = 1;

		ssdp_list = NULL;
		if(ssdp_seek(&ssdp_list) == -1 || standalone == 1) {
			logprintf(LOG_NOTICE, "no pilight ssdp connections found");
			char server[16] = "127.0.0.1";
			if((sockfd = socket_connect(server, (unsigned short)socket_get_port())) == -1) {
				logprintf(LOG_ERR, "could not connect to pilight-daemon");
				continue;
			}
		} else {
			if((sockfd = socket_connect(ssdp_list->ip, ssdp_list->port)) == -1) {
				logprintf(LOG_ERR, "could not connect to pilight-daemon");
				continue;
			}
		}

		if(ssdp_list != NULL) {
			ssdp_free(ssdp_list);
		}

		jclient = json_mkobject();
		joptions = json_mkobject();
		json_append_member(jclient, "action", json_mkstring("identify"));
		json_append_member(joptions, "config", json_mknumber(1, 0));
		json_append_member(joptions, "receiver", json_mknumber(1, 0));
		json_append_member(jclient, "options", joptions);
		json_append_member(jclient, "media", json_mkstring("all"));
		out = json_stringify(jclient, NULL);
		if(socket_write(sockfd, out) != (strlen(out)+strlen(EOSS))) {
			json_free(out);
			json_delete(jclient);
			continue;
		}
		json_free(out);
		json_delete(jclient);

		if(socket_read(sockfd, &recvBuff, 0) != 0
			 || strcmp(recvBuff, "{\"status\":\"success\"}") != 0) {
			continue;
		}

		while(client_loop) {
			if(sockfd <= 0) {
				break;
			}
			if(loop == 0) {
				client_loop = 0;
				break;
			}

			int z = socket_read(sockfd, &recvBuff, 1);
			if(z == -1) {
				sockfd = 0;
				break;
			} else if(z == 1) {
				continue;
			}

			char **array = NULL;
			unsigned int n = explode(recvBuff, "\n", &array), i = 0;
			for(i=0;i<n;i++) {
				events_queue(array[i]);
			}
			array_free(&array, n);
		}
	}

	if(recvBuff != NULL) {
		FREE(recvBuff);
		recvBuff = NULL;
	}
	if(sockfd > 0) {
		socket_close(sockfd);
	}
	return (void *)NULL;
}
