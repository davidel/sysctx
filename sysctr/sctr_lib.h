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


#if !defined(SCTR_LIB_H)
#define SCTR_LIB_H



pidhash_t *systr_pidhash_get(pid_t pid);
pidhash_t *systr_pidhash_add(pid_t pid, struct user_regs_struct const *ur);
void systr_pidhash_release(pidhash_t *ph);
int systr_reparent(pidhash_t *ph, pidhash_t *pph);
int systr_exit_group(pidhash_t *ph);
int systr_sysc_enter(pidhash_t *ph);
int systr_task_resume(pidhash_t *ph);
int systr_adjust_parent(pidhash_t *ph, pidhash_t *pph);



#endif

