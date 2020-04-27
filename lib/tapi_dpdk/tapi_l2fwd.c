/** @file
 * @brief Test API for l2fwd
 *
 * Test API for DPDK l2fwd helper functions
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 *
 * @author Georgiy Levashov <Georgiy.Levashov@oktetlabs.ru>
 */

#include "te_config.h"

#include <stdio.h>

#include "tapi_dpdk.h"
#include "tapi_l2fwd.h"
#include "tapi_mem.h"
#include "tapi_test.h"
#include "tapi_rpc_rte_eal.h"
#include "tapi_file.h"
#include "te_ethernet.h"
#include "tapi_job_factory_rpc.h"

te_errno
tapi_dpdk_l2fwd_start(tapi_dpdk_l2fwd_job_t *l2fwd_job)
{
    return tapi_job_start(l2fwd_job->job);
}

te_errno
tapi_dpdk_l2fwd_destroy(tapi_dpdk_l2fwd_job_t *l2fwd_job)
{
    te_errno rc;

    if (l2fwd_job == NULL)
        return 0;

    rc = tapi_job_destroy(l2fwd_job->job, TAPI_DPDK_L2FWD_TERM_TIMEOUT_MS);
    if (rc != 0)
        return rc;

    free(l2fwd_job->ta);
    return 0;
}

te_errno
tapi_dpdk_l2fwd_get_stats(tapi_dpdk_l2fwd_job_t *l2fwd_job,
                          te_meas_stats_t *tx,
                          te_meas_stats_t *rx)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    unsigned long tx_pkts = 0;
    unsigned long prev_tx_pkts = 0;
    unsigned long tx_pps = 0;
    unsigned long rx_pkts = 0;
    unsigned long prev_rx_pkts = 0;
    unsigned long rx_pps = 0;
    te_errno rc = 0;

    do {
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(l2fwd_job->packets_sent,
                                                   l2fwd_job->packets_received,
                                                   l2fwd_job->err_filter),
                              TAPI_DPDK_L2FWD_RECEIVE_TIMEOUT_MS, &buf);
        if (rc != 0)
            goto out;

        if (buf.eos)
            break;

        if (buf.filter == l2fwd_job->packets_sent)
        {
            if ((rc = te_strtoul(buf.data.ptr, 0, &tx_pkts)) != 0)
                goto out;

            tx_pps = tx_pkts - prev_tx_pkts;
            prev_tx_pkts = tx_pkts;

            if (tx != NULL &&
                te_meas_stats_update(tx, (double)tx_pps) ==
                TE_MEAS_STATS_UPDATE_NOMEM)
            {
                rc = TE_ENOMEM;
                goto out;
            }
        }
        else if (buf.filter == l2fwd_job->packets_received)
        {
            if ((rc = te_strtoul(buf.data.ptr, 0, &rx_pkts)) != 0)
                goto out;

            rx_pps = rx_pkts - prev_rx_pkts;
            prev_rx_pkts = rx_pkts;

            if (rx != NULL &&
                te_meas_stats_update(rx, (double)rx_pps) ==
                TE_MEAS_STATS_UPDATE_NOMEM)
            {
                rc = TE_ENOMEM;
                goto out;
            }
        }
        else if (buf.filter == l2fwd_job->err_filter)
        {
            WARN("Error message: %s", buf.data.ptr);
        }
        else
        {
            ERROR("Received buf from a job contains invalid filter pointer");
        }

        if (buf.dropped > 0)
            WARN("Dropped messages count: %lu", buf.dropped);

        te_string_reset(&buf.data);
    } while (TE_MEAS_STATS_CONTINUE(tx) ||
             TE_MEAS_STATS_CONTINUE(rx));

    if (TE_MEAS_STATS_CONTINUE(tx) ||
        TE_MEAS_STATS_CONTINUE(rx))
    {
        ERROR("Channel had been closed before required number of stats were received");
        rc = TE_RC(TE_TAPI, TE_EFAIL);
        goto out;
    }

out:
    te_string_free(&buf.data);

    return rc;
}

static void
append_default_l2fwd_args(int *l2fwd_argc, char ***l2fwd_argv)
{
    /* Use first port as a default port for l2fwd */
    tapi_dpdk_append_argument("-p", l2fwd_argc, l2fwd_argv);
    tapi_dpdk_append_argument("1", l2fwd_argc, l2fwd_argv);

    /* Set stats updating period to 1 second */
    tapi_dpdk_append_argument("-T", l2fwd_argc, l2fwd_argv);
    tapi_dpdk_append_argument("1", l2fwd_argc, l2fwd_argv);
}

te_errno
tapi_dpdk_create_l2fwd_job(rcf_rpc_server *rpcs, tapi_env *env,
                           size_t n_fwd_cpus, const tapi_cpu_prop_t *prop,
                           te_kvpair_h *test_args,
                           tapi_dpdk_l2fwd_job_t *l2fwd_job)
{
    te_string l2fwd_path = TE_STRING_INIT;
    tapi_cpu_index_t *cpu_ids = NULL;
    int l2fwd_argc = 0;
    char **l2fwd_argv = NULL;
    char *working_dir = NULL;
    const char *vdev_arg = NULL;
    unsigned int port_number = 0;
    te_errno rc = 0;
    size_t n_cpus = n_fwd_cpus;
    size_t n_cpus_grabbed;
    tapi_job_factory_t *factory = NULL;
    int i;

    if (n_fwd_cpus == 0)
    {
        ERROR("L2FWD cannot be run with 0 forwarding cores");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    cpu_ids = tapi_calloc(n_cpus, sizeof(*cpu_ids));
    rc = tapi_dpdk_grab_cpus_nonstrict_prop(rpcs->ta, n_cpus, 1,
                                            prop, &n_cpus_grabbed,
                                            cpu_ids);
    if (rc != 0)
        goto out;

    rc = cfg_get_instance_fmt(NULL, &working_dir, "/agent:%s/dir:", rpcs->ta);
    if (rc != 0)
    {
        ERROR("Failed to get working directory");
        goto out;
    }

    CHECK_RC(te_string_append(&l2fwd_path, "%sdpdk-l2fwd", working_dir));

    rc = tapi_dpdk_build_eal_arguments(rpcs, env, n_cpus_grabbed, cpu_ids,
                                       l2fwd_path.ptr, &l2fwd_argc,
                                       &l2fwd_argv);
    if (rc != 0)
        goto out;

    vdev_arg = tapi_dpdk_get_vdev_eal_argument(l2fwd_argc, l2fwd_argv);
    if (vdev_arg != NULL)
    {
        if ((rc = tapi_dpdk_get_vdev_port_number(vdev_arg, &port_number)) != 0)
            goto out;
    }

    /* Separate EAL arguments from l2fwd arguments */
    tapi_dpdk_append_argument("--", &l2fwd_argc, &l2fwd_argv);
    append_default_l2fwd_args(&l2fwd_argc, &l2fwd_argv);

    /* Terminate argv with NULL */
    tapi_dpdk_append_argument(NULL, &l2fwd_argc, &l2fwd_argv);

    rc = tapi_job_factory_rpc_create(rpcs, &factory);
    if (rc != 0)
    {
        ERROR("Failed to create factory for l2fwd job");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                          &(tapi_job_simple_desc_t){
                                .program = l2fwd_path.ptr,
                                .argv = (const char **)l2fwd_argv,
                                .job_loc = &l2fwd_job->job,
                                .stdin_loc = &l2fwd_job->in_channel,
                                .stdout_loc = &l2fwd_job->out_channels[0],
                                .stderr_loc = &l2fwd_job->out_channels[1],
                                .filters = TAPI_JOB_SIMPLE_FILTERS(
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "Packets sent:\\s*([0-9]+)",
                                     .extract = 1,
                                     .filter_var = &l2fwd_job->packets_sent,
                                    },
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "Packets received:\\s*([0-9]+)",
                                     .extract = 1,
                                     .filter_var = &l2fwd_job->packets_received,
                                    },
                                    {.use_stderr = TRUE,
                                     .log_level = TE_LL_ERROR,
                                     .readable = TRUE,
                                     .filter_var = &l2fwd_job->err_filter,
                                     .filter_name = "err",
                                    },
                                    {.use_stdout = TRUE,
                                     .log_level = TE_LL_RING,
                                     .readable = FALSE,
                                     .filter_name = "out",
                                    }
                                 )
                          });
    if (rc != 0)
        goto out;

    l2fwd_job->ta = tapi_strdup(rpcs->ta);
    l2fwd_job->port_number = port_number;

out:
    free(cpu_ids);
    free(working_dir);

    for (i = 0; i < l2fwd_argc; i++)
        free(l2fwd_argv[i]);
    free(l2fwd_argv);
    te_string_free(&l2fwd_path);
    tapi_job_factory_destroy(factory);

    return rc;
}

