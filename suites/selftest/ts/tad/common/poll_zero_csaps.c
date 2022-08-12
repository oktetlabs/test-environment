/** @file
 * @brief Test Environment
 *
 * Tests on generic TAD functionality.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 */

/** @page common-poll_zero_csaps Call traffic poll operation with zero CSAPs
 *
 * @objective Check that @b rcf_trpoll() returns @c TE_EINVAL, if it is
 *            called zero number of CSAPs.
 *
 * @param csaps_null    Should @a csaps pointer be @c NULL
 *
 * @par Scenario:
 *
 * -# Call @b rcf_trpoll( @c NULL or @c not-NULL, @c 0, @c 0 ).
 *    Check that @c TE_EINVAL is returned.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "common/poll_zero_csaps"

#include "te_config.h"

#include "tapi_test.h"


int
main(int argc, char *argv[])
{
    rcf_trpoll_csap     csaps = { NULL, CSAP_INVALID_HANDLE, 0 };
    te_bool             csaps_null;

    TEST_START;
    TEST_GET_BOOL_PARAM(csaps_null);

    rc = rcf_trpoll(csaps_null ? NULL : &csaps, 0, 0);
    if (TE_RC_GET_ERROR(rc) != TE_EINVAL)
    {
        TEST_FAIL("rcf_trpoll(%sNULL, 0, 0) returned %r instead of %r",
                  csaps_null ? "" : "not-", rc, TE_EINVAL);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
