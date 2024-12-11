/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Application main header file
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TESTER_H__
#define __TE_TESTER_H__

#include "te_config.h"

#include "tester_build.h"
#include "tester_conf.h"
#include "test_path.h"
#include "tester_term.h"
#include "tester_run.h"
#include "type_lib.h"
#include "tester_flags.h"
#include "tester_cmd_monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Tester global default arguments */
typedef struct script_def_arg {
    TAILQ_ENTRY(script_def_arg)  links;  /**< List links */
    char                        *id;     /**< Identifier */
    char                        *name;   /**< Script name */
    test_vars_args               args;   /**< Arguments */
} script_def_arg;

/** Head of the list of default values for scripts */
typedef TAILQ_HEAD(script_def_args, script_def_arg)
    script_def_args;

/** Tester global context */
typedef struct tester_global {
    unsigned int        rand_seed;  /**< Random seed */
    tester_flags        flags;      /**< Flags (see tester_flags.h) */
    tester_cfgs         cfgs;       /**< Configuration files */
    test_suites_info    suites;     /**< Information about test suites */
    test_paths          paths;      /**< Paths specified by caller */
    logic_expr         *targets;    /**< Target requirements expression */
    te_trc_db          *trc_db;     /**< TRC database handle */
    tqh_strings         trc_tags;   /**< TRC tags */
    testing_scenario    scenario;   /**< Testing scenario */
    test_requirements   reqs;       /**< List of requirements known by
                                     *   the tester */
    script_def_args     def_args;   /**< List of script default arguments
                                      *   for the package */
    char               *verdict;    /**< Verdict to stop tester run on */

    /** Percentage of all test iterations to choose randomly */
    double dial;

    cmd_monitor_descrs  cmd_monitors;   /**< Command monitors specifier via
                                             command line */
} tester_global;

extern tester_global tester_global_context;

/**
 * Log TRC tags as an MI message.
 *
 * Tag names and values are split at the first colon, no additional checks
 * are performed.
 *
 * @param trc_tags          TRC tags.
 *
 * @return Status code.
 */
extern te_errno tester_log_trc_tags(const tqh_strings *trc_tags);

#endif  /* __TE_TESTER_H__ */
