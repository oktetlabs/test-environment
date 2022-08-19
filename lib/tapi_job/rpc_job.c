/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC client API for Agent job control
 *
 * RPC client API for Agent job control functions (implementation)
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "tapi_job_internal.h"
#include "tapi_job_methods.h"
#include "tapi_rpc_job.h"
#include "log_bufs.h"
#include "logger_api.h"
#include "tapi_mem.h"
#include "te_alloc.h"
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

const tapi_job_methods_t rpc_job_methods = {
    .create = rpc_job_create,
    .start = rpc_job_start,
    .allocate_channels = rpc_job_allocate_channels,
    .kill = rpc_job_kill,
    .killpg = rpc_job_killpg,
    .wait = rpc_job_wait,
    .stop = rpc_job_stop,
    .destroy = rpc_job_destroy,
    .wrapper_add = rpc_job_wrapper_add,
    .wrapper_delete = rpc_job_wrapper_delete,
    .add_sched_param = rpc_job_add_sched_param,
};

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

te_errno
rpc_job_create(tapi_job_t *job, const char *spawner, const char *tool,
               const char **argv, const char **env)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

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
    TAPI_RPC_LOG(rpcs, job_create, "%s, %s, {%s}, {%s}", "%r job_id=%u",
                 in.spawner, in.tool,
                 te_args2log_buf(tlbp_argv, in.argv.argv_len - 1,
                                 (const char **)argv),
                 te_args2log_buf(tlbp_env, in.env.env_len - 1,
                                 (const char **)env),
                 out.retval, out.job_id);
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
        tapi_job_set_id(job, out.job_id);

    RETVAL_TE_ERRNO(job_create, out.retval);
}

te_errno
rpc_job_start(const tapi_job_t *job)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_start_in  in;
    tarpc_job_start_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);

    rcf_rpc_call(rpcs, "job_start", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_start, out.retval);

    TAPI_RPC_LOG(rpcs, job_start, "%u", "%r", in.job_id, out.retval);

    RETVAL_TE_ERRNO(job_start, out.retval);
}

static const char *
tarpc_uint_array2log_buf(te_log_buf *buf, size_t size, tarpc_uint *array)
{
    size_t i;

    for (i = 0; i < size; i++)
        te_log_buf_append(buf, "%s%u", i == 0 ? "" : ", ", array[i]);

    return te_log_buf_get(buf);
}

te_errno
rpc_job_allocate_channels(const tapi_job_t *job, te_bool input_channels,
                          unsigned int n_channels, unsigned int *channels)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_allocate_channels_in  in;
    tarpc_job_allocate_channels_out out;
    te_log_buf *tlbp_channels;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);
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

    RETVAL_TE_ERRNO(job_allocate_channels, out.retval);
}

te_errno
rpc_job_deallocate_channels(rcf_rpc_server *rpcs, unsigned int n_channels,
                            unsigned int *channels)
{
    tarpc_job_deallocate_channels_in  in;
    tarpc_job_deallocate_channels_out out;
    te_log_buf *tlbp_channels;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.channels.channels_val = channels;
    in.channels.channels_len = n_channels;

    rcf_rpc_call(rpcs, "job_deallocate_channels", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_deallocate_channels, out.retval);

    tlbp_channels = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_deallocate_channels, "%u, {%s}", "%r",
                 in.channels.channels_len,
                 tarpc_uint_array2log_buf(tlbp_channels,
                                          in.channels.channels_len,
                                          in.channels.channels_val),
                 out.retval);
    te_log_buf_free(tlbp_channels);

    RETVAL_TE_ERRNO(job_deallocate_channels, out.retval);
}

te_errno
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

    RETVAL_TE_ERRNO(job_attach_filter, out.retval);
}

te_errno
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

    RETVAL_TE_ERRNO(job_filter_add_regexp, out.retval);
}

te_errno
rpc_job_filter_add_channels(rcf_rpc_server *rpcs, unsigned int filter,
                            unsigned int n_channels, unsigned int *channels)
{
    tarpc_job_filter_add_channels_in  in;
    tarpc_job_filter_add_channels_out out;
    te_log_buf *tlbp_channels;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.filter = filter;
    in.channels.channels_val = channels;
    in.channels.channels_len = n_channels;

    rcf_rpc_call(rpcs, "job_filter_add_channels", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_filter_add_channels, out.retval);

    tlbp_channels = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_filter_add_channels, "%u, {%s}", "%r",
                 in.filter, tarpc_uint_array2log_buf(tlbp_channels,
                                                     in.channels.channels_len,
                                                     in.channels.channels_val),
                 out.retval);
    te_log_buf_free(tlbp_channels);

    RETVAL_TE_ERRNO(job_filter_add_channels, out.retval);
}

te_errno
rpc_job_filter_remove_channels(rcf_rpc_server *rpcs, unsigned int filter,
                               unsigned int n_channels, unsigned int *channels)
{
    tarpc_job_filter_remove_channels_in  in;
    tarpc_job_filter_remove_channels_out out;
    te_log_buf *tlbp_channels;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.filter = filter;
    in.channels.channels_val = channels;
    in.channels.channels_len = n_channels;

    rcf_rpc_call(rpcs, "job_filter_remove_channels", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_filter_remove_channels, out.retval);

    tlbp_channels = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_filter_remove_channels, "%u, {%s}", "%r",
                 in.filter, tarpc_uint_array2log_buf(tlbp_channels,
                                                     in.channels.channels_len,
                                                     in.channels.channels_val),
                 out.retval);
    te_log_buf_free(tlbp_channels);

    RETVAL_TE_ERRNO(job_filter_remove_channels, out.retval);
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

te_errno
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
    TAPI_RPC_LOG(rpcs, job_receive, "%u, {%s}, %d ms", "%r",
                 in.filters.filters_len,
                 tarpc_uint_array2log_buf(tlbp_filters,
                                          in.filters.filters_len,
                                          in.filters.filters_val),
                 in.timeout_ms, out.retval);
    te_log_buf_free(tlbp_filters);

    if (out.retval == 0 && buffer != NULL)
        tarpc_job_buffer_copy(&out.buffer, buffer);

    RETVAL_TE_ERRNO(job_receive, out.retval);
}

te_errno
rpc_job_receive_last(rcf_rpc_server *rpcs, unsigned int n_filters,
                     unsigned int *filters, int timeout_ms,
                     tarpc_job_buffer *buffer)
{
    tarpc_job_receive_last_in  in;
    tarpc_job_receive_last_out out;
    te_log_buf *tlbp_filters;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.filters.filters_val = filters;
    in.filters.filters_len = n_filters;
    in.timeout_ms = timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, timeout_ms, TAPI_RPC_JOB_BIG_TIMEOUT_MS);
    rcf_rpc_call(rpcs, "job_receive_last", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_receive_last, out.retval);

    tlbp_filters = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_receive_last, "%u, {%s}, %d ms", "%r",
                 in.filters.filters_len,
                 tarpc_uint_array2log_buf(tlbp_filters,
                                          in.filters.filters_len,
                                          in.filters.filters_val),
                 in.timeout_ms, out.retval);
    te_log_buf_free(tlbp_filters);

    if (out.retval == 0 && buffer != NULL)
        tarpc_job_buffer_copy(&out.buffer, buffer);

    RETVAL_TE_ERRNO(job_receive_last, out.retval);
}

te_errno
rpc_job_receive_many(rcf_rpc_server *rpcs, unsigned int n_filters,
                     unsigned int *filters, int timeout_ms,
                     tarpc_job_buffer **buffers, unsigned int *count)
{
    tarpc_job_receive_many_in in;
    tarpc_job_receive_many_out out;
    te_log_buf *tlbp_filters;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.filters.filters_val = filters;
    in.filters.filters_len = n_filters;
    in.timeout_ms = timeout_ms;
    in.count = *count;

    rpc_job_set_rpcs_timeout(rpcs, timeout_ms, TAPI_RPC_JOB_BIG_TIMEOUT_MS);
    rcf_rpc_call(rpcs, "job_receive_many", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_receive_many, out.retval);

    tlbp_filters = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_receive_many, "%u, {%s}, %d ms, %d", "%r count=%d",
                 in.filters.filters_len,
                 tarpc_uint_array2log_buf(tlbp_filters,
                                          in.filters.filters_len,
                                          in.filters.filters_val),
                 in.timeout_ms, in.count, out.retval,
                 out.buffers.buffers_len);
    te_log_buf_free(tlbp_filters);

    *buffers = out.buffers.buffers_val;
    *count = out.buffers.buffers_len;
    out.buffers.buffers_val = NULL;
    out.buffers.buffers_len = 0;

    RETVAL_TE_ERRNO(job_receive_many, out.retval);
}

void
tarpc_job_buffers_free(tarpc_job_buffer *buffers, unsigned int count)
{
    unsigned int i;

    if (buffers == NULL)
        return;

    for (i = 0; i < count; i++)
        free(buffers[i].data.data_val);

    free(buffers);
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

    RETVAL_TE_ERRNO(job_clear, out.retval);
}

te_errno
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

    RETVAL_TE_ERRNO(job_send, out.retval);
}

te_errno
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
    TAPI_RPC_LOG(rpcs, job_poll, "%u, {%s}, %d ms", "%r",
                 in.channels.channels_len,
                 tarpc_uint_array2log_buf(tlbp_channels,
                                          in.channels.channels_len,
                                          in.channels.channels_val),
                 in.timeout_ms, out.retval);
    te_log_buf_free(tlbp_channels);

    RETVAL_TE_ERRNO(job_poll, out.retval);
}

te_errno
rpc_job_kill(const tapi_job_t *job, int signo)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_kill_in  in;
    tarpc_job_kill_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);
    in.signo = signum_h2rpc(signo);

    rcf_rpc_call(rpcs, "job_kill", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_kill, out.retval);

    TAPI_RPC_LOG(rpcs, job_kill, "%u, %s", "%r", in.job_id,
                 signum_rpc2str(in.signo), out.retval);

    RETVAL_TE_ERRNO(job_kill, out.retval);
}

te_errno
rpc_job_killpg(const tapi_job_t *job, int signo)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_killpg_in  in;
    tarpc_job_killpg_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);
    in.signo = signum_h2rpc(signo);

    rcf_rpc_call(rpcs, "job_killpg", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_killpg, out.retval);

    TAPI_RPC_LOG(rpcs, job_killpg, "%u, %s", "%r", in.job_id,
                 signum_rpc2str(in.signo), out.retval);

    RETVAL_TE_ERRNO(job_killpg, out.retval);
}

static const char *
tarpc_job_status_type2str(tarpc_job_status_type type)
{
    switch (type)
    {
        case TARPC_JOB_STATUS_EXITED:
            return "exited";
        case TARPC_JOB_STATUS_SIGNALED:
            return "signaled";
        case TARPC_JOB_STATUS_UNKNOWN:
            return "unknown";
        default:
            return "INVALID";
    }
}

static const char *
tarpc_job_status2str(te_log_buf *tlbp, const tarpc_job_status *status)
{
    te_log_buf_append(tlbp, "%s(%d)",
                      tarpc_job_status_type2str(status->type), status->value);
    return te_log_buf_get(tlbp);
}

static te_errno
tarpc_job_status2tapi_job_status(const tarpc_job_status *from,
                                 tapi_job_status_t *to)
{
    to->value = from->value;
    switch (from->type)
    {
        case TARPC_JOB_STATUS_EXITED:
            to->type = TAPI_JOB_STATUS_EXITED;
            break;
        case TARPC_JOB_STATUS_SIGNALED:
            to->type = TAPI_JOB_STATUS_SIGNALED;
            break;
        case TARPC_JOB_STATUS_UNKNOWN:
            to->type = TAPI_JOB_STATUS_UNKNOWN;
            break;
        default:
            ERROR("Invalid TA RPC job status");
            return TE_EINVAL;
    }

    return 0;
}

te_errno
rpc_job_wait(const tapi_job_t *job, int timeout_ms, tapi_job_status_t *status)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_wait_in   in;
    tarpc_job_wait_out  out;
    te_log_buf         *tlbp = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);
    in.timeout_ms = timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, timeout_ms, TAPI_RPC_JOB_BIG_TIMEOUT_MS);
    rcf_rpc_call(rpcs, "job_wait", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_wait, out.retval);

    if (out.retval == 0)
        tlbp = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, job_wait, "%u, %d ms", "%r status=%s", in.job_id,
                 in.timeout_ms, out.retval,
                 out.retval == 0 ?
                     tarpc_job_status2str(tlbp, &out.status) : "N/A");

    te_log_buf_free(tlbp);

    if (out.retval == 0 && status != NULL)
    {
        te_errno rc = tarpc_job_status2tapi_job_status(&out.status, status);

        if (rc != 0)
            return rc;
    }

    RETVAL_TE_ERRNO(job_wait, out.retval);
}

te_errno
rpc_job_stop(const tapi_job_t *job, int signo, int term_timeout_ms)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_stop_in   in;
    tarpc_job_stop_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);
    in.signo = signum_h2rpc(signo);
    in.term_timeout_ms = term_timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, term_timeout_ms, RCF_RPC_UNSPEC_TIMEOUT);
    rcf_rpc_call(rpcs, "job_stop", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_stop, out.retval);

    TAPI_RPC_LOG(rpcs, job_stop, "%u, %s, %d ms", "%r", in.job_id,
                 signum_rpc2str(in.signo), in.term_timeout_ms, out.retval);

    RETVAL_TE_ERRNO(job_stop, out.retval);
}

te_errno
rpc_job_destroy(const tapi_job_t *job, int term_timeout_ms)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_destroy_in  in;
    tarpc_job_destroy_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);
    in.term_timeout_ms = term_timeout_ms;

    rpc_job_set_rpcs_timeout(rpcs, term_timeout_ms, RCF_RPC_DEFAULT_TIMEOUT);
    rcf_rpc_call(rpcs, "job_destroy", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_destroy, out.retval);

    TAPI_RPC_LOG(rpcs, job_destroy, "%u, %d ms", "%r",
                 in.job_id, in.term_timeout_ms, out.retval);

    RETVAL_TE_ERRNO(job_destroy, out.retval);
}

static te_errno
tapi_job_wrapper_priority2tarpc_job_wrapper_priority(
                                const tapi_job_wrapper_priority_t *from,
                                tarpc_job_wrapper_priority *to)
{
    switch(*from)
    {
        case TAPI_JOB_WRAPPER_PRIORITY_LOW:
            *to = TARPC_JOB_WRAPPER_PRIORITY_LOW;
            break;
        case TAPI_JOB_WRAPPER_PRIORITY_DEFAULT:
            *to = TARPC_JOB_WRAPPER_PRIORITY_DEFAULT;
            break;
        case TAPI_JOB_WRAPPER_PRIORITY_HIGH:
            *to = TARPC_JOB_WRAPPER_PRIORITY_HIGH;
            break;
        default:
            ERROR("Priority value is not supported");
            return TE_EFAIL;
    }

    return 0;
}

te_errno
rpc_job_wrapper_add(const tapi_job_t *job, const char *tool, const char **argv,
                    tapi_job_wrapper_priority_t priority,
                    unsigned int *wrapper_id)
{
    te_errno rc;
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_wrapper_add_in  in;
    tarpc_job_wrapper_add_out out;
    te_log_buf *tlbp_argv;
    size_t len;
    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.job_id = tapi_job_get_id(job);
    rc = tapi_job_wrapper_priority2tarpc_job_wrapper_priority(&priority,
                                                              &in.priority);
    if (rc != 0)
        return rc;

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

    rcf_rpc_call(rpcs, "job_wrapper_add", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_wrapper_add, out.retval);

    tlbp_argv = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, job_wrapper_add, "%u, %s, {%s}", "%r",
                 in.job_id, in.tool,
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

    RETVAL_TE_ERRNO(job_wrapper_add, out.retval);
}

te_errno
rpc_job_wrapper_delete(const tapi_job_t *job, unsigned int wrapper_id)
{
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_wrapper_delete_in  in;
    tarpc_job_wrapper_delete_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.wrapper_id = wrapper_id;
    in.job_id = tapi_job_get_id(job);

    rcf_rpc_call(rpcs, "job_wrapper_delete", &in, &out);

    TAPI_RPC_LOG(rpcs, job_wrapper_delete, "%u, %u", "%r",
                 in.job_id, in.wrapper_id, out.retval);
    RETVAL_TE_ERRNO(job_wrapper_delete, out.retval);
}

static te_errno
sched_affinity_param_alloc(tapi_job_sched_param *sched_param,
                           tarpc_job_sched_param *sched,
                           tarpc_job_sched_affinity **affinity)
{
    tarpc_job_sched_affinity *result;
    tapi_job_sched_affinity_param *af = sched_param->data;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate sched affinity");
        return TE_ENOMEM;
    }

    result->cpu_ids.cpu_ids_val = af->cpu_ids;
    result->cpu_ids.cpu_ids_len = af->cpu_ids_len;

    sched->data.type = TARPC_JOB_SCHED_AFFINITY;

    memcpy(&sched->data.tarpc_job_sched_param_data_u,
           result, sizeof(tarpc_job_sched_affinity));
    *affinity = result;
    return 0;
}

static te_errno
sched_priority_param_alloc(tapi_job_sched_param *sched_param,
                           tarpc_job_sched_param *sched,
                           tarpc_job_sched_priority **priority)
{
    tarpc_job_sched_priority *result;
    tapi_job_sched_priority_param *p = sched_param->data;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate sched priority");
        return TE_ENOMEM;
    }

    result->priority = p->priority;

    sched->data.type = TARPC_JOB_SCHED_PRIORITY;

    memcpy(&sched->data.tarpc_job_sched_param_data_u,
           result, sizeof(tarpc_job_sched_priority));
    return 0;
}

static te_errno
tapi_job_sched_param2tarpc_job_sched_param(tapi_job_sched_param *sched_tapi,
                                           tarpc_job_sched_param **sched_tarpc,
                                           size_t *sched_tarpc_len,
                                           tarpc_job_sched_affinity **affinity,
                                           tarpc_job_sched_priority **priority)
{
    tarpc_job_sched_param *sched = NULL;
    tarpc_job_sched_affinity *af = NULL;
    tarpc_job_sched_priority *pr = NULL;
    size_t count = 0;
    size_t i;
    te_errno rc;

    tapi_job_sched_param *item;

    for (item = sched_tapi; item->type != TAPI_JOB_SCHED_END; item++)
        count++;

    sched = TE_ALLOC(count * sizeof(*sched));
    if (sched == NULL)
    {
        ERROR("Failed to allocate sched parameters");
        rc = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < count; i++)
    {
        switch (sched_tapi[i].type)
        {
            case TAPI_JOB_SCHED_AFFINITY:
            {
                rc = sched_affinity_param_alloc(&sched_tapi[i], &sched[i],
                                                &af);
                if (rc != 0)
                    goto out;

                break;
            }

            case TAPI_JOB_SCHED_PRIORITY:
            {
                rc = sched_priority_param_alloc(&sched_tapi[i], &sched[i],
                                                &pr);
                if (rc != 0)
                    goto out;

                break;
            }

            default:
                ERROR("Unsupported scheduling parameter");
                rc = TE_EINVAL;
                goto out;
        }
    }

out:
    if (rc != 0)
    {
        free(sched);
        free(af);
        free(pr);
    }
    else
    {
        *sched_tarpc = sched;
        *sched_tarpc_len = count;
        *affinity = af;
        *priority = pr;
    }

    return rc;

}

te_errno
rpc_job_add_sched_param(const tapi_job_t *job,
                        tapi_job_sched_param *sched_tapi)
{
    te_errno rc;
    rcf_rpc_server *rpcs = tapi_job_get_rpcs(job);

    tarpc_job_sched_param *sched_param = NULL;
    size_t sched_param_len;
    tarpc_job_sched_affinity *affinity = NULL;
    tarpc_job_sched_priority *priority = NULL;

    tarpc_job_add_sched_param_in in;
    tarpc_job_add_sched_param_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rc = tapi_job_sched_param2tarpc_job_sched_param(sched_tapi, &sched_param,
                                                    &sched_param_len, &affinity,
                                                    &priority);
    if (rc != 0)
        return rc;

    in.job_id = tapi_job_get_id(job);
    in.param.param_len = sched_param_len;
    in.param.param_val = sched_param;

    rcf_rpc_call(rpcs, "job_add_sched_param", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(job_add_sched_param, out.retval);

    TAPI_RPC_LOG(rpcs, job_add_sched_param, "%u", "%r", in.job_id, out.retval);

    free(sched_param);
    free(affinity);
    free(priority);

    RETVAL_TE_ERRNO(job_add_sched_param, out.retval);
}
