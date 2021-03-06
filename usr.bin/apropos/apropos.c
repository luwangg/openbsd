/*      $OpenBSD: src/usr.bin/apropos/apropos.c,v 1.16 2013/11/25 18:02:49 deraadt Exp $      */
/*      $NetBSD: apropos.c,v 1.5 1995/09/04 20:46:20 tls Exp $      */

/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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

#include <sys/param.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../man/config.h"
#include "../man/pathnames.h"

static int *found, foundman;

#define	MAXLINELEN	8192		/* max line handled */

void apropos(char **, char *, int, char *, char *);
void lowstr(char *, char *);
int match(char *, char *);
void usage(void);

int
main(int argc, char *argv[])
{
	ENTRY *ep;
	TAG *tp;
	int ch, rv;
	char *conffile, *machine, **p, *p_augment, *p_path, *sflag;

	conffile = NULL;
	p_augment = p_path = NULL;
	machine = sflag = NULL;
	while ((ch = getopt(argc, argv, "C:M:m:P:S:s:")) != -1)
		switch (ch) {
		case 'C':
			conffile = optarg;
			break;
		case 'M':
		case 'P':		/* backward compatible */
			p_path = optarg;
			break;
		case 'm':
			p_augment = optarg;
			break;
		case 'S':
			machine = optarg;
			lowstr(machine, machine);
			break;
		case 's':
			sflag = optarg;
			lowstr(sflag, sflag);
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc < 1)
		usage();

	if ((found = calloc(argc, sizeof(int))) == NULL)
		err(1, NULL);

	for (p = argv; *p; ++p)			/* convert to lower-case */
		lowstr(*p, *p);

	if (p_augment)
		apropos(argv, p_augment, 1, sflag, machine);
	if (p_path || (p_path = getenv("MANPATH")))
		apropos(argv, p_path, 1, sflag, machine);
	else {
		config(conffile);
		ep = (tp = getlist("_whatdb")) == NULL ?
		    NULL : TAILQ_FIRST(&tp->list);
		for (; ep != NULL; ep = TAILQ_NEXT(ep, q))
			apropos(argv, ep->s, 0, sflag, machine);
	}

	if (!foundman)
		errx(1, "no %s file found", _PATH_WHATIS);

	rv = 1;
	for (p = argv; *p; ++p)
		if (found[p - argv])
			rv = 0;
		else
			(void)printf("%s: nothing appropriate\n", *p);
	exit(rv);
}

void
apropos(char **argv, char *path, int buildpath, char *sflag, char *machine)
{
	char *end, *name, **p;
	char buf[MAXLINELEN + 1], wbuf[MAXLINELEN + 1];
	char hold[MAXPATHLEN];
	size_t slen = 0, mlen = 0;

	if (sflag)
		slen = strlen(sflag);
	if (machine)
		mlen = strlen(machine);

	for (name = path; name; name = end) {	/* through name list */
		if ((end = strchr(name, ':')))
			*end++ = '\0';

		if (buildpath) {
			(void)snprintf(hold, sizeof(hold), "%s/%s", name,
			    _PATH_WHATIS);
			name = hold;
		}

		if (!freopen(name, "r", stdin))
			continue;

		foundman = 1;

		/* for each file found */
		while (fgets(buf, sizeof(buf), stdin)) {
			if (!strchr(buf, '\n')) {
				warnx("%s: line too long", name);
				continue;
			}
			lowstr(buf, wbuf);
			if (sflag || machine) {
				char *s = strstr(wbuf, ") - ");
				if (!s)
					continue;
				while (s > wbuf && *s != '/' && *s != '(')
					s--;
				if (machine && *s == '/' &&
				    strncmp(s+1, machine, mlen))
					continue;
				while (s > wbuf && *s != '(')
					s--;
				if (sflag && *s == '(' &&
				    strncmp(s+1, sflag, slen))
					continue;
			}
			for (p = argv; *p; ++p)
				if (match(wbuf, *p)) {
					(void)printf("%s", buf);
					found[p - argv] = 1;

					/* only print line once */
					while (*++p)
						if (match(wbuf, *p))
							found[p - argv] = 1;
					break;
				}
		}
	}
}

/*
 * match --
 *	match anywhere the string appears
 */
int
match(char *bp, char *str)
{
	int len;
	char test;

	if (!*bp)
		return (0);
	/* backward compatible: everything matches empty string */
	if (!*str)
		return (1);
	for (test = *str++, len = strlen(str); *bp;)
		if (test == *bp++ && !strncmp(bp, str, len))
			return (1);
	return (0);
}

/*
 * lowstr --
 *	convert a string to lower case
 */
void
lowstr(char *from, char *to)
{
	unsigned char ch;

	while ((ch = *from++) && ch != '\n')
		*to++ = isupper(ch) ? tolower(ch) : ch;
	*to = '\0';
}

/*
 * usage --
 *	print usage message and die
 */
void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: apropos [-C file] [-M path] [-m path] "
	    "[-S subsection] [-s section]\n"
	    "       keyword ...\n");
	exit(1);
}
