/** @file
 * @brief Auxiliary functions to fio TAPI
 *
 * Auxiliary functions for internal use in fio TAPI
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAPI FIO"

#include <math.h>
#include "te_string.h"
#include "rcf_rpc.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_fio.h"

static inline int16_t
get_default_timeout(const tapi_fio_opts *opts)
{
    const int16_t error = 30;
    const int16_t five_minutes = 5 * 60;
    const double numjobs_coef = ((double)opts->numjobs.value) /
                                TAPI_FIO_MAX_NUMJOBS;

    return opts->runtime_sec + round(five_minutes * numjobs_coef) + error;
}

static void
fio_app_close_descriptors(tapi_fio_app *app)
{
    if (app->rpcs == NULL)
        return;

    if (app->fd_stdout >= 0)
        RPC_CLOSE(app->rpcs, app->fd_stdout);
    if (app->fd_stderr >= 0)
        RPC_CLOSE(app->rpcs, app->fd_stderr);
}

/* See description in tapi_internal.h */
te_errno
fio_app_start(char *cmd, tapi_fio_app *app)
{
    tarpc_pid_t pid;
    int fd_stdout = -1;
    int fd_stderr = -1;

    RING("Run \"%s\"", cmd);
    pid = rpc_te_shell_cmd(app->rpcs, cmd, -1, NULL,
                           &fd_stdout, &fd_stderr);

    fio_app_close_descriptors(app);
    app->pid = pid;
    app->fd_stdout = fd_stdout;
    app->fd_stderr = fd_stderr;
    free(app->cmd);
    app->cmd = cmd;

    rpc_fcntl(app->rpcs,
              app->fd_stdout, RPC_F_SETPIPE_SZ,
              TAPI_FIO_MAX_REPORT);

    return 0;
}

/* See description in tapi_internal.h */
te_errno
fio_app_stop(tapi_fio_app *app)
{
    if (app->pid >= 0)
    {
        rpc_ta_kill_death(app->rpcs, app->pid);
        app->pid = -1;
    }

    return 0;
}

/* See description in tapi_internal.h */
te_errno
fio_app_wait(tapi_fio_app *app, int16_t timeout_sec)
{
    rpc_wait_status stat;
    tarpc_pid_t pid;

    if (timeout_sec == TAPI_FIO_TIMEOUT_DEFAULT)
        timeout_sec = get_default_timeout(&app->opts);
    RING("Waiting fio %d second(s)...", timeout_sec);
    app->rpcs->timeout = TE_SEC2MS(timeout_sec);
    RPC_AWAIT_IUT_ERROR(app->rpcs);
    pid = rpc_waitpid(app->rpcs, app->pid, &stat, 0);
    if (pid != app->pid)
    {
        ERROR("waitpid() failed with errno %r", RPC_ERRNO(app->rpcs));
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    app->pid = -1;

    return 0;
}
