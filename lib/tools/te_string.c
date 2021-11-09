/** @file
 * @brief Testing Results Comparator: common
 *
 * Routines to work with strings.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "logger_api.h"

#include "te_string.h"
#include "te_defs.h"
#include "te_dbuf.h"

void
te_string_free(te_string *str)
{
    str->len = str->size = 0;
    if (!str->ext_buf)
        free(str->ptr);
    str->ptr = NULL;
    str->ext_buf = FALSE;
}

/* See the description in te_string.h */
void
te_string_set_buf(te_string *str,
                  char *buf, size_t size,
                  size_t len)
{
    te_string_free(str);
    str->ptr = buf;
    str->size = size;
    str->len = len;
    str->ext_buf = TRUE;
}

te_errno
te_string_append(te_string *str, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = te_string_append_va(str, fmt, ap);
    va_end(ap);
    return rc;
}

/* See description in te_string.h */
te_errno
te_string_reserve(te_string *str, size_t size)
{
    size_t          pagesize = getpagesize();
    size_t          adj_size;
    size_t          malloc_header_size = 4 * sizeof(void *);
    char           *ptr;
    size_t          grow_factor = 1;
    size_t          grow = 0;

    /*
     * Here we don't reinvent the wheel, we comply to the GCC C++ library
     * https://github.com/gcc-mirror/gcc/blob/master/libstdc++-v3/include/
     *  bits/basic_string.tcc
     * with the exception of grow factor exponent used.
     */
    if (size < str->size)
        return 0;

    /*
     * Apply grow factor ^ exp until predefined limit, and if size < newsize <
     * (factor ^ exp) * size, then use (factor ^ exp) * size as a resulting
     * size.
     *
     * Using factor ^ exp might be costly in terms of RAM used, so we fall back
     * to a regular addend-based expansion if we can't find it even after
     * applying @c TE_STRING_GROW_FACTOR_EXP_LIMIT exponent
     */
    for (grow = 0; grow < TE_STRING_GROW_FACTOR_EXP_LIMIT; ++grow)
    {
        grow_factor *= TE_STRING_GROW_FACTOR;
        if (size > str->size && size < grow_factor * str->size)
        {
            size = grow_factor * str->size;
            break;
        }
    }

    /*
     * Apply correction taking malloc overhead into account, it works for
     * allocations over page size. Based on GCC C++ basic_string implementation.
     */
    adj_size = size + malloc_header_size;
    if (adj_size > pagesize && size > str->size)
    {
        size_t extra;

        extra = pagesize - adj_size % pagesize;
        size += extra;
    }

    /* Actually reallocate data */
    ptr = realloc(str->ptr, size);
    if (ptr == NULL)
    {
        ERROR("%s(): Memory allocation failure", __FUNCTION__);
        return TE_ENOMEM;
    }

    str->ptr = ptr;
    str->size = size;
    return 0;
}

te_errno
te_string_append_va(te_string *str, const char *fmt, va_list ap)
{
    te_errno rc;
    char    *s;
    size_t   rest;
    int      printed;
    te_bool  again;
    va_list  ap_start;

    if (str->ptr == NULL)
    {
        size_t new_size;

        assert(!str->ext_buf);

        new_size = str->size != 0 ? str->size : TE_STRING_INIT_LEN;
        str->ptr = malloc(new_size);
        if (str->ptr == NULL)
        {
            ERROR("%s(): Memory allocation failure", __FUNCTION__);
            return TE_ENOMEM;
        }

        str->size = new_size;
        str->len = 0;
    }

    assert(str->size > str->len);
    rest = str->size - str->len;
    do  {
        s = str->ptr + str->len;

        va_copy(ap_start, ap);
        printed = vsnprintf(s, rest, fmt, ap_start);
        va_end(ap_start);
        assert(printed >= 0);

        if ((size_t)printed >= rest)
        {
            if (str->ext_buf)
            {
                str->len = str->size - 1 /* '\0' */;
                ERROR("%s(): Not enough space in supplied buffer",
                      __FUNCTION__);
                return TE_ENOBUFS;
            }
            else
            {
                rc = te_string_reserve(str, str->len + printed + 1 /* '\0' */);
                if (rc != 0)
                    return rc;

                rest = str->size - str->len;
                /* Print again */
                again = TRUE;
            }
        }
        else
        {
            str->len += printed;
            again = FALSE;
        }
    } while (again);

    return 0;
}

te_errno
te_string_append_shell_arg_as_is(te_string *str, const char *arg)
{
    te_errno rc = 0;
    te_string arg_escaped = TE_STRING_INIT;

    do {
        const char *p;
        int len;

        p = strchr(arg, '\'');
        if (p == NULL)
            len = strlen(arg);
        else
            len = p - arg;

        /* Print up to ' or end of string */
        rc = te_string_append(&arg_escaped, "'%.*s'", len, arg);
        if (rc != 0)
            break;
        arg += len;

        if (*arg == '\'')
        {
            rc = te_string_append(&arg_escaped, "\\\'");
            if (rc != 0)
                break;
            arg++;
        }
    } while (*arg != '\0');

    if (rc != 0)
    {
        te_string_free(&arg_escaped);
        return rc;
    }

    return te_string_append(str, "%s", arg_escaped.ptr);
}

te_errno
te_string_append_shell_args_as_is(te_string *str, ...)
{
    va_list args;
    const char *arg;
    te_errno rc = 0;

    va_start(args, str);
    while ((arg = va_arg(args, const char *)) != NULL)
    {
        if (str->len != 0)
        {
            rc = te_string_append(str, " ");
            if (rc != 0)
                break;
        }

        rc = te_string_append_shell_arg_as_is(str, arg);
        if (rc != 0)
            break;
    }
    va_end(args);

    return rc;
}

char *
te_string_fmt_va(const char *fmt,
                 va_list     ap)
{
    te_string str = TE_STRING_INIT;

    if (te_string_append_va(&str, fmt, ap) != 0)
        te_string_free(&str);

    return str.ptr;
}

char *
te_string_fmt(const char *fmt, ...)
{
    va_list  ap;
    char *result;

    va_start(ap, fmt);
    result = te_string_fmt_va(fmt, ap);
    va_end(ap);

    return result;
}

void
te_string_cut_beginning(te_string *str, size_t len)
{
    assert(str != NULL);
    if (len > str->len)
        len = str->len;

    str->len -= len;

    if (str->ptr != NULL)
    {
        memmove(str->ptr, str->ptr + len, str->len);
        str->ptr[str->len] = '\0';
    }
}

void
te_string_cut(te_string *str, size_t len)
{
    assert(str != NULL);
    if (len > str->len)
        len = str->len;
    assert(str->len >= len);
    str->len -= len;
    if (str->ptr != NULL)
        str->ptr[str->len] = '\0';
}

/* See description in te_string.h */
te_errno
te_string_from_te_dbuf(te_string *testr, struct te_dbuf *dbuf)
{
    te_errno rc;

    rc = te_dbuf_append(dbuf, "", 1);
    if (rc != 0)
        return rc;

    testr->ext_buf = FALSE;
    testr->ptr = (char *)dbuf->ptr;
    testr->size = dbuf->size;
    testr->len = dbuf->len - 1; /* -1 is for ignoring null terminator */

    /* Forget about dbuf's memory ownership */
    dbuf->ptr = NULL;
    dbuf->size = 0;
    te_dbuf_reset(dbuf);

    return 0;
}

void
te_substring_find(te_substring_t *substr, const char *str)
{
    char *ch;

    if (!te_substring_is_valid(substr))
        return;

    ch = strstr(substr->base->ptr + substr->start, str);
    if (ch == NULL)
    {
        substr->start = SIZE_MAX;
        substr->len = 0;
    }
    else
    {
       substr->start = ch - substr->base->ptr;
       substr->len = strlen(str);
    }

    return;
}

te_errno
te_substring_replace(te_substring_t *substr, const char *str)
{
    te_errno rc;
    te_string tail = TE_STRING_INIT;

    if (substr->start + substr->len > substr->base->len)
    {
        ERROR("Substring position out of bounds");
        return TE_EINVAL;
    }

    // TODO: Rewrite this using te_string_reserve, memmove and memcpy
    rc = te_string_append(&tail, "%s",
                          substr->base->ptr + substr->start + substr->len);
    if (rc != 0)
        return rc;

    te_string_cut(substr->base, substr->base->len - substr->start);
    rc = te_string_append(substr->base, "%s%s", str, tail.ptr);
    if (rc != 0)
    {
        te_string_free(&tail);
        return rc;
    }

    substr->start += strlen(str);
    substr->len = 0;

    te_string_free(&tail);
    return 0;
}

void
te_substring_advance(te_substring_t *substr)
{
    substr->start += substr->len;
    substr->len = 0;
}

void
te_substring_limit(te_substring_t *substr, const te_substring_t *limit)
{
    substr->len = limit->start - substr->start;
}

te_errno
te_string_replace_all_substrings(te_string *str, const char *new,
                                 const char *old)
{
    te_substring_t iter = TE_SUBSTRING_INIT(str);
    te_errno rc = 0;

    while (1)
    {
        te_substring_find(&iter, old);

        if (!te_substring_is_valid(&iter))
            break;

        rc = te_substring_replace(&iter, new);
        if (rc != 0)
        {
            ERROR("Failed to replace '%s' to '%s'", new, old);
            break;
        }
    }

    return rc;
}
