/*	$OpenBSD: src/usr.bin/awk/run.c,v 1.4 1997/01/21 23:47:55 kstailey Exp $	*/
/****************************************************************
Copyright (C) AT&T and Lucent Technologies 1996
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the names of AT&T or Lucent Technologies
or any of their entities not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

AT&T AND LUCENT DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL AT&T OR LUCENT OR ANY OF THEIR
ENTITIES BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
USE OR PERFORMANCE OF THIS SOFTWARE.
****************************************************************/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "awk.h"
#include "awkgram.h"

#define tempfree(x)	if (istemp(x)) tfree(x); else

/*
#undef tempfree

void tempfree(Cell *p) {
	if (p->ctype == OCELL && (p->csub < CUNK || p->csub > CFREE)) {
		ERROR "bad csub %d in Cell %d %s",
			p->csub, p->ctype, p->sval WARNING;
	}
	if (istemp(p))
		tfree(p);
}
*/

#ifdef _NFILE
#ifndef FOPEN_MAX
#define FOPEN_MAX _NFILE
#endif
#endif

#ifndef	FOPEN_MAX
#define	FOPEN_MAX	40	/* max number of open files */
#endif

#ifndef RAND_MAX
#define RAND_MAX	32767	/* all that ansi guarantees */
#endif

jmp_buf env;

#define PA2NUM	29	/* max number of pat,pat patterns allowed */
int	paircnt;		/* number of them in use */
int	pairstack[PA2NUM];	/* state of each pat,pat */

Node	*winner = NULL;	/* root of parse tree */
Cell	*tmps;		/* free temporary cells for execution */

static Cell	truecell	={ OBOOL, BTRUE, 0, 0, 1.0, NUM };
Cell	*true	= &truecell;
static Cell	falsecell	={ OBOOL, BFALSE, 0, 0, 0.0, NUM };
Cell	*false	= &falsecell;
static Cell	breakcell	={ OJUMP, JBREAK, 0, 0, 0.0, NUM };
Cell	*jbreak	= &breakcell;
static Cell	contcell	={ OJUMP, JCONT, 0, 0, 0.0, NUM };
Cell	*jcont	= &contcell;
static Cell	nextcell	={ OJUMP, JNEXT, 0, 0, 0.0, NUM };
Cell	*jnext	= &nextcell;
static Cell	nextfilecell	={ OJUMP, JNEXTFILE, 0, 0, 0.0, NUM };
Cell	*jnextfile	= &nextfilecell;
static Cell	exitcell	={ OJUMP, JEXIT, 0, 0, 0.0, NUM };
Cell	*jexit	= &exitcell;
static Cell	retcell		={ OJUMP, JRET, 0, 0, 0.0, NUM };
Cell	*jret	= &retcell;
static Cell	tempcell	={ OCELL, CTEMP, 0, 0, 0.0, NUM };

Node	*curnode = NULL;	/* the node being executed, for debugging */

void run(Node *a)	/* execution of parse tree starts here */
{
	execute(a);
	closeall();
}

Cell *execute(Node *u)	/* execute a node of the parse tree */
{
	Cell *(*proc)(Node **, int);
	Cell *x;
	Node *a;

	if (u == NULL)
		return(true);
	for (a = u; ; a = a->nnext) {
		curnode = a;
		if (isvalue(a)) {
			x = (Cell *) (a->narg[0]);
			if ((x->tval & FLD) && !donefld)
				fldbld();
			else if ((x->tval & REC) && !donerec)
				recbld();
			return(x);
		}
		if (notlegal(a->nobj))	/* probably a Cell* but too risky to print */
			ERROR "illegal statement" FATAL;
		proc = proctab[a->nobj-FIRSTTOKEN];
		x = (*proc)(a->narg, a->nobj);
		if ((x->tval & FLD) && !donefld)
			fldbld();
		else if ((x->tval & REC) && !donerec)
			recbld();
		if (isexpr(a))
			return(x);
		if (isjump(x))
			return(x);
		if (a->nnext == NULL)
			return(x);
		tempfree(x);
	}
}


Cell *program(Node **a, int n)	/* execute an awk program */
{				/* a[0] = BEGIN, a[1] = body, a[2] = END */
	Cell *x;

	if (setjmp(env) != 0)
		goto ex;
	if (a[0]) {		/* BEGIN */
		x = execute(a[0]);
		if (isexit(x))
			return(true);
		if (isjump(x))
			ERROR "illegal break, continue, next or nextfile from BEGIN" FATAL;
		tempfree(x);
	}
	if (a[1] || a[2])
		while (getrec(record) > 0) {
			x = execute(a[1]);
			if (isexit(x))
				break;
			tempfree(x);
		}
  ex:
	if (setjmp(env) != 0)	/* handles exit within END */
		goto ex1;
	if (a[2]) {		/* END */
		x = execute(a[2]);
		if (isbreak(x) || isnext(x) || iscont(x))
			ERROR "illegal break, continue, next or nextfile from END" FATAL;
		tempfree(x);
	}
  ex1:
	return(true);
}

struct Frame {	/* stack frame for awk function calls */
	int nargs;	/* number of arguments in this call */
	Cell *fcncell;	/* pointer to Cell for function */
	Cell **args;	/* pointer to array of arguments after execute */
	Cell *retval;	/* return value */
};

#define	NARGS	50	/* max args in a call */

struct Frame *frame = NULL;	/* base of stack frames; dynamically allocated */
int	nframe = 0;		/* number of frames allocated */
struct Frame *fp = NULL;	/* frame pointer. bottom level unused */

Cell *call(Node **a, int n)	/* function call.  very kludgy and fragile */
{
	static Cell newcopycell = { OCELL, CCOPY, 0, (char *) "", 0.0, NUM|STR|DONTFREE };
	int i, ncall, ndef;
	Node *x;
	Cell *args[NARGS], *oargs[NARGS], *y, *z, *fcn;
	char *s;

	fcn = execute(a[0]);	/* the function itself */
	s = fcn->nval;
	if (!isfunc(fcn))
		ERROR "calling undefined function %s", s FATAL;
	if (frame == NULL) {
		fp = frame = (struct Frame *) calloc(nframe += 100, sizeof(struct Frame));
		if (frame == NULL)
			ERROR "out of space for stack frames calling %s", s FATAL;
	}
	for (ncall = 0, x = a[1]; x != NULL; x = x->nnext)	/* args in call */
		ncall++;
	ndef = (int) fcn->fval;			/* args in defn */
	dprintf( ("calling %s, %d args (%d in defn), fp=%d\n", s, ncall, ndef, fp-frame) );
	if (ncall > ndef)
		ERROR "function %s called with %d args, uses only %d",
			s, ncall, ndef WARNING;
	if (ncall + ndef > NARGS)
		ERROR "function %s has %d arguments, limit %d", s, ncall+ndef, NARGS FATAL;
	for (i = 0, x = a[1]; x != NULL; i++, x = x->nnext) {	/* get call args */
		dprintf( ("evaluate args[%d], fp=%d:\n", i, fp-frame) );
		y = execute(x);
		oargs[i] = y;
		dprintf( ("args[%d]: %s %f <%s>, t=%o\n",
			   i, y->nval, y->fval, isarr(y) ? "(array)" : (char*) y->sval, y->tval) );
		if (isfunc(y))
			ERROR "can't use function %s as argument in %s", y->nval, s FATAL;
		if (isarr(y))
			args[i] = y;	/* arrays by ref */
		else
			args[i] = copycell(y);
		tempfree(y);
	}
	for ( ; i < ndef; i++) {	/* add null args for ones not provided */
		args[i] = gettemp();
		*args[i] = newcopycell;
	}
	fp++;	/* now ok to up frame */
	if (fp >= frame + nframe) {
		int dfp = fp - frame;	/* old index */
		frame = (struct Frame *)
			realloc((char *) frame, (nframe += 100) * sizeof(struct Frame));
		if (frame == NULL)
			ERROR "out of space for stack frames in %s", s FATAL;
		fp = frame + dfp;
	}
	fp->fcncell = fcn;
	fp->args = args;
	fp->nargs = ndef;	/* number defined with (excess are locals) */
	fp->retval = gettemp();

	dprintf( ("start exec of %s, fp=%d\n", s, fp-frame) );
	y = execute((Node *)(fcn->sval));	/* execute body */
	dprintf( ("finished exec of %s, fp=%d\n", s, fp-frame) );

	for (i = 0; i < ndef; i++) {
		Cell *t = fp->args[i];
		if (isarr(t)) {
			if (t->csub == CCOPY) {
				if (i >= ncall) {
					freesymtab(t);
					t->csub = CTEMP;
				} else {
					oargs[i]->tval = t->tval;
					oargs[i]->tval &= ~(STR|NUM|DONTFREE);
					oargs[i]->sval = t->sval;
					tempfree(t);
				}
			}
		} else if (t != y) {	/* kludge to prevent freeing twice */
			t->csub = CTEMP;
			tempfree(t);
		}
	}
	tempfree(fcn);
	if (isexit(y) || isnext(y) || isnextfile(y))
		return y;
	tempfree(y);		/* this can free twice! */
	z = fp->retval;			/* return value */
	dprintf( ("%s returns %g |%s| %o\n", s, getfval(z), getsval(z), z->tval) );
	fp--;
	return(z);
}

Cell *copycell(Cell *x)	/* make a copy of a cell in a temp */
{
	Cell *y;

	y = gettemp();
	y->csub = CCOPY;	/* prevents freeing until call is over */
	y->nval = x->nval;
	y->sval = x->sval ? tostring(x->sval) : NULL;
	y->fval = x->fval;
	y->tval = x->tval & ~(CON|FLD|REC|DONTFREE);	/* copy is not constant or field */
							/* is DONTFREE right? */
	return y;
}

Cell *arg(Node **a, int n)	/* nth argument of a function */
{

	n = (int) a[0];	/* argument number, counting from 0 */
	dprintf( ("arg(%d), fp->nargs=%d\n", n, fp->nargs) );
	if (n+1 > fp->nargs)
		ERROR "argument #%d of function %s was not supplied",
			n+1, fp->fcncell->nval FATAL;
	return fp->args[n];
}

Cell *jump(Node **a, int n)	/* break, continue, next, nextfile, return */
{
	Cell *y;

	switch (n) {
	case EXIT:
		if (a[0] != NULL) {
			y = execute(a[0]);
			errorflag = getfval(y);
			tempfree(y);
		}
		longjmp(env, 1);
	case RETURN:
		if (a[0] != NULL) {
			y = execute(a[0]);
			if ((y->tval & (STR|NUM)) == (STR|NUM)) {
				setsval(fp->retval, getsval(y));
				fp->retval->fval = getfval(y);
				fp->retval->tval |= NUM;
			}
			else if (y->tval & STR)
				setsval(fp->retval, getsval(y));
			else if (y->tval & NUM)
				setfval(fp->retval, getfval(y));
			else		/* can't happen */
				ERROR "bad type variable %d", y->tval FATAL;
			tempfree(y);
		}
		return(jret);
	case NEXT:
		return(jnext);
	case NEXTFILE:
		nextfile();
		return(jnextfile);
	case BREAK:
		return(jbreak);
	case CONTINUE:
		return(jcont);
	default:	/* can't happen */
		ERROR "illegal jump type %d", n FATAL;
	}
	return 0;	/* not reached */
}

Cell *getline(Node **a, int n)	/* get next line from specific input */
{		/* a[0] is variable, a[1] is operator, a[2] is filename */
	Cell *r, *x;
	char buf[RECSIZE];
	FILE *fp;

	fflush(stdout);	/* in case someone is waiting for a prompt */
	r = gettemp();
	if (a[1] != NULL) {		/* getline < file */
		x = execute(a[2]);		/* filename */
		if ((int) a[1] == '|')	/* input pipe */
			a[1] = (Node *) LE;	/* arbitrary flag */
		fp = openfile((int) a[1], getsval(x));
		tempfree(x);
		if (fp == NULL)
			n = -1;
		else
			n = readrec(buf, sizeof(buf), fp);
		if (n <= 0) {
			;
		} else if (a[0] != NULL) {	/* getline var <file */
			setsval(execute(a[0]), buf);
		} else {			/* getline <file */
			if (!(recloc->tval & DONTFREE))
				xfree(recloc->sval);
			strcpy(record, buf);
			recloc->sval = record;
			recloc->tval = REC | STR | DONTFREE;
			if (isnumber(recloc->sval)) {
				recloc->fval = atof(recloc->sval);
				recloc->tval |= NUM;
			}
			donerec = 1; donefld = 0;
		}
	} else {			/* bare getline; use current input */
		if (a[0] == NULL)	/* getline */
			n = getrec(record);
		else {			/* getline var */
			n = getrec(buf);
			setsval(execute(a[0]), buf);
		}
	}
	setfval(r, (Awkfloat) n);
	return r;
}

Cell *getnf(Node **a, int n)	/* get NF */
{
	if (donefld == 0)
		fldbld();
	return (Cell *) a[0];
}

Cell *array(Node **a, int n)	/* a[0] is symtab, a[1] is list of subscripts */
{
	Cell *x, *y, *z;
	char *s;
	Node *np;
	char buf[RECSIZE];

	x = execute(a[0]);	/* Cell* for symbol table */
	buf[0] = 0;
	for (np = a[1]; np; np = np->nnext) {
		y = execute(np);	/* subscript */
		s = getsval(y);
		strcat(buf, s);		/* BUG: unchecked! */
		if (np->nnext)
			strcat(buf, *SUBSEP);
		tempfree(y);
	}
	if (!isarr(x)) {
		dprintf( ("making %s into an array\n", x->nval) );
		if (freeable(x))
			xfree(x->sval);
		x->tval &= ~(STR|NUM|DONTFREE);
		x->tval |= ARR;
		x->sval = (char *) makesymtab(NSYMTAB);
	}
	z = setsymtab(buf, "", 0.0, STR|NUM, (Array *) x->sval);
	z->ctype = OCELL;
	z->csub = CVAR;
	tempfree(x);
	return(z);
}

Cell *adelete(Node **a, int n)	/* a[0] is symtab, a[1] is list of subscripts */
{
	Cell *x, *y;
	Node *np;
	char buf[RECSIZE], *s;

	x = execute(a[0]);	/* Cell* for symbol table */
	if (!isarr(x))
		return true;
	if (a[1] == 0) {	/* delete the elements, not the table */
		freesymtab(x);
		x->tval &= ~STR;
		x->tval |= ARR;
		x->sval = (char *) makesymtab(NSYMTAB);
	} else {
		buf[0] = 0;
		for (np = a[1]; np; np = np->nnext) {
			y = execute(np);	/* subscript */
			s = getsval(y);
			strcat(buf, s);
			if (np->nnext)
				strcat(buf, *SUBSEP);
			tempfree(y);
		}
		freeelem(x, buf);
	}
	tempfree(x);
	return true;
}

Cell *intest(Node **a, int n)	/* a[0] is index (list), a[1] is symtab */
{
	Cell *x, *ap, *k;
	Node *p;
	char buf[RECSIZE];
	char *s;

	ap = execute(a[1]);	/* array name */
	if (!isarr(ap)) {
		dprintf( ("making %s into an array\n", ap->nval) );
		if (freeable(ap))
			xfree(ap->sval);
		ap->tval &= ~(STR|NUM|DONTFREE);
		ap->tval |= ARR;
		ap->sval = (char *) makesymtab(NSYMTAB);
	}
	buf[0] = 0;
	for (p = a[0]; p; p = p->nnext) {
		x = execute(p);	/* expr */
		s = getsval(x);
		strcat(buf, s);
		tempfree(x);
		if (p->nnext)
			strcat(buf, *SUBSEP);
	}
	k = lookup(buf, (Array *) ap->sval);
	tempfree(ap);
	if (k == NULL)
		return(false);
	else
		return(true);
}


Cell *matchop(Node **a, int n)	/* ~ and match() */
{
	Cell *x, *y;
	char *s, *t;
	int i;
	fa *pfa;
	int (*mf)(fa *, char *) = match, mode = 0;

	if (n == MATCHFCN) {
		mf = pmatch;
		mode = 1;
	}
	x = execute(a[1]);	/* a[1] = target text */
	s = getsval(x);
	if (a[0] == 0)		/* a[1] == 0: already-compiled reg expr */
		i = (*mf)((fa *) a[2], s);
	else {
		y = execute(a[2]);	/* a[2] = regular expr */
		t = getsval(y);
		pfa = makedfa(t, mode);
		i = (*mf)(pfa, s);
		tempfree(y);
	}
	tempfree(x);
	if (n == MATCHFCN) {
		int start = patbeg - s + 1;
		if (patlen < 0)
			start = 0;
		setfval(rstartloc, (Awkfloat) start);
		setfval(rlengthloc, (Awkfloat) patlen);
		x = gettemp();
		x->tval = NUM;
		x->fval = start;
		return x;
	} else if ((n == MATCH && i == 1) || (n == NOTMATCH && i == 0))
		return(true);
	else
		return(false);
}


Cell *boolop(Node **a, int n)	/* a[0] || a[1], a[0] && a[1], !a[0] */
{
	Cell *x, *y;
	int i;

	x = execute(a[0]);
	i = istrue(x);
	tempfree(x);
	switch (n) {
	case BOR:
		if (i) return(true);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y);
		if (i) return(true);
		else return(false);
	case AND:
		if ( !i ) return(false);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y);
		if (i) return(true);
		else return(false);
	case NOT:
		if (i) return(false);
		else return(true);
	default:	/* can't happen */
		ERROR "unknown boolean operator %d", n FATAL;
	}
	return 0;	/*NOTREACHED*/
}

Cell *relop(Node **a, int n)	/* a[0 < a[1], etc. */
{
	int i;
	Cell *x, *y;
	Awkfloat j;

	x = execute(a[0]);
	y = execute(a[1]);
	if (x->tval&NUM && y->tval&NUM) {
		j = x->fval - y->fval;
		i = j<0? -1: (j>0? 1: 0);
	} else {
		i = strcmp(getsval(x), getsval(y));
	}
	tempfree(x);
	tempfree(y);
	switch (n) {
	case LT:	if (i<0) return(true);
			else return(false);
	case LE:	if (i<=0) return(true);
			else return(false);
	case NE:	if (i!=0) return(true);
			else return(false);
	case EQ:	if (i == 0) return(true);
			else return(false);
	case GE:	if (i>=0) return(true);
			else return(false);
	case GT:	if (i>0) return(true);
			else return(false);
	default:	/* can't happen */
		ERROR "unknown relational operator %d", n FATAL;
	}
	return 0;	/*NOTREACHED*/
}

void tfree(Cell *a)	/* free a tempcell */
{
	if (freeable(a))
		xfree(a->sval);
	if (a == tmps)
		ERROR "tempcell list is curdled" FATAL;
	a->cnext = tmps;
	tmps = a;
}

Cell *gettemp(void)	/* get a tempcell */
{	int i;
	Cell *x;

	if (!tmps) {
		tmps = (Cell *) calloc(100, sizeof(Cell));
		if (!tmps)
			ERROR "out of space for temporaries" FATAL;
		for(i = 1; i < 100; i++)
			tmps[i-1].cnext = &tmps[i];
		tmps[i-1].cnext = 0;
	}
	x = tmps;
	tmps = x->cnext;
	*x = tempcell;
	return(x);
}

Cell *indirect(Node **a, int n)	/* $( a[0] ) */
{
	Cell *x;
	int m;
	char *s;

	x = execute(a[0]);
	m = getfval(x);
	if (m == 0 && !isnumber(s = getsval(x)))	/* suspicion! */
		ERROR "illegal field $(%s), name \"%s\"", s, x->nval FATAL;
  /* can x->nval ever be null??? */
		/* ERROR "illegal field $(%s)", s FATAL; */
	tempfree(x);
	x = fieldadr(m);
	x->ctype = OCELL;
	x->csub = CFLD;
	return(x);
}

Cell *substr(Node **a, int nnn)		/* substr(a[0], a[1], a[2]) */
{
	int k, m, n;
	char *s;
	int temp;
	Cell *x, *y, *z = 0;

	x = execute(a[0]);
	y = execute(a[1]);
	if (a[2] != 0)
		z = execute(a[2]);
	s = getsval(x);
	k = strlen(s) + 1;
	if (k <= 1) {
		tempfree(x);
		tempfree(y);
		if (a[2] != 0)
			tempfree(z);
		x = gettemp();
		setsval(x, "");
		return(x);
	}
	m = getfval(y);
	if (m <= 0)
		m = 1;
	else if (m > k)
		m = k;
	tempfree(y);
	if (a[2] != 0) {
		n = getfval(z);
		tempfree(z);
	} else
		n = k - 1;
	if (n < 0)
		n = 0;
	else if (n > k - m)
		n = k - m;
	dprintf( ("substr: m=%d, n=%d, s=%s\n", m, n, s) );
	y = gettemp();
	temp = s[n+m-1];	/* with thanks to John Linderman */
	s[n+m-1] = '\0';
	setsval(y, s + m - 1);
	s[n+m-1] = temp;
	tempfree(x);
	return(y);
}

Cell *sindex(Node **a, int nnn)		/* index(a[0], a[1]) */
{
	Cell *x, *y, *z;
	char *s1, *s2, *p1, *p2, *q;
	Awkfloat v = 0.0;

	x = execute(a[0]);
	s1 = getsval(x);
	y = execute(a[1]);
	s2 = getsval(y);

	z = gettemp();
	for (p1 = s1; *p1 != '\0'; p1++) {
		for (q=p1, p2=s2; *p2 != '\0' && *q == *p2; q++, p2++)
			;
		if (*p2 == '\0') {
			v = (Awkfloat) (p1 - s1 + 1);	/* origin 1 */
			break;
		}
	}
	tempfree(x);
	tempfree(y);
	setfval(z, v);
	return(z);
}

/*
 * printf-like conversions
 *   returns len of buf or -1 on error
 */
int format(char *buf, int bufsize, char *s, Node *a)
{
	char fmt[RECSIZE];
	char *p, *t, *os;
	Cell *x;
	int flag = 0, len = 0, n;

	os = s;
	p = buf;
	while (*s) {
		if (p - buf >= bufsize)
			return -1;
		if (*s != '%') {
		        len++;
			*p++ = *s++;
			continue;
		}
		if (*(s+1) == '%') {
			len++;
			*p++ = '%';
			s += 2;
			continue;
		}
		for (t=fmt; (*t++ = *s) != '\0'; s++) {
			if (isalpha(*s) && *s != 'l' && *s != 'h' && *s != 'L')
				break;	/* the ansi panoply */
			if (*s == '*') {
				x = execute(a);
				a = a->nnext;
				sprintf((char *)t-1, "%d", (int) getfval(x));
				t = fmt + strlen(fmt);
				tempfree(x);
			}
		}
		*t = '\0';
		if (t >= fmt + sizeof(fmt))
			ERROR "format item %.30s... too long", os FATAL;
		switch (*s) {
		case 'f': case 'e': case 'g': case 'E': case 'G':
			flag = 1;
			break;
		case 'd': case 'i':
			flag = 2;
			if(*(s-1) == 'l') break;
			*(t-1) = 'l';
			*t = 'd';
			*++t = '\0';
			break;
		case 'o': case 'x': case 'X': case 'u':
			flag = *(s-1) == 'l' ? 2 : 3;
			break;
		case 's':
			flag = 4;
			break;
		case 'c':
			flag = 5;
			break;
		default:
			ERROR "weird printf conversion %s", fmt WARNING;
			flag = 0;
			break;
		}
		if (a == NULL)
			ERROR "not enough args in printf(%s)", os FATAL;
		x = execute(a);
		a = a->nnext;
		switch (flag) {
		case 0:	sprintf((char *)p, "%s", fmt);	/* unknown, so dump it too */
			p += len += strlen(p);
			sprintf((char *)p, "%s", getsval(x));
			break;
		case 1:	sprintf((char *)p, (char *)fmt, getfval(x)); break;
		case 2:	sprintf((char *)p, (char *)fmt, (long) getfval(x)); break;
		case 3:	sprintf((char *)p, (char *)fmt, (int) getfval(x)); break;
		case 4:
			t = getsval(x);
			n = strlen(t);
			if (n >= bufsize)
				ERROR "huge string (%d chars) in printf %.30s...",
					n, t FATAL;
			sprintf((char *)p, (char *)fmt, t);
			break;
		case 5:
			isnum(x) ?
			  (getfval(x) ?
			    sprintf((char *)p, (char *)fmt, (int) getfval(x))
                                      : len++)
				 : sprintf((char *)p, (char *)fmt, getsval(x)[0]);
			break;
		}
		tempfree(x);
		p += len += strlen(p);
		s++;
	}
	*p = '\0';
	for ( ; a; a = a->nnext)		/* evaluate any remaining args */
		execute(a);
	return (len);
}

Cell *awksprintf(Node **a, int n)		/* sprintf(a[0]) */
{
	Cell *x;
	Node *y;
	char buf[3*RECSIZE];

	y = a[0]->nnext;
	x = execute(a[0]);
	if (format(buf, sizeof buf, getsval(x), y) == -1)
		ERROR "sprintf string %.30s... too long", buf FATAL;
	tempfree(x);
	x = gettemp();
	x->sval = tostring(buf);
	x->tval = STR;
	return(x);
}

Cell *awkprintf(Node **a, int n)		/* printf */
{	/* a[0] is list of args, starting with format string */
	/* a[1] is redirection operator, a[2] is redirection file */
	FILE *fp;
	Cell *x;
	Node *y;
	char buf[3*RECSIZE];
	int len;

	y = a[0]->nnext;
	x = execute(a[0]);
	if ((len = format(buf, sizeof buf, getsval(x), y)) == -1)
		ERROR "printf string %.30s... too long", buf FATAL;
	tempfree(x);
	if (a[1] == NULL) {
		if (write(1, buf, len) != len)
			ERROR "write error on stdout" FATAL;
	} else {
		fp = redirect((int)a[1], a[2]);
		ferror(fp);	/* XXX paranoia */
		if (write(fileno(fp), buf, len) != len)
			ERROR "write error on %s", filename(fp) FATAL;
	}
	return(true);
}

Cell *arith(Node **a, int n)	/* a[0] + a[1], etc.  also -a[0] */
{
	Awkfloat i, j = 0;
	double v;
	Cell *x, *y, *z;

	x = execute(a[0]);
	i = getfval(x);
	tempfree(x);
	if (n != UMINUS) {
		y = execute(a[1]);
		j = getfval(y);
		tempfree(y);
	}
	z = gettemp();
	switch (n) {
	case ADD:
		i += j;
		break;
	case MINUS:
		i -= j;
		break;
	case MULT:
		i *= j;
		break;
	case DIVIDE:
		if (j == 0)
			ERROR "division by zero" FATAL;
		i /= j;
		break;
	case MOD:
		if (j == 0)
			ERROR "division by zero in mod" FATAL;
		modf(i/j, &v);
		i = i - j * v;
		break;
	case UMINUS:
		i = -i;
		break;
	case POWER:
		if (j >= 0 && modf(j, &v) == 0.0)	/* pos integer exponent */
			i = ipow(i, (int) j);
		else
			i = errcheck(pow(i, j), "pow");
		break;
	default:	/* can't happen */
		ERROR "illegal arithmetic operator %d", n FATAL;
	}
	setfval(z, i);
	return(z);
}

double ipow(double x, int n)	/* x**n.  ought to be done by pow, but isn't always */
{
	double v;

	if (n <= 0)
		return 1;
	v = ipow(x, n/2);
	if (n % 2 == 0)
		return v * v;
	else
		return x * v * v;
}

Cell *incrdecr(Node **a, int n)		/* a[0]++, etc. */
{
	Cell *x, *z;
	int k;
	Awkfloat xf;

	x = execute(a[0]);
	xf = getfval(x);
	k = (n == PREINCR || n == POSTINCR) ? 1 : -1;
	if (n == PREINCR || n == PREDECR) {
		setfval(x, xf + k);
		return(x);
	}
	z = gettemp();
	setfval(z, xf);
	setfval(x, xf + k);
	tempfree(x);
	return(z);
}

Cell *assign(Node **a, int n)	/* a[0] = a[1], a[0] += a[1], etc. */
{		/* this is subtle; don't muck with it. */
	Cell *x, *y;
	Awkfloat xf, yf;
	double v;

	y = execute(a[1]);
	x = execute(a[0]);
	if (n == ASSIGN) {	/* ordinary assignment */
		if (x == y && !(x->tval & (FLD|REC)))	/* self-assignment: */
			;		/* leave alone unless it's a field */
		else if ((y->tval & (STR|NUM)) == (STR|NUM)) {
			setsval(x, getsval(y));
			x->fval = getfval(y);
			x->tval |= NUM;
		}
		else if (y->tval & STR)
			setsval(x, getsval(y));
		else if (y->tval & NUM)
			setfval(x, getfval(y));
		else
			funnyvar(y, "read value of");
		tempfree(y);
		return(x);
	}
	xf = getfval(x);
	yf = getfval(y);
	switch (n) {
	case ADDEQ:
		xf += yf;
		break;
	case SUBEQ:
		xf -= yf;
		break;
	case MULTEQ:
		xf *= yf;
		break;
	case DIVEQ:
		if (yf == 0)
			ERROR "division by zero in /=" FATAL;
		xf /= yf;
		break;
	case MODEQ:
		if (yf == 0)
			ERROR "division by zero in %%=" FATAL;
		modf(xf/yf, &v);
		xf = xf - yf * v;
		break;
	case POWEQ:
		if (yf >= 0 && modf(yf, &v) == 0.0)	/* pos integer exponent */
			xf = ipow(xf, (int) yf);
		else
			xf = errcheck(pow(xf, yf), "pow");
		break;
	default:
		ERROR "illegal assignment operator %d", n FATAL;
		break;
	}
	tempfree(y);
	setfval(x, xf);
	return(x);
}

Cell *cat(Node **a, int q)	/* a[0] cat a[1] */
{
	Cell *x, *y, *z;
	int n1, n2;
	char *s;

	x = execute(a[0]);
	y = execute(a[1]);
	getsval(x);
	getsval(y);
	n1 = strlen(x->sval);
	n2 = strlen(y->sval);
	s = (char *) malloc(n1 + n2 + 1);
	if (s == NULL)
		ERROR "out of space concatenating %.15s... and %.15s...",
			x->sval, y->sval FATAL;
	strcpy(s, x->sval);
	strcpy(s+n1, y->sval);
	tempfree(y);
	z = gettemp();
	z->sval = s;
	z->tval = STR;
	tempfree(x);
	return(z);
}

Cell *pastat(Node **a, int n)	/* a[0] { a[1] } */
{
	Cell *x;

	if (a[0] == 0)
		x = execute(a[1]);
	else {
		x = execute(a[0]);
		if (istrue(x)) {
			tempfree(x);
			x = execute(a[1]);
		}
	}
	return x;
}

Cell *dopa2(Node **a, int n)	/* a[0], a[1] { a[2] } */
{
	Cell *x;
	int pair;

	pair = (int) a[3];
	if (pairstack[pair] == 0) {
		x = execute(a[0]);
		if (istrue(x))
			pairstack[pair] = 1;
		tempfree(x);
	}
	if (pairstack[pair] == 1) {
		x = execute(a[1]);
		if (istrue(x))
			pairstack[pair] = 0;
		tempfree(x);
		x = execute(a[2]);
		return(x);
	}
	return(false);
}

Cell *split(Node **a, int nnn)	/* split(a[0], a[1], a[2]); a[3] is type */
{
	Cell *x = 0, *y, *ap;
	char *s;
	int sep;
	char *t, temp, num[10], *fs = 0;
	int n, tempstat;

	y = execute(a[0]);	/* source string */
	s = getsval(y);
	if (a[2] == 0)		/* fs string */
		fs = *FS;
	else if ((int) a[3] == STRING) {	/* split(str,arr,"string") */
		x = execute(a[2]);
		fs = getsval(x);
	} else if ((int) a[3] == REGEXPR)
		fs = (char*) "(regexpr)";	/* split(str,arr,/regexpr/) */
	else
		ERROR "illegal type of split()" FATAL;
	sep = *fs;
	ap = execute(a[1]);	/* array name */
	freesymtab(ap);
	dprintf( ("split: s=|%s|, a=%s, sep=|%s|\n", s, ap->nval, fs) );
	ap->tval &= ~STR;
	ap->tval |= ARR;
	ap->sval = (char *) makesymtab(NSYMTAB);

	n = 0;
	if ((*s != '\0' && strlen(fs) > 1) || (int) a[3] == REGEXPR) {	/* reg expr */
		fa *pfa;
		if ((int) a[3] == REGEXPR) {	/* it's ready already */
			pfa = (fa *) a[2];
		} else {
			pfa = makedfa(fs, 1);
		}
		if (nematch(pfa,s)) {
			tempstat = pfa->initstat;
			pfa->initstat = 2;
			do {
				n++;
				sprintf((char *)num, "%d", n);
				temp = *patbeg;
				*patbeg = '\0';
				if (isnumber(s))
					setsymtab(num, s, atof((char *)s), STR|NUM, (Array *) ap->sval);
				else
					setsymtab(num, s, 0.0, STR, (Array *) ap->sval);
				*patbeg = temp;
				s = patbeg + patlen;
				if (*(patbeg+patlen-1) == 0 || *s == 0) {
					n++;
					sprintf((char *)num, "%d", n);
					setsymtab(num, "", 0.0, STR, (Array *) ap->sval);
					pfa->initstat = tempstat;
					goto spdone;
				}
			} while (nematch(pfa,s));
		}
		n++;
		sprintf((char *)num, "%d", n);
		if (isnumber(s))
			setsymtab(num, s, atof((char *)s), STR|NUM, (Array *) ap->sval);
		else
			setsymtab(num, s, 0.0, STR, (Array *) ap->sval);
  spdone:
		pfa = NULL;
	} else if (sep == ' ') {
		for (n = 0; ; ) {
			while (*s == ' ' || *s == '\t' || *s == '\n')
				s++;
			if (*s == 0)
				break;
			n++;
			t = s;
			do
				s++;
			while (*s!=' ' && *s!='\t' && *s!='\n' && *s!='\0');
			temp = *s;
			*s = '\0';
			sprintf((char *)num, "%d", n);
			if (isnumber(t))
				setsymtab(num, t, atof((char *)t), STR|NUM, (Array *) ap->sval);
			else
				setsymtab(num, t, 0.0, STR, (Array *) ap->sval);
			*s = temp;
			if (*s != 0)
				s++;
		}
	} else if (sep == 0) {	/* new: split(s, a, "") => 1 char/elem */
		for (n = 0; *s != 0; s++) {
			char buf[2];
			n++;
			sprintf((char *)num, "%d", n);
			buf[0] = *s;
			buf[1] = 0;
			if (isdigit(buf[0]))
				setsymtab(num, buf, atof(buf), STR|NUM, (Array *) ap->sval);
			else
				setsymtab(num, buf, 0.0, STR, (Array *) ap->sval);
		}
	} else if (*s != 0) {
		for (;;) {
			n++;
			t = s;
			while (*s != sep && *s != '\n' && *s != '\0')
				s++;
			temp = *s;
			*s = '\0';
			sprintf((char *)num, "%d", n);
			if (isnumber(t))
				setsymtab(num, t, atof((char *)t), STR|NUM, (Array *) ap->sval);
			else
				setsymtab(num, t, 0.0, STR, (Array *) ap->sval);
			*s = temp;
			if (*s++ == 0)
				break;
		}
	}
	tempfree(ap);
	tempfree(y);
	if (a[2] != 0 && (int) a[3] == STRING)
		tempfree(x);
	x = gettemp();
	x->tval = NUM;
	x->fval = n;
	return(x);
}

Cell *condexpr(Node **a, int n)	/* a[0] ? a[1] : a[2] */
{
	Cell *x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	} else {
		tempfree(x);
		x = execute(a[2]);
	}
	return(x);
}

Cell *ifstat(Node **a, int n)	/* if (a[0]) a[1]; else a[2] */
{
	Cell *x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	} else if (a[2] != 0) {
		tempfree(x);
		x = execute(a[2]);
	}
	return(x);
}

Cell *whilestat(Node **a, int n)	/* while (a[0]) a[1] */
{
	Cell *x;

	for (;;) {
		x = execute(a[0]);
		if (!istrue(x))
			return(x);
		tempfree(x);
		x = execute(a[1]);
		if (isbreak(x)) {
			x = true;
			return(x);
		}
		if (isnext(x) || isexit(x) || isret(x))
			return(x);
		tempfree(x);
	}
}

Cell *dostat(Node **a, int n)	/* do a[0]; while(a[1]) */
{
	Cell *x;

	for (;;) {
		x = execute(a[0]);
		if (isbreak(x))
			return true;
		if (isnext(x) || isnextfile(x) || isexit(x) || isret(x))
			return(x);
		tempfree(x);
		x = execute(a[1]);
		if (!istrue(x))
			return(x);
		tempfree(x);
	}
}

Cell *forstat(Node **a, int n)	/* for (a[0]; a[1]; a[2]) a[3] */
{
	Cell *x;

	x = execute(a[0]);
	tempfree(x);
	for (;;) {
		if (a[1]!=0) {
			x = execute(a[1]);
			if (!istrue(x)) return(x);
			else tempfree(x);
		}
		x = execute(a[3]);
		if (isbreak(x))		/* turn off break */
			return true;
		if (isnext(x) || isexit(x) || isret(x))
			return(x);
		tempfree(x);
		x = execute(a[2]);
		tempfree(x);
	}
}

Cell *instat(Node **a, int n)	/* for (a[0] in a[1]) a[2] */
{
	Cell *x, *vp, *arrayp, *cp, *ncp;
	Array *tp;
	int i;

	vp = execute(a[0]);
	arrayp = execute(a[1]);
	if (!isarr(arrayp)) {
		return true;
	}
	tp = (Array *) arrayp->sval;
	tempfree(arrayp);
	for (i = 0; i < tp->size; i++) {	/* this routine knows too much */
		for (cp = tp->tab[i]; cp != NULL; cp = ncp) {
			setsval(vp, cp->nval);
			ncp = cp->cnext;
			x = execute(a[2]);
			if (isbreak(x)) {
				tempfree(vp);
				return true;
			}
			if (isnext(x) || isexit(x) || isret(x)) {
				tempfree(vp);
				return(x);
			}
			tempfree(x);
		}
	}
	return true;
}

#if 0
	/* if someone ever wants to run over the arrays in sorted order, */
	/* here it is.  but it will likely run slower, not faster. */
	
	int qstrcmp(p, q)
		char **p, **q;
	{
		return strcmp(*p, *q);
	}
	
	Cell *instat(Node **a, int n)	/* for (a[0] in a[1]) a[2] */
	{
		Cell *x, *vp, *arrayp, *cp, *ncp, *ret;
		Array *tp;
		int i, ne;
	#define BIGENOUGH 1000
		char *elems[BIGENOUGH], **ep;
	
		vp = execute(a[0]);
		arrayp = execute(a[1]);
		if (!isarr(arrayp))
			ERROR "%s is not an array", arrayp->nval FATAL;
		tp = (Array *) arrayp->sval;
		tempfree(arrayp);
		ep = elems;
		ret = true;
		if (tp->nelem >= BIGENOUGH)
			ep = (char **) malloc(tp->nelem * sizeof(char *));
	
		for (i = ne = 0; i < tp->size; i++)
			for (cp = tp->tab[i]; cp != NULL; cp = cp->cnext)
				ep[ne++] = cp->nval;
		if (ne != tp->nelem)
			ERROR "can't happen: lost elems %d vs. %d", ne, tp->nelem FATAL;
		qsort(ep, ne, sizeof(char *), qstrcmp);
		for (i = 0; i < ne; i++) {
			setsval(vp, ep[i]);
			x = execute(a[2]);
			if (isbreak(x)) {
				tempfree(vp);
				break;
			}
			if (isnext(x) || isnextfile(x) || isexit(x) || isret(x)) {
				tempfree(vp);
				ret = x;
				break;
			}
			tempfree(x);
		}
		if (ep != elems)
			free(ep);
		return ret;
	}
#endif


Cell *bltin(Node **a, int n)	/* builtin functions. a[0] is type, a[1] is arg list */
{
	Cell *x, *y;
	Awkfloat u;
	int t;
	char *p, buf[RECSIZE];
	Node *nextarg;
	FILE *fp;

	t = (int) a[0];
	x = execute(a[1]);
	nextarg = a[1]->nnext;
	switch (t) {
	case FLENGTH:
		u = strlen(getsval(x)); break;
	case FLOG:
		u = errcheck(log(getfval(x)), "log"); break;
	case FINT:
		modf(getfval(x), &u); break;
	case FEXP:
		u = errcheck(exp(getfval(x)), "exp"); break;
	case FSQRT:
		u = errcheck(sqrt(getfval(x)), "sqrt"); break;
	case FSIN:
		u = sin(getfval(x)); break;
	case FCOS:
		u = cos(getfval(x)); break;
	case FATAN:
		if (nextarg == 0) {
			ERROR "atan2 requires two arguments; returning 1.0" WARNING;
			u = 1.0;
		} else {
			y = execute(a[1]->nnext);
			u = atan2(getfval(x), getfval(y));
			tempfree(y);
			nextarg = nextarg->nnext;
		}
		break;
	case FSYSTEM:
		fflush(stdout);		/* in case something is buffered already */
		u = (Awkfloat) system((char *)getsval(x)) / 256;   /* 256 is unix-dep */
		break;
	case FRAND:
		/* in principle, rand() returns something in 0..RAND_MAX */
		u = (Awkfloat) (rand() % RAND_MAX) / RAND_MAX;
		break;
	case FSRAND:
		if (x->tval & REC)	/* no argument provided */
			u = time((time_t *)0);
		else
			u = getfval(x);
		srand((int) u); u = (int) u;
		break;
	case FTOUPPER:
	case FTOLOWER:
		strcpy(buf, getsval(x));
		if (t == FTOUPPER) {
			for (p = buf; *p; p++)
				if (islower(*p))
					*p = toupper(*p);
		} else {
			for (p = buf; *p; p++)
				if (isupper(*p))
					*p = tolower(*p);
		}
		tempfree(x);
		x = gettemp();
		setsval(x, buf);
		return x;
	case FFLUSH:
		if ((fp = openfile(GT, getsval(x))) == NULL)
			u = EOF;
		else
			u = fflush(fp);
		break;
	default:	/* can't happen */
		ERROR "illegal function type %d", t FATAL;
		break;
	}
	tempfree(x);
	x = gettemp();
	setfval(x, u);
	if (nextarg != 0) {
		ERROR "warning: function has too many arguments" WARNING;
		for ( ; nextarg; nextarg = nextarg->nnext)
			execute(nextarg);
	}
	return(x);
}

Cell *printstat(Node **a, int n)	/* print a[0] */
{
	Node *x;
	Cell *y;
	FILE *fp;

	if (a[1] == 0)	/* a[1] is redirection operator, a[2] is file */
		fp = stdout;
	else
		fp = redirect((int)a[1], a[2]);
	for (x = a[0]; x != NULL; x = x->nnext) {
		y = execute(x);
		fputs((char *)getsval(y), fp);
		tempfree(y);
		if (x->nnext == NULL)
			fputs((char *)*ORS, fp);
		else
			fputs((char *)*OFS, fp);
	}
	if (a[1] != 0)
		fflush(fp);
	if (ferror(fp))
		ERROR "write error on %s", filename(fp) FATAL;
	return(true);
}

Cell *nullproc(Node **a, int n)
{
	n = 0;
	a = 0;
	return 0;
}


FILE *redirect(int a, Node *b)	/* set up all i/o redirections */
{
	FILE *fp;
	Cell *x;
	char *fname;

	x = execute(b);
	fname = getsval(x);
	fp = openfile(a, fname);
	if (fp == NULL)
		ERROR "can't open file %s", fname FATAL;
	tempfree(x);
	return fp;
}

struct files {
	FILE	*fp;
	char	*fname;
	int	mode;	/* '|', 'a', 'w' => LE/LT, GT */
} files[FOPEN_MAX] ={
	{ stdin,  "/dev/stdin",  LT },	/* watch out: don't free this! */
	{ stdout, "/dev/stdout", GT },
	{ stderr, "/dev/stderr", GT }
};

FILE *openfile(int a, char *us)
{
	char *s = us;
	int i, m;
	FILE *fp = 0;

	if (*s == '\0')
		ERROR "null file name in print or getline" FATAL;
	for (i=0; i < FOPEN_MAX; i++)
		if (files[i].fname && strcmp(s, files[i].fname) == 0)
			if (a == files[i].mode || (a==APPEND && files[i].mode==GT))
				return files[i].fp;
	for (i=0; i < FOPEN_MAX; i++)
		if (files[i].fp == 0)
			break;
	if (i >= FOPEN_MAX)
		ERROR "%s makes too many open files", s FATAL;
	fflush(stdout);	/* force a semblance of order */
	m = a;
	if (a == GT) {
		fp = fopen(s, "w");
	} else if (a == APPEND) {
		fp = fopen(s, "a");
		m = GT;	/* so can mix > and >> */
	} else if (a == '|') {	/* output pipe */
		fp = popen(s, "w");
	} else if (a == LE) {	/* input pipe */
		fp = popen(s, "r");
	} else if (a == LT) {	/* getline <file */
		fp = strcmp(s, "-") == 0 ? stdin : fopen(s, "r");	/* "-" is stdin */
	} else	/* can't happen */
		ERROR "illegal redirection %d", a FATAL;
	if (fp != NULL) {
		files[i].fname = tostring(s);
		files[i].fp = fp;
		files[i].mode = m;
	}
	return fp;
}

char *filename(FILE *fp)
{
	int i;

	for (i = 0; i < FOPEN_MAX; i++)
		if (fp == files[i].fp)
			return files[i].fname;
	return "???";
}

Cell *closefile(Node **a, int n)
{
	Cell *x;
	int i, stat;

	n = 0;
	x = execute(a[0]);
	getsval(x);
	for (i = 0; i < FOPEN_MAX; i++)
		if (files[i].fname && strcmp(x->sval, files[i].fname) == 0) {
			if (ferror(files[i].fp))
				ERROR "i/o error occurred on %s", files[i].fname WARNING;
			if (files[i].mode == '|' || files[i].mode == LE)
				stat = pclose(files[i].fp);
			else
				stat = fclose(files[i].fp);
			if (stat == EOF)
				ERROR "i/o error occurred closing %s", files[i].fname WARNING;
			if (i > 2)	/* don't do /dev/std... */
				xfree(files[i].fname);
			files[i].fname = NULL;	/* watch out for ref thru this */
			files[i].fp = NULL;
		}
	tempfree(x);
	return(true);
}

void closeall(void)
{
	int i, stat;

	for (i = 0; i < FOPEN_MAX; i++)
		if (files[i].fp) {
			if (ferror(files[i].fp))
				ERROR "i/o error occurred on %s", files[i].fname WARNING;
			if (files[i].mode == '|' || files[i].mode == LE)
				stat = pclose(files[i].fp);
			else
				stat = fclose(files[i].fp);
			if (stat == EOF)
				ERROR "i/o error occurred while closing %s", files[i].fname WARNING;
		}
}

#define	SUBSIZE	(20 * RECSIZE)

Cell *sub(Node **a, int nnn)	/* substitute command */
{
	char *sptr, *pb, *q;
	Cell *x, *y, *result;
	char buf[SUBSIZE], *t;
	fa *pfa;

	x = execute(a[3]);	/* target string */
	t = getsval(x);
	if (a[0] == 0)		/* 0 => a[1] is already-compiled regexpr */
		pfa = (fa *) a[1];	/* regular expression */
	else {
		y = execute(a[1]);
		pfa = makedfa(getsval(y), 1);
		tempfree(y);
	}
	y = execute(a[2]);	/* replacement string */
	result = false;
	if (pmatch(pfa, t)) {
		pb = buf;
		sptr = t;
		while (sptr < patbeg)
			*pb++ = *sptr++;
		sptr = getsval(y);
		while (*sptr != 0 && pb < buf + SUBSIZE - 1)
			if (*sptr == '\\' && *(sptr+1) == '&') {
				sptr++;		/* skip \, */
				*pb++ = *sptr++; /* add & */
			} else if (*sptr == '&') {
				sptr++;
				for (q = patbeg; q < patbeg+patlen; )
					*pb++ = *q++;
			} else
				*pb++ = *sptr++;
		*pb = '\0';
		if (pb >= buf + SUBSIZE)
			ERROR "sub() result %.30s too big", buf FATAL;
		sptr = patbeg + patlen;
		if ((patlen == 0 && *patbeg) || (patlen && *(sptr-1)))
			while ((*pb++ = *sptr++) != 0)
				;
		if (pb >= buf + SUBSIZE)
			ERROR "sub() result %.30s too big", buf FATAL;
		setsval(x, buf);
		result = true;;
	}
	tempfree(x);
	tempfree(y);
	return result;
}

Cell *gsub(Node **a, int nnn)	/* global substitute */
{
	Cell *x, *y;
	char *rptr, *sptr, *t, *pb;
	char buf[SUBSIZE];
	fa *pfa;
	int mflag, tempstat, num;

	mflag = 0;	/* if mflag == 0, can replace empty string */
	num = 0;
	x = execute(a[3]);	/* target string */
	t = getsval(x);
	if (a[0] == 0)		/* 0 => a[1] is already-compiled regexpr */
		pfa = (fa *) a[1];	/* regular expression */
	else {
		y = execute(a[1]);
		pfa = makedfa(getsval(y), 1);
		tempfree(y);
	}
	y = execute(a[2]);	/* replacement string */
	if (pmatch(pfa, t)) {
		tempstat = pfa->initstat;
		pfa->initstat = 2;
		pb = buf;
		rptr = getsval(y);
		do {
			/*
			char *p;
			int i;
			printf("target string: %s, *patbeg = %o, patlen = %d\n",
				t, *patbeg, patlen);
			printf("	match found: ");
			p=patbeg;
			for (i=0; i<patlen; i++)
				printf("%c", *p++);
			printf("\n");
			*/
			if (patlen == 0 && *patbeg != 0) {	/* matched empty string */
				if (mflag == 0) {	/* can replace empty */
					num++;
					sptr = rptr;
					while (*sptr != 0 && pb < buf + SUBSIZE-1)
						if (*sptr == '\\' && *(sptr+1) == '&') {
							sptr++;
							*pb++ = *sptr++;
						} else if (*sptr == '&') {
							char *q;
							sptr++;
							for (q = patbeg; q < patbeg+patlen; )
								*pb++ = *q++;
						} else
							*pb++ = *sptr++;
				}
				if (*t == 0)	/* at end */
					goto done;
				*pb++ = *t++;
				if (pb >= buf + SUBSIZE-1)
					ERROR "gsub() result %.30s too big", buf FATAL;
				mflag = 0;
			}
			else {	/* matched nonempty string */
				num++;
				/* if (patlen <= 0)
					ERROR "4: buf=%s, patlen %d, t=%s, patbeg=%s", buf, patlen, t, patbeg WARNING; */
				sptr = t;
				while (sptr < patbeg && pb < buf + SUBSIZE-1)
					*pb++ = *sptr++;
				sptr = rptr;
				while (*sptr != 0 && pb < buf + SUBSIZE-1)
					if (*sptr == '\\' && *(sptr+1) == '&') {
						sptr++;
						*pb++ = *sptr++;
					} else if (*sptr == '&') {
						char *q;
						sptr++;
						for (q = patbeg; q < patbeg+patlen; )
							*pb++ = *q++;
					} else
						*pb++ = *sptr++;
				t = patbeg + patlen;
				if (patlen == 0 || *t == 0 || *(t-1) == 0)
					goto done;
				if (pb >= buf + SUBSIZE-1)
					ERROR "gsub() result %.30s too big", buf FATAL;
				mflag = 1;
			}
		} while (pmatch(pfa,t));
		sptr = t;
		while ((*pb++ = *sptr++) != 0)
			;
	done:	if (pb >= buf + SUBSIZE-1)
			ERROR "gsub() result %.30s too big", buf FATAL;
		*pb = '\0';
		setsval(x, buf);
		pfa->initstat = tempstat;
	}
	tempfree(x);
	tempfree(y);
	x = gettemp();
	x->tval = NUM;
	x->fval = num;
	return(x);
}
