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


#if !defined(SCTR_UTIL_H)
#define SCTR_UTIL_H



int systr_set_cloexec(int fd);
int systr_hash_init(shash_t *sh, int size);
llist_t *systr_hash_bucket(shash_t *sh, unsigned long val);
void systr_hash_free(shash_t *sh, void (*ffunc)(llist_t *));
int systr_procmem_read(pidhash_t *ph, int where, long addr, char *buf, int size);
int systr_procmem_write(pidhash_t *ph, int where, long addr, const char *buf, int size);
int systr_procszmem_read(pidhash_t *ph, int where, long addr, char *buf, int bmax);
int systr_ctx_restore(pidhash_t *ph);



#endif

