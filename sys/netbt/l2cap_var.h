/*	$OpenBSD: src/sys/netbt/Attic/l2cap_var.h,v 1.1 2005/01/14 12:04:02 grange Exp $	*/

/*
 * ng_btsocket_l2cap.h
 *
 * Copyright (c) 2001-2002 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: ng_btsocket_l2cap.h,v 1.4 2003/03/25 23:53:33 max Exp $
 * $FreeBSD: src/sys/netgraph/bluetooth/include/ng_btsocket_l2cap.h,v 1.3 2003/11/14 03:45:29 emax Exp $
 */

#ifndef _NETGRAPH_BTSOCKET_L2CAP_H_
#define _NETGRAPH_BTSOCKET_L2CAP_H_

#include <sys/timeout.h>

/*
 * L2CAP routing entry
 */

struct ng_hook;
struct ng_message;

struct ng_btsocket_l2cap_rtentry {
	bdaddr_t				 src;  /* source BD_ADDR */
	struct ng_hook				*hook; /* downstream hook */
	LIST_ENTRY(ng_btsocket_l2cap_rtentry)	 next; /* link to next */
};
typedef struct ng_btsocket_l2cap_rtentry	ng_btsocket_l2cap_rtentry_t;
typedef struct ng_btsocket_l2cap_rtentry *	ng_btsocket_l2cap_rtentry_p;

/*****************************************************************************
 *****************************************************************************
 **                          SOCK_RAW L2CAP sockets                         **
 *****************************************************************************
 *****************************************************************************/

#define NG_BTSOCKET_L2CAP_RAW_SENDSPACE	NG_L2CAP_MTU_DEFAULT
#define NG_BTSOCKET_L2CAP_RAW_RECVSPACE	NG_L2CAP_MTU_DEFAULT

/*
 * Bluetooth raw L2CAP socket PCB
 */

struct ng_btsocket_l2cap_raw_pcb {
	struct socket				*so;	/* socket */

	u_int32_t				 flags; /* flags */
#define NG_BTSOCKET_L2CAP_RAW_PRIVILEGED	(1 << 0)

	bdaddr_t				 src;	/* source address */
	bdaddr_t				 dst;	/* dest address */
	ng_btsocket_l2cap_rtentry_p		 rt;    /* routing info */

	u_int32_t				 token;	/* message token */
	struct ng_mesg				*msg;   /* message */

#if 0
	struct mtx				 pcb_mtx; /* pcb mutex */
#endif

	LIST_ENTRY(ng_btsocket_l2cap_raw_pcb)	 next;  /* link to next PCB */
};
typedef struct ng_btsocket_l2cap_raw_pcb	ng_btsocket_l2cap_raw_pcb_t;
typedef struct ng_btsocket_l2cap_raw_pcb *	ng_btsocket_l2cap_raw_pcb_p;

#define	so2l2cap_raw_pcb(so) \
	((struct ng_btsocket_l2cap_raw_pcb *)((so)->so_pcb))

/*
 * Bluetooth raw L2CAP socket methods
 */

#ifdef _KERNEL

void l2cap_raw_init(void);
int  ng_btsocket_l2cap_raw_abort      (struct socket *);
int  ng_btsocket_l2cap_raw_attach     (struct socket *, int, struct proc *);
int  ng_btsocket_l2cap_raw_bind       (struct socket *, struct sockaddr *,
                                       struct proc *);
int  ng_btsocket_l2cap_raw_connect    (struct socket *, struct sockaddr *,
                                       struct proc *);
int  ng_btsocket_l2cap_raw_control    (struct socket *, u_long, caddr_t,
                                       struct ifnet *, struct proc *);
int  ng_btsocket_l2cap_raw_detach     (struct socket *);
int  ng_btsocket_l2cap_raw_disconnect (struct socket *);
int  ng_btsocket_l2cap_raw_peeraddr   (struct socket *, struct sockaddr **);
int  ng_btsocket_l2cap_raw_send       (struct socket *, int, struct mbuf *,
                                       struct sockaddr *, struct mbuf *,
                                       struct proc *);
int  ng_btsocket_l2cap_raw_sockaddr   (struct socket *, struct sockaddr **);

#endif /* _KERNEL */

/*****************************************************************************
 *****************************************************************************
 **                    SOCK_SEQPACKET L2CAP sockets                         **
 *****************************************************************************
 *****************************************************************************/

#define NG_BTSOCKET_L2CAP_SENDSPACE	NG_L2CAP_MTU_DEFAULT /* (64 * 1024) */
#define NG_BTSOCKET_L2CAP_RECVSPACE	(64 * 1024)

/*
 * Bluetooth L2CAP socket PCB
 */

struct ng_btsocket_l2cap_pcb {
	struct socket			*so;	     /* Pointer to socket */

	bdaddr_t			 src;	     /* Source address */
	bdaddr_t			 dst;	     /* Destination address */

	u_int16_t			 psm;	     /* PSM */
	u_int16_t			 cid;	     /* Local channel ID */

	u_int16_t			 flags;      /* socket flags */
#define NG_BTSOCKET_L2CAP_CLIENT	(1 << 0)     /* socket is client */
#define NG_BTSOCKET_L2CAP_TIMO		(1 << 1)     /* timeout pending */

	u_int8_t			 state;      /* socket state */
#define NG_BTSOCKET_L2CAP_CLOSED	0            /* socket closed */
#define NG_BTSOCKET_L2CAP_CONNECTING	1            /* wait for connect */
#define NG_BTSOCKET_L2CAP_CONFIGURING	2            /* wait for config */
#define NG_BTSOCKET_L2CAP_OPEN		3            /* socket open */
#define NG_BTSOCKET_L2CAP_DISCONNECTING	4            /* wait for disconnect */

	u_int8_t			 cfg_state;  /* config state */
#define	NG_BTSOCKET_L2CAP_CFG_IN	(1 << 0)     /* incoming path done */
#define	NG_BTSOCKET_L2CAP_CFG_OUT	(1 << 1)     /* outgoing path done */
#define	NG_BTSOCKET_L2CAP_CFG_BOTH \
	(NG_BTSOCKET_L2CAP_CFG_IN | NG_BTSOCKET_L2CAP_CFG_OUT)

#define	NG_BTSOCKET_L2CAP_CFG_IN_SENT	(1 << 2)     /* L2CAP ConfigReq sent */
#define	NG_BTSOCKET_L2CAP_CFG_OUT_SENT	(1 << 3)     /* ---/--- */

	u_int16_t			 imtu;       /* Incoming MTU */
	ng_l2cap_flow_t			 iflow;      /* Input flow spec */

	u_int16_t			 omtu;       /* Outgoing MTU */
	ng_l2cap_flow_t			 oflow;      /* Outgoing flow spec */

	u_int16_t			 flush_timo; /* flush timeout */   
	u_int16_t			 link_timo;  /* link timeout */ 

	struct timeout			 timo;       /* timeout */

	u_int32_t			 token;	     /* message token */
	ng_btsocket_l2cap_rtentry_p	 rt;         /* routing info */

#if 0
	struct mtx			 pcb_mtx;    /* pcb mutex */
#endif

	LIST_ENTRY(ng_btsocket_l2cap_pcb) next;      /* link to next PCB */
};
typedef struct ng_btsocket_l2cap_pcb	ng_btsocket_l2cap_pcb_t;
typedef struct ng_btsocket_l2cap_pcb *	ng_btsocket_l2cap_pcb_p;

#define	so2l2cap_pcb(so) \
	((struct ng_btsocket_l2cap_pcb *)((so)->so_pcb))

/*
 * Bluetooth L2CAP socket methods
 */

#ifdef _KERNEL

void l2cap_init(void);
int  ng_btsocket_l2cap_abort      (struct socket *);
int  ng_btsocket_l2cap_accept     (struct socket *, struct sockaddr **);
int  ng_btsocket_l2cap_attach     (struct socket *, int, struct proc *);
int  ng_btsocket_l2cap_bind       (struct socket *, struct sockaddr *,
                                   struct proc *);
int  ng_btsocket_l2cap_connect    (struct socket *, struct sockaddr *,
                                   struct proc *);
int  ng_btsocket_l2cap_control    (struct socket *, u_long, caddr_t,
                                   struct ifnet *, struct proc *);
int  l2cap_raw_ctloutput(int, struct socket *, int, int, struct mbuf **);
int  ng_btsocket_l2cap_detach     (struct socket *);
int  ng_btsocket_l2cap_disconnect (struct socket *);
int  ng_btsocket_l2cap_listen     (struct socket *, struct proc *);
int  ng_btsocket_l2cap_peeraddr   (struct socket *, struct sockaddr **);
int  ng_btsocket_l2cap_send       (struct socket *, int, struct mbuf *,
                                   struct sockaddr *, struct mbuf *,
                                   struct proc *);
int  ng_btsocket_l2cap_sockaddr   (struct socket *, struct sockaddr **);

int  l2cap_raw_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
	struct mbuf *);

#endif /* _KERNEL */

#endif /* _NETGRAPH_BTSOCKET_L2CAP_H_ */
