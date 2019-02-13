/** @file
 * @brief ONVMe target
 *
 * API for control ONVMe target of NVMe Over Fabrics
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#define TE_LGR_USER "NVME ONVMe Target"

#include "te_defs.h"
#include "te_alloc.h"
#include "te_log_stack.h"
#include "te_sockaddr.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_misc.h"
#include "tapi_test.h"

#include "tapi_nvme_onvme_target.h"

typedef struct onvme_proc {
    tarpc_pid_t pid;
    int stdout_fd;
    int stderr_fd;
    tapi_nvme_onvme_target_opts opts;
} onvme_proc;

#define ONVME_PROC_INIT (onvme_proc) {              \
    .pid = -1,                                      \
    .stdout_fd = -1,                                \
    .stderr_fd = -1,                                \
    .opts = TAPI_NVME_ONVME_TARGET_OPTS_DEFAULTS,   \
}
#define ONVME_PROC_SIGINT_TIMEOUT     (5)
#define ONVME_PROC_INIT_TIMEOUT       (15)

/* See description in tapi_nvme_onvme_target.h */
te_errno
tapi_nvme_onvme_target_init(struct tapi_nvme_target *target, void *opts)
{
    onvme_proc *proc;

    if ((proc = TE_ALLOC(sizeof(*proc))) == NULL)
        return TE_ENOMEM;

    *proc = ONVME_PROC_INIT;
    if (opts != NULL)
    {
        tapi_nvme_onvme_target_opts *onvme_opts = opts;
        proc->opts = *onvme_opts;
    }
    target->impl = (void*)proc;
    return 0;
}


static void
onvme_output_print(tapi_nvme_target *target)
{
    te_string out = TE_STRING_INIT;
    te_string err = TE_STRING_INIT;
    onvme_proc *proc = (onvme_proc*)target->impl;
    int out_fd = proc->stdout_fd;
    int err_fd = proc->stderr_fd;

    if (proc->pid == -1)
        return;

    if (out_fd != 0)
        rpc_read_fd2te_string(target->rpcs, out_fd, TE_SEC2MS(1), 0, &out);
    if (err_fd != 0)
        rpc_read_fd2te_string(target->rpcs, err_fd, TE_SEC2MS(1), 0, &err);

    RING("stdout:\n%s\n"
         "stdout:\n%s\n", out.ptr, err.ptr);

    te_string_free(&out);
    te_string_free(&err);
}

static te_errno
onvme_build_cmd(te_string *cmd, tapi_nvme_target *target)
{
    onvme_proc *proc = (onvme_proc*)target->impl;

    CHECK_RC(te_string_append(cmd, "onvme-target-start "));

    CHECK_RC(te_string_append(cmd, "--port %hu ",
                              ntohs(te_sockaddr_get_port(target->addr))));

    if (proc->opts.cores.ptr != NULL)
    {
        CHECK_RC(te_string_append(cmd, "--cores %s ",
                                  proc->opts.cores.ptr));
    }

    if (proc->opts.is_nullblock)
        CHECK_RC(te_string_append(cmd, "--use-null --no-nvme "));

    return 0;
}

/* See description in tapi_nvme_onvme_target.h */
te_errno
tapi_nvme_onvme_target_setup(tapi_nvme_target *target)
{
    te_errno rc;
    tarpc_pid_t pid;
    te_string command = TE_STRING_INIT;
    onvme_proc *proc = (onvme_proc *)target->impl;
    int stdout_fd, stderr_fd;

    if (proc == NULL)
        return TE_EINVAL;

    te_log_stack_push("ONVMe target setup start");

    if ((rc = onvme_build_cmd(&command, target)) != 0)
        return rc;

    RING("Run %s", command.ptr);
    pid = rpc_te_shell_cmd(target->rpcs, command.ptr, -1,
                           NULL, &stdout_fd, &stderr_fd);

    te_string_free(&command);

    if (pid == -1) {
        return TE_ENOTCONN;
    }

    te_motivated_sleep(ONVME_PROC_INIT_TIMEOUT, "Give target a while to start");

    proc->pid = pid;
    proc->stdout_fd = stdout_fd;
    proc->stderr_fd = stderr_fd;

    onvme_output_print(target);

    return 0;
}

typedef enum bind_nvme_type {
    BIND_NVME_KERNEL,
    BIND_NVME_USER,
} bind_nvme_type;

static te_errno
bind_nvme(tapi_nvme_target *target, bind_nvme_type btype)
{
    tarpc_pid_t pid;
    static const char *btype_str[] = {
        [BIND_NVME_KERNEL] = "kernel",
        [BIND_NVME_USER] = "user",
    };

    assert(btype >= 0 && btype < TE_ARRAY_LEN(btype_str));

    pid = rpc_te_shell_cmd(target->rpcs, "bind-nvme.sh %s", -1,
                           NULL, NULL, NULL, btype_str[btype]);

    return pid == -1 ? TE_EFAIL : 0;
}

/* See description in tapi_nvme_onvme_target.h */
void
tapi_nvme_onvme_target_cleanup(tapi_nvme_target *target)
{
    int rc;
    onvme_proc *proc = (onvme_proc *)target->impl;
    tapi_nvme_onvme_target_opts old_opts;

    if (proc == NULL || proc->pid == -1)
        return;

    te_log_stack_push("ONVMe target cleanup start");

    RPC_AWAIT_IUT_ERROR(target->rpcs);
    rc = rpc_ta_kill_and_wait(target->rpcs, proc->pid, RPC_SIGINT,
                              ONVME_PROC_SIGINT_TIMEOUT);

    /* timeout of rpc_ta_kill_and_wait */
    if (rc == -2)
    {
        RPC_AWAIT_IUT_ERROR(target->rpcs);
        rc = rpc_ta_kill_death(target->rpcs, proc->pid);
    }

    onvme_output_print(target);

    if (proc->stderr_fd >= 0)
        rpc_close(target->rpcs, proc->stderr_fd);
    if (proc->stdout_fd >= 0)
        rpc_close(target->rpcs, proc->stdout_fd);

    if (rc == -1)
    {
        ERROR("Cannot kill proccess with pid=%d on %s, rc=%r", proc->pid,
              target->rpcs->ta, RPC_ERRNO(target->rpcs));
    }

    (void)bind_nvme(target, BIND_NVME_KERNEL);

    old_opts = proc->opts;
    *proc = ONVME_PROC_INIT;
    proc->opts = old_opts;
}

/* See description in tapi_nvme_onvme_target.h */
void
tapi_nvme_onvme_target_fini(tapi_nvme_target *target)
{
    onvme_proc *proc = (onvme_proc *)target->impl;

    if (proc == NULL)
        return;

    te_string_free(&proc->opts.cores);
    free(proc);

    target->impl = NULL;
}

