/** @file
 * @brief Tester Subsystem
 *
 * Test parameters (variables, arguments) iteration definitions.
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

#ifndef __TE_TESTER_ITERS_H__
#define __TE_TESTER_ITERS_H__

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif 

#include "te_defs.h"

#include "test_params.h"


/** Test parameters iteration entry */
typedef struct test_param_iteration {
    TAILQ_ENTRY(test_param_iteration)   links;  /**< List links */
    test_params                 params;     /**< List of parameters */
    const struct test_params   *base;       /**< Base parameters */
    te_bool                     has_reqs;   /**< Do parameters have
                                                 associated requirements? */
} test_param_iteration;

/** List of test parameters iterations */
typedef TAILQ_HEAD(test_param_iterations, test_param_iteration)
    test_param_iterations;


/**
 * Allocate new test parameter iteration with empty set of parameters.
 *
 * @return Pointer to allocated memory or NULL.
 */
extern test_param_iteration *test_param_iteration_new(void);

/**
 * Clone existing test parameters iteration.
 *
 * @param i         Existing iteration
 * @param clone_all Clone all parameters or marked only
 *                  (used to remove non-handdown variables)
 *
 * @return Clone of the passed iteration or NULL.
 */
extern test_param_iteration *test_param_iteration_clone(
                                 const test_param_iteration *i,
                                 te_bool clone_all);

/**
 * Free test parameters iteration.
 *
 * @param p     Iteration to be freed
 */
extern void test_param_iteration_free(test_param_iteration *p);

/**
 * Free list of test parameters iterations.
 *
 * @param iters     List of test parameters iterations to be freed    
 */
extern void test_param_iterations_free(test_param_iterations *iters);

/**
 * Free a test parameter.
 *
 * @param p     Test parameter to be freed 
 */
extern void test_param_free(test_param *param);

#endif /* !__TE_TESTER_ITERS_H__ */
