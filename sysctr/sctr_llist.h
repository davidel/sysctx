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

#if !defined(SCTR_LLIST_H)
#define SCTR_LLIST_H


#define LLIST_INIT(p) (p)->prev = (p)->next = (p)
#define LLIST_ADD(i, p, n) do { \
	llist_t *__p = (p), *__n = (n); \
	(i)->prev = (__p), (i)->next = (__n); \
	(__p)->next = (i), (__n)->prev = (i); \
} while (0)
#define LLIST_ADDH(i, h) LLIST_ADD(i, h, (h)->next)
#define LLIST_ADDT(i, h) LLIST_ADD(i, (h)->prev, h)
#define LLIST_DEL(i) do { \
	llist_t *__p = (i)->prev, *__n = (i)->next; \
	__p->next = __n, __n->prev = __p; \
} while (0)
#define LLIST_DEL_INIT(i) do { \
	llist_t *__p = (i)->prev, *__n = (i)->next; \
	__p->next = __n, __n->prev = __p; \
	LLIST_INIT(i); \
} while (0)
#define LLIST_EMPTY(h) ((h)->next == (h))
#define LLIST_FIRST(h) ((h)->next != (h) ? (h)->next: NULL)
#define LLIST_LAST(h) ((h)->prev != (h) ? (h)->prev: NULL)
#define LLIST_NEXT(i, h) ((i)->next != (h) ? (i)->next: NULL)
#define LLIST_PREV(i, h) ((i)->prev != (h) ? (i)->prev: NULL)
#define LLIST_ITEM(p, t, f) ((t *) OFFSET_PTR(p, -OFFSET_OF(t, f)))


typedef struct s_llist {
	struct s_llist *prev, *next;
} llist_t;



#endif

