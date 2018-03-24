/** @file
 * @brief Tester Subsystem
 *
 * Tester flags definitions.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TESTER_FLAGS_H__
#define __TE_TESTER_FLAGS_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Flags of the Tester global context */
enum tester_flags {
    TESTER_VERBOSE      = (1 << 0),   /**< The first level of verbosity:
                                           output of run path to stdout */
    TESTER_VVERB        = (1 << 1),   /**< The second level of verbosity:
                                           additional output of test IDs
                                           on run paths */
    TESTER_OUT_EXP      = (1 << 2),   /**< Additional output of status
                                           together with TRC verdict */
    TESTER_OUT_TIN      = (1 << 3),   /**< Additional output of TINs */

    TESTER_FAKE         = (1 << 4),   /**< Fake run */

    TESTER_QUIET_SKIP   = (1 << 5),   /**< Quiet skip of tests */
    TESTER_OUT_TEST_PARAMS  = (1 << 6),  /**< Output test params to
                                              the console */
    TESTER_VERB_SKIP   = (1 << 7),   /**< Print 'skipped' in case of
                                          silent mode
                                          is on by default */


    TESTER_NO_BUILD     = (1 << 8),   /**< Don't build any Test Suites */
    TESTER_NO_RUN       = (1 << 9),   /**< Don't run any Test Suites */
    TESTER_NO_CS        = (1 << 10),  /**< Don't interact with
                                           Configurator */
    TESTER_NO_CFG_TRACK = (1 << 11),  /**< Don't track configuration 
                                           changes */
    TESTER_NO_LOGUES    = (1 << 12),  /**< Prologues/epilogues are
                                           disabled */
    TESTER_NO_SIMULT    = (1 << 13),  /**< Disable simultaneousness */
    TESTER_NO_TRC       = (1 << 14),  /**< Don't use TRC */

    TESTER_VALGRIND     = (1 << 16),  /**< Run scripts under valgrind */
    TESTER_GDB          = (1 << 17),  /**< Run scripts under GDB in
                                           interactive mode */
    TESTER_RUNPATH      = (1 << 18),  /**< Flag to be used on run path
                                           only */
    TESTER_RUN          = (1 << 19),  /**< End of run path */

    TESTER_FORCE_RUN    = (1 << 20),  /**< Run tests regardless requested 
                                           path */

    TESTER_MIX_VALUES   = (1 << 21),
    TESTER_MIX_ARGS     = (1 << 22),
    TESTER_MIX_TESTS    = (1 << 23),
    TESTER_MIX_ITERS    = (1 << 24),
    TESTER_MIX_SESSIONS = (1 << 25),

    TESTER_LOG_IGNORE_RUN_NAME = (1 << 26), /**< Force testing flow logging
                                                 to ignore run item name */

    TESTER_LOG_REQS_LIST = (1 << 27), /**< Log list of known requirements to
                                       * the log file */

    TESTER_INLOGUE      = (1 << 28),  /**< Is in prologue/epilogue */

    TESTER_INTERACTIVE  = (1 << 29),  /**< Interactive mode */
    TESTER_SHUTDOWN     = (1 << 30),  /**< Shutdown test scenario */
    TESTER_BREAK_SESSION = (1 << 31), /**< Break test session on Ctrl-C */
};

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_FLAGS_H__ */
