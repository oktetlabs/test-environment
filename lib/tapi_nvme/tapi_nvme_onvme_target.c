/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief ONVMe target
 *
 * API for control ONVMe target of NVMe Over Fabrics
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "NVME ONVMe Target"

#include "te_defs.h"
#include "te_alloc.h"
#include "te_log_stack.h"
#include "te_sockaddr.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_misc.h"
#include "tapi_test.h"
#include "te_vector.h"

#include "tapi_nvme_onvme_target.h"
#include "tapi_job_factory_rpc.h"

#define ONVME_PROC_SIGINT_TIMEOUT     (5)
#define ONVME_PROC_INIT_TIMEOUT       (15)
#define ONVME_PROC_FINI_TIMEOUT       (2)

/* See description in tapi_nvme_onvme_target.h */
te_errno
tapi_nvme_onvme_target_init(struct tapi_nvme_target *target, void *opts)
{
    tapi_nvme_onvme_target_proc *proc;

    if ((proc = TE_ALLOC(sizeof(*proc))) == NULL)
        return TE_ENOMEM;

    *proc = TAPI_NVME_ONVME_TARGET_PROC_DEFAULTS;
    if (opts != NULL)
    {
        tapi_nvme_onvme_target_opts *onvme_opts = opts;
        proc->opts = *onvme_opts;
    }
    target->impl = (void*)proc;
    return 0;
}

static void
onvme_args_free(te_vec *onvme_args)
{
    char **arg;

    TE_VEC_FOREACH(onvme_args, arg)
        free(*arg);
    te_vec_free(onvme_args);
}

static te_errno
onvme_build_args(te_vec *args, tapi_nvme_target *target)
{
    te_errno rc;
    te_vec result = TE_VEC_INIT(const char *);
    tapi_nvme_onvme_target_proc *proc =
        (tapi_nvme_onvme_target_proc*)target->impl;
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
            onvme_args_free(&result);                                   \
            return rc;                                                  \
        }                                                               \
    } while(0)

    ADD_ARG("onvme-target-start");
    ADD_ARG("--use-null");
    ADD_ARG("--port");
    ADD_ARG("%hu", ntohs(te_sockaddr_get_port(target->addr)));
    if (proc->opts.cores.ptr != NULL)
    {
        ADD_ARG("--cores");
        ADD_ARG("%s", proc->opts.cores.ptr);
    }
    if (proc->opts.is_nullblock)
        ADD_ARG("--no-nvme");
    if (proc->opts.max_worker_conn != 0)
    {
        ADD_ARG("--max-worker-connections");
        ADD_ARG("%d", proc->opts.max_worker_conn);
    }
    if (proc->opts.log_level >= 0 && proc->opts.log_level <= 4)
    {
        ADD_ARG("--log-level");
        ADD_ARG("%d", proc->opts.log_level);
    }

#undef ADD_ARG

    if ((rc = TE_VEC_APPEND(&result, end_arg)) != 0)
    {
        onvme_args_free(&result);
        return rc;
    }

    *args = result;
    return 0;
}

/* See description in tapi_nvme_onvme_target.h */
te_errno
tapi_nvme_onvme_target_setup(tapi_nvme_target *target)
{
    te_errno rc;
    te_vec onvme_args;
    tapi_job_factory_t *factory = NULL;
    tapi_nvme_onvme_target_proc *proc =
        (tapi_nvme_onvme_target_proc *)target->impl;

    if (proc == NULL)
        return TE_EINVAL;

    te_log_stack_push("ONVMe target setup start");

    if ((rc = onvme_build_args(&onvme_args, target)) != 0)
        return rc;

    rc = tapi_job_factory_rpc_create(target->rpcs, &factory);
    if (rc != 0)
        return rc;

    rc = tapi_job_create(factory, NULL, "onvme-target-start",
                         (const char **)onvme_args.data.ptr,
                         NULL, &proc->onvme_job);

    onvme_args_free(&onvme_args);
    tapi_job_factory_destroy(factory);

    if (rc != 0)
        return rc;

    rc = tapi_job_alloc_output_channels(proc->onvme_job, 2, proc->out_chs);
    if (rc != 0)
        return rc;

    rc = tapi_job_attach_filter(
        TAPI_JOB_CHANNEL_SET(proc->out_chs[0], proc->out_chs[1]),
        "ONVMe", FALSE, TE_LL_WARN, NULL);
    if (rc != 0)
        return rc;

    if ((rc = tapi_job_start(proc->onvme_job)) != 0)
        return rc;

    te_motivated_sleep(ONVME_PROC_INIT_TIMEOUT, "Give target a while to start");

    return 0;
}

/* See description in tapi_nvme_onvme_target.h */
void
tapi_nvme_onvme_target_cleanup(tapi_nvme_target *target)
{
    tapi_nvme_onvme_target_proc *proc =
        (tapi_nvme_onvme_target_proc *)target->impl;
    te_errno rc;

    if (proc == NULL)
        return;

    if ((rc = tapi_job_killpg(proc->onvme_job, SIGINT)) != 0)
        ERROR("Cannot killpg for ONVMe process, rc=%r", rc);

    te_motivated_sleep(ONVME_PROC_SIGINT_TIMEOUT, "Waiting for target stop");

    if ((rc = tapi_job_destroy(proc->onvme_job, ONVME_PROC_FINI_TIMEOUT)) != 0)
        ERROR("Cannot tapi_job_destroy for ONVMe process, rc=%r", rc);
}

/* See description in tapi_nvme_onvme_target.h */
void
tapi_nvme_onvme_target_fini(tapi_nvme_target *target)
{
    tapi_nvme_onvme_target_proc *proc =
        (tapi_nvme_onvme_target_proc *)target->impl;

    if (proc == NULL)
        return;

    te_string_free(&proc->opts.cores);
    free(proc);

    target->impl = NULL;
}
