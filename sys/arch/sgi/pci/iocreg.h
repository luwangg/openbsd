/*	$OpenBSD: src/sys/arch/sgi/pci/iocreg.h,v 1.1 2008/04/07 22:53:00 miod Exp $	*/

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
 * Register definitions for SGI IOC ASIC.
 */

#define	IOC3_MCR		0x00030

#define	IOC3_RTC_BASE		0x20168

#define	IOC3_UARTA_BASE		0x20178
#define	IOC3_UARTB_BASE		0x20170

#define	IOC3_BYTEBUS_0		0x80000
#define	IOC3_BYTEBUS_1		0xa0000
#define	IOC3_BYTEBUS_2		0xc0000
#define	IOC3_BYTEBUS_3		0xe0000
