/** @file
 * @brief Tester Subsystem
 *
 * Routines to deal with testing scenario.
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
 *
 * @todo: Definition of exclude.
 */
typedef enum {
    /* operation codes */
    TESTING_ACT_OR,
    TESTING_ACT_SUBTRACT,
    TESTING_ACT_EXCLUDE,
} testing_act_op;

#ifndef INTRVL_TQ_POSTV_BIG
#define INTRVL_TQ_POSTV_BIG  (1 << (8 * sizeof(int) - 2))
#endif


/**
 * ibar: Please, provide function description.
 */
static void
get_operation_result(testing_act *seg0, testing_act *seg1,
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

    /* there have to be an overlap */
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
 * ibar: Please, provide function description.
 */
static testing_act *
get_next_with_gaps(testing_act **elm_p, testing_act *gap,
                   unsigned int *the_end_p)
{
    unsigned int    the_end = *the_end_p;
    testing_act    *elm = *elm_p;
    testing_act    *next = NULL;


    if (elm == NULL)
        return NULL;

    if (the_end < elm->first) /* pre-gap */
    {
        gap->first = the_end;
        gap->last = elm->first - 1;
        next = gap;
    }
    else if (the_end == elm->first) /* elm itself */
    {
        next = elm;
    }
    else /* move to the next elm and get either pre-gap or elm itself */
    {
        elm = elm->links.tqe_next;
        next = get_next_with_gaps(&elm, gap, &the_end);
    }

    if (next == NULL )
    {   
        /* final gap to "infinity" */
        gap->first = the_end;
        gap->last = INTRVL_TQ_POSTV_BIG - 1;
        next = gap;
    }

    *the_end_p  = next->last + 1;    
    *elm_p      = elm;

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
    testing_act    *next0, *next1;   /* it points to gap or elm */
    unsigned int    the_end_0, the_end_1; /* how far we've already gone */
    unsigned int    need_get_next1 = 1;   /* flag */
    testing_act    *overlap;         /* overlap of two intervals */
    testing_act    *overlap_grow;    /* cumulative overlap */

    testing_scenario    h_rslt; /* resulting list */
    te_bool             result_replace; /* put result into h0 */


    result_replace = (h_rslt_p == h0); /* result should replace h0 */

    TAILQ_INIT(&h_rslt);

    /* a single obj to hold parameters of all list0 gaps */
    if ((gap0 = calloc(1, sizeof(*gap0))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);
    the_end_0 = 0;
    elm0 = h0->tqh_first;
    next0 = gap0;

    /* a single obj to hold parameters of all list1 gaps */
    if ((gap1 = calloc(1, sizeof(*gap1))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);
    the_end_1 = 0;
    elm1 = h1->tqh_first;
    next1 = gap1;

    /* a single obj to pass parameters of all overlaps */
    if ((overlap = calloc(1, sizeof(*overlap))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);
    
    /*
     * will be allocated again each time the current one is finished
     * and inserted at the end of the resulting list
     */
    if ((overlap_grow = calloc(1, sizeof(*overlap_grow))) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);
    
    
    while (1) /* interate the fist set of intervals */
    {
        elm0_prev = elm0;
        next0 = get_next_with_gaps(&elm0, gap0, &the_end_0);
        if (result_replace && elm0_prev != elm0)
        {   
            /* we empty list0 one entry at a time as we can */
            TAILQ_REMOVE(h0, elm0_prev, links);
        }
        if (next0 == NULL)
            break;

        while (1) /* interate the second set of intervals */
        {
            if (need_get_next1 == 0)
            {
                need_get_next1 = 1; /* reset on each pass */
            }
            else
            {
                next1 = get_next_with_gaps(&elm1, gap1, &the_end_1);
                if (next1 == NULL)
                    break;
            }

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
                    need_get_next1 = 0;
                }
                break; /* need to step forward next0 now */
            }           
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
        free(h0);
    }
    
    *h_rslt_p = h_rslt;

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
                 const unsigned int flags)
{
    testing_act *act = malloc(sizeof(*act));

    if (act != NULL)
    {
        act->first = first;
        act->last = last;
        act->flags = flags;
    }

    return act;
}

/* See the description in tester_run.h */
te_errno
scenario_add_act(testing_scenario *scenario,
                 const unsigned int first,
                 const unsigned int last,
                 const unsigned int flags)
{
    testing_act *act = scenario_new_act(first, last, flags);

    if (act == NULL)
        return TE_ENOMEM;

    TAILQ_INSERT_TAIL(scenario, act, links);

    VERB("New testing scenario %p act: (%u,%u,0x%x)", scenario,
         act->first, act->last, act->flags);

    return 0;
}

/* See the description in tester_run.h */
te_errno
scenario_act_copy(testing_scenario *scenario, const testing_act *act)
{
    return scenario_add_act(scenario, act->first, act->last, act->flags);
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
                     unsigned int bit_weight)
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
                                      0);
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
                              0);
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
scenario_add_flags(testing_scenario *scenario, const unsigned int flags)
{
    testing_act *act;

    TAILQ_FOREACH(act, scenario, links)
    {
        act->flags |= flags;
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
void
scenario_exclude(testing_scenario *scenario, testing_scenario *exclude)
{
#if 0
    rc = testing_scenarios_op(scenario, exclude, scenario,
                              TESTER_ACT_EXCLUDE);
#else
    UNUSED(scenario);
    UNUSED(exclude);
#endif
}

/* See the description in tester_run.h */
te_errno
scenario_merge(testing_scenario *scenario, testing_scenario *add,
               unsigned int flags)
{
    testing_act *cur;
    testing_act *prev;
    testing_act *add_p;
    testing_act *add_p_next;

    if (TAILQ_EMPTY(scenario))
    {
        scenario_append(scenario, add, 1);
        scenario_add_flags(scenario, flags);
        return 0;
    }

    for (cur = TAILQ_FIRST(scenario), prev = NULL, add_p = TAILQ_FIRST(add);
         add_p != NULL;
         add_p = add_p_next)
    {
        add_p_next = TAILQ_NEXT(add_p, links);

        while (cur != NULL && add_p->first > cur->last)
        {
            prev = cur;
            cur = TAILQ_NEXT(cur, links);
        }

        if (cur == NULL)
        {
            /* New act is after the last act of the scenario */
            TAILQ_REMOVE(add, add_p, links);
            add_p->flags |= flags;
            TAILQ_INSERT_TAIL(scenario, add_p, links);
        }
        else if (cur->first > add_p->last)
        {
            /* New act is between two acts of the scenario */
            TAILQ_REMOVE(add, add_p, links);
            add_p->flags |= flags;
            TAILQ_INSERT_AFTER(scenario, prev, add_p, links);
        }
        else
        {
            /* New act has intersection with some acts of the scenario */
            assert(FALSE); /* TODO */
        }
    }
    return 0;
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
              unsigned int step)
{
    unsigned int    next_id;

    assert(act != NULL);
    if (*act == NULL)
    {
        VERB("step=%u -> STOP (nowhere)", *act, step);
        return TESTING_STOP;
    }

    assert(act_id != NULL);
    assert((*act_id >= (*act)->first) && (*act_id <= (*act)->last));

    next_id = *act_id + step;
    if (next_id <= (*act)->last)
    {
        *act_id = next_id;
        VERB("step=%u -> FORWARD", step);
        return TESTING_FORWARD;
    }

    while ((*act = TAILQ_NEXT(*act, links)) != NULL)
    {
        if ((*act)->first <= *act_id)
        {
            /* Move backward */
            *act_id = (*act)->first;
            VERB("step=%u -> BACKWARD", step);
            return TESTING_BACKWARD;
        }
        else if ((*act)->first >= next_id)
        {
            /* First ID of the action is greater or equal to next ID */
            *act_id = (*act)->first;
            VERB("step=%u -> FORWARD", step);
            return TESTING_FORWARD;
        }
        else if ((*act)->last >= next_id)
        {
            /* Next ID is in the middle of this action */
            *act_id = next_id;
            VERB("step=%u -> FORWARD", step);
            return TESTING_FORWARD;
        }
        else
        {
            /* All this action should be skipped */
            continue;
        }
    }

    VERB("step=%u -> STOP (end-of-scenario)", step);
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
                 "(%u,%u,0x%x)-", act->first, act->last, act->flags);
    }
    return buf;
}
