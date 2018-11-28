/*
 * The ARP Scanner (arp-scan) is Copyright (C) 2005-2013 Roy Hills,
 * NTA Monitor Ltd.
 *
 * This file is part of arp-scan.
 *
 * arp-scan is FREE software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the FREE Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * arp-scan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with arp-scan.  If not, see <http://www.gnu.org/licenses/>.
 *
 * arp-scan -- The ARP Scanner
 *
 * Author:	Roy Hills
 * Date:	13 October 2005
 *
 * Usage:
 *    arp-scan [options] [host...]
 *
 * Description:
 *
 * arp-scan sends the specified ARP packet to the specified hosts
 * and displays any responses received.
 *
 * The ARP protocol is defined in RFC 826 Ethernet Address Resolution Protocol
 *
 * arp-scan was stripped and adapter by CurlyMo 2014 for pilight.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#ifdef _WIN32
	#define WPCAP
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <windows.h>
	#include <psapi.h>
	#include <tlhelp32.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
	#define MSG_NOSIGNAL 0
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/if_ether.h>
	#include <netinet/tcp.h>
	#include <net/route.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <net/if.h>
	#ifdef __FREEBSD__
		#include <net/if_dl.h>
		#include <net/if_types.h>
	#else
		#include <sys/ioctl.h>
	#endif
	#include <regex.h>
	#include <netdb.h>
#endif
#include <pcap.h>

#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include "network.h"
#include "mem.h"
#include "log.h"
#include "arp.h"

#define MAXLINE 						255
#define MAX_FRAME 					2048
#define PACKET_OVERHEAD 		18
#define MINIMUM_FRAME_SIZE 	46
#define ETHER_HDR_SIZE 			14
#define ARP_PKT_SIZE 				28
#define FRAMING_ETHERNET_II	0
#define FRAMING_LLC_SNAP		1

typedef struct host_entry {
   double timeout;
   struct in_addr addr;
   struct timeval last_send_time;
   unsigned short num_sent;
   unsigned short num_recv;
   unsigned short found;
   unsigned char live;
	 uint8_t mac[ETH_ALEN];
} host_entry;

typedef struct ether_hdr {
	uint8_t dest_addr[ETH_ALEN];
	uint8_t src_addr[ETH_ALEN];
	uint16_t frame_type;
} ether_hdr;

typedef struct arp_ether_ipv4 {
	uint16_t ar_hrd;
	uint16_t ar_pro;
	uint8_t ar_hln;
	uint8_t ar_pln;
	uint16_t ar_op;
	uint8_t ar_sha[ETH_ALEN];
	uint32_t ar_sip;
	uint8_t ar_tha[ETH_ALEN];
	uint32_t ar_tip;
} arp_ether_ipv4;

static struct host_entry **helist;
static struct host_entry **cursor;
static unsigned num_hosts = 0;
static unsigned live_count = 0;

static void advance_cursor(void) {
	if(live_count > 0) {
		do {
			if(cursor == (helist+(num_hosts-1))) {
				cursor = helist;
			} else {
				cursor++;
			}
		} while(!(*cursor)->live);
	}
}

static void remove_host(host_entry **he) {
  if((*he)->live == 1) {
		(*he)->live = 0;
		live_count--;
		if(*he == *cursor) {
			advance_cursor();
		}
	}
}

static void timeval_diff(const struct timeval *a, const struct timeval *b, struct timeval *diff) {
	struct timeval temp;

	temp.tv_sec = b->tv_sec;
	temp.tv_usec = b->tv_usec;

	if(a->tv_usec < temp.tv_usec) {
		int nsec = (temp.tv_usec - a->tv_usec) / 1000000 + 1;
		temp.tv_usec -= 1000000 * nsec;
		temp.tv_sec += nsec;
	}
	if(a->tv_usec - temp.tv_usec > 1000000) {
		int nsec = (a->tv_usec - temp.tv_usec) / 1000000;
		temp.tv_usec += 1000000 * nsec;
		temp.tv_sec -= nsec;
	}

	diff->tv_sec = a->tv_sec - temp.tv_sec;
	diff->tv_usec = a->tv_usec - temp.tv_usec;
}


static void marshal_arp_pkt(unsigned char *buffer, ether_hdr *frame_hdr, arp_ether_ipv4 *arp_pkt, int *buf_siz) {
	unsigned char *cp;
	int packet_size;

	cp = buffer;

	packet_size = sizeof(frame_hdr->dest_addr) + sizeof(frame_hdr->src_addr) +
							 sizeof(frame_hdr->frame_type) +
							 sizeof(arp_pkt->ar_hrd) + sizeof(arp_pkt->ar_pro) +
							 sizeof(arp_pkt->ar_hln) + sizeof(arp_pkt->ar_pln) +
							 sizeof(arp_pkt->ar_op)  + sizeof(arp_pkt->ar_sha) +
							 sizeof(arp_pkt->ar_sip) + sizeof(arp_pkt->ar_tha) +
							 sizeof(arp_pkt->ar_tip);

	memcpy(cp, &(frame_hdr->dest_addr), sizeof(frame_hdr->dest_addr));
	cp += sizeof(frame_hdr->dest_addr);
	memcpy(cp, &(frame_hdr->src_addr), sizeof(frame_hdr->src_addr));
	cp += sizeof(frame_hdr->src_addr);

	memcpy(cp, &(frame_hdr->frame_type), sizeof(frame_hdr->frame_type));
	cp += sizeof(frame_hdr->frame_type);

	memcpy(cp, &(arp_pkt->ar_hrd), sizeof(arp_pkt->ar_hrd));
	cp += sizeof(arp_pkt->ar_hrd);
	memcpy(cp, &(arp_pkt->ar_pro), sizeof(arp_pkt->ar_pro));
	cp += sizeof(arp_pkt->ar_pro);
	memcpy(cp, &(arp_pkt->ar_hln), sizeof(arp_pkt->ar_hln));
	cp += sizeof(arp_pkt->ar_hln);
	memcpy(cp, &(arp_pkt->ar_pln), sizeof(arp_pkt->ar_pln));
	cp += sizeof(arp_pkt->ar_pln);
	memcpy(cp, &(arp_pkt->ar_op), sizeof(arp_pkt->ar_op));
	cp += sizeof(arp_pkt->ar_op);
	memcpy(cp, &(arp_pkt->ar_sha), sizeof(arp_pkt->ar_sha));
	cp += sizeof(arp_pkt->ar_sha);
	memcpy(cp, &(arp_pkt->ar_sip), sizeof(arp_pkt->ar_sip));
	cp += sizeof(arp_pkt->ar_sip);
	memcpy(cp, &(arp_pkt->ar_tha), sizeof(arp_pkt->ar_tha));
	cp += sizeof(arp_pkt->ar_tha);
	memcpy(cp, &(arp_pkt->ar_tip), sizeof(arp_pkt->ar_tip));
	cp += sizeof(arp_pkt->ar_tip);

	*buf_siz = packet_size;
}

static int unmarshal_arp_pkt(const unsigned char *buffer, size_t buf_len, ether_hdr *frame_hdr, arp_ether_ipv4 *arp_pkt, unsigned char *extra_data, size_t *extra_data_len) {
	const unsigned char *cp;
	int framing = FRAMING_ETHERNET_II;
	cp = buffer;

	memcpy(&(frame_hdr->dest_addr), cp, sizeof(frame_hdr->dest_addr));
	cp += sizeof(frame_hdr->dest_addr);
	memcpy(&(frame_hdr->src_addr), cp, sizeof(frame_hdr->src_addr));
	cp += sizeof(frame_hdr->src_addr);

	memcpy(&(frame_hdr->frame_type), cp, sizeof(frame_hdr->frame_type));
	cp += sizeof(frame_hdr->frame_type);

	if(*cp == 0xAA && *(cp+1) == 0xAA && *(cp+2) == 0x03) {
		cp += 8;
		framing = FRAMING_LLC_SNAP;
	}

	memcpy(&(arp_pkt->ar_hrd), cp, sizeof(arp_pkt->ar_hrd));
	cp += sizeof(arp_pkt->ar_hrd);
	memcpy(&(arp_pkt->ar_pro), cp, sizeof(arp_pkt->ar_pro));
	cp += sizeof(arp_pkt->ar_pro);
	memcpy(&(arp_pkt->ar_hln), cp, sizeof(arp_pkt->ar_hln));
	cp += sizeof(arp_pkt->ar_hln);
	memcpy(&(arp_pkt->ar_pln), cp, sizeof(arp_pkt->ar_pln));
	cp += sizeof(arp_pkt->ar_pln);
	memcpy(&(arp_pkt->ar_op), cp, sizeof(arp_pkt->ar_op));
	cp += sizeof(arp_pkt->ar_op);
	memcpy(&(arp_pkt->ar_sha), cp, sizeof(arp_pkt->ar_sha));
	cp += sizeof(arp_pkt->ar_sha);
	memcpy(&(arp_pkt->ar_sip), cp, sizeof(arp_pkt->ar_sip));
	cp += sizeof(arp_pkt->ar_sip);
	memcpy(&(arp_pkt->ar_tha), cp, sizeof(arp_pkt->ar_tha));
	cp += sizeof(arp_pkt->ar_tha);
	memcpy(&(arp_pkt->ar_tip), cp, sizeof(arp_pkt->ar_tip));
	cp += sizeof(arp_pkt->ar_tip);

	return framing;
}

void arp_add_host(const char *host_name) {

	if((helist = REALLOC(helist, ((num_hosts+1) * sizeof(struct host_entry *)))) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	if((helist[num_hosts] = MALLOC(sizeof(struct host_entry))) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	helist[num_hosts]->addr.s_addr = inet_addr(host_name);
	helist[num_hosts]->live = 1;
	helist[num_hosts]->timeout = 500 * 1000;	/* Convert from ms to us */
	helist[num_hosts]->num_sent = 0;
	helist[num_hosts]->num_recv = 0;
	helist[num_hosts]->found = 0;
	helist[num_hosts]->last_send_time.tv_sec = 0;
	helist[num_hosts]->last_send_time.tv_usec = 0;
	num_hosts++;
}

static int send_packet(pcap_t *pcap_handle, host_entry *he, struct timeval *last_packet_time, char source_mac[ETH_ALEN]) {
	struct ether_hdr frame_hdr;
	struct arp_ether_ipv4 arpei;
	unsigned char buf[MAX_FRAME];
	unsigned char target_mac[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	unsigned char arp_tha[ETH_ALEN] = {0, 0, 0, 0, 0, 0};
	int buflen = 0;
	int nsent = 0;

	memcpy(frame_hdr.dest_addr, target_mac, ETH_ALEN);
	memcpy(frame_hdr.src_addr, source_mac, ETH_ALEN);
	frame_hdr.frame_type = htons(0x0806);

	memset(&arpei, '\0', sizeof(arp_ether_ipv4));
	arpei.ar_hrd = htons(1);
	arpei.ar_pro = htons(0x0800);
	arpei.ar_hln = 6;
	arpei.ar_pln = 4;
	arpei.ar_op = htons(1);
	memcpy(arpei.ar_sha, source_mac, ETH_ALEN);
	memcpy(arpei.ar_tha, arp_tha, ETH_ALEN);
	arpei.ar_sip = 0;
	if(he != NULL) {
		arpei.ar_tip = he->addr.s_addr;
	}

	marshal_arp_pkt(buf, &frame_hdr, &arpei, &buflen);

	if(he == NULL) {
		return buflen;
	}

	if(he->live == 0) {
		logprintf(LOG_ERR, "send_packet called on non-live host");
		return -1;
	}

	gettimeofday(last_packet_time, NULL);
	he->last_send_time.tv_sec  = last_packet_time->tv_sec;
	he->last_send_time.tv_usec = last_packet_time->tv_usec;
	he->num_sent++;

	nsent = pcap_sendpacket(pcap_handle, buf, buflen);
	if(nsent < 0) {
		logprintf(LOG_ERR, "ERROR: failed to send packet");
		return -1;
	}

	return buflen;
}

static struct host_entry *find_host(host_entry **he, struct in_addr *addr) {
	host_entry **p;
	int found = 0;

	if(*he == NULL) {
		return NULL;
	}

	p = he;

  do {
		if((*p)->addr.s_addr == addr->s_addr) {
			found = 1;
		} else {
			if(p == helist) {
				p = helist + (num_hosts-1);	/* Wrap round to end */
			} else {
				p--;
			}
		}
	} while(!found && p != he);

	if(found == 1) {
		return *p;
	} else {
		return NULL;
	}
}

static void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet_in) {
	struct arp_ether_ipv4 arpei;
	struct ether_hdr frame_hdr;
	size_t n = header->caplen;
	struct in_addr source_ip;
	struct host_entry *temp_cursor = NULL;

	if(n < ETHER_HDR_SIZE + ARP_PKT_SIZE) {
		logprintf(LOG_ERR, "%d byte packet too short to decode\n", (int)n);
		return;
   }

	unmarshal_arp_pkt(packet_in, n, &frame_hdr, &arpei, NULL, NULL);
	source_ip.s_addr = arpei.ar_sip;
	temp_cursor = find_host(cursor, &source_ip);

	if(temp_cursor != NULL) {
		temp_cursor->num_recv++;
		if(temp_cursor->live == 1) {
			temp_cursor->found = 1;
			memcpy(temp_cursor->mac, arpei.ar_sha, ETH_ALEN);
		}
		remove_host(&temp_cursor);
	}
}

static int recvfrom_wto(long unsigned int tmo, pcap_t *pcap_handle) {
#ifdef _WIN32
	WaitForSingleObject(pcap_getevent(pcap_handle), (DWORD)tmo);
#else
	fd_set readset;
	struct timeval to;
	int n = 0, pcap_fd = 0;
	if((pcap_fd = pcap_get_selectable_fd(pcap_handle)) < 0) {
		logprintf(LOG_ERR, "pcap_fileno: %s", pcap_geterr(pcap_handle));
		return -1;
	}
	FD_ZERO(&readset);
	if(pcap_fd >= 0)
		FD_SET(pcap_fd, &readset);
		to.tv_sec  = (__time_t)(tmo/(long unsigned int)1000000);
		to.tv_usec = (__suseconds_t)(tmo - (long unsigned int)((time_t)1000000 * to.tv_sec));
		n = select(pcap_fd+1, &readset, NULL, NULL, &to);
	if(n < 0) {
		logprintf(LOG_ERR, "select");
		return -1;
	} else if (n == 0 && pcap_fd >= 0) {
		return 1;
	}
#endif
	if(pcap_handle != NULL) {
		if((pcap_dispatch(pcap_handle, -1, callback, NULL)) == -1) {
			logprintf(LOG_ERR, "pcap_dispatch: %s", pcap_geterr(pcap_handle));
			return -1;
		}
		return 0;
	}
	return -1;
}

int arp_resolv(char *if_name, char *srcmac, char *dstmac, char **ip) {
	struct timeval now, diff, last_packet_time;
	pcap_t *pcap_handle = NULL;
	unsigned long int loop_timediff = 0, host_timediff = 0;
	unsigned long int req_interval = 0, select_timeout = 0;
	unsigned long int cum_err = 0, interval = 0;
	char *if_cpy = NULL, error[PCAP_ERRBUF_SIZE], *e = error;
	int found = -1;
	int reset_cum_err = 0, first_timeout = 1;
	int i = 0;

	if((if_cpy = MALLOC(strlen(if_name)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(if_cpy, if_name);

#ifdef _WIN32
	int match = 0;
	pcap_if_t *alldevs = NULL, *d = NULL;

	if(pcap_findalldevs(&alldevs, e) == -1){
		logprintf(LOG_ERR, "pcap_findalldevs: %s", e);
		goto close;
	}

	d = alldevs;
	match = 0;
	for(d=alldevs;d;d=d->next) {
		if(strstr(d->name, if_cpy) != NULL) {
			match = 1;
			if((if_cpy = REALLOC(if_cpy, strlen(d->name)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(if_cpy, d->name);
			break;
		}
	}
	if(alldevs != NULL) {
		pcap_freealldevs(alldevs);
	}
	if(match == 0) {
		logprintf(LOG_ERR, "could not full interface name for %s", if_name);
		goto close;
	}
#endif

	if((pcap_handle = pcap_open_live(if_cpy, 64, 0, 3, e)) == NULL) {
		logprintf(LOG_ERR, "pcap_open_live: %s", e);
		goto close;
	}

	if((pcap_setnonblock(pcap_handle, 1, e)) < 0) {
		logprintf(LOG_ERR, "pcap_setnonblock: %s", e);
		goto close;
	}

	live_count = num_hosts;
	cursor = helist;
	last_packet_time.tv_sec=0;
	last_packet_time.tv_usec=0;

	interval = 10000;

	reset_cum_err = 1;
	req_interval = interval;
  while(live_count > 0) {
		gettimeofday(&now, NULL);
    timeval_diff(&now, &last_packet_time, &diff);
    loop_timediff = (unsigned long int)(1000000*diff.tv_sec + diff.tv_usec);
    if(loop_timediff >= (unsigned)req_interval) {
			timeval_diff(&now, &((*cursor)->last_send_time), &diff);
			host_timediff = (unsigned long int)(1000000*diff.tv_sec + diff.tv_usec);
			if(host_timediff >= (*cursor)->timeout) {
				if(reset_cum_err > 0) {
					cum_err = 0;
					req_interval = interval;
					reset_cum_err = 0;
				} else {
					cum_err += loop_timediff - interval;
					if(req_interval >= cum_err) {
						req_interval = req_interval - cum_err;
					} else {
						req_interval = 0;
					}
				}
				select_timeout = req_interval;
				if((*cursor)->num_sent >= 2) {
					remove_host(cursor);
					if(first_timeout > 0) {
						timeval_diff(&now, &((*cursor)->last_send_time), &diff);
						host_timediff = (unsigned long int)(1000000*diff.tv_sec + diff.tv_usec);
						while(host_timediff >= (*cursor)->timeout && live_count) {
							if((*cursor)->live == 1) {
								remove_host(cursor);
							} else {
								advance_cursor();
							}
							timeval_diff(&now, &((*cursor)->last_send_time), &diff);
							host_timediff = (unsigned long int)(1000000*diff.tv_sec + diff.tv_usec);
						}
						first_timeout=0;
					}
					gettimeofday(&last_packet_time, NULL);
				} else {
					if((*cursor)->num_sent > 0) {
						(*cursor)->timeout *= 1.5;
					}
					if(send_packet(pcap_handle, *cursor, &last_packet_time, srcmac) == -1) {
						goto close;
					}
					advance_cursor();
				}
			} else {
				select_timeout = (long unsigned int)((*cursor)->timeout - host_timediff);
				reset_cum_err = 1;
      }
		} else {
			select_timeout = req_interval - loop_timediff;
		}
		if(recvfrom_wto(select_timeout, pcap_handle) == -1) {
			goto close;
		}
	}

	for(i=0;i<num_hosts;i++) {
		if(helist[i]->found == 1) {
			char fmac[ETH_ALEN];
			memset(fmac, '\0', ETH_ALEN);
			sprintf(fmac, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
													helist[i]->mac[0], helist[i]->mac[1],
													helist[i]->mac[2], helist[i]->mac[3],
													helist[i]->mac[4], helist[i]->mac[5]);
			if(strcmp(fmac, dstmac) == 0) {
				memset(*ip, '\0', INET_ADDRSTRLEN+1);
				inet_ntop(AF_INET, (void *)&(helist[i]->addr), *ip, INET_ADDRSTRLEN+1);
				found = 1;
				break;
			}
		}
	}

close:
	if(pcap_handle != NULL) {
		pcap_close(pcap_handle);
	}
	if(if_cpy != NULL) {
		FREE(if_cpy);
	}
  for(i=0;i<num_hosts;i++) {
		FREE(helist[i]);
	}
	FREE(helist);
	helist = NULL;
	num_hosts = 0;

	if(found == 1) {
		return 0;
	} else {
		return -1;
	}
}
