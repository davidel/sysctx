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

#if !defined(SYSCTR_H)
#define SYSCTR_H


#define SYSTR_RET_CONTINUE 0
#define SYSTR_RET_DETACH 1

#define SYSTR_SYSCALL 1
#define SYSTR_PARAM_1 2
#define SYSTR_PARAM_2 3
#define SYSTR_PARAM_3 4
#define SYSTR_PARAM_4 5
#define SYSTR_PARAM_5 6
#define SYSTR_PARAM_6 7
#define SYSTR_PARAM_RCODE 8
#define SYSTR_REG_SP 9
#define SYSTR_REG_IP 10

#define SYSTR_DATA_SECT 1
#define SYSTR_TEXT_SECT 2

typedef struct s_trsyscall {} *trsyscall_t;


int systr_init_library(void);
void systr_cleanup_library(void);
int systr_run(char **av);
int systr_stop(void);
int systr_trace_syscall(int syscall, int (*scfunc)(void *, trsyscall_t),
			void *priv);
int systr_untrace_syscall(int syscall);
int systr_get_pid(trsyscall_t tsc, unsigned long *pid);
int systr_get_param(trsyscall_t tsc, int param, unsigned long *pparam);
int systr_set_params(trsyscall_t tsc, ...);
int systr_is_entry(trsyscall_t tsc);
int systr_pmem_read(trsyscall_t tsc, int where, unsigned long addr, char *buf, int size);
int systr_pmem_write(trsyscall_t tsc, int where, unsigned long addr, const char *buf, int size);
int systr_pszmem_read(trsyscall_t tsc, int where, unsigned long addr, char *buf, int bmax);



#endif

