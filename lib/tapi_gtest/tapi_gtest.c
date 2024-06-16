/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Wrapper for Google Test
 *
 * Test API for run Google Test binaries
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI GTest"

#include <signal.h>
#include "te_config.h"
#include "tapi_test_log.h"
#include "te_vector.h"

#include "tapi_gtest.h"

static te_errno
gtest_build_argv(tapi_gtest *gtest, te_vec *argv)
{
    te_errno rc = 0;
    tapi_gtest_opts *opts = &gtest->opts;
    const tapi_job_opt_bind opts_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_STRING("--gtest_filter=", true, tapi_gtest_opts,
                            gtest_filter),
        TAPI_JOB_OPT_STRING("--device_name=", true, tapi_gtest_opts,
                            dev_name),
        TAPI_JOB_OPT_BOOL("--gtest_also_run_disabled_tests", tapi_gtest_opts,
                          run_disabled),
        TAPI_JOB_OPT_BOOL("--ipv4_only", tapi_gtest_opts, ipv4_only),
        TAPI_JOB_OPT_BOOL("--gtest_color=no", tapi_gtest_opts, no_col),
        TAPI_JOB_OPT_UINT_T("--gtest_random_seed=", true, NULL,
                            tapi_gtest_opts, rand_seed),
        TAPI_JOB_OPT_UINT_T("--verbs_mtu=", true, NULL,
                            tapi_gtest_opts, verbs_mtu)
    );

    rc = tapi_job_opt_build_args(gtest->bin, opts_binds, opts, argv);
    if (rc != 0)
        return rc;

    return 0;
}

/* See description in tapi_gtest.h */
te_errno
tapi_gtest_init(tapi_gtest *gtest, tapi_job_factory_t *factory)
{
    te_errno rc;
    te_vec gtest_argv = TE_VEC_INIT(const char *);
    tapi_job_simple_desc_t desc;
    te_string buf = TE_STRING_INIT;

    assert(gtest != NULL);
    assert(gtest->bin != NULL);
    assert(gtest->name != NULL);
    assert(gtest->group != NULL);

    desc = (tapi_job_simple_desc_t) {
        .program = gtest->bin,
        .job_loc = &gtest->impl.job,
        .stdout_loc = &gtest->impl.out[0],
        .stderr_loc = &gtest->impl.out[1],
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {
                .use_stdout = true,
                .filter_name = "stdout-gtest",
                .log_level = TE_LL_RING,
            },
            {
                .use_stderr = true,
                .filter_name = "stderr-gtest",
                .log_level = TE_LL_ERROR,
            }
        ),
    };

    te_string_append(&buf, "%s.%s", gtest->group, gtest->name);
    gtest->opts.gtest_filter = buf.ptr;
    if ((rc = gtest_build_argv(gtest, &gtest_argv)) != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    desc.argv = (const char **) gtest_argv.data.ptr;
    rc = tapi_job_simple_create(factory, &desc);

    te_vec_deep_free(&gtest_argv);
    return rc;
}

/* See description in tapi_gtest.h */
te_errno
tapi_gtest_start(tapi_gtest *gtest)
{
    assert(gtest != NULL);
    assert(gtest->name != NULL);
    assert(gtest->group != NULL);
    assert(gtest->impl.job != NULL);

    VERB("GTest '%s.%s' start", gtest->group, gtest->name);

    return tapi_job_start(gtest->impl.job);
}

/* See description in tapi_gtest.h */
te_errno
tapi_gtest_wait(tapi_gtest *gtest, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    assert(gtest != NULL);
    assert(gtest->name != NULL);
    assert(gtest->group != NULL);
    assert(gtest->impl.job != NULL);

    if ((rc = tapi_job_wait(gtest->impl.job, timeout_ms, &status)) != 0)
        return rc;

    if (status.value == 0 && status.type == TAPI_JOB_STATUS_EXITED)
        return 0;

    switch(status.type)
    {
        case TAPI_JOB_STATUS_EXITED:
            ERROR_ARTIFACT("GTest '%s.%s' exited with exit code %d",
                           gtest->group, gtest->name, status.value);
            return TE_EFAIL;
        case TAPI_JOB_STATUS_SIGNALED:
            ERROR_ARTIFACT("GTest '%s.%s' got signal %d: %s",
                           gtest->group, gtest->name, status.value,
                           strsignal(status.value));
            return TE_EFAIL;
        case TAPI_JOB_STATUS_UNKNOWN:
            ERROR_ARTIFACT("GTest '%s.%s': status unknown",
                           gtest->group, gtest->name);
            return TE_EFAIL;
    }

    /* unreachable */
    return TE_EFAIL;
}

static te_errno
gtest_stop(tapi_gtest *gtest)
{
    if (tapi_job_is_running(gtest->impl.job))
        return tapi_job_kill(gtest->impl.job, SIGINT);

    return 0;
}

te_errno
tapi_gtest_stop(tapi_gtest *gtest)
{
    assert(gtest != NULL);
    assert(gtest->impl.job != NULL);

    return gtest_stop(gtest);
}

/* See description in tapi_gtest.h */
te_errno
tapi_gtest_fini(tapi_gtest *gtest)
{
    te_errno rc;
    const int term_timeout_ms = -1;

    if (gtest == NULL || gtest->impl.job == NULL)
        return 0;

    if ((rc = gtest_stop(gtest)) != 0)
        return rc;

    return tapi_job_destroy(gtest->impl.job, term_timeout_ms);
}
