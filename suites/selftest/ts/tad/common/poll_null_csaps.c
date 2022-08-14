/** @file
 * @brief Test Environment
 *
 * Tests on generic TAD functionality.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page common-poll_null_csaps Call traffic poll operation with NULL CSAPs pointer and positive number of CSAPs
 *
 * @objective Check that @b rcf_trpoll() returns @c TE_EFAULT, if it is
 *            called with positive number of CSAPs and @c NULL @a csaps
 *            pointer.
 *
 * @par Scenario:
 *
 * -# Call @b rcf_trpoll( @c NULL, @c 1, @c 0 ).
 *    Check that @c TE_EFAULT is returned.
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "common/poll_null_csaps"

#include "te_config.h"

#include "tapi_test.h"


int
main(int argc, char *argv[])
{
    TEST_START;

    rc = rcf_trpoll(NULL, 1, 0);
    if (TE_RC_GET_ERROR(rc) != TE_EFAULT)
    {
        TEST_FAIL("rcf_trpoll(NULL, 1, 0) returned %r instead of %r",
                  rc, TE_EFAULT);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
