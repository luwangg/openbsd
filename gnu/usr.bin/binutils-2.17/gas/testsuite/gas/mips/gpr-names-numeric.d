#objdump: -dr --prefix-addresses --show-raw-insn -M gpr-names=numeric
#name: MIPS GPR disassembly (numeric)
#source: gpr-names.s

# Check objdump's handling of -M gpr-names=foo options.

.*: +file format .*mips.*

Disassembly of section .text:
0+0000 <[^>]*> 3c000000 	lui	\$0,0x0
0+0004 <[^>]*> 3c010000 	lui	\$1,0x0
0+0008 <[^>]*> 3c020000 	lui	\$2,0x0
0+000c <[^>]*> 3c030000 	lui	\$3,0x0
0+0010 <[^>]*> 3c040000 	lui	\$4,0x0
0+0014 <[^>]*> 3c050000 	lui	\$5,0x0
0+0018 <[^>]*> 3c060000 	lui	\$6,0x0
0+001c <[^>]*> 3c070000 	lui	\$7,0x0
0+0020 <[^>]*> 3c080000 	lui	\$8,0x0
0+0024 <[^>]*> 3c090000 	lui	\$9,0x0
0+0028 <[^>]*> 3c0a0000 	lui	\$10,0x0
0+002c <[^>]*> 3c0b0000 	lui	\$11,0x0
0+0030 <[^>]*> 3c0c0000 	lui	\$12,0x0
0+0034 <[^>]*> 3c0d0000 	lui	\$13,0x0
0+0038 <[^>]*> 3c0e0000 	lui	\$14,0x0
0+003c <[^>]*> 3c0f0000 	lui	\$15,0x0
0+0040 <[^>]*> 3c100000 	lui	\$16,0x0
0+0044 <[^>]*> 3c110000 	lui	\$17,0x0
0+0048 <[^>]*> 3c120000 	lui	\$18,0x0
0+004c <[^>]*> 3c130000 	lui	\$19,0x0
0+0050 <[^>]*> 3c140000 	lui	\$20,0x0
0+0054 <[^>]*> 3c150000 	lui	\$21,0x0
0+0058 <[^>]*> 3c160000 	lui	\$22,0x0
0+005c <[^>]*> 3c170000 	lui	\$23,0x0
0+0060 <[^>]*> 3c180000 	lui	\$24,0x0
0+0064 <[^>]*> 3c190000 	lui	\$25,0x0
0+0068 <[^>]*> 3c1a0000 	lui	\$26,0x0
0+006c <[^>]*> 3c1b0000 	lui	\$27,0x0
0+0070 <[^>]*> 3c1c0000 	lui	\$28,0x0
0+0074 <[^>]*> 3c1d0000 	lui	\$29,0x0
0+0078 <[^>]*> 3c1e0000 	lui	\$30,0x0
0+007c <[^>]*> 3c1f0000 	lui	\$31,0x0
	\.\.\.
