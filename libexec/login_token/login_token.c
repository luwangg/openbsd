/*	$OpenBSD: src/libexec/login_token/login_token.c,v 1.1 2000/12/20 01:41:53 millert Exp $	*/

/*-
 * Copyright (c) 1995, 1996 Berkeley Software Design, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Berkeley Software Design,
 *      Inc.
 * 4. The name of Berkeley Software Design, Inc.  may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI $From: login_token.c,v 1.2 1996/09/04 05:33:05 prb Exp $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <err.h>
#include <readpassphrase.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <login_cap.h>
#include <bsd_auth.h>

#include "token.h"

int
main(argc, argv)
	int argc;
	char **argv;
{
	FILE *back = NULL;
	char *class = 0;
	char *username = 0;
	char *instance;
	char challenge[1024];
	char response[1024];
	char *pp;
	int c;
	int mode = 0;
	struct rlimit cds;

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog(NULL, LOG_ODELAY, LOG_AUTH);

	cds.rlim_cur = 0;
	cds.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &cds) < 0)
		syslog(LOG_ERR, "couldn't set core dump size to 0: %m");

	if (token_init(argv[0]) < 0) {
		syslog(LOG_ERR, "unknown token type");
		errx(1, "unknown token type");
	}

	while ((c = getopt(argc, argv, "ds:v:")) != EOF)
		switch(c) {
		case 'd':		/* to remain undocumented */
			back = stdout;
			break;
		case 'v':
			break;
		case 's':	/* service */
			if (strcmp(optarg, "login") == 0)
				mode = 0;
			else if (strcmp(optarg, "challenge") == 0)
				mode = 1;
			else if (strcmp(optarg, "response") == 0)
				mode = 2;
			else {
				syslog(LOG_ERR, "%s: invalid service", optarg);
				exit(1);
			}
			break;
		default:
			syslog(LOG_ERR, "usage error");
			exit(1);
		}

	switch(argc - optind) {
	case 2:
		class = argv[optind + 1];
	case 1:
		username = argv[optind];
		break;
	default:
		syslog(LOG_ERR, "usage error");
		exit(1);
	}


	if (back == NULL && (back = fdopen(3, "r+")) == NULL)  {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}
	if (mode == 2) {
		mode = 0;
		c = -1;
		while (++c < sizeof(challenge) &&
		    read(3, &challenge[c], 1) == 1) {
			if (challenge[c] == '\0' && ++mode == 2)
				break;
			if (challenge[c] == '\0' && mode == 1)
				pp = challenge + c + 1;
		}
		if (mode < 2) {
			syslog(LOG_ERR, "protocol error on back channel");
			exit(1);
		}
	} else {
		tokenchallenge(username, challenge, sizeof(challenge),
		    tt->proper);
		if (mode == 1) {
			fprintf(back, BI_VALUE " challenge %s\n",
			    auth_mkvalue(challenge));
			fprintf(back, BI_CHALLENGE "\n");
			exit(0);
		}

		pp = readpassphrase(challenge, response, sizeof(response), 0);
		if (!pp || *pp == '\0') {
			char buf[64];
			snprintf(buf, sizeof(buf), "%s Response [echo on]: ",
			    tt->proper);
			pp = readpassphrase(buf, response, sizeof(response),
			    RPP_ECHO_ON);
		}
	}

	if (tokenverify(username, challenge, pp) == 0) {
		fprintf(back, BI_AUTH "\n");

		if ((instance = strchr(username, '.'))) {
			*instance++ = 0;
			if (strcmp(instance, "root") == 0)
				fprintf(back, BI_ROOTOKAY "\n");
		}
		fprintf(back, BI_SECURE "\n");
		exit(0);
	}

	fprintf(back, BI_REJECT "\n");
	exit(1);
}
