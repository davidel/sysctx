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

