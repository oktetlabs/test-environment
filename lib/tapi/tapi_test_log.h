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

#ifndef __TE_TAPI_TEST_LOG_H__
#define __TE_TAPI_TEST_LOG_H__

#include "te_errno.h"
#include "logger_api.h"
#include "tapi_jmp.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Terminate a test with success status.
 */
#define TEST_SUCCESS    (TAPI_JMP_DO(0))

/**
 * Terminate a test with failure status. It is assumed that error is
 * already reported.
 */
#define TEST_STOP       (TAPI_JMP_DO(TE_EFAIL))

/**
 * Terminate a test with failure status, report an error.
 *
 * @param fmt       error message format string with parameters
 */
#define TEST_FAIL(fmt...) \
    do {                                                            \
        ERROR("Test Failed in %s, line %d, %s()",                   \
              __FILE__, __LINE__, __FUNCTION__);                    \
        ERROR(fmt);                                                 \
        TEST_STOP;                                                  \
    } while (0)

/**
 * Macro should be used to output verdict from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define RING_VERDICT(fmt...) \
    do {                                        \
        TE_LOG_RING(TE_LOG_CMSG_USER, fmt);     \
        te_test_verdict(fmt);                   \
    } while (0)

/**
 * Macro should be used to output verdict with WARN log level from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define WARN_VERDICT(fmt...) \
    do {                                        \
        TE_LOG_WARN(TE_LOG_CMSG_USER, fmt);     \
        te_test_verdict(fmt);                   \
    } while (0)

/**
 * Macro should be used to output verdict with ERROR log level from tests.
 *
 * @param fmt  the content of the verdict as format string with arguments
 */
#define ERROR_VERDICT(fmt...) \
    do {                                        \
        TE_LOG_ERROR(TE_LOG_CMSG_USER, fmt);    \
        te_test_verdict(fmt);                   \
    } while (0)

/**
 * Terminate a test with failure status, report an error as verdict.
 *
 * @param fmt       error message format string with parameters
 */
#define TEST_VERDICT(fmt...) \
    do {                        \
        ERROR_VERDICT(fmt);     \
        TEST_STOP;              \
    } while (0)


/**
 * Compose test verdict message and send it to Tester.
 *
 * @param fmt           printf()-like format string with TE extensions
 *
 * @note The function uses @e te_test_id global variable.
 */
extern void te_test_verdict(const char *fmt, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TEST_LOG_H__ */
