/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Routines to deal with testing scenario.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** Logging user name to be used here */
#define TE_LGR_USER     "Scenario"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "logger_api.h"

#include "tester_defs.h"
#include "tester_run.h"


/**
 * By example: Given two lists L0 and L1 each consisting of intervals
 * defined by testing_act:
 * L0: [1 AB 5] [ 8 B 13]  (A, B, etc. are flags-properties of intervals)
 * L1: [3 A  9] [11 A 15]
 * operation OR(merge) will produce the followint list:
 * [1 AB 5] [6 A 7] [8 AB 9] [10 B 10] [11 AB 13] [14 A 15]
 * As one can see, the gaps are "implied" intervals with property 0;
 *
 * Subtract: 0-flag=0; flag-0=flag; AB-B=A, thus: flag0^(flag0&flag1)
 * The result:
 * [1 AB 2] [3 B 5] [8 B 13]
 */
typedef enum {
    /* operation codes */
    TESTING_ACT_OR,
    TESTING_ACT_SUBTRACT,
} testing_act_op;

#ifndef INTRVL_TQ_POSTV_BIG
#define INTRVL_TQ_POSTV_BIG  (1 << (8 * sizeof(int) - 2))
#endif


/**
 * Given two overlapping intervals (segment0 and segment1), a new
 * interval is formed which is an overlap of the original two.
 * overlap->flags = seg0->flags (op_code) seg1->flags
 */
static void
get_operation_result(const testing_act *seg0, const testing_act *seg1,
                     testing_act *overlap, testing_act_op op_code)
{
    unsigned int    len_sum;
    int             twice_centr_dist; /* double the distance between
                                         the 2 centers */

    len_sum = (seg0->last - seg0->first) + (seg1->last - seg1->first);

    twice_centr_dist =
        (seg0->last + seg0->first) - (seg1->last + seg1->first);

    twice_centr_dist =
        (twice_centr_dist >= 0) ? twice_centr_dist : -twice_centr_dist;

    /* there has to be an overlap */
    assert((unsigned)twice_centr_dist <= len_sum);

    overlap->first = (seg0->first < seg1->first) ?
                         seg1->first : seg0->first;
    overlap->last = (seg0->last > seg1->last) ?
                        seg1->last : seg0->last;

    switch (op_code)
    {
        case TESTING_ACT_OR:
            overlap->flags = seg0->flags | seg1->flags;
            break;

        case TESTING_ACT_SUBTRACT:
            overlap->flags = seg0->flags ^ (seg0->flags & seg1->flags);
            break;

        default:
            assert(FALSE);
    }
}


/**
 * Consider a sequence of intervals with possible gaps:
 * [gap] elm [gap] elm ...
 * On entry, we have some already chosen elm (*elm_p) and
 * an index (*the_end_p) which could point
 * 1. to a gap before the chosen elm: then the gap is returned
 *    as "next" and the elm is unchanged.
 * 2. to the beginning of the elm: then the elm itself is returned.
 * 3. beyond the beginning of the elm: then
 *    *elm_p is updated to be the next real elm (not a gap) and
 *    either this next elm
 *    or a gap in front of it (if exists) is returned as "next".
 * In all cases, (*the_end_p) is updated to point to next->last + 1 .
 */
static testing_act *
get_next_with_gaps(testing_act **elm_p, testing_act *gap,
                   unsigned int *the_end_p)
{
    testing_act    *elm;
    testing_act    *next;

    assert(elm_p != NULL);
    assert(gap != NULL);
    assert(the_end_p != NULL);

    elm = *elm_p;
    if (elm == NULL) /* final gap to "infinity" */
    {
        if (*the_end_p == INTRVL_TQ_POSTV_BIG)
            return NULL;

        gap->first = *the_end_p;
        gap->last = INTRVL_TQ_POSTV_BIG - 1;
        next = gap;
    }
    else if (*the_end_p < elm->first) /* pre-gap */
    {
        gap->first = *the_end_p;
        gap->last = elm->first - 1;
        next = gap;
    }
    else if (*the_end_p == elm->first) /* elm itself */
    {
        next = elm;
    }
    else /* move to the next elm and get either pre-gap or elm itself */
    {
        *elm_p = TAILQ_NEXT(elm, links);
        next = get_next_with_gaps(elm_p, gap, the_end_p);
        assert(*the_end_p == next->last + 1);
    }

    *the_end_p = next->last + 1;

    return next;
}

/**
 * Given two lists of intervals (h0 and h1), the function produces
 * a list *h_rslt_p  which is the result of (h0 operation h1).
 * If h0 address is passed as h_rslt_p, elements of h0 are removed
 * and freed as soon as they are not needed for further processing.
 * At the end h0 (head itself) is also freed. On exit, *h_rslt_p
 * will contain the head of the new list (the result of the operation).
 *
 * @param h0        head of list0
 * @param h1        head of list1
 * @param h_rslt_p  an address to store a head of the resulting list.
 *
 * @return Status code.
 */
te_errno
testing_scenarios_op(testing_scenario *h0, testing_scenario *h1,
                     testing_scenario *h_rslt_p,
                     testing_act_op op_code)
{
    testing_act    *elm0_prev, *elm0, *elm1; /* real list entries */
    /* pseudo entries for gaps between the real intervals: */
    testing_act    *gap0, *gap1;
    testing_act    *next0, *next1 = NULL;   /* it points to gap or elm */
    unsigned int    the_end_0, the_end_1; /* how far we've already gone */
    te_bool         need_get_next1 = TRUE;   /* flag */
    testing_act    *overlap;         /* overlap of two intervals */
    testing_act    *overlap_grow;    /* cumulative overlap */

    testing_scenario    h_rslt; /* resulting list */
    te_bool             result_replace; /* put result into h0 */


    result_replace = (h_rslt_p == h0); /* result should replace h0 */

    TAILQ_INIT(&h_rslt);

    /* We have processed nothing yet */
    the_end_0 = the_end_1 = 0;

    /*
     * Start from the first element of the list0 and the list1
     * correspondingly (may be NULL)
     */
    elm0 = h0->tqh_first;
    elm1 = h1->tqh_first;

    /* a single obj to hold parameters of all list0 gaps */
    if ((gap0 = calloc(1, sizeof(*gap0))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);

    /* a single obj to hold parameters of all list1 gaps */
    if ((gap1 = calloc(1, sizeof(*gap1))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);

    /* a single obj to pass parameters of all overlaps */
    if ((overlap = calloc(1, sizeof(*overlap))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);

    /*
     * will be allocated again each time the current one is finished
     * and inserted at the end of the resulting list
     */
    if ((overlap_grow = calloc(1, sizeof(*overlap_grow))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);

    /*
     * Init, so it could only be either the first elm or the pregap.
     * Thus, definitely no worry about freeing elm0_prev
     */
    next0 = get_next_with_gaps(&elm0, gap0, &the_end_0);

    while (1) /* interate the fist set of intervals */
    {
        if (next0 == NULL)
            break;

        while (1) /* interate the second set of intervals */
        {
            if (!need_get_next1)
            {
                need_get_next1 = TRUE; /* reset on each pass */
            }
            else
            {
                next1 = get_next_with_gaps(&elm1, gap1, &the_end_1);
            }
            if (next1 == NULL)
                break;

            get_operation_result(next0, next1, overlap, op_code);

            if (overlap_grow->flags != overlap->flags)
            {
                /* a different overlap, push the previous into the list */
                if (overlap_grow->flags != 0)
                {
                    /* insert only non-blank intervals */
                    TAILQ_INSERT_TAIL(&h_rslt, overlap_grow, links);
                    /* need to alloc a new obj to accumulate overlaps: */
                    if ((overlap_grow = malloc(sizeof(*overlap_grow)))
                        == NULL)
                        return TE_RC(TE_TESTER, TE_ENOMEM);
                }
                /* start a new accumulation: */
                memcpy(overlap_grow, overlap, sizeof(*overlap_grow));
            }
            else /* extend the cumulative interval */
            {
                overlap_grow->last = overlap->last;
            }

            if (next1->last >= next0->last)
            {
                if (next1->last > next0->last)
                {
                    /* still have to process this next1 and new next0: */
                    need_get_next1 = FALSE;
                }
                break; /* need to step forward next0 now */
            }
        }

        /* step forward next0 */
        elm0_prev = elm0;
        next0 = get_next_with_gaps(&elm0, gap0, &the_end_0);

        if (result_replace && elm0_prev != elm0)
        {
            /* we empty list0, one entry at a time as we can */
            TAILQ_REMOVE(h0, elm0_prev, links);
            free(elm0_prev); /* assuming elm's are malloc'ed properly */
        }
    }

    /* Append the last overlap_grow if non-blank */
    if (overlap_grow->flags != 0)
    {
        TAILQ_INSERT_TAIL(&h_rslt, overlap_grow, links);
    }
    else
    {
        free(overlap_grow);
    }

    free(gap0);
    free(gap1);
    free(overlap);

    if (result_replace)
    {
        assert(h0->tqh_first == NULL);
    }

    TAILQ_CONCAT(h_rslt_p, &h_rslt, links);

    return 0;
}


/* See the description in tester_run.h */
void
scenario_act_free(testing_act *act)
{
    free(act);
}

/* See the description in tester_run.h */
void
scenario_free(testing_scenario *scenario)
{
    testing_act *act;

    while ((act = TAILQ_FIRST(scenario)) != NULL)
    {
        TAILQ_REMOVE(scenario, act, links);
        scenario_act_free(act);
    }
}


/* See the description in tester_run.h */
testing_act *
scenario_new_act(const unsigned int first, const unsigned int last,
                 const tester_flags flags)
{
    testing_act *act = malloc(sizeof(*act));

    if (act != NULL)
    {
        act->first = first;
        act->last = last;
        act->flags = flags;
        act->hash = NULL;
    }

    return act;
}

/* See the description in tester_run.h */
te_errno
scenario_add_act(testing_scenario *scenario,
                 const unsigned int first,
                 const unsigned int last,
                 const tester_flags flags,
                 const char *hash)
{
    testing_act *act = scenario_new_act(first, last, flags);

    if (act == NULL)
        return TE_ENOMEM;

    act->hash = hash;

    TAILQ_INSERT_TAIL(scenario, act, links);

    VERB("New testing scenario %p act: (%u,%u,0x%"
         TE_PRINTF_TESTER_FLAGS "x)", scenario,
         act->first, act->last, act->flags);

    return 0;
}

/* See the description in tester_run.h */
te_errno
scenario_act_copy(testing_scenario *scenario, const testing_act *act)
{
    return scenario_add_act(scenario, act->first, act->last, act->flags,
                            act->hash);
}

/* See the description in tester_run.h */
te_errno
scenario_copy(testing_scenario *dst, const testing_scenario *src)
{
    te_errno            rc;
    const testing_act  *act;

    TAILQ_FOREACH(act, src, links)
    {
        rc = scenario_act_copy(dst, act);
        if (rc != 0)
            return rc;
    }
    return 0;
}


/* See the description in tester_run.h */
te_errno
scenario_by_bit_mask(testing_scenario *scenario, unsigned int offset,
                     const uint8_t *bm, unsigned int bm_len,
                     unsigned int bit_weight, const char *hash)
{
    te_errno        rc;
    unsigned int    bit;
    te_bool         started = FALSE;
    unsigned int    start = 0; /* Just to make compiler happy */

    ENTRY("scenario=%p offset=%u bm=%p bm_len=%u bit_weight=%u",
          scenario, offset, bm, bm_len, bit_weight);

    for (bit = 0; bit < bm_len; ++bit)
    {
        if (started)
        {
            if (!bit_mask_is_set(bm, bit))
            {
                started = FALSE;
                rc = scenario_add_act(scenario,
                                      offset + start * bit_weight,
                                      offset + bit * bit_weight - 1,
                                      0, hash);
                if (rc != 0)
                    return rc;
            }
        }
        else
        {
            if (bit_mask_is_set(bm, bit))
            {
                started = TRUE;
                start = bit;
            }
        }
    }
    if (started)
    {
        rc = scenario_add_act(scenario,
                              offset + start * bit_weight,
                              offset + bm_len * bit_weight - 1,
                              0, hash);
        if (rc != 0)
            return rc;
    }
    return 0;
}

/* See the description in tester_run.h */
te_errno
scenario_append(testing_scenario *scenario, testing_scenario *subscenario,
                unsigned int iterate)
{
    te_errno        rc;
    unsigned int    i;
    testing_act    *act;

    /* Copy subscenario to scenario (iterate - 1) times */
    for (i = 0; i < iterate - 1; ++i)
    {
        rc = scenario_copy(scenario, subscenario);
        if (rc != 0)
            return rc;
    }

    /* Move subscenario to scenario */
    while ((act = TAILQ_FIRST(subscenario)) != NULL)
    {
        TAILQ_REMOVE(subscenario, act, links);
        TAILQ_INSERT_TAIL(scenario, act, links);
    }

    return 0;
}

/* See the description in tester_run.h */
void
scenario_apply_to(testing_scenario *scenario, unsigned int from)
{
    testing_act *act = TAILQ_FIRST(scenario);
    testing_act *cur;
    testing_act *next;

    if (act != NULL)
    {
        act->first = from;
        next = TAILQ_NEXT(act, links);
        while ((cur = next) != NULL)
        {
            assert(act->last < cur->last);
            act->last = cur->last;
            next = TAILQ_NEXT(cur, links);
            TAILQ_REMOVE(scenario, cur, links);
            scenario_act_free(cur);
        }
        if (act->first > act->last)
        {
            TAILQ_REMOVE(scenario, act, links);
            scenario_act_free(act);
        }
    }
}

/* See the description in tester_run.h */
void
scenario_apply_from(testing_scenario *scenario, unsigned int to)
{
    testing_act *act = TAILQ_FIRST(scenario);
    testing_act *cur;
    testing_act *next;

    if (act != NULL)
    {
        act->last = to;
        next = TAILQ_NEXT(act, links);
        while ((cur = next) != NULL)
        {
            next = TAILQ_NEXT(cur, links);
            TAILQ_REMOVE(scenario, cur, links);
            scenario_act_free(cur);
        }
        if (act->first > act->last)
        {
            TAILQ_REMOVE(scenario, act, links);
            scenario_act_free(act);
        }
    }
}

/* See the description in tester_run.h */
void
scenario_add_flags(testing_scenario *scenario, const tester_flags flags)
{
    testing_act *act;

    TAILQ_FOREACH(act, scenario, links)
    {
        act->flags |= flags;
    }
}

/* See the description in tester_run.h */
void
scenario_del_acts_by_flags(testing_scenario *scenario, tester_flags flags)
{
    testing_act *cur;
    testing_act *nxt;

    if (flags == 0)
        return;

    TAILQ_FOREACH_SAFE(cur, scenario, links, nxt)
    {
        if ((cur->flags & flags) == flags)
        {
            TAILQ_REMOVE(scenario, cur, links);
            scenario_act_free(cur);
        }
    }
}

/* See the description in tester_run.h */
void
scenario_del_acts_with_no_flags(testing_scenario *scenario)
{
    testing_act *cur;
    testing_act *nxt;

    TAILQ_FOREACH_SAFE(cur, scenario, links, nxt)
    {
        if (cur->flags == 0)
        {
            TAILQ_REMOVE(scenario, cur, links);
            scenario_act_free(cur);
        }
    }
}

/* See the description in tester_run.h */
void
scenario_glue(testing_scenario *scenario)
{
    testing_act *cur;
    testing_act *next;

    TAILQ_FOREACH_SAFE(cur, scenario, links, next)
    {
        assert(next->first > cur->last);
        if (cur->flags == next->flags && next->first - cur->last == 1)
        {
            cur->last = next->last;
            TAILQ_REMOVE(scenario, next, links);
            scenario_act_free(next);
            next = cur;
        }
    }
}

/* See the description in tester_run.h */
te_errno
scenario_exclude(testing_scenario *scenario, testing_scenario *exclude,
                 tester_flags flags)
{
    te_errno    rc;

    scenario_add_flags(exclude, flags);

    rc = testing_scenarios_op(scenario, exclude, scenario,
                              TESTING_ACT_SUBTRACT);
    if (rc == 0)
        scenario_del_acts_with_no_flags(scenario);

    return rc;
}

/* See the description in tester_run.h */
te_errno
scenario_merge(testing_scenario *scenario, testing_scenario *add,
               tester_flags flags)
{
    scenario_add_flags(add, flags);

    return testing_scenarios_op(scenario, add, scenario,
                                TESTING_ACT_OR);
}

/* See the description in tester_run.h */
te_errno
scenario_apply_flags(testing_scenario *scenario,
                     const testing_scenario *flags)
{
    const testing_act  *flag_act;
    testing_act        *act;
    testing_act        *new_act;

    TAILQ_FOREACH(flag_act, flags, links)
    {
        TAILQ_FOREACH(act, scenario, links)
        {
            if (act->first <= flag_act->last &&
                act->last >= flag_act->first)
            {
                /* Acts have intersection */
                if ((act->flags & flag_act->flags) == flag_act->flags)
                {
                    /* 'act' already has all flags from 'flag_act' */
                }
                else if (act->first >= flag_act->first &&
                         act->last <= flag_act->last)
                {
                    /* 'act' is subset of 'flags_act' */
                    act->flags |= flag_act->flags;
                }
                else
                {
                    if (act->first < flag_act->first)
                    {
                        /* Split current act into two parts */
                        new_act = scenario_new_act(flag_act->first,
                                                   act->last,
                                                   act->flags);
                        if (new_act == NULL)
                            return TE_ENOMEM;
                        TAILQ_INSERT_AFTER(scenario, act, new_act, links);
                        act->last = flag_act->first - 1;
                        /* Move to the second fragment */
                        act = new_act;
                    }
                    if (act->last > flag_act->last)
                    {
                        /* Split current act into two parts */
                        new_act = scenario_new_act(flag_act->last + 1,
                                                   act->last,
                                                   act->flags);
                        if (new_act == NULL)
                            return TE_ENOMEM;
                        TAILQ_INSERT_AFTER(scenario, act, new_act, links);
                        act->last = flag_act->last;
                        act->flags |= flag_act->flags;
                        /* Move to the last fragment of current act */
                        act = new_act;
                    }
                    else
                    {
                        act->flags |= flag_act->flags;
                    }
                }
            }
        }
    }

    return 0;
}


/* See the description in tester_run.h */
testing_direction
scenario_step(const testing_act **act, unsigned int *act_id,
              unsigned int start_id, unsigned int next_id,
              te_bool skip)
{
    assert(act != NULL);
    if (*act == NULL)
    {
        VERB("next_id=%u -> STOP (nowhere)", next_id);
        return TESTING_STOP;
    }

    assert(act_id != NULL);
    assert((*act_id >= (*act)->first) && (*act_id <= (*act)->last));

    if (next_id < *act_id)
    {
        /* next_id is before the current act */
        VERB("next_id=%u -> FORWARD", next_id);
        return TESTING_FORWARD;
    }
    else if (next_id >= (*act)->first &&
             next_id <= (*act)->last)
    {
        /* next_id is within current act */
        *act_id = next_id;
        VERB("next_id=%u -> FORWARD", next_id);
        return TESTING_FORWARD;
    }

    /*
     * next_id is after the current scenario act, so
     * move to the next act.
     */

    /*
     * If requested, this loop skips all the next
     * acts within [start_id, next_id), until act
     * outside of this interval is encountered.
     * It allows to skip all the acts from the
     * skipped session instead of trying to rerun
     * it for each of them.
     */
    while ((*act = TAILQ_NEXT(*act, links)) != NULL)
    {
        if (!skip)
            break;

        if ((*act)->first < start_id ||
            (*act)->last >= next_id)
            break;
    }

    if (*act == NULL)
    {
        VERB("next_id=%u -> STOP", next_id);
        return TESTING_STOP;
    }

    *act_id = (*act)->first;
    if (next_id <= *act_id)
    {
        /*
         * The next act is at or after next_id - continue
         * walking forward.
         */
        VERB("next_id=%u -> FORWARD", next_id);
        return TESTING_FORWARD;
    }
    else if (next_id > *act_id)
    {
        /*
         * The next act is before next_id - move
         * backward in testing configuration.
         * This function should not be called
         * again before testing moved to <= *act_id,
         * see run_this_item() usage.
         */
        VERB("next_id=%u -> BACKWARD", next_id);
        return TESTING_BACKWARD;
    }

    /* Execution should not reach here */
    assert(0);
    return TESTING_STOP;
}


/* See the description in tester_run.h */
const char *
scenario_to_str(const testing_scenario *scenario)
{
    static char buf[1024];

    const testing_act  *act;

    *buf = '\0';
    TAILQ_FOREACH(act, scenario, links)
    {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "(%u,%u,0x%" TE_PRINTF_TESTER_FLAGS "x)-",
                 act->first, act->last, act->flags);
    }
    return buf;
}
