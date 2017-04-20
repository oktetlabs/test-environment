/** @file
 * @brief Auxiliary functions to performance TAPI
 *
 * Auxiliary functions for internal use in performance TAPI
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#include "performance_internal.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpcsock_macros.h"


/* See description in performance_internal.h */
void
perf_app_close_descriptors(tapi_perf_app *app)
{
    if (app->rpcs == NULL)
        return;

    if (app->stdout >= 0)
        RPC_CLOSE(app->rpcs, app->stdout);
    if (app->stderr >= 0)
        RPC_CLOSE(app->rpcs, app->stderr);
}

/* See description in performance_internal.h */
te_errno
perf_app_start(rcf_rpc_server *rpcs, char *cmd, tapi_perf_app *app)
{
    tarpc_pid_t pid;
    int stdout = -1;
    int stderr = -1;

    RING("Run \"%s\"", cmd);
    pid = rpc_te_shell_cmd(rpcs, cmd, -1, NULL, &stdout, &stderr);
    if (pid < 0)
    {
        ERROR("Failed to start perf tool");
        free(cmd);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    perf_app_close_descriptors(app);
    app->rpcs = rpcs;
    app->pid = pid;
    app->stdout = stdout;
    app->stderr = stderr;
    free(app->cmd);
    app->cmd = cmd;

    return 0;
}

/* See description in performance_internal.h */
te_errno
perf_app_stop(tapi_perf_app *app)
{
    if (app->pid >= 0)
    {
        rpc_ta_kill_death(app->rpcs, app->pid);
        app->pid = -1;
    }

    return 0;   /* Just to use it similarly to perf_app_start function */
}

/* See description in performance_internal.h */
te_errno
perf_app_wait(tapi_perf_app *app, uint16_t timeout)
{
    rpc_wait_status stat;
    tarpc_pid_t pid;

    app->rpcs->timeout = TE_SEC2MS(timeout);
    RPC_AWAIT_IUT_ERROR(app->rpcs);
    pid = rpc_waitpid(app->rpcs, app->pid, &stat, 0);
    if (pid != app->pid)
    {
        ERROR("waitpid() failed with errno %r", RPC_ERRNO(app->rpcs));
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    app->pid = -1;

    /* Check for errors */
    if (stat.value != 0 || stat.flag != RPC_WAIT_STATUS_EXITED)
        return TE_RC(TE_TAPI, TE_ESHCMD);

    return 0;
}
