/*	$OpenBSD: src/lib/libcurses/curses.h,v 1.26 1999/03/16 15:25:08 millert Exp $	*/

/****************************************************************************
 * Copyright (c) 1998 Free Software Foundation, Inc.                        *
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
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995               *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/

/* $From: curses.h.in,v 1.81 1999/02/28 23:38:22 tom Exp $ */

#ifndef __NCURSES_H
#define __NCURSES_H

#define CURSES 1
#define CURSES_H 1

/* This should be defined for the enhanced functionality to be visible.
 * However, none of the wide-character (enhanced) functionality is implemented.
 * So we do not define it (yet).
#define _XOPEN_CURSES 1
 */

/* These are defined only in curses.h, and are used for conditional compiles */
#define NCURSES_VERSION_MAJOR 5
#define NCURSES_VERSION_MINOR 0
#define NCURSES_VERSION_PATCH 990316

/* This is defined in more than one ncurses header, for identification */
#undef  NCURSES_VERSION
#define NCURSES_VERSION "5.0"

#ifdef NCURSES_NOMACROS
#define NCURSES_ATTR_T attr_t
#endif

#ifndef NCURSES_ATTR_T
#define NCURSES_ATTR_T int
#endif

#undef  NCURSES_CONST
#define NCURSES_CONST 

typedef unsigned long chtype;

#include <stdio.h>
#include <unctrl.h>
#include <stdarg.h>	/* we need va_list */
#ifdef _XOPEN_SOURCE_EXTENDED
#include <stddef.h>	/* we want wchar_t */
#endif /* _XOPEN_SOURCE_EXTENDED */

/* XSI and SVr4 specify that curses implements 'bool'.  However, C++ may also
 * implement it.  If so, we must use the C++ compiler's type to avoid conflict
 * with other interfaces.
 */

#undef TRUE
#define TRUE    1

#undef FALSE
#define FALSE   0

#if !defined(__cplusplus) || !1
typedef char bool;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * XSI attributes.  In the ncurses implementation, they are identical to the
 * A_ attributes.
 */
#define WA_ATTRIBUTES	A_ATTRIBUTES
#define WA_NORMAL	A_NORMAL
#define WA_STANDOUT	A_STANDOUT
#define WA_UNDERLINE	A_UNDERLINE
#define WA_REVERSE	A_REVERSE
#define WA_BLINK	A_BLINK
#define WA_DIM		A_DIM
#define WA_BOLD		A_BOLD
#define WA_ALTCHARSET	A_ALTCHARSET
#define WA_INVIS	A_INVIS
#define WA_PROTECT	A_PROTECT
#define WA_HORIZONTAL	A_HORIZONTAL
#define WA_LEFT		A_LEFT
#define WA_LOW		A_LOW
#define WA_RIGHT	A_RIGHT
#define WA_TOP		A_TOP
#define WA_VERTICAL	A_VERTICAL

/* colors */
extern int COLORS;
extern int COLOR_PAIRS;

#define COLOR_BLACK	0
#define COLOR_RED	1
#define COLOR_GREEN	2
#define COLOR_YELLOW	3
#define COLOR_BLUE	4
#define COLOR_MAGENTA	5
#define COLOR_CYAN	6
#define COLOR_WHITE	7

/* line graphics */

extern	chtype acs_map[];

/* VT100 symbols begin here */
#define ACS_ULCORNER	(acs_map['l'])	/* upper left corner */
#define ACS_LLCORNER	(acs_map['m'])	/* lower left corner */
#define ACS_URCORNER	(acs_map['k'])	/* upper right corner */
#define ACS_LRCORNER	(acs_map['j'])	/* lower right corner */
#define ACS_LTEE	(acs_map['t'])	/* tee pointing right */
#define ACS_RTEE	(acs_map['u'])	/* tee pointing left */
#define ACS_BTEE	(acs_map['v'])	/* tee pointing up */
#define ACS_TTEE	(acs_map['w'])	/* tee pointing down */
#define ACS_HLINE	(acs_map['q'])	/* horizontal line */
#define ACS_VLINE	(acs_map['x'])	/* vertical line */
#define ACS_PLUS	(acs_map['n'])	/* large plus or crossover */
#define ACS_S1		(acs_map['o'])	/* scan line 1 */
#define ACS_S9		(acs_map['s'])	/* scan line 9 */
#define ACS_DIAMOND	(acs_map['`'])	/* diamond */
#define ACS_CKBOARD	(acs_map['a'])	/* checker board (stipple) */
#define ACS_DEGREE	(acs_map['f'])	/* degree symbol */
#define ACS_PLMINUS	(acs_map['g'])	/* plus/minus */
#define ACS_BULLET	(acs_map['~'])	/* bullet */
/* Teletype 5410v1 symbols begin here */
#define ACS_LARROW	(acs_map[','])	/* arrow pointing left */
#define ACS_RARROW	(acs_map['+'])	/* arrow pointing right */
#define ACS_DARROW	(acs_map['.'])	/* arrow pointing down */
#define ACS_UARROW	(acs_map['-'])	/* arrow pointing up */
#define ACS_BOARD	(acs_map['h'])	/* board of squares */
#define ACS_LANTERN	(acs_map['i'])	/* lantern symbol */
#define ACS_BLOCK	(acs_map['0'])	/* solid square block */
/*
 * These aren't documented, but a lot of System Vs have them anyway
 * (you can spot pprryyzz{{||}} in a lot of AT&T terminfo strings).
 * The ACS_names may not match AT&T's, our source didn't know them.
 */
#define ACS_S3		(acs_map['p'])	/* scan line 3 */
#define ACS_S7		(acs_map['r'])	/* scan line 7 */
#define ACS_LEQUAL	(acs_map['y'])	/* less/equal */
#define ACS_GEQUAL	(acs_map['z'])	/* greater/equal */
#define ACS_PI		(acs_map['{'])	/* Pi */
#define ACS_NEQUAL	(acs_map['|'])	/* not equal */
#define ACS_STERLING	(acs_map['}'])	/* UK pound sign */

/*
 * Line drawing ACS names are of the form ACS_trbl, where t is the top, r
 * is the right, b is the bottom, and l is the left.  t, r, b, and l might
 * be B (blank), S (single), D (double), or T (thick).  The subset defined
 * here only uses B and S.
 */
#define ACS_BSSB	ACS_ULCORNER
#define ACS_SSBB	ACS_LLCORNER
#define ACS_BBSS	ACS_URCORNER
#define ACS_SBBS	ACS_LRCORNER
#define ACS_SBSS	ACS_RTEE
#define ACS_SSSB	ACS_LTEE
#define ACS_SSBS	ACS_BTEE
#define ACS_BSSS	ACS_TTEE
#define ACS_BSBS	ACS_HLINE
#define ACS_SBSB	ACS_VLINE
#define ACS_SSSS	ACS_PLUS

#if	!defined(ERR) || ((ERR) != -1)
#define ERR     (-1)
#endif

#if	!defined(OK) || ((OK) != 0)
#define OK      (0)
#endif

/* values for the _flags member */
#define _SUBWIN         0x01	/* is this a sub-window? */
#define _ENDLINE        0x02	/* is the window flush right? */
#define _FULLWIN        0x04	/* is the window full-screen? */
#define _SCROLLWIN      0x08	/* bottom edge is at screen bottom? */
#define _ISPAD	        0x10	/* is this window a pad? */
#define _HASMOVED       0x20	/* has cursor moved since last refresh? */
#define _WRAPPED        0x40	/* cursor was just wrappped */

/*
 * this value is used in the firstchar and lastchar fields to mark
 * unchanged lines
 */
#define _NOCHANGE       -1

/*
 * this value is used in the oldindex field to mark lines created by insertions
 * and scrolls.
 */
#define _NEWINDEX	-1

typedef struct screen  SCREEN;
typedef struct _win_st WINDOW;

typedef	chtype	attr_t;		/* ...must be at least as wide as chtype */

#ifdef _XOPEN_SOURCE_EXTENDED
#ifndef _WCHAR_T
typedef unsigned long wchar_t;
#endif /* _WCHAR_T */
#ifndef _WINT_T
typedef long int wint_t;
#endif /* _WINT_T */

#define CCHARW_MAX	5
typedef struct
{
    attr_t	attr;
    wchar_t	chars[CCHARW_MAX];
}
cchar_t;
#endif /* _XOPEN_SOURCE_EXTENDED */

struct ldat
{
	chtype  *text;		/* text of the line */
	short   firstchar;	/* first changed character in the line */
	short   lastchar;	/* last changed character in the line */
	short   oldindex;	/* index of the line at last update */
};

struct _win_st
{
	short   _cury, _curx;	/* current cursor position */

	/* window location and size */
	short   _maxy, _maxx;	/* maximums of x and y, NOT window size */
	short   _begy, _begx;	/* screen coords of upper-left-hand corner */

	short   _flags;		/* window state flags */

	/* attribute tracking */
	attr_t  _attrs;		/* current attribute for non-space character */
	chtype  _bkgd;		/* current background char/attribute pair */

	/* option values set by user */
	bool	_notimeout;	/* no time out on function-key entry? */
	bool    _clear;		/* consider all data in the window invalid? */
	bool    _leaveok;	/* OK to not reset cursor on exit? */
	bool    _scroll;	/* OK to scroll this window? */
	bool    _idlok;		/* OK to use insert/delete line? */
	bool    _idcok;		/* OK to use insert/delete char? */
	bool	_immed;		/* window in immed mode? (not yet used) */
	bool	_sync;		/* window in sync mode? */
	bool    _use_keypad;    /* process function keys into KEY_ symbols? */
	int	_delay;		/* 0 = nodelay, <0 = blocking, >0 = delay */

	struct ldat *_line;	/* the actual line data */

	/* global screen state */
	short	_regtop;	/* top line of scrolling region */
	short	_regbottom;	/* bottom line of scrolling region */

	/* these are used only if this is a sub-window */
	int	_parx;		/* x coordinate of this window in parent */
	int	_pary;		/* y coordinate of this window in parent */
	WINDOW	*_parent;	/* pointer to parent if a sub-window */

	/* these are used only if this is a pad */
	struct pdat
	{
	    short _pad_y,      _pad_x;
	    short _pad_top,    _pad_left;
	    short _pad_bottom, _pad_right;
	} _pad;

	short   _yoffset;	/* real begy is _begy + _yoffset */
};

extern WINDOW   *stdscr;
extern WINDOW   *curscr;
extern WINDOW   *newscr;

extern int	LINES;
extern int	COLS;
extern int	TABSIZE;

/*
 * This global was an undocumented feature under AIX curses.
 */
extern int ESCDELAY;	/* ESC expire time in milliseconds */

extern char *keybound (int, int);
extern int define_key (char *, int);
extern int keyok (int, bool);
extern int resizeterm (int, int);
extern int use_default_colors (void);
extern int use_extended_names (bool);
extern int wresize (WINDOW *, int, int);

extern char ttytype[];		/* needed for backward compatibility */

/*
 * GCC (and some other compilers) define '__attribute__'; we're using this
 * macro to alert the compiler to flag inconsistencies in printf/scanf-like
 * function calls.  Just in case '__attribute__' isn't defined, make a dummy.
 * G++ doesn't accept it anyway.
 */
#if defined(__cplusplus) || (!defined(__GNUC__) && !defined(__attribute__))
#define __attribute__(p) /* nothing */
#endif

/*
 * We cannot define these in ncurses_cfg.h, since they require parameters to be
 * passed (that's non-portable).
 */
#if	GCC_PRINTF
#define GCC_PRINTFLIKE(fmt,var) __attribute__((format(printf,fmt,var)))
#else
#define GCC_PRINTFLIKE(fmt,var) /*nothing*/
#endif

#if	GCC_SCANF
#define GCC_SCANFLIKE(fmt,var)  __attribute__((format(scanf,fmt,var)))
#else
#define GCC_SCANFLIKE(fmt,var)  /*nothing*/
#endif

#ifndef	GCC_NORETURN
#define	GCC_NORETURN /* nothing */
#endif

#ifndef	GCC_UNUSED
#define	GCC_UNUSED /* nothing */
#endif

/*
 * Function prototypes.  This is the complete XSI Curses list of required
 * functions.  Those marked `generated' will have sources generated from the
 * macro definitions later in this file, in order to satisfy XPG4.2
 * requirements.
 */

extern int addch(const chtype);				/* generated */
extern int addchnstr(const chtype *, int);		/* generated */
extern int addchstr(const chtype *);			/* generated */
extern int addnstr(const char *, int);			/* generated */
extern int addstr(const char *);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int addnwstr(const wchar_t *, int);		/* missing */
extern int addwstr(const wchar_t *);			/* missing */
extern int add_wch(const cchar_t *);			/* missing */
extern int add_wchnstr(const cchar_t *, int);		/* missing */
extern int add_wchstr(const cchar_t *);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int attroff(NCURSES_ATTR_T);			/* generated */
extern int attron(NCURSES_ATTR_T);			/* generated */
extern int attrset(NCURSES_ATTR_T);			/* generated */
extern int attr_get(attr_t *, short *, void *);		/* generated */
extern int attr_off(attr_t, void *);			/* generated */
extern int attr_on(attr_t, void *);			/* generated */
extern int attr_set(attr_t, short, void *);		/* generated */
extern int baudrate(void);				/* implemented */
extern int beep(void);					/* implemented */
extern int bkgd(chtype);				/* generated */
extern void bkgdset(chtype);				/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern void bkgrndset(const cchar_t *);			/* missing */
extern int bkgrnd(const cchar_t *);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int border(chtype,chtype,chtype,chtype,chtype,chtype,chtype,chtype);	/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int border_set(const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int box(WINDOW *, chtype, chtype);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int box_set(WINDOW *, const cchar_t *, const cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern bool can_change_color(void);			/* implemented */
extern int cbreak(void);				/* implemented */
extern int chgat(int, attr_t, short, const void *);	/* generated */
extern int clear(void);					/* generated */
extern int clearok(WINDOW *,bool);			/* implemented */
extern int clrtobot(void);				/* generated */
extern int clrtoeol(void);				/* generated */
extern int color_content(short,short*,short*,short*);	/* implemented */
extern int color_set(short,void*);			/* generated */
extern int COLOR_PAIR(int);				/* generated */
extern int copywin(const WINDOW*,WINDOW*,int,int,int,int,int,int,int);	/* implemented */
extern int curs_set(int);				/* implemented */
extern int def_prog_mode(void);				/* implemented */
extern int def_shell_mode(void);			/* implemented */
extern int delay_output(int);				/* implemented */
extern int delch(void);					/* generated */
extern void delscreen(SCREEN *);			/* implemented */
extern int delwin(WINDOW *);				/* implemented */
extern int deleteln(void);				/* generated */
extern WINDOW *derwin(WINDOW *,int,int,int,int);	/* implemented */
extern int doupdate(void);				/* implemented */
extern WINDOW *dupwin(WINDOW *);			/* implemented */
extern int echo(void);					/* implemented */
extern int echochar(const chtype);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int echo_wchar(const cchar_t *);			/* missing */
extern int erasewchar(wchar_t*);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int endwin(void);				/* implemented */
extern char erasechar(void);				/* implemented */
extern void filter(void);				/* implemented */
extern int flash(void);					/* implemented */
extern int flushinp(void);				/* implemented */
extern chtype getbkgd(WINDOW *);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int getbkgrnd(cchar_t *);			/* missing */
extern int getcchar(const cchar_t *, wchar_t*, attr_t*, short*, void*);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int getch(void);					/* generated */
extern int getnstr(char *, int);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int getn_wstr(wint_t *, int);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int getstr(char *);				/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int get_wch(wint_t *);				/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern WINDOW *getwin(FILE *);				/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int get_wstr(wint_t *);				/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int halfdelay(int);				/* implemented */
extern bool has_colors(void);				/* implemented */
extern bool has_ic(void);				/* implemented */
extern bool has_il(void);				/* implemented */
extern int hline(chtype, int);				/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int hline_set(const cchar_t *, int);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern void idcok(WINDOW *, bool);			/* implemented */
extern int idlok(WINDOW *, bool);			/* implemented */
extern void immedok(WINDOW *, bool);			/* implemented */
extern chtype inch(void);				/* generated */
extern int inchnstr(chtype *, int);			/* generated */
extern int inchstr(chtype *);				/* generated */
extern WINDOW *initscr(void);				/* implemented */
extern int init_color(short,short,short,short);		/* implemented */
extern int init_pair(short,short,short);		/* implemented */
extern int innstr(char *, int);				/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int innwstr(wchar_t *, int);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int insch(chtype);				/* generated */
extern int insdelln(int);				/* generated */
extern int insertln(void);				/* generated */
extern int insnstr(const char *, int);			/* generated */
extern int insstr(const char *);			/* generated */
extern int instr(char *);				/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int ins_nwstr(const wchar_t *, int);		/* missing */
extern int ins_wch(const cchar_t *);			/* missing */
extern int ins_wstr(const wchar_t *);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int intrflush(WINDOW *,bool);			/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int inwstr(wchar_t *);				/* missing */
extern int in_wch(NCURSES_CONST cchar_t *);		/* missing */
extern int in_wchstr(NCURSES_CONST cchar_t *);		/* missing */
extern int in_wchnstr(NCURSES_CONST cchar_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern bool isendwin(void);				/* implemented */
extern bool is_linetouched(WINDOW *,int);		/* implemented */
extern bool is_wintouched(WINDOW *);			/* implemented */
extern NCURSES_CONST char *keyname(int);		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern char *key_name(wchar_t);				/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int keypad(WINDOW *,bool);			/* implemented */
extern char killchar(void);				/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int killwchar(wchar_t *);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int leaveok(WINDOW *,bool);			/* implemented */
extern char *longname(void);				/* implemented */
extern int meta(WINDOW *,bool);				/* implemented */
extern int move(int, int);				/* generated */
extern int mvaddch(int, int, const chtype);		/* generated */
extern int mvaddchnstr(int, int, const chtype *, int);	/* generated */
extern int mvaddchstr(int, int, const chtype *);	/* generated */
extern int mvaddnstr(int, int, const char *, int);	/* generated */
extern int mvaddstr(int, int, const char *);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvaddnwstr(int, int, const wchar_t *, int);	/* missing */
extern int mvaddwstr(int, int, const wchar_t *);	/* missing */
extern int mvadd_wch(int, int, const cchar_t *);	/* missing */
extern int mvadd_wchnstr(int, int, const cchar_t *, int);/* missing */
extern int mvadd_wchstr(int, int, const cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvchgat(int, int, int, attr_t, short, const void *);	/* generated */
extern int mvcur(int,int,int,int);			/* implemented */
extern int mvdelch(int, int);				/* generated */
extern int mvderwin(WINDOW *, int, int);		/* implemented */
extern int mvgetch(int, int);				/* generated */
extern int mvgetnstr(int, int, char *, int);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvgetn_wstr(int, int, wint_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvgetstr(int, int, char *);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvget_wch(int, int, wint_t *);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvget_wstr(int, int, wint_t *);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvhline(int, int, chtype, int);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvhline_set(int, int, const cchar_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern chtype mvinch(int, int);				/* generated */
extern int mvinchnstr(int, int, chtype *, int);		/* generated */
extern int mvinchstr(int, int, chtype *);		/* generated */
extern int mvinnstr(int, int, char *, int);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvinnwstr(int, int, wchar_t *, int);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvinsch(int, int, chtype);			/* generated */
extern int mvinsnstr(int, int, const char *, int);	/* generated */
extern int mvinsstr(int, int, const char *);		/* generated */
extern int mvinstr(int, int, char *);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvins_nwstr(int, int, const wchar_t *, int);	/* missing */
extern int mvins_wch(int, int, const cchar_t *);	/* missing */
extern int mvins_wstr(int, int, const wchar_t *);	/* missing */
extern int mvinwstr(int, int, wchar_t *);		/* missing */
extern int mvin_wch(int, int, NCURSES_CONST cchar_t *);	/* missing */
extern int mvin_wchstr(int, int, NCURSES_CONST cchar_t *);	/* missing */
extern int mvin_wchnstr(int, int, NCURSES_CONST cchar_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvprintw(int,int, NCURSES_CONST char *,...)	/* implemented */
		GCC_PRINTFLIKE(3,4);
extern int mvscanw(int,int, NCURSES_CONST char *,...)	/* implemented */
		GCC_SCANFLIKE(3,4);
extern int mvvline(int, int, chtype, int);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvvline_set(int, int, const cchar_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvwaddch(WINDOW *, int, int, const chtype);	/* generated */
extern int mvwaddchnstr(WINDOW *, int, int, const chtype *, int);/* generated */
extern int mvwaddchstr(WINDOW *, int, int, const chtype *);	/* generated */
extern int mvwaddnstr(WINDOW *, int, int, const char *, int);	/* generated */
extern int mvwaddstr(WINDOW *, int, int, const char *);	/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwaddnwstr(WINDOW *, int, int, const wchar_t *, int);/* missing */
extern int mvwaddwstr(WINDOW *, int, int, const wchar_t *);	/* missing */
extern int mvwadd_wch(WINDOW *, int, int, const cchar_t *);	/* missing */
extern int mvwadd_wchnstr(WINDOW *, int, int, const cchar_t *, int); /* missing */
extern int mvwadd_wchstr(WINDOW *, int, int, const cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvwchgat(WINDOW *, int, int, int, attr_t, short, const void *);/* generated */
extern int mvwdelch(WINDOW *, int, int);		/* generated */
extern int mvwgetch(WINDOW *, int, int);		/* generated */
extern int mvwgetnstr(WINDOW *, int, int, char *, int);	/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwgetn_wstr(WINDOW *, int, int, wint_t *, int);/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvwgetstr(WINDOW *, int, int, char *);	/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwget_wch(WINDOW *, int, int, wint_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwget_wstr(WINDOW *, int, int, wint_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvwhline(WINDOW *, int, int, chtype, int);	/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwhline_set(WINDOW *, int, int, const cchar_t *, int);/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvwin(WINDOW *,int,int);			/* implemented */
extern chtype mvwinch(WINDOW *, int, int);			/* generated */
extern int mvwinchnstr(WINDOW *, int, int, chtype *, int);	/* generated */
extern int mvwinchstr(WINDOW *, int, int, chtype *);		/* generated */
extern int mvwinnstr(WINDOW *, int, int, char *, int);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwinnwstr(WINDOW *, int, int, wchar_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvwinsch(WINDOW *, int, int, chtype);		/* generated */
extern int mvwinsnstr(WINDOW *, int, int, const char *, int);	/* generated */
extern int mvwinsstr(WINDOW *, int, int, const char *);		/* generated */
extern int mvwinstr(WINDOW *, int, int, char *);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwins_nwstr(WINDOW *, int,int, const wchar_t *,int); /* missing */
extern int mvwins_wch(WINDOW *, int, int, const cchar_t *);	/* missing */
extern int mvwins_wstr(WINDOW *, int, int, const wchar_t *);	/* missing */
extern int mvwinwstr(WINDOW *, int, int, wchar_t *);		/* missing */
extern int mvwin_wch(WINDOW *, int, int, NCURSES_CONST cchar_t *);	/* missing */
extern int mvwin_wchnstr(WINDOW *, int,int, NCURSES_CONST cchar_t *,int); /* missing */
extern int mvwin_wchstr(WINDOW *, int, int, NCURSES_CONST cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int mvwprintw(WINDOW*,int,int, NCURSES_CONST char *,...)	/* implemented */
		GCC_PRINTFLIKE(4,5);
extern int mvwscanw(WINDOW *,int,int, NCURSES_CONST char *,...)	/* implemented */
		GCC_SCANFLIKE(4,5);
extern int mvwvline(WINDOW *,int, int, chtype, int);	/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int mvwvline_set(WINDOW *, int,int, const cchar_t *,int); /* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int napms(int);					/* implemented */
extern WINDOW *newpad(int,int);				/* implemented */
extern SCREEN *newterm(NCURSES_CONST char *,FILE *,FILE *);	/* implemented */
extern WINDOW *newwin(int,int,int,int);			/* implemented */
extern int nl(void);					/* implemented */
extern int nocbreak(void);				/* implemented */
extern int nodelay(WINDOW *,bool);			/* implemented */
extern int noecho(void);				/* implemented */
extern int nonl(void);					/* implemented */
extern void noqiflush(void);				/* implemented */
extern int noraw(void);					/* implemented */
extern int notimeout(WINDOW *,bool);			/* implemented */
extern int overlay(const WINDOW*,WINDOW *);		/* implemented */
extern int overwrite(const WINDOW*,WINDOW *);		/* implemented */
extern int pair_content(short,short*,short*);		/* implemented */
extern int PAIR_NUMBER(int);				/* generated */
extern int pechochar(WINDOW *, const chtype);		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int pecho_wchar(WINDOW *, const cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int pnoutrefresh(WINDOW*,int,int,int,int,int,int);/* implemented */
extern int prefresh(WINDOW *,int,int,int,int,int,int);	/* implemented */
extern int printw(NCURSES_CONST char *,...)		/* implemented */
		GCC_PRINTFLIKE(1,2);
extern int putp(const char *);				/* implemented */
extern int putwin(WINDOW *, FILE *);			/* implemented */
extern void qiflush(void);				/* implemented */
extern int raw(void);					/* implemented */
extern int redrawwin(WINDOW *);				/* generated */
extern int refresh(void);				/* generated */
extern int resetty(void);				/* implemented */
extern int reset_prog_mode(void);			/* implemented */
extern int reset_shell_mode(void);			/* implemented */
extern int ripoffline(int, int (*init)(WINDOW *, int));	/* implemented */
extern int savetty(void);				/* implemented */
extern int scanw(NCURSES_CONST char *,...)		/* implemented */
		GCC_SCANFLIKE(1,2);
extern int scr_dump(const char *);			/* implemented */
extern int scr_init(const char *);			/* implemented */
extern int scrl(int);					/* generated */
extern int scroll(WINDOW *);				/* generated */
extern int scrollok(WINDOW *,bool);			/* implemented */
extern int scr_restore(const char *);			/* implemented */
extern int scr_set(const char *);			/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int setcchar(cchar_t *, const wchar_t *, const attr_t, short, const void *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int setscrreg(int,int);				/* generated */
extern SCREEN *set_term(SCREEN *);			/* implemented */
extern int slk_attroff(const chtype);			/* implemented */
extern int slk_attr_off(const attr_t, void *);		/* generated:WIDEC */
extern int slk_attron(const chtype);			/* implemented */
extern int slk_attr_on(attr_t,void*);			/* generated:WIDEC */
extern int slk_attrset(const chtype);			/* implemented */
extern attr_t slk_attr(void);				/* implemented */
extern int slk_attr_set(const attr_t,short,void*);	/* implemented */
extern int slk_clear(void);				/* implemented */
extern int slk_color(short);				/* implemented */
extern int slk_init(int);				/* implemented */
extern char *slk_label(int);				/* implemented */
extern int slk_noutrefresh(void);			/* implemented */
extern int slk_refresh(void);				/* implemented */
extern int slk_restore(void);				/* implemented */
extern int slk_set(int,const char *,int);		/* implemented */
extern int slk_touch(void);				/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int slk_wset(int, const wchar_t *, int);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int standout(void);				/* generated */
extern int standend(void);				/* generated */
extern int start_color(void);				/* implemented */
extern WINDOW *subpad(WINDOW *, int, int, int, int);	/* implemented */
extern WINDOW *subwin(WINDOW *,int,int,int,int);	/* implemented */
extern int syncok(WINDOW *, bool);			/* implemented */
extern chtype termattrs(void);				/* implemented */
extern attr_t term_attrs(void);				/* missing */
extern char *termname(void);				/* implemented */
extern int tigetflag(NCURSES_CONST char *);		/* implemented */
extern int tigetnum(NCURSES_CONST char *);		/* implemented */
extern char *tigetstr(NCURSES_CONST char *);		/* implemented */
extern void timeout(int);				/* generated */
extern char *tparm(NCURSES_CONST char *, ...);		/* implemented */
extern int typeahead(int);				/* implemented */
extern int ungetch(int);				/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int unget_wch(const wchar_t);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int untouchwin(WINDOW *);			/* generated */
extern void use_env(bool);				/* implemented */
extern int vidattr(chtype);				/* implemented */
extern int vid_attr(attr_t, short, void *);		/* generated:WIDEC */
extern int vidputs(chtype, int (*)(int));		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int vid_puts(attr_t, short, void *, int (*)(int)); /* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int vline(chtype, int);				/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int vline_set(const cchar_t *, int);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int vwprintw(WINDOW *, NCURSES_CONST char *,va_list);	/* implemented */
extern int vw_printw(WINDOW *, NCURSES_CONST char *,va_list);	/* generated */
extern int vwscanw(WINDOW *, NCURSES_CONST char *,va_list);	/* implemented */
extern int vw_scanw(WINDOW *, NCURSES_CONST char *,va_list);	/* generated */
extern int waddch(WINDOW *, const chtype);		/* implemented */
extern int waddchnstr(WINDOW *,const chtype *const,int); /* implemented */
extern int waddchstr(WINDOW *,const chtype *);		/* generated */
extern int waddnstr(WINDOW *,const char *const,int);	/* implemented */
extern int waddstr(WINDOW *,const char *);		/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int waddwstr(WINDOW *,const wchar_t *);		/* missing */
extern int waddnwstr(WINDOW *,const wchar_t *,int);	/* missing */
extern int wadd_wch(WINDOW *,const cchar_t *);		/* missing */
extern int wadd_wchnstr(WINDOW *,const cchar_t *,int);	/* missing */
extern int wadd_wchstr(WINDOW *,const cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int wattron(WINDOW *, int);			/* generated */
extern int wattroff(WINDOW *, int);			/* generated */
extern int wattrset(WINDOW *, int);			/* generated */
extern int wattr_get(WINDOW *, attr_t *, short *, void *);	/* generated */
extern int wattr_on(WINDOW *, NCURSES_CONST attr_t, void *);	/* implemented */
extern int wattr_off(WINDOW *, NCURSES_CONST attr_t, void *);	/* implemented */
extern int wattr_set(WINDOW *, attr_t, short, void *);		/* generated */
extern int wbkgd(WINDOW *,const chtype);		/* implemented */
extern void wbkgdset(WINDOW *,chtype);			/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern void wbkgrndset(WINDOW *,const cchar_t *);	/* missing */
extern int wbkgrnd(WINDOW *,const cchar_t *);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int wborder(WINDOW *,chtype,chtype,chtype,chtype,chtype,chtype,chtype,chtype);	/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int wborder_set(WINDOW *,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*,const cchar_t*);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int wchgat(WINDOW *, int, attr_t, short, const void *);/* implemented */
extern int wclear(WINDOW *);				/* implemented */
extern int wclrtobot(WINDOW *);				/* implemented */
extern int wclrtoeol(WINDOW *);				/* implemented */
extern int wcolor_set(WINDOW*,short,void*);		/* implemented */
extern void wcursyncup(WINDOW *);			/* implemented */
extern int wdelch(WINDOW *);				/* implemented */
extern int wdeleteln(WINDOW *);				/* generated */
extern int wechochar(WINDOW *, const chtype);		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int wecho_wchar(WINDOW *, const cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int werase(WINDOW *);				/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int wgetbkgrnd(WINDOW *, cchar_t *);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int wgetch(WINDOW *);				/* implemented */
extern int wgetnstr(WINDOW *,char *,int);		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int wgetn_wstr(WINDOW *,wint_t *, int);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int wgetstr(WINDOW *, char *);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int wget_wch(WINDOW *, wint_t *);		/* missing */
extern int wget_wstr(WINDOW *, wint_t *);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int whline(WINDOW *, chtype, int);		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int whline_set(WINDOW *, const cchar_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern chtype winch(WINDOW *);				/* implemented */
extern int winchnstr(WINDOW *, chtype *, int);		/* implemented */
extern int winchstr(WINDOW *, chtype *);		/* generated */
extern int winnstr(WINDOW *, char *, int);		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int winnwstr(WINDOW *, wchar_t *, int);		/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int winsch(WINDOW *, chtype);			/* implemented */
extern int winsdelln(WINDOW *,int);			/* implemented */
extern int winsertln(WINDOW *);				/* generated */
extern int winsnstr(WINDOW *, const char *,int);	/* implemented */
extern int winsstr(WINDOW *, const char *);		/* generated */
extern int winstr(WINDOW *, char *);			/* generated */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int wins_nwstr(WINDOW *, const wchar_t *, int);	/* missing */
extern int wins_wch(WINDOW *, const cchar_t *);		/* missing */
extern int wins_wstr(WINDOW *, const wchar_t *);	/* missing */
extern int winwstr(WINDOW *, wchar_t *);		/* missing */
extern int win_wch(WINDOW *, NCURSES_CONST cchar_t *);	/* missing */
extern int win_wchnstr(WINDOW *, NCURSES_CONST cchar_t *, int);	/* missing */
extern int win_wchstr(WINDOW *, NCURSES_CONST cchar_t *);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int wmove(WINDOW *,int,int);			/* implemented */
extern int wnoutrefresh(WINDOW *);			/* implemented */
extern int wprintw(WINDOW *, NCURSES_CONST char *,...)	/* implemented */
		GCC_PRINTFLIKE(2,3);
extern int wredrawln(WINDOW *,int,int);			/* implemented */
extern int wrefresh(WINDOW *);				/* implemented */
extern int wscanw(WINDOW *, NCURSES_CONST char *,...)	/* implemented */
		GCC_SCANFLIKE(2,3);
extern int wscrl(WINDOW *,int);				/* implemented */
extern int wsetscrreg(WINDOW *,int,int);		/* implemented */
extern int wstandout(WINDOW *);				/* generated */
extern int wstandend(WINDOW *);				/* generated */
extern void wsyncdown(WINDOW *);			/* implemented */
extern void wsyncup(WINDOW *);				/* implemented */
extern void wtimeout(WINDOW *,int);			/* implemented */
extern int wtouchln(WINDOW *,int,int,int);		/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern wchar_t *wunctrl(cchar_t *);			/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */
extern int wvline(WINDOW *,chtype,int);			/* implemented */
#ifdef _XOPEN_SOURCE_EXTENDED
extern int wvline_set(WINDOW *, const cchar_t *, int);	/* missing */
#endif /* _XOPEN_SOURCE_EXTENDED */

extern bool mouse_trafo(int*, int*, bool);              /* generated */

/* attributes */

#define NCURSES_BITS(mask,shift) ((mask) << ((shift) + 8))

#define A_NORMAL	0L
#define A_ATTRIBUTES	NCURSES_BITS(~(1UL - 1UL),0)
#define A_CHARTEXT	(NCURSES_BITS(1UL,0) - 1UL)
#define A_COLOR		NCURSES_BITS(((1UL) << 8) - 1UL,0)
#define A_STANDOUT	NCURSES_BITS(1UL,8)
#define A_UNDERLINE	NCURSES_BITS(1UL,9)
#define A_REVERSE	NCURSES_BITS(1UL,10)
#define A_BLINK		NCURSES_BITS(1UL,11)
#define A_DIM		NCURSES_BITS(1UL,12)
#define A_BOLD		NCURSES_BITS(1UL,13)
#define A_ALTCHARSET	NCURSES_BITS(1UL,14)
#define A_INVIS		NCURSES_BITS(1UL,15)

/* Tradeoff on 32-bit machines ('protect' vs widec).  The others (e.g., left
 * highlight are not implemented in any terminal descriptions, anyway.
 */
#if ((16 + 8) < 32)
#define A_PROTECT	NCURSES_BITS(1UL,16)
#define A_HORIZONTAL	NCURSES_BITS(1UL,17)
#define A_LEFT		NCURSES_BITS(1UL,18)
#define A_LOW		NCURSES_BITS(1UL,19)
#define A_RIGHT		NCURSES_BITS(1UL,20)
#define A_TOP		NCURSES_BITS(1UL,21)
#define A_VERTICAL	NCURSES_BITS(1UL,22)
#else
#define A_PROTECT	0L
#define A_HORIZONTAL	0L
#define A_LEFT		0L
#define A_LOW		0L
#define A_RIGHT		0L
#define A_TOP		0L
#define A_VERTICAL	0L
#endif

#define COLOR_PAIR(n)	NCURSES_BITS(n, 0)
#define PAIR_NUMBER(a)	(((a) & A_COLOR) >> 8)

/*
 * pseudo functions
 */
#define wgetstr(w, s)		wgetnstr(w, s, -1)
#define getnstr(s, n)		wgetnstr(stdscr, s, n)

#define setterm(term)		setupterm(term, 1, (int *)0)

#define fixterm()		reset_prog_mode()
#define resetterm()		reset_shell_mode()
#define saveterm()		def_prog_mode()
#define crmode()		cbreak()
#define nocrmode()		nocbreak()
#define gettmode()

#define getyx(win,y,x)   	(y = (win)?(win)->_cury:ERR, x = (win)?(win)->_curx:ERR)
#define getbegyx(win,y,x)	(y = (win)?(win)->_begy:ERR, x = (win)?(win)->_begx:ERR)
#define getmaxyx(win,y,x)	(y = (win)?((win)->_maxy + 1):ERR, x = (win)?((win)->_maxx + 1):ERR)
#define getparyx(win,y,x)	(y = (win)?(win)->_pary:ERR, x = (win)?(win)->_parx:ERR)
#define getsyx(y,x) do { if(newscr->_leaveok) (y)=(x)=-1; \
			 else getyx(newscr,(y),(x)); \
		    } while(0)
#define setsyx(y,x) do { if((y)==-1 && (x)==-1) newscr->_leaveok=TRUE; \
			 else {newscr->_leaveok=FALSE;wmove(newscr,(y),(x));} \
		    } while(0)

/* It seems older SYSV curses versions define these */
#define getattrs(win)		((win)?(win)->_attrs:A_NORMAL)
#define getcurx(win)		((win)?(win)->_curx:ERR)
#define getcury(win)		((win)?(win)->_cury:ERR)
#define getbegx(win)		((win)?(win)->_begx:ERR)
#define getbegy(win)		((win)?(win)->_begy:ERR)
#define getmaxx(win)		((win)?((win)->_maxx + 1):ERR)
#define getmaxy(win)		((win)?((win)->_maxy + 1):ERR)
#define getparx(win)		((win)?(win)->_parx:ERR)
#define getpary(win)		((win)?(win)->_pary:ERR)

#define wstandout(win)      	(wattrset(win,A_STANDOUT))
#define wstandend(win)      	(wattrset(win,A_NORMAL))
#define wattr_set(win,a,p,opts) ((win)->_attrs = (((a) & ~A_COLOR) | COLOR_PAIR(p)), OK)

#define wattron(win,at)		wattr_on(win, at, (void *)0)
#define wattroff(win,at)	wattr_off(win, at, (void *)0)
#define wattrset(win,at)    	((win)->_attrs = (at))

#define scroll(win)		wscrl(win,1)

#define touchwin(win)		wtouchln((win), 0, getmaxy(win), 1)
#define touchline(win, s, c)	wtouchln((win), s, c, 1)
#define untouchwin(win)		wtouchln((win), 0, getmaxy(win), 0)

#define box(win, v, h)		wborder(win, v, v, h, h, 0, 0, 0, 0)
#define border(ls, rs, ts, bs, tl, tr, bl, br)	wborder(stdscr, ls, rs, ts, bs, tl, tr, bl, br)
#define hline(ch, n)		whline(stdscr, ch, n)
#define vline(ch, n)		wvline(stdscr, ch, n)

#define winstr(w, s)		winnstr(w, s, -1)
#define winchstr(w, s)		winchnstr(w, s, -1)
#define winsstr(w, s)		winsnstr(w, s, -1)

#define redrawwin(w)		wredrawln(w, 0, w->_maxy+1)
#define waddstr(win,str)	waddnstr(win,str,-1)
#define waddchstr(win,str)	waddchnstr(win,str,-1)

/*
 * pseudo functions for standard screen
 */

#define addch(ch)      		waddch(stdscr,ch)
#define addchnstr(str,n)	waddchnstr(stdscr,str,n)
#define addchstr(str)		waddchstr(stdscr,str)
#define addnstr(str,n)		waddnstr(stdscr,str,n)
#define addstr(str)    		waddnstr(stdscr,str,-1)
#define attroff(at)    		wattroff(stdscr,at)
#define attron(at)     		wattron(stdscr,at)
#define attrset(at)    		wattrset(stdscr,at)
#define bkgd(ch)		wbkgd(stdscr,ch)
#define bkgdset(ch)		wbkgdset(stdscr,ch)
#define clear()        		wclear(stdscr)
#define clrtobot()     		wclrtobot(stdscr)
#define clrtoeol()     		wclrtoeol(stdscr)
#define color_set(c,o)		wcolor_set(stdscr,c,o)
#define delch()        		wdelch(stdscr)
#define deleteln()     		winsdelln(stdscr,-1)
#define echochar(c)		wechochar(stdscr,c)
#define erase()        		werase(stdscr)
#define getch()        		wgetch(stdscr)
#define getstr(str)    		wgetstr(stdscr,str)
#define inch()       		winch(stdscr)
#define inchnstr(s,n)		winchnstr(stdscr,s,n)
#define inchstr(s)		winchstr(stdscr,s)
#define innstr(s,n)		winnstr(stdscr,s,n)
#define insch(c)       		winsch(stdscr,c)
#define insdelln(n)		winsdelln(stdscr,n)
#define insertln()     		winsdelln(stdscr,1)
#define insnstr(s,n)		winsnstr(stdscr,s,n)
#define insstr(s)		winsstr(stdscr,s)
#define instr(s)		winstr(stdscr,s)
#define move(y,x)     		wmove(stdscr,y,x)
#define refresh()      		wrefresh(stdscr)
#define scrl(n)			wscrl(stdscr,n)
#define setscrreg(t,b) 		wsetscrreg(stdscr,t,b)
#define standend()     		wstandend(stdscr)
#define standout()     		wstandout(stdscr)
#define timeout(delay)		wtimeout(stdscr,delay)
#define wdeleteln(win)     	winsdelln(win,-1)
#define winsertln(win)     	winsdelln(win,1)

/*
 * mv functions
 */

#define mvwaddch(win,y,x,ch)    	(wmove(win,y,x) == ERR ? ERR : waddch(win,ch))
#define mvwaddchnstr(win,y,x,str,n)	(wmove(win,y,x) == ERR ? ERR : waddchnstr(win,str,n))
#define mvwaddchstr(win,y,x,str)  	(wmove(win,y,x) == ERR ? ERR : waddchnstr(win,str,-1))
#define mvwaddnstr(win,y,x,str,n)	(wmove(win,y,x) == ERR ? ERR : waddnstr(win,str,n))
#define mvwaddstr(win,y,x,str)  	(wmove(win,y,x) == ERR ? ERR : waddnstr(win,str,-1))
#define mvwdelch(win,y,x)       	(wmove(win,y,x) == ERR ? ERR : wdelch(win))
#define mvwgetch(win,y,x)       	(wmove(win,y,x) == ERR ? ERR : wgetch(win))
#define mvwgetnstr(win,y,x,str,n)    	(wmove(win,y,x) == ERR ? ERR : wgetnstr(win,str,n))
#define mvwgetstr(win,y,x,str)      	(wmove(win,y,x) == ERR ? ERR : wgetstr(win,str))
#define mvwhline(win,y,x,c,n)     	(wmove(win,y,x) == ERR ? ERR : whline(win,c,n))
#define mvwinch(win,y,x)        	(wmove(win,y,x) == ERR ? (chtype)ERR : winch(win))
#define mvwinchnstr(win,y,x,s,n)	(wmove(win,y,x) == ERR ? ERR : winchnstr(win,s,n))
#define mvwinchstr(win,y,x,s)		(wmove(win,y,x) == ERR ? ERR : winchstr(win,s))
#define mvwinnstr(win,y,x,s,n)		(wmove(win,y,x) == ERR ? ERR : winnstr(win,s,n))
#define mvwinsch(win,y,x,c)     	(wmove(win,y,x) == ERR ? ERR : winsch(win,c))
#define mvwinsnstr(win,y,x,s,n)		(wmove(win,y,x) == ERR ? ERR : winsnstr(win,s,n))
#define mvwinsstr(win,y,x,s)		(wmove(win,y,x) == ERR ? ERR : winsstr(win,s))
#define mvwinstr(win,y,x,s)		(wmove(win,y,x) == ERR ? ERR : winstr(win,s))
#define mvwvline(win,y,x,c,n)     	(wmove(win,y,x) == ERR ? ERR : wvline(win,c,n))

#define mvaddch(y,x,ch)         	mvwaddch(stdscr,y,x,ch)
#define mvaddchnstr(y,x,str,n)		mvwaddchnstr(stdscr,y,x,str,n)
#define mvaddchstr(y,x,str)		mvwaddchstr(stdscr,y,x,str)
#define mvaddnstr(y,x,str,n)		mvwaddnstr(stdscr,y,x,str,n)
#define mvaddstr(y,x,str)       	mvwaddstr(stdscr,y,x,str)
#define mvdelch(y,x)            	mvwdelch(stdscr,y,x)
#define mvgetch(y,x)            	mvwgetch(stdscr,y,x)
#define mvgetnstr(y,x,str,n)		mvwgetnstr(stdscr,y,x,str,n)
#define mvgetstr(y,x,str)           	mvwgetstr(stdscr,y,x,str)
#define mvhline(y,x,c,n)		mvwhline(stdscr,y,x,c,n)
#define mvinch(y,x)             	mvwinch(stdscr,y,x)
#define mvinchnstr(y,x,s,n)		mvwinchnstr(stdscr,y,x,s,n)
#define mvinchstr(y,x,s)		mvwinchstr(stdscr,y,x,s)
#define mvinnstr(y,x,s,n)		mvwinnstr(stdscr,y,x,s,n)
#define mvinsch(y,x,c)          	mvwinsch(stdscr,y,x,c)
#define mvinsnstr(y,x,s,n)		mvwinsnstr(stdscr,y,x,s,n)
#define mvinsstr(y,x,s)			mvwinsstr(stdscr,y,x,s)
#define mvinstr(y,x,s)			mvwinstr(stdscr,y,x,s)
#define mvvline(y,x,c,n)		mvwvline(stdscr,y,x,c,n)

/*
 * XSI curses macros for XPG4 conformance.
 * The underlying functions needed to make these work are:
 * waddnwstr(), waddchnwstr(), wadd_wch(), wborder_set(), wchgat(),
 * wecho_wchar(), wgetn_wstr(), wget_wch(), whline_set(), vhline_set(),
 * winnwstr(), wins_nwstr(), wins_wch(), win_wch(), win_wchnstr().
 * Except for wchgat(), these are not yet implemented.  They will be someday.
 */
#define add_wch(c)			wadd_wch(stdscr,c)
#define addnwstr(wstr,n)		waddnwstr(stdscr,wstr,n)
#define addwstr(wstr,n)			waddnwstr(stdscr,wstr,-1)
#define attr_get(a,pair,opts)		wattr_get(stdscr,a,pair,opts)
#define attr_off(a,opts)		wattr_off(stdscr,a,opts)
#define attr_on(a,opts)			wattr_on(stdscr,a,opts)
#define attr_set(a,pair,opts)		wattr_set(stdscr,a,pair,opts)
#define box_set(w,v,h)			wborder_set(w,v,v,h,h,0,0,0,0)
#define chgat(n,a,c,o)			wchgat(stdscr,n,a,c,o)
#define echo_wchar(c)			wecho_wchar(stdscr,c)
#define getbkgd(win)			((win)->_bkgd)
#define get_wch(c)			wget_wch(stdscr,c)
#define get_wstr(t)			wgetn_wstr(stdscr,t,-1)
#define getn_wstr(t,n)			wgetn_wstr(stdscr,t,n)
#define hline_set(c,n)			whline_set(stdscr,c,n)
#define in_wch(c)			win_wch(stdscr,c)
#define in_wchnstr(c,n)			win_wchnstr(stdscr,c,n)
#define in_wchstr(c)			win_wchnstr(stdscr,c,-1)
#define innwstr(c,n)			winnwstr(stdscr,c,n)
#define ins_nwstr(t,n)			wins_nwstr(stdscr,t,n)
#define ins_wch(c)			wins_wch(stdscr,c)
#define ins_wstr(t)			wins_nwstr(stdscr,t,-1)
#define inwstr(c)			winnwstr(stdscr,c,-1)

#define mvadd_wch(y,x,c)		mvwadd_wch(stdscr,y,x,c)
#define mvaddnwstr(y,x,wstr,n)		mvwaddnwstr(stdscr,y,x,wstr,n)
#define mvaddwstr(y,x,wstr,n)		mvwaddnwstr(stdscr,y,x,wstr,-1)
#define mvchgat(y,x,n,a,c,o)		mvwchgat(stdscr,y,x,n,a,c,o)
#define mvget_wch(y,x,c)		mvwget_wch(stdscr,y,x,c)
#define mvget_wstr(y,x,t)		mvwgetn_wstr(stdscr,y,x,t,-1)
#define mvgetn_wstr(y,x,t,n)		mvwgetn_wstr(stdscr,y,x,t,n)
#define mvhline_set(y,x,c,n)		mvwhline_set(stdscr,y,x,c,n)
#define mvin_wch(y,x,c)			mvwin_wch(stdscr,y,x,c)
#define mvin_wchnstr(y,x,c,n)		mvwin_wchnstr(stdscr,y,x,c,n)
#define mvin_wchstr(y,x,c)		mvwin_wchnstr(stdscr,y,x,c,-1)
#define mvinnwstr(y,x,c,n)		mvwinnwstr(stdscr,y,x,c,n)
#define mvins_nwstr(y,x,t,n)		mvwins_nwstr(stdscr,y,x,t,n)
#define mvins_wch(y,x,c)		mvwins_wch(stdscr,y,x,c)
#define mvins_wstr(y,x,t)		mvwins_nwstr(stdscr,y,x,t,-1)
#define mvinwstr(y,x,c)			mvwinnwstr(stdscr,y,x,c,-1)
#define mvvline_set(y,x,c,n)		mvwvline_set(stdscr,y,x,c,n)

#define mvwadd_wch(win,y,x,c)		(wmove(win,y,x) == ERR ? ERR : wadd_wch(stdscr,c))
#define mvwaddnwstr(win,y,x,wstr,n)	(wmove(win,y,x) == ERR ? ERR : waddnwstr(stdscr,wstr,n))
#define mvwaddwstr(win,y,x,wstr,n)	(wmove(win,y,x) == ERR ? ERR : waddnwstr(stdscr,wstr,-1))
#define mvwchgat(win,y,x,n,a,c,o)	(wmove(win,y,x) == ERR ? ERR : wchgat(win,n,a,c,o))
#define mvwget_wch(win,y,x,c)		(wmove(win,y,x) == ERR ? ERR : wget_wch(win,c))
#define mvwget_wstr(win,y,x,t)		(wmove(win,y,x) == ERR ? ERR : wgetn_wstr(win,t,-1))
#define mvwgetn_wstr(win,y,x,t,n)	(wmove(win,y,x) == ERR ? ERR : wgetn_wstr(win,t,n))
#define mvwhline_set(win,y,x,c,n)	(wmove(win,y,x) == ERR ? ERR : whline_set(win,c,n))
#define mvwin_wch(win,y,x,c)		(wmove(win,y,x) == ERR ? ERR : win_wch(win,c))
#define mvwin_wchnstr(win,y,x,c,n)	(wmove(win,y,x) == ERR ? ERR : win_wchnstr(stdscr,c,n))
#define mvwin_wchstr(win,y,x,c)		(wmove(win,y,x) == ERR ? ERR : win_wchnstr(stdscr,c,-1))
#define mvwinnwstr(win,y,x,c,n)		(wmove(win,y,x) == ERR ? ERR : winnwstr(stdscr,c,n))
#define mvwins_nwstr(win,y,x,t,n)	(wmove(win,y,x) == ERR ? ERR : wins_nwstr(stdscr,t,n))
#define mvwins_wch(win,y,x,c)		(wmove(win,y,x) == ERR ? ERR : wins_wch(stdscr,c))
#define mvwins_wstr(win,y,x,t)		(wmove(win,y,x) == ERR ? ERR : wins_nwstr(stdscr,t,-1))
#define mvwinwstr(win,y,x,c)		(wmove(win,y,x) == ERR ? ERR : winnwstr(stdscr,c,-1))
#define mvwvline_set(win,y,x,c,n)	(wmove(win,y,x) == ERR ? ERR : wvline_set(win,c,n))

#define slk_attr_off(a,v)		((v) ? ERR : slk_attroff(a))
#define slk_attr_on(a,v)		((v) ? ERR : slk_attron(a))

#define vid_attr(a,pair,opts)		vidattr(a)
#define vline_set(c,n)			wvline_set(stdscr,c,n)
#define waddwstr(win,wstr,n)		waddnwstr(win,wstr,-1)
#define wattr_get(win,a,p,opts)		((void)((a) != 0 && (*(a) = (win)->_attrs)), \
					 (void)((p) != 0 && (*(p) = PAIR_NUMBER((win)->_attrs))), \
					 OK)
#define wget_wstr(w,t)			wgetn_wstr(w,t,-1)
#define win_wchstr(w,c)			win_wchnstr(w,c,-1)
#define wins_wstr(w,t)			wins_nwstr(w,t,-1)
#define winwstr(w,c)			winnwstr(w,c,-1)


/*
 * XSI curses deprecates SVr4 vwprintw/vwscanw, which are supposed to use
 * varargs.h.  It adds new calls vw_printw/vw_scanw, which are supposed to
 * use POSIX stdarg.h.  The ncurses versions of vwprintw/vwscanw already
 * use stdarg.h, so...
 */
#define vw_printw		vwprintw
#define vw_scanw		vwscanw

/*
 * Pseudo-character tokens outside ASCII range.  The curses wgetch() function
 * will return any given one of these only if the corresponding k- capability
 * is defined in your terminal's terminfo entry.
 */
#define KEY_CODE_YES	0400		/* A wchar_t contains a key code */
#define KEY_MIN		0401		/* Minimum curses key */
#define KEY_BREAK       0401            /* Break key (unreliable) */
#define KEY_DOWN        0402            /* Down-arrow */
#define KEY_UP          0403		/* Up-arrow */
#define KEY_LEFT        0404		/* Left-arrow */
#define KEY_RIGHT       0405            /* Right-arrow */
#define KEY_HOME        0406            /* Home key (upward+left arrow) */
#define KEY_BACKSPACE   0407            /* Backspace (unreliable) */
#define KEY_F0          0410            /* Function keys.  Space for 64 */
#define KEY_F(n)        (KEY_F0+(n))    /* Value of function key n */
#define KEY_DL          0510            /* Delete line */
#define KEY_IL          0511            /* Insert line */
#define KEY_DC          0512            /* Delete character */
#define KEY_IC          0513            /* Insert char or enter insert mode */
#define KEY_EIC         0514            /* Exit insert char mode */
#define KEY_CLEAR       0515            /* Clear screen */
#define KEY_EOS         0516            /* Clear to end of screen */
#define KEY_EOL         0517            /* Clear to end of line */
#define KEY_SF          0520            /* Scroll 1 line forward */
#define KEY_SR          0521            /* Scroll 1 line backward (reverse) */
#define KEY_NPAGE       0522            /* Next page */
#define KEY_PPAGE       0523            /* Previous page */
#define KEY_STAB        0524            /* Set tab */
#define KEY_CTAB        0525            /* Clear tab */
#define KEY_CATAB       0526            /* Clear all tabs */
#define KEY_ENTER       0527            /* Enter or send (unreliable) */
#define KEY_SRESET      0530            /* Soft (partial) reset (unreliable) */
#define KEY_RESET       0531            /* Reset or hard reset (unreliable) */
#define KEY_PRINT       0532            /* Print */
#define KEY_LL          0533            /* Home down or bottom (lower left) */

/* The keypad is arranged like this: */
/* a1    up    a3   */
/* left   b2  right  */
/* c1   down   c3   */

#define KEY_A1		0534		/* Upper left of keypad */
#define KEY_A3		0535		/* Upper right of keypad */
#define KEY_B2		0536		/* Center of keypad */
#define KEY_C1		0537		/* Lower left of keypad */
#define KEY_C3		0540		/* Lower right of keypad */
#define KEY_BTAB	0541		/* Back tab */
#define KEY_BEG		0542		/* Beg (beginning) */
#define KEY_CANCEL	0543		/* Cancel */
#define KEY_CLOSE	0544		/* Close */
#define KEY_COMMAND	0545		/* Cmd (command) */
#define KEY_COPY	0546		/* Copy */
#define KEY_CREATE	0547		/* Create */
#define KEY_END		0550		/* End */
#define KEY_EXIT	0551		/* Exit */
#define KEY_FIND	0552		/* Find */
#define KEY_HELP	0553		/* Help */
#define KEY_MARK	0554		/* Mark */
#define KEY_MESSAGE	0555		/* Message */
#define KEY_MOVE	0556		/* Move */
#define KEY_NEXT	0557		/* Next */
#define KEY_OPEN	0560		/* Open */
#define KEY_OPTIONS	0561		/* Options */
#define KEY_PREVIOUS	0562		/* Prev (previous) */
#define KEY_REDO	0563		/* Redo */
#define KEY_REFERENCE	0564		/* Ref (reference) */
#define KEY_REFRESH	0565		/* Refresh */
#define KEY_REPLACE	0566		/* Replace */
#define KEY_RESTART	0567		/* Restart */
#define KEY_RESUME	0570		/* Resume */
#define KEY_SAVE	0571		/* Save */
#define KEY_SBEG	0572		/* Shifted Beg (beginning) */
#define KEY_SCANCEL	0573		/* Shifted Cancel */
#define KEY_SCOMMAND	0574		/* Shifted Command */
#define KEY_SCOPY	0575		/* Shifted Copy */
#define KEY_SCREATE	0576		/* Shifted Create */
#define KEY_SDC		0577		/* Shifted Delete char */
#define KEY_SDL		0600		/* Shifted Delete line */
#define KEY_SELECT	0601		/* Select */
#define KEY_SEND	0602		/* Shifted End */
#define KEY_SEOL	0603		/* Shifted Clear line */
#define KEY_SEXIT	0604		/* Shifted Dxit */
#define KEY_SFIND	0605		/* Shifted Find */
#define KEY_SHELP	0606		/* Shifted Help */
#define KEY_SHOME	0607		/* Shifted Home */
#define KEY_SIC		0610		/* Shifted Input */
#define KEY_SLEFT	0611		/* Shifted Left arrow */
#define KEY_SMESSAGE	0612		/* Shifted Message */
#define KEY_SMOVE	0613		/* Shifted Move */
#define KEY_SNEXT	0614		/* Shifted Next */
#define KEY_SOPTIONS	0615		/* Shifted Options */
#define KEY_SPREVIOUS	0616		/* Shifted Prev */
#define KEY_SPRINT	0617		/* Shifted Print */
#define KEY_SREDO	0620		/* Shifted Redo */
#define KEY_SREPLACE	0621		/* Shifted Replace */
#define KEY_SRIGHT	0622		/* Shifted Right arrow */
#define KEY_SRSUME	0623		/* Shifted Resume */
#define KEY_SSAVE	0624		/* Shifted Save */
#define KEY_SSUSPEND	0625		/* Shifted Suspend */
#define KEY_SUNDO	0626		/* Shifted Undo */
#define KEY_SUSPEND	0627		/* Suspend */
#define KEY_UNDO	0630		/* Undo */
#define KEY_MOUSE	0631		/* Mouse event has occurred */
#define KEY_RESIZE	0632		/* Terminal resize event */
#define KEY_MAX		0777		/* Maximum key value */

/* mouse interface */
#define NCURSES_MOUSE_VERSION	1

/* event masks */
#define	BUTTON1_RELEASED	000000000001L
#define	BUTTON1_PRESSED		000000000002L
#define	BUTTON1_CLICKED		000000000004L
#define	BUTTON1_DOUBLE_CLICKED	000000000010L
#define	BUTTON1_TRIPLE_CLICKED	000000000020L
#define BUTTON1_RESERVED_EVENT	000000000040L
#define	BUTTON2_RELEASED	000000000100L
#define	BUTTON2_PRESSED		000000000200L
#define	BUTTON2_CLICKED		000000000400L
#define	BUTTON2_DOUBLE_CLICKED	000000001000L
#define	BUTTON2_TRIPLE_CLICKED	000000002000L
#define BUTTON2_RESERVED_EVENT	000000004000L
#define	BUTTON3_RELEASED	000000010000L
#define	BUTTON3_PRESSED		000000020000L
#define	BUTTON3_CLICKED		000000040000L
#define	BUTTON3_DOUBLE_CLICKED	000000100000L
#define	BUTTON3_TRIPLE_CLICKED	000000200000L
#define BUTTON3_RESERVED_EVENT	000000400000L
#define	BUTTON4_RELEASED	000001000000L
#define	BUTTON4_PRESSED		000002000000L
#define	BUTTON4_CLICKED		000004000000L
#define	BUTTON4_DOUBLE_CLICKED	000010000000L
#define	BUTTON4_TRIPLE_CLICKED	000020000000L
#define BUTTON4_RESERVED_EVENT	000040000000L
#define BUTTON_CTRL		000100000000L
#define BUTTON_SHIFT		000200000000L
#define BUTTON_ALT		000400000000L
#define	ALL_MOUSE_EVENTS	000777777777L
#define	REPORT_MOUSE_POSITION	001000000000L

/* macros to extract single event-bits from masks */
#define	BUTTON_RELEASE(e, x)		((e) & (001 << (6 * ((x) - 1))))
#define	BUTTON_PRESS(e, x)		((e) & (002 << (6 * ((x) - 1))))
#define	BUTTON_CLICK(e, x)		((e) & (004 << (6 * ((x) - 1))))
#define	BUTTON_DOUBLE_CLICK(e, x)	((e) & (010 << (6 * ((x) - 1))))
#define	BUTTON_TRIPLE_CLICK(e, x)	((e) & (020 << (6 * ((x) - 1))))
#define	BUTTON_RESERVED_EVENT(e, x)	((e) & (040 << (6 * ((x) - 1))))

typedef unsigned long mmask_t;

typedef struct
{
    short id;		/* ID to distinguish multiple devices */
    int x, y, z;	/* event coordinates (character-cell) */
    mmask_t bstate;	/* button state bits */
}
MEVENT;

extern int getmouse(MEVENT *);
extern int ungetmouse(MEVENT *);
extern mmask_t mousemask(mmask_t, mmask_t *);
extern bool wenclose(const WINDOW *, int, int);
extern int mouseinterval(int);
extern bool wmouse_trafo(const WINDOW* win,int* y, int* x, bool to_screen);

#define mouse_trafo(y,x,to_screen) wmouse_trafo(stdscr,y,x,to_screen)

/* other non-XSI functions */

extern int mcprint(char *, int);	/* direct data to printer */
extern int has_key(int);		/* do we have given key? */

/* Debugging : use with libncurses_g.a */

extern void _tracef(const char *, ...) GCC_PRINTFLIKE(1,2);
extern void _tracedump(const char *, WINDOW *);
extern char *_traceattr(attr_t);
extern char *_traceattr2(int, chtype);
extern char *_nc_tracebits(void);
extern char *_tracechar(const unsigned char);
extern char *_tracechtype(chtype);
extern char *_tracechtype2(int, chtype);
extern char *_tracemouse(const MEVENT *);
extern void trace(const unsigned int);

/* trace masks */
#define TRACE_DISABLE	0x0000	/* turn off tracing */
#define TRACE_TIMES	0x0001	/* trace user and system times of updates */
#define TRACE_TPUTS	0x0002	/* trace tputs calls */
#define TRACE_UPDATE	0x0004	/* trace update actions, old & new screens */
#define TRACE_MOVE	0x0008	/* trace cursor moves and scrolls */
#define TRACE_CHARPUT	0x0010	/* trace all character outputs */
#define TRACE_ORDINARY	0x001F	/* trace all update actions */
#define TRACE_CALLS	0x0020	/* trace all curses calls */
#define TRACE_VIRTPUT	0x0040	/* trace virtual character puts */
#define TRACE_IEVENT	0x0080	/* trace low-level input processing */
#define TRACE_BITS	0x0100	/* trace state of TTY control bits */
#define TRACE_ICALLS	0x0200	/* trace internal/nested calls */
#define TRACE_CCALLS	0x0400	/* trace per-character calls */
#define TRACE_MAXIMUM	0xffff	/* maximum trace level */

#if defined(TRACE) || defined(NCURSES_TEST)
extern int _nc_optimize_enable;		/* enable optimizations */
extern const char *_nc_visbuf(const char *);
#define OPTIMIZE_MVCUR		0x01	/* cursor movement optimization */
#define OPTIMIZE_HASHMAP	0x02	/* diff hashing to detect scrolls */
#define OPTIMIZE_SCROLL		0x04	/* scroll optimization */
#define OPTIMIZE_ALL		0xff	/* enable all optimizations (dflt) */
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NCURSES_H */
