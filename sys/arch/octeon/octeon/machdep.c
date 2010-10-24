/*	$OpenBSD: src/sys/arch/octeon/octeon/machdep.c,v 1.4 2010/10/24 15:40:03 miod Exp $ */

/*
 * Copyright (c) 2009, 2010 Miodrag Vallat.
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
 * Copyright (c) 2003-2004 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/msgbuf.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/exec_elf.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <ddb/db_interface.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/memconf.h>

#include <dev/cons.h>

#include <mips64/archtype.h>

#include <machine/octeon_pcmap_regs.h>

#include <octeon/dev/obiovar.h>
#include <octeon/dev/octeonreg.h>

/* The following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* Machine "architecture" */
char	cpu_model[30];
char	pmon_bootp[80];

/*
 * Declare these as initialized data so we can patch them.
 */
#ifndef	BUFCACHEPERCENT
#define	BUFCACHEPERCENT	5	/* Can be changed in config. */
#endif
#ifndef	BUFPAGES
#define BUFPAGES 0		/* Can be changed in config. */
#endif
int	bufpages = BUFPAGES;
int	bufcachepercent = BUFCACHEPERCENT;

/*
 * Even though the system is 64bit, the hardware is constrained to up
 * to 2G of contigous physical memory (direct 2GB DMA area), so there
 * is no particular constraint. paddr_t is long so: 
 */
struct uvm_constraint_range  dma_constraint = { 0x0, 0xffffffffUL };
struct uvm_constraint_range *uvm_md_constraints[] = { NULL };

vm_map_t exec_map;
vm_map_t phys_map;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int   safepri = 0;

caddr_t	msgbufbase;
vaddr_t	uncached_base;

int	physmem;		/* Max supported memory, changes to actual. */
int	ncpu = 1;		/* At least one CPU in the system. */
struct	user *proc0paddr;
int	kbd_reset;

struct cpu_hwinfo bootcpu_hwinfo;

/* Pointers to the start and end of the symbol table. */
caddr_t	ssym;
caddr_t	esym;
caddr_t	ekern;

struct phys_mem_desc mem_layout[MAXMEMSEGS];

void	dumpsys(void);
void	dumpconf(void);
extern	void parsepmonbp(void);
vaddr_t	mips_init(__register_t, __register_t, __register_t, __register_t);
boolean_t is_memory_range(paddr_t, psize_t, psize_t);
void	octeon_memory_init(void);

cons_decl(com_oct);
struct consdev octcons = cons_init(com_oct);

#define btoc(x) (((x)+PAGE_MASK)>>PAGE_SHIFT)

void
octeon_memory_init(void)
{
	uint64_t phys_avail[10 + 2];
	uint64_t startpfn, endpfn;
	uint32_t realmem;
	extern char end[];
	int i;
	startpfn = atop(CKSEG0_TO_PHYS((vaddr_t)&end) + PAGE_SIZE);
	endpfn = atop(96 << 20);
	mem_layout[0].mem_first_page = startpfn;
	mem_layout[0].mem_last_page = endpfn;
	mem_layout[0].mem_freelist = VM_FREELIST_DEFAULT;

	physmem = endpfn - startpfn;
	uint32_t realmem_bytes;

#if 0
	if (octeon_board_real()) {
		realmem_bytes = (octeon_dram - PAGE_SIZE);
		realmem_bytes &= ~(PAGE_SIZE - 1);
	} else 
#endif
	{
		/* Simulator we limit to 96 meg */
		realmem_bytes = (96 << 20);
	}
	/* phys_avail regions are in bytes */
	memset((void*)phys_avail,0,sizeof(phys_avail));
	phys_avail[0] = (CKSEG0_TO_PHYS((uint64_t)&end) + 
			 PAGE_SIZE ) & ~(PAGE_SIZE - 1);
#if 0
	if (octeon_board_real()) {
		if (realmem_bytes > OCTEON_DRAM_FIRST_256_END)
			phys_avail[1] = OCTEON_DRAM_FIRST_256_END;
		else
			phys_avail[1] = realmem_bytes;
		realmem_bytes -= OCTEON_DRAM_FIRST_256_END;
		realmem_bytes &= ~(PAGE_SIZE - 1);
		mem_layout[0].mem_last_page = atop(phys_avail[1]);
	} else
#endif
	{
		/* Simulator gets 96Meg period. */
		phys_avail[1] = (96 << 20);
	}
	/*-
	 * Octeon Memory looks as follows:
         *   PA
	 * First 256 MB DR0
	 * 0000 0000 0000 0000     to  0000 0000 0000 0000
	 * 0000 0000 0FFF FFFF     to  0000 0000 0FFF FFFF
	 * Second 256 MB DR1 
	 * 0000 0004 1000 0000     to  0000 0004 1000 0000
	 * 0000 0004 1FFF FFFF     to  0000 0004 1FFF FFFF
	 * Over 512MB Memory DR2  15.5GB
	 * 0000 0000 2000 0000     to  0000 0000 2000 0000
	 * 0000 0003 FFFF FFFF     to  0000 0003 FFFF FFFF
	 *
	 */
	physmem = btoc(phys_avail[1] - phys_avail[0]);
#if 0
	if (octeon_board_real()){
		if(realmem_bytes > OCTEON_DRAM_FIRST_256_END){
			/* take out the upper non-cached 1/2 */
			phys_avail[2] = 0x410000000ULL;
			phys_avail[3] = (0x410000000ULL 
					 + OCTEON_DRAM_FIRST_256_END);
			physmem += btoc(phys_avail[3] - phys_avail[2]);
			mem_layout[1].mem_first_page = atop(phys_avail[2]);
			mem_layout[1].mem_last_page = atop(phys_avail[3]-1);
			mem_layout[1].mem_freelist = VM_FREELIST_DEFAULT;
			realmem_bytes -= OCTEON_DRAM_FIRST_256_END;
			/* Now map the rest of the memory */
			phys_avail[4] = 0x20000000ULL;
			phys_avail[5] = (0x20000000ULL + realmem_bytes);
			physmem += btoc(phys_avail[5] - phys_avail[4]);
			mem_layout[2].mem_first_page = atop(phys_avail[4]);
			mem_layout[2].mem_last_page = atop(phys_avail[5]-1);
			mem_layout[2].mem_freelist = VM_FREELIST_DEFAULT;
			realmem_bytes=0;
		}else{
			/* Now map the rest of the memory */
			phys_avail[2] = 0x410000000ULL;
			phys_avail[3] = (0x410000000ULL + realmem_bytes);
			physmem += btoc(phys_avail[3] - phys_avail[2]);
			mem_layout[1].mem_first_page = atop(phys_avail[2]);
			mem_layout[1].mem_last_page = atop(phys_avail[3]-1);
			mem_layout[1].mem_freelist = VM_FREELIST_DEFAULT;
			realmem_bytes=0;
		}
 	}
#endif
 	realmem = physmem;
#if 0
	printf("Total DRAM Size 0x%016X\n", (uint32_t) octeon_dram);
#endif
	for(i=0;phys_avail[i];i+=2){
		printf("Bank %d = 0x%016lX   ->  0x%016lX\n",i>>1, 
		       (long)phys_avail[i], (long)phys_avail[i+1]);
	}
	for( i=0;mem_layout[i].mem_last_page;i++){
		printf("mem_layout[%d] page 0x%016lX -> 0x%016lX\n",i,
		       mem_layout[i].mem_first_page,
		       mem_layout[i].mem_last_page);
	}
}

/*
 * Do all the stuff that locore normally does before calling main().
 * Reset mapping and set up mapping to hardware and init "wired" reg.
 */
vaddr_t
mips_init(__register_t a0, __register_t a1, __register_t a2 __unused,
	__register_t a3)
{
	uint prid;
	vaddr_t xtlb_handler;
	int i;

	extern char start[], edata[], end[];
	extern char exception[], e_exception[];
	extern void xtlb_miss;

#ifdef MULTIPROCESSOR
	/*
	 * Set curcpu address on primary processor.
	 */
	setcurcpu(&cpu_info_primary);
#endif
	/*
	 * Clear the compiled BSS segment in OpenBSD code.
	 */

	bzero(edata, end - edata);

	/*
	 * Set up early console output.
	 */
	cn_tab = &octcons;

	/*
	 * Reserve space for the symbol table, if it exists.
	 */
	ssym = (char *)(vaddr_t)*(int32_t *)end;
	if (((long)ssym - (long)end) >= 0 &&
	    ((long)ssym - (long)end) <= 0x1000 &&
	    ssym[0] == ELFMAG0 && ssym[1] == ELFMAG1 &&
	    ssym[2] == ELFMAG2 && ssym[3] == ELFMAG3) {
		/* Pointers exist directly after kernel. */
		esym = (char *)(vaddr_t)*((int32_t *)end + 1);
		ekern = esym;
	} else {
		/* Pointers aren't setup either... */
		ssym = NULL;
		esym = NULL;
		ekern = end;
	}

	prid = cp0_get_prid();

	/* XXX: We should get clockrate from boot descriptor */
	bootcpu_hwinfo.clock = 500000000;

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;

	uncached_base = PHYS_TO_XKPHYS(0, CCA_NC);

	octeon_memory_init();

	/*
	 * Set pagesize to enable use of page macros and functions.
	 * Commit available memory to UVM system.
	 */

	uvmexp.pagesize = PAGE_SIZE;
	uvm_setpagesize();

	for (i = 0; i < MAXMEMSEGS && mem_layout[i].mem_last_page != 0; i++) {
		uint64_t fp, lp;
		uint64_t firstkernpage, lastkernpage;
		unsigned int freelist;
		paddr_t firstkernpa, lastkernpa;

		/* kernel is linked in CKSEG0 */
		firstkernpa = CKSEG0_TO_PHYS((vaddr_t)start);
		lastkernpa = CKSEG0_TO_PHYS((vaddr_t)ekern);

		firstkernpage = atop(trunc_page(firstkernpa));
		lastkernpage = atop(round_page(lastkernpa));

		fp = mem_layout[i].mem_first_page;
		lp = mem_layout[i].mem_last_page;
		freelist = mem_layout[i].mem_freelist;

		/* Account for kernel and kernel symbol table. */
		if (fp >= firstkernpage && lp < lastkernpage)
			continue;	/* In kernel. */

		if (lp < firstkernpage || fp > lastkernpage) {
			uvm_page_physload(fp, lp, fp, lp, freelist);
			continue;	/* Outside kernel. */
		}

		if (fp >= firstkernpage)
			fp = lastkernpage;
		else if (lp < lastkernpage)
			lp = firstkernpage;
		else { /* Need to split! */
			uint64_t xp = firstkernpage;
			uvm_page_physload(fp, xp, fp, xp, freelist);
			fp = lastkernpage;
		}
		if (lp > fp) {
			uvm_page_physload(fp, lp, fp, lp, freelist);
		}
	}

	bootcpu_hwinfo.c0prid = prid;
	bootcpu_hwinfo.type = (prid >> 8) & 0xff;
	/* FPU reports itself as type 5, version 0.1... */
	bootcpu_hwinfo.c1prid = bootcpu_hwinfo.c0prid;
	bootcpu_hwinfo.tlbsize = 64;
	bcopy(&bootcpu_hwinfo, &curcpu()->ci_hw, sizeof(struct cpu_hwinfo));

	/*
	 * Configure cache.
	 */

	Octeon_ConfigCache(curcpu());
	Octeon_SyncCache(curcpu());

	tlb_set_page_mask(TLB_PAGE_MASK);
	tlb_set_wired(0);
	tlb_flush(bootcpu_hwinfo.tlbsize);
	tlb_set_wired(UPAGES / 2);

	/*
	 * Get a console, very early but after initial mapping setup.
	 */

	consinit();
	printf("Initial setup done, switching console.\n");

	/*
	 * Init message buffer.
	 */
	msgbufbase = (caddr_t)pmap_steal_memory(MSGBUFSIZE, NULL,NULL);
	initmsgbuf(msgbufbase, MSGBUFSIZE);

	/*
	 * Allocate U page(s) for proc[0], pm_tlbpid 1.
	 */

	proc0.p_addr = proc0paddr = curcpu()->ci_curprocpaddr =
	    (struct user *)pmap_steal_memory(USPACE, NULL, NULL);
	proc0.p_md.md_regs = (struct trap_frame *)&proc0paddr->u_pcb.pcb_regs;
	tlb_set_pid(1);

	/*
	 * Bootstrap VM system.
	 */

	pmap_bootstrap();

	/*
	 * Copy down exception vector code.
	 */

	bcopy(exception, (char *)CACHE_ERR_EXC_VEC, e_exception - exception);
	bcopy(exception, (char *)GEN_EXC_VEC, e_exception - exception);

	/*
	 * Build proper TLB refill handler trampolines.
	 */

	xtlb_handler = (vaddr_t)&xtlb_miss;
	build_trampoline(TLB_MISS_EXC_VEC, xtlb_handler);
	build_trampoline(XTLB_MISS_EXC_VEC, xtlb_handler);

	/*
	 * Turn off bootstrap exception vectors.
	 * (this is done by PMON already, but it doesn't hurt to be safe)
	 */

	setsr(getsr() & ~SR_BOOT_EXC_VEC);
	proc0.p_md.md_regs->sr = getsr();

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Return the new kernel stack pointer.
	 */
	return ((vaddr_t)proc0paddr + USPACE - 64);
}

/*
 * Console initialization: called early on from main, before vm init or startup.
 * Do enough configuration to choose and initialize a console.
 */
void
consinit()
{
	static int console_ok = 0;

	if (console_ok == 0) {
		cninit();
		console_ok = 1;
	}
}

/*
 * cpu_startup: allocate memory for variable-sized tables, initialize CPU, and 
 * do auto-configuration.
 */
void
cpu_startup()
{
	vaddr_t minaddr, maxaddr;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("real mem = %u (%uMB)\n", ptoa((psize_t)physmem),
	    ptoa((psize_t)physmem)/1024/1024);

	/*
	 * Allocate a submap for exec arguments. This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	minaddr = vm_map_min(kernel_map);
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
	/* Allocate a submap for physio. */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	printf("avail mem = %u (%uMB)\n", ptoa(uvmexp.free),
	    ptoa(uvmexp.free)/1024/1024);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	if (boothowto & RB_CONFIG) {
#ifdef BOOT_CONFIG
		user_config();
#else
		printf("kernel does not support -c; continuing..\n");
#endif
	}
}

/*
 * Machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;		/* Overloaded */

	switch (name[0]) {
	case CPU_KBDRESET:
		if (securelevel > 0)
			return (sysctl_rdint(oldp, oldlenp, newp, kbd_reset));
		return (sysctl_int(oldp, oldlenp, newp, newlen, &kbd_reset));
	default:
		return EOPNOTSUPP;
	}
}

int	waittime = -1;

void
boot(int howto)
{

	/* Take a snapshot before clobbering any registers. */
	if (curproc)
		savectx(curproc->p_addr, 0);

	if (cold) {
		/*
		 * If the system is cold, just halt, unless the user
		 * explicitly asked for reboot.
		 */
		if ((howto & RB_USERREQ) == 0)
			howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		extern struct proc proc0;
		/* fill curproc with live object */
		if (curproc == NULL)
			curproc = &proc0;
		/*
		 * Synchronize the disks...
		 */
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr will be out of
		 * sync; adjust it now.
		 */
		if ((howto & RB_TIMEBAD) == 0) {
			resettodr();
		} else {
			printf("WARNING: not updating battery clock\n");
		}
	}

	uvm_shutdown();
	(void) splhigh();		/* Extreme priority. */

	if (howto & RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		if (howto & RB_POWERDOWN)
			printf("System Power Down not supported,"
			" halting system.\n");
		else
			printf("System Halt.\n");
	} else {
		void (*__reset)(void) = (void (*)(void))RESET_EXC_VEC;
		printf("System restart.\n");
		(void)disableintr();
		tlb_set_wired(0);
		tlb_flush(bootcpu_hwinfo.tlbsize);
		__reset();
	}

	for (;;) ;
	/*NOTREACHED*/
}

u_long	dumpmag = 0x8fca0101;	/* Magic number for savecore. */
int	dumpsize = 0;			/* Also for savecore. */
long	dumplo = 0;

void
dumpconf(void)
{
	int nblks;

	if (dumpdev == NODEV ||
	    (nblks = (bdevsw[major(dumpdev)].d_psize)(dumpdev)) == 0)
		return;
	if (nblks <= ctod(1))
		return;

	dumpsize = ptoa(physmem);
	if (dumpsize > atop(round_page(dbtob(nblks - dumplo))))
		dumpsize = atop(round_page(dbtob(nblks - dumplo)));
	else if (dumplo == 0)
		dumplo = nblks - btodb(ptoa(physmem));

	/*
	 * Don't dump on the first page in case the dump device includes a 
	 * disk label.
	 */
	if (dumplo < btodb(PAGE_SIZE))
		dumplo = btodb(PAGE_SIZE);
}

void
dumpsys()
{
	/* XXX TBD */
}

boolean_t
is_memory_range(paddr_t pa, psize_t len, psize_t limit)
{
	struct phys_mem_desc *seg;
	uint64_t fp, lp;
	int i;

	fp = atop(pa);
	lp = atop(round_page(pa + len));

	if (limit != 0 && lp > atop(limit))
		return FALSE;

	for (i = 0, seg = mem_layout; i < MAXMEMSEGS; i++, seg++)
		if (fp >= seg->mem_first_page && lp <= seg->mem_last_page)
			return TRUE;

	return FALSE;
}

#ifdef MULTIPROCESSOR
unsigned octeon_ap_boot = ~0;
struct cpu_info *cpu_info_boot_secondary = NULL;
void
hw_cpu_boot_secondary(struct cpu_info *ci)
{
	vaddr_t kstack;

	kstack = alloc_contiguous_pages(USPACE);
	if (kstack == NULL)
		panic("unable to allocate idle stack\n");
	ci->ci_curprocpaddr = (void *)kstack;

	cpu_info_boot_secondary = ci;
	
	octeon_ap_boot = ci->ci_cpuid;

	while (!cpuset_isset(&cpus_running, ci))
		;
}

void
hw_cpu_hatch(struct cpu_info *ci)
{
	int s;
	
	/*
	 * Set curcpu address on this processor.
	 */
	setcurcpu(ci);

	/*
	 * Make sure we can access the extended address space.
	 * Note that r10k and later do not allow XUSEG accesses
	 * from kernel mode unless SR_UX is set.
	 */
	setsr(getsr() | SR_KX | SR_UX);

	tlb_set_page_mask(TLB_PAGE_MASK);
	tlb_set_wired(0);
	tlb_flush(64);
	tlb_set_wired(UPAGES / 2);

	tlb_set_pid(0);

	/*
	 * Turn off bootstrap exception vectors.
	 */
	setsr(getsr() & ~SR_BOOT_EXC_VEC);

	/*
	 * Clear out the I and D caches.
	 */
	Octeon_ConfigCache(ci);
	Mips_SyncCache(ci);

	cpu_startclock(ci);

	ncpus++;
	cpuset_add(&cpus_running, ci);

	mips64_ipi_init();
	obio_setintrmask(0);

	spl0();
	(void)updateimask(0);

	SCHED_LOCK(s);
	cpu_switchto(NULL, sched_chooseproc());
}

int
hw_ipi_intr_establish(int (*func)(void *), u_long cpuid)
{
	obio_intr_establish(CIU_INT_MBOX(cpuid), IPL_IPI, func,
		(void *)cpuid, NULL);
	return 0;
};

void
hw_ipi_intr_set(u_long cpuid)
{
	*(uint64_t *)OCTEON_CIU_MBOX_SETX(cpuid) = 1;
}

void
hw_ipi_intr_clear(u_long cpuid)
{
	*(uint64_t *)OCTEON_CIU_MBOX_CLRX(cpuid) = 1;
}

#endif
