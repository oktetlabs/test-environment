/** @file
 * @brief Tester Subsystem
 *
 * Requirements definitions.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TESTER_REQS_H__
#define __TE_TESTER_REQS_H__

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "logic_expr.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Element of the list of requirements */
typedef struct test_requirement {
    TAILQ_ENTRY(test_requirement)   links;  /**< List links */
    char       *id;         /**< Identifier */
    char       *ref;        /**< Reference */
    te_bool     sticky;     /**< Is it sticky requirement? */
} test_requirement;

/** Head of the list of requirements */
typedef TAILQ_HEAD(test_requirements, test_requirement) test_requirements;


/* Forwards */
struct tester_ctx;
struct run_item;
struct test_iter_arg;


/**
 * Create a new target requirement and insert it using logical 'and'
 * with current target.
 *
 * @param targets   Location of the targer requirements expression
 * @param req       String requirement
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval TE_ENOMEM    Memory allocation failure.
 */
extern te_errno tester_new_target_reqs(logic_expr **targets,
                                       const char  *req);


/**
 * Clone list of requirements.
 *
 * @param reqs      List of requirements to be cloned
 * @param new_reqs  New list of requirements (must be initialized)
 *
 * @return Status code.
 */
extern te_errno test_requirements_clone(const test_requirements *reqs,
                                        test_requirements *new_reqs);

/**
 * Free list of requirements.
 *
 * @param reqs      List of requirements to be freed
 */
extern void test_requirements_free(test_requirements *reqs);

/**
 * Determine whether running of the test required.
 *
 * @param targets       Target requirements expression
 * @parma sticky_reqs   List of collected sticky requirements
 * @param test          Test to be checked
 * @param args          Array with test iteration arguments (run item
 *                      has fixed number of arguments)
 * @param flags         Current Tester context flags
 * @param quiet         Be quiet
 *
 * @retval TRUE         Run is required
 * @retval FALSE        Run is not required
 */
extern te_bool tester_is_run_required(
                   const logic_expr           *targets,
                   const test_requirements    *sticky_reqs,
                   const struct run_item      *test,
                   const struct test_iter_arg *args,
                   unsigned int                flags,
                   te_bool                     quiet);

/**
 * Add sticky requirements to the context.
 *
 * @param sticky_reqs   List of requirements in the current context
 * @param reqs          List of requirements
 *
 * @return Status code.
 */
extern te_errno tester_get_sticky_reqs(test_requirements *sticky_reqs,
                                       const test_requirements *reqs);

/**
 * Print requirements expression to static buffer.
 *
 * @param expr      Requirements expression
 *
 * @return Pointer to the static buffer with printed expression.
 */
extern const char *tester_reqs_expr_to_string(const logic_expr *expr);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_REQS_H__ */
