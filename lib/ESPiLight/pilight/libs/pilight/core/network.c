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
#ifdef __FreeBSD__
	#include <net/if_dl.h>
#endif

#include "../config/settings.h"
#include "mem.h"
#include "network.h"
#include "log.h"

static unsigned int ***whitelist_cache = NULL;
static unsigned int whitelist_number;

int inetdevs(char ***array) {
	unsigned int nrdevs = 0, i = 0, match = 0;

#ifdef _WIN32
	IP_ADAPTER_INFO *pAdapter = NULL;
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(buflen);

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = MALLOC(buflen);
	}

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			match = 0;

			for(i=0;i<nrdevs;i++) {
				if(strcmp((*array)[i], pAdapter->AdapterName) == 0) {
					match = 1;
					break;
				}
			}
			if(match == 0 && strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0) {
				if((*array = REALLOC(*array, sizeof(char *)*(nrdevs+1))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if(((*array)[nrdevs] = MALLOC(strlen(pAdapter->AdapterName)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy((*array)[nrdevs], pAdapter->AdapterName);
				nrdevs++;
			}
		}
	}
	if(pAdapterInfo != NULL) {
		FREE(pAdapterInfo);
	}
#else
	int family = 0, s = 0;
	char host[NI_MAXHOST];
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;

#ifdef __FreeBSD__
	if(rep_getifaddrs(&ifaddr) == -1) {
		logprintf(LOG_ERR, "could not get network adapter information");
		exit(EXIT_FAILURE);
	}
#else
	if(getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
#endif

	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if(ifa->ifa_addr == NULL) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if((strstr(ifa->ifa_name, "lo") == NULL && strstr(ifa->ifa_name, "vbox") == NULL
		    && strstr(ifa->ifa_name, "dummy") == NULL) && (family == AF_INET || family == AF_INET6)) {
			memset(host, '\0', NI_MAXHOST);

			s = getnameinfo(ifa->ifa_addr,
                           (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                 sizeof(struct sockaddr_in6),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if(s != 0) {
				logprintf(LOG_ERR, "getnameinfo() failed: %s", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			if(strlen(host) > 0) {
				match = 0;
				for(i=0;i<nrdevs;i++) {
					if(strcmp((*array)[i], ifa->ifa_name) == 0) {
						match = 1;
						break;
					}
				}
				if(match == 0) {
					if((*array = REALLOC(*array, sizeof(char *)*(nrdevs+1))) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					if(((*array)[nrdevs] = MALLOC(strlen(ifa->ifa_name)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy((*array)[nrdevs], ifa->ifa_name);
					nrdevs++;
				}
			}
		}
	}

#ifdef __FreeBSD__
	rep_freeifaddrs(ifaddr);
#else
	freeifaddrs(ifaddr);
#endif

#endif // _WIN32
	return (int)nrdevs;
}

int dev2mac(char *ifname, char **mac) {
	memset(*mac, 0, ETH_ALEN);

#ifdef _WIN32
	int i = 0;

	IP_ADAPTER_INFO *pAdapter = NULL;
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = malloc(buflen);

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = malloc(buflen);
	}

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			if(strcmp(ifname, pAdapter->AdapterName) == 0) {
				for(i = 0; i < pAdapter->AddressLength; i++) {
					memcpy(*mac, pAdapter->Address, ETH_ALEN);
					free(pAdapterInfo);
					return 0;
				}
				break;
			}
		}
	}
	free(pAdapterInfo);
#endif

#ifdef linux
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	struct ifreq s;

	if(fd < 0) {
		logprintf(LOG_ERR, "could not open new socket");
		return -1;
	}

	memset(&s, '\0', sizeof(struct ifreq));
	strcpy(s.ifr_name, ifname);
	if(ioctl(fd, SIOCGIFHWADDR, &s) == 0) {
		memcpy(*mac, s.ifr_addr.sa_data, ETH_ALEN);
		close(fd);
		return 0;
	} else {
		logprintf(LOG_ERR, "ioctl SIOCGIFHWADDR failed");
	}
	close(fd);
#endif

#ifdef __FreeBSD__
	struct ifaddrs *ifap = NULL, *ifaptr = NULL;

	if(getifaddrs(&ifap) == 0) {
		for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
			if(strcmp((ifaptr)->ifa_name, ifname) == 0 && (((ifaptr)->ifa_addr)->sa_family == AF_LINK)) {
				memcpy(*mac, (unsigned char *)LLADDR((struct sockaddr_dl *)(ifaptr)->ifa_addr), ETH_ALEN);
				freeifaddrs(ifap);
				return 0;
			}
		}
	}
#endif

	return -1;
}

#ifdef __FreeBSD__
int dev2ip(char *dev, char **ip, __sa_family_t type) {
#else
int dev2ip(char *dev, char **ip, sa_family_t type) {
#endif

#ifdef _WIN32
	IP_ADAPTER_INFO *pAdapter = NULL;
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = MALLOC(buflen);

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = MALLOC(buflen);
	}

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			if(strcmp(dev, pAdapter->AdapterName) == 0) {
				strcpy(*ip, pAdapter->IpAddressList.IpAddress.String);
				break;
			}
		}
	}
	if(pAdapterInfo != NULL) {
		FREE(pAdapterInfo);
	}
#else
	struct ifaddrs *ifaddr, *ifa;
	char host[NI_MAXHOST];
	int family, s, n;

	if(getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we can free list later */
	for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
		if(ifa->ifa_addr == NULL)
			continue;
		family = ifa->ifa_addr->sa_family;
		if(strcmp(ifa->ifa_name, dev) == 0 && family == type) {
			s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if(s != 0) {
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			strcpy(*ip, host);
		}
	}

	freeifaddrs(ifaddr);
#endif

	return 0;
}

int host2ip(char *host, char **ip) {
	int rv = 0;
	struct addrinfo hints, *servinfo, *p;

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "WSAStartup");
		exit(EXIT_FAILURE);
	}
#endif

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(host, NULL , NULL, &servinfo)) != 0) {
		/*LCOV_EXCL_START*/
		logprintf(LOG_NOTICE, "getaddrinfo: %s, %s", host, gai_strerror(rv));
		return -1;
		/*LCOV_EXCL_STOP*/
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if(p->ai_family == AF_INET6) {
			if((*ip = MALLOC(INET6_ADDRSTRLEN+1)) == NULL) {
				OUT_OF_MEMORY
			}
			struct sockaddr_in6 *h = NULL;
			memcpy(&h, &p->ai_addr, sizeof(struct sockaddr_in6 *));
			memset(*ip, '\0', INET6_ADDRSTRLEN+1);
			uv_inet_ntop(p->ai_family, (void *)&(h->sin6_addr), *ip, INET6_ADDRSTRLEN+1);
		} else if(p->ai_family == AF_INET) {
			if((*ip = MALLOC(INET_ADDRSTRLEN+1)) == NULL) {
				OUT_OF_MEMORY
			}
			struct sockaddr_in *h = NULL;
			memcpy(&h, &p->ai_addr, sizeof(struct sockaddr_in *));
			memset(*ip, '\0', INET_ADDRSTRLEN+1);
			uv_inet_ntop(p->ai_family, (void *)&(h->sin_addr), *ip, INET_ADDRSTRLEN+1);
		}
		if(*ip != NULL && strlen(*ip) > 0) {
			int r = p->ai_family;
			freeaddrinfo(servinfo);
			return r;
		}
	}

	freeaddrinfo(servinfo);
	return -1;
}

#ifdef _WIN32
const char *inet_ntop(int af, const void *src, char *dst, int cnt) {
	struct sockaddr_in srcaddr;

	memset(&srcaddr, 0, sizeof(struct sockaddr_in));
	memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

	srcaddr.sin_family = af;
	if(WSAAddressToString((struct sockaddr *)&srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD)&cnt) != 0) {
		DWORD rv = WSAGetLastError();
		printf("WSAAddressToString() : %d\n", (int)rv);
		return NULL;
	}
	return dst;
}

int inet_pton(int af, const char *src, void *dst) {
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN+1];

	ZeroMemory(&ss, sizeof(ss));
	/* stupid non-const API */
	strncpy(src_copy, src, INET6_ADDRSTRLEN+1);
	src_copy[INET6_ADDRSTRLEN] = 0;

	if(WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
		switch(af) {
			case AF_INET:
				*(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
				return 1;
			case AF_INET6:
				*(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
				return 1;
			default:
				return 0;
		}
	}
	return 0;
}
#endif

#ifdef __FreeBSD__
struct sockaddr *sockaddr_dup(struct sockaddr *sa) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct sockaddr *ret;
	socklen_t socklen;
#ifdef HAVE_SOCKADDR_SA_LEN
	socklen = sa->sa_len;
#else
	socklen = sizeof(struct sockaddr_storage);
#endif
	if((ret = CALLOC(1, socklen)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	if (ret == NULL)
		return NULL;
	memcpy(ret, sa, socklen);
	return ret;
}

int rep_getifaddrs(struct ifaddrs **ifap) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct ifconf ifc;
	char buff[8192];
	int fd = 0, i = 0, n = 0;
	struct ifreq ifr, *ifrp = NULL;
	struct ifaddrs *curif = NULL, *ifa = NULL;
	struct ifaddrs *lastif = NULL;

	memset(buff, 0, 8192);

	*ifap = NULL;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}

	memset(&ifc, 0, sizeof(struct ifconf));
	memset(&ifr, 0, sizeof(struct ifreq));

	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;

	if (ioctl(fd, SIOCGIFCONF, &ifc) != 0) {
		close(fd);
		return -1;
	}

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifdef __FreeBSD__
#define ifreq_size(i) max(sizeof(struct ifreq),\
     sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
#else
#define ifreq_size(i) sizeof(struct ifreq)
#endif

	n = ifc.ifc_len;

	for (i = 0; i < n; i+= (int)ifreq_size(*ifrp) ) {
		int match = 0;
		ifrp = (struct ifreq *)((char *) ifc.ifc_buf+i);
		for(ifa = *ifap; ifa != NULL; ifa = ifa->ifa_next) {
			if(strcmp(ifrp->ifr_name, ifa->ifa_name) == 0) {
				match = 1;
				break;
			}
		}
		if(match == 1) {
			continue;
		}
		curif = CALLOC(1, sizeof(struct ifaddrs));
		if(curif == NULL) {
			freeifaddrs(*ifap);
			close(fd);
			return -1;
		}

		curif->ifa_name = MALLOC(IFNAMSIZ+1);
		if(curif->ifa_name == NULL) {
			FREE(curif);
			freeifaddrs(*ifap);
			close(fd);
			return -1;
		}
		memset(curif->ifa_name, 0, IFNAMSIZ+1);
		strncpy(curif->ifa_name, ifrp->ifr_name, IFNAMSIZ);
		strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);

		curif->ifa_flags = (unsigned int)ifr.ifr_flags;
		curif->ifa_dstaddr = NULL;
		curif->ifa_data = NULL;
		curif->ifa_next = NULL;

		curif->ifa_addr = NULL;
		if(ioctl(fd, SIOCGIFADDR, &ifr) != -1) {
			curif->ifa_addr = sockaddr_dup(&ifr.ifr_addr);
			if (curif->ifa_addr == NULL) {
				FREE(curif->ifa_name);
				FREE(curif);
				freeifaddrs(*ifap);
				close(fd);
				return -1;
			}
		}

		curif->ifa_netmask = NULL;
		if (ioctl(fd, SIOCGIFNETMASK, &ifr) != -1) {
			curif->ifa_netmask = sockaddr_dup(&ifr.ifr_addr);
			if (curif->ifa_netmask == NULL) {
				if (curif->ifa_addr != NULL) {
					FREE(curif->ifa_addr);
				}
				FREE(curif->ifa_name);
				FREE(curif);
				freeifaddrs(*ifap);
				close(fd);
				return -1;
			}
		}

		if (lastif == NULL) {
			*ifap = curif;
		} else {
			lastif->ifa_next = curif;
		}
		lastif = curif;
	}

	close(fd);

	return 0;
}

void rep_freeifaddrs(struct ifaddrs *ifaddr) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct ifaddrs *ifa;
	while(ifaddr) {
		ifa = ifaddr;
		FREE(ifa->ifa_name);
		FREE(ifa->ifa_addr);
		FREE(ifa->ifa_netmask);
		ifaddr = ifaddr->ifa_next;
		FREE(ifa);
	}
	FREE(ifaddr);
}
#endif

int whitelist_check(char *ip) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *whitelist = NULL;
	unsigned int client[4] = {0};
	int x = 0, i = 0, error = 1;
	unsigned int n = 0;
	char **array = NULL;
	char wip[16] = {'\0'};

	/* Check if there are any whitelisted ip address */
	if(settings_find_string("whitelist", &whitelist) != 0) {
		return 0;
	}

	if(strlen(whitelist) == 0) {
		return 0;
	}

	/* Explode ip address to a 4 elements int array */
	n = explode(ip, ".", &array);
	x = 0;
	for(x=0;x<n;x++) {
		client[x] = (unsigned int)atoi(array[x]);
		FREE(array[x]);
	}
	if(n > 0) {
		FREE(array);
	}


	if(whitelist_cache == NULL) {
		char *tmp = whitelist;
		x = 0;
		/* Loop through all whitelised ip addresses */
		while(*tmp != '\0') {
			/* Remove any comma's and spaces */
			while(*tmp == ',' || *tmp == ' ') {
				tmp++;
			}
			/* Save ip address in temporary char array */
			wip[x] = *tmp;
			x++;
			tmp++;

			/* Each ip address is either terminated by a comma or EOL delimiter */
			if(*tmp == '\0' || *tmp == ',') {
				x = 0;
				if((whitelist_cache = REALLOC(whitelist_cache, (sizeof(unsigned int ***)*(whitelist_number+1)))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if((whitelist_cache[whitelist_number] = MALLOC(sizeof(unsigned int **)*2)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				/* Lower boundary */
				if((whitelist_cache[whitelist_number][0] = MALLOC(sizeof(unsigned int *)*4)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				/* Upper boundary */
				if((whitelist_cache[whitelist_number][1] = MALLOC(sizeof(unsigned int *)*4)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}

				/* Turn the whitelist ip address into a upper and lower boundary.
				   If the ip address doesn't contain a wildcard, then the upper
				   and lower boundary are the same. If the ip address does contain
				   a wildcard, then this lower boundary number will be 0 and the
				   upper boundary number 255. */
				i = 0;
				n = explode(wip, ".", &array);
				for(i=0;i<n;i++) {
					if(strcmp(array[i], "*") == 0) {
						whitelist_cache[whitelist_number][0][i] = 0;
						whitelist_cache[whitelist_number][1][i] = 255;
					} else {
						whitelist_cache[whitelist_number][0][i] = (unsigned int)atoi(array[i]);
						whitelist_cache[whitelist_number][1][i] = (unsigned int)atoi(array[i]);
					}
					FREE(array[i]);
				}
				if(n > 0) {
					FREE(array);
				}
				memset(wip, '\0', 16);
				whitelist_number++;
			}
		}
	}

	for(x=0;x<whitelist_number;x++) {
		/* Turn the different ip addresses into one single number and compare those
		   against each other to see if the ip address is inside the lower and upper
		   whitelisted boundary */
		unsigned int wlower = whitelist_cache[x][0][0] << 24 | whitelist_cache[x][0][1] << 16 | whitelist_cache[x][0][2] << 8 | whitelist_cache[x][0][3];
		unsigned int wupper = whitelist_cache[x][1][0] << 24 | whitelist_cache[x][1][1] << 16 | whitelist_cache[x][1][2] << 8 | whitelist_cache[x][1][3];
		unsigned int nip = client[0] << 24 | client[1] << 16 | client[2] << 8 | client[3];

		/* Always allow 127.0.0.1 connections */
		if((nip >= wlower && nip <= wupper) || (nip == 2130706433)) {
			error = 0;
		}
	}

	return error;
}

void whitelist_free(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int i = 0;
	if(whitelist_cache) {
		for(i=0;i<whitelist_number;i++) {
			FREE(whitelist_cache[i][0]);
			FREE(whitelist_cache[i][1]);
			FREE(whitelist_cache[i]);
		}
		FREE(whitelist_cache);
	}
}
