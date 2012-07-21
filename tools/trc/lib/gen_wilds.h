/** @file
 * @brief Testing Results Comparator: update tool
 *
 * Definition of structures and functions used to generate wildcards
 * from full subset structure build for a given test. Full subset
 * structure contains for each possible iteration record (including
 * wildcards) the set of iterations described by it. The task is to
 * select as small as possible number of (wildcard) iteration records
 * enough to describe the test.
 *
 * Copyright (C) 2005-2012 Test Environment authors (see file AUTHORS
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_errno.h"

/** Set of numbers */
typedef struct set {
    int     *els;       /**< Array of numbers in the set */
    int      num;       /**< Number of elements in the set */
    int      max;       /**< Maximum number of elements in array */
    int      n_diff;    /**< Number of elements in the set not
                             covered by other sets in solution
                             (used by greedy set cover algorithm) */
    int      id;        /**< Set ID used to find set of iterations related
                             to this abstract set structure */
} set;

/** Definition of problem to be solved */
typedef struct problem {
    set     *sets;      /**< Sets */
    int      set_num;   /**< Number of sets */
    int      set_max;   /**< Maximum number of sets can be included
                             before reallocation of array is required */
    int      elm_num;   /**< Total number of different elements in all
                             the sets */

    int     *sol;       /**< Solution of problem (array of set numbers) */
    int      sol_num;   /**< Number of sets in solution */
} problem;

/**
 * Free problem structure.
 *
 * @param p     Problem structure pointer
 */
extern void problem_free(problem *p);

/** Algorithms types to be used for solving a problem */
typedef enum alg_type {
    ALG_EXACT_COV_DLX,      /**< DLX algorithm for exact set cover */
    ALG_EXACT_COV_GREEDY,   /**< Greedy alrorithm for exact set cover */
    ALG_EXACT_COV_BOTH,     /**< Use both DLX and greedy algorithms for
                                 exact set cover, then select the best
                                 solution */
    ALG_SET_COV_GREEDY,     /**< Greedy algorithm for set cover */
} alg_type;

/**
 * Solve the problem used required algorithm(s).
 *
 * @param p     Problem structure pointer
 * @param at    Algorithm type
 *
 * @return Status code
 */
extern te_errno get_fss_solution(problem *p, alg_type at);
