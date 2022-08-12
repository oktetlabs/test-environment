/** @file
 * @brief Print test state in case of failure
 *
 * Implementation of TAPI to print test state in case of failure
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "Test Fail State TAPI"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif

/** Maximum length of string describing test state */
#define TEST_STATE_LEN_MAX    1000

/**
 * State of the test that should be logged in case of test failure if
 * @b log_test_fail_state behaviour is enabled.
 */
static char test_fail_state[TEST_STATE_LEN_MAX] = { '\0' };
/** Test substate to be logged together with @b test_fail_state */
static char test_fail_substate[TEST_STATE_LEN_MAX] = { '\0' };

/* See description in tapi_test_log.h */
void
te_test_fail_state_update(const char *fmt, ...)
{
    va_list ap;

    if (fmt != NULL)
    {
        va_start(ap, fmt);
        (void)vsnprintf(test_fail_state, sizeof(test_fail_state),
                        fmt, ap);
        va_end(ap);
    }
    else
    {
        test_fail_state[0] = '\0';
    }
}

/* See description in tapi_test_log.h */
void
te_test_fail_substate_update(const char *fmt, ...)
{
    va_list ap;

    if (fmt != NULL)
    {
        va_start(ap, fmt);
        (void)vsnprintf(test_fail_substate, sizeof(test_fail_substate),
                        fmt, ap);
        va_end(ap);
    }
    else
    {
        test_fail_substate[0] = '\0';
    }
}

/* See description in tapi_test_log.h */
const char *
te_test_fail_state_get(void)
{
    if (test_fail_state[0] != '\0')
        return test_fail_state;
    else
        return NULL;
}

/* See description in tapi_test_log.h */
const char *
te_test_fail_substate_get(void)
{
    if (test_fail_substate[0] != '\0')
        return test_fail_substate;
    else
        return NULL;
}
