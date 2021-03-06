/*	$OpenBSD: src/usr.bin/passwd/passwd.c,v 1.26 2014/05/19 15:05:13 tedu Exp $	*/

/*
 * Copyright (c) 1988 The Regents of the University of California.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <rpcsvc/ypclnt.h>

/*
 * Note on configuration:
 *      Generally one would not use both Kerberos and YP
 *      to maintain passwords.
 *
 */

int use_kerberos;
int use_yp;

#ifdef YP
int force_yp;
#endif

extern int local_passwd(char *, int);
extern int yp_passwd(char *);
extern int krb5_passwd(int, char **);
extern int _yp_check(char **);
void usage(int retval);

int
main(int argc, char **argv)
{
	extern int optind;
	char *username;
	int ch;
#ifdef	YP
	int status = 0;
#endif
#ifdef	YP
	use_yp = _yp_check(NULL);
	if (use_yp) {
		char *dom;

		yp_get_default_domain(&dom);
		yp_unbind(dom);
	}
#endif

	/* Process args and options */
	while ((ch = getopt(argc, argv, "ly")) != -1)
		switch (ch) {
		case 'l':		/* change local password file */
			use_kerberos = 0;
			use_yp = 0;
			break;
		case 'y':		/* change YP password */
#ifdef	YP
			if (!use_yp) {
				fprintf(stderr, "passwd: YP not in use.\n");
				exit(1);
			}
			use_kerberos = 0;
			use_yp = 1;
			force_yp = 1;
			break;
#else
			fprintf(stderr, "passwd: YP not compiled in\n");
			exit(1);
#endif
		default:
			usage(1);
		}

	argc -= optind;
	argv += optind;

	username = getlogin();
	if (username == NULL) {
		fprintf(stderr, "passwd: who are you ??\n");
		exit(1);
	}

	switch (argc) {
	case 0:
		break;
	case 1:
		username = argv[0];
		break;
	default:
		usage(1);
	}

#ifdef	YP
	if (force_yp || ((status = local_passwd(username, 0)) && use_yp))
		exit(yp_passwd(username));
	exit(status);
#else
	exit(local_passwd(username, 0));
#endif
}

void
usage(int retval)
{
	fprintf(stderr, "usage: passwd [-l | -y] [user]\n");
	exit(retval);
}
