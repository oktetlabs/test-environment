/** @file
 * @brief Test API for stress tool routine
 *
 * Test API to control 'stress' tool.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "TAPI STRESS"

#include <stddef.h>
#include <signal.h>

#include "tapi_stress.h"
#include "te_alloc.h"
#include "tapi_cfg_cpu.h"

struct tapi_stress_app {
    tapi_job_t *job;
    tapi_job_channel_t *out_chs[2];
    tapi_job_channel_t *wrong_usage_filter;
};

static const tapi_job_opt_bind stress_tool_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_OMITTABLE("--cpu", FALSE, NULL, tapi_stress_opt, cpu),
    TAPI_JOB_OPT_UINT_OMITTABLE("--io", FALSE, NULL, tapi_stress_opt, io),
    TAPI_JOB_OPT_UINT_OMITTABLE("--vm", FALSE, NULL, tapi_stress_opt, vm),
    TAPI_JOB_OPT_UINT_OMITTABLE("--timeout", FALSE, NULL, tapi_stress_opt,
                                timeout_s)
);

static te_errno
tapi_stress_complete_opts(tapi_stress_opt *opt,
                          const tapi_job_factory_t *factory)
{
    const char *ta;
    size_t n_cpus;
    te_errno rc;

    if (opt->cpu != 0)
        return 0;

    ta = tapi_job_factory_ta(factory);
    if (ta == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = tapi_cfg_get_all_threads(ta, &n_cpus, NULL);
    if (rc != 0)
        return rc;

    opt->cpu = n_cpus;
    return 0;
}

te_errno
tapi_stress_create(tapi_job_factory_t *factory,
                   const tapi_stress_opt *opt,
                   struct tapi_stress_app **app)
{
    struct tapi_stress_app *result = NULL;
    tapi_stress_opt effective_opt = *opt;
    const char *path = "stress";
    te_vec args;
    te_errno rc;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    rc = tapi_stress_complete_opts(&effective_opt, factory);
    if (rc != 0)
        goto out;

    rc = tapi_job_opt_build_args(path, stress_tool_binds, &effective_opt,
                                 &args);
    if (rc != 0)
        goto out;

    rc = tapi_job_simple_create(factory,
                                &(tapi_job_simple_desc_t){
                                    .program = path,
                                    .argv = (const char **)args.data.ptr,
                                    .job_loc = &result->job,
                                    .stdout_loc = &result->out_chs[0],
                                    .stderr_loc = &result->out_chs[1],
                                    .filters = TAPI_JOB_SIMPLE_FILTERS(
                                        {.use_stderr = TRUE,
                                         .log_level = TE_LL_ERROR,
                                         .readable = FALSE,
                                         .filter_name = "stress stderr",
                                        },
                                        {.use_stdout = TRUE,
                                         .log_level = TE_LL_RING,
                                         .readable = FALSE,
                                         .filter_name = "stress stdout",
                                        },
                                        {.use_stdout = TRUE,
                                         .use_stderr = TRUE,
                                         .log_level = TE_LL_ERROR,
                                         .readable = TRUE,
                                         .filter_var = &result->wrong_usage_filter,
                                         .filter_name = "stress usage error",
                                         .re = "Usage:\\s*stress"
                                        }
                                     )
                                });
    te_vec_deep_free(&args);
    if (rc != 0)
        goto out;

    *app = result;

out:
    if (rc != 0)
        free(result);

    return rc;
}

te_errno
tapi_stress_start(struct tapi_stress_app *app)
{
    te_errno rc;
    static const int usage_poll_timeout_ms = 100;

    rc = tapi_job_start(app->job);
    if (rc != 0)
        return rc;

    rc = tapi_job_poll(TAPI_JOB_CHANNEL_SET(app->wrong_usage_filter),
                       usage_poll_timeout_ms);
    if (rc == 0)
    {
        ERROR("Wrong stress tool usage");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    else if (rc != TE_ETIMEDOUT)
    {
        ERROR("Failed to poll stress tool");
        return rc;
    }

    return 0;
}

te_errno
tapi_stress_stop(struct tapi_stress_app *app, int timeout_ms)
{
    tapi_job_status_t status;
    te_errno rc;

    rc = tapi_job_killpg(app->job, SIGTERM);
    if (rc != 0 && rc != TE_ESRCH)
    {
        ERROR("Failed to kill stress tool");
        return rc;
    }

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
    {
        ERROR("Failed to wait for stress tool");
        return rc;
    }

    if ((status.type == TAPI_JOB_STATUS_EXITED && status.value != 0) ||
        (status.type == TAPI_JOB_STATUS_SIGNALED && status.value != SIGTERM) ||
        status.type == TAPI_JOB_STATUS_UNKNOWN)
    {
        ERROR("The stress tool exited abnormally");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return 0;
}

void
tapi_stress_destroy(struct tapi_stress_app *app)
{
    te_errno rc;

    if (app == NULL)
        return;

    rc = tapi_job_destroy(app->job, -1);
    if (rc != 0)
        ERROR("The stress tool destruction failed");

    free(app);
}
