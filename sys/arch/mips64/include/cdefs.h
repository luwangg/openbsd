/*	$OpenBSD: src/sys/arch/mips64/include/cdefs.h,v 1.6 2013/03/28 17:30:45 martynas Exp $	*/

/*
 * Copyright (c) 2002-2003 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _MIPS64_CDEFS_H_
#define	_MIPS64_CDEFS_H_

#define __strong_alias(alias,sym)			\
	__asm__(".global " __STRING(alias) " ; "	\
	    __STRING(alias) " = " __STRING(sym))
#define __weak_alias(alias,sym)				\
	__asm__(".weak " __STRING(alias) " ; "		\
	    __STRING(alias) " = " __STRING(sym))
#define	__warn_references(sym,msg)			\
	__asm__(".section .gnu.warning." __STRING(sym)	\
	    " ; .ascii \"" msg "\" ; .text")
#define	__indr_references(sym,msg)	/* nothing */

#endif /* !_MIPS64_CDEFS_H_ */
