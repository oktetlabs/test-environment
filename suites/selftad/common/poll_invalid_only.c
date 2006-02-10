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

/** @page common-poll_invalid_only Call traffic poll operation for invalid only CSAPs
 *
 * @objective Check @b rcf_trpoll() behaviour with only invalid CSAP.
 *
 * @param n_csaps       Number of CSAPs
 * @param ta_null       @c NULL Test Agent name or not
 * @param zero_timeout  Zero timeout or not
 * @param zero_status   Initialize status as zero or not
 *
 * @par Scenario:
 * 
 * -# Prepare @b rcf_trpoll_csap structure with @a n_csaps entries:
 *      - @a ta is @c NULL or @c not-NULL (unknown TA);
 *      - @a csap is @c CSAP_INVALID_HANDLE;
 *      - @a status is @c 0 or @c -1.
 * -# Call @b rcf_trpoll() with prepared structure and zero or non-zero
 *    timeout. Check that @c 0 is returned and @a status is set to
 *    @c TE_ETADCSAPNOTEX.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#define TE_TEST_NAME    "common/poll_invalid_only"

#include "te_config.h"

#include "tapi_test.h"


int
main(int argc, char *argv[])
{
    rcf_trpoll_csap    *csaps = NULL;
    unsigned int        n_csaps;
    te_bool             ta_null;
    te_bool             zero_timeout;
    te_bool             zero_status;
    unsigned int        i;

    TEST_START;
    TEST_GET_INT_PARAM(n_csaps);
    TEST_GET_BOOL_PARAM(ta_null);
    TEST_GET_BOOL_PARAM(zero_timeout);
    TEST_GET_BOOL_PARAM(zero_status);

    if (n_csaps == 0)
        TEST_FAIL("Invalid number of CSAPs as parameter");

    CHECK_NOT_NULL(csaps = calloc(n_csaps, sizeof(*csaps)));
    
    for (i = 0; i < n_csaps; ++i)
    {
        csaps[i].ta = ta_null ? NULL : "UnknownTA";
        csaps[i].csap_id = CSAP_INVALID_HANDLE;
        csaps[i].status = zero_status ? 0 : -1;
    }

    rc = rcf_trpoll(csaps, n_csaps, zero_timeout ? 0 : rand_range(1, 1000));
    if (rc != 0)
    {
        TEST_FAIL("rcf_trpoll() with CSAP_INVALID_HANDLE failed: %r",
                  rc);
    }
    for (i = 0; i < n_csaps; ++i)
    {
        if (TE_RC_GET_ERROR(csaps[i].status) != TE_ETADCSAPNOTEX)
        {
            TEST_FAIL("rcf_trpoll() with CSAP_INVALID_HANDLE in #%u "
                      "request set status to %r instead of %r",
                      i, csaps[i].status, TE_ETADCSAPNOTEX);
        }
    }

    TEST_SUCCESS;

cleanup:

    free(csaps);

    TEST_END;
}
