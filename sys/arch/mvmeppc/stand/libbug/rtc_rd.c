/*	$OpenBSD: src/sys/arch/mvmeppc/stand/libbug/Attic/rtc_rd.c,v 1.3 2004/01/24 21:12:38 miod Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>

#include "libbug.h"

void
mvmeprom_rtc_rd(ptime)
	struct mvmeprom_time *ptime;
{
	asm volatile ("mr 3, %0": : "r" (ptime));
	MVMEPROM_CALL(MVMEPROM_RTC_RD);
}
