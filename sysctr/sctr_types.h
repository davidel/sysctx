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

#if !defined(SCTR_TYPES_H)
#define SCTR_TYPES_H


#define SYSTR_PHF_NEW (1 << 0)
#define SYSTR_PHF_ATTACHED (1 << 1)
#define SYSTR_PHF_ZOMBIE (1 << 2)
#define SYSTR_PHF_SUSPENDED (1 << 3)
#define SYSTR_PHF_THREAD (1 << 4)
#define SYSTR_PHF_ONEXIT (1 << 5)

#define SYSTR_RESF_PARAM1 (1 << 0)
#define SYSTR_RESF_PARAM2 (1 << 1)
#define SYSTR_RESF_PARAM3 (1 << 2)
#define SYSTR_RESF_PARAM4 (1 << 3)
#define SYSTR_RESF_PARAM5 (1 << 4)
#define SYSTR_RESF_PARAM6 (1 << 5)
#define SYSTR_RESF_SCALLN (1 << 6)
#define SYSTR_RESF_RCODE (1 << 7)


typedef struct s_shash {
	int size;
	llist_t *bucks;
} shash_t;

typedef struct s_pidwait {
	llist_t lnk;
	struct s_pidhash *ph;
	pid_t pidwait, pgidwait;
	struct s_pidhash *wcph;
} pidwait_t;

typedef struct s_pidhash {
	llist_t lnk;
	llist_t clnk;
	struct s_pidhash *parent;
	llist_t childs;
	llist_t zombs;
	llist_t wlist;
	pidwait_t pw;
	int cstatus, estatus;
	unsigned long long sigmask;
	int psyscall, syscall;
	pid_t pid;
	unsigned long flags;
	struct user_regs_struct ur, resur;
	unsigned long resmask, sresmask;
} pidhash_t;

typedef struct s_scallhash {
	llist_t lnk;
	int syscall;
	pidhash_t *ph;
	int (*scfunc)(void *, trsyscall_t);
	void *priv;
} scallhash_t;



#endif

