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

#include "sctr_include.h"


#define SYSTR_PIDHASH_SIZE 5381
#define SYSTR_SCALLHASH_SIZE 293



static scallhash_t *systr_scall_add(int syscall, int (*scfunc)(void *, trsyscall_t),
				    void *priv);
static void systr_scall_release(scallhash_t *scall);
static void systr_scallhash_free(llist_t *pos);
static scallhash_t *systr_scall_get(int syscall);
static void systr_pidhash_zombie_release(pidhash_t *ph);
static int systr_detach(pid_t pid, int psig);
static pidhash_t *systr_zombie_pick(pidhash_t *ph, pid_t pid, pid_t pgid);
static void systr_phash_free(llist_t *pos);
static int systr_grpenum_proc(void *priv, pid_t pid);
static int systr_spill_sig(unsigned long long *sigmask);
static int systr_handle_fork(pidhash_t *ph);
static int systr_can_wait(pidhash_t *ph, pid_t wpid, pid_t pgid);
static int systr_handle_wait(pidhash_t *ph);
static int systr_trace(pid_t tpid);
static int systr_setup_parent_signals(void);
static int systr_setup_child_signals(void);
static int systr_exec_process(char **av);


int systr_dbglev = 4;
static volatile int sstop;
static shash_t hpid;
static shash_t hscall;



static scallhash_t *systr_scall_add(int syscall, int (*scfunc)(void *, trsyscall_t),
				    void *priv) {
	scallhash_t *scall;
	llist_t *head;

	head = systr_hash_bucket(&hscall, syscall);
	if (!(scall = (scallhash_t *) malloc(sizeof(scallhash_t))))
		return NULL;
	scall->syscall = syscall;
	scall->scfunc = scfunc;
	scall->priv = priv;
	scall->ph = NULL;
	LLIST_ADDT(&scall->lnk, head);

	return scall;
}

static void systr_scall_release(scallhash_t *scall) {

	if (!LLIST_EMPTY(&scall->lnk))
		LLIST_DEL_INIT(&scall->lnk);
	free(scall);
}

static void systr_scallhash_free(llist_t *pos) {
	scallhash_t *scall;

	scall = LLIST_ITEM(pos, scallhash_t, lnk);
	systr_scall_release(scall);
}

static scallhash_t *systr_scall_get(int syscall) {
	llist_t *head, *pos;
	scallhash_t *scall;

	head = systr_hash_bucket(&hscall, syscall);
	for (pos = LLIST_FIRST(head); pos; pos = LLIST_NEXT(pos, head)) {
		scall = LLIST_ITEM(pos, scallhash_t, lnk);
		if (scall->syscall == syscall)
			return scall;
	}

	return NULL;
}

pidhash_t *systr_pidhash_get(pid_t pid) {
	llist_t *head, *pos;
	pidhash_t *ph;

	head = systr_hash_bucket(&hpid, pid);
	for (pos = LLIST_FIRST(head); pos; pos = LLIST_NEXT(pos, head)) {
		ph = LLIST_ITEM(pos, pidhash_t, lnk);
		if (ph->pid == pid)
			return ph;
	}
	return NULL;
}

pidhash_t *systr_pidhash_add(pid_t pid, struct user_regs_struct const *ur) {
	llist_t *head;
	pidhash_t *ph;

	head = systr_hash_bucket(&hpid, pid);
	if (!(ph = (pidhash_t *) malloc(sizeof(pidhash_t))))
		return NULL;
	ph->pid = pid;
	if (ur)
		ph->ur = *ur;
	else
		memset(&ph->ur, 0, sizeof(ph->ur));
	LLIST_INIT(&ph->clnk);
	LLIST_INIT(&ph->childs);
	LLIST_INIT(&ph->zombs);
	LLIST_INIT(&ph->wlist);
	LLIST_INIT(&ph->pw.lnk);
	ph->parent = NULL;
	ph->estatus = ph->cstatus = -1;
	ph->sigmask = 0;
	ph->flags = SYSTR_PHF_NEW;
	ph->resmask = 0;
	ph->psyscall = ph->syscall = -1;
	LLIST_ADDT(&ph->lnk, head);

	return ph;
}

static void systr_pidhash_zombie_release(pidhash_t *ph) {

	if (!LLIST_EMPTY(&ph->clnk))
		LLIST_DEL_INIT(&ph->clnk);
	free(ph);
}

static int systr_detach(pid_t pid, int psig) {
	int status;
	pid_t wpid;

	if (ptrace(PTRACE_DETACH, pid, NULL, (void *) psig)) {
		if (kill(pid, 0) < 0) {
			if (errno != ESRCH) {
				DBGPRINT(2, "[%d] error: ptrace(PTRACE_DETACH, %d, %d)\n",
					 pid, pid, psig);
				return -1;
			}
		} else if (kill(pid, SIGSTOP) < 0) {
			DBGPRINT(2, "[%d] error: ptrace(PTRACE_DETACH, %d)\n",
				 pid, pid);
			return -1;
		} else {
			for (;;) {
				if (systr_waitpid(pid, &wpid, &status) < 0) {
					if (errno == ECHILD || errno != EINVAL)
						break;
				}
			}
			if (WIFSTOPPED(status)) {
				psig = WSTOPSIG(status);
				if (psig == SIGSTOP) {
					if (ptrace(PTRACE_DETACH, pid, NULL, NULL)) {
						DBGPRINT(2, "[%d] error: ptrace(PTRACE_DETACH, %d)\n",
							 pid, pid);
						return -1;
					}
				} else if (ptrace(PTRACE_CONT, pid, NULL,
						  psig == SIGTRAP ? NULL: (void *) psig)) {
					DBGPRINT(2, "[%d] error: ptrace(PTRACE_CONT, %d, %d)\n",
						 pid, pid, psig == SIGTRAP ? 0: psig);
					return -1;
				}
			}
		}
	}

	return 0;
}

void systr_pidhash_release(pidhash_t *ph) {
	pid_t pgid;
	llist_t *head, *pos, *next;
	pidhash_t *cph, *pph, *wph;
	pidwait_t *pw;

	ph->estatus = ph->cstatus;
	head = &ph->zombs;
	for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
		cph = LLIST_ITEM(pos, pidhash_t, clnk);
		next = LLIST_NEXT(pos, head);
		systr_pidhash_zombie_release(cph);
	}
	head = &ph->childs;
	for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
		cph = LLIST_ITEM(pos, pidhash_t, clnk);
		next = LLIST_NEXT(pos, head);
		systr_reparent(cph, NULL);
	}
	if (!LLIST_EMPTY(&ph->lnk))
		LLIST_DEL_INIT(&ph->lnk);
	if (!LLIST_EMPTY(&ph->clnk))
		LLIST_DEL_INIT(&ph->clnk);
	if (ph->flags & SYSTR_PHF_ATTACHED)
		systr_detach(ph->pid, systr_spill_sig(&ph->sigmask));
	if ((ph->flags & SYSTR_PHF_ONEXIT) && (pph = ph->parent) != NULL) {
		ph->flags &= ~SYSTR_PHF_ATTACHED;
		ph->flags |= SYSTR_PHF_ZOMBIE;
		LLIST_ADDT(&ph->clnk, &pph->zombs);

		pgid = getpgid(ph->pid);
		head = &pph->wlist;
		for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
			pw = LLIST_ITEM(pos, pidwait_t, lnk);
			next = LLIST_NEXT(pos, head);
			wph = pw->ph;

			if ((wph->pw.pidwait < 0 || ph->pid == wph->pw.pidwait ||
			     (!wph->pw.pidwait && pgid == wph->pw.pgidwait))) {
				SYSTR_SET_RCODE(wph->pid, &wph->resur, ph->pid);
				LLIST_DEL_INIT(pos);
				LLIST_DEL_INIT(&ph->clnk);
				wph->pw.wcph = ph;
				systr_task_resume(wph);
				DBGPRINT(5, "[%d] resume %d\n", ph->pid, wph->pid);
				break;
			}
		}
		for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
			pw = LLIST_ITEM(pos, pidwait_t, lnk);
			next = LLIST_NEXT(pos, head);
			wph = pw->ph;

			if (!systr_can_wait(wph, wph->pw.pidwait, wph->pw.pgidwait)) {
				LLIST_DEL_INIT(pos);
				systr_task_resume(wph);
				DBGPRINT(5, "[%d] resume %d (ECHILD)\n", ph->pid, wph->pid);
			}
		}
	} else
		free(ph);
}

int systr_reparent(pidhash_t *ph, pidhash_t *pph) {

	if (!LLIST_EMPTY(&ph->clnk))
		LLIST_DEL_INIT(&ph->clnk);
	ph->parent = pph;
	if (pph)
		LLIST_ADDT(&ph->clnk, &pph->childs);

	return 0;
}

static pidhash_t *systr_zombie_pick(pidhash_t *ph, pid_t pid, pid_t pgid) {
	llist_t *head, *pos, *next;
	pidhash_t *zph;

	head = &ph->zombs;
	for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
		zph = LLIST_ITEM(pos, pidhash_t, clnk);
		next = LLIST_NEXT(pos, head);
		if (pid < 0 || zph->pid == pid)
			return zph;
		if (!pid && getpgid(zph->pid) == pgid)
			return zph;
	}
	if (ph->parent && (ph->flags & SYSTR_PHF_THREAD)) {
		head = &ph->parent->zombs;
		for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
			zph = LLIST_ITEM(pos, pidhash_t, clnk);
			next = LLIST_NEXT(pos, head);
			if (pid < 0 || zph->pid == pid)
				return zph;
			if (!pid && getpgid(zph->pid) == pgid)
				return zph;
		}
	}

	return NULL;
}

static void systr_phash_free(llist_t *pos) {
	pidhash_t *ph;

	ph = LLIST_ITEM(pos, pidhash_t, lnk);
	systr_pidhash_release(ph);
}

static int systr_grpenum_proc(void *priv, pid_t pid) {
	pidhash_t *ph = (pidhash_t *) priv, *gph;

	if (ph->pid != pid && (gph = systr_pidhash_get(pid)) != NULL) {
		if (ph->flags & SYSTR_PHF_ONEXIT)
			gph->flags |= SYSTR_PHF_ONEXIT;
		systr_pidhash_release(gph);
	}

	return 0;
}

int systr_exit_group(pidhash_t *ph) {
	pid_t tgid;

	if (systr_get_tgid_ppid(ph->pid, &tgid, NULL) == 0 &&
	    tgid == ph->pid)
		systr_enum_tgid_group(tgid, systr_grpenum_proc, ph);
	systr_pidhash_release(ph);
	return 0;
}

int systr_sysc_enter(pidhash_t *ph) {

	return ph->psyscall > 0 ? ph->psyscall: 0;
}

static int systr_spill_sig(unsigned long long *sigmask) {
	int i;
	unsigned long long sigs;

	for (i = 0, sigs = *sigmask; sigs; i++, sigs >>= 1)
		if (sigs & 1) {
			*sigmask &= ~(1ULL << i);
			return i;
		}

	return 0;
}

int systr_task_resume(pidhash_t *ph) {
	int psig;

	if (ph->flags & SYSTR_PHF_SUSPENDED) {
		psig = systr_spill_sig(&ph->sigmask);
		if (ptrace(PTRACE_SYSCALL, ph->pid, NULL, (void *) psig))
			DBGPRINT(2, "[%d] error: ptrace(PTRACE_SYSCALL, %d, %d)\n",
				 ph->pid, ph->pid, psig);
		ph->flags &= ~SYSTR_PHF_SUSPENDED;
	}

	return 0;
}

int systr_adjust_parent(pidhash_t *ph, pidhash_t *pph) {
	pid_t ppid, tgid;

	if (systr_get_tgid_ppid(ph->pid, &tgid, &ppid) < 0)
		return -1;
	if (tgid == ph->pid) {
		if (!pph && !(pph = systr_pidhash_get(ppid))) {
			DBGPRINT(1, "unable to get parent (%d)\n", ppid);
			return -1;
		}
	} else {
		if (!(pph = systr_pidhash_get(tgid))) {
			DBGPRINT(1, "unable to get thread group leader (%d)\n", tgid);
			return -1;
		}
		ph->flags |= SYSTR_PHF_THREAD;
		DBGPRINT(7, "[%d] is thread: tgid=%d\n", ph->pid, tgid);
	}
	systr_reparent(ph, pph);

	return 0;
}

static int systr_handle_fork(pidhash_t *ph) {
	pid_t cpid;
	pidhash_t *cph;

	if (!systr_sysc_enter(ph)) {
		if ((cpid = (pid_t) SYSTR_GET_RCODE(ph->pid, &ph->ur)) <= 0)
			return 0;
		if ((cph = systr_pidhash_get(cpid)) != NULL) {

		} else {
			if (!(cph = systr_pidhash_add(cpid, NULL)))
				return -1;
			cph->resur = ph->resur;
			cph->resmask = ph->sresmask;

		}
		systr_adjust_parent(cph, ph);
	}

	return 0;
}

static int systr_can_wait(pidhash_t *ph, pid_t wpid, pid_t pgid) {
	llist_t *head, *pos, *next;
	pidhash_t *cph, *pph;

	if (wpid > 0) {
		if (!(cph = systr_pidhash_get(wpid)))
			return 0;
		if (cph->parent && (ph->flags & cph->flags & SYSTR_PHF_THREAD))
			return cph->parent == ph->parent;

		return cph->parent == ph;
	}
	pph = ph->parent && (ph->flags & SYSTR_PHF_THREAD) ? ph->parent: ph;
	head = &pph->childs;
	if (wpid < 0)
		return !LLIST_EMPTY(head);
	for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
		cph = LLIST_ITEM(pos, pidhash_t, clnk);
		next = LLIST_NEXT(pos, head);
		if (getpgid(cph->pid) == pgid)
			return 1;
	}

	return 0;
}

static int systr_handle_wait(pidhash_t *ph) {
	pid_t wpid, ewpid, ewpgid;
	unsigned long saddr, options;
	pidhash_t *cph;

	if (systr_sysc_enter(ph)) {
		wpid = (pid_t) SYSTR_GET_PAR1(ph->pid, &ph->ur);
		options = (unsigned long) SYSTR_GET_PAR3(ph->pid, &ph->ur);

		ewpid = wpid;
		ewpgid = 0;
		if (!ewpid)
			ewpgid = getpgid(ph->pid);
		else if (ewpid < -1) {
			ewpid = 0;
			ewpgid = -wpid;
		}
		cph = systr_zombie_pick(ph, ewpid, ewpgid);
		ph->resur = ph->ur;
		if (cph) {
			ph->pw.wcph = cph;
			SYSTR_SET_RCODE(ph->pid, &ph->resur, cph->pid);
			ph->resmask = SYSTR_RESF_RCODE;
		} else if (options & WNOHANG) {
			ph->pw.wcph = NULL;
			SYSTR_SET_RCODE(ph->pid, &ph->resur, 0);
			ph->resmask = SYSTR_RESF_RCODE;
		} else {
			ph->pw.wcph = NULL;
			SYSTR_SET_RCODE(ph->pid, &ph->resur, -ECHILD);
			ph->resmask = SYSTR_RESF_RCODE;
			if (systr_can_wait(ph, ewpid, ewpgid)) {
				ph->pw.pidwait = ewpid;
				ph->pw.pgidwait = ewpgid;
				ph->pw.ph = ph;
				if (ph->parent && (ph->flags & SYSTR_PHF_THREAD))
					LLIST_ADDT(&ph->pw.lnk, &ph->parent->wlist);
				else
					LLIST_ADDT(&ph->pw.lnk, &ph->wlist);
				ph->flags |= SYSTR_PHF_SUSPENDED;
			}
		}
	} else {
		saddr = (unsigned long) SYSTR_GET_PAR2(ph->pid, &ph->ur);
		if (saddr && ph->pw.wcph && ptrace(PTRACE_POKEDATA, ph->pid, (void *) saddr,
						   (void *) ph->pw.wcph->estatus))
			DBGPRINT(2, "[%d] error: ptrace(PTRACE_POKEDATA, %d, %x)\n",
				 ph->pid, ph->pid, saddr);
		if (ph->pw.wcph) {
			systr_pidhash_zombie_release(ph->pw.wcph);
			ph->pw.wcph = NULL;
		}
	}

	return 0;
}

static int systr_trace(pid_t tpid) {
	int status, psig, syscall, trcret;
	pid_t pid, cpid;
	pidhash_t *ph, *cph;
	scallhash_t *scall;
	struct user_regs_struct ur;

	if (!(ph = systr_pidhash_add(tpid, NULL)))
		return -1;

	while (!sstop && systr_waitpid(-1, &pid, &status) == 0) {
		if (WIFSIGNALED(status) || WIFEXITED(status)) {
			DBGPRINT(6, "[%d] died (%d)\n", pid,
				 WIFSIGNALED(status) ? WTERMSIG(status): 0);
			if ((ph = systr_pidhash_get(pid)) != NULL) {
				ph->cstatus = status;
				ph->flags |= SYSTR_PHF_ONEXIT;
				systr_exit_group(ph);
			}
			continue;
		}
		if (!WIFSTOPPED(status)) {
			DBGPRINT(2, "[%d] Died (WTF?)\n", pid);
			if ((ph = systr_pidhash_get(pid)) != NULL) {
				ph->cstatus = status;
				ph->flags |= SYSTR_PHF_ONEXIT;
				systr_exit_group(ph);
			}
			continue;
		}
		psig = WSTOPSIG(status);
		if (ptrace(PTRACE_GETREGS, pid, NULL, &ur)) {
			DBGPRINT(2, "[%d] error: ptrace(PTRACE_GETREGS, %d)\n",
				 pid, pid);
			continue;
		}
		if (!(ph = systr_pidhash_get(pid))) {
			DBGPRINT(4, "[%d] unknown pid %u (added)\n", pid, pid);
			if (!(ph = systr_pidhash_add(pid, &ur))) {
				systr_detach(pid, psig);
				continue;
			}
			SYSTR_SET_TRACE_OPTIONS(pid);
			ph->cstatus = status;
			ph->flags &= ~SYSTR_PHF_NEW;
			ph->flags |= SYSTR_PHF_ATTACHED;
			systr_adjust_parent(ph, NULL);
			goto ack_syscall;
		}
		ph->flags &= ~SYSTR_PHF_SUSPENDED;
		ph->ur = ur;
		if (ph->resmask)
			systr_ctx_restore(ph);
		syscall = SYSTR_GET_SCALLN(pid, &ph->ur);
		ph->cstatus = status;
		if (ph->psyscall > 0 && ph->psyscall != syscall)
			ph->psyscall = -1;
		ph->syscall = syscall;
		ph->psyscall = ph->psyscall < 0 ? syscall: -1;
		DBGPRINT(9, "[%d] SCALLN=%d  PSIG=%d\n", pid, syscall, psig);
		if (ph->flags & SYSTR_PHF_NEW) {
			DBGPRINT(8, "[%d] new pid %u\n", pid, pid);
			SYSTR_SET_TRACE_OPTIONS(pid);
			ph->psyscall = -1;
			ph->flags &= ~SYSTR_PHF_NEW;
			ph->flags |= SYSTR_PHF_ATTACHED;
			if (psig != SIGTRAP)
				ph->sigmask |= 1ULL << psig;
			goto ack_syscall;
		}
		if (psig != SIGTRAP) {
			DBGPRINT(6, "[%d] signal received = %d\n", pid, psig);
			ph->sigmask |= 1ULL << psig;
		}
		if (syscall >= 0 &&
		    (scall = systr_scall_get(syscall)) != NULL) {
			scall->ph = ph;

			if ((*scall->scfunc)(scall->priv,
					     (trsyscall_t) scall) == SYSTR_RET_DETACH) {
				systr_exit_group(ph);
				continue;
			}
		}
		if ((trcret = systr_trace_trap(ph)) < 0)
			return -1;
		else if (trcret > 0)
			continue;
		if (SYSTR_IS_FORK(syscall) && systr_handle_fork(ph) < 0)
			return -1;
		if (SYSTR_IS_WAIT(syscall) && systr_handle_wait(ph) < 0)
			return -1;
	ack_syscall:
		if (!(ph->flags & SYSTR_PHF_SUSPENDED)) {
			psig = systr_spill_sig(&ph->sigmask);
			if (ptrace(PTRACE_SYSCALL, pid, NULL, (void *) psig))
				DBGPRINT(2, "[%d] error: ptrace(PTRACE_SYSCALL, %d, %d)\n",
					 pid, pid, psig);
		}
	}

	return 0;
}

static int systr_setup_parent_signals(void) {
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGTTOU, &sa, NULL);
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	sa.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sa, NULL);

	return 0;
}

static int systr_setup_child_signals(void) {


	return 0;
}

static int systr_exec_process(char **av) {
	pid_t pid;

	pid = fork();
	if (pid == 0) {
		systr_setup_child_signals();

		if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
			DBGPRINT(2, "[%d] error: ptrace(PTRACE_TRACEME)\n",
				 getpid());
			exit(errno);
		}

		if (execvp(av[0], av))
			perror(av[0]);

		exit(errno);
	} else if (pid < 0) {
		DBGPRINT(2, "[%d] fork error\n", getpid());
		return -1;
	}

	systr_setup_parent_signals();

	return systr_trace(pid);
}

int systr_init_library(void) {

	if (systr_hash_init(&hpid, SYSTR_PIDHASH_SIZE) < 0)
		return -1;
	if (systr_hash_init(&hscall, SYSTR_SCALLHASH_SIZE) < 0) {
		systr_hash_free(&hpid, systr_phash_free);
		return -1;
	}

	return 0;
}

void systr_cleanup_library(void) {

	systr_hash_free(&hpid, systr_phash_free);
	systr_hash_free(&hscall, systr_scallhash_free);
}

int systr_run(char **av) {

	sstop = 0;

	return systr_exec_process(av);
}

int systr_stop(void) {

	sstop++;

	return 0;
}

int systr_trace_syscall(int syscall, int (*scfunc)(void *, trsyscall_t),
			void *priv) {
	scallhash_t *scall;

	if (!(scall = systr_scall_add(syscall, scfunc, priv)))
		return -1;

	return 0;
}

int systr_untrace_syscall(int syscall) {
	scallhash_t *scall;

	if (!(scall = systr_scall_get(syscall)))
		return -1;
	systr_scall_release(scall);

	return 0;
}

int systr_get_pid(trsyscall_t tsc, unsigned long *pid) {
	scallhash_t *scall = (scallhash_t *) tsc;

	*pid = (unsigned long) scall->ph->pid;

	return 0;
}

int systr_get_param(trsyscall_t tsc, int param, unsigned long *pparam) {
	scallhash_t *scall = (scallhash_t *) tsc;

	switch (param) {
	case SYSTR_SYSCALL:
		*pparam = (unsigned long) SYSTR_GET_SCALLN(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_PARAM_1:
		*pparam = (unsigned long) SYSTR_GET_PAR1(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_PARAM_2:
		*pparam = (unsigned long) SYSTR_GET_PAR2(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_PARAM_3:
		*pparam = (unsigned long) SYSTR_GET_PAR3(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_PARAM_4:
		*pparam = (unsigned long) SYSTR_GET_PAR4(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_PARAM_5:
		*pparam = (unsigned long) SYSTR_GET_PAR5(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_PARAM_6:
		*pparam = (unsigned long) SYSTR_GET_PAR6(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_PARAM_RCODE:
		*pparam = (unsigned long) SYSTR_GET_RCODE(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_REG_SP:
		*pparam = (unsigned long) SYSTR_GET_SP(scall->ph->pid, &scall->ph->ur);
		break;
	case SYSTR_REG_IP:
		*pparam = (unsigned long) SYSTR_GET_IP(scall->ph->pid, &scall->ph->ur);
		break;
	default:
		return -1;
	}

	return 0;
}

int systr_set_param(trsyscall_t tsc, ...) {
	int param;
	unsigned long value;
	scallhash_t *scall = (scallhash_t *) tsc;
	struct user_regs_struct ur;
	va_list ap;

	ur = scall->ph->ur;
	va_start(ap, tsc);
	while ((param = va_arg(ap, int)) > 0) {
		value = va_arg(ap, unsigned long);
		switch (param) {
		case SYSTR_SYSCALL:
			SYSTR_SET_SCALLN(scall->ph->pid, &ur, value);
			break;
		case SYSTR_PARAM_1:
			SYSTR_SET_PAR1(scall->ph->pid, &ur, value);
			break;
		case SYSTR_PARAM_2:
			SYSTR_SET_PAR2(scall->ph->pid, &ur, value);
			break;
		case SYSTR_PARAM_3:
			SYSTR_SET_PAR3(scall->ph->pid, &ur, value);
			break;
		case SYSTR_PARAM_4:
			SYSTR_SET_PAR4(scall->ph->pid, &ur, value);
			break;
		case SYSTR_PARAM_5:
			SYSTR_SET_PAR5(scall->ph->pid, &ur, value);
			break;
		case SYSTR_PARAM_6:
			SYSTR_SET_PAR6(scall->ph->pid, &ur, value);
			break;
		case SYSTR_PARAM_RCODE:
			SYSTR_SET_RCODE(scall->ph->pid, &ur, value);
			break;
		case SYSTR_REG_SP:
			SYSTR_SET_SP(scall->ph->pid, &ur, value);
			break;
		case SYSTR_REG_IP:
			SYSTR_SET_IP(scall->ph->pid, &ur, value);
			break;
		default:
			va_end(ap);
			return -1;
		}
	}
	va_end(ap);

	if (ptrace(PTRACE_SETREGS, scall->ph->pid, NULL, &ur)) {
		DBGPRINT(2, "[%d] error: ptrace(PTRACE_SETREGS, %d)\n",
			 scall->ph->pid, scall->ph->pid);
		return -1;
	}

	scall->ph->ur = ur;

	return 0;
}

int systr_is_entry(trsyscall_t tsc) {
	scallhash_t *scall = (scallhash_t *) tsc;

	return systr_sysc_enter(scall->ph);
}

int systr_pmem_read(trsyscall_t tsc, int where, unsigned long addr, char *buf, int size) {
	scallhash_t *scall = (scallhash_t *) tsc;

	return systr_procmem_read(scall->ph, where, (long) addr, buf, size);
}

int systr_pmem_write(trsyscall_t tsc, int where, unsigned long addr, const char *buf, int size) {
	scallhash_t *scall = (scallhash_t *) tsc;

	return systr_procmem_write(scall->ph, where, (long) addr, buf, size);
}

int systr_pszmem_read(trsyscall_t tsc, int where, unsigned long addr, char *buf, int bmax) {
	scallhash_t *scall = (scallhash_t *) tsc;

	return systr_procszmem_read(scall->ph, where, (long) addr, buf, bmax);
}

