/*	$OpenBSD: src/sbin/fsck_ext2fs/pass3.c,v 1.3 1997/06/14 04:16:56 downsj Exp $	*/
/*	$NetBSD: pass3.c,v 1.1 1997/06/11 11:21:55 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Manuel Bouyer.
 * Copyright (c) 1980, 1986, 1993
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
 */

#ifndef lint
#if 0
static char sccsid[] = "@(#)pass3.c	8.1 (Berkeley) 6/5/93";
#else
#if 0
static char rcsid[] = "$NetBSD: pass3.c,v 1.1 1997/06/11 11:21:55 bouyer Exp $";
#else
static char rcsid[] = "$OpenBSD: src/sbin/fsck_ext2fs/pass3.c,v 1.3 1997/06/14 04:16:56 downsj Exp $";
#endif
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ext2fs/ext2fs_dinode.h>
#include <ufs/ext2fs/ext2fs.h>
#include "fsck.h"
#include "extern.h"

void
pass3()
{
	register struct inoinfo **inpp, *inp;
	ino_t orphan;
	int loopcnt;

	for (inpp = &inpsort[inplast - 1]; inpp >= inpsort; inpp--) {
		inp = *inpp;
		if (inp->i_number == EXT2_ROOTINO ||
		    !(inp->i_parent == 0 || statemap[inp->i_number] == DSTATE))
			continue;
		if (statemap[inp->i_number] == DCLEAR)
			continue;
		for (loopcnt = 0; ; loopcnt++) {
			orphan = inp->i_number;
			if (inp->i_parent == 0 ||
			    statemap[inp->i_parent] != DSTATE ||
			    loopcnt > numdirs)
				break;
			inp = getinoinfo(inp->i_parent);
		}
		(void)linkup(orphan, inp->i_dotdot);
		inp->i_parent = inp->i_dotdot = lfdir;
		lncntp[lfdir]--;
		statemap[orphan] = DFOUND;
		propagate();
	}
}
