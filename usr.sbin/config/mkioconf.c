/*	$OpenBSD: src/usr.sbin/config/mkioconf.c,v 1.6 1996/09/06 08:53:44 maja Exp $	*/
/*	$NetBSD: mkioconf.c,v 1.38 1996/03/17 06:29:27 cgd Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	from: @(#)mkioconf.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

/*
 * Make ioconf.c.
 */
static int cforder __P((const void *, const void *));
static int emitcfdata __P((FILE *));
static int emitexterns __P((FILE *));
static int emithdr __P((FILE *));
static int emitloc __P((FILE *));
static int emitlocnames __P((FILE *));
static int emitpseudo __P((FILE *));
static int emitpv __P((FILE *));
static int emitroots __P((FILE *));
static int emitvec __P((FILE *));
static char *vecname __P((char *, const char *, int));

static const char *s_i386;

#define	SEP(pos, max)	(((u_int)(pos) % (max)) == 0 ? "\n\t" : " ")

/*
 * NEWLINE can only be used in the emitXXX functions.
 * In most cases it can be subsumed into an fprintf.
 */
#define	NEWLINE		if (putc('\n', fp) < 0) return (1)

int
mkioconf()
{
	register FILE *fp;
	register char *fname;
	int v;

	s_i386 = intern("i386");

	fname = path("ioconf.c");
	qsort(packed, npacked, sizeof *packed, cforder);
	if ((fp = fopen(fname, "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write %s: %s\n",
		    fname, strerror(errno));
		return (1);
	}
	v = emithdr(fp);
	if (v != 0 || emitvec(fp) || emitexterns(fp) || emitloc(fp) ||
	    emitlocnames(fp) || emitpv(fp) || emitcfdata(fp) ||
	    emitroots(fp) || emitpseudo(fp)) {
		if (v >= 0)
			(void)fprintf(stderr,
			    "config: error writing %s: %s\n",
			    fname, strerror(errno));
		(void)fclose(fp);
		/* (void)unlink(fname); */
		free(fname);
		return (1);
	}
	(void)fclose(fp);
	free(fname);
	return (0);
}

static int
cforder(a, b)
	const void *a, *b;
{
	register int n1, n2;

	n1 = (*(struct devi **)a)->i_cfindex;
	n2 = (*(struct devi **)b)->i_cfindex;
	return (n1 - n2);
}

static int
emithdr(ofp)
	register FILE *ofp;
{
	register FILE *ifp;
	register int n;
	char ifn[200], buf[BUFSIZ];

	if (fprintf(ofp, "\
/*\n\
 * MACHINE GENERATED: DO NOT EDIT\n\
 *\n\
 * ioconf.c, from \"%s\"\n\
 */\n\n", conffile) < 0)
		return (1);
	(void)sprintf(ifn, "ioconf.incl.%s", machine);
	if ((ifp = fopen(ifn, "r")) != NULL) {
		while ((n = fread(buf, 1, sizeof(buf), ifp)) > 0)
			if (fwrite(buf, 1, n, ofp) != n)
				return (1);
		if (ferror(ifp)) {
			(void)fprintf(stderr, "config: error reading %s: %s\n",
			    ifn, strerror(errno));
			(void)fclose(ifp);
			return (-1);
		}
		(void)fclose(ifp);
	} else {
		if (fputs("\
#include <sys/param.h>\n\
#include <sys/device.h>\n", ofp) < 0)
			return (1);
	}
	return (0);
}

static int
emitexterns(fp)
	register FILE *fp;
{
	register struct devbase *d;
	register struct deva *da;

	NEWLINE;
	for (d = allbases; d != NULL; d = d->d_next) {
		if (!devbase_has_instances(d, WILD))
			continue;
		if (fprintf(fp, "extern struct cfdriver %s_cd;\n",
			    d->d_name) < 0)
			return (1);
	}
	NEWLINE;
	for (da = alldevas; da != NULL; da = da->d_next) {
		if (!deva_has_instances(da, WILD))
			continue;
		if (fprintf(fp, "extern struct cfattach %s_ca;\n",
			    da->d_name) < 0)
			return (1);
	}
	NEWLINE;
	return (0);
}

static int
emitloc(fp)
	register FILE *fp;
{
	register int i;

	if (fprintf(fp, "\n/* locators */\n\
static int loc[%d] = {", locators.used) < 0)
		return (1);
	for (i = 0; i < locators.used; i++)
		if (fprintf(fp, "%s%s,", SEP(i, 8), locators.vec[i]) < 0)
			return (1);
	return (fprintf(fp, "\n};\n") < 0);
}

static int nlocnames, maxlocnames = 8;
static char **locnames;

short
addlocname(name)
	char *name;
{
	int i;

	if (locnames == NULL || nlocnames+1 > maxlocnames) {
		maxlocnames *= 4;
		locnames = (char **)realloc(locnames, maxlocnames * sizeof(char *));
	}
	for (i = 0; i < nlocnames; i++)
		if (strcmp(name, locnames[i]) == 0)
			return (i);
	/*printf("adding %s at %d\n", name, nlocnames);*/
	locnames[nlocnames++] = name;
	return (nlocnames - 1);
}

static int nlocnami, maxlocnami = 8;
static short *locnami;

void
addlocnami(index)
	short index;
{
	if (locnami == NULL || nlocnami+1 > maxlocnami) {
		maxlocnami *= 4;
		locnami = (short *)realloc(locnami, maxlocnami * sizeof(short));
	}
	locnami[nlocnami++] = index;
}


/*
 * Emit locator names
 * XXX the locnamp[] table is not compressed like it should be!
 */
static int
emitlocnames(fp)
	register FILE *fp;
{
	register struct devi **p, *i;
	register struct nvlist *nv;
	register struct attr *a;
	int added, start;
	int v, j, x;

#if 1
	addlocnami(-1);
	for (p = packed; (i = *p) != NULL; p++) {
		/*printf("child %s\n", i->i_name);*/

		/* initialize all uninitialized parents */
		for (x = 0; x < i->i_pvlen; x++) {
			if (i->i_parents[x]->i_plocnami)
				continue;
			start = nlocnami;

			/* add all the names */
			a = i->i_atattr;
			nv = a->a_locs;
			added = 0;
			for (nv = a->a_locs, v = 0; nv != NULL;
			    nv = nv->nv_next, v++) {
				addlocnami(addlocname(nv->nv_name));
				added = 1;
			}
			/* terminate list of names */
			if (added)
				addlocnami(-1);
			else
				start--;

			/*printf("bus %s starts at %d\n", i->i_parents[x]->i_name,
			    start);*/
			i->i_parents[x]->i_plocnami = start;

		}
	}
	for (p = packed; (i = *p) != NULL; p++)
		if (i->i_pvlen)
			i->i_locnami = i->i_parents[0]->i_plocnami;
#else
	addlocnami(-1);
	for (p = packed; (i = *p) != NULL; p++) {

		i->i_locnami = nlocnami;

		/* add all the names */
		a = i->i_atattr;
		nv = a->a_locs;
		for (nv = a->a_locs, v = 0; nv != NULL; nv = nv->nv_next, v++)
			addlocnami(addlocname(nv->nv_name));

		/* terminate list of names */
		addlocnami(-1);

	}
#endif
	if (fprintf(fp, "\nchar *locnames[] = {\n") < 0)
		return (1);
	for (j = 0; j < nlocnames; j++)
		if (fprintf(fp, "\t\"%s\",\n", locnames[j]) < 0)
			return (1);
	if (fprintf(fp, "};\n\n") < 0)
		return (1);

	if (fprintf(fp,
	    "/* each entry is an index into locnames[]; -1 terminates */\n") < 0)
		return (1);
	if (fprintf(fp, "short locnamp[] = {") < 0)
		return (1);
	for (j = 0; j < nlocnami; j++)
		if (fprintf(fp, "%s%d,", SEP(j, 8), locnami[j]) < 0)
			return (1);
	return (fprintf(fp, "\n};\n") < 0);
}


/*
 * Emit global parents-vector.
 */
static int
emitpv(fp)
	register FILE *fp;
{
	register int i;

	if (fprintf(fp, "\n/* size of parent vectors */\n\
int pv_size = %d;\n", parents.used) < 0)
		return (1);
	if (fprintf(fp, "\n/* parent vectors */\n\
short pv[%d] = {", parents.used) < 0)
		return (1);
	for (i = 0; i < parents.used; i++)
		if (fprintf(fp, "%s%d,", SEP(i, 16), parents.vec[i]) < 0)
			return (1);
	return (fprintf(fp, "\n};\n") < 0);
}

/*
 * Emit the cfdata array.
 */
static int
emitcfdata(fp)
	register FILE *fp;
{
	register struct devi **p, *i, **par;
	register int unit, v;
	register const char *vs, *state, *basename, *attachment;
	register struct nvlist *nv;
	register struct attr *a;
	char *loc;
	char locbuf[20];

	if (fprintf(fp, "\n\
#define NORM FSTATE_NOTFOUND\n\
#define STAR FSTATE_STAR\n\
#define DNRM FSTATE_DNOTFOUND\n\
#define DSTR FSTATE_DSTAR\n\
\n\
struct cfdata cfdata[] = {\n\
    /* attachment       driver        unit  state loc     flags parents nm ivstubs */\n") < 0)
		return (1);
	for (p = packed; (i = *p) != NULL; p++) {
		/* the description */
		if (fprintf(fp, "/*%3d: %s at ", i->i_cfindex, i->i_name) < 0)
			return (1);
		par = i->i_parents;
		for (v = 0; v < i->i_pvlen; v++)
			if (fprintf(fp, "%s%s", v == 0 ? "" : "|",
			    i->i_parents[v]->i_name) < 0)
				return (1);
		if (v == 0 && fputs("root", fp) < 0)
			return (1);
		a = i->i_atattr;
		nv = a->a_locs;
		for (nv = a->a_locs, v = 0; nv != NULL; nv = nv->nv_next, v++)
			if (fprintf(fp, " %s %s",
			    nv->nv_name, i->i_locs[v]) < 0)
				return (1);
		if (fputs(" */\n", fp) < 0)
			return (-1);

		/* then the actual defining line */
		basename = i->i_base->d_name;
		attachment = i->i_atdeva->d_name;
		if (i->i_unit == STAR) {
			unit = i->i_base->d_umax;
			if (i->i_disable) {
				state = "DSTR";
			} else {
				state = "STAR";
			}
		} else {
			unit = i->i_unit;
			if (i->i_disable) {
				state = "DNRM";
			} else {
				state = "NORM";
			}
		}
		if (i->i_ivoff < 0) {
			vs = "";
			v = 0;
		} else {
			vs = "vec+";
			v = i->i_ivoff;
		}
		if (i->i_locoff >= 0) {
			(void)sprintf(locbuf, "loc+%3d", i->i_locoff);
			loc = locbuf;
		} else
			loc = "loc";
		if (fprintf(fp, "\
    {&%s_ca,%s&%s_cd,%s%2d, %s, %7s, %#4x, pv+%2d, %d, %s%d},\n",
		    attachment, strlen(attachment) < 6 ? "\t\t" : "\t",
		    basename, strlen(basename) < 3 ? "\t\t" : "\t", unit,
		    state, loc, i->i_cfflags, i->i_pvoff, i->i_locnami,
		    vs, v) < 0)
			return (1);
	}
	if (fprintf(fp, "    {0},\n    {0},\n    {0},\n    {0},\n") < 0)
		return (1);
	if (fprintf(fp, "    {0},\n    {0},\n    {0},\n    {0},\n") < 0)
		return (1);
	return (fputs("    {(struct cfattach *)-1}\n};\n", fp) < 0);
}

/*
 * Emit the table of potential roots.
 */
static int
emitroots(fp)
	register FILE *fp;
{
	register struct devi **p, *i;
	int cnt = 0;

	if (fputs("\nshort cfroots[] = {\n", fp) < 0)
		return (1);
	for (p = packed; (i = *p) != NULL; p++) {
		if (i->i_at != NULL)
			continue;
		if (i->i_unit != 0 &&
		    (i->i_unit != STAR || i->i_base->d_umax != 0))
			(void)fprintf(stderr,
			    "config: warning: `%s at root' is not unit 0\n",
			    i->i_name);
		if (fprintf(fp, "\t%2d /* %s */,\n",
		    i->i_cfindex, i->i_name) < 0)
			return (1);
		cnt++;
	}
	if (fputs("\t-1\n};\n", fp) < 0)
		return (1);

	return(fprintf(fp, "\nint cfroots_size = %d;\n", cnt+1) < 0);
}

/*
 * Emit pseudo-device initialization.
 */
static int
emitpseudo(fp)
	register FILE *fp;
{
	register struct devi *i;
	register struct devbase *d;

	if (fputs("\n/* pseudo-devices */\n", fp) < 0)
		return (1);
	for (i = allpseudo; i != NULL; i = i->i_next)
		if (fprintf(fp, "extern void %sattach __P((int));\n",
		    i->i_base->d_name) < 0)
			return (1);
	if (fputs("\nstruct pdevinit pdevinit[] = {\n", fp) < 0)
		return (1);
	for (i = allpseudo; i != NULL; i = i->i_next) {
		d = i->i_base;
		if (fprintf(fp, "\t{ %sattach, %d },\n",
		    d->d_name, d->d_umax) < 0)
			return (1);
	}
	return (fputs("\t{ 0, 0 }\n};\n", fp) < 0);
}

/*
 * Emit interrupt vector declarations, and calculate offsets.
 */
static int
emitvec(fp)
	register FILE *fp;
{
	register struct nvlist *head, *nv;
	register struct devi **p, *i;
	register int j, nvec, unit;
	char buf[200];

	nvec = 0;
	for (p = packed; (i = *p) != NULL; p++) {
		if ((head = i->i_atdeva->d_vectors) == NULL)
			continue;
		if ((unit = i->i_unit) == STAR)
			panic("emitvec unit==STAR");
		if (nvec == 0)
			NEWLINE;
		for (j = 0, nv = head; nv != NULL; j++, nv = nv->nv_next)
			if (fprintf(fp,
			    "/* IVEC %s %d */ extern void %s();\n",
			    nv->nv_name, unit,
			    vecname(buf, nv->nv_name, unit)) < 0)
				return (1);
		nvec += j + 1;
	}
	if (nvec == 0)
		return (0);
	if (fprintf(fp, "\nstatic void (*vec[%d]) __P((void)) = {", nvec) < 0)
		return (1);
	nvec = 0;
	for (p = packed; (i = *p) != NULL; p++) {
		if ((head = i->i_atdeva->d_vectors) == NULL)
			continue;
		i->i_ivoff = nvec;
		unit = i->i_unit;
		for (nv = head; nv != NULL; nv = nv->nv_next)
			if (fprintf(fp, "%s%s,",
			    SEP(nvec++, 4),
			    vecname(buf, nv->nv_name, unit)) < 0)
				return (1);
		if (fprintf(fp, "%s0,", SEP(nvec++, 4)) < 0)
			return (1);
	}
	return (fputs("\n};\n", fp) < 0);
}

static char *
vecname(buf, name, unit)
	char *buf;
	const char *name;
	int unit;
{

	/* @#%* 386 uses a different name format */
	if (machine == s_i386) {
		(void)sprintf(buf, "V%s%d", name, unit);
		return (buf);
	}
	(void)sprintf(buf, "X%s%d", name, unit);
	return (buf);
}
