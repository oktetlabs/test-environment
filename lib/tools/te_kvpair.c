/** @file
 * @brief Key-value pairs API
 *
 * Implemetation of API for working with key-value pairs.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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
#include "logger_api.h"

/* See the description in te_kvpair.h */
void
te_kvpair_init(te_kvpair_h *head)
{
    TAILQ_INIT(head);
}

static void
te_kvpair_remove(te_kvpair_h *head, te_kvpair *pair)
{
    TAILQ_REMOVE(head, pair, links);
    free(pair->key);
    free(pair->value);
    free(pair);
}

/* See the description in te_kvpair.h */
void
te_kvpair_fini(te_kvpair_h *head)
{
    te_kvpair *p;

    if (head == NULL)
        return;

    while ((p = TAILQ_FIRST(head)) != NULL)
        te_kvpair_remove(head, p);
}

/* See the description in te_kvpair.h */
const char *
te_kvpairs_get(const te_kvpair_h *head, const char *key)
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
te_kvpairs_del(te_kvpair_h *head, const char *key)
{
    te_kvpair *p;

    assert(head != NULL);
    assert(key != NULL);

    TAILQ_FOREACH(p, head, links)
    {
        if (strcmp(key, p->key) == 0)
        {
            te_kvpair_remove(head, p);
            return 0;
        }
    }

    return TE_ENOENT;
}


/* See the description in te_kvpair.h */
te_errno
te_kvpair_add_va(te_kvpair_h *head, const char *key,
                 const char *value_fmt, va_list ap)
{
    te_kvpair  *p;
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

    rc = te_vasprintf(&p->value, value_fmt, ap);
    if (rc < 0)
    {
        free(p->key);
        free(p);

        return TE_EFAIL;
    }

    TAILQ_INSERT_TAIL(head, p, links);

    return 0;
}

/* See the description in te_kvpair.h */
te_errno
te_kvpair_add(te_kvpair_h *head, const char *key,
              const char *value_fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, value_fmt);
    rc = te_kvpair_add_va(head, key, value_fmt, ap);
    va_end(ap);

    return rc;
}

/* See the description in te_kvpair.h */
te_errno
te_kvpair_to_str(const te_kvpair_h *head, te_string *str)
{
    te_kvpair *p;
    te_errno   rc;
    te_bool    first = TRUE;

    assert(head != NULL);
    assert(str != NULL);

    TAILQ_FOREACH(p, head, links)
    {
        rc = te_string_append(str, "%s%s=%s", first ? "" : ":",
                              p->key, p->value);
        if (rc != 0)
            return rc;

        first = FALSE;
    }
    return 0;
}

/* See the description in te_kvpair.h */
te_errno
te_kvpair_from_str(const char *str, te_kvpair_h *head)
{
    char       *dup;
    char       *token;
    char       *saveptr1;
    te_errno    rc;

    assert(str != NULL);
    assert(head != NULL);

    if ((dup = strdup(str)) == NULL)
        return TE_ENOMEM;

    for (token = strtok_r(dup, TE_KVPAIR_STR_DELIMITER, &saveptr1);
         token != NULL;
         token = strtok_r(NULL, TE_KVPAIR_STR_DELIMITER, &saveptr1))
    {
        char   *key;
        char   *val;

        if (*token == '=')
        {
            ERROR("Wrong token '%s': empty key is not allowed", token);
            free(dup);
            te_kvpair_fini(head);
            return TE_EINVAL;
        }

        key = strtok_r(token, "=", &val);
        if ((rc = te_kvpair_add(head, key, val)) != 0)
        {
            free(dup);
            te_kvpair_fini(head);
            return rc;
        }
    }
    free(dup);
    return 0;
}
