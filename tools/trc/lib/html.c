/** @file
 * @brief Testing Results Comparator
 *
 * Helper functions to prepare HTML reports.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_errno.h"
#include "logger_api.h"

#include "trc_html.h"
#include "re_subst.h"


#define WRITE_STR(str) \
    do {                                                    \
        if (str != NULL && fputs(str, f) == EOF)            \
        {                                                   \
            rc = te_rc_os2te(errno);                        \
            assert(rc != 0);                                \
            ERROR("Writing to the file failed: %r", rc);    \
            goto cleanup;                                   \
        }                                                   \
    } while (0)


/* See the description in trc_html.h */
te_errno
te_test_result_to_html(FILE *f, const te_test_result *result)
{
    te_errno                rc = 0;
    const te_test_verdict  *v;

    assert(f != NULL);
    if (result == NULL)
    {
        WRITE_STR("<span>");
        WRITE_STR(te_test_status_to_str(TE_TEST_UNSPEC));
        WRITE_STR("</span>");
        return 0;
    }

    WRITE_STR("<span>");
    WRITE_STR(te_test_status_to_str(result->status));
    WRITE_STR("</span>");

    if (TAILQ_EMPTY(&result->verdicts))
        return 0;

    WRITE_STR("<br/><br/>");
    TAILQ_FOREACH(v, &result->verdicts, links)
    {
        WRITE_STR("<span>");
        WRITE_STR(v->str);
        if (TAILQ_NEXT(v, links) != NULL)
            WRITE_STR("; ");
        WRITE_STR("</span><br/>");
    }

cleanup:
    return rc;
}

/* See the description in trc_html.h */
te_errno
trc_test_result_to_html(FILE *f, const trc_exp_result_entry *result)
{
    te_errno    rc;

    assert(f != NULL);
    assert(result != NULL);

    rc = te_test_result_to_html(f, &result->result);
    if (rc != 0)
        return rc;

    if (result->key != NULL)
    {
        fputs("<br/>Key: ", f);
        trc_re_key_substs(TRC_RE_KEY_URL, result->key, f);
    }
    if (result->notes != NULL)
    {
        fputs("<br/>Notes: ", f);
        fputs(result->notes, f);
    }

    return 0;
}

/* See the description in trc_html.h */
te_errno
trc_exp_result_to_html(FILE *f, const trc_exp_result *result,
                       unsigned int flags, tqh_strings *tags)
{
    te_errno                    rc = 0;
    const trc_exp_result_entry *p;

    UNUSED(flags);

    assert(f != NULL);

    if (result == NULL)
    {
#if 0
        WRITE_STR("(see iterations)");
#endif
        return 0;
    }
    if (result->tags_str != NULL)
    {
        char *str = result->tags_str;
        char *tag_p;
        char *tag_q;
        char *tag_str;
        tqe_string *tag;

        while (*str != '\0')
        {
            tag_p = NULL;
            TAILQ_FOREACH(tag, tags, links)
            {
                tag_q = strstr(str, tag->v);
                if (tag_q == NULL)
                    continue;

                if ((tag_p == NULL) || (tag_p > tag_q))
                {
                    tag_p = tag_q;
                    tag_str = tag->v;
                }
            }
            if (tag_p == NULL)
            {
                WRITE_STR(str);
                break;
            }

            if (tag_p > str)
            {
                char tag_fmt[16];
                snprintf(tag_fmt, 16, "%%.%"TE_PRINTF_SIZE_T"ds",
                         tag_p - str);
                fprintf(f, tag_fmt, str);
            }

            fprintf(f, "<b>%s</b>", tag_str);
            str = tag_p + strlen(tag_str);
        }
        WRITE_STR("<br/><br/>");
    }
    for (p = TAILQ_FIRST(&result->results);
         p != NULL && rc == 0;
         p = TAILQ_NEXT(p, links))
    {
        if (p != TAILQ_FIRST(&result->results))
            WRITE_STR("<br/><br/>");
        rc = trc_test_result_to_html(f, p);
    }

cleanup:
    return rc;
}

#include "trc_tags.h" /* for te_string */
#include "te_alloc.h"

/**
 * Split long string containing values separated by commas
 *
 * @param s     String
 *
 * @return Splitted string or NULL
 */
static te_string *
split_long_string(char *s, unsigned int max_len)
{
    size_t      len = 0;
    const char *p0 = s;
    const char *p1 = strchr(p0, ',');
    const char *p2 = strchr(p1 + 1, ',');
    te_string  *value = TE_ALLOC(sizeof(*value));

    if (value == NULL)
    {
        ERROR("%s(): memory allocation failed", __FUNCTION__);
        return NULL;
    }

    do {
        while (p2 != NULL && p2 - p0 < (int)max_len)
        {
            p1 = p2;
            p2 = strchr(p1 + 1, ',');
        }
        if (p2 == NULL && p2 - p0 < (int)max_len)
        {
            te_string_append(value, p0);
            break;
        }

        len += p1 - p0 + 1;
        te_string_append(value, p0);
        te_string_cut(value, value->len - len);
        te_string_append(value, "<wbr/>");
        len += 6;
        assert(len == value->len);
        p0 = p1 + 1;
        p1 = strchr(p0, ',');
        if (p1 == NULL)
        {
            te_string_append(value, p0);
            break;
        }
    } while (TRUE);

    return value;
}


/* See the description in trc_html.h */
te_errno
trc_test_iter_args_to_html(FILE *f, const trc_test_iter_args *args,
                           unsigned int flags)
{
    trc_test_iter_arg  *p;

    UNUSED(flags);

    TAILQ_FOREACH(p, &args->head, links)
    {
        if (strlen(p->value) > 80 && strpbrk(p->value, " \n\r\t") == 0 &&
            strchr(p->value, ',') != NULL)
        {
            te_string *value = split_long_string(p->value, 80);

            if (value == NULL)
                return TE_ENOMEM;

            fprintf(f, "<a name=\"param\">%s</a>=<a name=\"%s_val\">"
                    "%s</a><br/>",p->name, p->name,
                    value->ptr);
            te_string_free(value);
        }
        else
            fprintf(f, "<a name=\"param\">%s</a>=<a name=\"%s_val\">"
                    "%s</a><br/>", p->name, p->name,
                    p->value);
    }

    return 0;
}

/* See the description in trc_html.h */
te_errno
trc_report_iter_args_to_html(FILE *f, const trc_report_argument *args,
                             unsigned int args_n, unsigned int max_len,
                             unsigned int flags)
{
    unsigned int i;

    UNUSED(flags);

    for (i = 0; i < args_n; i++)
    {
        if (args[i].variable)
            continue;

        if (strlen(args[i].value) > 80 &&
            strpbrk(args[i].value, " \n\r\t") == 0 &&
            strchr(args[i].value, ',') != NULL)
        {
            te_string *value = split_long_string(args[i].value, max_len);

            if (value == NULL)
                return TE_ENOMEM;

            fprintf(f, "<a name=\"param\">%s</a>=<a name=\"%s_val\">"
                    "%s</a><br/>", args[i].name, args[i].name,
                    value->ptr);

            te_string_free(value);
        }
        else
            fprintf(f, "<a name=\"param\">%s</a>=<a name=\"%s_val\">"
                    "%s</a><br/>", args[i].name, args[i].name,
                    args[i].value);
    }

    return 0;
}
