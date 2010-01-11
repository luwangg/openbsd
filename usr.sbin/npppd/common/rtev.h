/*-
 * Copyright (c) 2009 Internet Initiative Japan Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef	RTEV_H
#define	RTEV_H

#ifndef	NO_RTEV_WRAPPER
#define	getifaddrs(ifa)		rtev_getifaddrs(ifa)
#define	if_nametoindex(ifname)	rtev_if_nametoindex(ifname);
#define	freeifaddrs(ifa)	((void)0)
#endif

#define	RTEV_UPDATE_IFA_ON_DEMAND	0x0001

#ifdef __cplusplus
extern "C" {
#endif

int             rtev_libevent_init (int, int, int, int);
void            rtev_fini (void);
int             rtev_write (void *);
int             rtev_getifaddrs (struct ifaddrs **);
int             rtev_ifa_is_primary (const char *, struct sockaddr *);
inline int      rtev_get_event_serial (void);
struct ifaddrs  *rtev_getifaddrs_by_ifname (const char *);
struct ifaddrs  *rtev_getifaddrs_by_sockaddr(struct sockaddr const *);
unsigned int    rtev_if_nametoindex (const char *);
int             rtev_has_write_pending(void);


#ifdef __cplusplus
}
#endif

#endif
