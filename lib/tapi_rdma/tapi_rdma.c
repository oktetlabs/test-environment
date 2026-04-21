/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2024 Advanced Micro Devices, Inc. */
/** @file
 * @brief Generic Test API to interact with RDMA links
 *
 * Generic API to interact with RDMA links.
 */
#define TE_LGR_USER     "TAPI RDMA"

#include "te_config.h"
#include "te_alloc.h"
#include "te_string.h"
#include "te_vector.h"
#include "te_str.h"
#include "log_bufs.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_rdma.h"

#define RDMA_TOOL "rdma"

/* See description in tapi_rdma.h */
te_errno
tapi_rdma_link_get_stats(rcf_rpc_server *rpcs, const char *link,
                         tapi_rdma_link_stats_t **stats)
{
    te_errno            rc;
    tapi_job_factory_t *factory = NULL;
    te_vec              stat_vec = TE_VEC_INIT(tapi_rdma_link_stat_t);
    tapi_job_t         *job = NULL;
    tapi_job_channel_t *chan_out;
    tapi_job_channel_t *chan_err;
    tapi_job_channel_t *filter_names;
    tapi_job_channel_t *filter_values;
    tapi_job_buffer_t   buf_name = TAPI_JOB_BUFFER_INIT;
    tapi_job_buffer_t   buf_value = TAPI_JOB_BUFFER_INIT;
    tapi_job_status_t   status;

    const char *argv[] = {
        RDMA_TOOL,
        "statistic",
        "show",
        "link",
        link,
        NULL,
    };

    tapi_job_simple_desc_t job_desc = {
        .program = RDMA_TOOL,
        .argv = argv,
        .stdout_loc = &chan_out,
        .stderr_loc = &chan_err,
        .job_loc = &job,
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {
                .use_stderr = true,
                .filter_name = "Stderror",
                .log_level = TE_LL_ERROR,
            },
            {
                .use_stdout = true,
                .filter_name = "Statistics names",
                .re = "\\s(\\w+)\\s",
                .readable = true,
                .extract = 1,
                .filter_var = &filter_names,
            },
            {
                .use_stdout = true,
                .filter_name = "Statistics values",
                .re = "\\s(-?\\d+)\\b",
                .readable = true,
                .extract = 1,
                .filter_var = &filter_values,
            }
        ),
    };

    rc = tapi_job_factory_rpc_create(rpcs, &factory);
    if (rc != 0)
        goto out;

    rc = tapi_job_simple_create(factory, &job_desc);
    if (rc != 0)
        goto out;

    rc = tapi_job_start(job);
    if (rc != 0)
        goto out;

    for (;;) {
        tapi_rdma_link_stat_t stat;
        intmax_t value;

        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter_names), -1,
                              &buf_name);
        if (rc != 0)
        {
            ERROR("Error receiving RDMA statistic name: %r", rc);
            goto out;
        }
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter_values), -1,
                              &buf_value);
        if (rc != 0)
        {
            ERROR("Error receiving RDMA statistic value: %r", rc);
            goto out;
        }

        if (buf_name.eos || buf_name.eos)
            break;

        rc = te_strtoimax(buf_value.data.ptr, 0, &value);
        if (rc != 0)
        {
            ERROR("Error parsing RDMA statistic value '%s': %r",
                  buf_value.data.ptr, rc);
            goto out;
        }

        stat.name = strdup(buf_name.data.ptr);
        stat.value = value;

        te_string_reset(&buf_name.data);
        te_string_reset(&buf_value.data);

        TE_VEC_APPEND(&stat_vec, stat);
    }

    if (!buf_name.eos)
        WARN("RDMA statistics names are still readable");
    if (!buf_value.eos)
        WARN("RDMA statistics values are still readable");

    rc = tapi_job_wait(job, -1, &status);
    if (rc != 0)
        goto out;

    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
    {
        ERROR("RDMA utility finished abnormally");
        rc = TE_RC(TE_TAPI, TE_EFAIL);
        goto out;
    }

    *stats = TE_ALLOC(sizeof(tapi_rdma_link_stats_t));
    (*stats)->stats = (tapi_rdma_link_stat_t *)stat_vec.data.ptr;
    (*stats)->size = te_vec_size(&stat_vec);

out:
    te_string_free(&buf_name.data);
    te_string_free(&buf_value.data);
    tapi_job_destroy(job, -1);
    tapi_job_factory_destroy(factory);
    return rc;
}

/* See description in tapi_rdma.h */
te_errno
tapi_rdma_link_diff_stats(const tapi_rdma_link_stats_t *old_stats,
                          const tapi_rdma_link_stats_t *new_stats,
                          tapi_rdma_link_stats_t **diff_stats)
{
    te_vec diff = TE_VEC_INIT(tapi_rdma_link_stat_t);

    unsigned int i;
    unsigned int j;
    unsigned int matches = 0;

    /*
     * The set of statistics reported by the rdma utility is unlikely to change
     * between calls, but leave some warnings in case it actually happens at
     * some point in the future.
     */
    if (old_stats->size != new_stats->size)
    {
        WARN("%s: input arrays have different sizes: %u and %u",
             __FUNCTION__, old_stats->size, new_stats->size);
    }

    for (i = 0; i < old_stats->size; i++)
    {
        for (j = 0; j < new_stats->size; j++)
        {
            if (strcmp(new_stats->stats[j].name, old_stats->stats[i].name) == 0)
            {
                matches++;
                if (new_stats->stats[j].value != old_stats->stats[i].value)
                {
                    tapi_rdma_link_stat_t stat;

                    stat.name = strdup(new_stats->stats[j].name);
                    stat.value = new_stats->stats[j].value - old_stats->stats[i].value;

                    TE_VEC_APPEND(&diff, stat);
                }
            }
        }
    }

    *diff_stats = TE_ALLOC(sizeof(tapi_rdma_link_stats_t));
    (*diff_stats)->stats = (tapi_rdma_link_stat_t *)diff.data.ptr;
    (*diff_stats)->size = te_vec_size(&diff);

    if (matches != old_stats->size)
    {
        WARN("%s: only %u old stats have been found among %u new ones",
             __FUNCTION__, matches, old_stats->size);
    }

    return 0;
}

/* See description in tapi_rdma.h */
void
tapi_rdma_link_log_stats(tapi_rdma_link_stats_t *stats, const char *description,
                         const char *pattern, bool non_empty,
                         te_log_level log_level)
{
    unsigned int  i;
    te_log_buf   *buf;
    bool          stats_printed = false;

    if (stats == NULL)
    {
        ERROR("Tried to log NULL RDMA statistics");
        return;
    }

    buf = te_log_buf_alloc();

    te_log_buf_append(buf, "%s:\n", description);

    for (i = 0; i < stats->size; i++)
    {
        if (pattern == NULL || strstr(stats->stats[i].name, pattern) != NULL)
        {
            te_log_buf_append(buf, "  %s: %"PRIi64"\n", stats->stats[i].name,
                stats->stats[i].value);
            stats_printed = true;
        }
    }

    if (!stats_printed)
        te_log_buf_append(buf, "<none>");

    if (!non_empty || stats_printed)
        LOG_MSG(log_level, "%s", te_log_buf_get(buf));

    te_log_buf_free(buf);
}

/* See description in tapi_rdma.h */
void
tapi_rdma_link_free_stats(tapi_rdma_link_stats_t *stats)
{
    unsigned int i;

    if (stats == NULL)
        return;

    for (i = 0; i < stats->size; i++)
        free(stats->stats[i].name);

    free(stats->stats);
    free(stats);
}
