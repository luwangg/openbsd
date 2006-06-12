/*	$OpenBSD: src/usr.bin/cvs/init.c,v 1.23 2006/06/12 13:56:00 xsa Exp $	*/
/*
 * Copyright (c) 2004 Jean-Francois Brousseau <jfb@openbsd.org>
 * Copyright (c) 2006 Xavier Santolaria <xsa@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"

#include "cvs.h"
#include "init.h"
#include "log.h"
#include "proto.h"

int	cvs_init(int, char **);
void	cvs_init_local(void);

static void init_mkdir(const char *, mode_t);
static void init_mkfile(char *, const char *const *);

struct cvsroot_file {
	char			*cf_path;
	const char *const	*cf_content;
};

static const struct cvsroot_file cvsroot_files[] = {
	{ CVS_PATH_CHECKOUTLIST,	NULL			},
	{ CVS_PATH_COMMITINFO,		NULL			},
	{ CVS_PATH_CONFIG,		config_contents		},
	{ CVS_PATH_CVSWRAPPERS,		NULL			},
	{ CVS_PATH_EDITINFO,		NULL			},
	{ CVS_PATH_HISTORY,		NULL			},
	{ CVS_PATH_LOGINFO,		NULL			},
	{ CVS_PATH_MODULES,		NULL			},
	{ CVS_PATH_NOTIFY_R,		NULL			},
	{ CVS_PATH_RCSINFO,		NULL			},
	{ CVS_PATH_TAGINFO,		NULL			},
	{ CVS_PATH_VALTAGS,		NULL			},
	{ CVS_PATH_VERIFYMSG,		NULL			}
};

static const char *cvsroot_dirs[2] = {
	CVS_PATH_ROOT, CVS_PATH_EMPTYDIR
};

#define INIT_NFILES	(sizeof(cvsroot_files)/sizeof(cvsroot_files[0]))
#define INIT_NDIRS	(sizeof(cvsroot_dirs)/sizeof(cvsroot_dirs[0]))

struct cvs_cmd cvs_cmd_init = {
	CVS_OP_INIT, CVS_REQ_INIT, "init",
	{ },
	"Create a CVS repository if it doesn't exist",
	"",
	"",
	NULL,
	cvs_init
};

int
cvs_init(int argc, char **argv)
{
	if (argc > 1)
		fatal("%s", cvs_cmd_init.cmd_synopsis);

	if (current_cvsroot->cr_method == CVS_METHOD_LOCAL)
		cvs_init_local();

	return (0);
}

void
cvs_init_local(void)
{
	u_int i;
	char *path;

	cvs_log(LP_TRACE, "cvs_init_local()");

	/* Create repository root directory if it does not already exist */
	init_mkdir(current_cvsroot->cr_dir, 0777);

	path = xmalloc(MAXPATHLEN);

	for (i = 0; i < INIT_NDIRS; i++) {
		if (cvs_path_cat(current_cvsroot->cr_dir,
		    cvsroot_dirs[i], path, MAXPATHLEN) >= MAXPATHLEN)
			fatal("cvs_init_local: truncation");

		init_mkdir(path, 0777);
	}

	for (i = 0; i < INIT_NFILES; i++) {
		if (cvs_path_cat(current_cvsroot->cr_dir,
		    cvsroot_files[i].cf_path, path, MAXPATHLEN) >= MAXPATHLEN)
			fatal("cvs_init_local: truncation");

		init_mkfile(path, cvsroot_files[i].cf_content);
	}

	xfree(path);
}

static void
init_mkdir(const char *path, mode_t mode)
{
	struct stat st;

	if (mkdir(path, mode) == -1) {
		if (!(errno == EEXIST ||
		    (errno == EACCES && (stat(path, &st) == 0) &&
		    S_ISDIR(st.st_mode)))) {
			fatal("init_mkdir: mkdir: `%s': %s",
			    path, strerror(errno));
		}
	}
}

static void
init_mkfile(char *path, const char *const *content)
{
	BUF *b;
	size_t len;
	int fd, openflags, rcsflags;
	char *d, *rpath;
	const char *const *p;
	RCSFILE *file;

	len = 0;
	fd = -1;
	d = NULL;
	openflags = O_WRONLY|O_CREAT|O_EXCL;
	rcsflags = RCS_RDWR|RCS_CREATE;

	if ((fd = open(path, openflags, 0444)) == -1)
		fatal("init_mkfile: open: `%s': %s", path, strerror(errno));

	if (content != NULL) {
		for (p = content; *p != NULL; ++p) {
			len = strlen(*p);
			b = cvs_buf_alloc(len, BUF_AUTOEXT);

			if (cvs_buf_append(b, *p, strlen(*p)) < 0)
				fatal("init_mkfile: cvs_buf_append");

			if (cvs_buf_write_fd(b, fd) < 0)
				fatal("init_mkfile: cvs_buf_write_fd");

			cvs_buf_free(b);
		}
	}

	/*
	 * Make sure history and val-tags files are world-writable.
	 * Every user should be able to write to them.
	 */
	if (strcmp(strrchr(CVS_PATH_HISTORY, '/'), strrchr(path, '/')) == 0 ||
	    strcmp(strrchr(CVS_PATH_VALTAGS, '/'), strrchr(path, '/')) == 0) {
		(void)fchmod(fd, 0666);
		goto out;
	}

	rpath = xstrdup(path);
	if (strlcat(rpath, RCS_FILE_EXT, MAXPATHLEN) >= MAXPATHLEN)
		fatal("init_mkfile: truncation");

	if ((file = rcs_open(rpath, fd, rcsflags, 0444)) == NULL)
		fatal("failed to create RCS file for `%s'", path);

	if ((b = cvs_buf_load(path, BUF_AUTOEXT)) == NULL)
		fatal("init_mkfile: failed to load %s", path);

	cvs_buf_putc(b, '\0');
	d = cvs_buf_release(b);

	if (rcs_rev_add(file, RCS_HEAD_REV, "initial checkin", -1, NULL) == -1)
		fatal("init_mkfile: failed to add new revision");

	if (rcs_deltatext_set(file, file->rf_head, d) == -1)
		fatal("init_mkfile: failed to set delta");

	file->rf_flags &= ~RCS_SYNCED;
	rcs_close(file);
	xfree(rpath);
out:
	(void)close(fd);

	if (d != NULL)
		xfree(d);
}
