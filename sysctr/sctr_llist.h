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

