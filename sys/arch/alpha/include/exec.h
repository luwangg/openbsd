/*	$OpenBSD: src/sys/arch/alpha/include/exec.h,v 1.3 1996/07/29 22:58:42 niklas Exp $	*/
/*	$NetBSD: exec.h,v 1.1 1995/02/13 23:07:37 cgd Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _ALPHA_EXEC_H_
#define	_ALPHA_EXEC_H_

#define cpu_exec_aout_makecmds(p, epp)	ENOEXEC

/* Size of a page in an object file. */
#define	__LDPGSZ	8192

#define ELF_TARG_CLASS		ELFCLASS64
#define ELF_TARG_DATA		ELFDATA2LSB
#define ELF_TARG_MACH		EM_ALPHA

#define DO_AOUT			/* support a.out */
#define DO_ECOFF		/* support ECOFF */

#endif /* !_ALPHA_EXEC_H_ */
