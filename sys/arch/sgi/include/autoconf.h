/*	$OpenBSD: src/sys/arch/sgi/include/autoconf.h,v 1.29 2010/01/09 23:34:29 miod Exp $ */

/*
 * Copyright (c) 2001-2003 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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

/*
 * Definitions used by autoconfiguration.
 */

#ifndef _MACHINE_AUTOCONF_H_
#define _MACHINE_AUTOCONF_H_

#include <machine/bus.h>

/*
 * Structure holding all misc config information.
 */
struct cpu_info;

struct sys_rec {
	int	system_type;
	int	system_subtype;		/* IP35 only */

	/* Published cache operations. */
	void    (*_SyncCache)(struct cpu_info *);
	void    (*_InvalidateICache)(struct cpu_info *, vaddr_t, size_t);
	void    (*_SyncDCachePage)(struct cpu_info *, vaddr_t);
	void    (*_HitSyncDCache)(struct cpu_info *, vaddr_t, size_t);
	void    (*_IOSyncDCache)(struct cpu_info *, vaddr_t, size_t, int);
	void    (*_HitInvalidateDCache)(struct cpu_info *, vaddr_t, size_t);

	/* Serial console configuration. */
	struct mips_bus_space console_io;
};

extern struct sys_rec sys_config;

struct mainbus_attach_args {
	const char	*maa_name;
	int16_t		 maa_nasid;
};

#include <mips64/autoconf.h>

void	enaddr_aton(const char *, u_int8_t *);
u_long	bios_getenvint(const char *);

struct device;

void	ip27_setup(void);
void	ip27_autoconf(struct device *);
void	ip30_setup(void);
void	ip30_autoconf(struct device *);
void	ip32_setup(void);

extern char osloadpartition[256];
extern int16_t masternasid;
extern int16_t currentnasid;

#endif /* _MACHINE_AUTOCONF_H_ */
