/** @file
 * @brief Tester Subsystem
 *
 * Test parameters definitions.
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

#ifndef __TE_ENGINE_TESTER_TEST_PARAMS_H__
#define __TE_ENGINE_TESTER_TEST_PARAMS_H__

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif 

#include "te_defs.h"


/** Test real parameter with specified value. */
typedef struct test_param {
    TAILQ_ENTRY(test_param) links;  /**< List links */

    char       *name;   /**< Parameter name */
    char       *value;  /**< Current parameter value */
    const char *req;    /**< Requirement ID or NULL */
    te_bool     clone;  /**< Clone this entry or not */
} test_param;

/** Head of the test real parameters list */
typedef TAILQ_HEAD(test_params, test_param) test_params;


#endif /* !__TE_ENGINE_TESTER_TEST_PARAMS_H__ */
