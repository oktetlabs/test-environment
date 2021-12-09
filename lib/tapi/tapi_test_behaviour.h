/** @file
 * @brief Test Behaviour API
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 *
 */

#ifndef __TE_TAPI_TEST_BEHAVIOUR_H__
#define __TE_TAPI_TEST_BEHAVIOUR_H__

#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * List of test behaviour switches.
 */
typedef struct test_behaviour {
    te_bool wait_on_fail; /**< Wait before going to cleanup in case of
                           * test failure */
    te_bool wait_on_cleanup; /**< Wait before going to cleanup regardless of
                              * the test result. If both wait_on_fail and
                              * this one are set - wait will be done
                              * just once */
    te_bool log_stack;    /**< Enable log stack collection */

    te_bool log_test_fail_state;  /**< Log state/substate of the
                                       test on failure */

    te_bool log_all_rpc; /**< If @c TRUE, all RPC calls should be logged
                              even if @b silent or @b silent_pass is set
                              to @c TRUE for an RPC server. */

    te_bool cleanup_fd_leak_check; /**< After closing FD in
                                        CLEANUP_RPC_CLOSE(), check with
                                        rpc_fstat() that FD is really
                                        closed */

    te_bool cleanup_fd_close_enforce_libc; /**< If enabled, use libc-version
                                                of close() in
                                                CLEANUP_RPC_CLOSE() */
    unsigned int prologue_sleep; /**< How many sencond to sleep in prologue */

    te_bool fail_verdict;   /**< If enabled, log error verdict from
                                 TEST_FAIL() and CLEANUP_TEST_FAIL(),
                                 otherwise just an error */
    te_bool rpc_fail_verdict;    /**< If enabled, any unexpected failure of
                                      RPC call resulting in jump to cleanup
                                      will print verdict */

    te_bool use_chk_funcs; /**< If @c TRUE, in RPC calls for read(), recv(),
                                recvfrom(), poll(), ppoll() call their
                                __[function]_chk() variants on TA */

    unsigned int iface_toggle_delay_ms; /**< How many (msec) to wait after
                                         * downing an interface before bringing
                                         * it up */
} test_behaviour;

/**
 * Storage for test behaviour switches.
 *
 * This variable is introduced to make possible checking for test behaviours
 * outside of main().
 * It is assumed that this variable is filled once on test start
 * and after that it can only be read, so no mutex is used to
 * protect it.
 */
extern test_behaviour test_behaviour_storage;

/**
 * Get behavoiur switch
 */
#define TEST_BEHAVIOUR(name_)                   \
    (test_behaviour_storage. name_)

/**
 * Fill test behaviour structure based on /local/test values. Ignores
 * missing instances and sets corresponding switch to false, but
 * logs a WARN.
 *
 * @param behaviour          Behaviour structure to be filled in.
 */
extern void test_behaviour_get(test_behaviour *behaviour);

/**
 * This variable is set to @c TRUE when the verdict about FD not
 * being properly closed is printed the first time from
 * CLEANUP_RPC_CLOSE(), to avoid printing it multiple times when
 * closing many FDs.
 *
 * It is used to implement "cleanup_fd_leak_check" behaviour.
 */
extern te_bool fd_not_closed_verdict;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* __TE_TAPI_TEST_BEHAVIOUR_H__ */
