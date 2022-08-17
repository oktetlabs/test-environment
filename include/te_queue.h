/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief sys/queue.h
 *
 * Macros to deal with linked lists. DO NOT MODIFY IT! It is almost
 * exact copy of sys/queue.h, since it is widely used in TE sources,
 * but does not present on some platforms.
 *
 * UPDATE: this file had to be modified because sys/queue.h sometimes
 * does not contain macros like TAILQ_FOREACH_SAFE(); in this case
 * such macros may be defined using remaining ones.
 *
 * The latest copy is obtained from FreeBSD CVS ToT. Mention of
 * _KERNEL is removed. Own defition of a list is made iff sys/queue.h
 * does not present or it does not provide corresponding list.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef    __TE_QUEUE_H__
#define    __TE_QUEUE_H__

#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

/*-
 * Copyright (c) 1991, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *    @(#)queue.h    8.5 (Berkeley) 8/20/94
 * $FreeBSD: /repoman/r/ncvs/src/sys/sys/queue.h,v 1.68 2006/10/24 11:20:29 ru Exp $
 */

/*
 * This file defines four types of data structures: singly-linked lists,
 * singly-linked tail queues, lists and tail queues.
 *
 * A singly-linked list is headed by a single forward pointer. The elements
 * are singly linked for minimum space and pointer manipulation overhead at
 * the expense of O(n) removal for arbitrary elements. New elements can be
 * added to the list after an existing element or at the head of the list.
 * Elements being removed from the head of the list should use the explicit
 * macro for this purpose for optimum efficiency. A singly-linked list may
 * only be traversed in the forward direction.  Singly-linked lists are ideal
 * for applications with large datasets and few or no removals or for
 * implementing a LIFO queue.
 *
 * A singly-linked tail queue is headed by a pair of pointers, one to the
 * head of the list and the other to the tail of the list. The elements are
 * singly linked for minimum space and pointer manipulation overhead at the
 * expense of O(n) removal for arbitrary elements. New elements can be added
 * to the list after an existing element, at the head of the list, or at the
 * end of the list. Elements being removed from the head of the tail queue
 * should use the explicit macro for this purpose for optimum efficiency.
 * A singly-linked tail queue may only be traversed in the forward direction.
 * Singly-linked tail queues are ideal for applications with large datasets
 * and few or no removals or for implementing a FIFO queue.
 *
 * A list is headed by a single forward pointer (or an array of forward
 * pointers for a hash table header). The elements are doubly linked
 * so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before
 * or after an existing element or at the head of the list. A list
 * may only be traversed in the forward direction.
 *
 * A tail queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or
 * after an existing element, at the head of the list, or at the end of
 * the list. A tail queue may be traversed in either direction.
 *
 * For details on the use of these macros, see the queue(3) manual page.
 *
 *
 *                SLIST    LIST    STAILQ    TAILQ
 * _HEAD            +    +    +    +
 * _HEAD_INITIALIZER        +    +    +    +
 * _ENTRY            +    +    +    +
 * _INIT            +    +    +    +
 * _EMPTY            +    +    +    +
 * _FIRST            +    +    +    +
 * _NEXT            +    +    +    +
 * _PREV            -    -    -    +
 * _LAST            -    -    +    +
 * _FOREACH            +    +    +    +
 * _FOREACH_SAFE        +    +    +    +
 * _FOREACH_REVERSE        -    -    -    +
 * _FOREACH_REVERSE_SAFE    -    -    -    +
 * _INSERT_HEAD            +    +    +    +
 * _INSERT_BEFORE        -    +    -    +
 * _INSERT_AFTER        +    +    +    +
 * _INSERT_TAIL            -    -    +    +
 * _CONCAT            -    -    +    +
 * _REMOVE_HEAD            +    -    +    -
 * _REMOVE            +    +    +    +
 *
 */

#ifndef TRACEBUF
#ifdef QUEUE_MACRO_DEBUG
/* Store the last 2 places the queue element or head was altered */
struct qm_trace {
    char * lastfile;
    int lastline;
    char * prevfile;
    int prevline;
};

#define    TRACEBUF    struct qm_trace trace;
#define    TRASHIT(x)    do {(x) = (void *)-1;} while (0)

#define    QMD_TRACE_HEAD(head) do {                    \
    (head)->trace.prevline = (head)->trace.lastline;        \
    (head)->trace.prevfile = (head)->trace.lastfile;        \
    (head)->trace.lastline = __LINE__;                \
    (head)->trace.lastfile = __FILE__;                \
} while (0)

#define    QMD_TRACE_ELEM(elem) do {                    \
    (elem)->trace.prevline = (elem)->trace.lastline;        \
    (elem)->trace.prevfile = (elem)->trace.lastfile;        \
    (elem)->trace.lastline = __LINE__;                \
    (elem)->trace.lastfile = __FILE__;                \
} while (0)

#else
#define    QMD_TRACE_ELEM(elem)
#define    QMD_TRACE_HEAD(head)
#define    TRACEBUF
#define    TRASHIT(x)
#endif    /* QUEUE_MACRO_DEBUG */
#endif /* !TRACEBUF */


#ifndef SLIST_HEAD

/*
 * Singly-linked List declarations.
 */
#define    SLIST_HEAD(name, type)                        \
struct name {                                \
    struct type *slh_first;    /* first element */            \
}

#define    SLIST_HEAD_INITIALIZER(head)                    \
    { NULL }

#define    SLIST_ENTRY(type)                        \
struct {                                \
    struct type *sle_next;    /* next element */            \
}

/*
 * Singly-linked List functions.
 */
#define    SLIST_EMPTY(head)    ((head)->slh_first == NULL)

#define    SLIST_FIRST(head)    ((head)->slh_first)

#define    SLIST_FOREACH(var, head, field)                    \
    for ((var) = SLIST_FIRST((head));                \
        (var);                            \
        (var) = SLIST_NEXT((var), field))

#define    SLIST_INIT(head) do {                        \
    SLIST_FIRST((head)) = NULL;                    \
} while (0)

#define    SLIST_INSERT_AFTER(slistelm, elm, field) do {            \
    SLIST_NEXT((elm), field) = SLIST_NEXT((slistelm), field);    \
    SLIST_NEXT((slistelm), field) = (elm);                \
} while (0)

#define    SLIST_INSERT_HEAD(head, elm, field) do {            \
    SLIST_NEXT((elm), field) = SLIST_FIRST((head));            \
    SLIST_FIRST((head)) = (elm);                    \
} while (0)

#define    SLIST_NEXT(elm, field)    ((elm)->field.sle_next)

#define    SLIST_REMOVE(head, elm, type, field) do {            \
    if (SLIST_FIRST((head)) == (elm)) {                \
        SLIST_REMOVE_HEAD((head), field);            \
    }                                \
    else {                                \
        struct type *curelm = SLIST_FIRST((head));        \
        while (SLIST_NEXT(curelm, field) != (elm))        \
            curelm = SLIST_NEXT(curelm, field);        \
        SLIST_NEXT(curelm, field) =                \
            SLIST_NEXT(SLIST_NEXT(curelm, field), field);    \
    }                                \
    TRASHIT((elm)->field.sle_next);                    \
} while (0)

#define    SLIST_REMOVE_HEAD(head, field) do {                \
    SLIST_FIRST((head)) = SLIST_NEXT(SLIST_FIRST((head)), field);    \
} while (0)

#endif /* !SLIST_HEAD */

#ifndef SLIST_FOREACH_SAFE
#define SLIST_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = SLIST_FIRST((head));                           \
        (var) && ((tvar) = SLIST_NEXT((var), field), 1);        \
        (var) = (tvar))
#endif

#ifndef SLIST_FOREACH_PREVPTR
#define SLIST_FOREACH_PREVPTR(var, varp, head, field) \
    for ((varp) = &SLIST_FIRST((head));                         \
        ((var) = *(varp)) != NULL;                              \
        (varp) = &SLIST_NEXT((var), field))
#endif


#ifndef STAILQ_HEAD

/*
 * Singly-linked Tail queue declarations.
 */
#define    STAILQ_HEAD(name, type)                        \
struct name {                                \
    struct type *stqh_first;/* first element */            \
    struct type **stqh_last;/* addr of last next element */        \
}

#define    STAILQ_HEAD_INITIALIZER(head)                    \
    { NULL, &(head).stqh_first }

#define    STAILQ_ENTRY(type)                        \
struct {                                \
    struct type *stqe_next;    /* next element */            \
}

/*
 * Singly-linked Tail queue functions.
 */
#define    STAILQ_CONCAT(head1, head2) do {                \
    if (!STAILQ_EMPTY((head2))) {                    \
        *(head1)->stqh_last = (head2)->stqh_first;        \
        (head1)->stqh_last = (head2)->stqh_last;        \
        STAILQ_INIT((head2));                    \
    }                                \
} while (0)

#define    STAILQ_EMPTY(head)    ((head)->stqh_first == NULL)

#define    STAILQ_FIRST(head)    ((head)->stqh_first)

#define    STAILQ_FOREACH(var, head, field)                \
    for((var) = STAILQ_FIRST((head));                \
       (var);                            \
       (var) = STAILQ_NEXT((var), field))


#define    STAILQ_INIT(head) do {                        \
    STAILQ_FIRST((head)) = NULL;                    \
    (head)->stqh_last = &STAILQ_FIRST((head));            \
} while (0)

#define    STAILQ_INSERT_AFTER(head, tqelm, elm, field) do {        \
    if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((tqelm), field)) == NULL)\
        (head)->stqh_last = &STAILQ_NEXT((elm), field);        \
    STAILQ_NEXT((tqelm), field) = (elm);                \
} while (0)

#define    STAILQ_INSERT_HEAD(head, elm, field) do {            \
    if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL)    \
        (head)->stqh_last = &STAILQ_NEXT((elm), field);        \
    STAILQ_FIRST((head)) = (elm);                    \
} while (0)

#define    STAILQ_INSERT_TAIL(head, elm, field) do {            \
    STAILQ_NEXT((elm), field) = NULL;                \
    *(head)->stqh_last = (elm);                    \
    (head)->stqh_last = &STAILQ_NEXT((elm), field);            \
} while (0)

/*
 * Avoid using this macro - it may be missing in <sys/queue.h> and it
 * cannot be redefined via other marcos.
 */
#define    STAILQ_LAST(head, type, field)                    \
    (STAILQ_EMPTY((head)) ?                        \
        NULL :                            \
            ((struct type *)(void *)                \
        ((char *)((head)->stqh_last) - offsetof(struct type, field))))

#define    STAILQ_NEXT(elm, field)    ((elm)->field.stqe_next)

#define    STAILQ_REMOVE(head, elm, type, field) do {            \
    if (STAILQ_FIRST((head)) == (elm)) {                \
        STAILQ_REMOVE_HEAD((head), field);            \
    }                                \
    else {                                \
        struct type *curelm = STAILQ_FIRST((head));        \
        while (STAILQ_NEXT(curelm, field) != (elm))        \
            curelm = STAILQ_NEXT(curelm, field);        \
        if ((STAILQ_NEXT(curelm, field) =            \
             STAILQ_NEXT(STAILQ_NEXT(curelm, field), field)) == NULL)\
            (head)->stqh_last = &STAILQ_NEXT((curelm), field);\
    }                                \
    TRASHIT((elm)->field.stqe_next);                \
} while (0)

#define    STAILQ_REMOVE_HEAD(head, field) do {                \
    if ((STAILQ_FIRST((head)) =                    \
         STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL)        \
        (head)->stqh_last = &STAILQ_FIRST((head));        \
} while (0)

#endif /* !STAILQ_HEAD */

#ifndef STAILQ_FOREACH_SAFE
#define STAILQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = STAILQ_FIRST((head));                            \
        (var) && ((tvar) = STAILQ_NEXT((var), field), 1);         \
        (var) = (tvar))
#endif


#ifndef LIST_HEAD

/*
 * List declarations.
 */
#define    LIST_HEAD(name, type)                        \
struct name {                                \
    struct type *lh_first;    /* first element */            \
}

#define    LIST_HEAD_INITIALIZER(head)                    \
    { NULL }

#define    LIST_ENTRY(type)                        \
struct {                                \
    struct type *le_next;    /* next element */            \
    struct type **le_prev;    /* address of previous next element */    \
}

/*
 * List functions.
 */

#if defined(INVARIANTS)
#define    QMD_LIST_CHECK_HEAD(head, field) do {                \
    if (LIST_FIRST((head)) != NULL &&                \
        LIST_FIRST((head))->field.le_prev !=            \
         &LIST_FIRST((head)))                    \
        panic("Bad list head %p first->prev != head", (head));    \
} while (0)

#define    QMD_LIST_CHECK_NEXT(elm, field) do {                \
    if (LIST_NEXT((elm), field) != NULL &&                \
        LIST_NEXT((elm), field)->field.le_prev !=            \
         &((elm)->field.le_next))                    \
             panic("Bad link elm %p next->prev != elm", (elm));    \
} while (0)

#define    QMD_LIST_CHECK_PREV(elm, field) do {                \
    if (*(elm)->field.le_prev != (elm))                \
        panic("Bad link elm %p prev->next != elm", (elm));    \
} while (0)
#else
#define    QMD_LIST_CHECK_HEAD(head, field)
#define    QMD_LIST_CHECK_NEXT(elm, field)
#define    QMD_LIST_CHECK_PREV(elm, field)
#endif /* (INVARIANTS) */

#define    LIST_EMPTY(head)    ((head)->lh_first == NULL)

#define    LIST_FIRST(head)    ((head)->lh_first)

#define    LIST_FOREACH(var, head, field)                    \
    for ((var) = LIST_FIRST((head));                \
        (var);                            \
        (var) = LIST_NEXT((var), field))

#define    LIST_INIT(head) do {                        \
    LIST_FIRST((head)) = NULL;                    \
} while (0)

#define    LIST_INSERT_AFTER(listelm, elm, field) do {            \
    QMD_LIST_CHECK_NEXT(listelm, field);                \
    if ((LIST_NEXT((elm), field) = LIST_NEXT((listelm), field)) != NULL)\
        LIST_NEXT((listelm), field)->field.le_prev =        \
            &LIST_NEXT((elm), field);                \
    LIST_NEXT((listelm), field) = (elm);                \
    (elm)->field.le_prev = &LIST_NEXT((listelm), field);        \
} while (0)

#define    LIST_INSERT_BEFORE(listelm, elm, field) do {            \
    QMD_LIST_CHECK_PREV(listelm, field);                \
    (elm)->field.le_prev = (listelm)->field.le_prev;        \
    LIST_NEXT((elm), field) = (listelm);                \
    *(listelm)->field.le_prev = (elm);                \
    (listelm)->field.le_prev = &LIST_NEXT((elm), field);        \
} while (0)

#define    LIST_INSERT_HEAD(head, elm, field) do {                \
    QMD_LIST_CHECK_HEAD((head), field);                \
    if ((LIST_NEXT((elm), field) = LIST_FIRST((head))) != NULL)    \
        LIST_FIRST((head))->field.le_prev = &LIST_NEXT((elm), field);\
    LIST_FIRST((head)) = (elm);                    \
    (elm)->field.le_prev = &LIST_FIRST((head));            \
} while (0)

#define    LIST_NEXT(elm, field)    ((elm)->field.le_next)

#define    LIST_REMOVE(elm, field) do {                    \
    QMD_LIST_CHECK_NEXT(elm, field);                \
    QMD_LIST_CHECK_PREV(elm, field);                \
    if (LIST_NEXT((elm), field) != NULL)                \
        LIST_NEXT((elm), field)->field.le_prev =         \
            (elm)->field.le_prev;                \
    *(elm)->field.le_prev = LIST_NEXT((elm), field);        \
    TRASHIT((elm)->field.le_next);                    \
    TRASHIT((elm)->field.le_prev);                    \
} while (0)

#endif /* !LIST_HEAD */

#ifndef LIST_FOREACH_SAFE
#define LIST_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = LIST_FIRST((head));                            \
        (var) && ((tvar) = LIST_NEXT((var), field), 1);         \
        (var) = (tvar))
#endif


#ifndef TAILQ_HEAD

/*
 * Tail queue declarations.
 */
#define    TAILQ_HEAD(name, type)                        \
struct name {                                \
    struct type *tqh_first;    /* first element */            \
    struct type **tqh_last;    /* addr of last next element */        \
    TRACEBUF                            \
}

#define    TAILQ_HEAD_INITIALIZER(head)                    \
    { NULL, &(head).tqh_first }

#define    TAILQ_ENTRY(type)                        \
struct {                                \
    struct type *tqe_next;    /* next element */            \
    struct type **tqe_prev;    /* address of previous next element */    \
    TRACEBUF                            \
}

/*
 * Tail queue functions.
 */
#if defined(INVARIANTS)
#define    QMD_TAILQ_CHECK_HEAD(head, field) do {                \
    if (!TAILQ_EMPTY(head) &&                    \
        TAILQ_FIRST((head))->field.tqe_prev !=            \
         &TAILQ_FIRST((head)))                    \
        panic("Bad tailq head %p first->prev != head", (head));    \
} while (0)

#define    QMD_TAILQ_CHECK_TAIL(head, field) do {                \
    if (*(head)->tqh_last != NULL)                    \
            panic("Bad tailq NEXT(%p->tqh_last) != NULL", (head));     \
} while (0)

#define    QMD_TAILQ_CHECK_NEXT(elm, field) do {                \
    if (TAILQ_NEXT((elm), field) != NULL &&                \
        TAILQ_NEXT((elm), field)->field.tqe_prev !=            \
         &((elm)->field.tqe_next))                    \
        panic("Bad link elm %p next->prev != elm", (elm));    \
} while (0)

#define    QMD_TAILQ_CHECK_PREV(elm, field) do {                \
    if (*(elm)->field.tqe_prev != (elm))                \
        panic("Bad link elm %p prev->next != elm", (elm));    \
} while (0)
#else
#define    QMD_TAILQ_CHECK_HEAD(head, field)
#define    QMD_TAILQ_CHECK_TAIL(head, headname)
#define    QMD_TAILQ_CHECK_NEXT(elm, field)
#define    QMD_TAILQ_CHECK_PREV(elm, field)
#endif /* (INVARIANTS) */

#define    TAILQ_CONCAT(head1, head2, field) do {                \
    if (!TAILQ_EMPTY(head2)) {                    \
        *(head1)->tqh_last = (head2)->tqh_first;        \
        (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;    \
        (head1)->tqh_last = (head2)->tqh_last;            \
        TAILQ_INIT((head2));                    \
        QMD_TRACE_HEAD(head1);                    \
        QMD_TRACE_HEAD(head2);                    \
    }                                \
} while (0)

#define    TAILQ_EMPTY(head)    ((head)->tqh_first == NULL)

#define    TAILQ_FIRST(head)    ((head)->tqh_first)

#define    TAILQ_FOREACH(var, head, field)                    \
    for ((var) = TAILQ_FIRST((head));                \
        (var);                            \
        (var) = TAILQ_NEXT((var), field))

#define    TAILQ_FOREACH_REVERSE(var, head, headname, field)        \
    for ((var) = TAILQ_LAST((head), headname);            \
        (var);                            \
        (var) = TAILQ_PREV((var), headname, field))

#define    TAILQ_INIT(head) do {                        \
    TAILQ_FIRST((head)) = NULL;                    \
    (head)->tqh_last = &TAILQ_FIRST((head));            \
    QMD_TRACE_HEAD(head);                        \
} while (0)

#define    TAILQ_INSERT_AFTER(head, listelm, elm, field) do {        \
    QMD_TAILQ_CHECK_NEXT(listelm, field);                \
    if ((TAILQ_NEXT((elm), field) = TAILQ_NEXT((listelm), field)) != NULL)\
        TAILQ_NEXT((elm), field)->field.tqe_prev =         \
            &TAILQ_NEXT((elm), field);                \
    else {                                \
        (head)->tqh_last = &TAILQ_NEXT((elm), field);        \
        QMD_TRACE_HEAD(head);                    \
    }                                \
    TAILQ_NEXT((listelm), field) = (elm);                \
    (elm)->field.tqe_prev = &TAILQ_NEXT((listelm), field);        \
    QMD_TRACE_ELEM(&(elm)->field);                    \
    QMD_TRACE_ELEM(&listelm->field);                \
} while (0)

#define    TAILQ_INSERT_BEFORE(listelm, elm, field) do {            \
    QMD_TAILQ_CHECK_PREV(listelm, field);                \
    (elm)->field.tqe_prev = (listelm)->field.tqe_prev;        \
    TAILQ_NEXT((elm), field) = (listelm);                \
    *(listelm)->field.tqe_prev = (elm);                \
    (listelm)->field.tqe_prev = &TAILQ_NEXT((elm), field);        \
    QMD_TRACE_ELEM(&(elm)->field);                    \
    QMD_TRACE_ELEM(&listelm->field);                \
} while (0)

#define    TAILQ_INSERT_HEAD(head, elm, field) do {            \
    QMD_TAILQ_CHECK_HEAD(head, field);                \
    if ((TAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL)    \
        TAILQ_FIRST((head))->field.tqe_prev =            \
            &TAILQ_NEXT((elm), field);                \
    else                                \
        (head)->tqh_last = &TAILQ_NEXT((elm), field);        \
    TAILQ_FIRST((head)) = (elm);                    \
    (elm)->field.tqe_prev = &TAILQ_FIRST((head));            \
    QMD_TRACE_HEAD(head);                        \
    QMD_TRACE_ELEM(&(elm)->field);                    \
} while (0)

#define    TAILQ_INSERT_TAIL(head, elm, field) do {            \
    QMD_TAILQ_CHECK_TAIL(head, field);                \
    TAILQ_NEXT((elm), field) = NULL;                \
    (elm)->field.tqe_prev = (head)->tqh_last;            \
    *(head)->tqh_last = (elm);                    \
    (head)->tqh_last = &TAILQ_NEXT((elm), field);            \
    QMD_TRACE_HEAD(head);                        \
    QMD_TRACE_ELEM(&(elm)->field);                    \
} while (0)

#define    TAILQ_LAST(head, headname)                    \
    (*(((struct headname *)((head)->tqh_last))->tqh_last))

#define    TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define    TAILQ_PREV(elm, headname, field)                \
    (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#define    TAILQ_REMOVE(head, elm, field) do {                \
    QMD_TAILQ_CHECK_NEXT(elm, field);                \
    QMD_TAILQ_CHECK_PREV(elm, field);                \
    if ((TAILQ_NEXT((elm), field)) != NULL)                \
        TAILQ_NEXT((elm), field)->field.tqe_prev =         \
            (elm)->field.tqe_prev;                \
    else {                                \
        (head)->tqh_last = (elm)->field.tqe_prev;        \
        QMD_TRACE_HEAD(head);                    \
    }                                \
    *(elm)->field.tqe_prev = TAILQ_NEXT((elm), field);        \
    TRASHIT((elm)->field.tqe_next);                    \
    TRASHIT((elm)->field.tqe_prev);                    \
    QMD_TRACE_ELEM(&(elm)->field);                    \
} while (0)

#endif /* !TAILQ_HEAD */

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = TAILQ_FIRST((head));                           \
        (var) && ((tvar) = TAILQ_NEXT((var), field), 1);        \
        (var) = (tvar))
#endif

#ifndef TAILQ_FOREACH_REVERSE_SAFE
#define TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar) \
    for ((var) = TAILQ_LAST((head), headname);                            \
        (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);        \
        (var) = (tvar))
#endif


#ifndef CIRCLEQ_HEAD

/*
 * Circular queue definitions.
 */
#define CIRCLEQ_HEAD(name, type)                                        \
struct name {                                                           \
        struct type *cqh_first;         /* first element */             \
        struct type *cqh_last;          /* last element */              \
}

#define CIRCLEQ_HEAD_INITIALIZER(head)                                  \
        { CIRCLEQ_END(&head), CIRCLEQ_END(&head) }

#define CIRCLEQ_ENTRY(type)                                             \
struct {                                                                \
        struct type *cqe_next;          /* next element */              \
        struct type *cqe_prev;          /* previous element */          \
}

/*
 * Circular queue access methods
 */
#define CIRCLEQ_FIRST(head)             ((head)->cqh_first)
#define CIRCLEQ_LAST(head)              ((head)->cqh_last)
#define CIRCLEQ_END(head)               ((void *)(head))
#define CIRCLEQ_NEXT(elm, field)        ((elm)->field.cqe_next)
#define CIRCLEQ_PREV(elm, field)        ((elm)->field.cqe_prev)
#define CIRCLEQ_EMPTY(head)                                             \
        (CIRCLEQ_FIRST(head) == CIRCLEQ_END(head))

#define CIRCLEQ_FOREACH(var, head, field)                               \
        for((var) = CIRCLEQ_FIRST(head);                                \
            (var) != CIRCLEQ_END(head);                                 \
            (var) = CIRCLEQ_NEXT(var, field))

#define CIRCLEQ_FOREACH_REVERSE(var, head, field)                       \
        for((var) = CIRCLEQ_LAST(head);                                 \
            (var) != CIRCLEQ_END(head);                                 \
            (var) = CIRCLEQ_PREV(var, field))

/*
 * Circular queue functions.
 */
#define CIRCLEQ_INIT(head) do {                                         \
        (head)->cqh_first = CIRCLEQ_END(head);                          \
        (head)->cqh_last = CIRCLEQ_END(head);                           \
} while (0)

#define CIRCLEQ_INSERT_AFTER(head, listelm, elm, field) do {            \
        (elm)->field.cqe_next = (listelm)->field.cqe_next;              \
        (elm)->field.cqe_prev = (listelm);                              \
        if ((listelm)->field.cqe_next == CIRCLEQ_END(head))             \
                (head)->cqh_last = (elm);                               \
        else                                                            \
                (listelm)->field.cqe_next->field.cqe_prev = (elm);      \
        (listelm)->field.cqe_next = (elm);                              \
} while (0)

#define CIRCLEQ_INSERT_BEFORE(head, listelm, elm, field) do {           \
        (elm)->field.cqe_next = (listelm);                              \
        (elm)->field.cqe_prev = (listelm)->field.cqe_prev;              \
        if ((listelm)->field.cqe_prev == CIRCLEQ_END(head))             \
                (head)->cqh_first = (elm);                              \
        else                                                            \
                (listelm)->field.cqe_prev->field.cqe_next = (elm);      \
        (listelm)->field.cqe_prev = (elm);                              \
} while (0)

#define CIRCLEQ_INSERT_HEAD(head, elm, field) do {                      \
        (elm)->field.cqe_next = (head)->cqh_first;                      \
        (elm)->field.cqe_prev = CIRCLEQ_END(head);                      \
        if ((head)->cqh_last == CIRCLEQ_END(head))                      \
                (head)->cqh_last = (elm);                               \
        else                                                            \
                (head)->cqh_first->field.cqe_prev = (elm);              \
        (head)->cqh_first = (elm);                                      \
} while (0)

#define CIRCLEQ_INSERT_TAIL(head, elm, field) do {                      \
        (elm)->field.cqe_next = CIRCLEQ_END(head);                      \
        (elm)->field.cqe_prev = (head)->cqh_last;                       \
        if ((head)->cqh_first == CIRCLEQ_END(head))                     \
                (head)->cqh_first = (elm);                              \
        else                                                            \
                (head)->cqh_last->field.cqe_next = (elm);               \
        (head)->cqh_last = (elm);                                       \
} while (0)

#define CIRCLEQ_REMOVE(head, elm, field) do {                           \
        if ((elm)->field.cqe_next == CIRCLEQ_END(head))                 \
                (head)->cqh_last = (elm)->field.cqe_prev;               \
        else                                                            \
                (elm)->field.cqe_next->field.cqe_prev =                 \
                    (elm)->field.cqe_prev;                              \
        if ((elm)->field.cqe_prev == CIRCLEQ_END(head))                 \
                (head)->cqh_first = (elm)->field.cqe_next;              \
        else                                                            \
                (elm)->field.cqe_prev->field.cqe_next =                 \
                    (elm)->field.cqe_next;                              \
} while (0)

#define CIRCLEQ_REPLACE(head, elm, elm2, field) do {                    \
        if (((elm2)->field.cqe_next = (elm)->field.cqe_next) ==         \
            CIRCLEQ_END(head))                                          \
                (head).cqh_last = (elm2);                               \
        else                                                            \
                (elm2)->field.cqe_next->field.cqe_prev = (elm2);        \
        if (((elm2)->field.cqe_prev = (elm)->field.cqe_prev) ==         \
            CIRCLEQ_END(head))                                          \
                (head).cqh_first = (elm2);                              \
        else                                                            \
                (elm2)->field.cqe_prev->field.cqe_next = (elm2);        \
} while (0)

#endif /* !CIRCLEQ_HEAD */

#ifndef CIRCLEQ_REPLACE
#define CIRCLEQ_REPLACE(head, elm, elm2, field) \
    do {                                                  \
        CIRCLEQ_INSERT_AFTER(head, elm, elm2, field);     \
        CIRCLEQ_REMOVE(head, elm, field);                 \
    } while (0)
#endif

#endif /* !__TE_QUEUE_H__ */
