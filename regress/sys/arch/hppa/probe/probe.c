/*	$OpenBSD: src/regress/sys/arch/hppa/probe/probe.c,v 1.2 2014/04/18 14:38:21 guenther Exp $	*/

/*
 * Written by Michael Shalayeff, 2004. Public Domain.
 */

#include <sys/param.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>

char moo[] = "moo";	/* writable */
const char blah[] = "blah";	/* not */
volatile char *label;

#define	prober(r,a)	__asm volatile(	\
    "prober	(%2),%1,%0" : "=r" (r) : "r" (3), "r" (a));
#define	proberi(r,a)	__asm volatile(	\
    "proberi	(%2),%1,%0" : "=r" (r) : "i" (3), "r" (a));
#define	probew(r,a)	__asm volatile(	\
    "probew	(%2),%1,%0" : "=r" (r) : "r" (3), "r" (a));
#define	probewi(r,a)	__asm volatile(	\
    "probewi	(%2),%1,%0" : "=r" (r) : "i" (3), "r" (a));

void
sigsegv(int sig, siginfo_t *sip, void *scp)
{
	char buf[1024];

	snprintf(buf, sizeof buf, "%s not decoded\n", label);
	write(STDOUT_FILENO, buf, strlen(buf));
        _exit(1);
}

int
main(int argc, char *argv[])
{
	struct sigaction sa;
	int rv;

	sa.sa_sigaction = &sigsegv;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGSEGV, &sa, NULL);

#define	test_probe(n,a,r)	\
	label = #n;		\
	n(rv, a);		\
	if (rv != (r))		\
		errx(1, "%s(%p) returned %d", label, (a), rv);

	test_probe(prober, 1, 0);
	test_probe(prober, &blah, 1);

	test_probe(proberi, 1, 0);
	test_probe(proberi, &blah, 1);

	test_probe(probew, 1, 0);
	test_probe(probew, &blah, 0);
	test_probe(probew, &moo, 1);

	test_probe(probewi, 1, 0);
	test_probe(probewi, &blah, 0);
	test_probe(probewi, &moo, 1);

	exit(0);
}
