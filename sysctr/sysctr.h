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

