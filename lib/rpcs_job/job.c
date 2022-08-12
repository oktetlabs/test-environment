/** @file
 * @brief RPC for Agent job control
 *
 * RPC routines implementation to call Agent job control functions
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 */

#define TE_LGR_USER     "RPC JOB"

#include "te_config.h"

#include "ta_job.h"
#include "te_alloc.h"
#include "te_str.h"
#include "rpc_server.h"

#include "agentlib.h"

static ta_job_manager_t *manager = NULL;
static te_bool is_manager_init = FALSE;
/**
 * Initialize job manager if it is not initialized yet.
 * Since the backend does not have a clear entry point (i.e. the function that
 * is called before any others), the macro must be called at the beginning
 * of each backend function which uses job manager. Otherwise there's
 * a risk that the manager will be used uninitialized. Note that this macro also
 * forces the backend functions to have te_errno as a return type.
 */
#define INIT_MANAGER_IF_NEEDED(_manager)           \
    do {                                           \
        te_errno rc_;                              \
                                                   \
        if (!is_manager_init)                      \
        {                                          \
            rc_ = ta_job_manager_init(&_manager);  \
            if (rc_ != 0)                          \
                return rc_;                        \
                                                   \
            is_manager_init = TRUE;                \
        }                                          \
    } while(0)

/* Note: argv and env MUST remain valid after a successful call */
static te_errno
job_create(const char *spawner, const char *tool, char **argv,
           char **env, unsigned int *job_id)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_create(manager, spawner, tool, argv, env, job_id);
}

static te_errno
job_start(unsigned int job_id)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_start(manager, job_id);
}

static te_errno
job_allocate_channels(unsigned int job_id, te_bool input_channels,
                      unsigned int n_channels, unsigned int *channels)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_allocate_channels(manager, job_id, input_channels,
                                    n_channels, channels);
}

static te_errno
job_deallocate_channels(unsigned int n_channels, unsigned int *channels)
{
    INIT_MANAGER_IF_NEEDED(manager);

    ta_job_deallocate_channels(manager, n_channels, channels);

    return 0;
}

static te_errno
job_attach_filter(const char *filter_name, unsigned int n_channels,
                  unsigned int *channels, te_bool readable,
                  te_log_level log_level, unsigned int *filter_id)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_attach_filter(manager, filter_name, n_channels, channels,
                                readable, log_level, filter_id);
}

static te_errno
job_filter_add_regexp(unsigned int filter_id, char *re, unsigned int extract)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_filter_add_regexp(manager, filter_id, re, extract);
}

static te_errno
job_filter_add_channels(unsigned int filter_id, unsigned int n_channels,
                        unsigned int *channels)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_filter_add_channels(manager, filter_id, n_channels, channels);
}

static te_errno
job_filter_remove_channels(unsigned int filter_id, unsigned int n_channels,
                           unsigned int *channels)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_filter_remove_channels(manager, filter_id, n_channels,
                                         channels);
}

static te_errno
job_poll(unsigned int n_channels, unsigned int *channel_ids, int timeout_ms,
         te_bool filter_only)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_poll(manager, n_channels, channel_ids,
                       timeout_ms, filter_only);
}

static void
ta_job_buffer2tarpc_job_buffer(const ta_job_buffer_t *from,
                               tarpc_job_buffer *to)
{
    to->channel = from->channel_id;
    to->filter = from->filter_id;
    to->dropped = from->dropped;
    to->eos = from->eos;
    to->data.data_val = from->data;
    to->data.data_len = from->size;
}

/* Receive first message from filter queue and remove it from there. */
static te_errno
job_receive(unsigned int n_filters, unsigned int *filters, int timeout_ms,
            tarpc_job_buffer *buffer)
{
    te_errno rc;
    ta_job_buffer_t ta_buf;

    rc = ta_job_receive(manager, n_filters, filters, timeout_ms, &ta_buf);
    if (rc == 0)
        ta_job_buffer2tarpc_job_buffer(&ta_buf, buffer);

    return rc;
}

/*
 * Receive last (or second-to-last) message from filter queue,
 * do not remove it from there.
 */
static te_errno
job_receive_last(unsigned int n_filters, unsigned int *filters, int timeout_ms,
                 tarpc_job_buffer *buffer)
{
    te_errno rc;
    ta_job_buffer_t ta_buf;

    INIT_MANAGER_IF_NEEDED(manager);

    rc = ta_job_receive_last(manager, n_filters, filters, timeout_ms, &ta_buf);
    if (rc == 0)
        ta_job_buffer2tarpc_job_buffer(&ta_buf, buffer);

    return rc;
}

/* Receive multiple messages at once. */
static te_errno
job_receive_many(unsigned int n_filters, unsigned int *filters, int timeout_ms,
                 tarpc_job_buffer **buffers, unsigned int *count)
{
    te_errno rc;
    ta_job_buffer_t *ta_bufs;
    unsigned int i;

    INIT_MANAGER_IF_NEEDED(manager);

    rc = ta_job_receive_many(manager, n_filters, filters, timeout_ms, &ta_bufs,
                             count);
    if (rc == 0)
    {
        *buffers = calloc(*count, sizeof(**buffers));
        if (*buffers == NULL)
        {
            free(ta_bufs);
            return TE_ENOMEM;
        }

        for (i = 0; i < *count; i++)
            ta_job_buffer2tarpc_job_buffer(&ta_bufs[i], &((*buffers)[i]));
    }

    free(ta_bufs);

    return rc;
}

static te_errno
job_clear(unsigned int n_filters, unsigned int *filters)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_clear(manager, n_filters, filters);
}

static te_errno
job_send(unsigned int channel_id, size_t count, uint8_t *buf)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_send(manager, channel_id, count, buf);
}

static te_errno
job_kill(unsigned int job_id, int signo)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_kill(manager, job_id, signo);
}

static te_errno
job_killpg(unsigned int job_id, int signo)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_killpg(manager, job_id, signo);
}

static te_errno
ta_job_status2tarpc_job_status(const ta_job_status_t *from,
                               tarpc_job_status *to)
{
    to->value = from->value;
    switch (from->type)
    {
        case TA_JOB_STATUS_EXITED:
            to->type = TARPC_JOB_STATUS_EXITED;
            break;
        case TA_JOB_STATUS_SIGNALED:
            to->type = TARPC_JOB_STATUS_SIGNALED;
            break;
        case TA_JOB_STATUS_UNKNOWN:
            to->type = TARPC_JOB_STATUS_UNKNOWN;
            break;
        default:
            ERROR("Invalid TA job status");
            return TE_EINVAL;
    }

    return 0;
}

static te_errno
job_wait(unsigned int job_id, int timeout_ms, tarpc_job_status *status)
{
    te_errno rc;
    ta_job_status_t ta_status;

    INIT_MANAGER_IF_NEEDED(manager);

    rc = ta_job_wait(manager, job_id, timeout_ms, &ta_status);
    if (rc == 0)
        rc = ta_job_status2tarpc_job_status(&ta_status, status);

    return rc;
}

static te_errno
job_stop(unsigned int job_id, int signo, int term_timeout_ms)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_stop(manager, job_id, signo, term_timeout_ms);
}

static te_errno
job_destroy(unsigned int job_id, int term_timeout_ms)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_destroy(manager, job_id, term_timeout_ms);
}

static te_errno
tarpc_job_wrapper_priority2ta_job_wrapper_priority(
                             const tarpc_job_wrapper_priority *from,
                             ta_job_wrapper_priority_t *to)
{
    switch(*from)
    {
        case TARPC_JOB_WRAPPER_PRIORITY_LOW:
            *to = TA_JOB_WRAPPER_PRIORITY_LOW;
            break;

        case TARPC_JOB_WRAPPER_PRIORITY_DEFAULT:
            *to = TA_JOB_WRAPPER_PRIORITY_DEFAULT;
            break;

        case TARPC_JOB_WRAPPER_PRIORITY_HIGH:
            *to = TA_JOB_WRAPPER_PRIORITY_HIGH;
            break;

        default:
            ERROR("Priority value is not supported");
            return TE_EFAIL;
    }

    return 0;
}

static te_errno
job_wrapper_add(const char *tool, char **argv, unsigned int job_id,
                tarpc_job_wrapper_priority priority, unsigned int *wrapper_id)
{
    te_errno rc;
    ta_job_wrapper_priority_t ta_priority;

    INIT_MANAGER_IF_NEEDED(manager);

    rc = tarpc_job_wrapper_priority2ta_job_wrapper_priority(&priority,
                                                            &ta_priority);
    if (rc != 0)
        return rc;

    return ta_job_wrapper_add(manager, tool, argv, job_id, ta_priority,
                              wrapper_id);
}

static te_errno
job_wrapper_delete(unsigned int job_id, unsigned int wrapper_id)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_wrapper_delete(manager, job_id, wrapper_id);
}

static te_errno
job_add_sched_param(unsigned int job_id, te_sched_param *sched_params)
{
    INIT_MANAGER_IF_NEEDED(manager);

    return ta_job_add_sched_param(manager, job_id, sched_params);
}

static te_errno
sched_affinity_param_alloc(te_sched_affinity_param **af,
                           tarpc_job_sched_param_data *data)
{
    int cpu_ids_len;
    te_sched_affinity_param *affinity;
    affinity = TE_ALLOC(sizeof(te_sched_affinity_param));
    if (affinity == NULL)
        return TE_ENOMEM;

    cpu_ids_len = data->tarpc_job_sched_param_data_u.
                    affinity.cpu_ids.cpu_ids_len;

    affinity->cpu_ids_len = cpu_ids_len;
    affinity->cpu_ids = TE_ALLOC(cpu_ids_len * sizeof(int));
    if (affinity->cpu_ids == NULL)
        return TE_ENOMEM;

    memcpy(affinity->cpu_ids,
           data->tarpc_job_sched_param_data_u.affinity.cpu_ids.cpu_ids_val,
           cpu_ids_len * sizeof(int));
    *af = affinity;
    return 0;
}

static te_errno
sched_priority_param_alloc(te_sched_priority_param **prio,
                           tarpc_job_sched_param_data *data)
{
    te_sched_priority_param *priority;
    priority = TE_ALLOC(sizeof(te_sched_priority_param));
    if (priority == NULL)
        return TE_ENOMEM;

    priority->priority = data->tarpc_job_sched_param_data_u.prio.priority;
    *prio = priority;
    return 0;
}

TARPC_FUNC_STATIC(job_create, {},
{
    char **argv = NULL;
    char **env = NULL;
    unsigned int i;

    if (in->argv.argv_len != 0)
    {
        argv = calloc(in->argv.argv_len, sizeof(*argv));
        if (argv == NULL)
            goto err;

        for (i = 0; i < in->argv.argv_len - 1; ++i)
        {
            argv[i] = strdup(in->argv.argv_val[i].str.str_val);
            if (argv[i] == NULL)
                goto err;
        }
    }

    if (in->env.env_len != 0)
    {
        env = calloc(in->env.env_len, sizeof(*env));
        if (env == NULL)
            goto err;

        for (i = 0; i < in->env.env_len - 1; ++i)
        {
            env[i] = strdup(in->env.env_val[i].str.str_val);
            if (env[i] == NULL)
                goto err;
        }
    }

    MAKE_CALL(out->retval = func(in->spawner, in->tool, argv, env,
                                 &out->job_id));
    out->common.errno_changed = FALSE;
    if (out->retval != 0)
        goto free;
    else
        goto done;

err:
    out->common._errno = TE_RC(TE_RPCS, TE_ENOMEM);
    out->retval = -out->common._errno;
free:
    te_str_free_array(argv);
    te_str_free_array(env);
done:
    ;
})

TARPC_FUNC_STATIC(job_allocate_channels,
{
    COPY_ARG(channels);
},
{
    MAKE_CALL(out->retval = func(in->job_id, in->input_channels,
                                 in->n_channels, out->channels.channels_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_deallocate_channels, {},
{
    MAKE_CALL(out->retval = func(in->channels.channels_len,
                                 in->channels.channels_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_attach_filter, {},
{
    MAKE_CALL(out->retval = func(in->filter_name, in->channels.channels_len,
                                 in->channels.channels_val,
                                 in->readable, in->log_level, &out->filter));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_filter_add_regexp, {},
{
    MAKE_CALL(out->retval = func(in->filter, in->re, in->extract));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_filter_add_channels, {},
{
    MAKE_CALL(out->retval = func(in->filter, in->channels.channels_len,
                                 in->channels.channels_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_filter_remove_channels, {},
{
  MAKE_CALL(out->retval = func(in->filter, in->channels.channels_len,
                               in->channels.channels_val));
  out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_start, {},
{
    MAKE_CALL(out->retval = func(in->job_id));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_receive, {},
{
    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val,
                                 in->timeout_ms, &out->buffer));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_receive_last, {},
{
    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val,
                                 in->timeout_ms, &out->buffer));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_receive_many, {},
{
    tarpc_job_buffer *bufs = NULL;
    unsigned int bufs_count = in->count;

    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val,
                                 in->timeout_ms, &bufs, &bufs_count));
    out->common.errno_changed = FALSE;

    out->buffers.buffers_val = bufs;
    out->buffers.buffers_len = bufs_count;
})

TARPC_FUNC_STATIC(job_clear, {},
{
    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_send, {},
{
    MAKE_CALL(out->retval = func(in->channel, in->buf.buf_len,
                                 in->buf.buf_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_poll, {},
{
    MAKE_CALL(out->retval = func(in->channels.channels_len,
                                 in->channels.channels_val,
                                 in->timeout_ms, FALSE));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_kill, {},
{
    MAKE_CALL(out->retval = func(in->job_id, signum_rpc2h(in->signo)));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_killpg, {},
{
    MAKE_CALL(out->retval = func(in->job_id, signum_rpc2h(in->signo)));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_wait, {},
{
    MAKE_CALL(out->retval = func(in->job_id, in->timeout_ms, &out->status));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_stop, {},
{
    MAKE_CALL(out->retval = func(in->job_id,
                                 signum_rpc2h(in->signo),
                                 in->term_timeout_ms));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_destroy, {},
{
    MAKE_CALL(out->retval = func(in->job_id, in->term_timeout_ms));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_wrapper_add, {},
{
    char **argv = NULL;
    unsigned int i;

    if (in->argv.argv_len != 0)
    {
        argv = calloc(in->argv.argv_len, sizeof(*argv));
        if (argv == NULL)
            goto err;

        for (i = 0; i < in->argv.argv_len - 1; ++i)
        {
            argv[i] = strdup(in->argv.argv_val[i].str.str_val);
            if (argv[i] == NULL)
                goto err;
        }
    }

    MAKE_CALL(out->retval = func(in->tool, argv, in->job_id,
                                 in->priority, &out->wrapper_id));
    out->common.errno_changed = FALSE;
    goto done;

err:
    out->common._errno = TE_RC(TE_RPCS, TE_ENOMEM);
    out->retval = -out->common._errno;
    for (i = 0; argv != NULL && argv[i] != NULL; i++)
        free(argv[i]);
    free(argv);
done:
    ;
})

TARPC_FUNC_STATIC(job_wrapper_delete, {},
{
    MAKE_CALL(out->retval = func(in->job_id, in->wrapper_id));
})

TARPC_FUNC_STATIC(job_add_sched_param, {},
{
    te_sched_param *sched_param = NULL;
    te_sched_affinity_param *affinity = NULL;
    te_sched_priority_param *prio = NULL;
    int i;
    int len = in->param.param_len;
    te_errno rc;

    sched_param = TE_ALLOC((len + 1) * sizeof(te_sched_param));
    if (sched_param == NULL)
    {
        rc = TE_ENOMEM;
        goto err;
    }

    for (i = 0; i < len; i++)
    {
        switch (in->param.param_val[i].data.type)
        {
            case TARPC_JOB_SCHED_AFFINITY:
            {
                sched_param[i].type = TE_SCHED_AFFINITY;
                rc = sched_affinity_param_alloc(&affinity,
                                                &in->param.param_val[i].data);
                if (rc != 0)
                    goto err;

                sched_param[i].data = (void *)affinity;
                break;
            }

            case TARPC_JOB_SCHED_PRIORITY:
            {
                sched_param[i].type = TE_SCHED_PRIORITY;

                rc = sched_priority_param_alloc(&prio,
                                                &in->param.param_val[i].data);
                if (rc != 0)
                    goto err;

                sched_param[i].data = (void *)prio;
                break;
            }
            default:
                ERROR("Unsupported scheduling parameter");
                rc = TE_EINVAL;
                goto err;
        }
    }

    sched_param[len].type = TE_SCHED_END;
    sched_param[len].data = NULL;

    MAKE_CALL(out->retval = func(in->job_id, sched_param));
    out->common.errno_changed = FALSE;
    goto done;
err:
    free(sched_param);
    if (affinity != NULL)
    {
        free(affinity->cpu_ids);
        free(affinity);
    }
    free(prio);

    out->common._errno = TE_RC(TE_RPCS, rc);
    out->retval = -out->common._errno;
done:
    ;
})
