/*	$OpenBSD: src/sbin/ipnat/Attic/ipnat.c,v 1.36 2000/03/13 23:40:19 kjell Exp $	*/

/*
 * Copyright (C) 1993-1998 by Darren Reed.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and due credit is given
 * to the original author and the contributors.
 *
 * Added redirect stuff and a variety of bug fixes. (mcn@EnGarde.com)
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#if !defined(__SVR4) && !defined(__svr4__)
#include <strings.h>
#else
#include <sys/byteorder.h>
#endif
#include <sys/time.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#if defined(sun) && (defined(__svr4__) || defined(__SVR4))
# include <sys/ioccom.h>
# include <sys/sysmacros.h>
#endif
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#if __FreeBSD_version >= 300000
# include <net/if_var.h>
#endif
#include <netdb.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <ctype.h>
#include <netinet/ip_fil_compat.h>
#include <netinet/ip_fil.h>
#include <netinet/ip_proxy.h>
#include <netinet/ip_nat.h>
#include "kmem.h"

#if	defined(sun) && !SOLARIS2
# define	STRERROR(x)	sys_errlist[x]
extern	char	*sys_errlist[];
#else
# define	STRERROR(x)	strerror(x)
#endif

#if !defined(lint)
static const char sccsid[] ="@(#)ipnat.c	1.9 6/5/96 (C) 1993 Darren Reed";
static const char rcsid[] = "@(#)$IPFilter: ipnat.c,v 2.1.2.2 1999/12/04 02:09:30 darrenr Exp $";
#endif


#if	SOLARIS
#define	bzero(a,b)	memset(a,0,b)
#endif

extern	char	*optarg;
char	*nlistf = NULL, *memf = NULL;
extern	ipnat_t	*natparse __P((char *, int));
extern	void	natparsefile __P((int, char *, int));
extern	void	printnat __P((ipnat_t *, int, void *));

#if defined(__OpenBSD__)
extern  int	if_addr __P((char *, struct in_addr *));
#endif

u_32_t	hostnum __P((char *, int *, int));
u_32_t	hostmask __P((char *));
void	dostats __P((int, int)), flushtable __P((int, int));
void	usage __P((char *));
int	countbits __P((u_32_t));
char	*getnattype __P((ipnat_t *));
int	main __P((int, char*[]));
void	printaps __P((ap_session_t *, int));
char	*getsumd __P((u_32_t));

#define	OPT_REM		1
#define	OPT_NODO	2
#define	OPT_STAT	4
#define	OPT_LIST	8
#define	OPT_VERBOSE	16
#define	OPT_FLUSH	32
#define	OPT_CLEAR	64
#define	OPT_HITS	128


void usage(name)
char *name;
{
	fprintf(stderr, "%s: [-CFhlnrsv] [-f filename]\n", name);
	exit(1);
}


char *getsumd(sum)
u_32_t sum;
{
	static char sumdbuf[17];

	if (sum & NAT_HW_CKSUM)
		sprintf(sumdbuf, "hw(%#0x)", sum & 0xffff);
	else
		sprintf(sumdbuf, "%#0x", sum);
	return sumdbuf;
}


int main(argc, argv)
int argc;
char *argv[];
{
	char	*file = NULL;
	int	fd = -1, opts = 0, c;

	while ((c = getopt(argc, argv, "CFf:hlnrsv")) != -1)
		switch (c)
		{
		case 'C' :
			opts |= OPT_CLEAR;
			break;
		case 'f' :
			file = optarg;
			break;
		case 'F' :
			opts |= OPT_FLUSH;
			break;
		case 'h' :
			opts |=OPT_HITS;
			break;
		case 'l' :
			opts |= OPT_LIST;
			break;
		case 'n' :
			opts |= OPT_NODO;
			break;
		case 'r' :
			opts |= OPT_REM;
			break;
		case 's' :
			opts |= OPT_STAT;
			break;
		case 'v' :
			opts |= OPT_VERBOSE;
			break;
		default :
			usage(argv[0]);
		}

	if (!(opts & OPT_NODO) && ((fd = open(IPL_NAT, O_RDWR)) == -1) &&
	    ((fd = open(IPL_NAT, O_RDONLY)) == -1)) {
		(void) fprintf(stderr, "%s: open: %s\n", IPL_NAT,
			STRERROR(errno));
		exit(-1);
	}

	if (opts & (OPT_FLUSH|OPT_CLEAR))
		flushtable(fd, opts);
	if (file)
		natparsefile(fd, file, opts);
	if (opts & (OPT_LIST|OPT_STAT))
		dostats(fd, opts);
	return 0;
}


/*
 * count consecutive 1's in bit mask.  If the mask generated by counting
 * consecutive 1's is different to that passed, return -1, else return #
 * of bits.
 */
int	countbits(ip)
u_32_t	ip;
{
	u_32_t	ipn;
	int	cnt = 0, i, j;

	ip = ipn = ntohl(ip);
	for (i = 32; i; i--, ipn *= 2)
		if (ipn & 0x80000000)
			cnt++;
		else
			break;
	ipn = 0;
	for (i = 32, j = cnt; i; i--, j--) {
		ipn *= 2;
		if (j > 0)
			ipn++;
	}
	if (ipn == ip)
		return cnt;
	return -1;
}


void printaps(aps, opts)
ap_session_t *aps;
int opts;
{
	ap_session_t ap;
	aproxy_t apr;
	raudio_t ra;

	if (kmemcpy((char *)&ap, (long)aps, sizeof(ap)))
		return;
	if (kmemcpy((char *)&apr, (long)ap.aps_apr, sizeof(apr)))
		return;
	printf("\tproxy %s/%d use %d flags %x\n", apr.apr_label,
		apr.apr_p, apr.apr_ref, apr.apr_flags);
	printf("\t\tproto %d flags %#x bytes ", ap.aps_p, ap.aps_flags);
#ifdef	USE_QUAD_T
	printf("%qu pkts %qu", ap.aps_bytes, ap.aps_pkts);
#else
	printf("%lu pkts %lu", ap.aps_bytes, ap.aps_pkts);
#endif
	printf(" data %p psiz %d\n", ap.aps_data, ap.aps_psiz);
	if ((ap.aps_p == IPPROTO_TCP) && (opts & OPT_VERBOSE)) {
		printf("\t\tstate[%u,%u], sel[%d,%d]\n",
			ap.aps_state[0], ap.aps_state[1],
			ap.aps_sel[0], ap.aps_sel[1]);
#if (defined(NetBSD) && (NetBSD >= 199905) && (NetBSD < 1991011)) || \
    (__FreeBSD_version >= 300000) || defined(OpenBSD)
		printf("\t\tseq: off %hd/%hd min %x/%x\n",
			ap.aps_seqoff[0], ap.aps_seqoff[1],
			ap.aps_seqmin[0], ap.aps_seqmin[1]);
		printf("\t\tack: off %hd/%hd min %x/%x\n",
			ap.aps_ackoff[0], ap.aps_ackoff[1],
			ap.aps_ackmin[0], ap.aps_ackmin[1]);
#else
		printf("\t\tseq: off %hd/%hd min %lx/%lx\n",
			ap.aps_seqoff[0], ap.aps_seqoff[1],
			ap.aps_seqmin[0], ap.aps_seqmin[1]);
		printf("\t\tack: off %hd/%hd min %lx/%lx\n",
			ap.aps_ackoff[0], ap.aps_ackoff[1],
			ap.aps_ackmin[0], ap.aps_ackmin[1]);
#endif
	}

	if (!strcmp(apr.apr_label, "raudio") && ap.aps_psiz == sizeof(ra)) {
		if (kmemcpy((char *)&ra, (long)ap.aps_data, sizeof(ra)))
			return;
		printf("\tReal Audio Proxy:\n");
		printf("\t\tSeen PNA: %d\tVersion: %d\tEOS: %d\n",
			ra.rap_seenpna, ra.rap_version, ra.rap_eos);
		printf("\t\tMode: %#x\tSBF: %#x\n", ra.rap_mode, ra.rap_sbf);
		printf("\t\tPorts:pl %hu, pr %hu, sr %hu\n",
			ra.rap_plport, ra.rap_prport, ra.rap_srport);
	}
}


/*
 * Get a nat filter type given its kernel address.
 */
char *getnattype(ipnat)
ipnat_t *ipnat;
{
	char *which;
	ipnat_t ipnatbuff;

	if (!ipnat || (ipnat && kmemcpy((char *)&ipnatbuff, (long)ipnat,
					sizeof(ipnatbuff))))
		return "???";

	switch (ipnatbuff.in_redir)
	{
	case NAT_MAP :
		which = "MAP";
		break;
	case NAT_MAPBLK :
		which = "MAP-BLOCK";
		break;
	case NAT_REDIRECT :
		which = "RDR";
		break;
	case NAT_BIMAP :
		which = "BIMAP";
		break;
	default :
		which = "unknown";
		break;
	}
	return which;
}


void dostats(fd, opts)
int fd, opts;
{
	natstat_t ns;
	ipnat_t	ipn;
	nat_t	**nt[2], *np, nat;
	int	i = 0;

	bzero((char *)&ns, sizeof(ns));

	if (!(opts & OPT_NODO) && ioctl(fd, SIOCGNATS, &ns) == -1) {
		perror("ioctl(SIOCGNATS)");
		return;
	}

	if (opts & OPT_STAT) {
		printf("mapped\tin\t%lu\tout\t%lu\n",
			ns.ns_mapped[0], ns.ns_mapped[1]);
		printf("added\t%lu\texpired\t%lu\n",
			ns.ns_added, ns.ns_expire);
		printf("inuse\t%lu\nrules\t%lu\n", ns.ns_inuse, ns.ns_rules);
		if (opts & OPT_VERBOSE)
			printf("table %p list %p\n", ns.ns_table, ns.ns_list);
	}
	if (opts & OPT_LIST) {
		printf("List of active MAP/Redirect filters:\n");
		while (ns.ns_list) {
			if (kmemcpy((char *)&ipn, (long)ns.ns_list,
				    sizeof(ipn))) {
				perror("kmemcpy");
				break;
			}
			if (opts & OPT_HITS)
				printf("%d ", ipn.in_hits);
			printnat(&ipn, opts & OPT_VERBOSE, (void *)ns.ns_list);
			ns.ns_list = ipn.in_next;
		}

		nt[0] = (nat_t **)malloc(sizeof(*nt) * NAT_SIZE);
		if (kmemcpy((char *)nt[0], (long)ns.ns_table[0],
			    sizeof(**nt) * NAT_SIZE)) {
			perror("kmemcpy");
			return;
		}

		printf("\nList of active sessions:\n");

		for (np = ns.ns_instances; np; np = nat.nat_next) {
			if (kmemcpy((char *)&nat, (long)np, sizeof(nat)))
				break;

			printf("%s %-15s %-5hu <- ->", getnattype(nat.nat_ptr),
			       inet_ntoa(nat.nat_inip), ntohs(nat.nat_inport));
			printf(" %-15s %-5hu", inet_ntoa(nat.nat_outip),
				ntohs(nat.nat_outport));
			printf(" [%s %hu]", inet_ntoa(nat.nat_oip),
				ntohs(nat.nat_oport));
			if (opts & OPT_VERBOSE) {
				printf("\n\tage %lu use %hu sumd %s/",
					nat.nat_age, nat.nat_use,
					getsumd(nat.nat_sumd[0]));
				printf("%s pr %u bkt %d flags %x ",
					getsumd(nat.nat_sumd[1]), nat.nat_p,
					i, nat.nat_flags);
#ifdef	USE_QUAD_T
				printf("bytes %qu pkts %qu",
					nat.nat_bytes, nat.nat_pkts);
#else
				printf("bytes %lu pkts %lu",
					nat.nat_bytes, nat.nat_pkts);
#endif
#if SOLARIS
				printf(" %lx", nat.nat_ipsumd);
#endif
			}
			putchar('\n');
			if (nat.nat_aps)
				printaps(nat.nat_aps, opts);
		}

		free(nt[0]);
	}
}


u_32_t	hostmask(msk)
char	*msk;
{
	int	bits = -1;
	u_32_t	mask;

	if (!isdigit(*msk))
		return (u_32_t)-1;
	if (strchr(msk, '.'))
		return inet_addr(msk);
	if (strchr(msk, 'x'))
		return (u_32_t)strtol(msk, NULL, 0);
	/*
	 * set x most significant bits
	 */
	for (mask = 0, bits = atoi(msk); bits; bits--) {
		mask /= 2;
		mask |= ntohl(inet_addr("128.0.0.0"));
	}
	mask = htonl(mask);
	return mask;
}


/*
 * returns an ip address as a long var as a result of either a DNS lookup or
 * straight inet_addr() call
 */
u_32_t	hostnum(host, resolved, linenum)
char	*host;
int	*resolved;
int     linenum;
{
	struct	hostent	*hp;
	struct	netent	*np;
#if defined(__OpenBSD__)
	struct in_addr	addr;
#endif

	*resolved = 0;
	if (!strcasecmp("any", host))
		return 0L;
	if (isdigit(*host))
		return inet_addr(host);

#if defined(__OpenBSD__)
	/* attempt a map from interface name to address */
	if (if_addr(host, &addr))
		return (u_32_t)addr.s_addr;
#endif
	if (!(hp = gethostbyname(host))) {
		if (!(np = getnetbyname(host))) {
			*resolved = -1;
			fprintf(stderr, "Line %d: can't resolve hostname: %s\n", linenum, host);
			return 0;
		}
		return htonl(np->n_net);
	}
	return *(u_32_t *)hp->h_addr;
}


void flushtable(fd, opts)
int fd, opts;
{
	int n = 0;

	if (opts & OPT_FLUSH) {
		n = 0;
		if (!(opts & OPT_NODO) && ioctl(fd, SIOCFLNAT, &n) == -1)
			perror("ioctl(SIOCFLNAT)");
		else
			printf("%d entries flushed from NAT table\n", n);
	}

	if (opts & OPT_CLEAR) {
		n = 0;
		if (!(opts & OPT_NODO) && ioctl(fd, SIOCCNATL, &n) == -1)
			perror("ioctl(SIOCCNATL)");
		else
			printf("%d entries flushed from NAT list\n", n);
	}
}
