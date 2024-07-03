/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Control WPA client
 *
 * Test API to control WPA client tool.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "tapi_wpa_cli.h"
#include "te_str.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_job_opt.h"

/* Path to WPA client tool */
#define TAPI_WPA_CLI_TOOL "/sbin/wpa_cli"

/* Common wpa_cli arguments */
typedef struct wpa_cli_common_opt {
    /* Wireless network interface name wpa_cli should operate with */
    const char *ifname;
} wpa_cli_common_opt;

static const tapi_job_opt_bind wpa_cli_common_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("-i", false, wpa_cli_common_opt, ifname)
);

/* See description in tapi_wpa_cli.h */
te_errno
tapi_wpa_cli(rcf_rpc_server *rpcs, const char *ifname,
             const char *command[], int timeout_ms)
{
    tapi_job_factory_t *factory = NULL;
    tapi_job_t *job = NULL;
    tapi_job_status_t status;
    tapi_job_channel_t *chout;
    tapi_job_channel_t *cherr;
    wpa_cli_common_opt opts;
    te_vec args = TE_VEC_INIT(char *);
    te_errno rc;
    te_errno tmprc;

    if (te_str_is_null_or_empty(ifname) || command == NULL ||
        te_str_is_null_or_empty(command[0]))
    {
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    opts.ifname = ifname;

    rc = tapi_job_opt_build_args(TAPI_WPA_CLI_TOOL, wpa_cli_common_binds,
                                 &opts, &args);

    if (rc == 0)
        rc = tapi_job_opt_append_strings(command, &args);

    if (rc != 0)
    {
        ERROR("Failed to build job arguments");
        goto out;
    }

    rc = tapi_job_factory_rpc_create(rpcs, &factory);
    if (rc != 0)
        goto out;

    rc = tapi_job_simple_create(factory,
                &(tapi_job_simple_desc_t){
                    .program = TAPI_WPA_CLI_TOOL,
                    .argv = (const char **)args.data.ptr,
                    .job_loc = &job,
                    .stdout_loc = &chout,
                    .stderr_loc = &cherr,
                    .filters = TAPI_JOB_SIMPLE_FILTERS(
                        {
                            .use_stdout = true,
                            .log_level = TE_LL_RING,
                            .readable = false,
                            .filter_name = "stdout",
                        },
                        {
                            .use_stderr = true,
                            .log_level = TE_LL_ERROR,
                            .readable = false,
                            .filter_name = "stderr",
                        }
                    )
                });

    if (rc != 0)
        goto out;

    rc = tapi_job_start(job);
    if (rc != 0)
        goto out;

    tmprc = tapi_job_wait(job, timeout_ms, &status);
    if (TE_RC_GET_ERROR(tmprc) == TE_EINPROGRESS)
    {
        tmprc = tapi_job_kill(job, SIGKILL);
        if (rc == 0)
            rc = tmprc;
        tmprc = tapi_job_wait(job, 0, &status);
        if (rc == 0)
            rc = tmprc;
    }

out:
    tmprc = tapi_job_destroy(job, -1);
    if (rc == 0)
        rc = tmprc;
    tapi_job_factory_destroy(factory);
    te_vec_deep_free(&args);

    return TE_RC_UPSTREAM(TE_TAPI, rc);
}
