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

#ifndef __FreeBSD__
	#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#ifdef _WIN32
	#if _WIN32_WINNT < 0x0501
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#include <winsock2.h>
	#include <windows.h>
	#include <psapi.h>
	#include <tlhelp32.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
#else
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
	#include <net/if.h>
	#include <ifaddrs.h>
	#include <pwd.h>
	#include <sys/mount.h>
	#include <sys/ioctl.h>
#endif

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifndef __USE_XOPEN
	#define __USE_XOPEN
#endif
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include "../config/settings.h"
#include "mem.h"
#include "common.h"
#include "network.h"
#include "log.h"

char *progname = NULL;
#ifndef _WIN32
static int procmounted = 0;
#endif

static const char base64table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static pthread_mutex_t atomic_lock;
static pthread_mutexattr_t atomic_attr;

void atomicinit(void) {
	pthread_mutexattr_init(&atomic_attr);
	pthread_mutexattr_settype(&atomic_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&atomic_lock, &atomic_attr);
}

void atomiclock(void) {
	pthread_mutex_lock(&atomic_lock);
}

void atomicunlock(void) {
	pthread_mutex_unlock(&atomic_lock);
}

void array_free(char ***array, int len) {
	int i = 0;
	if(len > 0) {
		for(i=0;i<len;i++) {
			FREE((*array)[i]);
		}
		FREE((*array));
	}
}

unsigned int explode(const char *str, const char *delimiter, char ***output) {
	if(str == NULL || output == NULL) {
		return 0;
	}
	unsigned int i = 0, n = 0, y = 0;
	size_t l = 0, p = 0;
	if(delimiter != NULL) {
		l = strlen(str);
		p = strlen(delimiter);
	}
	while(i < l) {
		if(strncmp(&str[i], delimiter, p) == 0) {
			if((i-y) > 0) {
				if((*output = REALLOC(*output, sizeof(char *)*(n+1))) == NULL) {
					OUT_OF_MEMORY
				}
				if(((*output)[n] = MALLOC((i-y)+1)) == NULL) {
					OUT_OF_MEMORY
				}
				strncpy((*output)[n], &str[y], i-y);
				(*output)[n][(i-y)] = '\0';
				n++;
			}
			y=i+p;
		}
		i++;
	}
	if(strlen(&str[y]) > 0) {
		if((*output = REALLOC(*output, sizeof(char *)*(n+1))) == NULL) {
			OUT_OF_MEMORY
		}
		if(((*output)[n] = MALLOC((i-y)+1)) == NULL) {
			OUT_OF_MEMORY
		}
		strncpy((*output)[n], &str[y], i-y);
		(*output)[n][(i-y)] = '\0';
		n++;
	}
	return n;
}

#ifdef _WIN32
int check_instances(const wchar_t *prog) {
	HANDLE m_hStartEvent = CreateEventW(NULL, FALSE, FALSE, prog);
	if(m_hStartEvent == NULL) {
		CloseHandle(m_hStartEvent);
		return 0;
	}

	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(m_hStartEvent);
		m_hStartEvent = NULL;
		return 0;
	}
	return -1;
}

int setenv(const char *name, const char *value, int overwrite) {
	if(name == NULL) {
		errno = EINVAL;
		return -1;
	}
	if(overwrite == 0 && getenv(name) != NULL) {
		return 0; // name already defined and not allowed to overwrite. Treat as OK.
	}
	if(value == NULL) {
		return unsetenv(name);
	}
	char c[strlen(name)+1+strlen(value)+1]; // one for "=" + one for term zero
	strcat(c, name);
	strcat(c, "=");
	strcat(c, value);
	return putenv(c);
}

int unsetenv(const char *name) {
	if(name == NULL) {
		errno = EINVAL;
		return -1;
	}
	char c[strlen(name)+1+1]; // one for "=" + one for term zero
	strcat(c, name);
	strcat(c, "=");
	return putenv(c);
}

int isrunning(const char *program) {
	DWORD aiPID[1000], iCb = 1000;
	DWORD iCbneeded = 0;
	int iNumProc = 0, i = 0;
	char szName[MAX_PATH];
	int iLenP = 0;
	HANDLE hProc;
	HMODULE hMod;

	iLenP = strlen(program);
	if(iLenP < 1 || iLenP > MAX_PATH)
		return -1;

	if(EnumProcesses(aiPID, iCb, &iCbneeded) <= 0) {
		return -1;
	}

	iNumProc = iCbneeded / sizeof(DWORD);

	for(i=0;i<iNumProc;i++) {
		strcpy(szName, "Unknown");
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aiPID[i]);

		if(hProc) {
			if(EnumProcessModules(hProc, &hMod, sizeof(hMod), &iCbneeded)) {
				GetModuleBaseName(hProc, hMod, szName, MAX_PATH);
			}
		}
		CloseHandle(hProc);

		if(strstr(szName, program) != NULL) {
			return aiPID[i];
		}
	}

	return -1;
}
#else
int isrunning(const char *program) {
	int pid = -1;
	char *tmp = MALLOC(strlen(program)+1);
	if(tmp == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(tmp, program);
	if((pid = findproc(tmp, NULL, 1)) > 0) {
		FREE(tmp);
		return pid;
	}
	FREE(tmp);
	return -1;
}
#endif

#ifdef __FreeBSD__
int findproc(char *cmd, char *args, int loosely) {
#else
pid_t findproc(char *cmd, char *args, int loosely) {
#endif
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

#ifndef _WIN32
	DIR* dir;
	struct dirent* ent;
	char fname[512], cmdline[1024];
	int fd = 0, ptr = 0, match = 0, i = 0, y = '\n', x = 0;

	if(procmounted == 0) {
		if((dir = opendir("/proc"))) {
			i = 0;
			while((ent = readdir(dir)) != NULL) {
				i++;
			}
			closedir(dir);
			if(i == 2) {
#ifdef __FreeBSD__
				mount("procfs", "/proc", 0, "");
#else
				mount("proc", "/proc", "procfs", 0, "");
#endif
				if((dir = opendir("/proc"))) {
					i = 0;
					while((ent = readdir(dir)) != NULL) {
						i++;
					}
					closedir(dir);
					if(i == 2) {
						logprintf(LOG_ERR, "/proc filesystem not properly mounted");
						return -1;
					}
				}
			}
		} else {
			logprintf(LOG_ERR, "/proc filesystem not properly mounted");
			return -1;
		}
		procmounted = 1;
	}
	if((dir = opendir("/proc"))) {
		while((ent = readdir(dir)) != NULL) {
			if(isNumeric(ent->d_name) == 0) {
				snprintf(fname, 512, "/proc/%s/cmdline", ent->d_name);
				if((fd = open(fname, O_RDONLY, 0)) > -1) {
					memset(cmdline, '\0', sizeof(cmdline));
					if((ptr = (int)read(fd, cmdline, sizeof(cmdline)-1)) > -1) {
						i = 0, match = 0, y = '\n';
						/* Replace all NULL terminators for newlines */
						for(i=0;i<ptr-1;i++) {
							if(i < ptr && cmdline[i] == '\0') {
								cmdline[i] = (char)y;
								y = ' ';
							}
						}

						match = 0;
						/* Check if program matches */
						char **array = NULL;
						unsigned int n = explode(cmdline, "\n", &array);

						if(n == 0) {
							close(fd);
							continue;
						}
						if((strcmp(array[0], cmd) == 0 && loosely == 0)
							 || (strstr(array[0], cmd) != NULL && loosely == 1)) {
							match++;
						}

						if(args != NULL && match == 1) {
							if(n <= 1) {
								close(fd);
								for(x=0;x<n;x++) {
									FREE(array[x]);
								}
								if(n > 0) {
									FREE(array);
								}
								continue;
							}
							if(strcmp(array[1], args) == 0) {
								match++;
							}

							if(match == 2) {
								pid_t pid = (pid_t)atol(ent->d_name);
								close(fd);
								closedir(dir);
								for(x=0;x<n;x++) {
									FREE(array[x]);
								}
								if(n > 0) {
									FREE(array);
								}
								return pid;
							}
						} else if(match > 0) {
							pid_t pid = (pid_t)atol(ent->d_name);
							close(fd);
							closedir(dir);
							for(x=0;x<n;x++) {
								FREE(array[x]);
							}
							if(n > 0) {
								FREE(array);
							}
							return pid;
						}
						for(x=0;x<n;x++) {
							FREE(array[x]);
						}
						if(n > 0) {
							FREE(array);
						}
					}
					close(fd);
				}
			}
		}
		closedir(dir);
	}
#endif
	return -1;
}

int isNumeric(char *s) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(s == NULL || *s == '\0' || *s == ' ')
		return -1;

	char *p = NULL;
	strtod(s, &p);
	return (*p == '\0') ? 0 : -1;
}

int nrDecimals(char *s) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	unsigned int b = 0, c = strlen(s), i = 0;
	int a = 0;
	for(i=0;i<c;i++) {
		if(b == 1) {
			a++;
		}
		if(s[i] == '.') {
			b = 1;
		}
	}
	return a;
}

int name2uid(char const *name) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

#ifndef _WIN32
	if(name != NULL) {
		struct passwd *pwd = getpwnam(name); /* don't free, see getpwnam() for details */
		if(pwd) {
			return (int)pwd->pw_uid;
		}
	}
#endif
	return -1;
}

int which(const char *program) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char path[1024];
	strcpy(path, getenv("PATH"));
	char **array = NULL;
	unsigned int n = 0, i = 0;
	int found = -1;

	n = explode(path, ":", &array);
	for(i=0;i<n;i++) {
		char exec[strlen(array[i])+8];
		strcpy(exec, array[i]);
		strcat(exec, "/");
		strcat(exec, program);

		if(access(exec, X_OK) != -1) {
			found = 0;
			break;
		}
	}
	for(i=0;i<n;i++) {
		FREE(array[i]);
	}
	if(n > 0) {
		FREE(array);
	}
	return found;
}

int ishex(int x) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	return(x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F');
}

const char *rstrstr(const char* haystack, const char* needle) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char* loc = 0;
	char* found = 0;
	size_t pos = 0;

	while ((found = strstr(haystack + pos, needle)) != 0) {
		loc = found;
		pos = (size_t)((found - haystack) + 1);
	}

	return loc;
}

void alpha_random(char *s, const int len) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
	int i = 0;

	for(i = 0; i < len; ++i) {
			s[i] = alphanum[(unsigned int)rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}

int urldecode(const char *s, char *dec) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *o = NULL;
	const char *end = s + strlen(s);
	int c = 0;

	for(o = dec; s <= end; o++) {
		c = *s++;
		if(c == '+') {
			c = ' ';
		} else if(c == '%' && (!ishex(*s++) || !ishex(*s++)	|| !sscanf(s - 2, "%2x", &c))) {
			return -1;
		}
		if(dec) {
			sprintf(o, "%c", c);
		}
	}

	return (int)(o - dec);
}

static char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

char *urlencode(char *str) {
	char *pstr = str, *buf = MALLOC(strlen(str) * 3 + 1), *pbuf = buf;
	while(*pstr) {
		if(isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
			*pbuf++ = *pstr;
		else if(*pstr == ' ')
			*pbuf++ = '+';
		else
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

char *base64decode(char *src, size_t len, size_t *decsize) {
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int l = 0;
  size_t size = 0;
  char *dec = NULL;
  char buf[3];
  char tmp[4];

  dec = MALLOC(0);
  if(dec == NULL) {
		return NULL;
	}

  while(len--) {
    if('=' == src[j]) {
			break;
		}
    if(!(isalnum(src[j]) || src[j] == '+' || src[j] == '/')) {
			break;
		}

    tmp[i++] = src[j++];

    if(i == 4) {
      for(i = 0; i < 4; ++i) {
        for(l = 0; l < 64; ++l) {
          if(tmp[i] == base64table[l]) {
            tmp[i] = (char)l;
            break;
          }
        }
      }

      buf[0] = (char)((tmp[0] << 2) + ((tmp[1] & 0x30) >> 4));
      buf[1] = (char)(((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2));
      buf[2] = (char)(((tmp[2] & 0x3) << 6) + tmp[3]);

      dec = REALLOC(dec, size + 3);
      for(i = 0; i < 3; ++i) {
        dec[size++] = buf[i];
      }

      i = 0;
    }
  }

  if(i > 0) {
    for(j = i; j < 4; ++j) {
      tmp[j] = '\0';
    }

    for(j = 0; j < 4; ++j) {
			for(l = 0; l < 64; ++l) {
				if(tmp[j] == base64table[l]) {
					tmp[j] = (char)l;
					break;
				}
			}
    }

    buf[0] = (char)((tmp[0] << 2) + ((tmp[1] & 0x30) >> 4));
    buf[1] = (char)(((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2));
    buf[2] = (char)(((tmp[2] & 0x3) << 6) + tmp[3]);

    dec = REALLOC(dec, (size_t)(size + (size_t)(i - 1)));
    for(j = 0; (j < i - 1); ++j) {
      dec[size++] = buf[j];
    }
  }

  dec = REALLOC(dec, size + 1);
  dec[size] = '\0';

  if(decsize != NULL) {
		*decsize = size;
	}

  return dec;
}

char *base64encode(char *src, size_t len) {
  unsigned int i = 0;
  unsigned int j = 0;
  char *enc = NULL;
  size_t size = 0;
  char buf[4];
  char tmp[3];

  enc = MALLOC(0);
  if(enc == NULL) {
		return NULL;
	}

  while(len--) {
    tmp[i++] = *(src++);

    if(i == 3) {
      buf[0] = (char)((tmp[0] & 0xfc) >> 2);
      buf[1] = (char)(((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4));
      buf[2] = (char)(((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6));
      buf[3] = (char)(tmp[2] & 0x3f);

      enc = REALLOC(enc, size + 4);
      for(i = 0; i < 4; ++i) {
        enc[size++] = base64table[(int)buf[i]];
      }

      i = 0;
    }
  }

  if(i > 0) {
    for(j = i; j < 3; ++j) {
      tmp[j] = '\0';
    }

		buf[0] = (char)((tmp[0] & 0xfc) >> 2);
		buf[1] = (char)(((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4));
		buf[2] = (char)(((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6));
		buf[3] = (char)(tmp[2] & 0x3f);

    for(j = 0; (j < i + 1); ++j) {
      enc = REALLOC(enc, size+1);
      enc[size++] = base64table[(int)buf[j]];
    }

    while((i++ < 3)) {
      enc = REALLOC(enc, size+1);
      enc[size++] = '=';
    }
  }

  enc = REALLOC(enc, size+1);
  enc[size] = '\0';

  return enc;
}

void rmsubstr(char *s, const char *r) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	while((s=strstr(s, r))) {
		size_t l = strlen(r);
		memmove(s, s+l, 1+strlen(s+l));
	}
}

char *hostname(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char name[255] = {'\0'};
	char *host = NULL, **array = NULL;
	unsigned int n = 0, i = 0;

	gethostname(name, 254);
	if(strlen(name) > 0) {
		n = explode(name, ".", &array);
		if(n > 0) {
			if((host = MALLOC(strlen(array[0])+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(host, array[0]);
		}
	}
	for(i=0;i<n;i++) {
		FREE(array[i]);
	}
	if(n > 0) {
		FREE(array);
	}
	return host;
}

char *distroname(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char dist[32];
	memset(dist, '\0', 32);
	char *distro = NULL;

#ifdef _WIN32
	strcpy(dist, "Windows");
#elif defined(__FreeBSD__)
	strcpy(dist, "FreeBSD/0.0");
#else
	int rc = 1;
	struct stat sb;
	if((rc = stat("/etc/redhat-release", &sb)) == 0) {
		strcpy(dist, "RedHat/0.0");
	} else if((rc = stat("/etc/SuSE-release", &sb)) == 0) {
		strcpy(dist, "SuSE/0.0");
	} else if((rc = stat("/etc/mandrake-release", &sb)) == 0) {
		strcpy(dist, "Mandrake/0.0");
	} else if((rc = stat("/etc/debian-release", &sb)) == 0) {
		strcpy(dist, "Debian/0.0");
	} else if((rc = stat("/etc/debian_version", &sb)) == 0) {
		strcpy(dist, "Debian/0.0");
	} else {
		strcpy(dist, "Unknown/0.0");
	}
#endif
	if(strlen(dist) > 0) {
		if((distro = MALLOC(strlen(dist)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(distro, dist);
		return distro;
	} else {
		return NULL;
	}
}

/* The UUID is either generated from the
   processor serial number or from the
   onboard LAN controller mac address */
char *genuuid(char *ifname) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char mac[ETH_ALEN], *upnp_id = NULL, *p = mac;
	char serial[UUID_LENGTH+1];

	memset(serial, '\0', UUID_LENGTH+1);
#ifndef _WIN32
	char a[1024];
	FILE *fp = fopen("/proc/cpuinfo", "r");
	if(fp != NULL) {
		while(!feof(fp)) {
			if(fgets(a, 1024, fp) == 0) {
				break;
			}
			if(strstr(a, "Serial") != NULL) {
				sscanf(a, "Serial          : %16s%*[ \n\r]", (char *)&serial);
				if(strlen(serial) > 0 &&
					 ((isNumeric(serial) == EXIT_SUCCESS && atoi(serial) > 0) ||
					  (isNumeric(serial) == EXIT_FAILURE))) {
					memmove(&serial[5], &serial[4], 16);
					serial[4] = '-';
					memmove(&serial[8], &serial[7], 13);
					serial[7] = '-';
					memmove(&serial[11], &serial[10], 10);
					serial[10] = '-';
					memmove(&serial[14], &serial[13], 7);
					serial[13] = '-';
					if((upnp_id = MALLOC(UUID_LENGTH+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(upnp_id, serial);
					fclose(fp);
					return upnp_id;
				}
			}
		}
		fclose(fp);
	}

#endif
	if(dev2mac(ifname, &p) == 0) {
		if((upnp_id = MALLOC(UUID_LENGTH+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		memset(upnp_id, '\0', UUID_LENGTH+1);
		snprintf(upnp_id, UUID_LENGTH,
				"0000-%02x-%02x-%02x-%02x%02x%02x",
				(unsigned char)p[0], (unsigned char)p[1], (unsigned char)p[2],
				(unsigned char)p[3], (unsigned char)p[4], (unsigned char)p[5]);
		return upnp_id;
	}

	return NULL;
}

/* Check if a given file exists */
int file_exists(char *filename) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct stat sb;
	return stat(filename, &sb);
}

/* Check if a given path exists */
int path_exists(char *fil) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct stat s;
	char tmp[strlen(fil)+1];
	strcpy(tmp, fil);

	atomiclock();
	/* basename isn't thread safe */
	char *filename = basename(tmp);
	atomicunlock();

	char path[(strlen(tmp)-strlen(filename))+1];
	size_t i = (strlen(tmp)-strlen(filename));

	memset(path, '\0', sizeof(path));
	memcpy(path, tmp, i);
	snprintf(path, i, "%s", tmp);

/*
 * dir stat doens't work on windows if path has a trailing slash
 */
#ifdef _WIN32
	if(path[i-1] == '\\' || path[i-1] == '/') {
		path[i-1] = '\0';
	}
#endif

	if(strcmp(filename, tmp) != 0) {
		int err = stat(path, &s);
		if(err == -1) {
			if(ENOENT == errno) {
				return EXIT_FAILURE;
			} else {
				return EXIT_FAILURE;
			}
		} else {
			if(S_ISDIR(s.st_mode)) {
				return EXIT_SUCCESS;
			} else {
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}

/* Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk> */
/* Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk> */
//  1: val > ref
// -1: val < ref
//  0: val == ref
int vercmp(char *val, char *ref) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int vc, rc;
	long vl, rl;
	char *vp, *rp;
	char *vsep, *rsep;

	if(!val) {
		strcpy(val, "");
	}
	if(!ref) {
		strcpy(ref, "");
	}
	while(1) {
		vp = val;
		while(*vp && !isdigit(*vp)) {
			vp++;
		}
		rp = ref;
		while(*rp && !isdigit(*rp)) {
			rp++;
		}
		while(1) {
			vc =(val == vp) ? 0 : *val++;
			rc =(ref == rp) ? 0 : *ref++;
			if(!rc && !vc) {
				break;
			}
			if(vc && !isalpha(vc)) {
				vc += 256;
			}
			if(rc && !isalpha(rc)) {
				rc += 256;
			}
			if(vc != rc) {
				return vc - rc;
			}
		}
		val = vp;
		ref = rp;
		vl = 0;
		if(isdigit(*vp)) {
			vl = strtol(val, (char**)&val, 10);
		}
		rl = 0;
		if(isdigit(*rp)) {
			rl = strtol(ref, (char**)&ref, 10);
		}
		if(vl != rl) {
			return (int)(vl - rl);
		}

		vc = *val;
		rc = *ref;
		vsep = strchr(".-", vc);
		rsep = strchr(".-", rc);

		if((vsep && !rsep) || !*val) {
			return 0;
		}

		if((!vsep && rsep) || !*ref) {
			return +1;
		}

		if(!*val && !*ref) {
			return 0;
		}
	}
}

char *uniq_space(char *str){
	char *from = NULL, *to = NULL;
	int spc=0;
	to = from = str;
	while(1){
		if(spc == 1 && *from == ' ' && to[-1] == ' ') {
			++from;
		} else {
			spc = (*from == ' ') ? 1 : 0;
			*to++ = *from++;
			if(!to[-1]) {
				break;
			}
		}
	}
	return str;
}

int str_replace(char *search, char *replace, char **str) {
	unsigned short match = 0;
	int len = (int)strlen(*str);
	int nlen = 0;
	int slen = (int)strlen(search);
	int rlen = (int)strlen(replace);
	int x = 0;
	while(x < len) {
		if(strncmp(&(*str)[x], search, (size_t)slen) == 0) {
			match = 1;
			int rpos = (x + (slen - rlen));
			if(rpos < 0) {
				slen -= rpos;
				rpos = 0;
			}
			nlen = len - (slen - rlen);
			if(len < nlen) {
				if(((*str) = REALLOC((*str), (size_t)nlen+1)) == NULL) { /*LCOV_EXCL_LINE*/
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				memset(&(*str)[len], '\0', (size_t)(nlen-len));
			}
			len = nlen;
			memmove(&(*str)[x], &(*str)[rpos], (size_t)(len-x));
			strncpy(&(*str)[x], replace, (size_t)rlen);
			(*str)[len] = '\0';
			x += rlen-1;
		}
		x++;
	}
	if(match == 1) {
		return (int)len;
	} else {
		return -1;
	}
}

int strnicmp(char const *a, char const *b, size_t len) {
	int i = 0;

	if(a == NULL || b == NULL) {
		return -1;
	}
	if(len == 0) {
		return 0;
	}

	for(;i++<len; a++, b++) {
		int d = tolower(*a) - tolower(*b);
		if(d != 0 || !*a || i == len) {
			return d;
		}
	}
	return -1;
}

int stricmp(char const *a, char const *b) {
	if(a == NULL || b == NULL) {
		return -1;
	}

	for(;; a++, b++) {
			int d = tolower(*a) - tolower(*b);
			if(d != 0 || !*a)
				return d;
	}
}

int file_get_contents(char *file, char **content) {
	FILE *fp = NULL;
	size_t bytes = 0;
	struct stat st;

	if((fp = fopen(file, "rb")) == NULL) {
		logprintf(LOG_ERR, "cannot open file: %s", file);
		return -1;
	}

	fstat(fileno(fp), &st);
	bytes = (size_t)st.st_size;

	if((*content = CALLOC(bytes+1, sizeof(char))) == NULL) {
		fprintf(stderr, "out of memory\n");
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	if(fread(*content, sizeof(char), bytes, fp) == -1) {
		logprintf(LOG_ERR, "cannot read file: %s", file);
		return -1;
	}
	fclose(fp);
	return 0;
}

char *str_trim_right(char *str, const char *trim_away) {
	char *end = str;
	if (str != NULL && trim_away != NULL && *trim_away != '\0') {
		for (end += strlen(str); end > str && strchr(trim_away, end[-1]) != NULL; --end)
			;
		*end = '\0';
	}
	return str;
}

int checkdnsrr(const char *domain, const char *type)
{
	logprintf(LOG_STACK, "%s(%s,%s)", __FUNCTION__, domain ? domain : "(NULL)", type ? type : "(NULL)");
	return 0;	// NYI.
}

/**
 * Check if an email address is valid.
 * @param char* address The email address to check. Leading+trailing white spaces are accepted.
 * @param bool allow_lists If true, addr is allowed to be a comma separated list of email addresses.
 * @param bool check_domain_can_mail If true, check if domain exists in DNS system and accepts email.
 * @return int Values < 0 indicate error. Values >= 0 indicate success.
 * Where -1: syntax error, -2: DNS check for a domain failed, -8: severe internal error.
 */
int check_email_addr(const char *address, int allow_lists, int check_domain_can_mail) {
	if(address == NULL) {
		return -1;
	}

	#define ALPHANUMERICS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

	static const char white_spaces[] = " \t";
	static const char local_valid_chars[] = ALPHANUMERICS ".!#$%&'*+-\\/=?^_`{|}~";
	static const char domain_valid_chars[] = ALPHANUMERICS ".-"; // . and - are not allowed everywhere.

	const char *at = NULL, *domain_name = NULL;
	char *addr = NULL;
	char **addr_list = NULL;
	size_t addr_count = explode(address, ",", &addr_list), ii = 0;
	int ret = -1;
	for(ii = 0; ii < addr_count; ii++) {
		ret = -1;	// assume error
		if (allow_lists == 0 && addr_count > 1) {
			break;	// only one address allowed.
		}
		addr = addr_list[ii];
		addr += strspn(addr, white_spaces);	// trim left
		str_trim_right(addr, white_spaces);

		// Now check addr. First some quick pre-checks:
		at = strchr(addr, '@');
		if(at == NULL || strstr(addr, "..") != NULL) {
			break;	// no @ found or two consecutive dots found somewhere.
		}
		domain_name = at+1;

		if(at == addr || at[-1] == '.' || addr[0] == '.') {
			break;	// local part emtpy, or starts or ends with a dot.
		}
		if(addr+strspn(addr, local_valid_chars) < at) {
			break;	// local part contains invalid chars.
		}
		if(strchr(domain_name, '.') == NULL || strlen(domain_name) < 5) {
			break;	// not at least one dot in domain or domain is not at least 2 x 2 chars.
		}
		if(strchr(".-", domain_name[0]) != NULL || strchr(".-", domain_name[strlen(domain_name)-1]) != NULL) {
			break;	// domain starts or ends with a dot or dash.
		}
		if(strstr(domain_name, ".-") != NULL || strstr(domain_name, "-.") != NULL) {
			break;	// a dot delimited domain part starts or ends with a dash.
		}
		if(domain_name[strspn(domain_name, domain_valid_chars)] != '\0') {
			break;	// domain contains invalid chars.
		}

		// Finally check domain name against DNS if desired:

		if(check_domain_can_mail != 0 && checkdnsrr(domain_name, "MX") < 0) {
			ret = -2;
			break;	// domain not found or does not accept emails.
		}

		ret = 0; // Address ok (and all ok if this is the last one in the list).
	}
	array_free(&addr_list, addr_count);
	return ret;

	#undef ALPHANUMERICS
}
