/** @file
 * @brief Test API to set/get test run status.
 *
 * Implementation of API to set/get test run status.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI test run status"

#include "te_config.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "tapi_test_run_status.h"

/** Test run status. */
static te_test_run_status test_run_status = TE_TEST_RUN_STATUS_OK;
/** Mutex protecting test_run_status. */
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

/* See description in tapi_test_run_status.h */
te_test_run_status
tapi_test_run_status_get(void)
{
    te_test_run_status rc;

    pthread_mutex_lock(&status_mutex);
    rc = test_run_status;
    pthread_mutex_unlock(&status_mutex);

    return rc;;
}

/* See description in tapi_test_run_status.h */
void
tapi_test_run_status_set(te_test_run_status status)
{
    pthread_mutex_lock(&status_mutex);
    test_run_status = status;
    pthread_mutex_unlock(&status_mutex);
}
