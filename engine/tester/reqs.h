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

#ifndef __TE_TESTER_REQS_H__
#define __TE_TESTER_REQS_H__

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif 

#include "te_defs.h"


/** Types of expression elements */
typedef enum reqs_expr_type {
    TESTER_REQS_EXPR_VALUE,     /**< Simple value */
    TESTER_REQS_EXPR_NOT,       /**< Logical 'not' */
    TESTER_REQS_EXPR_AND,       /**< Logical 'and' */
    TESTER_REQS_EXPR_OR,        /**< Logical 'or' */
} reqs_expr_type;

/** Element of the requirements expression */
typedef struct reqs_expr {
    reqs_expr_type  type;               /**< Type of expression element */
    union {
        char               *value;      /**< Simple value */
        struct reqs_expr   *unary;      /**< Unary expression */
        struct {
            struct reqs_expr   *lhv;        /**< Left hand value */
            struct reqs_expr   *rhv;        /**< Right hand value */
        } binary;                       /**< Binary expression */
    } u;                                /**< Type specific data */
} reqs_expr;

/** Target requirements expression */
typedef reqs_expr target_reqs_expr;


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
struct test_params;


/**
 * Parse requirements expression.
 *
 * @param str       String to be parsed
 * @param expr      Location for pointer to parsed expression
 *
 * @return Status code.
 */
extern int tester_reqs_expr_parse(const char *str, reqs_expr **expr);

/**
 * Create a new target requirement and insert it using logical 'and'
 * with current target.
 *
 * @param targets   Location of the targer requirements expression
 * @param req       String requirement
 *
 * @return Status code.
 * @retval 0        Success.
 * @retval ENOMEM   Memory allocation failure.
 */
extern int tester_new_target_reqs(reqs_expr **targets, const char *req);

/**
 * Create binary operation expression.
 *
 * @param type      Type of binary operation
 * @param lhv       Left hand value
 * @param rhv       Right hand value
 *
 * @return Pointer to the result or NULL
 */
extern reqs_expr *reqs_expr_binary(reqs_expr_type type,
                                   reqs_expr *lhv, reqs_expr *rhv);

/**
 * Free requirements expression.
 *
 * @param p         Expression to be freed
 */
extern void tester_reqs_expr_free(reqs_expr *p);

/**
 * Free requirements expression (non-recursive).
 *
 * @param p         Expression to be freed without subexpressions
 */
extern void tester_reqs_expr_free_nr(reqs_expr *p);

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
 * @param test      Test to be checked
 * @param params    List of real test parameters
 * @param quiet     Be quiet
 *
 * @retval TRUE     Run is required
 * @retval FALSE    Run is not required
 */
extern te_bool tester_is_run_required(const struct tester_ctx  *ctx,
                                      const struct run_item    *test,
                                      const struct test_params *params,
                                      te_bool                   quiet);

#endif /* !__TE_TESTER_REQS_H__ */
