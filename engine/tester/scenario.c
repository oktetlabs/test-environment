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

    while ((act = scenario->tqh_first) != NULL)
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

    for (act = src->tqh_first; act != NULL; act = act->links.tqe_next)
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
    while ((act = subscenario->tqh_first) != NULL)
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
    testing_act *act = scenario->tqh_first;
    testing_act *cur;
    testing_act *next;

    if (act != NULL)
    {
        act->first = from;
        next = act->links.tqe_next;
        while ((cur = next) != NULL)
        {
            assert(act->last < cur->last);
            act->last = cur->last;
            next = cur->links.tqe_next;
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
    testing_act *act = scenario->tqh_first;
    testing_act *cur;
    testing_act *next;

    if (act != NULL)
    {
        act->last = to;
        next = act->links.tqe_next;
        while ((cur = next) != NULL)
        {
            next = cur->links.tqe_next;
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

    for (act = scenario->tqh_first; act != NULL; act = act->links.tqe_next)
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

    for (cur = scenario->tqh_first; cur != NULL; cur = next)
    {
        next = cur->links.tqe_next;
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
    UNUSED(scenario);
    UNUSED(exclude);
    assert(FALSE);
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

    if (scenario->tqh_first == 0)
    {
        scenario_append(scenario, add, 1);
        scenario_add_flags(scenario, flags);
        return 0;
    }

    for (cur = scenario->tqh_first, prev = NULL, add_p = add->tqh_first;
         add_p != NULL;
         add_p = add_p_next)
    {
        add_p_next = add_p->links.tqe_next;

        while (cur != NULL && add_p->first > cur->last)
        {
            prev = cur;
            cur = cur->links.tqe_next;
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

    for (flag_act = flags->tqh_first;
         flag_act != NULL;
         flag_act = flag_act->links.tqe_next)
    {
        for (act = scenario->tqh_first;
             act != NULL;
             act = act->links.tqe_next)
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

    while ((*act = (*act)->links.tqe_next) != NULL)
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
    for (act = scenario->tqh_first; act != NULL; act = act->links.tqe_next)
    {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "(%u,%u,0x%x)-", act->first, act->last, act->flags);
    }
    return buf;
}
