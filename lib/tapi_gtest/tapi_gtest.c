/** @file
 * @brief Wrapper for Google Test
 *
 * Test API for run Google Test binaries
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAPI GTest"

#include "te_config.h"
#include "tapi_test_log.h"
#include "te_vector.h"

#include "tapi_gtest.h"

static void
gtest_args_free(te_vec *gtest_args)
{
    char **arg = NULL;

    TE_VEC_FOREACH(gtest_args, arg)
        free(*arg);

    te_vec_free(gtest_args);
}

static te_errno
gtest_build_command(tapi_gtest *gtest, te_vec *args)
{
    te_errno rc;
    te_vec result = TE_VEC_INIT(const char *);
    const char *end_arg = NULL;

#define ADD_ARG(cmd_fmt...)                                             \
    do {                                                                \
        te_string buf = TE_STRING_INIT;                                 \
                                                                        \
        rc = te_string_append(&buf, cmd_fmt);                           \
                                                                        \
        if (rc != 0 || (rc = TE_VEC_APPEND(&result, buf.ptr)) != 0)     \
        {                                                               \
            te_string_free(&buf);                                       \
            gtest_args_free(&result);                                   \
            return rc;                                                  \
        }                                                               \
    } while(0)

    ADD_ARG("%s", gtest->bin);
    ADD_ARG("--gtest_filter=%s.%s", gtest->group, gtest->name);
    ADD_ARG("--gtest_random_seed=%d", gtest->rand_seed);
    ADD_ARG("--gtest_color=no");
    if (gtest->run_disabled)
        ADD_ARG("--gtest_also_run_disabled_tests");

#undef ADD_ARG

    if ((rc = TE_VEC_APPEND(&result, end_arg)) != 0)
    {
        gtest_args_free(&result);
        return rc;
    }

    *args = result;
    return 0;
}

/* See description in tapi_gtest.h */
te_errno
tapi_gtest_init(tapi_gtest *gtest, rcf_rpc_server *rpcs)
{
    te_errno rc;
    te_vec gtest_args = TE_VEC_INIT(const char *);
    tapi_job_simple_desc_t desc;

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
                .use_stdout = TRUE,
                .filter_name = "stdout-gtest",
                .log_level = TE_LL_RING,
            },
            {
                .use_stderr = TRUE,
                .filter_name = "stderr-gtest",
                .log_level = TE_LL_ERROR,
            }
        ),
    };

    if ((rc = gtest_build_command(gtest, &gtest_args)) != 0)
        return rc;

    desc.argv = (const char **) gtest_args.data.ptr;
    rc = tapi_job_rpc_simple_create(rpcs, &desc);

    gtest_args_free(&gtest_args);
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
    return tapi_job_kill(gtest->impl.job, SIGINT);
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
