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

#include "rcf_api.h"
#include "conf_api.h"
#include "logger_api.h"
#include "logger_ten.h"
#include "tapi_jmp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TE_LGR_USER
/* Tests must define TE_TEST_NAME to be used as 'te_lgr_entity'. */
#ifndef TE_TEST_NAME
#error TE_TEST_NAME must be defined before include of tapi_test.h
#endif
#endif

/* 
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
        result = (TE_RC_GET_ERROR(jmp_rc) == TE_EOK) ? EXIT_SUCCESS  \
                                                     : EXIT_FAILURE; \
        goto cleanup;                                                \
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
    (void)signal(SIGINT, sigint_handler);                           \
                                                                    \
    /* Initialize pseudo-random generator */                        \
    srand(time(NULL));                                              \
                                                                    \
    TAPI_ON_JMP(TEST_ON_JMP_DO);                                    \
    TEST_GET_INT_PARAM(te_test_id);                                 \
                                                                    \
    TEST_START_SPECIFIC

/**
 * The last action of the test @b main() function.
 *
 * @note To define test-specific action define TEST_END_SPECIFIC macro
 */
#define TEST_END \
    TEST_END_SPECIFIC;                                              \
    return result

/**
 * Terminate a test with success status.
 */
#define TEST_SUCCESS    (TAPI_JMP_DO(0))

/**
 * Terminate a test with failure status. It is assumed that error is
 * already reported.
 */
#define TEST_STOP       (TAPI_JMP_DO(TE_ETESTFAIL))

/**
 * Terminate a test with failure status, report an error.
 *
 * @param fmt       error message format string with parameters
 */
#define TEST_FAIL(fmt...) \
    do {                                                            \
        ERROR("Test Failed on line %d", __LINE__);                  \
        ERROR(fmt);                                                 \
        TEST_STOP;                                                  \
    } while (0)

/**
 * Macro should be used to output verdict from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define RING_VERDICT(fmt...) \
    LOG_RING(TE_LOG_CMSG_USER, fmt)

/**
 * Macro should be used to output verdict with ERROR log level from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define ERROR_VERDICT(fmt...) \
    LOG_ERROR(TE_LOG_CMSG_USER, fmt)

/**
 * Terminate a test with failure status, report an error as verdict.
 *
 * @param fmt       error message format string with parameters
 */
#define TEST_VERDICT(fmt..) \
    do {                    \
        ERROR_VERDICT(fmt); \
        TEST_STOP;          \
    } while (0)

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
    do {                                                       \
        int rc_;                                               \
                                                               \
        if ((rc_ = (expr_)) != 0)                              \
        {                                                      \
            ERROR("line %d: %s returns 0x%X, but expected 0",  \
                  __LINE__, # expr_, rc_);                     \
            result = EXIT_FAILURE;                             \
        }                                                      \
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


/**
 * Generic way to get mapped value of a parameter: string -> enum
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 * @param maps_      An array of mappings: string -> enum
 */
#define TEST_GET_ENUM_PARAM(var_name_, maps_...) \
    do {                                                               \
        const char *val_;                                              \
        int         mapped_val_;                                       \
        struct param_map_entry maps[] = {                              \
            maps_,                                                     \
            { NULL, 0 },                                               \
        };                                                             \
                                                                       \
        val_ = test_get_param(argc, argv, #var_name_);                 \
        if (val_ != NULL &&                                            \
            test_map_param_value(#var_name_, maps,                     \
                                 val_, &mapped_val_) == 0)             \
        {                                                              \
            (var_name_) = mapped_val_;                                 \
            break;                                                     \
        }                                                              \
        TEST_STOP;                                                     \
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
        (var_name_) = (int)strtol(str_val_, &end_ptr, 10);      \
        if (end_ptr == str_val_ || *end_ptr != '\0')            \
        {                                                       \
            TEST_FAIL("The value of '%s' parameter should be "  \
                      "an integer, but it is %s", #var_name_,   \
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


/** Define one entry in the list of maping entries */
#define MAPPING_LIST_ENTRY(entry_val_) \
    { #entry_val_, (int)RPC_ ## entry_val_ }

/** Entry for mapping parameter value from string to interger */
struct param_map_entry {
    const char *str_val; /**< value in string format */
    int         num_val; /**< Value in native numberic format */
};

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
 * @param str_val   String value
 * @param len       Octet string value length
 *
 * @return Octet string value or NULL int case of failure.
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
 * Signal handler to close TE when Ctrl-C is pressed.
 *
 * @param signum    - signal number
 */
extern void sigint_handler(int signum);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TEST_H__ */
