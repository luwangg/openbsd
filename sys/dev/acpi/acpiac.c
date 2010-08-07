/* $OpenBSD: src/sys/dev/acpi/acpiac.c,v 1.28 2010/08/07 16:55:38 canacar Exp $ */
/*
 * Copyright (c) 2005 Marco Peereboom <marco@openbsd.org>
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

#include <sys/param.h>
#include <sys/event.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/apmvar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpidev.h>
#include <dev/acpi/amltypes.h>
#include <dev/acpi/dsdt.h>

#include <sys/sensors.h>

int  acpiac_match(struct device *, void *, void *);
void acpiac_attach(struct device *, struct device *, void *);
int  acpiac_notify(struct aml_node *, int, void *);

void acpiac_refresh(void *);
int acpiac_getsta(struct acpiac_softc *);

struct cfattach acpiac_ca = {
	sizeof(struct acpiac_softc), acpiac_match, acpiac_attach
};

struct cfdriver acpiac_cd = {
	NULL, "acpiac", DV_DULL
};

const char *acpiac_hids[] = { ACPI_DEV_AC, 0 };

int
acpiac_match(struct device *parent, void *match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	struct cfdata *cf = match;

	/* sanity */
	return (acpi_matchhids(aa, acpiac_hids, cf->cf_driver->cd_name));
}

void
acpiac_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpiac_softc *sc = (struct acpiac_softc *)self;
	struct acpi_attach_args *aa = aux;

	sc->sc_acpi = (struct acpi_softc *)parent;
	sc->sc_devnode = aa->aaa_node;

	acpiac_getsta(sc);
	printf(": AC unit ");
	if (sc->sc_ac_stat == PSR_ONLINE)
		printf("online\n");
	else if (sc->sc_ac_stat == PSR_OFFLINE)
		printf("offline\n");
	else
		printf("in unknown state\n");

	strlcpy(sc->sc_sensdev.xname, DEVNAME(sc),
	    sizeof(sc->sc_sensdev.xname));
	strlcpy(sc->sc_sens[0].desc, "power supply",
	    sizeof(sc->sc_sens[0].desc));
	sc->sc_sens[0].type = SENSOR_INDICATOR;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[0]);
	sensordev_install(&sc->sc_sensdev);
	sc->sc_sens[0].value = sc->sc_ac_stat;

	aml_register_notify(sc->sc_devnode, aa->aaa_dev,
	    acpiac_notify, sc, ACPIDEV_NOPOLL);
}

void
acpiac_refresh(void *arg)
{
	struct acpiac_softc *sc = arg;

	acpiac_getsta(sc);
	sc->sc_sens[0].value = sc->sc_ac_stat;
	acpi_record_event(sc->sc_acpi, APM_POWER_CHANGE);
}

int
acpiac_getsta(struct acpiac_softc *sc)
{
	int64_t sta;

	if (aml_evalname(sc->sc_acpi, sc->sc_devnode, "_STA", 0, NULL, NULL)) {
		dnprintf(10, "%s: no _STA\n",
		    DEVNAME(sc));
	}

	if (aml_evalinteger(sc->sc_acpi, sc->sc_devnode, "_PSR", 0, NULL, &sta)) {
		dnprintf(10, "%s: no _PSR\n",
		    DEVNAME(sc));
		return (1);
	}
	sc->sc_ac_stat = sta;
	return (0);
}

int
acpiac_notify(struct aml_node *node, int notify_type, void *arg)
{
	struct acpiac_softc *sc = arg;

	dnprintf(10, "acpiac_notify: %.2x %s\n", notify_type,
	    sc->sc_devnode->name);

	switch (notify_type) {
	case 0x00:
	case 0x01:
	case 0x81:
		/*
		 * XXX some sony vaio's use the wrong notify type
		 * work around it by honoring it as a 0x80
		 */
		/* FALLTHROUGH */
	case 0x80:
		acpiac_refresh(sc);
		dnprintf(10, "A/C status: %d\n", sc->sc_ac_stat);
		break;
	}
	return (0);
}
