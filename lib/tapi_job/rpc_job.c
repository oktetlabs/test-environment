/** @file
 * @brief RPC client API for Agent job control
 *
 * RPC client API for Agent job control functions (implementation)
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#include "te_config.h"

#include "tapi_rpc_job.h"
#include "log_bufs.h"
#include "logger_api.h"
#include "tapi_mem.h"
#include "tapi_rpc_internal.h"
#include "tapi_test_log.h"

int
rpc_job_create(rcf_rpc_server *rpcs, const char *spawner,
               const char *tool, const char **argv, const char **env,
               unsigned int *job_id)
{
    tarpc_job_create_in  in;
    tarpc_job_create_out out;
    te_log_buf *tlbp_argv;
    te_log_buf *tlbp_env;
    size_t len;
    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (argv != NULL)
    {
        len = 0;
        for (i = 0; argv[i] != NULL; i++)
            len++;

        in.argv.argv_len = len;
        in.argv.argv_val = tapi_calloc(len, sizeof(*in.argv.argv_val));

        for (i = 0; i < len; i++)
            in.argv.argv_val[i].str = tapi_strdup(argv[i]);
    }

    if (env != NULL)
    {
        len = 0;
        for (i = 0; env[i] != NULL; i++)
            len++;

        in.env.env_len = len;
        in.env.env_val = tapi_calloc(len, sizeof(*in.env.env_val));

        for (i = 0; i < len; i++)
            in.env.env_val[i].str = tapi_strdup(env[i]);
    }

    if (spawner != NULL)
        in.spawner = tapi_strdup(spawner);

    if (tool != NULL)
        in.tool = tapi_strdup(tool);

    rcf_rpc_call(rpcs, "job_create", &in, &out);

    tlbp_argv = te_log_buf_alloc();
    tlbp_env = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_create, "%s, %s, {%s}, {%s}", "%r",
                 in.spawner, in.tool,
                 te_args2log_buf(tlbp_argv, in.argv.argv_len,
                                 (const char **)in.argv.argv_val),
                 te_args2log_buf(tlbp_env, in.env.env_len,
                                 (const char **)in.env.env_val),
                 out.retval);
    te_log_buf_free(tlbp_argv);
    te_log_buf_free(tlbp_env);

    if (in.argv.argv_val != NULL)
    {
        for (i = 0; i < in.argv.argv_len; i++)
            free(in.argv.argv_val[i].str);
        free(in.argv.argv_val);
    }
    if (in.env.env_val != NULL)
    {
        for (i = 0; i < in.env.env_len; i++)
            free(in.env.env_val[i].str);
        free(in.env.env_val);
    }
    free(in.spawner);
    free(in.tool);

    if (out.retval == 0)
        *job_id = out.job_id;

    RETVAL_INT(job_create, out.retval);
}

int
rpc_job_start(rcf_rpc_server *rpcs, unsigned int job_id)
{
    tarpc_job_start_in  in;
    tarpc_job_start_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;

    rcf_rpc_call(rpcs, "job_start", &in, &out);

    TAPI_RPC_LOG(rpcs, job_start, "%u", "%r", in.job_id, out.retval);

    RETVAL_INT(job_start, out.retval);
}

int
rpc_job_kill(rcf_rpc_server *rpcs, unsigned int job_id, rpc_signum signo)
{
    tarpc_job_kill_in  in;
    tarpc_job_kill_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;
    in.signo = signo;

    rcf_rpc_call(rpcs, "job_kill", &in, &out);

    TAPI_RPC_LOG(rpcs, job_kill, "%u, %s", "%r", in.job_id,
                 signum_rpc2str(in.signo), out.retval);

    RETVAL_INT(job_kill, out.retval);
}

int
rpc_job_wait(rcf_rpc_server *rpcs, unsigned int job_id, int timeout_ms,
             tarpc_job_status *status)
{
    tarpc_job_wait_in  in;
    tarpc_job_wait_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;
    in.timeout_ms = timeout_ms;

    rcf_rpc_call(rpcs, "job_wait", &in, &out);

    TAPI_RPC_LOG(rpcs, job_wait, "%u, %d", "%r", in.job_id,
                 in.timeout_ms, out.retval);

    if (out.retval == 0 && status != NULL)
        *status = out.status;

    RETVAL_INT(job_wait, out.retval);
}

int
rpc_job_destroy(rcf_rpc_server *rpcs, unsigned int job_id)
{
    tarpc_job_destroy_in  in;
    tarpc_job_destroy_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;

    rcf_rpc_call(rpcs, "job_destroy", &in, &out);

    TAPI_RPC_LOG(rpcs, job_destroy, "%u", "%r", in.job_id, out.retval);

    RETVAL_INT(job_destroy, out.retval);
}
