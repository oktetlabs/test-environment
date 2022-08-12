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
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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
