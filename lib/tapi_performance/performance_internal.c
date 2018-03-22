/** @file
 * @brief Auxiliary functions to performance TAPI
 *
 * Auxiliary functions for internal use in performance TAPI
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */
#include "te_config.h"
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "performance_internal.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test_log.h"
#include "tapi_test.h"
#include "te_units.h"


/*
 * Get default timeout according to application options.
 *
 * @param opts          Application options.
 *
 * @return Timeout (seconds).
 */
static int16_t
get_default_timeout(const tapi_perf_opts *opts)
{
    int64_t timeout_sec;

    if (opts->num_bytes > 0)
        timeout_sec = opts->num_bytes * 8 / 10000000;   /* Suppose 10Mbit/s */
    else
        timeout_sec = opts->duration_sec;

    timeout_sec += 10;  /* Just in case of some delay */

    if (timeout_sec > SHRT_MAX)
        timeout_sec = SHRT_MAX;

    return timeout_sec;
}


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
perf_app_wait(tapi_perf_app *app, int16_t timeout)
{
    rpc_wait_status stat;
    tarpc_pid_t pid;

    if (timeout == TAPI_PERF_TIMEOUT_DEFAULT)
        timeout = get_default_timeout(&app->opts);
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

/* See description in performance_internal.h */
char *
perf_get_tool_tuple(const tapi_perf_server *server,
                    const tapi_perf_client *client,
                    const tapi_perf_report *report)
{
    te_unit bandwidth;
    te_unit throughput;
    te_string buf = TE_STRING_INIT;
    const tapi_perf_opts *opts = &client->app.opts;

    bandwidth = te_unit_pack(opts->bandwidth_bits);
    throughput = te_unit_pack(report->bits_per_second);

    CHECK_RC(te_string_append(&buf, "ip=%s, ", proto_rpc2str(opts->ipversion)));
    CHECK_RC(te_string_append(&buf, "protocol=%s, ",
                              proto_rpc2str(opts->protocol)));
    CHECK_RC(te_string_append(&buf, "bandwidth=%.1f%sbits/sec, ",
                              bandwidth.value, te_unit_prefix2str(bandwidth.unit)));
    CHECK_RC(te_string_append(&buf, "num_bytes=%"PRId64", ", opts->num_bytes));
    CHECK_RC(te_string_append(&buf, "duration=%"PRId32"sec, ", opts->duration_sec));
    CHECK_RC(te_string_append(&buf, "length=%"PRId32"bytes, ", opts->length));
    CHECK_RC(te_string_append(&buf, "num_streams=%"PRId16", ", opts->streams));
    CHECK_RC(te_string_append(&buf, "res_num_bytes=%"PRIu64", ", report->bytes));
    CHECK_RC(te_string_append(&buf, "res_time=%.1fsec, ", report->seconds));
    CHECK_RC(te_string_append(&buf, "res_throughput=%.1f%sbits/sec, ",
                              throughput.value, te_unit_prefix2str(throughput.unit)));
    CHECK_RC(te_string_append(&buf, "server_cmd=\"%s\", ", server->app.cmd));
    CHECK_RC(te_string_append(&buf, "client_cmd=\"%s\"", client->app.cmd));

    return buf.ptr;
}
