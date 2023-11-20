/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief HAProxy tool TAPI
 *
 * TAPI to manage HAProxy tool.
 */

#define TE_LGR_USER "TAPI HAPROXY"

#include "tapi_haproxy.h"
#include "tapi_file.h"
#include "te_alloc.h"
#include "conf_api.h"

const tapi_haproxy_opt tapi_haproxy_default_opt = {
    .haproxy_path = NULL,
    .cfg_file = NULL,
    .cfg_opt = NULL,
    .verbose = FALSE,
};

static const te_enum_map tapi_haproxy_verbose_mapping[] = {
    {.name = "-V", .value = TRUE},
    {.name = "",  .value = FALSE},
    TE_ENUM_MAP_END
};

static const tapi_job_opt_bind haproxy_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_ENUM_BOOL(NULL, FALSE, tapi_haproxy_opt, verbose,
                           tapi_haproxy_verbose_mapping),
    TAPI_JOB_OPT_STRING("-f", FALSE, tapi_haproxy_opt, cfg_file)
);

/* See description in tapi_haproxy.h */
te_errno
tapi_haproxy_create(tapi_job_factory_t *factory, const tapi_haproxy_opt *opt,
                    tapi_haproxy_app **app)
{
    tapi_haproxy_app *result = NULL;
    const char *path = TAPI_HAPROXY_PATH;
    const char *ta;
    te_vec haproxy_args = TE_VEC_INIT(char *);
    tapi_haproxy_opt opt_copy;
    te_errno rc;

    ta = tapi_job_factory_ta(factory);
    assert(ta != NULL);

    result = TE_ALLOC(sizeof(*result));

    opt_copy = opt ? *opt : tapi_haproxy_default_opt;

    if (opt_copy.haproxy_path != NULL)
        path = opt_copy.haproxy_path;

    if (opt_copy.cfg_file == NULL && opt_copy.cfg_opt != NULL)
    {
        tapi_haproxy_cfg_create(ta, opt_copy.cfg_opt,
                                &result->generated_cfg_file);
        opt_copy.cfg_file = result->generated_cfg_file;
    }

    rc = tapi_job_opt_build_args(path, haproxy_binds, &opt_copy,
                                 &haproxy_args);
    if (rc != 0)
    {
        ERROR("Failed to build HAProxy options: %r", rc);
        goto cleanup;
    }

    rc = tapi_job_simple_create(factory,
                            &(tapi_job_simple_desc_t){
                                .program = path,
                                .argv = (const char **)haproxy_args.data.ptr,
                                .job_loc = &result->job,
                                .stdout_loc = &result->out_chs[0],
                                .stderr_loc = &result->out_chs[1],
                                .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = TRUE,
                                        .log_level   = TE_LL_RING,
                                        .filter_name = "haproxy stdout",
                                        .filter_var = &result->stdout_filter
                                    },
                                    {
                                        .use_stderr  = TRUE,
                                        .readable    = TRUE,
                                        .log_level   = TE_LL_WARN,
                                        .filter_name = "haproxy stderr",
                                        .filter_var = &result->stderr_filter
                                    }
                                )
                            });

    if (rc != 0)
    {
        ERROR("Failed to create job instance for HAProxy tool: %r", rc);
        goto cleanup;
    }

    *app = result;

cleanup:
    te_vec_deep_free(&haproxy_args);
    if (rc != 0)
    {
        tapi_haproxy_cfg_destroy(ta, result->generated_cfg_file);
        free(result->generated_cfg_file);
        free(result);
    }

    return rc;
}

/* See description in tapi_haproxy.h */
te_errno
tapi_haproxy_start(tapi_haproxy_app *app)
{
    return tapi_job_start(app->job);
}

/* See description in tapi_haproxy.h */
te_errno
tapi_haproxy_wait(tapi_haproxy_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    TAPI_JOB_CHECK_STATUS(status);

    return 0;
}

/* See description in tapi_haproxy.h */
te_errno
tapi_haproxy_kill(tapi_haproxy_app *app, int signo)
{
    return tapi_job_kill(app->job, signo);
}

/* See description in tapi_haproxy.h */
te_errno
tapi_haproxy_destroy(tapi_haproxy_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    if (app->generated_cfg_file != NULL)
    {
        tapi_job_factory_t *factory = tapi_job_get_factory(app->job);
        const char *ta = tapi_job_factory_ta(factory);
        assert(ta != NULL);

        tapi_haproxy_cfg_destroy(ta, app->generated_cfg_file);
        free(app->generated_cfg_file);
    }

    rc = tapi_job_destroy(app->job, TAPI_HAPROXY_TERM_TIMEOUT_MS);
    if (rc != 0)
        ERROR("Failed to destroy HAPoxy job: %r", rc);

    free(app);

    return rc;
}