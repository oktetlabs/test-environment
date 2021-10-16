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

#include "te_queue.h"
#include "tapi_job.h"
#include "tapi_job_factory_cfg.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_job_internal.h"
#include "tapi_job_methods.h"
#include "tapi_mem.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_job.h"
#include "tapi_test.h"
#include "te_alloc.h"

#define TAPI_JOB_CHECK_METHOD_SUPPORT(_job, _method) \
    do {                                                                       \
        if ((_job)->methods._method == NULL)                                   \
        {                                                                      \
            ERROR("The job was created by a factory that does not "            \
                  "support method '%s'", #_method);                            \
            return TE_RC(TE_TAPI, TE_EOPNOTSUPP);                              \
        }                                                                      \
    } while (0)

typedef struct channel_entry {
    SLIST_ENTRY(channel_entry) next;
    tapi_job_channel_t *channel;
} channel_entry;

typedef SLIST_HEAD(channel_entry_list, channel_entry) channel_entry_list;

typedef enum tapi_job_factory_type {
    TAPI_JOB_FACTORY_RPC,
    TAPI_JOB_FACTORY_CFG,
} tapi_job_factory_type;

struct tapi_job_factory_t {
    tapi_job_factory_type type;
    /** Backend-specific data */
    union {
        rcf_rpc_server *rpcs;
        const char *ta;
    } backend;
};

struct tapi_job_t {
    tapi_job_factory_t *factory;

    /** Backend-specific data */
    union {
        /** Identifies job created by RPC factory */
        unsigned int id;
    } backend;

    tapi_job_methods_t methods;

    SLIST_ENTRY(tapi_job_t) next;

    /* List of all channels/filters belonging to a job */
    channel_entry_list channel_entries;
};

struct tapi_job_wrapper_t {
    tapi_job_t *job;
    unsigned int id;
};

/* List of all jobs */
typedef SLIST_HEAD(job_list, tapi_job_t) job_list;

struct tapi_job_channel_t {
    int ref_count;
    /*
     * For output primary channels: list of filters attached to the channel;
     * For filters and input primary channels: empty list.
     */
    channel_entry_list filter_entries;
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

    SLIST_INIT(&channel->filter_entries);
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
        if (tapi_job_get_rpcs(job) == rpcs)
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

/* See description in tapi_job_factory_rpc.h */
te_errno
tapi_job_factory_rpc_create(rcf_rpc_server *rpcs,
                            struct tapi_job_factory_t **factory)
{
    tapi_job_factory_t *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    result->type = TAPI_JOB_FACTORY_RPC;
    result->backend.rpcs = rpcs;

    *factory = result;

    return 0;
}

/* See description in tapi_job_factory_cfg.h */
te_errno
tapi_job_factory_cfg_create(const char *ta, tapi_job_factory_t **factory)
{
    tapi_job_factory_t *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    result->type = TAPI_JOB_FACTORY_CFG;
    result->backend.ta = ta;

    *factory = result;

    return 0;
}

/* See description in tapi_job.h */
const char *
tapi_job_factory_ta(const tapi_job_factory_t *factory)
{
    if (factory == NULL)
    {
        ERROR("Failed to get test agent from NULL factory");
        return NULL;
    }

    switch (factory->type)
    {
        case TAPI_JOB_FACTORY_RPC:
            return factory->backend.rpcs->ta;

        case TAPI_JOB_FACTORY_CFG:
            return factory->backend.ta;

        default:
            ERROR("Invalid job factory type");
            return NULL;
    }
}

/* See description in tapi_job.h */
void
tapi_job_factory_destroy(tapi_job_factory_t *factory)
{
    if (factory == NULL)
        return;

    switch (factory->type)
    {
        case TAPI_JOB_FACTORY_RPC:
        case TAPI_JOB_FACTORY_CFG:
            break;

        default:
            ERROR("Invalid job factory type");
            break;
    }

    free(factory);
}

/* job->factory must already be set */
static void
init_methods(tapi_job_t *job)
{
    switch (job->factory->type)
    {
        case TAPI_JOB_FACTORY_RPC:
            job->methods = rpc_job_methods;
            break;

        default:
            /* job->factory was set so it is supposed to be valid */
            assert(0);
    }
}

/* See description in tapi_job_internal.h */
rcf_rpc_server *
tapi_job_get_rpcs(const tapi_job_t *job)
{
    if (job == NULL)
        TEST_FAIL("Job is NULL");

    if (job->factory->type != TAPI_JOB_FACTORY_RPC)
        TEST_FAIL("Job was not created by RPC factory");

    return job->factory->backend.rpcs;
}

/* See description in tapi_job_internal.h */
unsigned int
tapi_job_get_id(const tapi_job_t *job)
{
    if (job == NULL)
        TEST_FAIL("Job is NULL");

    if (job->factory->type != TAPI_JOB_FACTORY_RPC)
        TEST_FAIL("Job was not created by RPC factory");

    return job->backend.id;
}

/* See description in tapi_job_internal.h */
void
tapi_job_set_id(tapi_job_t *job, unsigned int id)
{
    if (job == NULL)
        TEST_FAIL("Job is NULL");

    if (job->factory->type != TAPI_JOB_FACTORY_RPC)
        TEST_FAIL("Job was not created by RPC factory");

    job->backend.id = id;
}

/* See description in tapi_job.h */
te_errno
tapi_job_create(tapi_job_factory_t *factory, const char *spawner,
                const char *program, const char **argv,
                const char **env, tapi_job_t **job)
{
    tapi_job_t *tapi_job;
    te_errno rc;

    if (factory == NULL)
    {
        ERROR("Job factory is NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (factory->type != TAPI_JOB_FACTORY_RPC)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    tapi_job = tapi_calloc(1, sizeof(*tapi_job));

    tapi_job->factory = factory;
    SLIST_INIT(&tapi_job->channel_entries);

    init_methods(tapi_job);

    TAPI_JOB_CHECK_METHOD_SUPPORT(tapi_job, create);
    rc = tapi_job->methods.create(tapi_job, spawner == NULL ? "" : spawner,
                                  program, argv, env);
    if (rc != 0)
    {
        free(tapi_job);
        return rc;
    }

    SLIST_INSERT_HEAD(&all_jobs, tapi_job, next);

    *job = tapi_job;
    return 0;
}

static te_errno
tapi_job_simple_alloc_channels(tapi_job_t *job, tapi_job_simple_desc_t *desc)
{
    tapi_job_channel_t *out_channels[2] = {NULL, NULL};
    tapi_job_channel_t *in_channel = NULL;
    int n_out_channels = 0;
    te_errno rc;

    if (desc->stdout_loc != NULL)
        n_out_channels = 1;
    if (desc->stderr_loc != NULL)
        n_out_channels = 2;

    if (n_out_channels > 0)
    {
        rc = tapi_job_alloc_output_channels(job, n_out_channels, out_channels);
        if (rc != 0)
            return rc;

        if (desc->stdout_loc != NULL)
            *desc->stdout_loc = out_channels[0];

        if (desc->stderr_loc != NULL)
            *desc->stderr_loc = out_channels[1];
    }

    if (desc->stdin_loc != NULL)
    {
        rc = tapi_job_alloc_input_channels(job, 1, &in_channel);
        if (rc != 0)
            return rc;

        *desc->stdin_loc = in_channel;
    }

    return 0;
}

te_errno
tapi_job_simple_create(tapi_job_factory_t *factory,
                       tapi_job_simple_desc_t *desc)
{
    te_errno rc;
    tapi_job_simple_filter_t *filter;

    if (*desc->job_loc != NULL)
    {
        ERROR("TAPI Job simple description is already associated with a job");
        return TE_EALREADY;
    }

    rc = tapi_job_create(factory, desc->spawner, desc->program, desc->argv,
                         desc->env, desc->job_loc);
    if (rc != 0)
        return rc;

    if ((rc = tapi_job_simple_alloc_channels(*desc->job_loc, desc)) != 0)
    {
        tapi_job_destroy(*desc->job_loc, -1);
        *desc->job_loc = NULL;
        return rc;
    }

    if (desc->filters == NULL)
        return 0;

    /* Last element should have use_stdout and  use_stderr set to FALSE */
    for (filter = desc->filters;
         filter->use_stdout || filter->use_stderr;
         filter++)
    {
        if ((rc = tapi_job_attach_simple_filter(desc, filter)) != 0)
        {
            tapi_job_destroy(*desc->job_loc, -1);
            *desc->job_loc = NULL;
            return rc;
        }
    }

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_factory_set_path(tapi_job_factory_t *factory)
{
    te_errno rc;
    char *ta_path = NULL;
    te_bool awaiting_error;
    rcf_rpc_server *rpcs;

    if (factory == NULL || factory->type != TAPI_JOB_FACTORY_RPC)
    {
        ERROR("Invalid factory passed to set path");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rpcs = factory->backend.rpcs;
    awaiting_error = RPC_AWAITING_ERROR(rpcs);

    rc = cfg_get_instance_string_fmt(&ta_path, "/agent:%s/env:PATH", rpcs->ta);
    if (rc != 0)
        return rc;

    RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_setenv(rpcs, "PATH", ta_path, TRUE) != 0)
        rc = RPC_ERRNO(rpcs);

    if (awaiting_error)
        RPC_AWAIT_IUT_ERROR(rpcs);

    free(ta_path);
    return rc;
}

/* See description in tapi_job.h */
te_errno
tapi_job_start(tapi_job_t *job)
{
    TAPI_JOB_CHECK_METHOD_SUPPORT(job, start);

    return job->methods.start(job);
}

/* See description in tapi_job.h */
te_errno
tapi_job_kill(tapi_job_t *job, int signo)
{
    TAPI_JOB_CHECK_METHOD_SUPPORT(job, kill);

    return job->methods.kill(job, signo);
}

/* See description in tapi_job.h */
te_errno
tapi_job_killpg(tapi_job_t *job, int signo)
{
    TAPI_JOB_CHECK_METHOD_SUPPORT(job, killpg);

    return job->methods.killpg(job, signo);
}

unsigned int
tapi_job_get_timeout(void)
{
    return TAPI_RPC_JOB_BIG_TIMEOUT_MS;
}

/* See description in tapi_job.h */
te_errno
tapi_job_wait(tapi_job_t *job, int timeout_ms, tapi_job_status_t *status)
{
    TAPI_JOB_CHECK_METHOD_SUPPORT(job, killpg);

    return job->methods.wait(job, timeout_ms, status);
}

te_bool
tapi_job_is_running(tapi_job_t *job)
{
    te_errno rc;

    TAPI_JOB_CHECK_METHOD_SUPPORT(job, wait);

    rc = job->methods.wait(job, 0, NULL);
    switch (rc)
    {
        case 0:
        case TE_ECHILD:
            return FALSE;
        case TE_EINPROGRESS:
            return TRUE;
        default:
            TEST_FAIL("Failed to check if a job is running");
            return FALSE;
    }
}

static void
add_channel_to_entry_list(tapi_job_channel_t *channel, channel_entry_list *list)
{
    channel_entry *entry;

    entry = tapi_malloc(sizeof(*entry));
    entry->channel = channel;

    SLIST_INSERT_HEAD(list, entry, next);
}

static channel_entry *
get_channel_entry(tapi_job_channel_t *channel, channel_entry_list *list)
{
    channel_entry *entry;

    SLIST_FOREACH(entry, list, next)
    {
        if (entry->channel == channel)
            return entry;
    }

    return NULL;
}

static void
remove_channel_entry_from_entry_list(channel_entry *entry,
                                     channel_entry_list *list)
{
    SLIST_REMOVE(list, entry, channel_entry, next);
    free(entry);
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

    TAPI_JOB_CHECK_METHOD_SUPPORT(job, allocate_channels);

    if (channels != NULL)
    {
        channel_ids = tapi_calloc(n_channels, sizeof(*channel_ids));
        channels_tmp = tapi_calloc(n_channels, sizeof(*channels_tmp));

        for (i = 0; i < n_channels; i++)
            channels_tmp[i] = tapi_malloc(sizeof(tapi_job_channel_t));
    }

    rc = job->methods.allocate_channels(job, input_channels, n_channels,
                                        channel_ids);
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
        init_primary_channel(job, tapi_job_get_rpcs(job), channel_ids[i],
                             channels_tmp[i]);
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

static void
destroy_filter_entry(channel_entry *filter_entry, channel_entry_list *list)
{
    if (--filter_entry->channel->ref_count <= 0)
        free(filter_entry->channel);

    remove_channel_entry_from_entry_list(filter_entry, list);
}

/*
 * Free resources occupied by an output primary channel and all filters
 * attached to it
 */
static void
destroy_channel(tapi_job_channel_t *channel)
{
    channel_entry *entry;
    channel_entry *entry_tmp;

    SLIST_FOREACH_SAFE(entry, &channel->filter_entries, next, entry_tmp)
    {
        channel_entry *job_filter_entry =
            get_channel_entry(entry->channel, &channel->job->channel_entries);

        /*
         * If the filter (entry->channel) is attached to the channel, it must be
         * found in the list of all job's channels
         */
        assert(job_filter_entry != NULL);

        remove_channel_entry_from_entry_list(entry, &channel->filter_entries);
        destroy_filter_entry(job_filter_entry, &channel->job->channel_entries);
    }
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
tapi_job_dealloc_channels(tapi_job_channel_set_t channels)
{
    te_errno rc;
    unsigned int i;
    unsigned int *channel_ids;
    unsigned int n_channels;
    rcf_rpc_server *rpcs;

    if ((rc = validate_channel_set(channels)) != 0)
        return rc;

    for (i = 0; channels[i] != NULL; i++)
    {
        if (!is_primary_channel(channels[i]))
        {
            ERROR("It is not allowed to deallocate filters, use "
                  "tapi_job_filter_remove_channels() instead");
            return TE_RC(TE_TAPI, TE_EPERM);
        }
    }

    rpcs = channels[0]->rpcs;

    alloc_id_array_from_channel_set(channels, &n_channels, &channel_ids);

    rc = rpc_job_deallocate_channels(rpcs, n_channels, channel_ids);
    if (rc != 0)
        return rc;

    for (i = 0; channels[i] != NULL; i++)
        destroy_channel(channels[i]);

    return 0;
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
            add_channel_to_entry_list(result, &channels[i]->filter_entries);
        }
        *filter = result;
    }

    return 0;
}

te_errno
tapi_job_attach_simple_filter(const tapi_job_simple_desc_t *desc,
                              tapi_job_simple_filter_t *filter)
{
    tapi_job_channel_t *channels[3] = {NULL};
    tapi_job_channel_t *result = NULL;
    unsigned int n_channels = 0;
    te_errno rc;

    if (*desc->job_loc == NULL)
    {
        ERROR("Attach simple filter failed: simple description is not associated with a job");
        return TE_ENOTCONN;
    }
    if (filter->use_stdout && desc->stdout_loc == NULL)
    {
        ERROR("Attach simple filter on stdout failed: no stdout channel");
        return TE_RC(TE_TAPI, TE_EPERM);
    }
    if (filter->use_stderr && desc->stderr_loc == NULL)
    {
        ERROR("Attach simple filter on stderr failed: no stderr channel");
        return TE_RC(TE_TAPI, TE_EPERM);
    }

    if (filter->use_stdout)
        channels[n_channels++] = *desc->stdout_loc;
    if (filter->use_stderr)
        channels[n_channels++] = *desc->stderr_loc;

    rc = tapi_job_attach_filter(channels, filter->filter_name,
                                filter->readable, filter->log_level, &result);
    if (rc != 0)
        return rc;

    if (filter->re != NULL)
    {
        rc = tapi_job_filter_add_regexp(result, filter->re, filter->extract);
        if (rc != 0)
            return rc;
    }

    if (filter->filter_var != NULL)
        *filter->filter_var = result;

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
tapi_job_filter_add_channels(tapi_job_channel_t *filter,
                             tapi_job_channel_set_t channels)
{
    te_errno rc;
    unsigned int *channel_ids;
    unsigned int n_channels;
    rcf_rpc_server *rpcs;
    unsigned int i;

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

    rc = rpc_job_filter_add_channels(rpcs, filter->id, n_channels, channel_ids);
    free(channel_ids);
    if (rc != 0)
        return rc;

    filter->ref_count += n_channels;
    for (i = 0; i < n_channels; i++)
    {
        add_channel_to_entry_list(filter, &channels[i]->job->channel_entries);
        add_channel_to_entry_list(filter, &channels[i]->filter_entries);
    }

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_filter_remove_channels(tapi_job_channel_t *filter,
                                tapi_job_channel_set_t channels)
{
    te_errno rc;
    unsigned int *channel_ids;
    unsigned int n_channels;
    rcf_rpc_server *rpcs;
    unsigned int i;

    if ((rc = validate_channel_set(channels)) != 0)
        return rc;

    rpcs = channels[0]->rpcs;

    alloc_id_array_from_channel_set(channels, &n_channels, &channel_ids);

    rc = rpc_job_filter_remove_channels(rpcs, filter->id, n_channels,
                                        channel_ids);
    free(channel_ids);
    if (rc != 0)
        return rc;

    for (i = 0; channels[i] != NULL; i++)
    {
        channel_entry *channel_filter_entry =
            get_channel_entry(filter, &channels[i]->filter_entries);
        channel_entry *job_filter_entry =
            get_channel_entry(filter, &channels[i]->job->channel_entries);

        if (channel_filter_entry != NULL)
        {
            assert(job_filter_entry != NULL);

            /*
             * Remove the filter from the list of filters attached to the
             * primary channel and from the list of all job's channels
             */
            remove_channel_entry_from_entry_list(channel_filter_entry,
                                                 &channels[i]->filter_entries);
            destroy_filter_entry(job_filter_entry,
                                 &channels[i]->job->channel_entries);
        }
    }

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_send(tapi_job_channel_t *channel, const te_string *str)
{
    return rpc_job_send(channel->rpcs, channel->id, str->ptr, str->size);
}

void
tapi_job_simple_send(tapi_job_channel_t *channel, const te_string *str)
{
    if (tapi_job_send(channel, str) != 0)
    {
        TEST_FAIL("Job simple send failed to send '%s' to channel with id %u",
                  str->ptr, channel->id);
    }
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

void
tapi_job_simple_poll(const tapi_job_channel_set_t wait_set, int timeout_ms)
{
    if (tapi_job_poll(wait_set, timeout_ms) != 0)
        TEST_FAIL("Job simple poll failed");
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

static te_errno
receive_common(const tapi_job_channel_set_t filters, int timeout_ms,
               tapi_job_buffer_t *buffer,
               te_errno (*rpc_job_receive_cb)(rcf_rpc_server *rpcs,
                                              unsigned int n_filters,
                                              unsigned int *filters,
                                              int timeout_ms,
                                              tarpc_job_buffer *buffer))
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

    rc = rpc_job_receive_cb(rpcs, n_channels, channel_ids, timeout_ms, &buf);
    free(channel_ids);
    if (rc != 0)
        return rc;

    tarpc_job_buffer2tapi_job_buffer(rpcs, &buf, buffer);
    free(buf.data.data_val);

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_receive(const tapi_job_channel_set_t filters, int timeout_ms,
                 tapi_job_buffer_t *buffer)
{
    return receive_common(filters, timeout_ms, buffer, rpc_job_receive);
}

te_errno
tapi_job_receive_last(const tapi_job_channel_set_t filters, int timeout_ms,
                      tapi_job_buffer_t *buffer)
{
    return receive_common(filters, timeout_ms, buffer, rpc_job_receive_last);
}

te_errno
tapi_job_receive_many(const tapi_job_channel_set_t filters, int timeout_ms,
                      tapi_job_buffer_t **buffers, unsigned int *count)
{
    unsigned int *channel_ids = NULL;
    unsigned int n_channels;
    te_errno rc;

    rcf_rpc_server *rpcs;
    tarpc_job_buffer *bufs = NULL;
    tapi_job_buffer_t *tapi_bufs = NULL;
    unsigned int bufs_count = *count;
    unsigned int i;

    *buffers = NULL;
    *count = 0;

    if ((rc = validate_channel_set(filters)) != 0)
        return rc;
    rpcs = filters[0]->rpcs;

    alloc_id_array_from_channel_set(filters, &n_channels, &channel_ids);

    rc = rpc_job_receive_many(rpcs, n_channels, channel_ids, timeout_ms,
                              &bufs, &bufs_count);
    free(channel_ids);

    if (bufs_count > 0)
    {
        tapi_bufs = TE_ALLOC(bufs_count * sizeof(*tapi_bufs));
        if (tapi_bufs == NULL)
        {
            if (rc == 0)
                rc = TE_RC(TE_TAPI, TE_ENOMEM);

            goto out;
        }

        for (i = 0; i < bufs_count; i++)
        {
            tapi_bufs[i] = (tapi_job_buffer_t)TAPI_JOB_BUFFER_INIT;
            tarpc_job_buffer2tapi_job_buffer(rpcs, &bufs[i], &tapi_bufs[i]);
        }
    }

    *buffers = tapi_bufs;
    *count = bufs_count;

out:

    tarpc_job_buffers_free(bufs, bufs_count);

    return rc;
}

void
tapi_job_buffers_free(tapi_job_buffer_t *buffers, unsigned int count)
{
    unsigned int i;

    if (buffers == NULL)
        return;

    for (i = 0; i < count; i++)
        te_string_free(&buffers[i].data);

    free(buffers);
}

te_bool
tapi_job_filters_have_data(const tapi_job_channel_set_t filters, int timeout_ms)
{
    te_errno rc;
    te_bool have_data = FALSE;
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;

    rc = tapi_job_receive_last(filters, timeout_ms, &buf);
    if (rc == 0 && !buf.eos)
        have_data = TRUE;

    te_string_free(&buf.data);

    return have_data;
}


void
tapi_job_simple_receive(const tapi_job_channel_set_t filters, int timeout_ms,
                        tapi_job_buffer_t *buffer)
{
    te_string_reset(&buffer->data);

    if (tapi_job_receive(filters, timeout_ms, buffer) != 0)
        TEST_FAIL("Job simple receive failed");
}

te_errno
tapi_job_receive_single(tapi_job_channel_t *filter, te_string *val,
                        int timeout_ms)
{
    te_errno rc;
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    te_bool matched = FALSE;

    while (1)
    {
        /*
         * There is no te_string_reset() since we fail anyway if there are
         * at least two successful receives.
         */
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter),
                              timeout_ms, &buf);
        if (rc != 0)
        {
            if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
                break;

            te_string_free(&buf.data);
            return rc;
        }

        if (buf.eos)
            break;

        if (matched)
        {
            ERROR("%s(): more than one message was read", __FUNCTION__);
            te_string_free(&buf.data);
            return TE_RC(TE_TAPI, TE_EPROTO);
        }

        matched = TRUE;
        *val = buf.data;
    }

    if (!matched)
    {
        ERROR("%s(): no data was received", __FUNCTION__);
        te_string_free(&buf.data);

        return TE_RC(TE_TAPI, TE_EPROTO);
    }

    return 0;
}

te_errno
tapi_job_clear(const tapi_job_channel_set_t filters)
{
    unsigned int *channel_ids;
    unsigned int n_channels;
    rcf_rpc_server *rpcs;

    te_errno rc;

    if ((rc = validate_channel_set(filters)) != 0)
        return rc;
    rpcs = filters[0]->rpcs;

    alloc_id_array_from_channel_set(filters, &n_channels, &channel_ids);

    rc = rpc_job_clear(rpcs, n_channels, channel_ids);
    free(channel_ids);

    return rc;
}

te_errno
tapi_job_stop(tapi_job_t *job, int signo, int term_timeout_ms)
{
    TAPI_JOB_CHECK_METHOD_SUPPORT(job, stop);

    return job->methods.stop(job, signo, term_timeout_ms);
}

/* See description in tapi_job.h */
te_errno
tapi_job_destroy(tapi_job_t *job, int term_timeout_ms)
{
    te_errno rc;
    channel_entry *entry;
    channel_entry *entry_tmp;

    TAPI_JOB_CHECK_METHOD_SUPPORT(job, destroy);

    if (job == NULL)
        return 0;

    rc = job->methods.destroy(job, term_timeout_ms);
    if (rc != 0)
        return rc;

    SLIST_FOREACH_SAFE(entry, &job->channel_entries, next, entry_tmp)
    {
        /* It also frees filters */
        if (is_primary_channel(entry->channel))
            destroy_channel(entry->channel);
    }

    SLIST_REMOVE(&all_jobs, job, tapi_job_t, next);
    free(job);

    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_wrapper_add(tapi_job_t *job, const char *tool, const char **argv,
                     tapi_job_wrapper_priority_t priority,
                     tapi_job_wrapper_t **wrap)
{
    te_errno rc;
    tapi_job_wrapper_t *wrapper;

    TAPI_JOB_CHECK_METHOD_SUPPORT(job, wrapper_add);

    wrapper = tapi_calloc(1, sizeof(*wrapper));
    rc = job->methods.wrapper_add(job, tool, argv, priority, &wrapper->id);
    if (rc != 0)
    {
        free(wrapper);
        return rc;
    }

    wrapper->job = job;

    *wrap = wrapper;
    return 0;
}

/* See description in tapi_job.h */
te_errno
tapi_job_wrapper_delete(tapi_job_wrapper_t *wrapper)
{
    te_errno rc;
    tapi_job_t *job = wrapper->job;

    TAPI_JOB_CHECK_METHOD_SUPPORT(job, wrapper_delete);

    if (wrapper == NULL)
        return 0;

    rc = job->methods.wrapper_delete(job, wrapper->id);
    if (rc != 0)
        return rc;

    free(wrapper);

    return 0;
}

te_errno
tapi_job_add_sched_param(tapi_job_t *job,
                         tapi_job_sched_param *sched_param)
{
    TAPI_JOB_CHECK_METHOD_SUPPORT(job, add_sched_param);

    return job->methods.add_sched_param(job, sched_param);
}
