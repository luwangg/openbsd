/*	$OpenBSD: src/sys/arch/i386/stand/libsa/memprobe.c,v 1.2 1997/03/31 23:06:30 mickey Exp $	*/

#include <sys/param.h>
#include "libsa.h"
#include "biosdev.h"

static int addrprobe __P((int));

void
memprobe()
{
	int ram;

	cnvmem = biosmem(0);
	extmem = biosmem(1);

	/* probe extended memory
	 *
	 * There is no need to do this in assembly language.  This are
	 * much easier to debug in C anyways.
	 */
	for(ram = 1024; ram < 512*1024; ram += 4){

		printf("Probing memory: %d KB\r", ram-1024);
		if(addrprobe(ram)) break;
	}

	printf("\n");
	extmem = ram - 1024;
}

/* addrprobe(kloc): Probe memory at address kloc * 1024.
 *
 * This is a hack, but it seems to work ok.  Maybe this is
 * the *real* way that you are supposed to do probing???
 */
static int addrprobe(int kloc){
	volatile int *loc, i;
	static int pat[] = {
		0x00000000, 0xFFFFFFFF,
		0x01010101, 0x10101010,
		0x55555555, 0xCCCCCCCC
	};

	/* Get location */
	loc = (int *)(kloc * 1024);

	/* Probe address */
	for(i = 0; i < sizeof(pat)/sizeof(pat[0]); i++){
		*loc = pat[i];
		if(*loc != pat[i]) return(1);
	}

	return(0);
}


