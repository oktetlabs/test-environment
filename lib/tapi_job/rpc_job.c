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

/*
 * Check that RPC status is OK and errno is unchanged,
 * in any other case set @p _var to #TE_ECORRUPTED.
 *
 * The function assumes to have RPC server handle as @b rpcs variable in
 * the context.
 *
 * @param _func         function name
 * @param _var          variable with return value
 */
#define CHECK_RPC_ERRNO_UNCHANGED(_func, _var) \
    CHECK_RETVAL_VAR_ERR_COND(_func, _var, FALSE, TE_RC(TE_TAPI, TE_ECORRUPTED), \
                              FALSE);

static void
rpc_job_set_rpcs_timeout(rcf_rpc_server *rpcs, int timeout_ms,
                         int timeout_for_negative_ms)
{
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        if (timeout_ms < 0)
            rpcs->timeout = timeout_for_negative_ms;
        else
            rpcs->timeout = timeout_ms + TE_SEC2MS(TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }
}

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
        len = 1;
        for (i = 0; argv[i] != NULL; i++)
            len++;

        in.argv.argv_len = len;
        in.argv.argv_val = tapi_calloc(len, sizeof(*in.argv.argv_val));

        for (i = 0; i < len - 1; i++)
        {
            in.argv.argv_val[i].str.str_val = tapi_strdup(argv[i]);
            in.argv.argv_val[i].str.str_len = strlen(argv[i]) + 1;
        }
    }

    if (env != NULL)
    {
        len = 1;
        for (i = 0; env[i] != NULL; i++)
            len++;

        in.env.env_len = len;
        in.env.env_val = tapi_calloc(len, sizeof(*in.env.env_val));

        for (i = 0; i < len - 1; i++)
        {
            in.env.env_val[i].str.str_val = tapi_strdup(env[i]);
            in.env.env_val[i].str.str_len = strlen(env[i]) + 1;
        }
    }

    if (spawner != NULL)
        in.spawner = tapi_strdup(spawner);

    if (tool != NULL)
        in.tool = tapi_strdup(tool);

    rcf_rpc_call(rpcs, "job_create", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_create, out.retval);

    tlbp_argv = te_log_buf_alloc();
    tlbp_env = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_create, "%s, %s, {%s}, {%s}", "%r",
                 in.spawner, in.tool,
                 te_args2log_buf(tlbp_argv, in.argv.argv_len - 1,
                                 (const char **)argv),
                 te_args2log_buf(tlbp_env, in.env.env_len - 1,
                                 (const char **)env),
                 out.retval);
    te_log_buf_free(tlbp_argv);
    te_log_buf_free(tlbp_env);

    if (in.argv.argv_val != NULL)
    {
        for (i = 0; i < in.argv.argv_len; i++)
            free(in.argv.argv_val[i].str.str_val);
        free(in.argv.argv_val);
    }
    if (in.env.env_val != NULL)
    {
        for (i = 0; i < in.env.env_len; i++)
            free(in.env.env_val[i].str.str_val);
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
    CHECK_RPC_ERRNO_UNCHANGED(job_start, out.retval);

    TAPI_RPC_LOG(rpcs, job_start, "%u", "%r", in.job_id, out.retval);

    RETVAL_INT(job_start, out.retval);
}

static const char *
tarpc_uint_array2log_buf(te_log_buf *buf, size_t size, tarpc_uint *array)
{
    size_t i;

    for (i = 0; i < size; i++)
        te_log_buf_append(buf, "%s%u", i == 0 ? "" : ", ", array[i]);

    return te_log_buf_get(buf);
}

int
rpc_job_allocate_channels(rcf_rpc_server *rpcs, unsigned int job_id,
                          te_bool input_channels, unsigned int n_channels,
                          unsigned int *channels)
{
    tarpc_job_allocate_channels_in  in;
    tarpc_job_allocate_channels_out out;
    te_log_buf *tlbp_channels;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;
    in.input_channels = input_channels;
    in.n_channels = n_channels;
    in.channels.channels_val = channels;
    in.channels.channels_len = channels == NULL ? 0 : n_channels;

    rcf_rpc_call(rpcs, "job_allocate_channels", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_allocate_channels, out.retval);

    tlbp_channels = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_allocate_channels, "%u, %s, %u, {%s}", "%r",
                 in.job_id, in.input_channels ? "input" : "output",
                 in.n_channels,
                 tarpc_uint_array2log_buf(tlbp_channels,
                                          out.channels.channels_len,
                                          out.channels.channels_val),
                 out.retval);
    te_log_buf_free(tlbp_channels);

    if (out.retval == 0 && channels != NULL &&
        out.channels.channels_val != NULL)
    {
        unsigned int i;

        for (i = 0; i < out.channels.channels_len; i++)
            channels[i] = out.channels.channels_val[i];
    }

    RETVAL_INT(job_allocate_channels, out.retval);
}

int
rpc_job_attach_filter(rcf_rpc_server *rpcs, const char *filter_name,
                      unsigned int n_channels, unsigned int *channels,
                      te_bool readable, te_log_level log_level,
                      unsigned int *filter)
{
    tarpc_job_attach_filter_in  in;
    tarpc_job_attach_filter_out out;
    te_log_buf *tlbp_channels;
    const char *log_lvl_str;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (filter_name != NULL)
        in.filter_name = tapi_strdup(filter_name);
    in.channels.channels_val = channels;
    in.channels.channels_len = n_channels;
    in.readable = readable;
    in.log_level = log_level;

    rcf_rpc_call(rpcs, "job_attach_filter", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_attach_filter, out.retval);

    log_lvl_str = te_log_level2str(log_level);
    tlbp_channels = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_attach_filter, "\"%s\", %u, {%s}, %s, %s, %u", "%r",
                 in.filter_name, in.channels.channels_len,
                 tarpc_uint_array2log_buf(tlbp_channels,
                                          in.channels.channels_len,
                                          in.channels.channels_val),
                 in.readable ? "readable" : "unreadable",
                 log_lvl_str == NULL ? "<NONE>" : log_lvl_str,
                 out.filter, out.retval);
    te_log_buf_free(tlbp_channels);

    if (out.retval == 0 && filter != NULL)
        *filter = out.filter;

    free(in.filter_name);

    RETVAL_INT(job_attach_filter, out.retval);
}

int
rpc_job_filter_add_regexp(rcf_rpc_server *rpcs, unsigned int filter,
                          const char *re, unsigned int extract)
{
    tarpc_job_filter_add_regexp_in  in;
    tarpc_job_filter_add_regexp_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.filter = filter;
    if (re != NULL)
        in.re = tapi_strdup(re);
    in.extract = extract;

    rcf_rpc_call(rpcs, "job_filter_add_regexp", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_filter_add_regexp, out.retval);

    TAPI_RPC_LOG(rpcs, job_filter_add_regexp, "%u, \"%s\", %u", "%r",
                 in.filter, in.re, in.extract, out.retval);
    free(in.re);

    RETVAL_INT(job_filter_add_regexp, out.retval);
}

static void
tarpc_job_buffer_copy(const tarpc_job_buffer *from, tarpc_job_buffer *to)
{
    to->channel = from->channel;
    to->filter = from->filter;
    to->eos = from->eos;
    to->dropped = from->dropped;
    to->data.data_len = from->data.data_len;

    if (from->data.data_len > 0)
    {
        to->data.data_val = tapi_memdup(from->data.data_val,
                                        from->data.data_len);
    }
    else
    {
        to->data.data_val = NULL;
    }
}

int
rpc_job_receive(rcf_rpc_server *rpcs, unsigned int n_filters,
                unsigned int *filters, int timeout_ms,
                tarpc_job_buffer *buffer)
{
    tarpc_job_receive_in  in;
    tarpc_job_receive_out out;
    te_log_buf *tlbp_filters;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.filters.filters_val = filters;
    in.filters.filters_len = n_filters;
    in.timeout_ms = timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, timeout_ms, TAPI_RPC_JOB_BIG_TIMEOUT_MS);
    rcf_rpc_call(rpcs, "job_receive", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_receive, out.retval);

    tlbp_filters = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_receive, "%u, {%s}, %d", "%r",
                 in.filters.filters_len,
                 tarpc_uint_array2log_buf(tlbp_filters,
                                          in.filters.filters_len,
                                          in.filters.filters_val),
                 in.timeout_ms, out.retval);
    te_log_buf_free(tlbp_filters);

    if (out.retval == 0 && buffer != NULL)
        tarpc_job_buffer_copy(&out.buffer, buffer);

    RETVAL_INT(job_receive, out.retval);
}

te_errno
rpc_job_clear(rcf_rpc_server *rpcs, unsigned int n_filters,
               unsigned int *filters)
{
    tarpc_job_clear_in  in;
    tarpc_job_clear_out out;
    te_log_buf *tlbp_filters;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.filters.filters_val = filters;
    in.filters.filters_len = n_filters;

    rcf_rpc_call(rpcs, "job_clear", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_clear, out.retval);

    tlbp_filters = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_clear, "%u, {%s}", "%r",
                 in.filters.filters_len,
                 tarpc_uint_array2log_buf(tlbp_filters,
                                          in.filters.filters_len,
                                          in.filters.filters_val),
                 out.retval);
    te_log_buf_free(tlbp_filters);

    RETVAL_INT(job_clear, out.retval);
}

int
rpc_job_send(rcf_rpc_server *rpcs, unsigned int channel,
             const void *buf, size_t count)
{
    tarpc_job_send_in  in;
    tarpc_job_send_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.buf.buf_val = (uint8_t *)buf;
    in.buf.buf_len = count;
    in.channel = channel;

    rcf_rpc_call(rpcs, "job_send", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_send, out.retval);

    TAPI_RPC_LOG(rpcs, job_send, "%u, %lu", "%r", in.channel,
                 in.buf.buf_len, out.retval);

    RETVAL_INT(job_send, out.retval);
}

int
rpc_job_poll(rcf_rpc_server *rpcs, unsigned int n_channels,
             unsigned int *channels, int timeout_ms)
{
    tarpc_job_poll_in  in;
    tarpc_job_poll_out out;
    te_log_buf *tlbp_channels;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.channels.channels_val = channels;
    in.channels.channels_len = n_channels;
    in.timeout_ms = timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, timeout_ms, TAPI_RPC_JOB_BIG_TIMEOUT_MS);
    rcf_rpc_call(rpcs, "job_poll", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_poll, out.retval);

    tlbp_channels = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_poll, "%u, {%s}, %d", "%r", in.channels.channels_len,
                 tarpc_uint_array2log_buf(tlbp_channels,
                                          in.channels.channels_len,
                                          in.channels.channels_val),
                 in.timeout_ms, out.retval);
    te_log_buf_free(tlbp_channels);

    RETVAL_INT(job_poll, out.retval);
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
    CHECK_RPC_ERRNO_UNCHANGED(job_kill, out.retval);

    TAPI_RPC_LOG(rpcs, job_kill, "%u, %s", "%r", in.job_id,
                 signum_rpc2str(in.signo), out.retval);

    RETVAL_INT(job_kill, out.retval);
}

int
rpc_job_killpg(rcf_rpc_server *rpcs, unsigned int job_id, rpc_signum signo)
{
    tarpc_job_killpg_in  in;
    tarpc_job_killpg_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;
    in.signo = signo;

    rcf_rpc_call(rpcs, "job_killpg", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_killpg, out.retval);

    TAPI_RPC_LOG(rpcs, job_killpg, "%u, %s", "%r", in.job_id,
                 signum_rpc2str(in.signo), out.retval);

    RETVAL_INT(job_killpg, out.retval);
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

    rpc_job_set_rpcs_timeout(rpcs, timeout_ms, TAPI_RPC_JOB_BIG_TIMEOUT_MS);
    rcf_rpc_call(rpcs, "job_wait", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_wait, out.retval);

    TAPI_RPC_LOG(rpcs, job_wait, "%u, %d", "%r", in.job_id,
                 in.timeout_ms, out.retval);

    if (out.retval == 0 && status != NULL)
        *status = out.status;

    RETVAL_INT(job_wait, out.retval);
}

int
rpc_job_stop(rcf_rpc_server *rpcs, unsigned int job_id, rpc_signum signo,
             int term_timeout_ms)
{
    tarpc_job_stop_in   in;
    tarpc_job_stop_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;
    in.signo = signo;
    in.term_timeout_ms = term_timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, term_timeout_ms, RCF_RPC_UNSPEC_TIMEOUT);
    rcf_rpc_call(rpcs, "job_stop", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_stop, out.retval);

    TAPI_RPC_LOG(rpcs, job_stop, "%u, %s", "%r", in.job_id,
                 signum_rpc2str(signo), out.retval);

    RETVAL_INT(job_stop, out.retval);
}

int
rpc_job_destroy(rcf_rpc_server *rpcs, unsigned int job_id, int term_timeout_ms)
{
    tarpc_job_destroy_in  in;
    tarpc_job_destroy_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = job_id;
    in.term_timeout_ms = term_timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, term_timeout_ms, RCF_RPC_DEFAULT_TIMEOUT);
    rcf_rpc_call(rpcs, "job_destroy", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_destroy, out.retval);

    TAPI_RPC_LOG(rpcs, job_destroy, "%u", "%r", in.job_id, out.retval);

    RETVAL_INT(job_destroy, out.retval);
}

int
rpc_job_wrapper_add(rcf_rpc_server *rpcs, unsigned int job_id,
                    const char *tool, const char **argv,
                    tarpc_job_wrapper_priority priority,
                    unsigned int *wrapper_id)
{
    tarpc_job_wrapper_add_in  in;
    tarpc_job_wrapper_add_out out;
    te_log_buf *tlbp_argv;
    size_t len;
    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (argv != NULL)
    {
        len = 1;
        for (i = 0; argv[i] != NULL; i++)
            len++;

        in.argv.argv_len = len;
        in.argv.argv_val = tapi_calloc(len, sizeof(*in.argv.argv_val));

        for (i = 0; i < len - 1; i++)
        {
            in.argv.argv_val[i].str.str_val = tapi_strdup(argv[i]);
            in.argv.argv_val[i].str.str_len = strlen(argv[i]) + 1;
        }
    }

    if (tool != NULL)
        in.tool = tapi_strdup(tool);

    in.job_id = job_id;
    in.priority = priority;

    rcf_rpc_call(rpcs, "job_wrapper_add", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_wrapper_add, out.retval);

    tlbp_argv = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_wrapper_add, "%s, {%s}", "%r",
                 in.tool,
                 te_args2log_buf(tlbp_argv, in.argv.argv_len - 1,
                                 (const char **)argv),
                 out.retval);
    te_log_buf_free(tlbp_argv);

    if (in.argv.argv_val != NULL)
    {
        for (i = 0; i < in.argv.argv_len; i++)
            free(in.argv.argv_val[i].str.str_val);
        free(in.argv.argv_val);
    }
    free(in.tool);

    if (out.retval == 0)
        *wrapper_id = out.wrapper_id;

    RETVAL_INT(job_wrapper_add, out.retval);
}

int
rpc_job_wrapper_delete(rcf_rpc_server *rpcs, unsigned int job_id,
                       unsigned int wrapper_id)
{
    tarpc_job_wrapper_delete_in  in;
    tarpc_job_wrapper_delete_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.wrapper_id = wrapper_id;
    in.job_id = job_id;

    rcf_rpc_call(rpcs, "job_wrapper_delete", &in, &out);

    TAPI_RPC_LOG(rpcs, job_wrapper_delete, "%u", "%r",
                 in.wrapper_id, out.retval);
    RETVAL_INT(job_wrapper_delete, out.retval);
}
