/** @file
 * @brief Test API for Agent job control
 *
 * Test API for Agent job control functions (implementation)
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#include "te_config.h"

#include "tapi_job.h"
#include "tapi_mem.h"
#include "tapi_rpc_job.h"
#include "tapi_test.h"

typedef struct channel_entry {
    SLIST_ENTRY(channel_entry) next;
    tapi_job_channel_t *channel;
} channel_entry;

/* List of all channels/filters belonging to a job */
typedef SLIST_HEAD(channel_entry_list, channel_entry) channel_entry_list;

struct tapi_job_t {
    rcf_rpc_server *rpcs;
    unsigned int id;

    SLIST_ENTRY(tapi_job_t) next;

    /* Head of channel entries */
    channel_entry_list channel_entries;
};

/* List of all jobs */
typedef SLIST_HEAD(job_list, tapi_job_t) job_list;

struct tapi_job_channel_t {
    int ref_count;
    tapi_job_t *job;

    rcf_rpc_server *rpcs;
    unsigned int id;
};

static job_list all_jobs = SLIST_HEAD_INITIALIZER(&all_jobs);

static void
init_channel(tapi_job_t *job, rcf_rpc_server *rpcs, unsigned int id,
             int ref_count, tapi_job_channel_t *channel)
{
    channel->job = job;
    channel->rpcs = rpcs;
    channel->id = id;
    channel->ref_count = ref_count;
}

static void
init_primary_channel(tapi_job_t *job, rcf_rpc_server *rpcs, unsigned int id,
                     tapi_job_channel_t *channel)
{
    return init_channel(job, rpcs, id, 1, channel);
}

/* Secondary channel (filter) does not belong to a particular job */
static void
init_secondary_channel(rcf_rpc_server *rpcs, unsigned int id, int ref_count,
                       tapi_job_channel_t *channel)
{
    return init_channel(NULL, rpcs, id, ref_count, channel);
}

static te_bool
is_primary_channel(const tapi_job_channel_t *channel)
{
    return (channel->job != NULL);
}

static tapi_job_channel_t *
get_channel(const rcf_rpc_server *rpcs, unsigned int id)
{
    channel_entry *entry;
    tapi_job_t *job;

    SLIST_FOREACH(job, &all_jobs, next)
    {
        if (job->rpcs == rpcs)
        {
            SLIST_FOREACH(entry, &job->channel_entries, next)
            {
                if (entry->channel->id == id)
                    return entry->channel;
            }
        }
    }

    return NULL;
}

/* See description in tapi_job.h */
te_errno
tapi_job_rpc_create(rcf_rpc_server *rpcs, const char *spawner,
                    const char *program, const char **argv,
                    const char **env, tapi_job_t **job)
{
    tapi_job_t *tapi_job;
    te_errno rc;

    if (rpcs == NULL)
    {
        ERROR("RPC server is NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    tapi_job = tapi_calloc(1, sizeof(*tapi_job));

    tapi_job->rpcs = rpcs;
    SLIST_INIT(&tapi_job->channel_entries);
    rc = rpc_job_create(rpcs, spawner == NULL ? "" : spawner, program,
                        argv, env, &tapi_job->id);
    if (rc != 0)
    {
        free(tapi_job);
        return rc;
    }

    SLIST_INSERT_HEAD(&all_jobs, tapi_job, next);

    *job = tapi_job;
    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_start(tapi_job_t *job)
{
    return rpc_job_start(job->rpcs, job->id);
}

/* See description in tapi_job.h */
te_errno
tapi_job_kill(tapi_job_t *job, int signo)
{
    return rpc_job_kill(job->rpcs, job->id, signum_h2rpc(signo));
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

/* See description in tapi_job.h */
te_errno
tapi_job_wait(tapi_job_t *job, int timeout_ms, tapi_job_status_t *status)
{
    tarpc_job_status tarpc_status;
    te_errno rc;

    rc = rpc_job_wait(job->rpcs, job->id, timeout_ms, &tarpc_status);

    if (rc != 0)
        return rc;

    if ((rc = tarpc_job_status2tapi_job_status(&tarpc_status, status)) != 0)
        return rc;

    return 0;
}

static void
add_channel_to_entry_list(tapi_job_channel_t *channel, channel_entry_list *list)
{
    channel_entry *entry;

    entry = tapi_malloc(sizeof(*entry));
    entry->channel = channel;

    SLIST_INSERT_HEAD(list, entry, next);
}

static te_errno
tapi_job_alloc_channels(tapi_job_t *job, te_bool input_channels,
                        unsigned int n_channels,
                        tapi_job_channel_t *channels[n_channels])
{
    tapi_job_channel_t **channels_tmp = NULL;
    unsigned int *channel_ids = NULL;
    unsigned int i;
    te_errno rc;

    if (channels != NULL)
    {
        channel_ids = tapi_calloc(n_channels, sizeof(*channel_ids));
        channels_tmp = tapi_calloc(n_channels, sizeof(*channels_tmp));

        for (i = 0; i < n_channels; i++)
            channels_tmp[i] = tapi_malloc(sizeof(tapi_job_channel_t));
    }

    rc = rpc_job_allocate_channels(job->rpcs, job->id, input_channels,
                                   n_channels, channel_ids);
    if (rc != 0)
    {
        free(channel_ids);
        if (channels_tmp != NULL)
        {
            for (i = 0; i < n_channels; i++)
                free(channels_tmp[i]);
            free(channels_tmp);
        }

        return rc;
    }

    for (i = 0; channels != NULL && i < n_channels; i++)
    {
        init_primary_channel(job, job->rpcs, channel_ids[i], channels_tmp[i]);
        channels[i] = channels_tmp[i];
        add_channel_to_entry_list(channels[i], &job->channel_entries);
    }

    free(channel_ids);
    free(channels_tmp);

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_alloc_input_channels(tapi_job_t *job, unsigned int n_channels,
                              tapi_job_channel_t *channels[n_channels])
{
    return tapi_job_alloc_channels(job, TRUE, n_channels, channels);
}

/* See description in tapi_job.h */
te_errno
tapi_job_alloc_output_channels(tapi_job_t *job, unsigned int n_channels,
                               tapi_job_channel_t *channels[n_channels])
{
    return tapi_job_alloc_channels(job, FALSE, n_channels, channels);
}

static te_errno
validate_channel_set(const tapi_job_channel_set_t channels)
{
    rcf_rpc_server *rpcs = NULL;
    unsigned int i;

    if (channels == NULL)
    {
        ERROR("Channel set with NULL value is rejected");
        return 0;
    }

    if (channels[0] == NULL)
    {
        ERROR("Empty channel set is rejected");
        return 0;
    }

    for (i = 0; channels[i] != NULL; i++)
    {
        if (rpcs != NULL && channels[i]->rpcs != rpcs)
            return TE_RC(TE_TAPI, TE_EXDEV);
        rpcs = channels[i]->rpcs;
    }

    return 0;
}

/* Note: pass @p channels after validation */
static void
alloc_id_array_from_channel_set(const tapi_job_channel_set_t channels,
                                unsigned int *n_channels,
                                unsigned int **channel_ids)
{
    unsigned int count = 0;
    unsigned int *ids;
    unsigned int i;

    for (i = 0; channels[i] != NULL; i++)
        count++;

    ids = tapi_calloc(count, sizeof(*ids));

    for (i = 0; i < count; i++)
        ids[i] = channels[i]->id;

    *n_channels = count;
    *channel_ids = ids;
}

/* See description in tapi_job.h */
te_errno
tapi_job_attach_filter(tapi_job_channel_set_t channels, const char *filter_name,
                       te_bool readable, te_log_level log_level,
                       tapi_job_channel_t **filter)
{
    tapi_job_channel_t *result = NULL;
    unsigned int *channel_ids;
    unsigned int n_channels;
    unsigned int result_id;
    rcf_rpc_server *rpcs;
    unsigned int i;
    te_errno rc;

    if ((rc = validate_channel_set(channels)) != 0)
        return rc;

    for (i = 0; channels[i] != NULL; i++)
    {
        if (!is_primary_channel(channels[i]))
        {
            ERROR("Attach filter fail: filters over filters are not supported");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
    }

    rpcs = channels[0]->rpcs;

    alloc_id_array_from_channel_set(channels, &n_channels, &channel_ids);

    if (filter != NULL)
        result = tapi_malloc(sizeof(*result));

    rc = rpc_job_attach_filter(rpcs,
                               filter_name == NULL ? "Unnamed" : filter_name,
                               n_channels, channel_ids, readable, log_level,
                               &result_id);
    free(channel_ids);
    if (rc != 0)
    {
        free(result);
        return rc;
    }

    if (filter != NULL)
    {
        unsigned int i;

        init_secondary_channel(rpcs, result_id, n_channels, result);

        for (i = 0; i < n_channels; i++)
        {
            add_channel_to_entry_list(result,
                                      &channels[i]->job->channel_entries);
        }
        *filter = result;
    }

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_filter_add_regexp(tapi_job_channel_t *filter, const char *re,
                           unsigned int extract)
{
    return rpc_job_filter_add_regexp(filter->rpcs, filter->id, re, extract);
}


/* See description in tapi_job.h */
te_errno
tapi_job_send(tapi_job_channel_t *channel, const te_string *str)
{
    return rpc_job_send(channel->rpcs, channel->id, str->ptr, str->size);
}

/* See description in tapi_job.h */
te_errno
tapi_job_poll(const tapi_job_channel_set_t wait_set, int timeout_ms)
{
    unsigned int *channel_ids;
    unsigned int n_channels;
    rcf_rpc_server *rpcs;
    te_errno rc;

    if ((rc = validate_channel_set(wait_set)) != 0)
        return rc;
    rpcs = wait_set[0]->rpcs;

    alloc_id_array_from_channel_set(wait_set, &n_channels, &channel_ids);

    rc = rpc_job_poll(rpcs, n_channels, channel_ids, timeout_ms);
    free(channel_ids);

    return rc;
}

static void
tarpc_job_buffer2tapi_job_buffer(const rcf_rpc_server *rpcs,
                                 const tarpc_job_buffer *from,
                                 tapi_job_buffer_t *to)
{
    te_errno rc;

    to->dropped = from->dropped;
    to->eos = from->eos;

    to->channel = get_channel(rpcs, from->channel);
    if (to->channel == NULL)
        TEST_FAIL("Failed to find a channel with id %u", from->channel);

    to->filter = get_channel(rpcs, from->filter);
    if (to->filter == NULL)
        TEST_FAIL("Failed to find a filter with id %u", from->filter);

    rc = te_string_append(&to->data, "%.*s", from->data.data_len,
                          from->data.data_val);
    if (rc != 0)
        TEST_FAIL("TE string append failed");
}

/* See description in tapi_job.h */
te_errno
tapi_job_receive(const tapi_job_channel_set_t filters, int timeout_ms,
                 tapi_job_buffer_t *buffer)
{
    unsigned int *channel_ids;
    unsigned int n_channels;
    rcf_rpc_server *rpcs;

    tarpc_job_buffer buf;
    te_errno rc;

    if ((rc = validate_channel_set(filters)) != 0)
        return rc;
    rpcs = filters[0]->rpcs;

    alloc_id_array_from_channel_set(filters, &n_channels, &channel_ids);

    rc = rpc_job_receive(rpcs, n_channels, channel_ids, timeout_ms, &buf);
    free(channel_ids);
    if (rc != 0)
        return rc;

    tarpc_job_buffer2tapi_job_buffer(rpcs, &buf, buffer);
    free(buf.data.data_val);

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_destroy(tapi_job_t *job)
{
    te_errno rc;
    channel_entry *entry;
    channel_entry *entry_tmp;

    if (job == NULL)
        return 0;

    rc = rpc_job_destroy(job->rpcs, job->id);
    if (rc != 0)
        return rc;

    SLIST_FOREACH_SAFE(entry, &job->channel_entries, next, entry_tmp)
    {
        if (--entry->channel->ref_count <= 0)
            free(entry->channel);
        free(entry);
    }

    SLIST_REMOVE(&all_jobs, job, tapi_job_t, next);
    free(job);

    return 0;
}
