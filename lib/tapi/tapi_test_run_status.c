/** @file
 * @brief Test API to set/get test run status.
 *
 * Implementation of API to set/get test run status.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
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
