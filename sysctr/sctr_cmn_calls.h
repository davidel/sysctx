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


#if !defined(SCTR_CMN_CALLS_H)
#define SCTR_CMN_CALLS_H


int systr_get_tgid_ppid(pid_t pid, pid_t *tgid, pid_t *ppid);
int systr_enum_tgid_group(pid_t tgid, int (*eproc)(void *, pid_t), void *priv);
int systr_trace_trap(pidhash_t *ph);
int systr_waitpid(pid_t wpid, pid_t *pid, int *status);


#endif

