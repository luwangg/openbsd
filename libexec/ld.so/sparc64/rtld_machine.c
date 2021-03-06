/*	$OpenBSD: src/libexec/ld.so/sparc64/rtld_machine.c,v 1.52 2014/04/16 10:52:59 guenther Exp $ */

/*
 * Copyright (c) 1999 Dale Rahn
 * Copyright (c) 2001 Niklas Hallqvist
 * Copyright (c) 2001 Artur Grabowski
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
 */
/*-
 * Copyright (c) 2000 Eduardo Horvath.
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _DYN_LOADER

#include <sys/types.h>
#include <sys/mman.h>

#include <nlist.h>
#include <link.h>
#include <signal.h>

#include "syscall.h"
#include "archdep.h"
#include "resolve.h"

/*
 * The following table holds for each relocation type:
 *	- the width in bits of the memory location the relocation
 *	  applies to (not currently used)
 *	- the number of bits the relocation value must be shifted to the
 *	  right (i.e. discard least significant bits) to fit into
 *	  the appropriate field in the instruction word.
 *	- flags indicating whether
 *		* the relocation involves a symbol
 *		* the relocation is relative to the current position
 *		* the relocation is for a GOT entry
 *		* the relocation is relative to the load address
 *
 */
#define _RF_S		0x80000000		/* Resolve symbol */
#define _RF_A		0x40000000		/* Use addend */
#define _RF_P		0x20000000		/* Location relative */
#define _RF_G		0x10000000		/* GOT offset */
#define _RF_B		0x08000000		/* Load address relative */
#define _RF_U		0x04000000		/* Unaligned */
#define _RF_SZ(s)	(((s) & 0xff) << 8)	/* memory target size */
#define _RF_RS(s)	((s) & 0xff)		/* right shift */
static int reloc_target_flags[] = {
	0,							/* NONE */
	_RF_S|_RF_A|		_RF_SZ(8)  | _RF_RS(0),		/* RELOC_8 */
	_RF_S|_RF_A|		_RF_SZ(16) | _RF_RS(0),		/* RELOC_16 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* RELOC_32 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(8)  | _RF_RS(0),		/* DISP_8 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(16) | _RF_RS(0),		/* DISP_16 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* DISP_32 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP_30 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP_22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HI22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 13 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LO10 */
	_RF_G|			_RF_SZ(32) | _RF_RS(0),		/* GOT10 */
	_RF_G|			_RF_SZ(32) | _RF_RS(0),		/* GOT13 */
	_RF_G|			_RF_SZ(32) | _RF_RS(10),	/* GOT22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PC10 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PC22 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WPLT30 */
	_RF_S|			_RF_SZ(32) | _RF_RS(0),		/* COPY */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* GLOB_DAT */
	_RF_S|			_RF_SZ(32) | _RF_RS(0),		/* JMP_SLOT */
	      _RF_A|	_RF_B|	_RF_SZ(64) | _RF_RS(0),		/* RELATIVE */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(32) | _RF_RS(0),		/* UA_32 */

	      _RF_A|		_RF_SZ(32) | _RF_RS(0),		/* PLT32 */
	      _RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HIPLT22 */
	      _RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LOPLT10 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PCPLT32 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PCPLT22 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PCPLT10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 11 */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* 64 */
	_RF_S|_RF_A|/*extra*/	_RF_SZ(32) | _RF_RS(0),		/* OLO10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(42),	/* HH22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(32),	/* HM10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* LM22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(42),	/* PC_HH22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(32),	/* PC_HM10 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PC_LM22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP16 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP19 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* GLOB_JMP */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 7 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 5 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 6 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(64) | _RF_RS(0),		/* DISP64 */
	      _RF_A|		_RF_SZ(64) | _RF_RS(0),		/* PLT64 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HIX22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LOX10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(22),	/* H44 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(12),	/* M44 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* L44 */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* REGISTER */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(64) | _RF_RS(0),		/* UA64 */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(16) | _RF_RS(0),		/* UA16 */
};

#define RELOC_RESOLVE_SYMBOL(t)		((reloc_target_flags[t] & _RF_S) != 0)
#define RELOC_PC_RELATIVE(t)		((reloc_target_flags[t] & _RF_P) != 0)
#define RELOC_BASE_RELATIVE(t)		((reloc_target_flags[t] & _RF_B) != 0)
#define RELOC_UNALIGNED(t)		((reloc_target_flags[t] & _RF_U) != 0)
#define RELOC_USE_ADDEND(t)		((reloc_target_flags[t] & _RF_A) != 0)
#define RELOC_TARGET_SIZE(t)		((reloc_target_flags[t] >> 8) & 0xff)
#define RELOC_VALUE_RIGHTSHIFT(t)	(reloc_target_flags[t] & 0xff)

static long reloc_target_bitmask[] = {
#define _BM(x)	(~(-(1ULL << (x))))
	0,				/* NONE */
	_BM(8), _BM(16), _BM(32),	/* RELOC_8, _16, _32 */
	_BM(8), _BM(16), _BM(32),	/* DISP8, DISP16, DISP32 */
	_BM(30), _BM(22),		/* WDISP30, WDISP22 */
	_BM(22), _BM(22),		/* HI22, _22 */
	_BM(13), _BM(10),		/* RELOC_13, _LO10 */
	_BM(10), _BM(13), _BM(22),	/* GOT10, GOT13, GOT22 */
	_BM(10), _BM(22),		/* _PC10, _PC22 */
	_BM(30), 0,			/* _WPLT30, _COPY */
	-1, _BM(32), -1,		/* _GLOB_DAT, JMP_SLOT, _RELATIVE */
	_BM(32), _BM(32),		/* _UA32, PLT32 */
	_BM(22), _BM(10),		/* _HIPLT22, LOPLT10 */
	_BM(32), _BM(22), _BM(10),	/* _PCPLT32, _PCPLT22, _PCPLT10 */
	_BM(10), _BM(11), -1,		/* _10, _11, _64 */
	_BM(10), _BM(22),		/* _OLO10, _HH22 */
	_BM(10), _BM(22),		/* _HM10, _LM22 */
	_BM(22), _BM(10), _BM(22),	/* _PC_HH22, _PC_HM10, _PC_LM22 */
	_BM(16), _BM(19),		/* _WDISP16, _WDISP19 */
	-1,				/* GLOB_JMP */
	_BM(7), _BM(5), _BM(6)		/* _7, _5, _6 */
	-1, -1,				/* DISP64, PLT64 */
	_BM(22), _BM(13),		/* HIX22, LOX10 */
	_BM(22), _BM(10), _BM(13),	/* H44, M44, L44 */
	-1, -1, _BM(16),		/* REGISTER, UA64, UA16 */
#undef _BM
};
#define RELOC_VALUE_BITMASK(t)	(reloc_target_bitmask[t])

void _dl_reloc_plt(elf_object_t *object, Elf_Word *where, Elf_Addr value,
	Elf_RelA *rela);
void _dl_install_plt(Elf_Word *pltgot, Elf_Addr proc);

int
_dl_md_reloc(elf_object_t *object, int rel, int relasz)
{
	long	i;
	long	numrela;
	long	relrel;
	int	fails = 0;
	Elf_Addr loff;
	Elf_Addr prev_value = 0;
	const Elf_Sym *prev_sym = NULL;
	Elf_RelA *relas;
	struct load_list *llist;

	loff = object->obj_base;
	numrela = object->Dyn.info[relasz] / sizeof(Elf64_Rela);
	relrel = rel == DT_RELA ? object->relacount : 0;
	relas = (Elf64_Rela *)(object->Dyn.info[rel]);

	if (relas == NULL)
		return(0);

	if (relrel > numrela) {
		_dl_printf("relacount > numrel: %ld > %ld\n", relrel, numrela);
		_dl_exit(20);
	}

	/*
	 * unprotect some segments if we need it.
	 */
	if ((object->dyn.textrel == 1) && (rel == DT_REL || rel == DT_RELA)) {
		for (llist = object->load_list; llist != NULL; llist = llist->next) {
			if (!(llist->prot & PROT_WRITE))
				_dl_mprotect(llist->start, llist->size,
				    llist->prot|PROT_WRITE);
		}
	}

	/* tight loop for leading RELATIVE relocs */
	for (i = 0; i < relrel; i++, relas++) {
		Elf_Addr *where;

#ifdef DEBUG
		if (ELF_R_TYPE(relas->r_info) != R_TYPE(RELATIVE)) {
			_dl_printf("RELACOUNT wrong\n");
			_dl_exit(20);
		}
#endif
		where = (Elf_Addr *)(relas->r_offset + loff);
		*where = relas->r_addend + loff;
	}
	for (; i < numrela; i++, relas++) {
		Elf_Addr *where, value, ooff, mask;
		Elf_Word type;
		const Elf_Sym *sym, *this;
		const char *symn;

		type = ELF_R_TYPE(relas->r_info);

		if (type == R_TYPE(NONE))
			continue;

		if (type == R_TYPE(JMP_SLOT) && rel != DT_JMPREL)
			continue;

		where = (Elf_Addr *)(relas->r_offset + loff);

		if (RELOC_USE_ADDEND(type))
			value = relas->r_addend;
		else
			value = 0;

		sym = NULL;
		symn = NULL;
		if (RELOC_RESOLVE_SYMBOL(type)) {
			sym = object->dyn.symtab;
			sym += ELF_R_SYM(relas->r_info);
			symn = object->dyn.strtab + sym->st_name;

			if (sym->st_shndx != SHN_UNDEF &&
			    ELF_ST_BIND(sym->st_info) == STB_LOCAL) {
				value += loff;
			} else if (sym == prev_sym) {
				value += prev_value;
			} else {
				this = NULL;
				ooff = _dl_find_symbol_bysym(object,
				    ELF_R_SYM(relas->r_info), &this,
				    SYM_SEARCH_ALL|SYM_WARNNOTFOUND|
				    ((type == R_TYPE(JMP_SLOT)) ?
					SYM_PLT : SYM_NOTPLT),
				    sym, NULL);
				if (this == NULL) {
resolve_failed:
					if (ELF_ST_BIND(sym->st_info) !=
					    STB_WEAK)
						fails++;
					continue;
				}
				prev_sym = sym;
				prev_value = (Elf_Addr)(ooff + this->st_value);
				value += prev_value;
			}
		}

		if (type == R_TYPE(JMP_SLOT)) {
			_dl_reloc_plt(object, (Elf_Word *)where, value, relas);
			continue;
		}

		if (type == R_TYPE(COPY)) {
			void *dstaddr = where;
			const void *srcaddr;
			const Elf_Sym *dstsym = sym, *srcsym = NULL;
			size_t size = dstsym->st_size;
			Elf_Addr soff;

			soff = _dl_find_symbol(symn, &srcsym,
			    SYM_SEARCH_OTHER|SYM_WARNNOTFOUND|SYM_NOTPLT,
			    dstsym, object, NULL);
			if (srcsym == NULL)
				goto resolve_failed;

			srcaddr = (void *)(soff + srcsym->st_value);
			_dl_bcopy(srcaddr, dstaddr, size);
			continue;
		}

		if (RELOC_PC_RELATIVE(type))
			value -= (Elf_Addr)where;
		if (RELOC_BASE_RELATIVE(type))
			value += loff;

		mask = RELOC_VALUE_BITMASK(type);
		value >>= RELOC_VALUE_RIGHTSHIFT(type);
		value &= mask;

		if (RELOC_UNALIGNED(type)) {
			/* Handle unaligned relocations. */
			Elf_Addr tmp = 0;
			char *ptr = (char *)where;
			int i, size = RELOC_TARGET_SIZE(type)/8;

			/* Read it in one byte at a time. */
			for (i=0; i<size; i++)
				tmp = (tmp << 8) | ptr[i];

			tmp &= ~mask;
			tmp |= value;

			/* Write it back out. */
			for (i=0; i<size; i++)
				ptr[i] = ((tmp >> (8*i)) & 0xff);
		} else if (RELOC_TARGET_SIZE(type) > 32) {
			*where &= ~mask;
			*where |= value;
		} else {
			Elf32_Addr *where32 = (Elf32_Addr *)where;

			*where32 &= ~mask;
			*where32 |= value;
		}
	}

	/* reprotect the unprotected segments */
	if ((object->dyn.textrel == 1) && (rel == DT_REL || rel == DT_RELA)) {
		for (llist = object->load_list; llist != NULL; llist = llist->next) {
			if (!(llist->prot & PROT_WRITE))
				_dl_mprotect(llist->start, llist->size,
				    llist->prot);
		}
	}

	return (fails);
}

/*
 * Instruction templates:
 */
#define	BAA	0x10400000	/*	ba,a	%xcc, 0 */
#define	SETHI	0x03000000	/*	sethi	%hi(0), %g1 */
#define	JMP	0x81c06000	/*	jmpl	%g1+%lo(0), %g0 */
#define	NOP	0x01000000	/*	sethi	%hi(0), %g0 */
#define	OR	0x82106000	/*	or	%g1, 0, %g1 */
#define	ORG5	0x8a116000	/*	or	%g5, 0, %g5 */
#define	XOR	0x82186000	/*	xor	%g1, 0, %g1 */
#define	MOV71	0x8210000f	/*	or	%o7, 0, %g1 */
#define	MOV17	0x9e100001	/*	or	%g1, 0, %o7 */
#define	CALL	0x40000000	/*	call	0 */
#define	SLLX	0x83287000	/*	sllx	%g1, 0, %g1 */
#define	SLLXG5	0x8b297000	/*	sllx	%g5, 0, %g5 */
#define	SRAX	0x83387000	/*	srax	%g1, 0, %g1 */
#define	SETHIG5	0x0b000000	/*	sethi	%hi(0), %g5 */
#define	ORG15	0x82804005	/*	or	%g1, %g5, %g1 */


/* %hi(v) with variable shift */
#define	HIVAL(v, s)	(((v) >> (s)) &  0x003fffff)
#define LOVAL(v)	((v) & 0x000003ff)

void
_dl_reloc_plt(elf_object_t *object, Elf_Word *where, Elf_Addr value,
    Elf_RelA *rela)
{
	Elf_Addr offset;

	/*
	 * At the PLT entry pointed at by `where', we now construct
	 * a direct transfer to the now fully resolved function
	 * address.
	 *
	 * A PLT entry is supposed to start by looking like this:
	 *
	 *	sethi	%hi(. - .PLT0), %g1
	 *	ba,a	%xcc, .PLT1
	 *	nop
	 *	nop
	 *	nop
	 *	nop
	 *	nop
	 *	nop
	 *
	 * When we replace these entries we start from the second
	 * entry and do it in reverse order so the last thing we
	 * do is replace the branch.  That allows us to change this
	 * atomically.
	 *
	 * We now need to find out how far we need to jump.  We
	 * have a choice of several different relocation techniques
	 * which are increasingly expensive.
	 */

	offset = value - ((Elf_Addr)where);
	if (rela->r_addend) {
		Elf_Addr *ptr = (Elf_Addr *)where;
		/*
		 * This entry is >32768.  The relocation points to a
		 * PC-relative pointer to the _dl_bind_start_0 stub at
		 * the top of the PLT section.  Update it to point to
		 * the target function.
		 */
		ptr[0] += value - object->Dyn.info[DT_PLTGOT];

	} else if ((int64_t)(offset-4) <= (1L<<20) &&
	    (int64_t)(offset-4) >= -(1L<<20)) {
		/*
		 * We're within 1MB -- we can use a direct branch insn.
		 *
		 * We can generate this pattern:
		 *
		 *	sethi	%hi(. - .PLT0), %g1
		 *	ba,a	%xcc, addr
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *
		 */
		where[1] = BAA | (((offset-4) >> 2) &0x3fffff);
		__asm volatile("iflush %0+4" : : "r" (where));
	} else if (value < (1UL<<32)) {
		/*
		 * We're within 32-bits of address zero.
		 *
		 * The resulting code in the jump slot is:
		 *
		 *	sethi	%hi(. - .PLT0), %g1
		 *	sethi	%hi(addr), %g1
		 *	jmp	%g1+%lo(addr)
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *
		 */
		where[2] = JMP   | LOVAL(value);
		where[1] = SETHI | HIVAL(value, 10);
		__asm volatile("iflush %0+8" : : "r" (where));
		__asm volatile("iflush %0+4" : : "r" (where));

	} else if (value > -(1UL<<32)) {
		/*
		 * We're within 32-bits of address -1.
		 *
		 * The resulting code in the jump slot is:
		 *
		 *	sethi	%hi(. - .PLT0), %g1
		 *	sethi	%hix(~addr), %g1
		 *	xor	%g1, %lox(~addr), %g1
		 *	jmp	%g1
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *
		 */
		where[3] = JMP;
		where[2] = XOR | ((~value) & 0x00001fff);
		where[1] = SETHI | HIVAL(~value, 10);
		__asm volatile("iflush %0+12" : : "r" (where));
		__asm volatile("iflush %0+8" : : "r" (where));
		__asm volatile("iflush %0+4" : : "r" (where));

	} else if ((int64_t)(offset-8) <= (1L<<31) &&
	    (int64_t)(offset-8) >= -((1L<<31) - 4)) {
		/*
		 * We're within 32-bits -- we can use a direct call insn
		 *
		 * The resulting code in the jump slot is:
		 *
		 *	sethi	%hi(. - .PLT0), %g1
		 *	mov	%o7, %g1
		 *	call	(.+offset)
		 *	 mov	%g1, %o7
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *
		 */
		where[3] = MOV17;
		where[2] = CALL	  | (((offset-8) >> 2) & 0x3fffffff);
		__asm volatile("iflush %0+12" : : "r" (where));
		__asm volatile("iflush %0+8" : : "r" (where));
		where[1] = MOV71;
		__asm volatile("iflush %0+4" : : "r" (where));

	} else if (value < (1L<<42)) {
		/*
		 * Target 42bits or smaller. 
		 * We can generate this pattern:
		 *
		 * The resulting code in the jump slot is:
		 *
		 *	sethi	%hi(. - .PLT0), %g1
		 *	sethi	%hi(addr >> 20), %g1
		 *	or	%g1, %lo(addr >> 10), %g1
		 *	sllx	%g1, 10, %g1
		 *	jmp	%g1+%lo(addr)
		 *	nop
		 *	nop
		 *	nop
		 *
		 * this can handle addresses 0 - 0x3fffffffffc
		 */
		where[4] = JMP   | LOVAL(value);
		where[3] = SLLX  | 10;
		where[2] = OR    | LOVAL(value >> 10);
		where[1] = SETHI | HIVAL(value, 20);
		__asm volatile("iflush %0+16" : : "r" (where));
		__asm volatile("iflush %0+12" : : "r" (where));
		__asm volatile("iflush %0+8" : : "r" (where));
		__asm volatile("iflush %0+4" : : "r" (where));

	} else if (value > -(1UL<<41)) {
		/*
		 * Large target >= 0xfffffe0000000000UL
		 * We can generate this pattern:
		 *
		 * The resulting code in the jump slot is:
		 *
		 *	sethi	%hi(. - .PLT0), %g1
		 *	sethi	%hi(addr >> 20), %g1
		 *	or	%g1, %lo(addr >> 10), %g1
		 *	sllx	%g1, 32, %g1
		 *	srax	%g1, 22, %g1
		 *	jmp	%g1+%lo(addr)
		 *	nop
		 *	nop
		 *	nop
		 *
		 */
		where[5] = JMP   | LOVAL(value);
		where[4] = SRAX  | 22;
		where[3] = SLLX  | 32;
		where[2] = OR   | LOVAL(value >> 10);
		where[1] = SETHI | HIVAL(value, 20);

		__asm volatile("iflush %0+16" : : "r" (where));
		__asm volatile("iflush %0+12" : : "r" (where));
		__asm volatile("iflush %0+8" : : "r" (where));
		__asm volatile("iflush %0+4" : : "r" (where));

	} else {
		/*
		 * We need to load all 64-bits
		 *
		 * The resulting code in the jump slot is:
		 *
		 *	sethi	%hi(. - .PLT0), %g1
		 *	sethi	%hi(addr >> 42), %g5
		 *	sethi	%hi(addr >> 10), %g1
		 *	or	%g1, %lo(addr >> 32), %g5
		 *	sllx	%g5, 32, %g5
		 *	or	%g1, %g5, %g1
		 *	jmp	%g1+%lo(addr)
		 *	nop
		 *
		 */
		where[6] = JMP | LOVAL(value);
		where[5] = ORG15;
		where[4] = SLLXG5 | 32;
		where[3] = ORG5 | LOVAL(value >> 32);
		where[2] = SETHI | HIVAL(value, 10);
		where[1] = SETHIG5 | HIVAL(value, 42);
		__asm volatile("iflush %0+24" : : "r" (where));
		__asm volatile("iflush %0+20" : : "r" (where));
		__asm volatile("iflush %0+16" : : "r" (where));
		__asm volatile("iflush %0+12" : : "r" (where));
		__asm volatile("iflush %0+8" : : "r" (where));
		__asm volatile("iflush %0+4" : : "r" (where));
	}
}

/*
 * Resolve a symbol at run-time.
 */
Elf_Addr
_dl_bind(elf_object_t *object, int index)
{
	Elf_RelA *rela;
	Elf_Word *addr;
	Elf_Addr ooff;
	const Elf_Sym *sym, *this;
	const char *symn;
	const elf_object_t *sobj;
	sigset_t savedmask;

	rela = (Elf_RelA *)(object->Dyn.info[DT_JMPREL]);
	if (ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT)) {
		/*
		 * XXXX
		 *
		 * The first four PLT entries are reserved.  There
		 * is some disagreement whether they should have
		 * associated relocation entries.  Both the SPARC
		 * 32-bit and 64-bit ELF specifications say that
		 * they should have relocation entries, but the
		 * 32-bit SPARC binutils do not generate them,
		 * and now the 64-bit SPARC binutils have stopped
		 * generating them too.
		 *
		 * So, to provide binary compatibility, we will
		 * check the first entry, if it is reserved it
		 * should not be of the type JMP_SLOT.  If it
		 * is JMP_SLOT, then the 4 reserved entries were
		 * not generated and our index is 4 entries too far.
		 */
		index -= 4;
	}

	rela += index;

	sym = object->dyn.symtab;
	sym += ELF64_R_SYM(rela->r_info);
	symn = object->dyn.strtab + sym->st_name;

	addr = (Elf_Word *)(object->obj_base + rela->r_offset);
	this = NULL;
	ooff = _dl_find_symbol(symn, &this,
	    SYM_SEARCH_ALL|SYM_WARNNOTFOUND|SYM_PLT, sym, object, &sobj);
	if (this == NULL) {
		_dl_printf("lazy binding failed!\n");
		*(volatile int *)0 = 0;		/* XXX */
	}

	if (sobj->traced && _dl_trace_plt(sobj, symn))
		return ooff + this->st_value;

	/* if PLT is protected, allow the write */
	if (object->plt_size != 0)  {
		_dl_thread_bind_lock(0, &savedmask);
		_dl_mprotect((void*)object->plt_start, object->plt_size,
		    PROT_READ|PROT_WRITE|PROT_EXEC);
	}

	_dl_reloc_plt(object, addr, ooff + this->st_value, rela);

	/* if PLT is (to be protected), change back to RO/X */
	if (object->plt_size != 0) {
		_dl_mprotect((void*)object->plt_start, object->plt_size,
		    PROT_READ|PROT_EXEC);
		_dl_thread_bind_lock(1, &savedmask);
	}

	return ooff + this->st_value;
}

/*
 * Install rtld function call into this PLT slot.
 */
#define SAVE		0x9de3bf50
#define SETHI_l0	0x21000000
#define SETHI_l1	0x23000000
#define OR_l0_l0	0xa0142000
#define SLLX_l0_32_l0	0xa12c3020
#define OR_l0_l1_l0	0xa0140011
#define JMPL_l0_o1	0x93c42000
#define MOV_g1_o0	0x90100001

void
_dl_install_plt(Elf_Word *pltgot, Elf_Addr proc)
{
	pltgot[0] = SAVE;
	pltgot[1] = SETHI_l0  | HIVAL(proc, 42);
	pltgot[2] = SETHI_l1  | HIVAL(proc, 10);
	pltgot[3] = OR_l0_l0  | LOVAL((proc) >> 32);
	pltgot[4] = SLLX_l0_32_l0;
	pltgot[5] = OR_l0_l1_l0;
	pltgot[6] = JMPL_l0_o1 | LOVAL(proc);
	pltgot[7] = MOV_g1_o0;
}

void _dl_bind_start_0(long, long);
void _dl_bind_start_1(long, long);

/*
 *	Relocate the Global Offset Table (GOT).
 */
int
_dl_md_reloc_got(elf_object_t *object, int lazy)
{
	int	fails = 0;
	Elf_Addr *pltgot = (Elf_Addr *)object->Dyn.info[DT_PLTGOT];
	Elf_Word *entry = (Elf_Word *)pltgot;
	Elf_Addr ooff;
	Elf_Addr plt_addr;
	const Elf_Sym *this;

	if (object->Dyn.info[DT_PLTREL] != DT_RELA)
		return (0);

	object->got_addr = 0;
	object->got_size = 0;
	this = NULL;
	ooff = _dl_find_symbol("__got_start", &this,
	    SYM_SEARCH_OBJ|SYM_NOWARNNOTFOUND|SYM_PLT, NULL,
	    object, NULL);
	if (this != NULL)
		object->got_addr = ooff + this->st_value;

	this = NULL;
	ooff = _dl_find_symbol("__got_end", &this,
	    SYM_SEARCH_OBJ|SYM_NOWARNNOTFOUND|SYM_PLT, NULL,
	    object, NULL);
	if (this != NULL)
		object->got_size = ooff + this->st_value  - object->got_addr;

	plt_addr = 0;
	object->plt_size = 0;
	this = NULL;
	ooff = _dl_find_symbol("__plt_start", &this,
	    SYM_SEARCH_OBJ|SYM_NOWARNNOTFOUND|SYM_PLT, NULL,
	    object, NULL);
	if (this != NULL)
		plt_addr = ooff + this->st_value;

	this = NULL;
	ooff = _dl_find_symbol("__plt_end", &this,
	    SYM_SEARCH_OBJ|SYM_NOWARNNOTFOUND|SYM_PLT, NULL,
	    object, NULL);
	if (this != NULL)
		object->plt_size = ooff + this->st_value  - plt_addr;

	if (object->got_addr == 0)
		object->got_start = 0;
	else {
		object->got_start = ELF_TRUNC(object->got_addr, _dl_pagesz);
		object->got_size += object->got_addr - object->got_start;
		object->got_size = ELF_ROUND(object->got_size, _dl_pagesz);
	}
	if (plt_addr == 0)
		object->plt_start = 0;
	else {
		object->plt_start = ELF_TRUNC(plt_addr, _dl_pagesz);
		object->plt_size += plt_addr - object->plt_start;
		object->plt_size = ELF_ROUND(object->plt_size, _dl_pagesz);
	}

	if (object->traced)
		lazy = 1;

	if (!lazy) {
		fails = _dl_md_reloc(object, DT_JMPREL, DT_PLTRELSZ);
	} else {
		_dl_install_plt(&entry[0], (Elf_Addr)&_dl_bind_start_0);
		_dl_install_plt(&entry[8], (Elf_Addr)&_dl_bind_start_1);

		pltgot[8] = (Elf_Addr)object;
	}
	if (object->got_size != 0)
		_dl_mprotect((void*)object->got_start, object->got_size,
		    PROT_READ);
	if (object->plt_size != 0)
		_dl_mprotect((void*)object->plt_start, object->plt_size,
		    PROT_READ|PROT_EXEC);

	return (fails);
}
