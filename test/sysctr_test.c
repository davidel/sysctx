/*
 *  LibSysCTr by Davide Libenzi ( System Call Tracing library )
 *  Copyright (C) 2004  Davide Libenzi
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <linux/unistd.h>
#include "sysctr.h"


static int open_scfunc(void *priv, trsyscall_t tsc) {
	int entry;
	unsigned long pid, param, rcode;
	char buf[512];

	systr_get_pid(tsc, &pid);
	entry = systr_is_entry(tsc);
	systr_get_param(tsc, SYSTR_PARAM_1, &param);
	if (!entry)
		systr_get_param(tsc, SYSTR_PARAM_RCODE, &rcode);
	buf[0] = 0;
	systr_pszmem_read(tsc, SYSTR_DATA_SECT, param, buf, sizeof(buf) - 1);

	fprintf(stderr, "[%lu] %s open(%s)", pid, entry ? "E": "X", buf);

	if (entry)
		fprintf(stderr, " = ?\n");
	else
		fprintf(stderr, " = %lu\n", rcode);

	return SYSTR_RET_CONTINUE;
}

static int close_scfunc(void *priv, trsyscall_t tsc) {
	int entry;
	unsigned long pid, param;

	systr_get_pid(tsc, &pid);
	entry = systr_is_entry(tsc);
	systr_get_param(tsc, SYSTR_PARAM_1, &param);

	fprintf(stderr, "[%lu] %s close(%d)\n", pid, entry ? "E": "X", param);

	return SYSTR_RET_CONTINUE;
}

static int exec_scfunc(void *priv, trsyscall_t tsc) {
	int entry;
	unsigned long pid, param;
	char buf[512];

	systr_get_pid(tsc, &pid);
	entry = systr_is_entry(tsc);
	systr_get_param(tsc, SYSTR_PARAM_1, &param);
	buf[0] = 0;
	if (entry)
		systr_pszmem_read(tsc, SYSTR_DATA_SECT, param, buf, sizeof(buf) - 1);

	fprintf(stderr, "[%lu] %s exec(%s)\n", pid, entry ? "E": "X", buf);

	return SYSTR_RET_CONTINUE;
}

static int fork_scfunc(void *priv, trsyscall_t tsc) {
	int entry;
	unsigned long pid, cpid;

	systr_get_pid(tsc, &pid);
	entry = systr_is_entry(tsc);
	if (entry)
		fprintf(stderr, "[%lu] E fork()\n", pid);
	else {
		systr_get_param(tsc, SYSTR_PARAM_RCODE, &cpid);
		fprintf(stderr, "[%lu] X fork() -> %lu\n", pid, cpid);
	}

	return SYSTR_RET_CONTINUE;
}

static int wait_scfunc(void *priv, trsyscall_t tsc) {
	int entry;
	unsigned long pid, res, wpid, options;

	systr_get_pid(tsc, &pid);
	systr_get_param(tsc, SYSTR_PARAM_1, &wpid);
	systr_get_param(tsc, SYSTR_PARAM_3, &options);
	entry = systr_is_entry(tsc);
	if (entry)
		fprintf(stderr, "[%lu] E wait(%ld, %lu)\n", pid, wpid, options);
	else {
		systr_get_param(tsc, SYSTR_PARAM_RCODE, &res);
		fprintf(stderr, "[%lu] X wait(%ld, %lu) = %ld\n", pid, wpid, options, res);
	}

	return SYSTR_RET_CONTINUE;
}

int main(int ac, char **av) {
	int i;

	if (systr_init_library() < 0)
		return 1;

	for (i = 1; i < ac; i++) {
		if (!strcmp(av[i], "--trace-exec"))
			systr_trace_syscall(__NR_execve, exec_scfunc, NULL);
		else if (!strcmp(av[i], "--trace-open"))
			systr_trace_syscall(__NR_open, open_scfunc, NULL);
		else if (!strcmp(av[i], "--trace-close"))
			systr_trace_syscall(__NR_close, close_scfunc, NULL);
		else if (!strcmp(av[i], "--trace-fork")) {
			systr_trace_syscall(__NR_fork, fork_scfunc, NULL);
			systr_trace_syscall(__NR_vfork, fork_scfunc, NULL);
			systr_trace_syscall(__NR_clone, fork_scfunc, NULL);
		} else if (!strcmp(av[i], "--trace-wait")) {
			systr_trace_syscall(__NR_waitpid, wait_scfunc, NULL);
			systr_trace_syscall(__NR_wait4, wait_scfunc, NULL);
		} else
			break;
	}

	systr_run(&av[i]);

	systr_cleanup_library();

	return 0;
}

