/** @file
 * @brief Testing Results Comparator: common
 *
 * Routines to work with TRC tags sets.
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_queue.h"
#include "logger_api.h"

#include "trc_tags.h"


/* See description in trc_tag.h */
te_errno
trc_add_tag(tqh_strings *tags, const char *name)
{
    tqe_string *p = calloc(1, sizeof(*p));

    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
        return TE_ENOMEM;
    }
    TAILQ_INSERT_TAIL(tags, p, links);
    p->v = strdup(name);
    if (p->v == NULL)
    {
        ERROR("strdup(%s) failed", name);
        return TE_ENOMEM;
    }
    return 0;
}

/**
 * Parse string with TRC tags and add them into the list.
 *
 * @param tags_str      String with tags
 * @param tags          List to add tags
 *
 * @return Status code.
 */
te_errno
trc_tags_str_to_list(tqh_strings *tags, char *tags_str)
{
    char *tag, *space;
    int   rc = 0;

    for (tag = tags_str + strspn(tags_str, " ");
         rc == 0 && tag != NULL && *tag != '\0';
         tag = (space == NULL) ? NULL : (space + strspn(space, " ")))
    {
        space = strchr(tag, ' ');
        if (space != NULL)
            *space++ = '\0';
        rc = trc_add_tag(tags, tag);
    }
    return rc;
}


void
te_string_free(te_string *str)
{
    str->len = str->size = 0;
    free(str->ptr);
    str->ptr = NULL;
}

te_errno
te_string_append(te_string *str, const char *fmt, ...)
{
    char       *s;
    size_t      rest;
    int         printed;
    va_list     ap;
    te_bool     again;

    if (str->ptr == NULL)
    {
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

        va_start(ap, fmt);
        printed = vsnprintf(s, rest, fmt, ap);
        assert(printed >= 0);
        va_end(ap);

        if ((size_t)printed >= rest)
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
    assert(str->len >= 0);
    if (len > str->len)
        len = str->len;
    assert(str->len >= len);
    str->len -= len;
    str->ptr[str->len] = '\0';
}
