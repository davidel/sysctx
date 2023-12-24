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


#if !defined(SCTR_LINUX_H)
#define SCTR_LINUX_H


#include <linux/user.h>
#include <linux/unistd.h>


#define SYSTR_SYS_LINUX 1

#define SYSTR_NULL_SYSCALL __NR_getpid
#define SYSTR_IS_FORK(n) ((n) == __NR_fork || (n) == __NR_vfork || (n) == __NR_clone)
#define SYSTR_IS_WAIT(n) ((n) == __NR_waitpid || (n) == __NR_wait4)

#if (defined(PTRACE_SETOPTIONS) && defined(PTRACE_O_TRACEFORK) && \
	defined(PTRACE_O_TRACEVFORK))

#define SYSTR_PT_OPTIONS (PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK)

#if defined(PTRACE_O_TRACESYSGOOD)
#define SYSTR_TMP SYSTR_PT_OPTIONS
#undef SYSTR_PT_OPTIONS
#define SYSTR_PT_OPTIONS (SYSTR_TMP | PTRACE_O_TRACESYSGOOD)
#undef SYSTR_TMP
#endif

#if defined(PTRACE_O_TRACECLONE)
#define SYSTR_TMP SYSTR_PT_OPTIONS
#undef SYSTR_PT_OPTIONS
#define SYSTR_PT_OPTIONS (SYSTR_TMP | PTRACE_O_TRACECLONE)
#undef SYSTR_TMP
#endif

#if defined(PTRACE_O_TRACEEXEC)
#define SYSTR_TMP SYSTR_PT_OPTIONS
#undef SYSTR_PT_OPTIONS
#define SYSTR_PT_OPTIONS (SYSTR_TMP | PTRACE_O_TRACEEXEC)
#undef SYSTR_TMP
#endif

#define SYSTR_HAS_FORK_TRACE 1
#define SYSTR_SET_TRACE_OPTIONS(p) ptrace(PTRACE_SETOPTIONS, p, NULL, SYSTR_PT_OPTIONS)

#else

#undef SYSTR_HAS_FORK_TRACE
#define SYSTR_SET_TRACE_OPTIONS(p) (void) 0

#endif



#if (defined(i386) || defined(__i386__))

#define SYSTR_GET_SCALLN(p, r) (r)->orig_eax
#define SYSTR_GET_PAR1(p, r) (r)->ebx
#define SYSTR_GET_PAR2(p, r) (r)->ecx
#define SYSTR_GET_PAR3(p, r) (r)->edx
#define SYSTR_GET_PAR4(p, r) (r)->esi
#define SYSTR_GET_PAR5(p, r) (r)->edi
#define SYSTR_GET_PAR6(p, r) (r)->ebp
#define SYSTR_GET_RCODE(p, r) (r)->eax
#define SYSTR_GET_SP(p, r) (r)->esp
#define SYSTR_GET_IP(p, r) (r)->eip

#define SYSTR_SET_SCALLN(p, r, v) (r)->orig_eax = (v)
#define SYSTR_SET_PAR1(p, r, v) (r)->ebx = (v)
#define SYSTR_SET_PAR2(p, r, v) (r)->ecx = (v)
#define SYSTR_SET_PAR3(p, r, v) (r)->edx = (v)
#define SYSTR_SET_PAR4(p, r, v) (r)->esi = (v)
#define SYSTR_SET_PAR5(p, r, v) (r)->edi = (v)
#define SYSTR_SET_PAR6(p, r, v) (r)->ebp = (v)
#define SYSTR_SET_RCODE(p, r, v) (r)->eax = (v)
#define SYSTR_SET_SP(p, r, v) (r)->esp = (v)
#define SYSTR_SET_IP(p, r, v) (r)->eip = (v)

#else
#error Your CPU is not supported by the sysctr library
#error Please mail to Davide Libenzi <davidel@xmailserver.org>
#endif


#endif

