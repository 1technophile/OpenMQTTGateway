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

#ifndef _LIBPROC_H_
#define _LIBPROC_H_

int ping(char *addr);

#endif
