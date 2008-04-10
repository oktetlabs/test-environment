/** @file
 * @brief Tester Subsystem
 *
 * Test path definitions.
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

#ifndef __TE_TESTER_TEST_PATH_H__
#define __TE_TESTER_TEST_PATH_H__

#include "te_queue.h"
#include "te_errno.h"

#include "tester_defs.h"
#include "tester_conf.h"
#include "tester_run.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Types of test paths.
 *
 * @attention Enum items have to be in the same order as corresponding
 *            options TESTER_OPT_*.
 */
typedef enum test_path_type {
    TEST_PATH_RUN,          /**< Test(s) to run */
    TEST_PATH_RUN_FORCE,    /**< Test(s) to run with values which are
                                 not mentioned in description */
    TEST_PATH_RUN_FROM,     /**< Start position of test(s) to run */
    TEST_PATH_RUN_TO,       /**< End position of test(s) to run */
    TEST_PATH_RUN_EXCLUDE,  /**< Tests to be excluded (not run) */

    TEST_PATH_VG,           /**< Test(s) be debugged using Valgrind */
    TEST_PATH_GDB,          /**< Test(s) be debugged using GDB */

    TEST_PATH_MIX,
    TEST_PATH_MIX_VALUES,
    TEST_PATH_MIX_ARGS,
    TEST_PATH_MIX_TESTS,
    TEST_PATH_MIX_ITERS,
    TEST_PATH_MIX_SESSIONS,
    TEST_PATH_NO_MIX,
    TEST_PATH_FAKE,
} test_path_type;

/** Style of the test path matching */
typedef enum test_path_match {
    TEST_PATH_EXACT,    /**< Exact match */
    TEST_PATH_GLOB,     /**< Glob-style match */
} test_path_match;


/** Test argument with set of values */
typedef struct test_path_arg {
    TAILQ_ENTRY(test_path_arg) links;   /**< List links */

    char               *name;   /**< Parameter name */
    test_path_match     match;  /**< Match style */
    tqh_strings         values; /**< List of values */
} test_path_arg;

/** Element of the test path */
typedef struct test_path_item {
    TAILQ_ENTRY(test_path_item) links;  /**< List links */

    TAILQ_HEAD(, test_path_arg) args;   /**< Arguments */

    char           *name;       /**< Path item name */
    unsigned int    select;     /**< Number of the iteration to run */
    unsigned int    step;       /**< Length of the cycle to apply number
                                     of the iteration to run */
    unsigned int    iterate;    /**< How many times to run */
} test_path_item;

/** List of test path items */
typedef TAILQ_HEAD(test_path_items, test_path_item) test_path_items;

/** Test path */
typedef struct test_path {
    TAILQ_ENTRY(test_path)  links;  /**< List links */
    char                   *str;    /**< String representation */
    test_path_type          type;   /**< Type of the test path */
    test_path_items         head;   /**< Head of the path */
    testing_scenario        scen;   /**< Testing scenario */
} test_path;

/** List of test paths */
typedef TAILQ_HEAD(test_paths, test_path) test_paths;


/**
 * Parse test path specification.
 *
 * @param path          Path with not parsed string representation
 *
 * @return Status code.
 */
extern te_errno tester_test_path_parse(test_path *path);

/**
 * Create a new test path item and insert it into the list.
 *
 * @param paths     Root of test paths
 * @param path      Path content
 * @param type      Type of the path
 *
 * @return Status code.
 */
extern te_errno test_path_new(test_paths *paths, const char *path,
                              test_path_type type);

/**
 * Free list of test paths.
 *
 * @param paths     Root of test paths to be freed
 */
extern void test_paths_free(test_paths *paths);


/**
 * Process requested tests paths and create testing scenario.
 * It no test paths are specified, scenario to run all tests is created.
 *
 * @param cfgs              Configurations
 * @param paths             Paths specified by user
 * @param scenario          Location for testing scenario
 * @param all_by_default    If tests to run are not specified, run
 *                          all by default
 *
 * @return Status code.
 */
extern te_errno tester_process_test_paths(
                    const tester_cfgs *cfgs,
                    test_paths        *paths,
                    testing_scenario  *scenario,
                    te_bool            all_by_default);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_TEST_PATH_H__ */
