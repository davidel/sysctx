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
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <linux/unistd.h>
#include "sysctr.h"



static int exsize;
static char exbuffer[200];


static int log_exec(unsigned long pid, char const *prg) {
	int len;
	char *ptr, *top, *next;
	char lbuf[1024];

	len = snprintf(lbuf, sizeof(lbuf) - 1, "[%lu] exec(%s)\n", pid, prg);
	if (exsize + len + 1 >= sizeof(exbuffer)) {
		openlog("exlogger", 0, LOG_USER);
		for (ptr = exbuffer, top = exbuffer + exsize; ptr < top;) {
			if (!(next = memchr(ptr, '\n', top - ptr)))
				break;
			*next++ = 0;
			syslog(LOG_INFO, "%s\n", ptr);
			ptr = next;
		}
		closelog();
		exsize = 0;
	}
	memcpy(exbuffer + exsize, lbuf, len + 1);
	exsize += len;
	return 0;
}

static int exec_scfunc(void *priv, trsyscall_t tsc) {
	unsigned long pid, param;
	char buf[512];

	systr_get_pid(tsc, &pid);
	systr_get_param(tsc, SYSTR_PARAM_1, &param);
	systr_pszmem_read(tsc, SYSTR_DATA_SECT, param, buf, sizeof(buf) - 1);
	if (systr_is_entry(tsc))
		log_exec(pid, buf);

	return SYSTR_RET_CONTINUE;
}

int main(int ac, char **av) {

	exsize = 0;

	systr_init_library();
	systr_trace_syscall(__NR_execve, exec_scfunc, NULL);
	systr_run(&av[1]);
	systr_cleanup_library();

	return 0;
}

