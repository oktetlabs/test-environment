/** @file
 * @brief Test API
 *
 * Macros to be used in tests. The header must be included from test
 * sources only. It is allowed to use the macros only from @b main()
 * function of the test.
 *
 * Copyright (C) 2004 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LIB_TAPI_TEST_PARAMS_H__
#define __TE_LIB_TAPI_TEST_PARAMS_H__

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "rcf_api.h"
#include "conf_api.h"
#include "logger_api.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 
 * New tests must define TEST_NAME to be used as 'te_lgr_entity',
 * old tests define LGR_ENTITY.
 */
#ifndef TEST_NAME
#define TEST_NAME   LGR_ENTITY
#endif

/* 
 * Define empty list of test-specific variables 
 * if test does not care about them.
 */
#ifndef TEST_START_VARS
#define TEST_START_VARS
#endif

/* Define empty test start procedure if test does not care about it. */
#ifndef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC
#endif

/* Define empty test end procedure if test does not care about it. */
#ifndef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC do { } while (0)
#endif

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
    if (argc < 2)                                                   \
    {                                                               \
        ERROR("Incorrect number of arguments for the test");        \
        return EXIT_FAILURE;                                        \
    }                                                               \
    /* Shift programm name */                                       \
    argc--;                                                         \
    argv++;                                                         \
                                                                    \
    /* Initialize te_lgr_entity variable */                         \
    te_lgr_entity = TEST_NAME;                                      \
    /*                                                              \
     * Install SIGINT signal handler to exit() with failure status, \
     * if test is terminated by user by Ctrl-C.                     \
     */                                                             \
    (void)signal(SIGINT, sigint_handler);                           \
                                                                    \
    rc = cfg_create_backup(&test_backup_name);                      \
    if (rc != 0)                                                    \
    {                                                               \
        ERROR("Cannot create configuration backup: %X", rc);        \
        return EXIT_FAILURE;                                        \
    }                                                               \
    TEST_START_SPECIFIC

/**
 * The last action of the test @b main() function.
 *
 * @note To define test-specific action define TEST_END_SPECIFIC macro
 */
#define TEST_END \
    TEST_END_SPECIFIC;                                              \
    if (test_backup_name != NULL)                                   \
    {                                                               \
        rc = cfg_restore_backup(test_backup_name);                  \
        if (rc != 0)                                                \
        {                                                           \
            ERROR("Failed to restore configuration backup: %X", rc);\
            result = EXIT_FAILURE;                                  \
        }                                                           \
        free(test_backup_name);                                     \
    }                                                               \
    rcf_api_cleanup();                                              \
    return result

/**
 * Terminate a test with success status.
 */
#define TEST_SUCCESS \
    do {                                                            \
        result = EXIT_SUCCESS;                                      \
        goto cleanup;                                               \
    } while (0)

/**
 * Terminate a test with failure status, report an error.
 *
 * @param fmt       error message format string with parameters
 */
#define TEST_FAIL(fmt...) \
    do {                                                            \
        ERROR(fmt);                                                 \
        result = EXIT_FAILURE;                                      \
        goto cleanup;                                               \
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
    do {                                                        \
        int rc_;                                                \
                                                                \
        if ((rc_ = (expr_)) != 0)                               \
        {                                                       \
            TEST_FAIL("line %d: %s returns %d, but expected 0", \
                      __LINE__, # expr_, rc_);                  \
        }                                                       \
    } while (0)

/**
 * Check an expression passed as the argument against zero.
 * The same as CHECK_RC, but does not go to 'cleanup' label.
 *
 * @param expr_  Expression to be checked
 */
#define CLEANUP_CHECK_RC(expr_) \
    do {                                                     \
        int rc_;                                             \
                                                             \
        if ((rc_ = (expr_)) != 0)                            \
        {                                                    \
            ERROR("line %d: %s returns %d, but expected 0",  \
                  __LINE__, # expr_, rc_);                   \
            result = EXIT_FAILURE;                           \
        }                                                    \
    } while (0)

/**
 * Check that the expression is not NULL
 */
#define CHECK_NOT_NULL(expr_) \
    do {                                                                \
        if ((expr_) == NULL)                                            \
        {                                                               \
            TEST_FAIL("Expression " #expr_ " in file %s line %d is "    \
                      "expected to be nut NULL, but it is",             \
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
        result = EXIT_FAILURE;                                         \
        goto cleanup;                                                  \
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
            result = EXIT_FAILURE;                               \
            goto cleanup;                                        \
        }                                                        \
    } while (0)


/**
 * The macro to get parameters of type 'int'
 *
 * @param var_name_  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_INT_PARAM(var_name_) \
    do {                                                         \
        const char *str_val_;                                    \
        char       *end_ptr;                                     \
                                                                 \
        if (((str_val_) = test_get_param(argc, argv,             \
                                         #var_name_)) == NULL)   \
        {                                                        \
            result = EXIT_FAILURE;                               \
            goto cleanup;                                        \
        }                                                        \
        (var_name_) = (int)strtol(str_val_, &end_ptr, 10);       \
        if (end_ptr == str_val_ || *end_ptr != '\0')             \
        {                                                        \
            TEST_FAIL("The value of '%s' parameter should be "   \
                      "an integer, but it is %s", #var_name_,    \
                      str_val_);                                 \
        }                                                        \
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


/**
 * Name of the backup created before any test steps - local for the
 * file.
 */
extern char *test_backup_name;


/**
 * Finds a particulat parameter in the list of parameters
 *
 * @param argc  number of parameters
 * @param argv  Array of parameters in form <param name>=<param value>
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
 * Signal handler to close TE when Ctrl-C is pressed.
 *
 * @param signum    - signal number
 */
extern void sigint_handler(int signum);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LIB_TAPI_TEST_PARAMS_H__ */
