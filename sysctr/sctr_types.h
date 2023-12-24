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

