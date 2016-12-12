/** @file
 * @brief Testing Results Comparator: common
 *
 * Routines to work with strings.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
#endif

#include "logger_api.h"

#include "te_string.h"
#include "te_defs.h"

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

te_errno
te_string_append_va(te_string *str, const char *fmt, va_list ap)
{
    char    *s;
    size_t   rest;
    int      printed;
    te_bool  again;
    va_list  ap_start;

    if (str->ptr == NULL)
    {
        assert(!str->ext_buf);
        str->ptr = malloc(TE_STRING_INIT_LEN);
        if (str->ptr == NULL)
        {
            ERROR("%s(): Memory allocation failure", __FUNCTION__);
            return TE_ENOMEM;
        }
        str->size = TE_STRING_INIT_LEN;
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
                /* We are going to extend buffer */
                rest = printed + 1 /* '\0' */ + TE_STRING_EXTRA_LEN;
                str->size = str->len + rest;
                str->ptr = realloc(str->ptr, str->size);
                if (str->ptr == NULL)
                {
                    ERROR("%s(): Memory allocation failure", __FUNCTION__);
                    return TE_ENOMEM;
                }
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
