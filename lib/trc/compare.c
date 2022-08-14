/** @file
 * @brief Testing Results Comparator
 *
 * Implementation of comparison routines.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "logger_api.h"

#include "te_trc.h"


/**
 * Compare two results.
 */
te_bool
te_test_results_equal(const te_test_result *lhv,
                      const te_test_result *rhv)
{
    const te_test_verdict  *v1;
    const te_test_verdict  *v2;

    if (lhv->status != rhv->status)
        return FALSE;

    for (v1 = TAILQ_FIRST(&lhv->verdicts),
         v2 = TAILQ_FIRST(&rhv->verdicts);
         v1 != NULL && v2 != NULL && strcmp(v1->str, v2->str) == 0;
         v1 = TAILQ_NEXT(v1, links), v2 = TAILQ_NEXT(v2, links));

#if 0
    if (v1 != NULL && v2 != NULL)
    {
        unsigned int   i;

        for (i = 0; v1->str[i] == v2->str[i]; ++i);
        RING("Diff at %u\n'%s'\n'%s'", i, v1->str + i, v2->str + i);
    }
#endif

    return (v1 == NULL) && (v2 == NULL);
}

/* See the description in te_trc.h */
const trc_exp_result_entry *
trc_is_result_expected(const trc_exp_result *expected,
                       const te_test_result *obtained)
{
    const trc_exp_result_entry *p;

    assert(expected != NULL);
    assert(obtained != NULL);

    for (p = TAILQ_FIRST(&expected->results);
         p != NULL && !te_test_results_equal(obtained, &p->result);
         p = TAILQ_NEXT(p, links));

    return p;
}

/* See the description in te_trc.h */
te_bool
trc_is_exp_result_skipped(const trc_exp_result *result)
{
    const trc_exp_result_entry *p;

    assert(result != NULL);

    TAILQ_FOREACH(p, &result->results, links)
    {
        if (p->result.status != TE_TEST_SKIPPED ||
            TAILQ_FIRST(&p->result.verdicts) != NULL)
        {
            return FALSE;
        }
    }

    return TRUE;
}
