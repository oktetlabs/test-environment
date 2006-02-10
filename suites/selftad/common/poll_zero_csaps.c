/** @file
 * @brief Test Environment
 *
 * Tests on generic TAD functionality.
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 * 
 * $Id: example.c 18619 2005-09-22 16:47:08Z artem $
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

#define TE_TEST_NAME    "common/poll_zero_csaps"

#include "te_config.h"

#include "logger_api.h"
#include "rcf_api.h"

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
