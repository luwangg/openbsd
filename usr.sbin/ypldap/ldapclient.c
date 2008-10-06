/* $OpenBSD: src/usr.sbin/ypldap/ldapclient.c,v 1.4 2008/10/06 11:20:20 aschrijver Exp $ */

/*
 * Copyright (c) 2008 Alexander Schrijver <aschrijver@openbsd.org>
 * Copyright (c) 2008 Pierre-Yves Ritschard <pyr@openbsd.org>
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
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/tree.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <event.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LDAP_DEPRECATED 1
#include "aldap.h"

#include "ypldap.h"

void    client_sig_handler(int, short, void *);
void    client_dispatch_parent(int, short, void *);
void    client_shutdown(void);
void    client_connect(int, short, void *);
void    client_configure(struct env *);
void    client_configure_wrapper(int, short, void *);
int	client_try_idm(struct env *, struct idm *);
void	client_try_idm_wrapper(int, short, void *);
void	client_try_server_wrapper(int, short, void *);

static int	remote_connect(const char *, const char *, struct addrinfo);

static int
remote_connect(const char *host, const char *port, struct addrinfo hints)
{
	struct addrinfo *res, *res0;
	int s, error;

	if ((error = getaddrinfo(host, port, &hints, &res)))
		errx(1, "getaddrinfo: %s", gai_strerror(error));

	res0 = res;
	do {
		if ((s = socket(res0->ai_family, res0->ai_socktype,
		    res0->ai_protocol)) < 0)
			continue;

		if (connect(s, res0->ai_addr, res0->ai_addrlen) == 0)
			break;
		else
			warn("connect to %s port %s (%s) failed", host, port, "tcp");

		close(s);
		s = -1;
	} while ((res0 = res0->ai_next) != NULL);

	freeaddrinfo(res);

	return (s);
}

void
client_sig_handler(int sig, short event, void *p)
{
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		client_shutdown();
		break;
	default:
		fatalx("unexpected signal");
	}
}

void
client_dispatch_parent(int fd, short event, void *p)
{
	int			 n;
	int			 shut = 0;
	struct imsg		 imsg;
	struct env		*env = p;
	struct imsgbuf		*ibuf = env->sc_ibuf;


	switch (event) {
	case EV_READ:
		if ((n = imsg_read(ibuf)) == -1)
			fatal("imsg_read error");
		if (n == 0)
			shut = 1;
		break;
	case EV_WRITE:
		if (msgbuf_write(&ibuf->w) == -1)
			fatal("msgbuf_write");
		imsg_event_add(ibuf);
		return;
	default:
		fatalx("unknown event");
	}

	for (;;) {
		if ((n = imsg_get(ibuf, &imsg)) == -1)
			fatal("client_dispatch_parent: imsg_read_error");
		if (n == 0)
			break;

		switch (imsg.hdr.type) {
		case IMSG_CONF_START: {
			struct env	params;

			if (env->sc_flags & F_CONFIGURING) {
				log_warnx("configuration already in progress");
				break;
			}
			memcpy(&params, imsg.data, sizeof(params));
			log_debug("configuration starting");
			env->sc_flags |= F_CONFIGURING;
			purge_config(env);
			memcpy(&env->sc_conf_tv, &params.sc_conf_tv,
			    sizeof(env->sc_conf_tv));
			env->sc_flags |= params.sc_flags;
			break;
		}
		case IMSG_CONF_IDM: {
			struct idm	*idm;

			if (!(env->sc_flags & F_CONFIGURING))
				break;
			if ((idm = calloc(1, sizeof(*idm))) == NULL)
				fatal(NULL);
			memcpy(idm, imsg.data, sizeof(*idm));
			idm->idm_env = env;
			TAILQ_INSERT_TAIL(&env->sc_idms, idm, idm_entry);
			break;
		}
		case IMSG_CONF_END:
			env->sc_flags &= ~F_CONFIGURING;
			log_debug("applying configuration");
			client_configure(env);
			break;
		default:
			log_debug("client_dispatch_parent: unexpect imsg %d",
			    imsg.hdr.type);

			break;
		}
		imsg_free(&imsg);
	}
	if (!shut)
		imsg_event_add(ibuf);
	else {
		/* this pipe is dead, so remove the event handler */
		event_del(&ibuf->ev);
		event_loopexit(NULL);
	}
}

void
client_shutdown(void)
{
	log_info("ldap client exiting");
	_exit(0);
}

pid_t
ldapclient(int pipe_main2client[2])
{
	pid_t		 pid;
	struct passwd	*pw;
	struct event	 ev_sigint;
	struct event	 ev_sigterm;
	struct env	 env;

	switch (pid = fork()) {
	case -1:
		fatal("cannot fork");
		break;
	case 0:
		break;
	default:
		return (pid);
	}

	bzero(&env, sizeof(env));
	TAILQ_INIT(&env.sc_idms);

	if ((pw = getpwnam(YPLDAP_USER)) == NULL)
		fatal("getpwnam");

#ifndef DEBUG
	if (chroot(pw->pw_dir) == -1)
		fatal("chroot");
	if (chdir("/") == -1)
		fatal("chdir");
#else
#warning disabling chrooting in DEBUG mode
#endif
	setproctitle("ldap client");
	ypldap_process = PROC_CLIENT;

#ifndef DEBUG
	if (setgroups(1, &pw->pw_gid) ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid))
		fatal("cannot drop privileges");
#else
#warning disabling privilege revocation in DEBUG mode
#endif

	event_init();
	signal_set(&ev_sigint, SIGINT, client_sig_handler, NULL);
	signal_set(&ev_sigterm, SIGTERM, client_sig_handler, NULL);
	signal_add(&ev_sigint, NULL);
	signal_add(&ev_sigterm, NULL);

	close(pipe_main2client[0]);
	if ((env.sc_ibuf = calloc(1, sizeof(*env.sc_ibuf))) == NULL)
		fatal(NULL);

	env.sc_ibuf->events = EV_READ;
	env.sc_ibuf->data = &env;
	imsg_init(env.sc_ibuf, pipe_main2client[1], client_dispatch_parent);
	event_set(&env.sc_ibuf->ev, env.sc_ibuf->fd, env.sc_ibuf->events,
	    env.sc_ibuf->handler, &env);
	event_add(&env.sc_ibuf->ev, NULL);

	event_dispatch();
	client_shutdown();

	return (0);

}

void
client_configure_wrapper(int fd, short event, void *p)
{
	struct env	*env = p;

	client_configure(env);
}

int
client_try_idm(struct env *env, struct idm *idm)
{
	int			i, j, fd;
	char			*attrs[ATTR_MAX+1];
	char			**ldap_attrs;
	const char		*where;
	struct idm_req		 ir;
	struct addrinfo		 hints;
	struct aldap_message	*m;
	struct aldap		*al;

	log_debug("trying directory: %s", idm->idm_name);

	bzero(&ir, sizeof(ir));
	imsg_compose(env->sc_ibuf, IMSG_START_UPDATE, 0, 0, &ir, sizeof(ir));

	/* connect to LDAP server */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	where = "connect";
	fd = remote_connect(idm->idm_name, "389", hints);
	if(fd == -1)
		goto bad;

	al = aldap_init(fd);
	if(al == NULL)
		goto bad;

	/* do a bind request */
	if(aldap_bind(al, idm->idm_binddn, idm->idm_bindcred) == -1)
		goto bad;
	where = "parsing";
	if((m = aldap_parse(al)) == NULL)
		goto bad;
	if(al->msgid != m->msgid)
		goto bad; /* XXX: error reporting */
	aldap_freemsg(m);

	bzero(attrs, sizeof(attrs));
	for (i = 0, j = 0; i < ATTR_MAX; i++) {
		if (idm->idm_flags & F_FIXED_ATTR(i))
			continue;
		attrs[j++] = idm->idm_attrs[i];
	}
	attrs[j] = NULL;

	/* do a search request */
	aldap_search(al, idm->idm_basedn, LDAP_SCOPE_SUBTREE,
	    idm->idm_filters[FILTER_USER], attrs, 0, 0, 0);

	/*
	 * build password line.
	 */
	while((m = aldap_parse(al)) != NULL) {
		if(al->msgid != m->msgid)
			goto bad; /* XXX: error reporting */
		if (m->message_type == LDAP_RES_SEARCH_RESULT)
			break;
		bzero(&ir, sizeof(ir));
		for (i = 0, j = 0; i < ATTR_MAX; i++) {
			if (idm->idm_flags & F_FIXED_ATTR(i)) {
				if (strlcat(ir.ir_line, idm->idm_attrs[i],
				    sizeof(ir.ir_line)) >= sizeof(ir.ir_line))
					/*
					 * entry yields a line > 1024, trash it.
					 */
					continue;
				if (i == ATTR_UID) {
					ir.ir_key.ik_uid = strtonum(
					    idm->idm_attrs[i], 0,
					    UID_MAX, NULL);
				}
			} else {
				aldap_match_entry(m, attrs[j++], &ldap_attrs);
				if (ldap_attrs == NULL || ldap_attrs[0] == NULL)
					continue;
				if (strlcat(ir.ir_line, ldap_attrs[0],
				    sizeof(ir.ir_line)) >= sizeof(ir.ir_line))
					continue;
				if (i == ATTR_UID) {
					ir.ir_key.ik_uid = strtonum(
					    ldap_attrs[0], 0, UID_MAX, NULL);
				}
				aldap_free_entry(ldap_attrs);
			}
			if (i != ATTR_SHELL)
				if (strlcat(ir.ir_line, ":",
				    sizeof(ir.ir_line)) >= sizeof(ir.ir_line))
					continue;
		}
		imsg_compose(env->sc_ibuf, IMSG_PW_ENTRY, 0, 0,
		    &ir, sizeof(ir));
	}

	/*
	 * exact same code but for groups.
	 */

	bzero(attrs, sizeof(attrs));
	for (i = ATTR_GR_MIN, j = 0; i < ATTR_GR_MAX; i++) {
		if (idm->idm_flags & F_FIXED_ATTR(i))
			continue;
		attrs[j++] = idm->idm_attrs[i];
	}
	attrs[j] = NULL;

	aldap_search(al, idm->idm_basedn, LDAP_SCOPE_SUBTREE,
	    idm->idm_filters[FILTER_GROUP], attrs, 0, 0, 0);

	while((m = aldap_parse(al)) != NULL) {
		if(al->msgid != m->msgid)
			goto bad; /* XXX: error reporting */
		if (m->message_type == LDAP_RES_SEARCH_RESULT)
			break;
		bzero(&ir, sizeof(ir));
		for (i = ATTR_GR_MIN, j = 0; i < ATTR_GR_MAX; i++) {
			if (idm->idm_flags & F_FIXED_ATTR(i)) {
				if (strlcat(ir.ir_line, idm->idm_attrs[i],
				    sizeof(ir.ir_line)) >= sizeof(ir.ir_line))
					/*
					 * entry yields a line > 1024, trash it.
					 */
					continue;
				if (i == ATTR_GR_GID) {
					ir.ir_key.ik_gid = strtonum(
					    idm->idm_attrs[i], 0,
					    GID_MAX, NULL);
				}
			} else {
				aldap_match_entry(m, attrs[j++], &ldap_attrs);
				if (ldap_attrs == NULL || ldap_attrs[0] == NULL)
					continue;
				if (strlcat(ir.ir_line, ldap_attrs[0],
				    sizeof(ir.ir_line)) >= sizeof(ir.ir_line))
					continue;
				if (i == ATTR_GR_GID) {
					ir.ir_key.ik_uid = strtonum(
					    ldap_attrs[0], 0, GID_MAX, NULL);
				}
				aldap_free_entry(ldap_attrs);
			}
			if (i != ATTR_GR_MEMBERS)
				if (strlcat(ir.ir_line, ":",
				    sizeof(ir.ir_line)) >= sizeof(ir.ir_line))
					continue;
		}
		imsg_compose(env->sc_ibuf, IMSG_GRP_ENTRY, 0, 0,
		    &ir, sizeof(ir));
	}

	return (0);
bad:
	log_debug("directory %s errored out in %s", idm->idm_name, where);
	return (-1);
}

void
client_configure(struct env *env)
{
	enum imsg_type	 finish;
	struct timeval	 tv;
	struct idm	*idm;

	log_debug("connecting to directories");
	finish = IMSG_END_UPDATE;
	imsg_compose(env->sc_ibuf, IMSG_START_UPDATE, 0, 0, NULL, 0);
	TAILQ_FOREACH(idm, &env->sc_idms, idm_entry)
		if (client_try_idm(env, idm) == -1) {
			finish = IMSG_TRASH_UPDATE;
			break;
		}
	imsg_compose(env->sc_ibuf, finish, 0, 0, NULL, 0);
	tv.tv_sec = env->sc_conf_tv.tv_sec;
	tv.tv_usec = env->sc_conf_tv.tv_usec;
	evtimer_set(&env->sc_conf_ev, client_configure_wrapper, env);
	evtimer_add(&env->sc_conf_ev, &tv);
}
