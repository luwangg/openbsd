/*	$OpenBSD: src/sys/dev/usb/uftdi.c,v 1.6 2002/06/18 00:59:16 nate Exp $ 	*/
/*	$NetBSD: uftdi.c,v 1.6 2001/01/23 21:56:17 augustss Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * FTDI FT8U100AX serial adapter driver
 */

/*
 * XXX This driver will not support multiple serial ports.
 * XXX The ucom layer needs to be extended first.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/ucomvar.h>

#include <dev/usb/uftdireg.h>

#ifdef UFTDI_DEBUG
#define DPRINTF(x)	if (uftdidebug) printf x
#define DPRINTFN(n,x)	if (uftdidebug>(n)) printf x
int uftdidebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define UFTDI_CONFIG_INDEX	0
#define UFTDI_IFACE_INDEX	0


/*
 * These are the maximum number of bytes transferred per frame.
 * The output buffer size cannot be increased due to the size encoding.
 */
#define UFTDIIBUFSIZE 64
#define UFTDIOBUFSIZE 64

struct uftdi_softc {
	USBBASEDEVICE		sc_dev;		/* base device */
	usbd_device_handle	sc_udev;	/* device */
	usbd_interface_handle	sc_iface;	/* interface */

	u_char			sc_msr;
	u_char			sc_lsr;

	device_ptr_t		sc_subdev;

	u_char			sc_dying;
};

Static void	uftdi_get_status(void *, int portno, u_char *lsr, u_char *msr);
Static void	uftdi_set(void *, int, int, int);
Static int	uftdi_param(void *, int, struct termios *);
Static int	uftdi_open(void *sc, int portno);
Static void	uftdi_read(void *sc, int portno, u_char **ptr,u_int32_t *count);
Static void	uftdi_write(void *sc, int portno, u_char *to, u_char *from,
			    u_int32_t *count);

struct ucom_methods uftdi_methods = {
	uftdi_get_status,
	uftdi_set,
	uftdi_param,
	NULL,
	uftdi_open,
	NULL,
	uftdi_read,
	uftdi_write,
};

USB_DECLARE_DRIVER(uftdi);

USB_MATCH(uftdi)
{
	USB_MATCH_START(uftdi, uaa);
	
	if (uaa->iface != NULL)
		return (UMATCH_NONE);

	DPRINTFN(20,("uftdi: vendor=0x%x, product=0x%x\n",
		     uaa->vendor, uaa->product));

	if (uaa->vendor == USB_VENDOR_FTDI &&
	    uaa->product == USB_PRODUCT_FTDI_SERIAL_8U100AX)
		return (UMATCH_VENDOR_PRODUCT);

	return (UMATCH_NONE);
}

USB_ATTACH(uftdi)
{
	USB_ATTACH_START(uftdi, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char devinfo[1024];
	char *devname = USBDEVNAME(sc->sc_dev);
	int i;
	usbd_status err;
	struct ucom_attach_args uca;

	DPRINTFN(10,("\nuftdi_attach: sc=%p\n", sc));

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, UFTDI_CONFIG_INDEX, 1);
	if (err) {
		printf("\n%s: failed to set configuration, err=%s\n",
		       devname, usbd_errstr(err));
		goto bad;
	}

	err = usbd_device2interface_handle(dev, UFTDI_IFACE_INDEX, &iface);
	if (err) {
		printf("\n%s: failed to get interface, err=%s\n",
		       devname, usbd_errstr(err));
		goto bad;
	}

	usbd_devinfo(dev, 0, devinfo);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", devname, devinfo);

	id = usbd_get_interface_descriptor(iface);

	sc->sc_udev = dev;
	sc->sc_iface = iface;

	uca.bulkin = uca.bulkout = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		int addr, dir, attr;
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: could not read endpoint descriptor"
			       ": %s\n", devname, usbd_errstr(err));
			goto bad;
		}
		
		addr = ed->bEndpointAddress;
		dir = UE_GET_DIR(ed->bEndpointAddress);
		attr = ed->bmAttributes & UE_XFERTYPE;
		if (dir == UE_DIR_IN && attr == UE_BULK)
			uca.bulkin = addr;
		else if (dir == UE_DIR_OUT && attr == UE_BULK)
			uca.bulkout = addr;
		else {
			printf("%s: unexpected endpoint\n", devname);
			goto bad;
		}
	}
	if (uca.bulkin == -1) {
		printf("%s: Could not find data bulk in\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (uca.bulkout == -1) {
		printf("%s: Could not find data bulk out\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	
	uca.portno = FTDI_PIT_SIOA;
	/* bulkin, bulkout set above */
	uca.ibufsize = UFTDIIBUFSIZE;
	uca.obufsize = UFTDIOBUFSIZE - 1;
	uca.ibufsizepad = UFTDIIBUFSIZE;
	uca.opkthdrlen = 1;
	uca.device = dev;
	uca.iface = iface;
	uca.methods = &uftdi_methods;
	uca.arg = sc;
	uca.info = NULL;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	DPRINTF(("uftdi: in=0x%x out=0x%x\n", uca.bulkin, uca.bulkout));
	sc->sc_subdev = config_found_sm(self, &uca, ucomprint, ucomsubmatch);

	USB_ATTACH_SUCCESS_RETURN;

bad:
	DPRINTF(("uftdi_attach: ATTACH ERROR\n"));
	sc->sc_dying = 1;
	USB_ATTACH_ERROR_RETURN;
}

int
uftdi_activate(device_ptr_t self, enum devact act)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_subdev != NULL)
			rv = config_deactivate(sc->sc_subdev);
		sc->sc_dying = 1;
		break;
	}
	return (rv);
}

int
uftdi_detach(device_ptr_t self, int flags)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;
	int rv = 0;

	DPRINTF(("uftdi_detach: sc=%p flags=%d\n", sc, flags));
	sc->sc_dying = 1;
	if (sc->sc_subdev != NULL) {
		rv = config_detach(sc->sc_subdev, flags);
		sc->sc_subdev = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return (0);
}

Static int
uftdi_open(void *vsc, int portno)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	struct termios t;

	DPRINTF(("uftdi_open: sc=%p\n", sc));

	if (sc->sc_dying)
		return (EIO);

	/* Perform a full reset on the device */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_RESET;
	USETW(req.wValue, FTDI_SIO_RESET_SIO);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	/* Set 9600 baud, 2 stop bits, no parity, 8 bits */
	t.c_ospeed = 9600;
	t.c_cflag = CSTOPB | CS8;
	(void)uftdi_param(sc, portno, &t);

	/* Turn on RTS/CTS flow control */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_FLOW_CTRL;
	USETW(req.wValue, 0);
	USETW2(req.wIndex, FTDI_SIO_RTS_CTS_HS, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	return (0);
}

Static void
uftdi_read(void *vsc, int portno, u_char **ptr, u_int32_t *count)
{
	struct uftdi_softc *sc = vsc;
	u_char msr, lsr;

	DPRINTFN(15,("uftdi_read: sc=%p, port=%d count=%d\n", sc, portno,
		     *count));

	msr = FTDI_GET_MSR(*ptr);
	lsr = FTDI_GET_LSR(*ptr);

#ifdef UFTDI_DEBUG
	if (*count != 2)
		DPRINTFN(10,("uftdi_read: sc=%p, port=%d count=%d data[0]="
			    "0x%02x\n", sc, portno, *count, (*ptr)[2]));
#endif

	if (sc->sc_msr != msr ||
	    (sc->sc_lsr & FTDI_LSR_MASK) != (lsr & FTDI_LSR_MASK)) {
		DPRINTF(("uftdi_read: status change msr=0x%02x(0x%02x) "
			 "lsr=0x%02x(0x%02x)\n", msr, sc->sc_msr,
			 lsr, sc->sc_lsr));
		sc->sc_msr = msr;
		sc->sc_lsr = lsr;
		ucom_status_change((struct ucom_softc *)sc->sc_subdev);
	}

	/* Pick up status and adjust data part. */
	*ptr += 2;
	*count -= 2;
}

Static void
uftdi_write(void *vsc, int portno, u_char *to, u_char *from, u_int32_t *count)
{
	DPRINTFN(10,("uftdi_write: sc=%p, port=%d count=%u data[0]=0x%02x\n",
		     vsc, portno, *count, from[0]));

	/* Make length tag and copy data */
	*to = FTDI_OUT_TAG(*count, portno);
	memcpy(to+1, from, *count);
	++*count;
}

Static void
uftdi_set(void *vsc, int portno, int reg, int onoff)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	int ctl;

	DPRINTF(("uftdi_set: sc=%p, port=%d reg=%d onoff=%d\n", vsc, portno,
		 reg, onoff));

	switch (reg) {
	case UCOM_SET_DTR:
		ctl = onoff ? FTDI_SIO_SET_DTR_HIGH : FTDI_SIO_SET_DTR_LOW;
		break;
	case UCOM_SET_RTS:
		ctl = onoff ? FTDI_SIO_SET_RTS_HIGH : FTDI_SIO_SET_RTS_LOW;
		break;
	case UCOM_SET_BREAK:
		/* XXX how do we set break? */
		return;
	default:
		return;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_MODEM_CTRL;
	USETW(req.wValue, ctl);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_set: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	(void)usbd_do_request(sc->sc_udev, &req, NULL);
}

Static int
uftdi_param(void *vsc, int portno, struct termios *t)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	int rate, data;

	DPRINTF(("uftdi_param: sc=%p\n", sc));

	if (sc->sc_dying)
		return (EIO);

	switch (t->c_ospeed) {
	case 300: rate = ftdi_sio_b300; break;
	case 600: rate = ftdi_sio_b600; break;
	case 1200: rate = ftdi_sio_b1200; break;
	case 2400: rate = ftdi_sio_b2400; break;
	case 4800: rate = ftdi_sio_b4800; break;
	case 9600: rate = ftdi_sio_b9600; break;
	case 19200: rate = ftdi_sio_b19200; break;
	case 38400: rate = ftdi_sio_b38400; break;
	case 57600: rate = ftdi_sio_b57600; break;
	case 115200: rate = ftdi_sio_b115200; break;
	default:
		return (EINVAL);
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_BAUD_RATE;
	USETW(req.wValue, rate);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	if (ISSET(t->c_cflag, CSTOPB))
		data = FTDI_SIO_SET_DATA_STOP_BITS_2;
	else
		data = FTDI_SIO_SET_DATA_STOP_BITS_1;
	if (ISSET(t->c_cflag, PARENB)) {
		if (ISSET(t->c_cflag, PARODD))
			data |= FTDI_SIO_SET_DATA_PARITY_ODD;
		else
			data |= FTDI_SIO_SET_DATA_PARITY_EVEN;
	} else
		data |= FTDI_SIO_SET_DATA_PARITY_NONE;
	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		data |= FTDI_SIO_SET_DATA_BITS(5);
		break;
	case CS6:
		data |= FTDI_SIO_SET_DATA_BITS(6);
		break;
	case CS7:
		data |= FTDI_SIO_SET_DATA_BITS(7);
		break;
	case CS8:
		data |= FTDI_SIO_SET_DATA_BITS(8);
		break;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	return (0);
}

void
uftdi_get_status(void *vsc, int portno, u_char *lsr, u_char *msr)
{
	struct uftdi_softc *sc = vsc;

	DPRINTF(("uftdi_status: msr=0x%02x lsr=0x%02x\n",
		 sc->sc_msr, sc->sc_lsr));

	if (msr != NULL)
		*msr = sc->sc_msr;
	if (lsr != NULL)
		*lsr = sc->sc_lsr;
}
