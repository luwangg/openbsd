/*	$OpenBSD: src/sbin/nfsd/nfsd.c,v 1.24 2004/05/10 15:26:49 deraadt Exp $	*/
/*	$NetBSD: nfsd.c,v 1.19 1996/02/18 23:18:56 mycroft Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static const char sccsid[] = "@(#)nfsd.c	8.9 (Berkeley) 3/29/95";
#else
static const char rcsid[] = "$OpenBSD: src/sbin/nfsd/nfsd.c,v 1.24 2004/05/10 15:26:49 deraadt Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

/* Global defs */
#ifdef DEBUG
#define	syslog(e, s)	fprintf(stderr,(s))
int	debug = 1;
#else
int	debug = 0;
#endif

struct	nfsd_srvargs nsd;

void	nonfs(int);
void	reapchild(int);
void	usage(void);

#define	MAXNFSDCNT	20
#define	DEFNFSDCNT	 4

/*
 * Nfs server daemon mostly just a user context for nfssvc()
 *
 * 1 - do file descriptor and signal cleanup
 * 2 - fork the nfsd(s)
 * 3 - create server socket(s)
 * 4 - register socket with portmap
 *
 * For connectionless protocols, just pass the socket into the kernel via.
 * nfssvc().
 * For connection based sockets, loop doing accepts. When you get a new
 * socket from accept, pass the msgsock into the kernel via. nfssvc().
 * The arguments are:
 *	-r - reregister with portmapper
 *	-t - support tcp nfs clients
 *	-u - support udp nfs clients
 * followed by "n" which is the number of nfsds' to fork off
 */
int
main(int argc, char *argv[])
{
	struct nfsd_args nfsdargs;
	struct sockaddr_in inetaddr, inetpeer;
	fd_set *ready, *sockbits;
	size_t fd_size;
	int ch, connect_type_cnt, i, maxsock = 0, msgsock;
	int nfsdcnt = DEFNFSDCNT, nfssvc_flag, on, reregister = 0, sock;
	int udpflag = 0, tcpflag = 0, tcpsock;
	const char *errstr = NULL;
	socklen_t len;

	while ((ch = getopt(argc, argv, "n:rtu")) != -1)
		switch (ch) {
		case 'n':
			nfsdcnt = strtonum(optarg, 1, MAXNFSDCNT, &errstr);
			if (errstr) {
				warnx("nfsd count %s %s; reset to %d",
				    optarg, errstr, DEFNFSDCNT);
				nfsdcnt = DEFNFSDCNT;
			}
			break;
		case 'r':
			reregister = 1;
			break;
		case 't':
			tcpflag = 1;
			break;
		case 'u':
			udpflag = 1;
			break;
		default:
			usage();
		};
	argv += optind;
	argc -= optind;

	/*
	 * XXX
	 * Backward compatibility, trailing number is the count of daemons.
	 */
	if (argc > 1)
		usage();
	if (argc == 1) {
		nfsdcnt = strtonum(argv[0], 1, MAXNFSDCNT, &errstr);
		if (errstr) {
			warnx("nfsd count %s %s; reset to %d",
			    argv[0], errstr, DEFNFSDCNT);
			nfsdcnt = DEFNFSDCNT;
		}
	}

	if (debug == 0) {
		daemon(0, 0);
		(void)signal(SIGHUP, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
		(void)signal(SIGSYS, nonfs);
	}
	(void)signal(SIGCHLD, reapchild);

	if (reregister) {
		if (udpflag &&
		    (!pmap_set(RPCPROG_NFS, 2, IPPROTO_UDP, NFS_PORT) ||
		     !pmap_set(RPCPROG_NFS, 3, IPPROTO_UDP, NFS_PORT)))
			err(1, "can't register with portmap for UDP.");
		if (tcpflag &&
		    (!pmap_set(RPCPROG_NFS, 2, IPPROTO_TCP, NFS_PORT) ||
		     !pmap_set(RPCPROG_NFS, 3, IPPROTO_TCP, NFS_PORT)))
			err(1, "can't register with portmap for TCP.");
		return (0);
	}
	openlog("nfsd", LOG_PID, LOG_DAEMON);

	for (i = 0; i < nfsdcnt; i++) {
		switch (fork()) {
		case -1:
			syslog(LOG_ERR, "fork: %m");
			return (1);
		case 0:
			break;
		default:
			continue;
		}

		setproctitle("server");
		nfssvc_flag = NFSSVC_NFSD;
		nsd.nsd_nfsd = NULL;
		while (nfssvc(nfssvc_flag, &nsd) < 0) {
			if (errno != ENEEDAUTH) {
				syslog(LOG_ERR, "nfssvc: %m");
				return (1);
			}
			nfssvc_flag = NFSSVC_NFSD | NFSSVC_AUTHINFAIL;
		}
		return (0);
	}

	/* If we are serving udp, set up the socket. */
	if (udpflag) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			syslog(LOG_ERR, "can't create udp socket");
			return (1);
		}
		memset(&inetaddr, 0, sizeof inetaddr);
		inetaddr.sin_family = AF_INET;
		inetaddr.sin_addr.s_addr = INADDR_ANY;
		inetaddr.sin_port = htons(NFS_PORT);
		inetaddr.sin_len = sizeof(inetaddr);
		if (bind(sock, (struct sockaddr *)&inetaddr,
		    sizeof(inetaddr)) < 0) {
			syslog(LOG_ERR, "can't bind udp addr");
			return (1);
		}
		if (!pmap_set(RPCPROG_NFS, 2, IPPROTO_UDP, NFS_PORT) ||
		    !pmap_set(RPCPROG_NFS, 3, IPPROTO_UDP, NFS_PORT)) {
			syslog(LOG_ERR, "can't register with udp portmap");
			return (1);
		}
		nfsdargs.sock = sock;
		nfsdargs.name = NULL;
		nfsdargs.namelen = 0;
		if (nfssvc(NFSSVC_ADDSOCK, &nfsdargs) < 0) {
			syslog(LOG_ERR, "can't Add UDP socket");
			return (1);
		}
		(void)close(sock);
	}

	/* Now set up the master server socket waiting for tcp connections. */
	on = 1;
	connect_type_cnt = 0;
	if (tcpflag) {
		if ((tcpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			syslog(LOG_ERR, "can't create tcp socket");
			return (1);
		}
		if (setsockopt(tcpsock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		memset(&inetaddr, 0, sizeof inetaddr);
		inetaddr.sin_family = AF_INET;
		inetaddr.sin_addr.s_addr = INADDR_ANY;
		inetaddr.sin_port = htons(NFS_PORT);
		inetaddr.sin_len = sizeof(inetaddr);
		if (bind(tcpsock, (struct sockaddr *)&inetaddr,
		    sizeof (inetaddr)) < 0) {
			syslog(LOG_ERR, "can't bind tcp addr");
			return (1);
		}
		if (listen(tcpsock, 5) < 0) {
			syslog(LOG_ERR, "listen failed");
			return (1);
		}
		if (!pmap_set(RPCPROG_NFS, 2, IPPROTO_TCP, NFS_PORT) ||
		    !pmap_set(RPCPROG_NFS, 3, IPPROTO_TCP, NFS_PORT)) {
			syslog(LOG_ERR, "can't register tcp with portmap");
			return (1);
		}
		maxsock = tcpsock;
		connect_type_cnt++;
	}

	if (connect_type_cnt == 0)
		return (0);

	setproctitle("master");

	/*
	 * Allocate space for the fd_set pointers and fill in sockbits
	 */
	fd_size = howmany(maxsock + 1, NFDBITS) * sizeof(fd_mask);
	sockbits = malloc(fd_size);
	ready = malloc(fd_size);
	if (sockbits == NULL || ready == NULL) {
		syslog(LOG_ERR, "cannot allocate memory");
		return (1);
	}
	memset(sockbits, 0, fd_size);
	if (tcpflag)
		FD_SET(tcpsock, sockbits);

	/*
	 * Loop forever accepting connections and passing the sockets
	 * into the kernel for the mounts.
	 */
	for (;;) {
		memcpy(ready, sockbits, fd_size);
		if (connect_type_cnt > 1) {
			if (select(maxsock + 1,
			    ready, NULL, NULL, NULL) < 1) {
				syslog(LOG_ERR, "select failed: %m");
				return (1);
			}
		}
		if (tcpflag && FD_ISSET(tcpsock, ready)) {
			len = sizeof(inetpeer);
			if ((msgsock = accept(tcpsock,
			    (struct sockaddr *)&inetpeer, &len)) < 0) {
				syslog(LOG_ERR, "accept failed: %m");
				return (1);
			}
			memset(inetpeer.sin_zero, 0, sizeof(inetpeer.sin_zero));
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&inetpeer;
			nfsdargs.namelen = sizeof(inetpeer);
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}
	}
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: nfsd [-rtu] [-n num_servers]\n");
	exit(1);
}

void
nonfs(int signo)
{
	int save_errno = errno;
	struct syslog_data sdata = SYSLOG_DATA_INIT;

	syslog_r(LOG_ERR, &sdata, "missing system call: NFS not available.");
	errno = save_errno;
}

void
reapchild(int signo)
{
	int save_errno = errno;

	while (wait3(NULL, WNOHANG, NULL) > 0)
		;
	errno = save_errno;
}
