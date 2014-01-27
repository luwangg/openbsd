/* $OpenBSD: src/usr.bin/tmux/cmd-kill-server.c,v 1.12 2014/01/27 23:57:35 nicm Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <signal.h>
#include <unistd.h>

#include "tmux.h"

/*
 * Kill the server and do nothing else.
 */

enum cmd_retval	 cmd_kill_server_exec(struct cmd *, struct cmd_q *);

const struct cmd_entry cmd_kill_server_entry = {
	"kill-server", NULL,
	"", 0, 0,
	"",
	0,
	NULL,
	cmd_kill_server_exec
};

const struct cmd_entry cmd_start_server_entry = {
	"start-server", "start",
	"", 0, 0,
	"",
	CMD_STARTSERVER,
	NULL,
	cmd_kill_server_exec
};

enum cmd_retval
cmd_kill_server_exec(struct cmd *self, unused struct cmd_q *cmdq)
{
	if (self->entry == &cmd_kill_server_entry)
		kill(getpid(), SIGTERM);

	return (CMD_RETURN_NORMAL);
}
