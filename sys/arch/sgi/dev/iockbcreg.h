/*	$OpenBSD: src/sys/arch/sgi/dev/iockbcreg.h,v 1.1 2009/11/18 19:03:27 miod Exp $	*/

/*
 * Copyright (c) 2008 Joel Sing.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Register definitions for the PS/2 controller part of SGI IOC3 and IOC4 ASICS.
 */

/* bits in KBC_CTRL_STATUS */
#define	IOC3_KBC_STATUS_KBD_WRITE_PENDING	0x00000001
#define	IOC3_KBC_STATUS_AUX_WRITE_PENDING	0x00000002
#define	IOC3_KBC_STATUS_KBD_DATA		0x00000010
#define	IOC3_KBC_STATUS_KBD_CLOCK		0x00000020
#define	IOC3_KBC_CTRL_KBD_PULL_DATA_LOW		0x00000040
#define	IOC3_KBC_CTRL_KBD_PULL_CLOCK_LOW	0x00000080
#define	IOC3_KBC_STATUS_AUX_DATA		0x00000100
#define	IOC3_KBC_STATUS_AUX_CLOCK		0x00000200
#define	IOC3_KBC_CTRL_AUX_PULL_DATA_LOW		0x00000400
#define	IOC3_KBC_CTRL_AUX_PULL_CLOCK_LOW	0x00000800
#define	IOC3_KBC_CTRL_KBD_CLAMP_1		0x00100000
#define	IOC3_KBC_CTRL_AUX_CLAMP_1		0x00200000
#define	IOC3_KBC_CTRL_KBD_CLAMP_3		0x00400000
#define	IOC3_KBC_CTRL_AUX_CLAMP_3		0x00800000

/* bits in KBC_*_RX */
#define	IOC3_KBC_DATA_0_VALID		0x80000000
#define	IOC3_KBC_DATA_1_VALID		0x40000000
#define	IOC3_KBC_DATA_2_VALID		0x20000000
#define	IOC3_KBC_DATA_VALID		(IOC3_KBC_DATA_0_VALID | \
					 IOC3_KBC_DATA_1_VALID | \
					 IOC3_KBC_DATA_2_VALID)
#define	IOC3_KBC_DATA_0_MASK		0x00ff0000
#define	IOC3_KBC_DATA_0_SHIFT			16
#define	IOC3_KBC_DATA_1_MASK		0x0000ff00
#define	IOC3_KBC_DATA_1_SHIFT			8
#define	IOC3_KBC_DATA_2_MASK		0x000000ff
#define	IOC3_KBC_DATA_2_SHIFT			0
