# as: -march=armv6kt2
# objdump: -dr --prefix-addresses --show-raw-insn

.*: +file format .*arm.*

Disassembly of section .text:
0+000 <[^>]+> f517 0f80 	cmn.w	r7, #4194304	; 0x400000
0+004 <[^>]+> f5b8 0f80 	cmp.w	r8, #4194304	; 0x400000
0+008 <[^>]+> f5a4 0980 	sub.w	r9, r4, #4194304	; 0x400000
0+00c <[^>]+> f506 0380 	add.w	r3, r6, #4194304	; 0x400000
0+010 <[^>]+> f160 4500 	sbc.w	r5, r0, #2147483648	; 0x80000000
0+014 <[^>]+> f147 4400 	adc.w	r4, r7, #2147483648	; 0x80000000
0+018 <[^>]+> f022 4600 	bic.w	r6, r2, #2147483648	; 0x80000000
0+01c <[^>]+> f002 4800 	and.w	r8, r2, #2147483648	; 0x80000000
0+020 <[^>]+> f06f 4300 	mvn.w	r3, #2147483648	; 0x80000000
0+024 <[^>]+> f04f 4100 	mov.w	r1, #2147483648	; 0x80000000
