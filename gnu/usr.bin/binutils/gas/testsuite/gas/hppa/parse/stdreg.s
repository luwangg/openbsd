	.SPACE $PRIVATE$
	.SUBSPA $DATA$,QUAD=1,ALIGN=8,ACCESS=31
	.SUBSPA $BSS$,QUAD=1,ALIGN=8,ACCESS=31,ZERO,SORT=82
	.SPACE $TEXT$
	.SUBSPA $LIT$,QUAD=0,ALIGN=8,ACCESS=44
	.SUBSPA $CODE$,QUAD=0,ALIGN=8,ACCESS=44,CODE_ONLY

	.SPACE $TEXT$
	.SUBSPA $CODE$

	.align 4
	.export foo
foo:
	.proc
	.callinfo no_calls
	.entry
	ldi 15,%sp
	ldi 15,%rp
	ldi 15,%dp
	ldi 15,%ret0
	ldi 15,%ret1
	ldi 15,%arg0
	ldi 15,%arg1
	ldi 15,%arg2
	ldi 15,%arg3
	.exit
 	.procend 	
