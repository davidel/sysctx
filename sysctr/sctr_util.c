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


#include "sctr_include.h"



static int systr_zerolen(const char *ptr, int n);




int systr_set_cloexec(int fd) {
	int arg;

	if ((arg = fcntl(fd, F_GETFD)) < 0 ||
	    fcntl(fd, F_SETFD, arg | FD_CLOEXEC) < 0)
		return -1;

	return 0;
}

int systr_hash_init(shash_t *sh, int size) {
	int i;

	if (!(sh->bucks = (llist_t *) malloc(size * sizeof(llist_t))))
		return -1;
	sh->size = size;
	for (i = 0; i < size; i++)
		LLIST_INIT(&sh->bucks[i]);

	return 0;
}

llist_t *systr_hash_bucket(shash_t *sh, unsigned long val) {

	return &sh->bucks[val % sh->size];
}

void systr_hash_free(shash_t *sh, void (*ffunc)(llist_t *)) {
	int i;
	llist_t *head, *pos, *next;

	for (i = 0; i < sh->size; i++) {
		head = &sh->bucks[i];
		for (next = LLIST_FIRST(head); (pos = next) != NULL;) {
			next = LLIST_NEXT(pos, head);
			LLIST_DEL_INIT(pos);
			(*ffunc)(pos);
		}
	}

	free(sh->bucks);
}

int systr_procmem_read(pidhash_t *ph, int where, long addr, char *buf, int size) {
	int n, rsize = size, rdop;
	union {
		long l;
		unsigned char b[sizeof(long)];
	} val;

	if (where == SYSTR_DATA_SECT)
		rdop = PTRACE_PEEKDATA;
	else if (where == SYSTR_TEXT_SECT)
		rdop = PTRACE_PEEKTEXT;
	else
		return -1;

	if (addr & (sizeof(long) - 1)) {
		n = sizeof(long) - (addr & (sizeof(long) - 1));
		addr &= ~(sizeof(long) - 1);
		errno = 0;
		val.l = ptrace(rdop, ph->pid, (char *) addr, 0);
		if (errno)
			return -1;
		memcpy(buf, &val.b[sizeof(long) - n], MIN(n, rsize));
		buf += n;
		rsize -= n;
		addr += sizeof(long);
	}
	while (rsize > 0) {
		errno = 0;
		val.l = ptrace(rdop, ph->pid, (char *) addr, 0);
		if (errno)
			return -1;
		n = MIN(rsize, sizeof(long));
		memcpy(buf, val.b, n);
		buf += n;
		rsize -= n;
		addr += sizeof(long);
	}

	return size;
}

int systr_procmem_write(pidhash_t *ph, int where, long addr, const char *buf, int size) {
	int n, rsize = size, rdop, wrop;
	union {
		long l;
		unsigned char b[sizeof(long)];
	} val;

	if (where == SYSTR_DATA_SECT)
		rdop = PTRACE_PEEKDATA, wrop = PTRACE_POKEDATA;
	else if (where == SYSTR_TEXT_SECT)
		rdop = PTRACE_PEEKTEXT, wrop = PTRACE_POKETEXT;
	else
		return -1;

	if (addr & (sizeof(long) - 1)) {
		n = sizeof(long) - (addr & (sizeof(long) - 1));
		addr &= ~(sizeof(long) - 1);
		errno = 0;
		val.l = ptrace(rdop, ph->pid, (char *) addr, 0);
		if (errno)
			return -1;
		memcpy(&val.b[sizeof(long) - n], buf, MIN(n, rsize));
		if (ptrace(wrop, ph->pid, (char *) addr, (char *) val.l) < 0)
			return -1;
		buf += n;
		rsize -= n;
		addr += sizeof(long);
	}
	while (rsize > 0) {
		if (rsize < sizeof(long)) {
			errno = 0;
			val.l = ptrace(rdop, ph->pid, (char *) addr, 0);
			if (errno)
				return -1;
			n = rsize;
		} else
			n = sizeof(long);
		memcpy(val.b, buf, n);
		if (ptrace(wrop, ph->pid, (char *) addr, (char *) val.l) < 0)
			return -1;
		buf += n;
		rsize -= n;
		addr += sizeof(long);
	}

	return size;
}

static int systr_zerolen(const char *ptr, int n) {
	int i;

	for (i = 0; i < n; i++, ptr++)
		if (!*ptr)
			break;
	return i;
}

int systr_procszmem_read(pidhash_t *ph, int where, long addr, char *buf, int bmax) {
	int n, zn, len = 0, rdop;
	union {
		long l;
		unsigned char b[sizeof(long)];
	} val;

	if (where == SYSTR_DATA_SECT)
		rdop = PTRACE_PEEKDATA;
	else if (where == SYSTR_TEXT_SECT)
		rdop = PTRACE_PEEKTEXT;
	else
		return -1;

	if (addr & (sizeof(long) - 1)) {
		n = sizeof(long) - (addr & (sizeof(long) - 1));
		if (bmax < n)
			return -1;
		addr &= ~(sizeof(long) - 1);
		errno = 0;
		val.l = ptrace(rdop, ph->pid, (char *) addr, 0);
		if (errno)
			return -1;
		memcpy(buf, &val.b[sizeof(long) - n], n);
		if ((zn = systr_zerolen(buf, n)) != n)
			return zn;
		buf += n;
		len += n;
		addr += sizeof(long);
	}
	for (;;) {
		if (len >= bmax)
			return -1;
		errno = 0;
		val.l = ptrace(rdop, ph->pid, (char *) addr, 0);
		if (errno)
			return -1;
		n = MIN(bmax - len, sizeof(long));
		memcpy(buf, val.b, n);
		if ((zn = systr_zerolen(buf, n)) != n) {
			len += zn;
			break;
		}
		buf += n;
		len += n;
		addr += sizeof(long);
	}

	return len;
}

int systr_ctx_restore(pidhash_t *ph) {
	unsigned long value;
	struct user_regs_struct ur;

	ur = ph->ur;
	if (ph->resmask & SYSTR_RESF_PARAM1) {
		value = (unsigned long) SYSTR_GET_PAR1(ph->pid, &ph->resur);
		SYSTR_SET_PAR1(ph->pid, &ur, value);
	}
	if (ph->resmask & SYSTR_RESF_PARAM2) {
		value = (unsigned long) SYSTR_GET_PAR2(ph->pid, &ph->resur);
		SYSTR_SET_PAR2(ph->pid, &ur, value);
	}
	if (ph->resmask & SYSTR_RESF_PARAM3) {
		value = (unsigned long) SYSTR_GET_PAR3(ph->pid, &ph->resur);
		SYSTR_SET_PAR3(ph->pid, &ur, value);
	}
	if (ph->resmask & SYSTR_RESF_PARAM4) {
		value = (unsigned long) SYSTR_GET_PAR4(ph->pid, &ph->resur);
		SYSTR_SET_PAR4(ph->pid, &ur, value);
	}
	if (ph->resmask & SYSTR_RESF_PARAM5) {
		value = (unsigned long) SYSTR_GET_PAR5(ph->pid, &ph->resur);
		SYSTR_SET_PAR5(ph->pid, &ur, value);
	}
	if (ph->resmask & SYSTR_RESF_PARAM6) {
		value = (unsigned long) SYSTR_GET_PAR6(ph->pid, &ph->resur);
		SYSTR_SET_PAR6(ph->pid, &ur, value);
	}
	if (ph->resmask & SYSTR_RESF_SCALLN) {
		value = (unsigned long) SYSTR_GET_SCALLN(ph->pid, &ph->resur);
		SYSTR_SET_SCALLN(ph->pid, &ur, value);
	}
	if (ph->resmask & SYSTR_RESF_RCODE) {
		value = (unsigned long) SYSTR_GET_RCODE(ph->pid, &ph->resur);
		SYSTR_SET_RCODE(ph->pid, &ur, value);
	}

	if (ptrace(PTRACE_SETREGS, ph->pid, NULL, &ur)) {
		DBGPRINT(2, "[%d] error: ptrace(PTRACE_SETREGS, %d)\n",
			 ph->pid, ph->pid);
		return -1;
	}

	ph->ur = ur;
	ph->sresmask = ph->resmask;
	ph->resmask = 0;

	return 0;
}

