/** @file
 * @brief Test API
 *
 * Macros to be used in tests. The header must be included from test
 * sources only. It is allowed to use the macros only from @b main()
 * function of the test.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_TEST_H__
#define __TE_TAPI_TEST_H__

#ifdef STDC_HEADERS
#include <stdlib.h>
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
#include "tapi_test_log.h"
#include "tapi_test_run_status.h"
#include "asn_impl.h"
#include "asn_usr.h"

#include "te_tools.h"
#include "te_param.h"
#include "te_kvpair.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_ts_tapi_test TAPI: Test
 * @ingroup te_ts
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

/**
 * Template action to be done on jump in the test.
 */
#define TEST_ON_JMP_DO \
    do {                                                             \
        if (result == EXIT_SUCCESS || result == EXIT_FAILURE) \
            result = (TE_RC_GET_ERROR(jmp_rc) == TE_EOK) ? EXIT_SUCCESS  \
                                                     : EXIT_FAILURE; \
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
    argc--;                                                         \
    argv++;                                                         \
                                                                    \
    /* Initialize te_lgr_entity variable */                         \
    te_lgr_entity = TE_TEST_NAME;                                   \
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
     * Set jump point to cleanup_specific label to handle           \
     * if TEST_START_SPECIFIC fail                                  \
     */                                                             \
    TAPI_ON_JMP(TEST_ON_JMP_DO_SPECIFIC);                           \
    TEST_GET_INT_PARAM(te_test_id);                                 \
                                                                    \
    /* Initialize pseudo-random generator */                        \
    {                                                               \
        int te_rand_seed;                                           \
                                                                    \
        TEST_GET_INT_PARAM(te_rand_seed);                           \
        srand(te_rand_seed);                                        \
        RING("Pseudo-random seed is %d", te_rand_seed);             \
    }                                                               \
                                                                    \
    TEST_START_SPECIFIC;                                            \
                                                                    \
    /* Resetup jump point to cleanup label */                       \
    TAPI_JMP_POP;                                                   \
    TAPI_ON_JMP(TEST_ON_JMP_DO)

/**
 * The last action of the test @b main() function.
 *
 * @note To define test-specific action define TEST_END_SPECIFIC macro
 */
#define TEST_END \
cleanup_specific:                                                   \
    TEST_END_SPECIFIC;                                              \
    if (result == EXIT_SUCCESS &&                                   \
        tapi_test_run_status_get() != TE_TEST_RUN_STATUS_OK)        \
    {                                                               \
        ERROR("Exiting with failure because of critical error "     \
              "during test execution");                             \
        result = EXIT_FAILURE;                                      \
    }                                                               \
    return result

/**
 * Terminate a test with skip status, reporting the reason.
 *
 * @param fmt       reason message format string with parameters
 */
#define TEST_SKIP(fmt...) \
    do {                                                \
        result = TE_EXIT_SKIP;                          \
        RING("Test Skipped in %s, line %d, %s()",       \
             __FILE__, __LINE__, __FUNCTION__);         \
        RING(fmt);                                      \
        TEST_STOP;                                      \
    } while (0)

/**
 * @defgroup te_ts_tapi_test_misc TAPI: Test misc
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
        if ((rc_ = (expr_)) != 0)                                      \
        {                                                              \
            TEST_FAIL("line %d: %s returns 0x%X (%r), but expected 0", \
                      __LINE__, # expr_, rc_, rc_);                    \
        }                                                              \
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
            TEST_FAIL("Expression " #expr_ " in file %s line %d is "    \
                      "expected to be not NULL, but it is",             \
                      __FILE__, __LINE__);                              \
        }                                                               \
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
 * @defgroup te_ts_tapi_test_param TAPI: Test parameters
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
 *     te_bool  oob;
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
 * Generic way to get mapped value of a parameter: string -> enum
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @param maps_      An array of mappings: string -> enum
 */
#define TEST_GET_ENUM_PARAM(var_name_, maps_...) \
    do {                                                    \
        const char *val_;                                   \
        int         mapped_val_;                            \
        struct param_map_entry maps[] = {                   \
            maps_,                                          \
            { NULL, 0 },                                    \
        };                                                  \
                                                            \
        val_ = test_get_param(argc, argv, #var_name_);      \
        if (val_ != NULL &&                                 \
            test_map_param_value(#var_name_, maps,          \
                                 val_, &mapped_val_) == 0)  \
        {                                                   \
            (var_name_) = mapped_val_;                      \
            break;                                          \
        }                                                   \
        memset(&(var_name_), 0, sizeof(var_name_));         \
        TEST_STOP;                                          \
    } while (0)


/**
 * The macro to get parameters of type 'char *', i.e. not parsed
 * string value of the parameter
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_STRING_PARAM(var_name_) \
    do {                                                         \
        if (((var_name_) = test_get_param(argc, argv,            \
                                          #var_name_)) == NULL)  \
        {                                                        \
            TEST_STOP;                                           \
        }                                                        \
    } while (0)


/**
 * The macro to get parameters of type 'int'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_INT_PARAM(var_name_) \
    do {                                                        \
        const char *str_val_;                                   \
        char       *end_ptr;                                    \
                                                                \
        str_val_ = test_get_param(argc, argv, #var_name_);      \
        if (str_val_ == NULL)                                   \
        {                                                       \
            TEST_STOP;                                          \
        }                                                       \
        (var_name_) = (int)strtol(str_val_, &end_ptr, 0);       \
        if (end_ptr == str_val_ || *end_ptr != '\0')            \
        {                                                       \
            TEST_FAIL("The value of '%s' parameter should be "  \
                      "an integer, but it is %s", #var_name_,   \
                      str_val_);                                \
        }                                                       \
    } while (0)


/**
 * Obtain the value of an 'unsigned int' parameter
 *
 * @param _parameter    The name to denote both the
 *                      target 'unsigned int' variable
 *                      and the parameter of interest
 */
#define TEST_GET_UINT_PARAM(_parameter) \
    do {                                                        \
        const char     *str_val;                                \
        unsigned long   ulint_val;                              \
        char           *end_ptr;                                \
                                                                \
        str_val = test_get_param(argc, argv, #_parameter);      \
        CHECK_NOT_NULL(str_val);                                \
                                                                \
        ulint_val = strtoul(str_val, &end_ptr, 0);              \
        if ((end_ptr == str_val) || (*end_ptr != '\0'))         \
            TEST_FAIL("Failed to convert '%s' to a number",     \
                      #_parameter);                             \
                                                                \
        if (ulint_val > UINT_MAX)                               \
            TEST_FAIL("'%s' parameter value is greater"         \
                      " than %u and cannot be stored in"        \
                      " an 'unsigned int' variable",            \
                      #_parameter, UINT_MAX);                   \
                                                                \
        _parameter = (unsigned int)ulint_val;                   \
    } while (0)


/**
 * The macro to get parameters of type 'int64'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_INT64_PARAM(var_name_) \
    do {                                                        \
        const char *str_val_;                                   \
                                                                \
        str_val_ = test_get_param(argc, argv, #var_name_);      \
        if (str_val_ == NULL)                                   \
        {                                                       \
            TEST_STOP;                                          \
        }                                                       \
        if (sscanf(str_val_, "%lld",                            \
                   (long long int *)&(var_name_)) != 1)         \
        {                                                       \
            TEST_FAIL("The value of '%s' parameter should be "  \
                      "an integer, but it is %s", #var_name_,   \
                      str_val_);                                \
        }                                                       \
    } while (0)

/**
 * The macro to get parameters of type 'double' ('float' also may be used)
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_DOUBLE_PARAM(var_name_) \
    do {                                                        \
        const char *str_val_;                                   \
        char       *end_ptr;                                    \
                                                                \
        str_val_ = test_get_param(argc, argv, #var_name_);      \
        if (str_val_ == NULL)                                   \
        {                                                       \
            TEST_STOP;                                          \
        }                                                       \
        (var_name_) = strtod(str_val_, &end_ptr);               \
        if (end_ptr == str_val_ || *end_ptr != '\0')            \
        {                                                       \
            TEST_FAIL("The value of '%s' parameter should be "  \
                      "a double, but it is %s", #var_name_,     \
                      str_val_);                                \
        }                                                       \
        if (((var_name_) == 0 ||                                \
             (var_name_) == + HUGE_VAL ||                       \
             (var_name_) == - HUGE_VAL) &&                      \
            errno == ERANGE)                                    \
        {                                                       \
            TEST_FAIL("The value of '%s' parameter is too "     \
                      "large or too small: %s", #var_name_,     \
                      str_val_);                                \
        }                                                       \
    } while (0)

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
            ((var_name_) = calloc(sizeof(int), (var_len_))) == NULL)    \
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
 * The list of values allowed for parameter of type 'te_bool'
 */
#define BOOL_MAPPING_LIST \
            { "TRUE", (int)TRUE }, \
            { "FALSE", (int)FALSE }

/**
 * Get the value of parameter of type 'te_bool'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'te_bool' (OUT)
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

/** ID assigned by the Tester to the test instance */
extern unsigned int te_test_id;

/**
 * Finds a particulat parameter in the list of parameters
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
                                struct param_map_entry *maps,
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
 * Scalable sleep (sleep scale times for _to_sleep seconds).
 *
 * @param _to_sleep     number of seconds to sleep is scale is 1
 */
#define SLEEP(_to_sleep)   te_sleep(test_sleep_scale() * (_to_sleep))

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
