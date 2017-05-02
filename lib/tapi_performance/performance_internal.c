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
#include "tapi_test_log.h"


/* See description in performance_internal.h */
void
perf_app_close_descriptors(tapi_perf_app *app)
{
    if (app->rpcs == NULL)
        return;

    if (app->fd_stdout >= 0)
        RPC_CLOSE(app->rpcs, app->fd_stdout);
    if (app->fd_stderr >= 0)
        RPC_CLOSE(app->rpcs, app->fd_stderr);
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
    app->fd_stdout = stdout;
    app->fd_stderr = stderr;
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

    return 0;
}

/* See description in performance_internal.h */
te_errno
perf_app_check_report(tapi_perf_app *app, tapi_perf_report *report,
                      const char *tag)
{
    size_t i;
    te_errno rc = 0;

    for (i = 0; i < TE_ARRAY_LEN(report->errors); i++)
    {
        if (report->errors[i] > 0)
        {
            rc = TE_RC(TE_TAPI, TE_EFAIL);
            ERROR_VERDICT("%s %s error: %s", tapi_perf_bench2str(app->bench),
                          tag, tapi_perf_error2str(i));
        }
    }
    return rc;
}

/* See description in performance_internal.h */
void
perf_app_dump_output(tapi_perf_app *app, const char *tag)
{
    RING("%s %s stdout:\n%s", tapi_perf_bench2str(app->bench),
         tag, app->stdout.ptr);
    RING("%s %s stderr:\n%s", tapi_perf_bench2str(app->bench),
         tag, app->stderr.ptr);
}
