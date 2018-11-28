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

#ifndef _WIN32
	#include <regex.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "common.h"
#include "mem.h"
#include "options.h"
#include "json.h"

static int getOptPos = 0;
static char *longarg = NULL;
static char *shortarg = NULL;
static char *gctmp = NULL;

int options_gc(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	FREE(longarg);
	FREE(shortarg);
	if(gctmp != NULL) {
		FREE(gctmp);
	}

	logprintf(LOG_DEBUG, "garbage collected options library");
	return EXIT_SUCCESS;
}

/* Add a value to the specific struct id */
void options_set_string(struct options_t **opt, int id, const char *val) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			if((temp->string_ = REALLOC(temp->string_, strlen(val)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			temp->vartype = JSON_STRING;
			strcpy(temp->string_, val);
			break;
		}
		temp = temp->next;
	}
}

/* Add a value to the specific struct id */
void options_set_number(struct options_t **opt, int id, double val) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			temp->number_ = val;
			temp->vartype = JSON_NUMBER;
			break;
		}
		temp = temp->next;
	}
}

/* Get a certain option value identified by the id */
int options_get_string(struct options_t **opt, int id, char **out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	*out = NULL;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			if(temp->string_ && temp->vartype == JSON_STRING) {
				*out = temp->string_;
				return 0;
			} else {
				return 1;
			}
		}
		temp = temp->next;
	}

	return 1;
}

/* Get a certain option value identified by the id */
int options_get_number(struct options_t **opt, int id, double *out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	*out = 0;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			if(temp->vartype == JSON_NUMBER) {
				*out = temp->number_;
			}
			return 0;
		}
		temp = temp->next;
	}

	return 1;
}

/* Get a certain option argument type identified by the id */
int options_get_argtype(struct options_t **opt, int id, int *out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	*out = 0;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			if(temp->argtype != 0) {
				*out = temp->argtype;
				return 0;
			} else {
				return 1;
			}
		}
		temp = temp->next;
	}

	return 1;
}

/* Get a certain option argument type identified by the id */
int options_get_conftype(struct options_t **opt, int id, int *out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	*out = 0;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			if(temp->argtype != 0) {
				*out = temp->conftype;
				return 0;
			} else {
				return 1;
			}
		}
		temp = temp->next;
	}

	return 1;
}

/* Get a certain option name identified by the id */
int options_get_name(struct options_t **opt, int id, char **out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	*out = NULL;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			if(temp->name) {
				*out = temp->name;
				return 0;
			} else {
				return 1;
			}
		}
		temp = temp->next;
	}

	return 1;
}

/* Get a certain regex mask identified by the name */
int options_get_mask(struct options_t **opt, int id, char **out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	*out = NULL;
	while(temp) {
		if(temp->id == id && temp->id > 0) {
			if(temp->mask) {
				*out = temp->mask;
				return 0;
			} else {
				return 1;
			}
		}
		temp = temp->next;
	}

	return 1;
}

/* Get a certain option id identified by the name */
int options_get_id(struct options_t **opt, char *name, int *out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = *opt;
	*out = 0;
	while(temp) {
		if(temp->name) {
			if(strcmp(temp->name, name) == 0) {
				if(temp->id > 0) {
					*out = temp->id;
					return 0;
				} else {
					return 1;
				}
			}
		}
		temp = temp->next;
	}

	return 1;
}

/* Parse all CLI arguments */
int options_parse(struct options_t **opt, int argc, char **argv, int error_check, char **optarg) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int c = 0;
	int itmp = 0;
#if !defined(__FreeBSD__) && !defined(_WIN32)
	char *mask;
	regex_t regex;
	int reti;
#endif

	char *ctmp = NULL;

	/* If have read all arguments, exit and reset */
	if(getOptPos>=(argc-1)) {
		getOptPos=0;
		if(*optarg) {
			FREE(*optarg);
			*optarg = NULL;
		}
		return -1;
	} else {
		getOptPos++;
		/* Reserve enough memory to store all variables */
		if((longarg = REALLOC(longarg, 4)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		if((shortarg = REALLOC(shortarg, 2)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		if((*optarg = REALLOC(*optarg, 4)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}

		/* The memory to null */
		memset(*optarg, '\0', 4);
		memset(shortarg, '\0', 2);
		memset(longarg, '\0', 4);

		/* Check if the CLI character contains an equals to (=) sign.
		   If it does, we have probably encountered a long argument */
		if(strchr(argv[getOptPos],'=')) {
			/* Copy all characters until the equals to sign.
			   This will probably be the name of the argument */
			if((longarg = REALLOC(longarg, strcspn(argv[getOptPos],"=")+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			memset(longarg, '\0', strcspn(argv[getOptPos],"=")+1);
			memcpy(longarg, &argv[getOptPos][0], strcspn(argv[getOptPos],"="));

			/* Then copy everything after the equals sign.
			   This will probably be the value of the argument */
			size_t i = strlen(&argv[getOptPos][strcspn(argv[getOptPos],"=")+1]);
			if((*optarg = REALLOC(*optarg, i+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			memset(*optarg, '\0', i+1);
			memcpy(*optarg, &argv[getOptPos][strcspn(argv[getOptPos],"=")+1], i);
		} else {
			/* If the argument does not contain a equals sign.
			   Store the argument to check later if it's a long argument */
			if((longarg = REALLOC(longarg, strlen(argv[getOptPos])+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(longarg, argv[getOptPos]);
		}

		/* A short argument only contains of two characters.
		   So only store the first two characters */
		if((shortarg = REALLOC(shortarg, strlen(argv[getOptPos])+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		memset(shortarg, '\0', 3);
		strncpy(shortarg, argv[getOptPos], 2);

		/* Check if the short argument and the long argument are equal,
		   then we probably encountered a short argument. Only store the
		   identifier character. If there are more CLI characters
		   after the current one, store it as the CLI value. However, only
		   do this if the first character of the argument doesn't contain*/
		if(strcmp(longarg, shortarg) == 0 && (getOptPos+1)<argc && argv[getOptPos+1][0] != '-') {
			if((*optarg = REALLOC(*optarg, strlen(argv[getOptPos+1])+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(*optarg, argv[getOptPos+1]);
			c = shortarg[1];
			getOptPos++;
		} else {
			/* If the short argument and the long argument are not equal,
			    then we probably encountered a long argument. */
			if(longarg[0] == '-' && longarg[1] == '-') {
				if((gctmp = REALLOC(gctmp, strlen(&longarg[2])+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(gctmp, &longarg[2]);

				/* Retrieve the short identifier for the long argument */
				if(options_get_id(opt, gctmp, &itmp) == 0) {
					c = itmp;
				} else if(argv[getOptPos][0] == '-') {
					c = shortarg[1];
				}
			} else if(argv[getOptPos][0] == '-' && strstr(shortarg, longarg) != 0) {
				c = shortarg[1];
			}
		}

		/* Check if the argument was expected */
		if(options_get_name(opt, c, &ctmp) != 0 && c > 0) {
			if(error_check == 1) {
				if(strcmp(longarg,shortarg) == 0) {
					if(shortarg[0] == '-') {
						logprintf(LOG_ERR, "invalid option -- '-%c'", c);
					} else {
						logprintf(LOG_ERR, "invalid option -- '%s'", longarg);
					}
				} else {
					logprintf(LOG_ERR, "invalid option -- '%s'", longarg);
				}
				goto gc;
			} else {
				return 0;
			}
		/* Check if an argument cannot have an argument that was set */
		} else if(strlen(*optarg) != 0 && options_get_argtype(opt, c, &itmp) == 0 && itmp == 1) {
			if(error_check == 1) {
				if(strcmp(longarg,shortarg) == 0) {
					logprintf(LOG_ERR, "option '-%c' doesn't take an argument", c);
				} else {
					logprintf(LOG_ERR, "option '%s' doesn't take an argument", longarg);
				}
				goto gc;
			} else {
				return 0;
			}
		/* Check if an argument required a value that wasn't set */
		} else if(strlen(*optarg) == 0 && options_get_argtype(opt, c, &itmp) == 0 && itmp == 2) {
			if(error_check == 1) {
				if(strcmp(longarg, shortarg) == 0) {
					logprintf(LOG_ERR, "option '-%c' requires an argument", c);
				} else {
					logprintf(LOG_ERR, "option '%s' requires an argument", longarg);
				}
				goto gc;
			} else {
				return 0;
			}
		/* Check if we have a valid argument */
		} else if(c == 0) {
			if(error_check == 1) {
				if(shortarg[0] == '-' && strstr(shortarg, longarg) != 0) {
					logprintf(LOG_ERR, "invalid option -- '-%c'", c);
				} else {
					logprintf(LOG_ERR, "invalid option -- '%s'", longarg);
				}
				goto gc;
			} else {
				return 0;
			}
		} else {
			/* If the argument didn't have a value, set it to 1 */
			if(strlen(*optarg) == 0) {
				options_set_string(opt, c, "1");
			} else {
#if !defined(__FreeBSD__) && !defined(_WIN32)
				if(error_check != 2) {
					/* If the argument has a regex mask, check if it passes */
					if(options_get_mask(opt, c, &mask) == 0) {
						reti = regcomp(&regex, mask, REG_EXTENDED);
						if(reti) {
							logprintf(LOG_ERR, "could not compile regex");
							goto gc;
						}
						reti = regexec(&regex, *optarg, 0, NULL, 0);
						if(reti == REG_NOMATCH || reti != 0) {
							if(error_check == 1) {
								if(shortarg[0] == '-') {
									logprintf(LOG_ERR, "invalid format -- '-%c'", c);
								} else {
									logprintf(LOG_ERR, "invalid format -- '%s'", longarg);
								}
								logprintf(LOG_ERR, "requires %s", mask);
							}
							regfree(&regex);
							goto gc;
						}
						regfree(&regex);
					}
				}
#endif
				options_set_string(opt, c, *optarg);
			}
			return c;
		}
	}

gc:
	getOptPos=0;
	FREE(*optarg);

	return -2;
}

/* Add a new option to the options struct */
void options_add(struct options_t **opt, int id, const char *name, int argtype, int conftype, int vartype, void *def, const char *mask) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *ctmp = NULL;
	char *nname = MALLOC(strlen(name)+1);
	int sid = 0;
	if(!nname) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(nname, name);
	int itmp = 0;
	if(!(argtype >= 0 && argtype <= 3)) {
		logprintf(LOG_CRIT, "tying to add an invalid option type");
		FREE(nname);
		exit(EXIT_FAILURE);
	} else if(!(conftype >= 0 && conftype <= NROPTIONTYPES)) {
		logprintf(LOG_CRIT, "trying to add an option of an invalid type");
		FREE(nname);
		exit(EXIT_FAILURE);
	} else if(!name) {
		logprintf(LOG_CRIT, "trying to add an option without name");
		FREE(nname);
		exit(EXIT_FAILURE);
	} else if(id != 0 && options_get_name(opt, id, &ctmp) == 0) {
		logprintf(LOG_CRIT, "duplicate option id: %c", id);
		FREE(nname);
		exit(EXIT_FAILURE);
	} else if(options_get_id(opt, nname, &sid) == 0 &&
			((options_get_conftype(opt, sid, &itmp) == 0 && itmp == conftype) ||
			(options_get_conftype(opt, sid, &itmp) != 0))) {
		logprintf(LOG_CRIT, "duplicate option name: %s", name);
		FREE(nname);
		exit(EXIT_FAILURE);
	} else {
		struct options_t *optnode = MALLOC(sizeof(struct options_t));
		if(!optnode) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		optnode->id = id;
		if((optnode->name = MALLOC(strlen(name)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(optnode->name, name);
		optnode->argtype = argtype;
		optnode->conftype = conftype;
		optnode->vartype = vartype;
		optnode->def = def;
		optnode->string_ = NULL;
		if(mask) {
			if((optnode->mask = MALLOC(strlen(mask)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(optnode->mask, mask);
		} else {
			optnode->mask = NULL;
		}
		optnode->next = *opt;
		*opt = optnode;
		FREE(nname);
	}
}

/* Merge two options structs */
void options_merge(struct options_t **a, struct options_t **b) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *temp = NULL;
	temp = *b;
	while(temp) {
		struct options_t *optnode = MALLOC(sizeof(struct options_t));
		if(!optnode) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		optnode->id = temp->id;
		if(temp->name) {
			if((optnode->name = MALLOC(strlen(temp->name)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			memset(optnode->name, '\0', strlen(temp->name)+1);
			strcpy(optnode->name, temp->name);
		} else {
			optnode->name = NULL;
		}
		if(temp->string_) {
			if((optnode->string_ = MALLOC(strlen(temp->string_)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			optnode->vartype = JSON_STRING;
			strcpy(optnode->string_, temp->string_);
		} else {
			optnode->string_ = NULL;
		}
		if(temp->mask) {
			if((optnode->mask = MALLOC(strlen(temp->mask)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(optnode->mask, temp->mask);
		} else {
			optnode->mask = NULL;
		}
		optnode->argtype = temp->argtype;
		optnode->conftype = temp->conftype;
		optnode->vartype = temp->vartype;
		optnode->def = temp->def;
		optnode->next = *a;
		*a = optnode;
		temp = temp->next;
	}
}

void options_delete(struct options_t *options) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct options_t *tmp;
	while(options) {
		tmp = options;
		if(tmp->mask) {
			FREE(tmp->mask);
		}
		if(tmp->vartype == JSON_STRING && tmp->string_) {
			FREE(tmp->string_);
		}
		FREE(tmp->name);
		options = options->next;
		FREE(tmp);
	}
	if(options != NULL) {
		FREE(options);
	}

	logprintf(LOG_DEBUG, "freed options struct");
}
