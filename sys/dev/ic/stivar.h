/*	$OpenBSD: src/sys/dev/ic/stivar.h,v 1.17 2005/02/27 22:10:58 miod Exp $	*/

/*
 * Copyright (c) 2000-2003 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IC_STIVAR_H_
#define _IC_STIVAR_H_

struct sti_screen {
	int		scr_devtype;

	bus_space_tag_t	iot, memt;
	bus_space_handle_t romh;
	bus_addr_t	base, fbaddr;
	bus_size_t	fblen;

	int		scr_bpp;

	struct sti_dd	scr_dd;		/* in word format */
	struct sti_font	scr_curfont;
	struct sti_cfg	scr_cfg;
	struct sti_ecfg	scr_ecfg;

	void		*scr_romfont;	/* ROM font copy, either in memory... */
	u_int		scr_fontmaxcol;	/* ...or in off-screen frame buffer */
	u_int		scr_fontbase;

	u_int8_t	scr_rcmap[STI_NCMAP],
			scr_gcmap[STI_NCMAP],
			scr_bcmap[STI_NCMAP];

	vaddr_t		scr_code;
	sti_init_t	init;
	sti_mgmt_t	mgmt;
	sti_unpmv_t	unpmv;
	sti_blkmv_t	blkmv;
	sti_test_t	test;
	sti_exhdl_t	exhdl;
	sti_inqconf_t	inqconf;
	sti_scment_t	scment;
	sti_dmac_t	dmac;
	sti_flowc_t	flowc;
	sti_utiming_t	utiming;
	sti_pmgr_t	pmgr;
	sti_util_t	util;

	u_int16_t	fbheight, fbwidth, oheight, owidth;
	u_int8_t	name[STI_DEVNAME_LEN];
};

struct sti_softc {
	struct device sc_dev;
	void *sc_ih;

	u_int	sc_flags;
#define	STI_TEXTMODE	0x0001
#define	STI_CLEARSCR	0x0002
#define	STI_CONSOLE	0x0004
#define	STI_ATTACHED	0x0008
	int	sc_nscreens;

	bus_space_tag_t iot, memt;
	bus_space_handle_t romh;
	bus_addr_t base;

	struct sti_screen *sc_scr;
	u_int	sc_wsmode;
};

void	sti_attach_common(struct sti_softc *sc, u_int codebase);
int	sti_cnattach(struct sti_screen *, bus_space_tag_t, bus_addr_t, u_int);
void	sti_describe(struct sti_softc *);
void	sti_end_attach(void *);

#endif /* _IC_STIVAR_H_ */
