/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API
 *
 * Macros to be used in tests. The header must be included from test
 * sources only. It is allowed to use the macros only from @b main()
 * function of the test.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_TEST_H__
#define __TE_TAPI_TEST_H__

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <ctype.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif

#include <math.h> /* For the TEST_GET_DOUBLE_PARAM error handling */

#include "rcf_api.h"
#include "conf_api.h"
#include "logger_api.h"
#include "logger_ten.h"
#include "tapi_jmp.h"
#include "tapi_test_behaviour.h"
#include "tapi_test_log.h"
#include "tapi_test_run_status.h"
#include "asn_impl.h"
#include "asn_usr.h"

#include "te_tools.h"
#include "te_param.h"
#include "te_kvpair.h"
#include "te_vector.h"
#include "te_compound.h"
#include "log_bufs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_ts_tapi_test Test
 * @ingroup te_ts_tapi
 * @{
 */

#ifndef TE_LGR_USER
/* Tests must define TE_TEST_NAME to be used as 'te_lgr_entity'. */
#ifndef TE_TEST_NAME
#error TE_TEST_NAME must be defined before include of tapi_test.h
#endif
#endif

/**
 * Define empty list of test-specific variables
 * if test does not care about them.
 */
#ifndef TEST_START_VARS
#define TEST_START_VARS
#endif

/** Define empty test start procedure if test does not care about it. */
#ifndef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC
#endif

/** Define empty test end procedure if test does not care about it. */
#ifndef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC do { } while (0)
#endif

#ifndef TEST_ON_JMP_DO_IF_SUCCESS
#define TEST_ON_JMP_DO_IF_SUCCESS do { } while (0)
#endif

#ifndef TEST_ON_JMP_DO_IF_FAILURE
#define TEST_ON_JMP_DO_IF_FAILURE do { } while (0)
#endif

/**
 * Template action to be done on jump in the test.
 */
#define TEST_ON_JMP_DO \
    do {                                                             \
        if (result == EXIT_SUCCESS || result == EXIT_FAILURE)        \
        {                                                            \
            switch (TE_RC_GET_ERROR(jmp_rc))                         \
            {                                                        \
                case TE_EOK:                                         \
                    result = EXIT_SUCCESS;                           \
                    break;                                           \
                case TE_ESKIP:                                       \
                    result = TE_EXIT_SKIP;                           \
                    break;                                           \
                case TE_EFAIL:                                       \
                default:                                             \
                    result = EXIT_FAILURE;                           \
                    break;                                           \
            }                                                        \
        }                                                            \
        if (result == EXIT_SUCCESS)                                  \
            TEST_ON_JMP_DO_IF_SUCCESS;                               \
        else if (result == EXIT_FAILURE)                             \
            TEST_ON_JMP_DO_IF_FAILURE;                               \
                                                                     \
        if (TEST_BEHAVIOUR(log_test_fail_state) &&                   \
            result == EXIT_FAILURE)                                  \
        {                                                            \
            const char *s = te_test_fail_state_get();                \
            if (s != NULL)                                           \
                TEST_ARTIFACT("STATE: %s", s);                       \
                                                                     \
            s = te_test_fail_substate_get();                         \
            if (s != NULL)                                           \
                TEST_ARTIFACT("SUBSTATE: %s", s);                    \
        }                                                            \
                                                                     \
        /* Behaviour switches handling section */                    \
        if (TEST_BEHAVIOUR(wait_on_cleanup) ||                       \
            (TEST_BEHAVIOUR(wait_on_fail) && result == EXIT_FAILURE))\
        {                                                            \
            printf("\n\nWe're about to jump to cleanup, "            \
                   "but tester config kindly asks \n"                \
                   "us to wait for a key to be pressed. \n\n"        \
                   "Press any key to continue...\n");                \
            getchar();                                               \
        }                                                            \
        TEST_STEP("Test cleanup");                                   \
        goto cleanup;                                                \
    } while (0)

/**
 * Template action to be done on jump in TEST_START_SPECIFIC.
 */
#define TEST_ON_JMP_DO_SPECIFIC \
    do {                                                             \
        if (result == EXIT_SUCCESS || result == EXIT_FAILURE) \
        result = (TE_RC_GET_ERROR(jmp_rc) == TE_EOK) ? EXIT_SUCCESS  \
                                                     : EXIT_FAILURE; \
        goto cleanup_specific;                                       \
    } while (0)

/**
 * Macro to add behaviour switches in code that does not call TEST_START. It
 * should not exist yet it does
 */
#define TEST_BEHAVIOUR_DEF \
    test_behaviour __behaviour; \
    test_behaviour_get(&__behaviour)

/**
 * The first action of any test @b main() function.
 *
 * Variable @a rc and @a result are defined.
 *
 * @note @b main() must get @a argc and @a argv parameters.
 * @note To define a set of variables accessed in each test, you should
 * define TEST_START_VARS macro as the list of additional variables
 * To define test-specific action define TEST_START_SPECIFIC macro
 */
#define TEST_START \
    int         rc;                                                 \
    int         result = EXIT_FAILURE;                              \
    TEST_START_VARS                                                 \
                                                                    \
    assert(tapi_test_run_status_get() == TE_TEST_RUN_STATUS_OK);    \
                                                                    \
    /* 'rc' may be unused in the test */                            \
    UNUSED(rc);                                                     \
                                                                    \
    /* Shift programm name */                                       \
    /* test_get_filename_param() relies on it */                    \
    argc--;                                                         \
    argv++;                                                         \
                                                                    \
    te_log_init(TE_TEST_NAME, ten_log_message);                     \
                                                                    \
    /*                                                              \
     * Install SIGINT signal handler to exit() with failure status, \
     * if test is terminated by user by Ctrl-C.                     \
     */                                                             \
    (void)signal(SIGINT, te_test_sig_handler);                      \
    /*                                                              \
     * Install SIGUSR1 signal handler to call TEST_FAIL().          \
     * In comparison with SIGINT, SIGUSR1 does not lead to testing  \
     * campaign termination.                                        \
     */                                                             \
    (void)signal(SIGUSR1, te_test_sig_handler);                     \
    (void)signal(SIGUSR2, te_test_sig_handler);                     \
                                                                    \
    /*                                                              \
     * Get te_test_id parameter which is required to associate      \
     * further logs with the test (including steps and jump point   \
     * management logs).                                            \
     */                                                             \
    te_test_id = test_get_test_id(argc, argv);                      \
    if ((te_test_id == TE_LOG_ID_UNDEFINED) &&                      \
        !test_is_cmd_monitor(argc, argv))                           \
        return EXIT_FAILURE;                                        \
    TEST_STEP("Test start");                                        \
    /*                                                              \
     * Set jump point to cleanup_specific label to handle           \
     * if TEST_START_SPECIFIC fail                                  \
     */                                                             \
    TAPI_ON_JMP(TEST_ON_JMP_DO_SPECIFIC);                           \
    /* Initialize pseudo-random generator */                        \
    {                                                               \
        int te_rand_seed;                                           \
                                                                    \
        TEST_GET_INT_PARAM(te_rand_seed);                           \
        srand(te_rand_seed);                                        \
        RING("Pseudo-random seed is %d", te_rand_seed);             \
    }                                                               \
                                                                    \
    /*                                                              \
     * Should happen before TS-specific start so that it            \
     * impacts the startup procedure                                \
     */                                                             \
    test_behaviour_get(&test_behaviour_storage);                    \
    if (TEST_BEHAVIOUR(log_stack))                                  \
        te_log_stack_init();                                        \
                                                                    \
    TEST_START_SPECIFIC;                                            \
                                                                    \
    /* Resetup jump point to cleanup label */                       \
    TAPI_JMP_POP;                                                   \
    TAPI_ON_JMP(TEST_ON_JMP_DO);                                    \
    TEST_STEP_RESET()

/**
 * The last action of the test @b main() function.
 *
 * @note To define test-specific action define TEST_END_SPECIFIC macro
 */
#define TEST_END \
cleanup_specific:                                                   \
    if (te_sigusr2_caught())                                        \
        RING_VERDICT("Test caught the SIGUSR2 signal");             \
    TEST_END_SPECIFIC;                                              \
    te_log_bufs_cleanup();                                          \
    if (result == EXIT_SUCCESS &&                                   \
        tapi_test_run_status_get() != TE_TEST_RUN_STATUS_OK)        \
    {                                                               \
        ERROR("Exiting with failure because of critical error "     \
              "during test execution");                             \
        result = EXIT_FAILURE;                                      \
    }                                                               \
    return result

/**
 * @defgroup te_ts_tapi_test_misc Test misc
 * Miscellaneous definitions useful for test scenarios.
 *
 * @ingroup te_ts_tapi_test
 * @{
 */

/**
 * Check an expression passed as the argument against zero.
 * If the expression is something not zero the macro reports an
 * error, sets 'result' variable to EXIT_FAILURE and goes to 'cleanup'
 * label.
 *
 * @param expr_  Expression to be checked
 */
#define CHECK_RC(expr_) \
    do {                                                               \
        int rc_;                                                       \
                                                                       \
        te_log_stack_reset();                                          \
        if ((rc_ = (expr_)) != 0)                                      \
        {                                                              \
            TEST_FAIL("line %d: %s returns 0x%X (%r), but expected 0", \
                      __LINE__, # expr_, rc_, rc_);                    \
        }                                                              \
        te_log_stack_reset();                                          \
    } while (0)

/**
 * Check an expression passed as the argument against zero.
 * The same as CHECK_RC, but does not go to 'cleanup' label.
 *
 * @param expr_  Expression to be checked
 */
#define CLEANUP_CHECK_RC(expr_) \
    do {                                                            \
        int rc_;                                                    \
                                                                    \
        if ((rc_ = (expr_)) != 0)                                   \
        {                                                           \
            ERROR("line %d: %s returns 0x%X (%r), but expected 0",  \
                  __LINE__, # expr_, rc_, rc_);                     \
            result = EXIT_FAILURE;                                  \
        }                                                           \
    } while (0)

/**
 * Check that the expression is not NULL
 */
#define CHECK_NOT_NULL(expr_) \
    do {                                                                \
        if ((expr_) == NULL)                                            \
        {                                                               \
            TEST_FAIL("Expression %s in file %s line %d is "            \
                      "expected to be not NULL, but it is",             \
                      (#expr_), __FILE__, __LINE__);                    \
        }                                                               \
    } while (0)

/*
 * Check that @p _expr evaluates to an expected @p _exp_length
 * (e.g. check than an I/O function returned an expected number of bytes)
 *
 * @param expr_        Expression to check
 * @param exp_length_  Expected length
 */
#define CHECK_LENGTH(expr_, exp_length_) \
    do {                                                               \
        ssize_t length_;                                               \
                                                                       \
        te_log_stack_reset();                                          \
        if ((length_ = (expr_)) != (ssize_t)(exp_length_))             \
        {                                                              \
            TEST_FAIL("line %d: %s returns %zd, but expected %zd",     \
                      __LINE__, # expr_, length_, exp_length_);        \
        }                                                              \
        te_log_stack_reset();                                          \
    } while (0)

/**
 * Same as CHECK_RC, but verdict in case RC expression result is not ZERO.
 * The macro will terminate(fail) the test after logging the verdict.
 *
 * @param expr_    Expression to be checked
 * @param verdict_ Verdict to be logged (supports format string)
 */
#define CHECK_RC_VERDICT(expr_, verdict_...)                            \
    do {                                                                \
        int rc_;                                                        \
                                                                        \
        te_log_stack_reset();                                           \
        if ((rc_ = (expr_)) != 0)                                       \
        {                                                               \
            ERROR_VERDICT(verdict_);                                    \
            TEST_FAIL("line %d: %s returns 0x%X (%r), but expected 0",  \
                      __LINE__, # expr_, rc_, rc_);                     \
        }                                                               \
        te_log_stack_reset();                                           \
    } while (0)

/**
 * Same as CHECK_RC_VERDICT, but log an artifact instead of verdict
 * The macro will terminate(fail) the test after logging the artifact.
 *
 * @param expr_     Expression to be checked
 * @param artifact_ Artifact to be logged (supports format string)
 */
#define CHECK_RC_ARTIFACT(expr_, artifact_...)                         \
    do {                                                               \
        int rc_;                                                       \
                                                                       \
        te_log_stack_reset();                                          \
        if ((rc_ = (expr_)) != 0)                                      \
        {                                                              \
            TEST_ARTIFACT(artifact_);                                  \
            TEST_FAIL("line %d: %s returns 0x%X (%r), but expected 0", \
                      __LINE__, # expr_, rc_, rc_);                    \
        }                                                              \
        te_log_stack_reset();                                          \
    } while (0)

/** Free variable and set its value to NULL */
#define FREE_AND_CLEAN(ptr_) \
    do {                     \
        free(ptr_);          \
        ptr_ = NULL;         \
    } while (0)

/**
 * Check that two buffers of specified length have the same content and
 * reports an error in case they are not
 *
 * @param buf1_     First buffer
 * @param buf2_     Second buffer
 * @param buf_len_  Buffer length
 */
#define CHECK_BUFS_EQUAL(buf1_, buf2_, buf_len_) \
    do {                                                    \
        if (memcmp(buf1_, buf2_, buf_len_) != 0)            \
        {                                                   \
            TEST_FAIL("The content of '"#buf1_ "' and '"    \
                      #buf2_ "' are different");            \
        }                                                   \
    } while (0)

/**@} <!-- END te_ts_tapi_test_misc --> */

/**
 * @defgroup te_ts_tapi_test_param Test parameters
 *
 * If you need to pass a parameter to a test scenarion you need
 * to specify that parameter in @ref te_engine_tester configuration file
 * (package description file). Each parameter is associated with symbolic
 * name that should be used as the key while getting parameter value in
 * test scenario context.
 *
 * The main function to process test parameters in test scenario context
 * is test_get_param(). It gets parameter name as an argument value and
 * returns string value associated with that parameter.
 *
 * Apart from base function test_get_param() there are a number of macros
 * that process type-specific parameters:
 * - TEST_GET_ENUM_PARAM();
 * - TEST_GET_STRING_PARAM();
 * - TEST_GET_INT_PARAM();
 * - TEST_GET_INT64_PARAM();
 * - TEST_GET_DOUBLE_PARAM();
 * - TEST_GET_OCTET_STRING_PARAM();
 * - TEST_GET_STRING_LIST_PARAM();
 * - TEST_GET_INT_LIST_PARAM();
 * - TEST_GET_BOOL_PARAM();
 * - TEST_GET_FILENAME_PARAM();
 * - TEST_GET_BUFF_SIZE().
 * .
 *
 * For example for the following test run (from @path{package.xml}):
 * @code
 * <run>
 *   <script name="comm_sender"/>
 *     <arg name="size">
 *       <value>1</value>
 *       <value>100</value>
 *     </arg>
 *     <arg name="oob">
 *       <value>TRUE</value>
 *       <value>FALSE</value>
 *     </arg>
 *     <arg name="msg">
 *       <value>Test message</value>
 *     </arg>
 * </run>
 * @endcode
 * we can have the following test scenario:
 * @code
 * int main(int argc, char **argv)
 * {
 *     int      size;
 *     bool     oob;
 *     char    *msg;
 *
 *     TEST_START;
 *
 *     TEST_GET_INT_PARAM(size);
 *     TEST_GET_BOOL_PARAM(oob);
 *     TEST_GET_STRING_PARAM(msg);
 *     ...
 * }
 * @endcode
 *
 * Please note that variable name passed to TEST_GET_xxx_PARAM() macro shall
 * be the same as expected parameter name.
 *
 * Test suite can also define parameters of enumeration type. For this kind
 * of parameters you will need to define a macro based on
 * TEST_GET_ENUM_PARAM().
 *
 * For example if you want to specify something like the following in your
 * @path{package.xml} files:
 * @code
 * <enum name="ledtype">
 *   <value>POWER</value>
 *   <value>USB</value>
 *   <value>ETHERNET</value>
 *   <value>WIFI</value>
 * </enum>
 *
 * <run>
 *   <script name="led_test"/>
 *     <arg name="led" type="ledtype"/>
 * </run>
 * @endcode
 * You can define something like this in your test suite header file
 * (@path{test_suite.h}):
 * @code
 * enum ts_led {
 *     TS_LED_POWER,
 *     TS_LED_USB,
 *     TS_LED_ETH,
 *     TS_LED_WIFI,
 * };
 * #define LEDTYPE_MAPPING_LIST \
 *            { "POWER", (int)TS_LED_POWER },  \
 *            { "USB", (int)TS_LED_USB },      \
 *            { "ETHERNET", (int)TS_LED_ETH }, \
 *            { "WIFI", (int)TS_LED_WIFI }
 *
 * #define TEST_GET_LED_PARAM(var_name_) \
 *             TEST_GET_ENUM_PARAM(var_name_, LEDTYPE_MAPPING_LIST)
 * @endcode
 *
 * Then in your test scenario you can write the following:
 * @code
 * int main(int argc, char **argv)
 * {
 *     enum ts_led led;
 *
 *     TEST_START;
 *
 *     TEST_GET_LED_PARAM(led);
 *     ...
 *     switch (led)
 *     {
 *         case TS_LED_POWER:
 *     ...
 *     }
 *     ...
 * }
 * @endcode
 *
 * @ingroup te_ts_tapi_test
 * @{
 */


/**
 * Check if the parameter identified by the given name is available
 *
 * @param var_name_  Parameter name for check
 *
 * @return result of check
 */
#define TEST_HAS_PARAM(var_name_) \
    (test_find_param(argc, argv, #var_name_) != NULL)

/**
 * Generic way to return mapped value of a parameter: string -> enum
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @param maps_      An array of mappings: string -> enum
 * @return mapped value
 */
#define TEST_ENUM_PARAM(var_name_, maps_...) \
    test_get_enum_param(argc, argv, #var_name_,         \
    (struct param_map_entry []) { maps_, {NULL, 0} })


/**
 * Generic way to get mapped value of a parameter: string -> enum
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @param maps_      An array of mappings: string -> enum
 */
#define TEST_GET_ENUM_PARAM(var_name_, maps_...) \
    (var_name_) = TEST_ENUM_PARAM(var_name_, maps_)


/**
 * The macro to return parameters of type 'char *', i.e. not parsed
 * string value of the parameter
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @return string value
 */
#define TEST_STRING_PARAM(var_name_) \
    test_get_string_param(argc, argv, #var_name_)


/**
 * The macro to get parameters of type 'char *', i.e. not parsed
 * string value of the parameter
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_STRING_PARAM(var_name_) \
    (var_name_) = TEST_STRING_PARAM(var_name_)


/**
 * The macro to return parameters of type 'int'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @return int value
 */
#define TEST_INT_PARAM(var_name_) \
    test_get_int_param(argc, argv, #var_name_)


/**
 * The macro to get parameters of type 'int'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_INT_PARAM(var_name_) \
    (var_name_) = TEST_INT_PARAM(var_name_)


/**
 * Obtain the value of an 'unsigned int' parameter
 *
 * @param var_name_     The name to denote both the
 *                      target 'unsigned int' variable
 *                      and the parameter of interest
 * @return unsigned int parameter
 */
#define TEST_UINT_PARAM(var_name_) \
    test_get_uint_param(argc, argv, #var_name_)

/**
 * Obtain the value of an 'unsigned int' parameter
 *
 * @param _parameter    The name to denote both the
 *                      target 'unsigned int' variable
 *                      and the parameter of interest
 */
#define TEST_GET_UINT_PARAM(_parameter) \
    (_parameter) = TEST_UINT_PARAM(_parameter)


/**
 * The macro to return parameters of type 'int64'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @return int64 value
 */
#define TEST_INT64_PARAM(var_name_) \
    test_get_int64_param(argc, argv, #var_name_)


/**
 * The macro to get parameters of type 'int64'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_INT64_PARAM(var_name_) \
    (var_name_) = TEST_INT64_PARAM(var_name_)

/**
 * The macro to return parameters of type 'uint64'
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return uint64 value
 */
#define TEST_UINT64_PARAM(var_name_) \
    test_get_uint64_param(argc, argv, #var_name_)

/**
 * The macro to get parameters of type 'uint64'
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_UINT64_PARAM(var_name_) \
    (var_name_) = TEST_UINT64_PARAM(var_name_)

/**
 * The macro to return parameters of type 'double' ('float' also may
 * be used)
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @return double value
 */
#define TEST_DOUBLE_PARAM(var_name_) \
    test_get_double_param(argc, argv, #var_name_)


/**
 * The macro to get parameters of type 'double' ('float' also may be used)
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_DOUBLE_PARAM(var_name_) \
    (var_name_) = TEST_DOUBLE_PARAM(var_name_)

/**
 * The macro to return default value of 'uint64' parameter
 * from local subtree.
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value.
 * @return uint64 value.
 */
#define TEST_DEFAULT_UINT64_PARAM(var_name_) \
    test_get_default_uint64_param(TE_TEST_NAME, #var_name_)

/**
 * The macro to return default value of 'double' parameter
 * from local subtree.
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value.
 * @return double value.
 */
#define TEST_DEFAULT_DOUBLE_PARAM(var_name_) \
    test_get_default_double_param(TE_TEST_NAME, #var_name_)

/**
 * The macro to get default value of 'uint64' parameter
 * from local subtree.
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value.
 */
#define TEST_GET_DEFAULT_UINT64_PARAM(var_name_) \
    ((var_name_) = TEST_DEFAULT_UINT64_PARAM(var_name_))

/**
 * The macro to get default value of 'double' parameter
 * from local subtree.
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value.
 */
#define TEST_GET_DEFAULT_DOUBLE_PARAM(var_name_) \
    ((var_name_) = TEST_DEFAULT_DOUBLE_PARAM(var_name_))

/**
 * The macro to get parameters of type 'value-unit' (double with unit prefix)
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @return double value
 */
#define TEST_VALUE_UNIT_PARAM(var_name_) \
    test_get_value_unit_param(argc, argv, #var_name_)

/**
 * The macro to get parameters of type 'value-unit' (double with unit prefix)
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_VALUE_UNIT_PARAM(var_name_) \
    (var_name_) = TEST_VALUE_UNIT_PARAM(var_name_)

/**
 * The macro to get binary-scaled parameters (unsigned with unit prefix).
 *
 * Unlike TEST_VALUE_UNIT_PARAM(), the macro uses binary prefixes
 * (i.e. 1024, not 1000).
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return unsigned value
 */
#define TEST_VALUE_BIN_UNIT_PARAM(var_name_) \
    test_get_value_bin_unit_param(argc, argv, #var_name_)

/**
 * The macro to get binary-scaled parameters (unsigned with unit prefix).
 *
 * Unlike TEST_GET_VALUE_UNIT_PARAM(), the macro uses binary prefixes
 * (i.e. 1024, not 1000).
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_VALUE_BIN_UNIT_PARAM(var_name_) \
    (var_name_) = TEST_VALUE_BIN_UNIT_PARAM(var_name_)

/**
 * The macro to get parameter of type 'octet string'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @param var_len_   TODO
 */
#define TEST_GET_OCTET_STRING_PARAM(var_name_, var_len_) \
    do {                                                                \
        const char *str_val_;                                           \
        unsigned char *oct_string_;                                     \
                                                                        \
        if (((str_val_) = test_get_param(argc, argv,                    \
                                         #var_name_)) == NULL)          \
        {                                                               \
            TEST_STOP;                                                  \
        }                                                               \
        oct_string_ = test_get_octet_string_param(str_val_, var_len_);  \
        if (oct_string_ == NULL)                                        \
        {                                                               \
            TEST_FAIL("Test cannot get octet string from %s parameter", \
                      str_val_);                                        \
        }                                                               \
        var_name_ = (unsigned char *)oct_string_;                       \
    } while (0)

/** Default separator for argument list */
#define TEST_LIST_PARAM_SEPARATOR   ','

/**
 * The macro to get parameter of type 'char *[]', i.e. array of
 * string values
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_STRING_LIST_PARAM(var_name_, var_len_) \
    do {                                                            \
        const char *str_val_;                                       \
                                                                    \
        if ((str_val_ = test_get_param(argc, argv,                  \
                                       #var_name_)) == NULL)        \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
        (var_len_) = test_split_param_list(str_val_,                \
                TEST_LIST_PARAM_SEPARATOR, &(var_name_));           \
        if ((var_len_) == 0)                                        \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)


/**
 * The macro to get parameter of type 'int []', i.e. array of numbers
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_INT_LIST_PARAM(var_name_, var_len_) \
    do {                                                                \
        const char  *str_val_;                                          \
        char       **str_array;                                         \
        int          i;                                                 \
        char        *end_ptr;                                           \
                                                                        \
        if ((str_val_ = test_get_param(argc, argv,                      \
                                       #var_name_)) == NULL)            \
        {                                                               \
            TEST_STOP;                                                  \
        }                                                               \
        (var_len_) = test_split_param_list(str_val_,                    \
                TEST_LIST_PARAM_SEPARATOR, &str_array);                 \
        if ((var_len_) == 0 ||                                          \
            ((var_name_) = calloc((var_len_), sizeof(int))) == NULL)    \
        {                                                               \
            TEST_STOP;                                                  \
        }                                                               \
        for (i = 0; i < (var_len_); i++)                                \
        {                                                               \
            (var_name_)[i] = (int)strtol(str_array[i], &end_ptr, 0);    \
            if (end_ptr == str_array[i] || *end_ptr != '\0')            \
            {                                                           \
                TEST_FAIL("The value of '%s' parameter should be "      \
                          "a list of interger, but %d-th entry "        \
                          "is '%s'", #var_name_, i, str_array[i]);      \
            }                                                           \
        }                                                               \
        free(str_array[0]); free(str_array);                            \
    } while (0)

/**
 * The macro to get parameter of type 'octet string[]', i.e. array of
 * octet strings
 *
 * @param var_name_     Variable whose name is the same as the name of
 *                      parameter we get the value
 * @param var_str_len_  Octet string lentgh
 * @param var_list_len_ Variable for length of the list
 */
#define TEST_GET_OCTET_STRING_LIST_PARAM(var_name_, var_list_len_, var_str_len_) \
    do {                                                                \
        const char  *str_val_;                                          \
                                                                        \
        str_val_ = test_get_param(argc, argv, #var_name_);              \
                                                                        \
        test_octet_strings2list(str_val_, (var_str_len_),               \
                                &(var_name_), &(var_list_len_));        \
    } while (0)

/**
 * The macro to get parameter of bit mask type.
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @param maps_      An array of mappings: string -> enum of each entry
 *                   of a bitmask
 */
#define TEST_GET_BIT_MASK_PARAM(var_name_, maps_...) \
    do {                                                                \
        const char  *str_val_;                                          \
        const char  *tmp_;                                              \
        char       **str_array;                                         \
        int          i;                                                 \
        int          bit_map_len;                                       \
        int          mapped_val_;                                       \
        struct param_map_entry maps[] = {                               \
            maps_,                                                      \
            { NULL, 0 },                                                \
        };                                                              \
                                                                        \
        memset(&(var_name_), 0, sizeof(var_name_));                     \
        if ((str_val_ = test_get_param(argc, argv,                      \
                                       #var_name_)) == NULL)            \
        {                                                               \
            TEST_STOP;                                                  \
        }                                                               \
        tmp_ = str_val_;                                                \
        while (isspace(*tmp_))                                          \
            tmp_++;                                                     \
        if (*tmp_ == '\0')                                              \
            break;                                                      \
                                                                        \
        bit_map_len = test_split_param_list(str_val_, '|', &str_array); \
        if (bit_map_len == 0)                                           \
        {                                                               \
            TEST_STOP;                                                  \
        }                                                               \
        for (i = 0; i < bit_map_len; i++)                               \
        {                                                               \
            if (test_map_param_value(#var_name_, maps,                  \
                                     str_array[i], &mapped_val_) != 0)  \
            {                                                           \
                memset(&(var_name_), 0, sizeof(var_name_));             \
                TEST_STOP;                                              \
            }                                                           \
            var_name_ |= mapped_val_;                                   \
        }                                                               \
        free(str_array[0]); free(str_array);                            \
    } while (0)

/**
 * The list of values allowed for parameter of type @c bool
 */
#define BOOL_MAPPING_LIST \
            { "TRUE", true }, \
            { "FALSE", false }

/**
 * Get the value of parameter of type @c bool
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type @c bool (OUT)
 */
#define TEST_GET_BOOL_PARAM(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, BOOL_MAPPING_LIST)


/**
 * The list of values allowed for parameter of type 'te_bool3'
 */
#define BOOL3_MAPPING_LIST \
    { "TRUE",    TE_BOOL3_TRUE },   \
    { "FALSE",   TE_BOOL3_FALSE },  \
    { "UNKNOWN", TE_BOOL3_UNKNOWN }

/**
 * Get the value of parameter of type 'te_bool3'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'te_bool3' (OUT)
 */
#define TEST_GET_BOOL3_PARAM(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, BOOL3_MAPPING_LIST)


/**
 * The macro to return parameters of type `char *` with a full filename
 * which is the result of test location and file name concatenation.
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 *
 * @return full filename allocated from heap and should be freed by caller
 * @retval NULL If relative path is empty
 */
#define TEST_FILENAME_PARAM(var_name_) \
    test_get_filename_param(argc, argv, #var_name_)


/**
 * The macro to get parameters of type `char *` with a full filename and
 * assign it to the variable with corresponding name.
 *
 * Full filename is a concatation of the running test location and
 * relative path specified in an argument with matching name.
 *
 * @c NULL is assigned if relative path is empty.
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_FILENAME_PARAM(var_name_) \
    (var_name_) = TEST_FILENAME_PARAM(var_name_)


/**
 * Get the value of parameter that represents IPv4 address.
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'in_addr_t' (OUT)
 */
#define TEST_GET_IP4_PARAM(var_name_) \
    do {                                                                \
        const char *str_val_;                                           \
        int         rc_;                                                \
                                                                        \
        if ((str_val_ = test_get_param(argc, argv,                      \
                                       #var_name_)) == NULL)            \
        {                                                               \
            TEST_STOP;                                                  \
        }                                                               \
        rc_ = inet_pton(AF_INET, str_val_, &(var_name_));               \
        if (rc_ != 1)                                                   \
            TEST_FAIL("The value of '" #var_name_ "' parameter is "     \
                      "not seem to be a valid IPv4 address: %s",        \
                      str_val_);                                        \
    } while (0)

/** Abstaract enum to 'buf_size' parameter */
typedef enum sapi_buf_size {
    SIZE_ZERO,
    SIZE_SHORT,
    SIZE_EXACT,
    SIZE_LONG,
} sapi_buf_size;

/** The list of values allowed for parameter of type 'sapi_buf_size' */
#define BUF_SIZE_MAPPING_LIST \
            { "0", (int)SIZE_ZERO },      \
            { "short", (int)SIZE_SHORT }, \
            { "exact", (int)SIZE_EXACT }, \
            { "long", (int)SIZE_LONG }

/**
 * Get the value of parameter of type 'sapi_buf_size'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'sapi_buf_size' (OUT)
 */
#define TEST_GET_BUFF_SIZE(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, BUF_SIZE_MAPPING_LIST)

/** Ethernet device states */
typedef enum {
    TEST_ETHDEV_UNINITIALIZED,

    TEST_ETHDEV_INITIALIZED,
    TEST_ETHDEV_CONFIGURED,
    TEST_ETHDEV_RX_SETUP_DONE,
    TEST_ETHDEV_TX_SETUP_DONE,
    TEST_ETHDEV_RXTX_SETUP_DONE,
    TEST_ETHDEV_STARTED,
    TEST_ETHDEV_STOPPED,
    TEST_ETHDEV_CLOSED,
    TEST_ETHDEV_DETACHED,
} test_ethdev_state;

/** The list of values allowed for parameter of type 'test_ethdev_state' */
#define ETHDEV_STATE_MAPPING_LIST \
            { "INITIALIZED", (int)TEST_ETHDEV_INITIALIZED },         \
            { "CONFIGURED", (int)TEST_ETHDEV_CONFIGURED },           \
            { "RX_SETUP_DONE", (int)TEST_ETHDEV_RX_SETUP_DONE },     \
            { "TX_SETUP_DONE", (int)TEST_ETHDEV_TX_SETUP_DONE },     \
            { "RXTX_SETUP_DONE", (int)TEST_ETHDEV_RXTX_SETUP_DONE }, \
            { "STARTED", (int)TEST_ETHDEV_STARTED },                 \
            { "STOPPED", (int)TEST_ETHDEV_STOPPED },                 \
            { "CLOSED", (int)TEST_ETHDEV_CLOSED },                   \
            { "DETACHED", (int)TEST_ETHDEV_DETACHED }

/**
 * Get the value of parameter of type 'test_ethdev_state'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'test_ethdev_state' (OUT)
 */
#define TEST_GET_ETHDEV_STATE(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, ETHDEV_STATE_MAPPING_LIST)

/** The list of values allowed for parameter of type 'tarpc_rte_filter_type' */
#define FILTER_TYPE_MAPPING_LIST \
            { "NONE", (int)TARPC_RTE_ETH_FILTER_NONE },            \
            { "MACVLAN", (int)TARPC_RTE_ETH_FILTER_MACVLAN },      \
            { "ETHERTYPE", (int)TARPC_RTE_ETH_FILTER_ETHERTYPE },  \
            { "FLEXIBLE", (int)TARPC_RTE_ETH_FILTER_FLEXIBLE },    \
            { "SYN", (int)TARPC_RTE_ETH_FILTER_SYN },              \
            { "NTUPLE", (int)TARPC_RTE_ETH_FILTER_NTUPLE },        \
            { "TUNNEL", (int)TARPC_RTE_ETH_FILTER_TUNNEL },        \
            { "FDIR", (int)TARPC_RTE_ETH_FILTER_FDIR },            \
            { "HASH", (int)TARPC_RTE_ETH_FILTER_HASH },            \
            { "L2_TUNNEL", (int)TARPC_RTE_ETH_FILTER_L2_TUNNEL },  \
            { "MAX", (int)TARPC_RTE_ETH_FILTER_MAX }

/**
 * Get the value of parameter of type 'tarpc_rte_filter_type'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'tarpc_rte_filter_type' (OUT)
 */
#define TEST_GET_FILTER_TYPE(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, FILTER_TYPE_MAPPING_LIST)

/** List of values allowed for parameter of 'tarpc_rte_eth_tunnel_type' type */
#define TUNNEL_TYPE_MAPPING_LIST \
            { "NONE",      (int)TARPC_RTE_TUNNEL_TYPE_NONE },      \
            { "VXLAN",     (int)TARPC_RTE_TUNNEL_TYPE_VXLAN },     \
            { "GENEVE",    (int)TARPC_RTE_TUNNEL_TYPE_GENEVE },    \
            { "TEREDO",    (int)TARPC_RTE_TUNNEL_TYPE_TEREDO },    \
            { "NVGRE",     (int)TARPC_RTE_TUNNEL_TYPE_NVGRE },     \
            { "IP_IN_GRE", (int)TARPC_RTE_TUNNEL_TYPE_IP_IN_GRE }, \
            { "L2_E_TAG",  (int)TARPC_RTE_L2_TUNNEL_TYPE_E_TAG },  \
            { "MAX",       (int)TARPC_RTE_TUNNEL_TYPE_MAX }

/**
 * Get parameter value of 'tarpc_rte_eth_tunnel_type' type
 *
 * @param var_name_ Variable name used to get "var_name_" parameter value
 *                  of 'tarpc_rte_eth_tunnel_type' type (OUT)
 */
#define TEST_GET_TUNNEL_TYPE(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, TUNNEL_TYPE_MAPPING_LIST)


/**
 * The macro to get a parameter representing some expected result.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * See test_get_expected_result_param() for the parameter value syntax.
 *
 * @return tapi_test_expected_result value
 */
#define TEST_EXPECTED_RESULT_PARAM(var_name_) \
    test_get_expected_result_param(argc, argv, #var_name_)

/**
 * The macro to assign a parameter representing some expected result.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_EXPECTED_RESULT_PARAM(var_name_) \
    (var_name_) = TEST_EXPECTED_RESULT_PARAM(var_name_)

/**
 * The macro populates a vector from multi-valued parameter.
 *
 * This is expected to be used with structured values in
 * the package description, e.g. provided there is the following
 * argument definition:
 *
 * @code{.xml}
 * <arg name="multival">
 *   <value>
 *      <field>Value 1</field>
 *      <field>Value 2</field>
 *      <field>Value 3</field>
 *    </value>
 * </arg>
 * @endcode
 *
 * the following code in the test may be used to retrieve all
 * values of @c multival:
 *
 * @code{.c}
 * te_vec multiple = TE_VEC_INIT(const char *);
 * ...
 * TEST_GET_PARAMS_VECTOR(multival, test_get_string_param);
 * @endcode
 *
 * The type of vector elements should be assignment-compatible
 * with the return type of @p accessor_.
 *
 * @param var_name_    target #te_vec
 * @param acceesor_    accessor function
 * @param ...          additional arguments to accessor
 */
#define TEST_GET_PARAMS_VECTOR(var_name_, accessor_, ...) \
    do {                                                                \
        te_string tmp_name_ = TE_STRING_INIT;                           \
        size_t idx_ = 0;                                                \
                                                                        \
        for (;;)                                                        \
        {                                                               \
            te_string_reset(&tmp_name_);                                \
            te_compound_build_name(&tmp_name_, #var_name_, NULL, idx_); \
            if (test_find_param(argc, argv, tmp_name_.ptr) == NULL)     \
                break;                                                  \
            TE_VEC_APPEND_RVALUE(&var_name_,                            \
                                 TE_TYPEOF(accessor_(argc, argv,        \
                                                     tmp_name_.ptr,     \
                                                     ##__VA_ARGS__)),   \
                                 accessor_(argc, argv,                  \
                                           tmp_name_.ptr,               \
                                           ##__VA_ARGS__));             \
            idx_++;                                                     \
        }                                                               \
        te_string_free(&tmp_name_);                                     \
    } while (0)

/** ID assigned by the Tester to the test instance */
extern unsigned int te_test_id;

/**
 * Check whether SIGUSR2 signal has ever been caught
 *
 * @retval @c false No signal has been caught
 * @retval @c true  Signal(-s) has(-ve) been caught.
 */
extern bool te_sigusr2_caught(void);

/**
 * Finds a particular parameter in the list of parameters
 *
 * @param argc  number of parameters
 * @param argv  Array of parameters in form "<param name>=<param value>"
 * @param name  Parameter name whose value we want to obtain
 *
 * @return Pointer to the parameter
 * @retval NULL if parameter is not in the list of parameters
 */

extern const char *test_find_param(int argc, char *argv[], const char *name);

/**
 * Finds value of particular parameter in the list of parameters
 *
 * @param argc  number of parameters
 * @param argv  Array of parameters in form "<param name>=<param value>"
 * @param name  Parameter name whose value we want to obtain
 *
 * @return Pointer to the value of the parameter
 */
extern const char *test_get_param(int argc, char *argv[], const char *name);

/**
 * Maps all possible values of some variable from string to numberical
 * representation.
 *
 * @param var_name  Variable name whose value is being converted
 * @param maps      Structure with mappings "str" -> num
 * @param str_val   String value that should be mapped
 * @param num_val   Mapped value (OUT)
 *
 * @return Status of the operation
 *
 * @retval  0 Mapping is successfully done
 * @retval -1 Mapping fails (in this case the procedure generates ERROR
 *            message that describes the problem found during the procedure)
 */
extern int test_map_param_value(const char *var_name,
                                const struct param_map_entry *maps,
                                const char *str_val, int *num_val);

/**
 * Transform string value to octet string value.
 *
 * @param str_val   String value (each byte is in hex format and
 *                  delimeter is ':' character
 * @param len       Expected octet string value length
 *
 * @return Octet string value or NULL int case of failure.
 *
 * @note Octet string returned by this function should be deallocated
 * with free() call.
 */
extern uint8_t *test_get_octet_string_param(const char *str_val,
                                            size_t len);


/**
 * Generic way to return mapped value of a parameter: string -> enum
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 * @param maps       An array of mappings: string -> enum
 */
extern int test_get_enum_param(int argc, char **argv,
                               const char *name,
                               const struct param_map_entry *maps);

/**
 * Return parameters of type 'char *', i.e. not parsed
 * string value of the parameter
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 */
extern const char * test_get_string_param(int argc, char **argv,
                                          const char *name);

/**
 * Return parameters of type `char *` with full filename which is
 * the result of test location and file name concatenation.
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 *
 * @return String allocated from heap which should be freed by caller
 * @retval @c NULL  If relative path is empty
 *
 * @sa #TEST_FILENAME_PARAM, #TEST_GET_FILENAME_PARAM
 */
extern char *test_get_filename_param(int argc, char **argv, const char *name);

/**
 * Return parameters of type 'int'
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 */
extern int test_get_int_param(int argc, char **argv,
                              const char *name);

/**
 * Dedicated function to get te_test_id before
 * any jump points installed (since logging of
 * jump point setup requires te_test_id to associate
 * the log message with the test).
 *
 * @param argc        Count of arguments
 * @param argv        List of arguments
 *
 * @return TE test ID
 */
extern unsigned int test_get_test_id(int argc, char **argv);

/**
 * Dedicated function to check that te_test_name starts with tester_monitor
 * that means that this test is a test based command monitor.
 *
 * @param argc        Count of arguments.
 * @param argv        List of arguments.
 *
 * @retval @c false This test is a normal test.
 * @retval @c true  This test is a test based command monitor.
 */
extern bool test_is_cmd_monitor(int argc, char **argv);

/**
 * Return parameters of type 'uint'
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 */
extern unsigned int test_get_uint_param(int argc, char **argv,
                                        const char *name);

/**
 * Return parameters of type 'long long int'
 *
 * @param argc       Count of arguments
 * @param argc       List of arguments
 * @param name       Name of parameter
 */
extern int64_t test_get_int64_param(int argc, char **argv,
                                    const char *name);

/**
 * Return parameters of type 'unsigned long long int'
 *
 * @param argc       count of arguments
 * @param argv       list of arguments
 * @param name       parameter name
 *
 * @return the value of a parameter @p name
 */
extern uint64_t test_get_uint64_param(int argc, char **argv,
                                      const char *name);

/**
 * Return parameters of type 'double'
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 */
extern double test_get_double_param(int argc, char **argv,
                                    const char *name);

/**
 * Get default value of string parameter from local subtree.
 *
 * @param test_name   Name of test the parameter belongs.
 * @param param_name  Parameter name.
 *
 * @return Default value of @c param_name.
 *
 * @note Function allocates memory with malloc(), which should be freed
 * with free() by the caller.
 */
extern char *test_get_default_string_param(const char *test_name,
                                           const char *param_name);

/**
 * Get default value of uint64 parameter from local subtree.
 *
 * @param test_name   Name of test the parameter belongs.
 * @param param_name  Parameter name.
 *
 * @return Default value of @c param_name.
 */
extern uint64_t test_get_default_uint64_param(const char *test_name,
                                              const char *param_name);

/**
 * Get default value of double parameter from local subtree.
 *
 * @param test_name   Name of test the parameter belongs.
 * @param param_name  Parameter name.
 *
 * @return Default value of @c param_name.
 */
extern double test_get_default_double_param(const char *test_name,
                                            const char *param_name);

/**
 * Return parameters of type 'value-unit' ('double')
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 */
extern double test_get_value_unit_param(int argc, char **argv,
                                        const char *name);

/**
 * Return parameters of type 'value-unit' (unsigned).
 *
 * Unlike test_get_value_unit_param(), the function uses binary base
 * prefixes (i.e. @c 1024, not @c 1000).
 *
 * @bug
 * The function uses doubles internally for unit conversion,
 * and a double cannot hold all range of 64-bit integral values,
 * so very large values (greater than approx. @c 4.5e15 or @c 4P)
 * may be represented imprecisely.
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 */
extern uintmax_t test_get_value_bin_unit_param(int argc, char **argv,
                                               const char *name);


/**
 * The description of an expected result.
 *
 * The structure should be considered opaque and only
 * handled by dedicated functions such as test_get_expected_result_param(),
 * TEST_EXPECTED_RESULT_PARAM(), TEST_GET_EXPECTED_RESULT_PARAM(),
 * tapi_test_check_expected_result(), tapi_test_check_expected_int_result().
 *
 * The values of this type may be freely copied, they do not contain any
 * dynamically-allocated resources.
 */
typedef struct tapi_test_expected_result {
    /**
     * The expected module code.
     *
     * TE_MIN_MODULE means any module.
     */
    te_module error_module;
    /** The expected error code (without a module). */
    te_errno error_code;
    /** The expected output string. */
    const char *output;
} tapi_test_expected_result;

/**
 * Check whether the actual result matches the expected one.
 *
 * The function will log details in case of a result mismatch, but
 * it won't automatically fail the test.
 *
 * @param expected  the description of the expected result
 * @param rc        actual status code
 * @param output    actual output
 *
 * @return @c true if the actual result matches the expected one.
 */
extern bool
tapi_test_check_expected_result(const tapi_test_expected_result *expected,
                                te_errno rc, const char *output);

/**
 * Check whether the actual integral result matches the expected one.
 *
 * The expected output in @p expected must be a string representing an
 * integral value; otherwise the function will fail the test.
 *
 * If @p expected contains @c NULL expected output, @p ival is not
 * checked at all.
 *
 * @param expected  the description of the expected result
 * @param rc        actual status code
 * @param ival      actual integral result
 *
 * @return @c true if the actual result matches the expected one.
 */
extern bool
tapi_test_check_expected_int_result(const tapi_test_expected_result *expected,
                                    te_errno rc, intmax_t ival);

/**
 * Get a parameter representing some expected result.
 *
 * The value has one of the following two forms:
 *
 * @code
 * [[MODULE_CODE "-" ] ERROR_CODE [ ":" EXPECTED_OUTPUT ]
 * [ "OK:" ] EXPECTED_OUTPUT
 * @endcode
 *
 * The module code and the error code correspond to names of
 * members of te_module and te_errno_codes without the @c TE_ prefix.
 *
 * If @c EXPECTED_OUTPUT is not provided, it is taken to be @c NULL.
 *
 *
 * For example:
 * - `OK:This is the output`
 * - `This is the output`
 * - `This is the output: it contains colons`
 * - `OK:ENOENT:This is the output` --- `ENOENT` here is part of the output.
 * - `ENOENT` --- the output is expected to be @ NULL
 * - `TAPI-ENOENT`
 * - `TAPI-ENOENT:Expected error message`
 *
 * Also see @path{suites/selftest/ts/minimal/parameters.c} fore more examples.
 *
 * @param argc       number of arguments
 * @param argv       list of arguments
 * @param name       name of the parameter
 *
 * @return the expected result description
 */
extern tapi_test_expected_result
test_get_expected_result_param(int argc, char **argv, const char *name);

/** @name Optional parameters.
 *
 * The functions and macros in this section are analogous to
 * plain parameter functions, but they allow to specify "missing"
 * value by providing literal @c - instead of a value.
 */
/**@{*/

/**
 * Same as test_get_string_param(), but returns @c NULL if
 * the parameter value is blank.
 *
 * @param argc       count of arguments
 * @param argv       list of arguments
 * @param name       name of parameter
 *
 * @return parameter value or @c NULL
 */
extern const char * test_get_opt_string_param(int argc, char **argv,
                                              const char *name);

/**
 * Same as test_get_uint_param(), but the return type is optional.
 *
 * @param argc       count of arguments
 * @param argv       list of arguments
 * @param name       name of parameter
 *
 * @return an optional unsigned value
 *
 * @sa te_optional_uint_t
 */
extern te_optional_uint_t test_get_opt_uint_param(int argc, char **argv,
                                                  const char *name);

/**
 * Same as test_get_uint64_param(), but the return type is optional.
 *
 * @param argc       count of arguments
 * @param argv       list of arguments
 * @param name       name of parameter
 *
 * @return an optional long unsigned value
 *
 * @sa te_optional_uintmax_t
 */
extern te_optional_uintmax_t test_get_opt_uint64_param(int argc, char **argv,
                                                       const char *name);

/**
 * Same as test_get_double_param() but the return type is optional.
 *
 * @param argc       count of arguments
 * @param argv       list of arguments
 * @param name       name of parameter
 *
 * @return an optional double value
 *
 * @sa te_optional_double_t
 */
extern te_optional_double_t test_get_opt_double_param(int argc, char **argv,
                                                      const char *name);

/**
 * Same as test_get_value_unit_param() but the return type is optional.
 *
 * @param argc       count of arguments
 * @param argv       list of arguments
 * @param name       name of parameter
 *
 * @return an optional double value
 *
 * @sa te_optional_double_t
 */
extern te_optional_double_t test_get_opt_value_unit_param(int argc,
                                                          char **argv,
                                                          const char *name);

/**
 * Same as test_get_value_bin_unit_param() but the return type is optional.
 *
 * @param argc       count of arguments
 * @param argv       list of arguments
 * @param name       name of parameter
 *
 * @return an optional long unsigned value
 *
 * @sa te_optional_uintmax_t
 */
extern te_optional_uintmax_t test_get_opt_value_bin_unit_param(
    int argc, char **argv, const char *name);

/**
 * Same as TEST_STRING_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return string value (@c NULL if "missing")
 */
#define TEST_OPT_STRING_PARAM(var_name_) \
    test_get_opt_string_param(argc, argv, #var_name_)

/**
 * Same as TEST_GET_STRING_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_OPT_STRING_PARAM(var_name_) \
    (var_name_) = TEST_OPT_STRING_PARAM(var_name_)


/**
 * Same as TEST_UINT_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return optional unsigned value
 */
#define TEST_OPT_UINT_PARAM(var_name_) \
    test_get_opt_uint_param(argc, argv, #var_name_)

/**
 * Same as TEST_GET_UINT_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_OPT_UINT_PARAM(var_name_) \
    (var_name_) = TEST_OPT_UINT_PARAM(var_name_)

/**
 * Same as TEST_UINT64_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return optional long unsigned value
 */
#define TEST_OPT_UINT64_PARAM(var_name_) \
    test_get_opt_uint64_param(argc, argv, #var_name_)

/**
 * Same as TEST_GET_UINT64_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_OPT_UINT64_PARAM(var_name_) \
    (var_name_) = TEST_OPT_UINT64_PARAM(var_name_)

/**
 * Same as TEST_DOUBLE_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return optional double value
 */
#define TEST_OPT_DOUBLE_PARAM(var_name_) \
    test_get_opt_double_param(argc, argv, #var_name_)


/**
 * Same as TEST_GET_DOUBLE_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_OPT_DOUBLE_PARAM(var_name_) \
    (var_name_) = TEST_OPT_DOUBLE_PARAM(var_name_)

/**
 * Same as TEST_VALUE_UNIT_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return optional double value
 */
#define TEST_OPT_VALUE_UNIT_PARAM(var_name_) \
    test_get_opt_value_unit_param(argc, argv, #var_name_)

/**
 * Same as TEST_GET_VALUE_UNIT_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_OPT_VALUE_UNIT_PARAM(var_name_) \
    (var_name_) = TEST_OPT_VALUE_UNIT_PARAM(var_name_)

/**
 * Same as TEST_VALUE_BIN_UNIT_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 *
 * @return optional long unsigned value
 */
#define TEST_OPT_VALUE_BIN_UNIT_PARAM(var_name_) \
    test_get_opt_value_bin_unit_param(argc, argv, #var_name_)

/**
 * Same as TEST_GET_VALUE_BIN_UNIT_PARAM() but for optional parameters.
 *
 * @param var_name_  variable with the same name as the name of
 *                   the desired parameter
 */
#define TEST_GET_OPT_VALUE_BIN_UNIT_PARAM(var_name_) \
    (var_name_) = TEST_OPT_VALUE_BIN_UNIT_PARAM(var_name_)

/**@}*/

/**
 * Print octet string.
 *
 * @param oct_string    Octet string
 * @param len           Octet string lentgh
 *
 * @return Buffer with string representation of octet string
 */
extern const char *print_octet_string(const uint8_t *oct_string,
                                      size_t len);

/**
 * Split parameter string into array of string, using specified separator.
 * This function creates a duplicate parameter string to avoid changing
 * the real parameter.
 *
 * @param list      String to parse
 * @param sep       Separator to split string
 * @param array_p   Array to return
 *
 * @return Length of the list or 0 on error
 */
extern int test_split_param_list(const char *list, char sep,
                                 char ***array_p);

/**
 * Function parses value and converts it to ASN value.
 * It also handles cfg links.
 *
 * @param ns    Preallocated buffer for updated value
 */
extern te_errno
tapi_asn_param_value_parse(char              *pwd,
                           char             **s,
                           const asn_type    *type,
                           asn_value        **parsed_val,
                           int               *parsed_syms,
                           char              *ns);

/**
 * Parse test ASN parameters.
 *
 * @param argc          Amount of test parameters
 * @param argv          Test parameters array
 * @param conf_prefix   Test ASN parameters prefix
 * @param conf_type     Test ASN parameters aggregation type
 * @param conf_value    Resulted ASN parameters aggregation value
 *
 * @return 0 if success, or corresponding error otherwise
 */
extern te_errno
tapi_asn_params_get(int argc, char **argv, const char *conf_prefix,
                    const asn_type *conf_type, asn_value *conf_value);

/** Add test arguments to the list of test parameters */
extern te_errno tapi_test_args2kvpairs(int argc, char *argv[],
                                       te_kvpair_h *args);

/**
 * Convert the string with several octet strings separated by commas
 * to array of octet strings
 *
 * @param str           String with octet strings
 * @param str_len       Octet string lentgh
 * @param oct_str_list  Array with octet strings (OUT)
 * @param list_len      Length of the array (OUT)
 */
extern void
test_octet_strings2list(const char *str, unsigned int str_len,
                        uint8_t ***oct_str_list, unsigned int *list_len);

/**@} <!-- END te_ts_tapi_test_param --> */

/** @addtogroup te_ts_tapi_test_misc
 * @{
 */

/**
 * Signal handler to close TE when Ctrl-C is pressed.
 *
 * @param signum    - signal number
 */
extern void te_test_sig_handler(int signum);

/* Scalable sleep primitives */

/** Maximum allowed sleep scale */
#define TE_MAX_SCALE    1000

/**
 * Function to get sleep scale.
 *
 * @return Scale.
 */
static inline unsigned int
test_sleep_scale(void)
{
    const char         *var_name = "TE_SLEEP_SCALE";
    const unsigned int  def_val = 1;
    const char         *value;
    const char         *end;
    unsigned long       scale;

    value = getenv(var_name);
    if (value == NULL || *value == '\0')
        return def_val;

    scale = strtoul(value, (char **)&end, 10);
    if (*end != '\0' || scale >= TE_MAX_SCALE)
    {
        ERROR("Invalid value '%s' in Environment variable '%s'",
              value, var_name);
        return def_val;
    }

    return scale;
}

/**
 * Scalable sleep (sleep scale times for _to_sleep seconds). Logs function
 * name. Use VSLEEP() instead unless you're absolutely sure what you're
 * doing.
 *
 * @param _to_sleep     number of seconds to sleep is scale is 1
 */
#define SLEEP(_to_sleep)  \
    te_motivated_sleep(test_sleep_scale() * (_to_sleep), __FUNCTION__)

/**
 * Scalable sleep (sleep scale times for _to_sleep seconds) that logs extra
 * information.
 *
 * @param _to_sleep     number of seconds to sleep is scale is 1
 * @param _msg          message to be logged
 */
#define VSLEEP(_to_sleep, _msg)                                         \
    te_motivated_sleep(test_sleep_scale() * (_to_sleep), (_msg))

/**
 * Scalable sleep (sleep scale times for _to_sleep milliseconds).
 *
 * @param _to_sleep     number of seconds to sleep is scale is 1
 */
#define MSLEEP(_to_sleep)   te_msleep(test_sleep_scale() * (_to_sleep))

/**
 * Scalable sleep (sleep scale times for _to_sleep microseconds).
 *
 * @param _to_sleep     number of seconds to sleep is scale is 1
 */
#define USLEEP(_to_sleep)   te_usleep(test_sleep_scale() * (_to_sleep))

/** Time to wait for a network activity, milliseconds. */
#define TAPI_WAIT_NETWORK_DELAY 500

/** Wait for network action to complete. Typically, send() on one side
 * and wait before non-blocing recv() on another side. */
#define TAPI_WAIT_NETWORK \
    do {                                                            \
        int msec = TAPI_WAIT_NETWORK_DELAY;                         \
        RING("Wait for network action: sleep for %d msec", msec);   \
        usleep(test_sleep_scale() * 1000 * msec);                   \
    } while (0)

/**@} <!-- END te_ts_tapi_test_misc --> */

/**@} <!-- END te_ts_tapi_test --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TEST_H__ */
