/*	$OpenBSD: src/sys/arch/hppa/dev/mem.c,v 1.7 2002/02/02 21:06:46 mickey Exp $	*/

/*
 * Copyright (c) 1998,1999 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF MIND,
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1991,1992,1994, The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Subject to your agreements with CMU,
 * permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: mem.c 1.9 94/12/16$
 */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>

#include <hppa/dev/cpudevs.h>
#include <hppa/dev/viper.h>

struct mem_softc {
	struct device sc_dev;

	volatile struct vi_trs *sc_vp;
};

int	memmatch __P((struct device *, void *, void *));
void	memattach __P((struct device *, struct device *, void *));

struct cfattach mem_ca = {
	sizeof(struct mem_softc), memmatch, memattach
};

struct cfdriver mem_cd = {
	NULL, "mem", DV_DULL
};

#define mmread  mmrw
#define mmwrite mmrw
cdev_decl(mm);

extern char *vmmap;
caddr_t zeropage;

/* A lock for the vmmap, 16-byte aligned as PA-RISC semaphores must be. */
static int32_t vmmap_lock __attribute__ ((aligned (16))) = 1;

int
memmatch(parent, cfdata, aux)   
	struct device *parent;
	void *cfdata;
	void *aux;
{
	register struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_MEMORY ||
	    ca->ca_type.iodc_sv_model != HPPA_MEMORY_PDEP)
		return 0;
	return 1;
}

void
memattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pdc_iodc_minit pdc_minit PDC_ALIGNMENT;
	register struct confargs *ca = aux;
	register struct mem_softc *sc = (struct mem_softc *)self;
	int err;

	printf (":");

	/* XXX check if we are dealing w/ Viper */
	if (ca->ca_hpa == (hppa_hpa_t)VIPER_HPA) {
		int s;

		sc->sc_vp = (struct vi_trs *)
			&((struct iomod *)ca->ca_hpa)->priv_trs;

		printf (" viper rev %x, ctrl %b",
			sc->sc_vp->vi_status.hw_rev,
			VI_CTRL, VIPER_BITS);

		s = splhigh();
		VI_CTRL |= VI_CTRL_ANYDEN;
		((struct vi_ctrl *)&VI_CTRL)->core_den = 0;
		((struct vi_ctrl *)&VI_CTRL)->sgc0_den = 0;
		((struct vi_ctrl *)&VI_CTRL)->sgc1_den = 0;
		((struct vi_ctrl *)&VI_CTRL)->core_prf = 1;
		sc->sc_vp->vi_control = VI_CTRL;
		splx(s);
#ifdef DEBUG
		printf (" >> %b", VI_CTRL, VIPER_BITS);
#endif
	} else
		sc->sc_vp = NULL;

	if ((err = pdc_call((iodcio_t)pdc, 0, PDC_IODC, PDC_IODC_NINIT,
			    &pdc_minit, ca->ca_hpa, PAGE0->imm_spa_size)) < 0)
		pdc_minit.max_spa = PAGE0->imm_max_mem;

	printf (" size %d", pdc_minit.max_spa / (1024*1024));
	if (pdc_minit.max_spa % (1024*1024))
		printf (".%d", pdc_minit.max_spa % (1024*1024));
	printf ("MB\n");
}

void
viper_setintrwnd(mask)
	u_int32_t mask;
{
	register struct mem_softc *sc;

	sc = mem_cd.cd_devs[0];

	if (sc->sc_vp)
		sc->sc_vp->vi_intrwd;
}

void
viper_eisa_en()
{
	register struct mem_softc *sc;

	sc = mem_cd.cd_devs[0];

	if (sc->sc_vp)
		((struct vi_ctrl *)&VI_CTRL)->eisa_den = 0;
}

int
mmopen(dev, flag, ioflag, p)
	dev_t dev;
	int flag;
	int ioflag;
	struct proc *p;
{
	return (0);
}

/*ARGSUSED*/
int
mmclose(dev, flag, mode, p)
	dev_t dev;  
	int flag, mode;
	struct proc *p;
{
	return (0);
}

int
mmrw(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	extern u_int totalphysmem;
	extern vaddr_t virtual_avail;
	struct iovec 	*iov;
	int32_t lockheld = 0;
	vaddr_t	v, o;
	vm_prot_t prot;
	int rw, error = 0;
	u_int 	c;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		case 0:				/*  /dev/mem  */

			/* If the address isn't in RAM, bail. */
			v = uio->uio_offset;
			if (btoc(v) > totalphysmem) {
				error = EFAULT;
				/* this will break us out of the loop */
				continue;
			}

			/*
			 * If the address is inside our large 
			 * directly-mapped kernel BTLB entry, 
			 * use kmem instead.
			 */
			if (v < virtual_avail)
				goto use_kmem;

			/*
			 * If we don't already hold the vmmap lock,
			 * acquire it.
			 */
			while (!lockheld) {
				__asm __volatile("ldcws 0(%1), %0\n\tsync"
				    : "=r" (lockheld) : "r" (&vmmap_lock));
				if (lockheld)
					break;
				error = tsleep((caddr_t)&vmmap_lock, 
				    PZERO | PCATCH,
				    "mmrw", 0);
				if (error)
					return (error);
			}

			/* Temporarily map the memory at vmmap. */
			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ :
			    VM_PROT_WRITE;
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(v), prot, prot|PMAP_WIRED);
			pmap_update(pmap_kernel());
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			error = uiomove((caddr_t)vmmap + o, c, uio);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
			    (vaddr_t)vmmap + PAGE_SIZE);
			pmap_update(pmap_kernel());
			break;

		case 1:				/*  /dev/kmem  */
			v = uio->uio_offset;
use_kmem:
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			rw = (uio->uio_rw == UIO_READ) ? B_READ : B_WRITE;
			if (!uvm_kernacc((caddr_t)v, c, rw)) {
				error = EFAULT;
				/* this will break us out of the loop */
				continue;
			}
			error = uiomove((caddr_t)v, c, uio);
			break;

		case 2:				/*  /dev/null  */
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

		case 12:			/*  /dev/zero  */
			/* Write to /dev/zero is ignored. */
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return (0);
			} 
			/* 
			 * On the first call, allocate and zero a page
			 * of memory for use with /dev/zero.
			 */
			if (zeropage == NULL) {
				zeropage = (caddr_t)
				    malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
				memset(zeropage, 0, PAGE_SIZE);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(zeropage, c, uio);
			break;

		default:
			return (ENXIO);
		}
	}

	/* If we hold the vmmap lock, release it. */
	if (lockheld) {
		__asm __volatile("sync\n\tstw	%1, 0(%0)"
		    :: "r" (&vmmap_lock), "r" (1));
		wakeup((caddr_t)&vmmap_lock);
	}

	return (error);
}

paddr_t
mmmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;  
{
	if (minor(dev) != 0)
		return (-1);

	/*
	 * Allow access only in RAM.
	 */
#if 0
	if (off < ctob(firstusablepage) ||
	    off >= ctob(lastusablepage + 1))
		return (-1);
#endif
	return (btop(off));
}

int
mmioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	return (EOPNOTSUPP);
}
