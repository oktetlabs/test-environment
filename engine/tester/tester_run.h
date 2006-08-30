/** @file
 * @brief Tester Subsystem
 *
 * Run scenario and related data types representation.
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
 * $Id: tester_conf.h 28469 2006-05-26 07:58:32Z arybchik $
 */

#ifndef __TE_TESTER_RUN_SCENARIO_H__
#define __TE_TESTER_RUN_SCENARIO_H__

#if WITH_TRC
#include "te_trc.h"
#else
#define te_trc_db   void
#endif

#include "te_queue.h"

#include "tester_reqs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward */
struct test_requirements;

/** Test argument with specified value. */
typedef struct test_iter_arg {
    const char                 *name;   /**< Parameter name */
    const char                 *value;  /**< Current parameter value */
    struct test_requirements    reqs;   /**< Associated requirements */
} test_iter_arg;


/** Act of the testing scenario */
typedef struct testing_act {
    TAILQ_ENTRY(testing_act)    links;  /**< Linked list links */

    unsigned int first; /**< Number of the first item */
    unsigned int last;  /**< Number of the last item */

    unsigned int flags; /**< Flags (tester_flags) for the act */

} testing_act;

/** Testing scenario is a sequence of acts */
typedef TAILQ_HEAD(testing_scenario, testing_act) testing_scenario;


/**
 * Free act of the testing scenario.
 *
 * @param act           Act to be freed
 */
extern void scenario_act_free(testing_act *act);

/**
 * Free the testing scenario.
 *
 * @param scenario      Scenario to be freed
 */
extern void scenario_free(testing_scenario *scenario);


/**
 * Allocate a new act in testing scenario.
 *
 * @param first         The first item
 * @param last          The last item
 * @param flags         Act flags
 *
 * @return Status code.
 */
extern testing_act *scenario_new_act(const unsigned int first,
                                     const unsigned int last,
                                     const unsigned int flags);

/**
 * Add new act in testing scenario.
 *
 * @param scenario      Testing scenario
 * @param first         The first item
 * @param last          The last item
 * @param flags         Act flags
 *
 * @return Status code.
 */
extern te_errno scenario_add_act(testing_scenario *scenario,
                                 const unsigned int first,
                                 const unsigned int last,
                                 const unsigned int flags);

/**
 * Copy act to scenario.
 *
 * @param scenario      Testing scenario
 * @param act           Act to be copied
 *
 * @return Status code.
 */
extern te_errno scenario_act_copy(testing_scenario  *scenario,
                                  const testing_act *act);

/**
 * Copy one scenario to another.
 *
 * @param dst           Destination scenario
 * @param src           Source scenario
 *
 * @return Status code.
 */
extern te_errno scenario_copy(testing_scenario       *dst,
                              const testing_scenario *src);

/**
 * Generate scenario by bit mask.
 *
 * @param scenario      Scenario to add acks
 * @param offset        Initial offset
 * @param bm            Bit mask with acts to run
 * @param bm_len        Length of the bit mask
 * @param bit_weight    Weight of single bit in the mask
 *
 * @return Status code.
 */
extern te_errno scenario_by_bit_mask(testing_scenario *scenario,
                                     unsigned int      offset,
                                     const uint8_t    *bm,
                                     unsigned int      bm_len,
                                     unsigned int      bit_weight);

/**
 * Append one scenario to another specified number of times.
 *
 * @param scenario      Scenario to append to
 * @param subscenario   Scenario to be iterated and appended
 * @param iterate       Number of iterations
 *
 * @return Status code.
 */
extern te_errno scenario_append(testing_scenario *scenario,
                                testing_scenario *subscenario,
                                unsigned int      iterate);


/**
 * Update testing scenario to have all items starting from 0 up to
 * the item with maximum identifier.
 *
 * @param scenario      Testing scenario
 * @param from          Minimum identifier
 */
extern void scenario_apply_to(testing_scenario *scenario,
                              unsigned int from);

/**
 * Update testing scenario to have all items starting from the mininum
 * identifier up to the @a to identifier.
 * 
 * @param scenario      Testing scenario
 * @param to            Maximum identifier
 */
extern void scenario_apply_from(testing_scenario *scenario,
                                unsigned int to);

/**
 * Add flags to all acts of the testing scenario.
 * 
 * @param scenario      A testing scenario
 * @param flags         Flags to add
 */
extern void scenario_add_flags(testing_scenario *scenario,
                               const unsigned int flags);

/**
 * Glue testing scenario acts with equal flags.
 *
 * @param scenario      Sorted testing scenario to be processed
 */
extern void scenario_glue(testing_scenario *scenario);

/**
 * Exclude one testing scenario from another.
 *
 * @param scenario      Sorted testing scenario to exclude from
 * @param exclude       Sorted testing scenario to be excluded
 *
 * @return Status code.
 */
extern void scenario_exclude(testing_scenario *scenario,
                             testing_scenario *exclude);

/**
 * Merge one testing scenario description into another.
 *
 * @param scenario      Sorted testing scenario to merge to
 * @param add           Sorted testing scenario to merge in
 * @param flags         Flags which have to have all item from @a add
 *
 * @return Status code.
 */
extern te_errno scenario_merge(testing_scenario *scenario,
                               testing_scenario *add,
                               unsigned int flags);

/**
 * Apply flags from scenario @a flags to testing scenario.
 *
 * @param scenario      Testing scenario (may be not sorted)
 * @param flags         Testing scenario with flags to apply
 *
 * @return Status code.
 */
extern te_errno scenario_apply_flags(testing_scenario *scenario,
                                     const testing_scenario *flags);


typedef enum testing_direction {
    TESTING_STOP,
    TESTING_FORWARD,
    TESTING_BACKWARD,
} testing_direction;

/**
 * Move on the testing scenario.
 *
 * @param act           Location of the current testing act
 * @param act_id        Location of the current action ID
 * @param step          Step
 *
 * @return Testing scenario movement direction.
 */
extern testing_direction scenario_step(const testing_act **act,
                                       unsigned int       *act_id,
                                       unsigned int        step);

/**
 * String representation of testing scenario.
 *
 * @param scenario      Testing scenario
 *
 * @return Pointer to static buffer with testing scenario as string.
 */
extern const char * scenario_to_str(const testing_scenario *scenario);


/* Forwards */
struct tester_cfgs;

/**
 * Run test configurations.
 *
 * @param scenario      Testing scenario
 * @param targets       Target requirements
 * @param cfgs          Tester configurations
 * @param trc_db        TRC database handle
 * @param flags         Flags
 *
 * @return Status code.
 */
extern te_errno tester_run(const testing_scenario   *scenario,
                           const struct logic_expr  *targets,
                           const struct tester_cfgs *cfgs,
                           const te_trc_db          *trc_db,
                           const unsigned int        flags);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_RUN_SCENARIO_H__ */
