/*	$OpenBSD: src/lib/libc/rpc/xdr_float.c,v 1.19 2011/07/26 09:36:25 martynas Exp $ */

/*
 * Copyright (c) 2010, Oracle America, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the "Oracle America, Inc." nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * xdr_float.c, Generic XDR routines implementation.
 *
 * These are the "floating point" xdr routines used to (de)serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

/*
 * NB: Not portable.
 * This routine works on machines with IEEE754 FP and Vaxen.
 */

#ifdef __vax__

/* What IEEE single precision floating point looks like on a Vax */
struct	ieee_single {
	unsigned int	mantissa: 23;
	unsigned int	exp     : 8;
	unsigned int	sign    : 1;
};

/* Vax single precision floating point */
struct	vax_single {
	unsigned int	mantissa1 : 7;
	unsigned int	exp       : 8;
	unsigned int	sign      : 1;
	unsigned int	mantissa2 : 16;
};

#define VAX_SNG_BIAS	0x81
#define IEEE_SNG_BIAS	0x7f

static struct sgl_limits {
	struct vax_single s;
	struct ieee_single ieee;
} sgl_limits[2] = {
	{{ 0x7f, 0xff, 0x0, 0xffff },	/* Max Vax */
	{ 0x0, 0xff, 0x0 }},		/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0 },	/* Min Vax */
	{ 0x0, 0x0, 0x0 }}		/* Min IEEE */
};

#else
#include <machine/endian.h>
#define IEEEFP
#endif

bool_t
xdr_float(XDR *xdrs, float *fp)
{
#ifdef IEEEFP
	bool_t rv;
	long tmpl;
#else
	struct ieee_single is;
	struct vax_single vs, *vsp;
	struct sgl_limits *lim;
	int i;
#endif
	switch (xdrs->x_op) {

	case XDR_ENCODE:
#ifdef IEEEFP 
		tmpl = *(int32_t *)fp;
		return (XDR_PUTLONG(xdrs, &tmpl));
#else
		vs = *((struct vax_single *)fp);
		for (i = 0, lim = sgl_limits;
			i < sizeof(sgl_limits)/sizeof(struct sgl_limits);
			i++, lim++) {
			if ((vs.mantissa2 == lim->s.mantissa2) &&
				(vs.exp == lim->s.exp) &&
				(vs.mantissa1 == lim->s.mantissa1)) {
				is = lim->ieee;
				goto shipit;
			}
		}
		is.exp = vs.exp - VAX_SNG_BIAS + IEEE_SNG_BIAS;
		is.mantissa = (vs.mantissa1 << 16) | vs.mantissa2;
	shipit:
		is.sign = vs.sign;
		return (XDR_PUTLONG(xdrs, (long *)&is));
#endif

	case XDR_DECODE:
#ifdef IEEEFP
		rv = XDR_GETLONG(xdrs, &tmpl);
		*(int32_t *)fp = tmpl;
		return (rv);
#else
		vsp = (struct vax_single *)fp;
		if (!XDR_GETLONG(xdrs, (long *)&is))
			return (FALSE);
		for (i = 0, lim = sgl_limits;
			i < sizeof(sgl_limits)/sizeof(struct sgl_limits);
			i++, lim++) {
			if ((is.exp == lim->ieee.exp) &&
				(is.mantissa == lim->ieee.mantissa)) {
				*vsp = lim->s;
				goto doneit;
			}
		}
		vsp->exp = is.exp - IEEE_SNG_BIAS + VAX_SNG_BIAS;
		vsp->mantissa2 = is.mantissa;
		vsp->mantissa1 = (is.mantissa >> 16);
	doneit:
		vsp->sign = is.sign;
		return (TRUE);
#endif

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}

#ifdef __vax__
/* What IEEE double precision floating point looks like on a Vax */
struct	ieee_double {
	unsigned int	mantissa1 : 20;
	unsigned int	exp       : 11;
	unsigned int	sign      : 1;
	unsigned int	mantissa2 : 32;
};

/* Vax double precision floating point */
struct  vax_double {
	unsigned int	mantissa1 : 7;
	unsigned int	exp       : 8;
	unsigned int	sign      : 1;
	unsigned int	mantissa2 : 16;
	unsigned int	mantissa3 : 16;
	unsigned int	mantissa4 : 16;
};

#define VAX_DBL_BIAS	0x81
#define IEEE_DBL_BIAS	0x3ff
#define MASK(nbits)	((1 << nbits) - 1)

static struct dbl_limits {
	struct	vax_double d;
	struct	ieee_double ieee;
} dbl_limits[2] = {
	{{ 0x7f, 0xff, 0x0, 0xffff, 0xffff, 0xffff },	/* Max Vax */
	{ 0x0, 0x7ff, 0x0, 0x0 }},			/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},		/* Min Vax */
	{ 0x0, 0x0, 0x0, 0x0 }}				/* Min IEEE */
};

#endif /* __vax__ */


bool_t
xdr_double(XDR *xdrs, double *dp)
{
#ifdef IEEEFP
	int32_t *i32p;
	bool_t rv;
	long tmpl;
#else
	long *lp;
	struct	ieee_double id;
	struct	vax_double vd;
	struct dbl_limits *lim;
	int i;
#endif

	switch (xdrs->x_op) {

	case XDR_ENCODE:
#ifdef IEEEFP
		i32p = (int32_t *)dp;
#if (BYTE_ORDER == BIG_ENDIAN) || defined(__arm__)
		tmpl = *i32p++;
		rv = XDR_PUTLONG(xdrs, &tmpl);
		if (!rv)
			return (rv);
		tmpl = *i32p;
		rv = XDR_PUTLONG(xdrs, &tmpl);
#else
		tmpl = *(i32p+1);
		rv = XDR_PUTLONG(xdrs, &tmpl);
		if (!rv)
			return (rv);
		tmpl = *i32p;
		rv = XDR_PUTLONG(xdrs, &tmpl);
#endif
		return (rv);
#else
		vd = *((struct vax_double *)dp);
		for (i = 0, lim = dbl_limits;
			i < sizeof(dbl_limits)/sizeof(struct dbl_limits);
			i++, lim++) {
			if ((vd.mantissa4 == lim->d.mantissa4) &&
				(vd.mantissa3 == lim->d.mantissa3) &&
				(vd.mantissa2 == lim->d.mantissa2) &&
				(vd.mantissa1 == lim->d.mantissa1) &&
				(vd.exp == lim->d.exp)) {
				id = lim->ieee;
				goto shipit;
			}
		}
		id.exp = vd.exp - VAX_DBL_BIAS + IEEE_DBL_BIAS;
		id.mantissa1 = (vd.mantissa1 << 13) | (vd.mantissa2 >> 3);
		id.mantissa2 = ((vd.mantissa2 & MASK(3)) << 29) |
				(vd.mantissa3 << 13) |
				((vd.mantissa4 >> 3) & MASK(13));
	shipit:
		id.sign = vd.sign;
		lp = (long *)&id;
		return (XDR_PUTLONG(xdrs, lp++) && XDR_PUTLONG(xdrs, lp));
#endif

	case XDR_DECODE:
#ifdef IEEEFP
		i32p = (int32_t *)dp;
#if (BYTE_ORDER == BIG_ENDIAN) || defined(__arm__)
		rv = XDR_GETLONG(xdrs, &tmpl);
		*i32p++ = tmpl;
		if (!rv)
			return (rv);
		rv = XDR_GETLONG(xdrs, &tmpl);
		*i32p = tmpl;
#else
		rv = XDR_GETLONG(xdrs, &tmpl);
		*(i32p+1) = tmpl;
		if (!rv)
			return (rv);
		rv = XDR_GETLONG(xdrs, &tmpl);
		*i32p = tmpl;
#endif
		return (rv);
#else
		lp = (long *)&id;
		if (!XDR_GETLONG(xdrs, lp++) || !XDR_GETLONG(xdrs, lp))
			return (FALSE);
		for (i = 0, lim = dbl_limits;
			i < sizeof(dbl_limits)/sizeof(struct dbl_limits);
			i++, lim++) {
			if ((id.mantissa2 == lim->ieee.mantissa2) &&
				(id.mantissa1 == lim->ieee.mantissa1) &&
				(id.exp == lim->ieee.exp)) {
				vd = lim->d;
				goto doneit;
			}
		}
		vd.exp = id.exp - IEEE_DBL_BIAS + VAX_DBL_BIAS;
		vd.mantissa1 = (id.mantissa1 >> 13);
		vd.mantissa2 = ((id.mantissa1 & MASK(13)) << 3) |
				(id.mantissa2 >> 29);
		vd.mantissa3 = (id.mantissa2 >> 13);
		vd.mantissa4 = (id.mantissa2 << 3);
	doneit:
		vd.sign = id.sign;
		*dp = *((double *)&vd);
		return (TRUE);
#endif

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}
