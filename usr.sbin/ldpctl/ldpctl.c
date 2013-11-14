/*	$OpenBSD: src/usr.sbin/ldpctl/ldpctl.c,v 1.16 2013/11/14 20:48:52 deraadt Exp $
 *
 * Copyright (c) 2009 Michele Marchetto <michele@openbsd.org>
 * Copyright (c) 2005 Claudio Jeker <claudio@openbsd.org>
 * Copyright (c) 2004, 2005 Esben Norby <norby@openbsd.org>
 * Copyright (c) 2003 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netmpls/mpls.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ldp.h"
#include "ldpd.h"
#include "ldpe.h"
#include "parser.h"

__dead void	 usage(void);
const char	*fmt_timeframe_core(time_t);
const char	*get_linkstate(int, int);
int		 show_interface_msg(struct imsg *);
int		 show_discovery_msg(struct imsg *);
int		 get_ifms_type(int);
int		 show_lib_msg(struct imsg *);
int		 show_nbr_msg(struct imsg *);
void		 show_fib_head(void);
int		 show_fib_msg(struct imsg *);
void		 show_interface_head(void);
int		 show_fib_interface_msg(struct imsg *);
const char	*get_media_descr(int);
void		 print_baudrate(u_int64_t);

struct imsgbuf	*ibuf;

__dead void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s command [argument ...]\n", __progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct sockaddr_un	 sun;
	struct parse_result	*res;
	struct imsg		 imsg;
	unsigned int		 ifidx = 0;
	int			 ctl_sock;
	int			 done = 0, verbose = 0;
	int			 n;

	/* parse options */
	if ((res = parse(argc - 1, argv + 1)) == NULL)
		exit(1);

	/* connect to ldpd control socket */
	if ((ctl_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	bzero(&sun, sizeof(sun));
	sun.sun_family = AF_UNIX;
	strlcpy(sun.sun_path, LDPD_SOCKET, sizeof(sun.sun_path));
	if (connect(ctl_sock, (struct sockaddr *)&sun, sizeof(sun)) == -1)
		err(1, "connect: %s", LDPD_SOCKET);

	if ((ibuf = malloc(sizeof(struct imsgbuf))) == NULL)
		err(1, NULL);
	imsg_init(ibuf, ctl_sock);
	done = 0;

	/* process user request */
	switch (res->action) {
	case NONE:
		usage();
		/* not reached */
	case SHOW:
	case SHOW_IFACE:
		printf("%-11s %-10s %-10s %-8s %-12s %3s\n",
		    "Interface", "State", "Linkstate", "Uptime",
		    "Hello Timers", "ac");
		if (*res->ifname) {
			ifidx = if_nametoindex(res->ifname);
			if (ifidx == 0)
				errx(1, "no such interface %s", res->ifname);
		}
		imsg_compose(ibuf, IMSG_CTL_SHOW_INTERFACE, 0, 0, -1,
		    &ifidx, sizeof(ifidx));
		break;
	case SHOW_DISC:
		printf("%-15s %-9s %-15s %-9s\n",
		    "ID", "Type", "Source", "Holdtime");
		imsg_compose(ibuf, IMSG_CTL_SHOW_DISCOVERY, 0, 0, -1,
		    NULL, 0);
		break;
	case SHOW_NBR:
		printf("%-15s %-18s %-15s %-10s\n", "ID",
		    "State", "Address", "Uptime");
		imsg_compose(ibuf, IMSG_CTL_SHOW_NBR, 0, 0, -1, NULL, 0);
		break;
	case SHOW_LIB:
		printf("%-20s %-17s %-14s %-14s %-10s\n", "Destination",
		    "Nexthop", "Local Label", "Remote Label", "In Use");
		imsg_compose(ibuf, IMSG_CTL_SHOW_LIB, 0, 0, -1, NULL, 0);
		break;
	case SHOW_FIB:
		if (!res->addr.s_addr)
			imsg_compose(ibuf, IMSG_CTL_KROUTE, 0, 0, -1,
			    &res->flags, sizeof(res->flags));
		else
			imsg_compose(ibuf, IMSG_CTL_KROUTE_ADDR, 0, 0, -1,
			    &res->addr, sizeof(res->addr));
		show_fib_head();
		break;
	case SHOW_FIB_IFACE:
		if (*res->ifname)
			imsg_compose(ibuf, IMSG_CTL_IFINFO, 0, 0, -1,
			    res->ifname, sizeof(res->ifname));
		else
			imsg_compose(ibuf, IMSG_CTL_IFINFO, 0, 0, -1, NULL, 0);
		show_interface_head();
		break;
	case FIB:
		errx(1, "fib couple|decouple");
		break;
	case FIB_COUPLE:
		imsg_compose(ibuf, IMSG_CTL_FIB_COUPLE, 0, 0, -1, NULL, 0);
		printf("couple request sent.\n");
		done = 1;
		break;
	case FIB_DECOUPLE:
		imsg_compose(ibuf, IMSG_CTL_FIB_DECOUPLE, 0, 0, -1, NULL, 0);
		printf("decouple request sent.\n");
		done = 1;
		break;
	case LOG_VERBOSE:
		verbose = 1;
		/* FALLTHROUGH */
	case LOG_BRIEF:
		imsg_compose(ibuf, IMSG_CTL_LOG_VERBOSE, 0, 0, -1,
		    &verbose, sizeof(verbose));
		printf("logging request sent.\n");
		done = 1;
		break;
	case RELOAD:
		imsg_compose(ibuf, IMSG_CTL_RELOAD, 0, 0, -1, NULL, 0);
		printf("reload request sent.\n");
		done = 1;
		break;
	}

	while (ibuf->w.queued)
		if (msgbuf_write(&ibuf->w) <= 0 && errno != EAGAIN)
			err(1, "write error");

	while (!done) {
		if ((n = imsg_read(ibuf)) == -1)
			errx(1, "imsg_read error");
		if (n == 0)
			errx(1, "pipe closed");

		while (!done) {
			if ((n = imsg_get(ibuf, &imsg)) == -1)
				errx(1, "imsg_get error");
			if (n == 0)
				break;
			switch (res->action) {
			case SHOW:
			case SHOW_IFACE:
				done = show_interface_msg(&imsg);
				break;
			case SHOW_DISC:
				done = show_discovery_msg(&imsg);
				break;
			case SHOW_NBR:
				done = show_nbr_msg(&imsg);
				break;
			case SHOW_LIB:
				done = show_lib_msg(&imsg);
				break;
			case SHOW_FIB:
				done = show_fib_msg(&imsg);
				break;
			case SHOW_FIB_IFACE:
				done = show_fib_interface_msg(&imsg);
				break;
			case NONE:
			case FIB:
			case FIB_COUPLE:
			case FIB_DECOUPLE:
			case LOG_VERBOSE:
			case LOG_BRIEF:
			case RELOAD:
				break;
			}
			imsg_free(&imsg);
		}
	}
	close(ctl_sock);
	free(ibuf);

	return (0);
}

int
get_ifms_type(int mediatype)
{
	switch (mediatype) {
	case IFT_ETHER:
		return (IFM_ETHER);
		break;
	case IFT_FDDI:
		return (IFM_FDDI);
		break;
	case IFT_CARP:
		return (IFM_CARP);
		break;
	default:
		return (0);
		break;
	}
}

#define	TF_BUFS	8
#define	TF_LEN	9

const char *
fmt_timeframe_core(time_t t)
{
	char		*buf;
	static char	 tfbuf[TF_BUFS][TF_LEN];	/* ring buffer */
	static int	 idx = 0;
	unsigned int	 sec, min, hrs, day, week;

	if (t == 0)
		return ("Stopped");

	buf = tfbuf[idx++];
	if (idx == TF_BUFS)
		idx = 0;

	week = t;

	sec = week % 60;
	week /= 60;
	min = week % 60;
	week /= 60;
	hrs = week % 24;
	week /= 24;
	day = week % 7;
	week /= 7;

	if (week > 0)
		snprintf(buf, TF_LEN, "%02uw%01ud%02uh", week, day, hrs);
	else if (day > 0)
		snprintf(buf, TF_LEN, "%01ud%02uh%02um", day, hrs, min);
	else
		snprintf(buf, TF_LEN, "%02u:%02u:%02u", hrs, min, sec);

	return (buf);
}

/* prototype defined in ldpd.h and shared with the kroute.c version */
u_int8_t
mask2prefixlen(in_addr_t ina)
{
	if (ina == 0)
		return (0);
	else
		return (33 - ffs(ntohl(ina)));
}

int
show_interface_msg(struct imsg *imsg)
{
	struct ctl_iface	*iface;
	char			*timers;

	switch (imsg->hdr.type) {
	case IMSG_CTL_SHOW_INTERFACE:
		iface = imsg->data;

		if (asprintf(&timers, "%u/%u", iface->hello_interval,
		    iface->hello_holdtime) == -1)
			err(1, NULL);

		printf("%-11s %-10s %-10s %-8s %12s %3u\n",
		    iface->name, if_state_name(iface->state),
		    get_linkstate(iface->mediatype, iface->linkstate),
		    iface->uptime == 0 ? "00:00:00" :
		    fmt_timeframe_core(iface->uptime), timers,
		    iface->adj_cnt);
		free(timers);
		break;
	case IMSG_CTL_END:
		printf("\n");
		return (1);
	default:
		break;
	}

	return (0);
}

int
show_discovery_msg(struct imsg *imsg)
{
	struct ctl_adj		*adj;

	switch (imsg->hdr.type) {
	case IMSG_CTL_SHOW_DISCOVERY:
		adj = imsg->data;

		printf("%-15s ", inet_ntoa(adj->id));
		switch(adj->type) {
		case HELLO_LINK:
			printf("%-9s %-15s ", "Link", adj->ifname);
			break;
		case HELLO_TARGETED:
			printf("%-9s %-15s ", "Targeted",
			    inet_ntoa(adj->src_addr));
			break;
		}
		printf("%-9u\n", adj->holdtime);
		break;
	case IMSG_CTL_END:
		printf("\n");
		return (1);
	default:
		break;
	}

	return (0);
}

int
show_lib_msg(struct imsg *imsg)
{
	struct ctl_rt	*rt;
	char		*dstnet, *remote;

	switch (imsg->hdr.type) {
	case IMSG_CTL_SHOW_LIB:
		rt = imsg->data;
		if (asprintf(&dstnet, "%s/%d", inet_ntoa(rt->prefix),
		    rt->prefixlen) == -1)
			err(1, NULL);
		if (!rt->in_use) {
			if (asprintf(&remote, "-") == -1)
				err(1, NULL);
		} else if (rt->connected || rt->remote_label == NO_LABEL) {
			if (asprintf(&remote, "Untagged") == -1)
				err(1, NULL);
		} else if (rt->remote_label == MPLS_LABEL_IMPLNULL) {
			if (asprintf(&remote, "Pop tag") == -1)
				err(1, NULL);
		} else {
			if (asprintf(&remote, "%u", rt->remote_label) == -1)
				err(1, NULL);
		}

		printf("%-20s %-17s %-14u %-14s %s\n", dstnet,
		    inet_ntoa(rt->nexthop), rt->local_label, remote,
		    rt->in_use ? "yes" : "no");
		free(remote);
		free(dstnet);

		break;
	case IMSG_CTL_END:
		printf("\n");
		return (1);
	default:
		break;
	}

	return (0);
}

int
show_nbr_msg(struct imsg *imsg)
{
	struct ctl_nbr	*nbr;

	switch (imsg->hdr.type) {
	case IMSG_CTL_SHOW_NBR:
		nbr = imsg->data;
		printf("%-15s %-19s", inet_ntoa(nbr->id),
		    nbr_state_name(nbr->nbr_state));
		printf("%-15s %-15s\n", inet_ntoa(nbr->addr),
		    nbr->uptime == 0 ? "-" : fmt_timeframe_core(nbr->uptime));
		break;
	case IMSG_CTL_END:
		printf("\n");
		return (1);
	default:
		break;
	}

	return (0);
}

void
show_fib_head(void)
{
	printf("Flags: C = Connected, S = Static\n");
	printf(" %-4s %-20s %-17s %-17s %s\n", "Prio", "Destination",
	    "Nexthop", "Local Label", "Remote Label");
}

int
show_fib_msg(struct imsg *imsg)
{
	struct kroute		*k;
	char			*p;

	switch (imsg->hdr.type) {
	case IMSG_CTL_KROUTE:
		if (imsg->hdr.len < IMSG_HEADER_SIZE + sizeof(struct kroute))
			errx(1, "wrong imsg len");
		k = imsg->data;

		if (k->flags & F_CONNECTED)
			printf("C");
		else if (k->flags & F_STATIC)
			printf("S");
		else
			printf(" ");

		printf(" %3d ", k->priority);
		if (asprintf(&p, "%s/%u", inet_ntoa(k->prefix),
		    k->prefixlen) == -1)
			err(1, NULL);
		printf("%-20s ", p);
		free(p);

		if (k->nexthop.s_addr)
			printf("%-18s", inet_ntoa(k->nexthop));
		else if (k->flags & F_CONNECTED)
			printf("link#%-13u", k->ifindex);

		if (k->local_label == NO_LABEL) {
			printf("%-18s", "-");
		} else if (k->local_label == MPLS_LABEL_IMPLNULL) {
			printf("%-18s", "imp-null");
		} else
			printf("%-18u", k->local_label);

		if (k->remote_label == NO_LABEL) {
			printf("-");
		} else if (k->remote_label == MPLS_LABEL_IMPLNULL) {
			printf("Pop");
		} else {
			printf("%u", k->remote_label);
		}

		printf("\n");

		break;
	case IMSG_CTL_END:
		printf("\n");
		return (1);
	default:
		break;
	}

	return (0);
}

void
show_interface_head(void)
{
	printf("%-15s%-15s%s\n", "Interface", "Flags",
	    "Link state");
}

int
show_fib_interface_msg(struct imsg *imsg)
{
	struct kif	*k;
	int		 ifms_type;

	switch (imsg->hdr.type) {
	case IMSG_CTL_IFINFO:
		k = imsg->data;
		printf("%-15s", k->ifname);
		printf("%-15s", k->flags & IFF_UP ? "UP" : "");
		ifms_type = get_ifms_type(k->media_type);
		if (ifms_type)
			printf("%s, ", get_media_descr(ifms_type));

		printf("%s", get_linkstate(k->media_type, k->link_state));

		if (k->link_state != LINK_STATE_DOWN && k->baudrate > 0) {
			printf(", ");
			print_baudrate(k->baudrate);
		}
		printf("\n");
		break;
	case IMSG_CTL_END:
		printf("\n");
		return (1);
	default:
		break;
	}

	return (0);
}

const struct if_status_description
		if_status_descriptions[] = LINK_STATE_DESCRIPTIONS;
const struct ifmedia_description
		ifm_type_descriptions[] = IFM_TYPE_DESCRIPTIONS;

const char *
get_media_descr(int media_type)
{
	const struct ifmedia_description	*p;

	for (p = ifm_type_descriptions; p->ifmt_string != NULL; p++)
		if (media_type == p->ifmt_word)
			return (p->ifmt_string);

	return ("unknown");
}

const char *
get_linkstate(int media_type, int link_state)
{
	const struct if_status_description *p;
	static char buf[8];

	for (p = if_status_descriptions; p->ifs_string != NULL; p++) {
		if (LINK_STATE_DESC_MATCH(p, media_type, link_state))
			return (p->ifs_string);
	}
	snprintf(buf, sizeof(buf), "[#%d]", link_state);
	return (buf);
}

void
print_baudrate(u_int64_t baudrate)
{
	if (baudrate > IF_Gbps(1))
		printf("%llu GBit/s", baudrate / IF_Gbps(1));
	else if (baudrate > IF_Mbps(1))
		printf("%llu MBit/s", baudrate / IF_Mbps(1));
	else if (baudrate > IF_Kbps(1))
		printf("%llu KBit/s", baudrate / IF_Kbps(1));
	else
		printf("%llu Bit/s", baudrate);
}
