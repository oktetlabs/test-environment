/** @file
 * @brief TAPI Job test suite CFG basic operations test
 *
 * TAPI Job test suite CFG basic operations test
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd., St.-Petersburg, Russia
 */

/** @page job-cfg_basic_operations Perform basic TAPI Job operations with ping created by CFG factory
 *
 * @objective Check support for creating, starting, waiting, killing, stopping,
 *            and destroying jobs created by CFG factory
 *
 * @par Scenario:
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME "job/cfg_basic_operations"

#include "te_config.h"

#include "tapi_job.h"
#include "tapi_job_factory_cfg.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    const char *ta = "Agt_A";
    const char *tool = "ping";
    tapi_job_factory_t *factory;
    tapi_job_t *job;
    tapi_job_status_t exit_status;

    te_errno res;
    int timeout_s = 3;

    TEST_START;

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_cfg_create(ta, &factory));

    TEST_STEP("Create ping job");
    CHECK_RC(tapi_job_create(factory, NULL, tool,
                             (const char *[]){tool, "localhost", NULL},
                             NULL, &job));

    TEST_STEP("Start the job");
    CHECK_RC(tapi_job_start(job));

    TEST_STEP("Wait for the process to run for %d seconds", timeout_s);
    res = tapi_job_wait(job, TE_SEC2MS(timeout_s), NULL);
    if (TE_RC_GET_ERROR(res) != TE_EINPROGRESS)
        TEST_FAIL("Ping is not running");

    TEST_STEP("Send SIGTERM");
    CHECK_RC(tapi_job_kill(job, SIGTERM));

    TEST_STEP("Check exit status of the job");
    CHECK_RC(tapi_job_wait(job, 0, &exit_status));
    if (exit_status.type != TAPI_JOB_STATUS_SIGNALED)
    {
        TEST_FAIL("Exit status of the process does not represent that it was "
                  "killed by signal");
    }
    else if (exit_status.value != SIGTERM)
    {
        TEST_FAIL("Exit status value is supposed to be equal to %d (SIGTERM "
                  "signal number), but the real value is %d",
                  SIGTERM, exit_status.value);
    }
    else
    {
        RING("The process was signaled via SIGTERM as expected");
    }

    TEST_STEP("Start the job over");
    CHECK_RC(tapi_job_start(job));

    VSLEEP(timeout_s, "Wait for the job to run");

    TEST_STEP("Stop the job");
    CHECK_RC(tapi_job_stop(job, -1, -1));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_job_destroy(job, -1));
    tapi_job_factory_destroy(factory);

    TEST_END;
}
