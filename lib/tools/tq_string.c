/** @file
 * @brief Tail queue of strings (char *).
 *
 * Implementation of API for working with tail queue of strings.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TQ String"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "tq_string.h"


/* See the description in tq_string.h */
void
tq_strings_free(tqh_strings *head, void (*value_free)(void *))
{
    tqe_string *p;

    if (head == NULL)
        return;

    while ((p = TAILQ_FIRST(head)) != NULL)
    {
        TAILQ_REMOVE(head, p, links);
        if (value_free != NULL)
            value_free(p->v);
        free(p);
    }
}

/* See the description in tq_string.h */
te_bool
tq_strings_equal(const tqh_strings *s1, const tqh_strings *s2)
{
    const tqe_string *p1;
    const tqe_string *p2;

    if (s1 == s2)
        return TRUE;
    if (s1 == NULL || s2 == NULL)
        return FALSE;

    for (p1 = TAILQ_FIRST(s1), p2 = TAILQ_FIRST(s2);
         p1 != NULL && p2 != NULL && strcmp(p1->v, p2->v) == 0;
         p1 = TAILQ_NEXT(p1, links), p2 = TAILQ_NEXT(p2, links));

    return (p1 == NULL) && (p2 == NULL);
}

/* See the description in tq_string.h */
te_errno
tq_strings_add_uniq_gen(tqh_strings *list, const char *value,
                        te_bool duplicate)
{
    tqe_string *p;

    assert(list != NULL);
    assert(value != NULL);

    for (p = TAILQ_FIRST(list);
         p != NULL && strcmp(value, p->v) != 0;
         p = TAILQ_NEXT(p, links));

    if (p == NULL)
    {
        p = TE_ALLOC(sizeof(*p));
        if (p == NULL)
            return TE_ENOMEM;

        if (duplicate)
        {
            p->v = strdup((char *)value);
            if (p->v == NULL)
            {
                free(p);
                return TE_ENOMEM;
            }
        }
        else
        {
            p->v = (char *)value;
        }

        TAILQ_INSERT_TAIL(list, p, links);

        return 0;
    }

    return 1;
}

/* See the description in tq_string.h */
te_errno
tq_strings_add_uniq(tqh_strings *list, const char *value)
{
    return tq_strings_add_uniq_gen(list, value, FALSE);
}

/* See the description in tq_string.h */
te_errno
tq_strings_add_uniq_dup(tqh_strings *list, const char *value)
{
    return tq_strings_add_uniq_gen(list, value, TRUE);
}

/** Mode argument for tq_strings_copy_move() */
typedef enum tq_strings_copy_mode {
    TQ_STRINGS_MOVE, /**< Remove tqe_string's from source queue,
                          add them to destination queue */
    TQ_STRINGS_SHALLOW_COPY, /**< Add tqe_string's pointing to the same
                                  strings to the destination queue, do not
                                  change the source queue */
    TQ_STRINGS_COPY, /**< Add tqe_string's pointing to newly allocated
                          duplicate strings to the destination queue,
                          do not change the source queue */
} tq_strings_copy_mode;

/**
 * Copy or move values from source queue to destination queue.
 *
 * @param dst       Destination queue
 * @param src       Source queue
 * @param mode      See tq_strings_copy_mode for supported modes
 *
 * @return Status code.
 */
static te_errno
tq_strings_copy_move(tqh_strings *dst,
                     tqh_strings *src,
                     tq_strings_copy_mode mode)
{
    tqe_string *str;
    tqe_string *str_aux;
    te_errno rc;

    TAILQ_FOREACH_SAFE(str, src, links, str_aux)
    {
        if (mode == TQ_STRINGS_MOVE)
        {
            TAILQ_REMOVE(src, str, links);
            TAILQ_INSERT_TAIL(dst, str, links);
        }
        else
        {
            rc = tq_strings_add_uniq_gen(
                          dst, str->v,
                          (mode == TQ_STRINGS_COPY ? TRUE : FALSE));
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/* See the description in tq_string.h */
te_errno
tq_strings_move(tqh_strings *dst,
                tqh_strings *src)
{
    return tq_strings_copy_move(dst, src, TQ_STRINGS_MOVE);
}

/* See the description in tq_string.h */
te_errno
tq_strings_copy(tqh_strings *dst,
                tqh_strings *src)
{
    return tq_strings_copy_move(dst, src, TQ_STRINGS_COPY);
}

/* See the description in tq_string.h */
te_errno
tq_strings_shallow_copy(tqh_strings *dst,
                        tqh_strings *src)
{
    return tq_strings_copy_move(dst, src, TQ_STRINGS_SHALLOW_COPY);
}
