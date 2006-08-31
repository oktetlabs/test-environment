/** @file
 * @brief Testing Results Comparator
 *
 * Implementation of comparison routines.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TRC compare"

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_defs.h"
#include "te_queue.h"
#include "te_test_result.h"

#include "te_trc.h"


/**
 * Compare two results.
 */
static te_bool
te_test_results_equal(const te_test_result *lhv,
                      const te_test_result *rhv)
{
    const te_test_verdict  *v1;
    const te_test_verdict  *v2;

    if (lhv->status != rhv->status)
        return FALSE;

    for (v1 = lhv->verdicts.tqh_first, v2 = rhv->verdicts.tqh_first;
         v1 != NULL && v2 != NULL && strcmp(v1->str, v2->str) == 0;
         v1 = v1->links.tqe_next, v2 = v2->links.tqe_next);
    
    return (v1 == NULL) && (v2 == NULL);
}

/* See the description in te_trc.h */
te_bool
trc_is_result_expected(const trc_exp_result *expected,
                       const te_test_result *obtained)
{
    const trc_exp_result_entry *p;

    for (p = expected->results.tqh_first;
         p != NULL;
         p = p->links.tqe_next)
    {
        if (te_test_results_equal(obtained, &p->result))
            return TRUE;
    }

    return FALSE;
}
