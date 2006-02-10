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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

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
