/** @file
 * @brief TAPI Job test suite epilogue
 *
 * TAPI Job test suite epilogue
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page job-epilogue Recreate job
 *
 * @objective Check support of TAPI Job recreate feature
 *
 * Recreate a job that was started in the test suite prologue and stop it.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "job/epilogue"

#include "te_config.h"

#include "tapi_job.h"
#include "tapi_job_factory_cfg.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    const char *ta = "Agt_A";
    const char *job_name = "date_job";
    tapi_job_factory_t *factory;
    tapi_job_t *job;

    int autorestart_timeout_s = 3;

    TEST_START;

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_cfg_create(ta, &factory));

    TEST_STEP("Recreate date job");
    CHECK_RC(tapi_job_recreate(factory, job_name, &job));

    VSLEEP(autorestart_timeout_s, "Wait for the job to produce output");

    TEST_STEP("Stop the job");
    CHECK_RC(tapi_job_stop(job, -1, -1));

    VSLEEP(autorestart_timeout_s, "Wait to ensure that the job stops producing "
                                  "output");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_job_destroy(job, -1));
    tapi_job_factory_destroy(factory);

    TEST_END;
}
