/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */
/*

acconfig.h - template used by autoheader to create config.h.in
config.h.in - used by autoconf to create config.h
config.h - created by autoconf; contains defines generated by autoconf

Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>

*/

#define RCSID(msg) \
static /**/const char *const rcsid[] = { (char *)rcsid, "\100(#)" msg }


/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define this to be the path of the xauth program. */
#define XAUTH_PATH "/usr/X11R6/bin/xauth"

/* This is defined if we found a lastlog file.  The presence of lastlog.h
   alone is not a sufficient indicator (at least newer BSD systems have
   lastlog but no lastlog.h. */
#define HAVE_LASTLOG 1

/* Define this if libutil.a contains BSD 4.4 compatible login(), logout(),
   and logwtmp() calls. */
#define HAVE_LIBUTIL_LOGIN 1

/* Location of system mail spool directory. */
#define MAIL_SPOOL_DIRECTORY "/var/mail"

/* Define this to use pipes instead of socketpairs for communicating with the
   client program.  Socketpairs do not seem to work on all systems. */
#define USE_PIPES 1

/* Define if you have the seteuid function.  */
#define HAVE_SETEUID 1

/* Define if you have the setlogin function.  */
#define HAVE_SETLOGIN 1
