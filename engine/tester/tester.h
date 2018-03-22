/** @file
 * @brief Tester Subsystem
 *
 * Application main header file
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 *
 * $Id:
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

/** Tester global context */
typedef struct tester_global {
    unsigned int        rand_seed;  /**< Random seed */
    unsigned int        flags;      /**< Flags (enum tester_flags) */
    tester_cfgs         cfgs;       /**< Configuration files */
    test_suites_info    suites;     /**< Information about test suites */
    test_paths          paths;      /**< Paths specified by caller */
    logic_expr         *targets;    /**< Target requirements expression */
    te_trc_db          *trc_db;     /**< TRC database handle */
    tqh_strings         trc_tags;   /**< TRC tags */
    testing_scenario    scenario;   /**< Testing scenario */
    test_requirements   reqs;       /**< List of requirements known by
                                     *   the tester */

    cmd_monitor_descrs  cmd_monitors;   /**< Command monitors specifier via
                                             command line */
} tester_global;

extern tester_global tester_global_context;

#endif  /* __TE_TESTER_H__ */
