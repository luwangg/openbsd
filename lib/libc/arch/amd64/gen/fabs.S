/*	$OpenBSD: src/lib/libc/arch/amd64/gen/fabs.S,v 1.6 2011/07/08 22:28:32 martynas Exp $	*/
/*	$NetBSD: fabs.S,v 1.1 2001/06/19 00:25:02 fvdl Exp $	*/

#include <machine/asm.h>

/*
 * Ok, this sucks. Is there really no way to push an xmm register onto
 * the FP stack directly?
 */

ENTRY(fabs)
	movsd	%xmm0, -8(%rsp)
	fldl	-8(%rsp)
	fabs
	fstpl	-8(%rsp)
	movsd	-8(%rsp),%xmm0
	ret
