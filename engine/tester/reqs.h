/** @file
 * @brief Tester Subsystem
 *
 * Requirements definitions.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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

#ifndef __TE_ENGINE_TESTER_REQS_H__
#define __TE_ENGINE_TESTER_REQS_H__

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif 

#include "te_defs.h"


/** Element of the list of requirements */
typedef struct test_requirement {
    TAILQ_ENTRY(test_requirement)   links;  /**< List links */
    char       *id;         /**< Identifier */
    char       *ref;        /**< Reference */
    te_bool     exclude;    /**< Should test on requirement be 
                                 included or excluded */
    te_bool     sticky;     /**< Is it sticky requirement? */
} test_requirement;

typedef TAILQ_HEAD(test_requirements, test_requirement) test_requirements;


/* Forwards */
struct tester_ctx;
struct test_params;


/**
 * Create a new requirement and insert it in the list.
 *
 * @param reqs      A list of requirements
 * @param req       String requirement
 *
 * @return Status code.
 * @retval 0        Success.
 * @retval ENOMEM   Memory allocation failure.
 */
extern int test_requirement_new(test_requirements *reqs, const char *req);

/**
 * Clone list of requirements.
 *
 * @param reqs      List of requirements to be cloned
 * @param new_reqs  New list of requirements (must be initialized)
 *
 * @return Status code.
 */
extern int test_requirements_clone(const test_requirements *reqs,
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
 * @param ctx       Tester context
 * @param reqs      Requirements coverted by the test
 * @param params    List of real test parameters
 *
 * @retval TRUE     Run is required
 * @retval FALSE    Run is not required
 */
extern te_bool tester_is_run_required(struct tester_ctx *ctx,
                                      test_requirements *reqs,
                                      const struct test_params *params);

#endif /* !__TE_ENGINE_TESTER_REQS_H__ */
