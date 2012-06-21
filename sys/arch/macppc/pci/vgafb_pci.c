/*	$OpenBSD: src/sys/arch/macppc/pci/Attic/vgafb_pci.c,v 1.25 2012/06/21 10:08:16 mpi Exp $	*/
/*	$NetBSD: vga_pci.c,v 1.4 1996/12/05 01:39:38 cgd Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/pte.h>

#include <dev/cons.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <arch/macppc/pci/vgafbvar.h>
#include <arch/macppc/pci/vgafb_pcivar.h>

#define PCI_VENDORID(x) ((x) & 0xFFFF)
#define PCI_CHIPID(x)   (((x) >> 16) & 0xFFFF)

struct vgafb_pci_softc {
	struct device sc_dev; 
 
	pcitag_t sc_pcitag;		/* PCI tag, in case we need it. */
	struct vgafb_config *sc_vc;	/* VGA configuration */ 
};

int vgafb_pci_probe(struct pci_attach_args *pa, int id, u_int32_t *ioaddr,
    u_int32_t *iosize, u_int32_t *memaddr, u_int32_t *memsize,
    u_int32_t *cacheable, u_int32_t *mmioaddr, u_int32_t *mmiosize);
int	vgafb_pci_match(struct device *, void *, void *);
void	vgafb_pci_attach(struct device *, struct device *, void *);

struct cfattach vgafb_pci_ca = {
	sizeof(struct vgafb_pci_softc), (cfmatch_t)vgafb_pci_match, vgafb_pci_attach,
};

pcitag_t vgafb_pci_console_tag;
struct vgafb_config vgafb_pci_console_vc;

#if 0
#define DEBUG_VGAFB
#endif

int
vgafb_pci_probe(struct pci_attach_args *pa, int id, u_int32_t *ioaddr,
    u_int32_t *iosize, u_int32_t *memaddr, u_int32_t *memsize,
    u_int32_t *cacheable, u_int32_t *mmioaddr, u_int32_t *mmiosize)
{
	bus_addr_t addr;
	bus_size_t size;
	int tcacheable;
	pci_chipset_tag_t pc = pa->pa_pc;
	int retval;
	int i;

	*iosize   = 0x0;
	*memsize  = 0x0;
	*mmiosize = 0x0;
	for (i = PCI_MAPREG_START; i <= PCI_MAPREG_PPB_END; i += 4) {
#ifdef DEBUG_VGAFB
		printf("vgafb confread %x %x\n",
			i, pci_conf_read(pc, pa->pa_tag, i));
#endif
		/* need to check more than just two base addresses? */
		if (PCI_MAPREG_TYPE(pci_conf_read(pc, pa->pa_tag, i)) ==
		    PCI_MAPREG_TYPE_IO) {
			retval = pci_io_find(pc, pa->pa_tag, i,
				&addr, &size);
			if (retval != 0) {
				continue;
			}
#ifdef DEBUG_VGAFB
	printf("vgafb_pci_probe: io %x addr %x size %x\n", i, addr, size);
#endif
			if (*iosize == 0) {
				*ioaddr = addr;
				*iosize = size;
			}

		} else {
			retval = pci_mem_find(pc, pa->pa_tag, i,
				&addr, &size, &tcacheable);
			if (retval != 0) {
				continue;
			}
#ifdef DEBUG_VGAFB
	printf("vgafb_pci_probe: mem %x addr %x size %x\n", i, addr, size);
#endif
			if (size == 0 || addr == 0) {
				/* ignore this entry */
			} else if (*memsize == 0) {
				/*
				 * first memory slot found goes into memory,
				 * this is for the case of no mmio
				 */
				*memaddr = addr;
				*memsize = size;
				*cacheable = tcacheable;
			} else {
				/*
				 * Oh, we have a second 'memory' 
				 * region, is this region the vga memory
				 * or mmio, we guess that memory is
				 * the larger of the two.
				 */ 
				 if (*memaddr >= size) {
					/* this is the mmio */
					*mmioaddr = addr;
					/* ATI driver maps 0x80000 mmio, grr */
					if (size < 0x80000) {
						size = 0x80000;
					}
					*mmiosize = size;
				 } else {
					/* this is the memory */
					*mmioaddr = *memaddr;
					*memaddr = addr;
					*mmiosize = *memsize;
					*memsize = size;
					*cacheable = tcacheable;
					/* ATI driver maps 0x80000 mmio, grr */
					if (*mmiosize < 0x80000) {
						*mmiosize = 0x80000;
					}
				 }
			}
		}
	}
#ifdef DEBUG_VGAFB
	printf("vgafb_pci_probe: id %x ioaddr %x, iosize %x, memaddr %x,\n memsize %x, mmioaddr %x, mmiosize %x\n",
		id, *ioaddr, *iosize, *memaddr, *memsize, *mmioaddr, *mmiosize);
#endif
	if (*iosize == 0) {
		if (id == 0) {
#ifdef powerpc
			/* this is only used if on openfirmware system and
			 * the device does not have a iobase config register,
			 * eg CirrusLogic 5434 VGA.  (they hardcode iobase to 0
			 * thus giving standard PC addresses for the registers) 
			 */
			int s;
			u_int32_t sizedata;

			/*
			 * Open Firmware (yuck) shuts down devices before
			 * entering a program so we need to bring them back
			 * 'online' to respond to bus accesses... so far
			 * this is true on the power.4e.
			 */
			s = splhigh();
			sizedata = pci_conf_read(pc, pa->pa_tag,
				PCI_COMMAND_STATUS_REG);
			sizedata |= (PCI_COMMAND_MASTER_ENABLE |
				     PCI_COMMAND_IO_ENABLE |
				     PCI_COMMAND_PARITY_ENABLE |
				     PCI_COMMAND_SERR_ENABLE);
			pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
				sizedata);
			splx(s);

#endif
			/* if this is the first card, allow it
			 * to be accessed in vga iospace
			 */
			*ioaddr = 0;
			*iosize = 0x10000; /* 64k, good as any */
		} else {
			/* iospace not available, assume 640x480, pray */
			*ioaddr = 0;
			*iosize = 0;
		}
	}
#ifdef DEBUG_VGAFB
	printf("vgafb_pci_probe: id %x ioaddr %x, iosize %x, memaddr %x,\n memsize %x, mmioaddr %x, mmiosize %x\n",
		id, *ioaddr, *iosize, *memaddr, *memsize, *mmioaddr, *mmiosize);
#endif
	
	/* io and mmio spaces are not required to attach */
	if (/* *iosize == 0 || */ *memsize == 0 /* || *mmiosize == 0 */)
		return (0);

	return (1);
}

int
vgafb_pci_match(struct device *parent, void *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (DEVICE_IS_VGA_PCI(pa->pa_class) == 0)
		return (0);

	/* If it's the console, we have a winner! */
	if (!bcmp(&pa->pa_tag, &vgafb_pci_console_tag, sizeof(pa->pa_tag))) {
		return (1);
	}

#ifdef DEBUG_VGAFB
	{
	int i;
		pci_chipset_tag_t pc = pa->pa_pc;
		for (i = 0x10; i < 0x24; i+=4) {
			printf("vgafb confread %x %x\n",
				i, pci_conf_read(pc, pa->pa_tag, i));
		}
	}
#endif

	return (0);
}

void
vgafb_pci_attach(struct device *parent, struct device  *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct vgafb_pci_softc *sc = (struct vgafb_pci_softc *)self;
	struct vgafb_config *vc;
	u_int32_t memaddr, memsize, cacheable;
	u_int32_t ioaddr, iosize;
	u_int32_t mmioaddr, mmiosize;
	int console;
	static int id = 0;
	int myid;

	myid = id;

	vgafb_pci_probe(pa, myid, &ioaddr, &iosize,
		&memaddr, &memsize, &cacheable, &mmioaddr, &mmiosize);


	console = (!bcmp(&pa->pa_tag, &vgafb_pci_console_tag, sizeof(pa->pa_tag)));
	if (console)
		vc = sc->sc_vc = &vgafb_pci_console_vc;
	else {
		vc = sc->sc_vc = (struct vgafb_config *)
		    malloc(sizeof(struct vgafb_config), M_DEVBUF, M_WAITOK);

		/* set up bus-independent VGA configuration */
		vgafb_init(pa->pa_iot, pa->pa_memt, vc,
		    memaddr, memsize, mmioaddr, mmiosize);
	}
	vc->vc_mmap = vgafb_mmap;
	vc->vc_ioctl = vgafb_ioctl;
	vc->membase = memaddr;
	vc->memsize = memsize;
	vc->mmiobase = mmioaddr;
	vc->mmiosize = mmiosize;

	sc->sc_pcitag = pa->pa_tag;

	if (iosize == 0)
		printf (", no io");

	if (mmiosize != 0)
		printf (", mmio");

	printf("\n");

	vgafb_wsdisplay_attach(self, vc, console);
	id++;
}
