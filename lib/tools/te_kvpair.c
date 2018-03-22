/** @file
 * @brief Key-value pairs API
 *
 * Implemetation of API for working with key-value pairs.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TE Key-value pairs"

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
#include "te_kvpair.h"

/* See the description in te_kvpair.h */
void
te_kvpair_init(te_kvpair_h *head)
{
    TAILQ_INIT(head);
}

/* See the description in te_kvpair.h */
void
te_kvpair_fini(te_kvpair_h *head)
{
    te_kvpair *p;

    if (head == NULL)
        return;

    while ((p = TAILQ_FIRST(head)) != NULL)
    {
        TAILQ_REMOVE(head, p, links);
        free(p->key);
        free(p->value);
        free(p);
    }
}

/* See the description in te_kvpair.h */
const char *
te_kvpairs_get(te_kvpair_h *head, const char *key)
{
    te_kvpair *p;

    assert(head != NULL);
    assert(key != NULL);

    for (p = TAILQ_FIRST(head); p != NULL; p = TAILQ_NEXT(p, links))
    {
        if (strcmp(key, p->key) == 0)
            return p->value;
    }

    return NULL;
}

/* See the description in te_kvpair.h */
te_errno
te_kvpair_add(te_kvpair_h *head, const char *key,
              const char *value_fmt, ...)
{
    te_kvpair  *p;
    va_list     ap;
    te_errno    rc;

    assert(head != NULL);
    assert(key != NULL);

    if (te_kvpairs_get(head, key) != NULL)
        return TE_EEXIST;

    p = TE_ALLOC(sizeof(*p));
    if (p == NULL)
        return TE_ENOMEM;

    p->key = strdup(key);
    if (p->key == NULL)
    {
        free(p);
        return TE_ENOMEM;
    }

    va_start(ap, value_fmt);
    rc = te_vasprintf(&p->value, value_fmt, ap);
    va_end(ap);

    if (rc < 0)
    {
        free(p->key);
        free(p);

        return TE_EFAIL;
    }

    TAILQ_INSERT_TAIL(head, p, links);

    return 0;
}
