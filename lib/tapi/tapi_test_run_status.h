/** @file
 * @brief Test API to set/get test run status.
 *
 * @defgroup te_ts_tapi_test_run_status Test run status
 * @ingroup te_ts_tapi_test
 * @{
 *
 * Definition of API to set/get test run status.
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
 */

#ifndef __TE_TAPI_TEST_RUN_STATUS_H__
#define __TE_TAPI_TEST_RUN_STATUS_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Test run status. */
typedef enum {
    TE_TEST_RUN_STATUS_OK = 0,  /**< Test execution is OK. */
    TE_TEST_RUN_STATUS_FAIL,    /**< Some critical error occured
                                     during test execution. */
} te_test_run_status;

/**
 * Get test run status.
 *
 * @return Test run status.
 */
extern te_test_run_status tapi_test_run_status_get(void);

/**
 * Set test run status.
 *
 * @param status    Test run status.
 */
extern void tapi_test_run_status_set(te_test_run_status status);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TEST_RUN_STATUS_H__ */

/**@} <!-- END te_ts_tapi_test_run_status --> */
