/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unbound DNS server tool TAPI.
 *
 * TAPI to handle unbound DNS server tool.
 */

#define TE_LGR_USER "TAPI UNBOUND"

#include <signal.h>

#include "tapi_dns_unbound.h"
#include "tapi_job_opt.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_enum.h"

#define TAPI_DNS_UNBOUND_PATH "unbound"

#define TAPI_DNS_UNBOUND_TERM_TIMEOUT_MS 3000

const tapi_dns_unbound_opt tapi_dns_unbound_default_opt = {
    .unbound_path = NULL,
    .cfg_file = NULL,
    .verbose = TAPI_DNS_UNBOUND_NOT_VERBOSE,
};

/** Mapping of possible values for unbound::verbose option. */
static const te_enum_map tapi_dns_unbound_verbose_mapping[] = {
    {.name = "-v",     .value = TAPI_DNS_UNBOUND_VERBOSE},
    {.name = "-vv",    .value = TAPI_DNS_UNBOUND_MORE_VERBOSE},
    {.name = "-vvv",   .value = TAPI_DNS_UNBOUND_VERBOSE_LL_QUERY},
    {.name = "-vvvv",  .value = TAPI_DNS_UNBOUND_VERBOSE_LL_ALGO},
    {.name = "-vvvvv", .value = TAPI_DNS_UNBOUND_VERBOSE_LL_CACHE},
    TE_ENUM_MAP_END
};

static const tapi_job_opt_bind unbound_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("-c", FALSE, tapi_dns_unbound_opt, cfg_file),
    TAPI_JOB_OPT_DUMMY("-dp"),
    TAPI_JOB_OPT_ENUM(NULL, FALSE, tapi_dns_unbound_opt, verbose,
                      tapi_dns_unbound_verbose_mapping)
);

te_errno
tapi_dns_unbound_create(tapi_job_factory_t *factory,
                        const tapi_dns_unbound_opt *opt,
                        tapi_dns_unbound_app **app)
{
    const char *exec_path = TAPI_DNS_UNBOUND_PATH;
    tapi_dns_unbound_app *unbound_app = NULL;
    te_vec unbound_args = TE_VEC_INIT(char *);
    te_errno rc;

    unbound_app = TE_ALLOC(sizeof(*unbound_app));

    if (opt == NULL)
        opt = &tapi_dns_unbound_default_opt;

    if (opt->unbound_path != NULL)
        exec_path = opt->unbound_path;

    rc = tapi_job_opt_build_args(exec_path, unbound_binds, opt, &unbound_args);
    if (rc != 0)
    {
        ERROR("Failed to build unbound server options");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                            &(tapi_job_simple_desc_t){
                                .program = exec_path,
                                .argv = (const char **)unbound_args.data.ptr,
                                .job_loc = &unbound_app->job,
                                .stdout_loc = &unbound_app->out_chs[0],
                                .stderr_loc = &unbound_app->out_chs[1],
                                .filters = TAPI_JOB_SIMPLE_FILTERS(
                                    {
                                        .use_stdout = TRUE,
                                        .readable = FALSE,
                                        .re = "\\[[0-9]+\\].+:.+: (.*)",
                                        .extract = 1,
                                        .log_level = TE_LL_RING,
                                        .filter_var = &unbound_app->out_filter,
                                        .filter_name = "unbound.out",
                                    },
                                    {
                                        .use_stderr = TRUE,
                                        .readable = FALSE,
                                        .re = ".+(notice|debug): (.*)",
                                        .extract = 2,
                                        .log_level = TE_LL_RING,
                                        .filter_var = &unbound_app->info_filter,
                                        .filter_name = "unbound.info",
                                    },
                                    {
                                        .use_stderr = TRUE,
                                        .readable = FALSE,
                                        .re = ".+error: (.*)",
                                        .extract = 1,
                                        .log_level = TE_LL_ERROR,
                                        .filter_var = &unbound_app->err_filter,
                                        .filter_name = "unbound.err",
                                    })
                            });
    if (rc != 0)
    {
        ERROR("Failed to create job instance for unbound server tool");
        goto out;
    }

    *app = unbound_app;

out:
    te_vec_deep_free(&unbound_args);
    if (rc != 0)
        free(unbound_app);

    return rc;
}

te_errno
tapi_dns_unbound_start(tapi_dns_unbound_app *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_dns_unbound_wait(tapi_dns_unbound_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    TAPI_JOB_CHECK_STATUS(status);

    return 0;
}

te_errno
tapi_dns_unbound_kill(tapi_dns_unbound_app *app, int signum)
{
    return tapi_job_kill(app->job, signum);
}

te_errno
tapi_dns_unbound_stop(tapi_dns_unbound_app *app)
{
    return tapi_job_stop(app->job, SIGTERM, TAPI_DNS_UNBOUND_TERM_TIMEOUT_MS);
}

te_errno
tapi_dns_unbound_destroy(tapi_dns_unbound_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_DNS_UNBOUND_TERM_TIMEOUT_MS);
    if (rc != 0)
        ERROR("Failed to destroy unbound job");

    free(app);

    return rc;
}
