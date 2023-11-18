/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Key-value pairs API
 *
 * Implemetation of API for working with key-value pairs.
 *
 *
 * Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved.
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
#include "te_string.h"
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
te_kvpairs_get_nth(const te_kvpair_h *head, const char *key, unsigned int index)
{
    te_kvpair *p;

    assert(head != NULL);
    assert(key != NULL);

    TAILQ_FOREACH(p, head, links)
    {
        if (strcmp(key, p->key) == 0)
        {
            if (index == 0)
                return p->value;
            else
                index--;
        }
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
te_kvpairs_del_all(te_kvpair_h *head, const char *key)
{
    te_kvpair *p;
    te_kvpair *tmp;
    te_errno rc = TE_ENOENT;

    assert(head != NULL);

    TAILQ_FOREACH_SAFE(p, head, links, tmp)
    {
        if (key == NULL || strcmp(key, p->key) == 0)
        {
            te_kvpair_remove(head, p);
            rc = 0;
        }
    }

    return rc;
}

/* See the description in te_kvpair.h */
void
te_kvpairs_copy(te_kvpair_h *dest, const te_kvpair_h *src)
{
    te_kvpairs_copy_key(dest, src, NULL);
}

static te_errno
kvpairs_copy(const char *key, const char *value, void *user)
{
    te_kvpair_h *dest = user;

    te_kvpair_push(dest, key, "%s", value);

    return 0;
}

/* See the description in te_kvpair.h */
void
te_kvpairs_copy_key(te_kvpair_h *dest, const te_kvpair_h *src, const char *key)
{
    te_kvpairs_foreach(src, kvpairs_copy, key, dest);
}

/* See the description in te_kvpair.h */
te_errno
te_kvpairs_foreach(const te_kvpair_h *head, te_kvpairs_iter_fn *callback,
                   const char *key, void *user)
{
    te_kvpair *p;
    te_errno rc = TE_ENOENT;

    assert(head != NULL);

    TAILQ_FOREACH(p, head, links)
    {
        if (key == NULL || strcmp(p->key, key) == 0)
        {
            rc = callback(p->key, p->value, user);
            if (rc != 0)
            {
                if (rc == TE_EOK)
                    rc = 0;
                break;
            }
        }
    }

    return rc;
}

static te_errno
kvpairs_count(const char *key, const char *value, void *user)
{
    UNUSED(key);
    UNUSED(value);

    (*(unsigned int *)user)++;
    return 0;
}

/* See the description in te_kvpair.h */
unsigned int
te_kvpairs_count(const te_kvpair_h *head, const char *key)
{
    unsigned int result = 0;

    te_kvpairs_foreach(head, kvpairs_count, key, &result);

    return result;
}

static te_errno
kvpairs_check(const char *key, const char *value, void *user)
{
    const char **exp_value = user;

    UNUSED(key);

    if (*exp_value == NULL || strcmp(value, *exp_value) == 0)
        return TE_EEXIST;

    return 0;
}

/* See the description in te_kvpair.h */
te_bool
te_kvpairs_has_kv(const te_kvpair_h *head, const char *key, const char *value)
{
    te_errno rc = te_kvpairs_foreach(head, kvpairs_check, key, &value);

    return rc == TE_EEXIST;
}

static te_errno
kvpairs_check_submap(const char *key, const char *value, void *user)
{
    const te_kvpair_h **supermap = user;

    if (te_kvpairs_has_kv(*supermap, key, value))
        return 0;

    return TE_ENODATA;
}

/* See the description in te_kvpair.h */
te_bool
te_kvpairs_is_submap(const te_kvpair_h *submap, const te_kvpair_h *supermap)
{
    te_errno rc = te_kvpairs_foreach(submap, kvpairs_check_submap, NULL,
                                     &supermap);

    /* TE_ENOENT case is only possible if submap is empty */
    return rc == 0 || rc == TE_ENOENT;
}

static te_errno
kvpairs_to_vec(const char *key, const char *value, void *user)
{
    te_vec *v = user;

    UNUSED(key);

    TE_VEC_APPEND(v, value);

    return 0;
}

/* See the description in te_kvpair.h */
te_errno
te_kvpairs_get_all(const te_kvpair_h *head, const char *key,
                   te_vec *result)
{
    return te_kvpairs_foreach(head, kvpairs_to_vec, key, result);
}

/* See the description in te_kvpair.h */
void
te_kvpair_push_va(te_kvpair_h *head, const char *key,
                  const char *value_fmt, va_list ap)
{
    te_kvpair  *p;
    te_string   value_str = TE_STRING_INIT;

    assert(head != NULL);
    assert(key != NULL);


    p = TE_ALLOC(sizeof(*p));

    p->key = TE_STRDUP(key);

    te_string_append_va(&value_str, value_fmt, ap);
    p->value = value_str.ptr;

    TAILQ_INSERT_HEAD(head, p, links);
}

/* See the description in te_kvpair.h */
void
te_kvpair_push(te_kvpair_h *head, const char *key,
               const char *value_fmt, ...)
{
    va_list  ap;

    va_start(ap, value_fmt);
    te_kvpair_push_va(head, key, value_fmt, ap);
    va_end(ap);
}

/* See the description in te_kvpair.h */
te_errno
te_kvpair_add_va(te_kvpair_h *head, const char *key,
                 const char *value_fmt, va_list ap)
{
    if (te_kvpairs_get(head, key) != NULL)
        return TE_EEXIST;

    te_kvpair_push_va(head, key, value_fmt, ap);
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
te_kvpair_to_str_gen(const te_kvpair_h *head, const char *delim, te_string *str)
{
    te_kvpair *p;
    te_bool    first = TRUE;

    assert(head != NULL);
    assert(delim != NULL);
    assert(str != NULL);

    TAILQ_FOREACH_REVERSE(p, head, te_kvpair_h, links)
    {
        te_string_append(str, "%s%s=%s", first ? "" : delim,
                         p->key, p->value);
        first = FALSE;
    }
    return 0;
}

/* See the description in te_kvpair.h */
te_errno
te_kvpair_to_str(const te_kvpair_h *head, te_string *str)
{
    return te_kvpair_to_str_gen(head, TE_KVPAIR_STR_DELIMITER, str);
}

/* See the description in te_kvpair.h */
void
te_kvpair_to_uri_query(const te_kvpair_h *head, te_string *str)
{
    te_kvpair *p;

    assert(head != NULL);
    assert(str != NULL);

    TAILQ_FOREACH_REVERSE(p, head, te_kvpair_h, links)
    {
        if (str->len > 0)
            te_string_append(str, "&");
        te_string_append_escape_uri(str, TE_STRING_URI_ESCAPE_QUERY_VALUE,
                                    p->key);
        te_string_append(str, "=");
        te_string_append_escape_uri(str, TE_STRING_URI_ESCAPE_QUERY_VALUE,
                                    p->value);
    }
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

    dup = TE_STRDUP(str);

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
        if ((rc = te_kvpair_add(head, key, "%s", val)) != 0)
        {
            free(dup);
            te_kvpair_fini(head);
            return rc;
        }
    }
    free(dup);
    return 0;
}
