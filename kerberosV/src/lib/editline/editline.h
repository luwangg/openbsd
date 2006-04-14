/*  Copyright 1992 Simmule Turner and Rich Salz.  All rights reserved. 
 *
 *  This software is not subject to any license of the American Telephone 
 *  and Telegraph Company or of the Regents of the University of California. 
 *
 *  Permission is granted to anyone to use this software for any purpose on
 *  any computer system, and to alter it and redistribute it freely, subject
 *  to the following restrictions:
 *  1. The authors are not responsible for the consequences of use of this
 *     software, no matter how awful, even if they arise from flaws in it.
 *  2. The origin of this software must not be misrepresented, either by
 *     explicit claim or by omission.  Since few users ever read sources,
 *     credits must appear in the documentation.
 *  3. Altered versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.  Since few users
 *     ever read sources, credits must appear in the documentation.
 *  4. This notice may not be removed or altered.
 */
/*  $KTH: editline.h,v 1.5 2005/04/24 18:56:14 lha Exp $ */

/* This a modifed version of editline */

#ifndef __HEIM_EDITLINE
#define __HEIM_EDITLINE 1

void	rl_initialize (void);
void	add_history (char*);
char*	readline (const char* prompt);
char*	rl_complete (char*, int*); /* warning diffrent api the gnu readline */
void	rl_reset_terminal (char*);

#endif

