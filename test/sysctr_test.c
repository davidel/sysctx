/*    Copyright 2023 Davide Libenzi
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

