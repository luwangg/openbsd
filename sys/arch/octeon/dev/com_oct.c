/*	$OpenBSD: src/sys/arch/octeon/dev/Attic/com_oct.c,v 1.1 2010/09/20 06:32:30 syuu Exp $	*/

/*
 * Copyright (c) 2001-2004 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <octeon/dev/obiovar.h>
#include <octeon/dev/octeonreg.h>

int	com_oct_probe(struct device *, void *, void *);
void	com_oct_attach(struct device *, struct device *, void *);

struct cfattach com_oct_ca = {
	sizeof(struct com_softc), com_oct_probe, com_oct_attach
};

extern struct cfdriver com_cd;

int
com_oct_probe(struct device *parent, void *match, void *aux)
{
	struct cfdata *cf = match;
	struct obio_attach_args *oba = aux;
	bus_space_tag_t iot = oba->oba_memt;
	bus_space_handle_t ioh;
	int rv = 0, console;

	if (strcmp(oba->oba_name, com_cd.cd_name) != 0)
		return 0;

	console = 1;

	/* if it's in use as console, it's there. */
	if (!(console && !comconsattached)) {
		if (bus_space_map(iot, oba->oba_baseaddr, COM_NPORTS, 0, &ioh)) {
			printf(": can't map uart registers\n");
			return 1;
		}
		rv = comprobe1(iot, ioh);
	} else
		rv = 1;

	/* make a config stanza with exact locators match over a generic line */
	if (cf->cf_loc[0] != -1)
		rv += rv;

	return rv;
}

void
com_oct_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_softc *sc = (void *)self;
	struct obio_attach_args *oba = aux;
	int console;

	console = 1;

	sc->sc_iot = oba->oba_memt;
	sc->sc_iobase = oba->oba_baseaddr;
	sc->sc_hwflags = 0;
	sc->sc_swflags = 0;
	sc->sc_frequency = curcpu()->ci_hw.clock;
	sc->sc_uarttype = COM_UART_16550;

	/* if it's in use as console, it's there. */
	if (bus_space_map(sc->sc_iot, sc->sc_iobase, COM_NPORTS, 0, &sc->sc_ioh)) {
		printf(": can't map uart registers\n");
		return;
	}

	if (console && !comconsattached) {
		/*
		 * If we are the console, reuse the existing bus_space
		 * information, so that comcnattach() invokes bus_space_map()
		 * with correct parameters.
		 */

		if (comcnattach(sc->sc_iot, sc->sc_iobase, 115200,
		    sc->sc_frequency, (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
			panic("can't setup serial console");
	}

	com_attach_subr(sc);

	obio_intr_establish(oba->oba_intr, IPL_TTY, comintr,
	    (void *)sc, sc->sc_dev.dv_xname);
}
