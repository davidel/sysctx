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


#include <ctype.h>
#include <sched.h>


#if defined(__NR_exit_group)
#define SYSTR_IS_SCALL_EXIT(n) ((n) == __NR_exit_group || (n) == __NR_exit)
#else
#define SYSTR_IS_SCALL_EXIT(n) ((n) == __NR_exit)
#endif


static int systr_get_tpgid(char const *sfname, pid_t *tgid, pid_t *ppid);



static int systr_get_tpgid(char const *sfname, pid_t *tgid, pid_t *ppid) {
	int fd, ssize;
	char *tmp;
	char buf[1024];

	if ((fd = open(sfname, O_RDONLY)) == -1)
		return -1;
	ssize = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (ssize <= 0)
		return -1;
	buf[ssize] = '\0';
	if (tgid) {
		if (!(tmp = strstr(buf, "Tgid:")))
			return -1;
		for (; *tmp && *tmp != '\n' && !isdigit(*tmp); tmp++);
		if (!isdigit(*tmp))
			return -1;
		*tgid = (pid_t) atoi(tmp);
	}
	if (ppid) {
		if (!(tmp = strstr(buf, "PPid:")))
			return -1;
		for (; *tmp && *tmp != '\n' && !isdigit(*tmp); tmp++);
		if (!isdigit(*tmp))
			return -1;
		*ppid = (pid_t) atoi(tmp);
	}

	return 0;
}

int systr_get_tgid_ppid(pid_t pid, pid_t *tgid, pid_t *ppid) {
	char fsname[128];

	sprintf(fsname, "/proc/%u/status", pid);
	if (systr_get_tpgid(fsname, tgid, ppid) < 0) {
		sprintf(fsname, "/proc/.%u/status", pid);
		if (systr_get_tpgid(fsname, tgid, ppid) < 0)
			return -1;
	}

	return 0;
}

int systr_enum_tgid_group(pid_t tgid, int (*eproc)(void *, pid_t), void *priv) {
	pid_t cpid, ctgid;
	DIR *dir;
	struct dirent *de;
	char fsname[128];

	sprintf(fsname, "/proc/%u/task", (unsigned int) tgid);
	if ((dir = opendir(fsname)) != NULL) {
		while ((de = readdir(dir)) != NULL) {
			if (de->d_fileno == 0 ||
			    !isdigit(de->d_name[0]))
				continue;
			cpid = (pid_t) atoi(de->d_name);
			if (cpid > 0)
				(*eproc)(priv, cpid);
		}
		closedir(dir);
	} else if ((dir = opendir("/proc")) != NULL) {
		while ((de = readdir(dir)) != NULL) {
			if (de->d_name[0] != '.' || !isdigit(de->d_name[1]))
				continue;
			cpid = (pid_t) atoi(de->d_name + 1);
			sprintf(fsname, "/proc/%s/status", de->d_name);
			if (systr_get_tpgid(fsname, &ctgid, NULL) == 0 && tgid == ctgid)
				(*eproc)(priv, cpid);
		}
		closedir(dir);
	}

	return 0;
}

int systr_trace_trap(pidhash_t *ph) {
	unsigned long param;
	struct user_regs_struct ur;

	if (SYSTR_IS_SCALL_EXIT(ph->syscall))
		ph->flags |= SYSTR_PHF_ONEXIT;

#if !defined(SYSTR_HAS_FORK_TRACE)
	if (systr_sysc_enter(ph)) {
		if (ph->syscall == __NR_fork || ph->syscall == __NR_vfork) {
			ur = ph->resur = ph->ur;
			SYSTR_SET_SCALLN(ph->pid, &ur, __NR_clone);
			SYSTR_SET_PAR1(ph->pid, &ur, CLONE_PTRACE | SIGCHLD);
			SYSTR_SET_PAR2(ph->pid, &ur, 0);
			if (ptrace(PTRACE_SETREGS, ph->pid, NULL, &ur))
				return -1;
			ph->resmask = SYSTR_RESF_SCALLN | SYSTR_RESF_PARAM1 | SYSTR_RESF_PARAM2;
		} else if (ph->syscall == __NR_clone) {
			ur = ph->resur = ph->ur;
			param = SYSTR_GET_PAR1(ph->pid, &ur);
			param |= CLONE_PTRACE;
			SYSTR_SET_PAR1(ph->pid, &ur, param);
			if (ptrace(PTRACE_SETREGS, ph->pid, NULL, &ur))
				return -1;
			ph->resmask = SYSTR_RESF_PARAM1;
		}
	}
#endif

	return 0;
}

int systr_waitpid(pid_t wpid, pid_t *pid, int *status) {

	while ((*pid = (pid_t) waitpid(wpid, status, __WALL)) == (pid_t) -1 &&
	       errno == EINTR);

	return *pid > 0 ? 0: -1;
}

