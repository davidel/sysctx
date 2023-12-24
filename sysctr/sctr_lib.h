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

