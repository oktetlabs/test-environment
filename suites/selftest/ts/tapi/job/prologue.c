/** @file
 * @brief TAPI Job test suite prologue
 *
 * TAPI Job test suite prologue
 *
 * Copyright (C) 2022 OKTET Labs Ltd., St.-Petersburg, Russia
 */

/** @page job-prologue Start date process
 *
 * @objective Check support of autorestart and recreate TAPI Job features
 *
 * Execute `date +%T` process with autorestart value set to 3 seconds so that
 * the process is restarted every 3 seconds and prints its output to log
 * during execution of all tests in this test suite.
 *
 * @par Scenario:
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME "job/prologue"

#include "te_config.h"

#include "tapi_job.h"
#include "tapi_job_factory_cfg.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    const char *ta = "Agt_A";
    const char *job_name = "date_job";
    const char *tool = "date";
    tapi_job_factory_t *factory;
    tapi_job_t *job;

    int autorestart_timeout_s = 3;

    TEST_START;

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_cfg_create(ta, &factory));

    TEST_STEP("Create date job");
    CHECK_RC(tapi_job_create_named(factory, job_name, NULL, tool,
                                   (const char *[]){tool, "+%T", NULL}, NULL,
                                   &job));

    TEST_STEP("Set autorestart for the job");
    CHECK_RC(tapi_job_set_autorestart(job, autorestart_timeout_s));

    TEST_STEP("Start the job");
    CHECK_RC(tapi_job_start(job));

    TEST_SUCCESS;

cleanup:
    tapi_job_factory_destroy(factory);

    TEST_END;
}
