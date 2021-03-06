/* $OpenBSD: src/lib/libcurses/trace/trace_buf.c,v 1.4 2010/01/12 23:22:07 nicm Exp $ */

/****************************************************************************
 * Copyright (c) 1998-2007,2008 Free Software Foundation, Inc.              *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Thomas E. Dickey                 1997-on                        *
 ****************************************************************************/
/*
 *	trace_buf.c - Tracing/Debugging buffers (attributes)
 */

#include <curses.priv.h>

MODULE_ID("$Id: trace_buf.c,v 1.14 2008/08/03 15:13:56 tom Exp $")

#define MyList _nc_globals.tracebuf_ptr
#define MySize _nc_globals.tracebuf_used

static char *
_nc_trace_alloc(int bufnum, size_t want)
{
#ifdef TRACE
    char *result = 0;

    if (bufnum >= 0) {
	if ((size_t) (bufnum + 1) > MySize) {
	    size_t need = (bufnum + 1) * 2;
	    if ((MyList = typeRealloc(TRACEBUF, need, MyList)) != 0) {
		while (need > MySize)
		    MyList[MySize++].text = 0;
	    }
	}

	if (MyList != 0) {
	    if (MyList[bufnum].text == 0
		|| want > MyList[bufnum].size) {
		MyList[bufnum].text = typeRealloc(char, want, MyList[bufnum].text);
		if (MyList[bufnum].text != 0)
		    MyList[bufnum].size = want;
	    }
	    result = MyList[bufnum].text;
	}
    }
#if NO_LEAKS
    else {
	if (MySize) {
	    if (MyList) {
		while (MySize--) {
		    if (MyList[MySize].text != 0) {
			free(MyList[MySize].text);
		    }
		}
		free(MyList);
		MyList = 0;
	    }
	    MySize = 0;
	}
    }
#endif
    return result;
#else
    return NULL;
#endif
}

/*
 * (re)Allocate a buffer big enough for the caller's wants.
 */
NCURSES_EXPORT(char *)
_nc_trace_buf(int bufnum, size_t want)
{
    char *result = _nc_trace_alloc(bufnum, want);
    if (result != 0)
	*result = '\0';
    return result;
}

/*
 * Append a new string to an existing buffer.
 */
NCURSES_EXPORT(char *)
_nc_trace_bufcat(int bufnum, const char *value)
{
    char *buffer = _nc_trace_alloc(bufnum, 0);
    if (buffer != 0) {
        size_t have = strlen(buffer), length;

	length = 1 + have + strlen(value);
	buffer = _nc_trace_alloc(bufnum, length);
	if (buffer != 0)
	     (void) strlcpy(buffer + have, value, length);

    }
    return buffer;
}
