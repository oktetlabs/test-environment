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
        if (fputs(str, f) == EOF)                           \
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
        WRITE_STR(te_test_status_to_str(TE_TEST_UNSPEC));
        return 0;
    }

    WRITE_STR(te_test_status_to_str(result->status));

    if (TAILQ_EMPTY(&result->verdicts))
        return 0;

    WRITE_STR("<br/><br/>");
    TAILQ_FOREACH(v, &result->verdicts, links)
    {
        WRITE_STR(v->str);
        if (TAILQ_NEXT(v, links) != NULL)
            WRITE_STR("; ");
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
        trc_re_key_substs(result->key, f);
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
                       unsigned int flags)
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
        WRITE_STR("<b>");
        WRITE_STR(result->tags_str);
        WRITE_STR("</b><br/><br/>");
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


/* See the description in trc_html.h */
te_errno
trc_test_iter_args_to_html(FILE *f, const trc_test_iter_args *args,
                           unsigned int flags)
{
    trc_test_iter_arg  *p;

    UNUSED(flags);

    TAILQ_FOREACH(p, &args->head, links)
    {
        fprintf(f, "%s=%s<BR/>", p->name, p->value);
    }

    return 0;
}
