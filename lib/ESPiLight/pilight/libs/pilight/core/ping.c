/*
 * icmpquery.c - send and receive ICMP queries for address mask
 *               and current time.
 *
 * Version 1.0.3
 *
 * Copyright 1998, 1999, 2000  David G. Andersen <angio@pobox.com>
 *                                        <danderse@cs.utah.edu>
 *                                        http://www.angio.net/
 *
 * All rights reserved.
 * This information is subject to change without notice and does not
 * represent a commitment on the part of David G. Andersen.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of David G. Andersen may not
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL DAVID G. ANDERSEN BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * Verified to work on:
 *    FreeBSD (2.x, 3.x)
 *    Linux 2.0.x, 2.2.0-pre1
 *    NetBSD 1.3
 *
 * Should work on Solaris and other platforms with BSD-ish stacks.
 *
 * If you compile it somewhere else, or it doesn't work somewhere,
 * please let me know.
 *
 * Compilation:  gcc icmpquery.c -o icmpquery
 *
 * One usage note:  In order to receive accurate time information,
 *                  the time on your computer must be correct; the
 *                  ICMP timestamp reply is a relative time figure.
 */


/* Some portions of this code are taken from FreeBSD's ping source.
 * Those portions are subject to the BSD copyright, which is appended
 * at the end of this file.  Namely, in_cksum.
 */

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	#define MSG_NOSIGNAL 0
#else
	#include <sys/cdefs.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/if_ether.h>
	#include <netinet/tcp.h>
	#include <net/route.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <netinet/in_systm.h>
	#include <netinet/ip.h>
	#include <netinet/ip_icmp.h>
#endif
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "common.h"
#include "network.h"
#include "log.h"
#include "mem.h"

#ifdef _WIN32
	typedef unsigned char u_int8_t;
	typedef unsigned short u_int16_t;
	typedef unsigned int u_int32_t;

	#define ICMP_ECHO		8
	#define ICMP_ECHOREPLY		0

	struct icmp_ra_addr	{
		u_int32_t ira_addr;
		u_int32_t ira_preference;
	};

	struct ip {
		u_char	ip_hl:4,		/* header length */
			ip_v:4;			/* version */
		u_char	ip_tos;			/* type of service */
		short	ip_len;			/* total length */
		u_short	ip_id;			/* identification */
		short	ip_off;			/* fragment offset field */
	#define	IP_DF 0x4000			/* dont fragment flag */
	#define	IP_MF 0x2000			/* more fragments flag */
		u_char	ip_ttl;			/* time to live */
		u_char	ip_p;			/* protocol */
		u_short	ip_sum;			/* checksum */
		struct	in_addr ip_src,ip_dst;	/* source and dest address */
	};

	struct icmp
	{
		u_int8_t  icmp_type;	/* type of message, see below */
		u_int8_t  icmp_code;	/* type sub code */
		u_int16_t icmp_cksum;	/* ones complement checksum of struct */
		union
		{
			u_char ih_pptr;		/* ICMP_PARAMPROB */
			struct in_addr ih_gwaddr;	/* gateway address */
			struct ih_idseq		/* echo datagram */
			{
				u_int16_t icd_id;
				u_int16_t icd_seq;
			} ih_idseq;
			u_int32_t ih_void;

			/* ICMP_UNREACH_NEEDFRAG -- Path MTU Discovery (RFC1191) */
			struct ih_pmtu
			{
				u_int16_t ipm_void;
				u_int16_t ipm_nextmtu;
			} ih_pmtu;

			struct ih_rtradv
			{
				u_int8_t irt_num_addrs;
				u_int8_t irt_wpa;
				u_int16_t irt_lifetime;
			} ih_rtradv;
		} icmp_hun;
	#define	icmp_pptr	icmp_hun.ih_pptr
	#define	icmp_gwaddr	icmp_hun.ih_gwaddr
	#define	icmp_id		icmp_hun.ih_idseq.icd_id
	#define	icmp_seq	icmp_hun.ih_idseq.icd_seq
	#define	icmp_void	icmp_hun.ih_void
	#define	icmp_pmvoid	icmp_hun.ih_pmtu.ipm_void
	#define	icmp_nextmtu	icmp_hun.ih_pmtu.ipm_nextmtu
	#define	icmp_num_addrs	icmp_hun.ih_rtradv.irt_num_addrs
	#define	icmp_wpa	icmp_hun.ih_rtradv.irt_wpa
	#define	icmp_lifetime	icmp_hun.ih_rtradv.irt_lifetime
		union
		{
			struct
			{
				u_int32_t its_otime;
				u_int32_t its_rtime;
				u_int32_t its_ttime;
			} id_ts;
			struct
			{
				struct ip idi_ip;
				/* options and then 64 bits of data */
			} id_ip;
			struct icmp_ra_addr id_radv;
			u_int32_t   id_mask;
			u_int8_t    id_data[1];
		} icmp_dun;
	#define	icmp_otime	icmp_dun.id_ts.its_otime
	#define	icmp_rtime	icmp_dun.id_ts.its_rtime
	#define	icmp_ttime	icmp_dun.id_ts.its_ttime
	#define	icmp_ip		icmp_dun.id_ip.idi_ip
	#define	icmp_radv	icmp_dun.id_radv
	#define	icmp_mask	icmp_dun.id_mask
	#define	icmp_data	icmp_dun.id_data
	};
#endif

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 *      From FreeBSD's ping.c
 */
static int in_cksum(int *addr, int len) {
	register int nleft = len;
	register int *w = addr;
	register int sum = 0;
	int answer = 0;

	/*
	* Our algorithm is simple, using a 32 bit accumulator (sum), we add
	* sequential 16 bit words to it, and at the end, fold back all the
	* carry bits from the top 16 bits into the lower 16 bits.
	*/
	while(nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if(nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return answer;
}

static int initpacket(char *buf) {
	struct ip *ip = (struct ip *)buf;
	struct icmp *icmp = (struct icmp *)(ip + 1);
	int icmplen = 20;
	struct in_addr fromaddr;
	fromaddr.s_addr = 0;

	ip->ip_src = fromaddr;
	ip->ip_v = 4;
	ip->ip_hl = sizeof *ip >> 2;
	ip->ip_tos = 0;
	ip->ip_id = htons(4321);
	ip->ip_ttl = 255;
	ip->ip_p = 1;
	ip->ip_sum = 0;

	icmp->icmp_seq = 1;
	icmp->icmp_cksum = 0;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;

	gettimeofday((struct timeval *)(icmp+8), NULL);
	memset(icmp+12, '\0',	8);
	ip->ip_len = sizeof(struct ip) + icmplen;
	return icmplen;
}

int ping(char *addr) {
	char buf[1500], buf1[INET_ADDRSTRLEN+1];
	struct ip *ip = (struct ip *)buf;
	struct icmp *icmp = (struct icmp *)(ip + 1);
	struct sockaddr_in dst;
	int icmplen = 0;
	int sockfd = 0, on = 1;
	long int fromlen = 0;

#ifdef _WIN32
	WSADATA wsa;

	if(WSAStartup(0x202, &wsa) != 0) {
		logprintf(LOG_ERR, "could not initialize new socket");
		exit(EXIT_FAILURE);
	}
#endif

	if((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		logperror(LOG_DEBUG, "socket");
		return -1;
	}

	if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, (const char *)&on, sizeof(on)) < 0) {
		logperror(LOG_DEBUG, "IP_HDRINCL");
		close(sockfd);
		return -1;
	}

#ifndef _WIN32
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0) {
#else
	int timeout = 1000;
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(struct timeval)) < 0) {
#endif
		logperror(LOG_DEBUG, "SO_RCVTIMEO");
		close(sockfd);
		return -1;
	}

	memset(buf, '\0', 1500);
	icmplen = initpacket(buf);
	dst.sin_family = AF_INET;

	ip->ip_dst.s_addr = inet_addr(addr);
	dst.sin_addr.s_addr = inet_addr(addr);
	dst.sin_port = htons(0);
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((int *)icmp, icmplen);
	if(sendto(sockfd, buf, ip->ip_len, 0, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
		logperror(LOG_DEBUG, "sendto");
		close(sockfd);
		return -1;
	}

	memset(buf, '\0', sizeof(buf));
	if((recvfrom(sockfd, buf, sizeof(buf), 0, NULL, (socklen_t *)&fromlen)) < 0) {
		logperror(LOG_DEBUG, "recvfrom");
		close(sockfd);
		return -1;
	}

	icmp = (struct icmp *)(buf + (ip->ip_hl << 2));
	memset(&buf1, '\0', INET_ADDRSTRLEN+1);
	inet_ntop(AF_INET, (void *)&(ip->ip_src), buf1, INET_ADDRSTRLEN+1);
	if(!((icmp->icmp_type == ICMP_ECHOREPLY || icmp->icmp_type == ICMP_ECHO) && strcmp(buf1, addr) == 0)) {
		logprintf(LOG_DEBUG, "unexpected status reply: %d code: %d at: %s from: %s",icmp->icmp_type, icmp->icmp_code, buf1, addr);
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}
