/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Generic Test API to run RDMA perf tests
 *
 * Generic API to run RDMA perf tests.
 */
#define TE_LGR_USER "TAPI RDMA perf"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_test_log.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_mem.h"
#include "tapi_rpc_misc.h"
#include "tapi_test.h"
#include "te_str.h"
#include "tapi_rdma_perf.h"
#include "te_sockaddr.h"
#include "te_vector.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"

/** Default values for common options of RDMA perf. */
const tapi_rdma_perf_common_opts tapi_rdma_perf_cmn_opts_def = {
    .port                              = TAPI_JOB_OPT_UINT_UNDEF,
    .mtu                               = TAPI_JOB_OPT_UINT_UNDEF,
    .conn_type                         = TAPI_JOB_OPT_ENUM_UNDEF,
    .ib_dev                            = NULL,
    .ib_port                           = TAPI_JOB_OPT_UINT_UNDEF,
    .gid_idx                           = TAPI_JOB_OPT_UINT_UNDEF,
    .use_rocm                          = TAPI_JOB_OPT_UINT_UNDEF,
    .msg_size                          = TAPI_JOB_OPT_UINTMAX_UNDEF,
    .iter_num                          = TAPI_JOB_OPT_UINTMAX_UNDEF,
    .rx_depth                          = TAPI_JOB_OPT_UINT_UNDEF,
    .duration_s                        = TAPI_JOB_OPT_UINT_UNDEF,
    .all_sizes                         = false,
};

/** Default values for options of latency RDMA perf tests. */
const tapi_rdma_perf_lat_opts tapi_rdma_perf_lat_opts_def = {
    .report_cycles                     = false,
    .report_histogram                  = false,
    .report_unsorted                   = false,
};

/** Default values for options of BW RDMA perf tests. */
const tapi_rdma_perf_bw_opts tapi_rdma_perf_bw_opts_def = {
    .bi_dir                            = false,
    .tx_depth                          = TAPI_JOB_OPT_UINT_UNDEF,
    .dualport                          = false,
    .duration_s                        = TAPI_JOB_OPT_UINT_UNDEF,
    .qp_num                            = TAPI_JOB_OPT_UINT_UNDEF,
    .cq_mod                            = TAPI_JOB_OPT_UINT_UNDEF,
};

/** Default values for options of RDMA perf tests with SEND transactions. */
const tapi_rdma_perf_send_opts tapi_rdma_perf_send_opts_def = {
    .rx_depth                          = TAPI_JOB_OPT_UINT_UNDEF,
    .mcast_qps_num                     = TAPI_JOB_OPT_UINT_UNDEF,
    .mcast_gid                         = TAPI_JOB_OPT_UINT_UNDEF,
};

/** Default values for options of RDMA perf tests with WRITE transactions. */
const tapi_rdma_perf_write_opts tapi_rdma_perf_write_opts_def = {
    .write_with_imm                    = false,
};

/** Default values for options of RDMA perf tests with READ transactions. */
const tapi_rdma_perf_read_opts tapi_rdma_perf_read_opts_def = {
    .outs_num                          = TAPI_JOB_OPT_UINT_UNDEF,
};

/** Default values for options of RDMA perf tests with ATOMIC transactions. */
const tapi_rdma_perf_atomic_opts tapi_rdma_perf_atomic_opts_def = {
    .type                              = TAPI_JOB_OPT_ENUM_UNDEF,
    .outs_num                          = TAPI_JOB_OPT_UINT_UNDEF,
};

/* See description in tapi_rdma_perf.h */
void
tapi_rdma_perf_app_destroy(tapi_rdma_perf_app *app)
{
    te_errno rc;

    if (app == NULL)
        return;

    rc = tapi_job_destroy(app->job, -1);
    if (rc != 0)
    {
        ERROR("Failed to destroy RDMA %s application, errno=%r",
              app->name, rc);
    }

    tapi_job_factory_destroy(app->factory);
    free(app);
}

/** Mapping of possible values for RDMA perf connection type. */
static const te_enum_map tapi_perftet_conn_map[] = {
    {.name = "RC",  .value = TAPI_RDMA_PERF_CONN_RC},
    {.name = "UC",  .value = TAPI_RDMA_PERF_CONN_UC},
    {.name = "UD",  .value = TAPI_RDMA_PERF_CONN_UD},
    {.name = "XRC", .value = TAPI_RDMA_PERF_CONN_XRC},
    {.name = "DC",  .value = TAPI_RDMA_PERF_CONN_DC},
    {.name = "SRD", .value = TAPI_RDMA_PERF_CONN_SRD},
    TE_ENUM_MAP_END
};

/** Mapping of possible values for RDMA perf atomic transaction types. */
static const te_enum_map tapi_perftet_atomic_type_map[] = {
    {.name = "CMP_AND_SWAP",
      .value = TAPI_RDMA_PERF_ATOMIC_CMP_AND_SWAP},
    {.name = "FETCH_AND_ADD",
     .value = TAPI_RDMA_PERF_ATOMIC_FETCH_AND_ADD},
    TE_ENUM_MAP_END
};

/* See description in tapi_rdma_perf.h */
const char *
tapi_rdma_perf_conn_str_get(tapi_rdma_perf_conn_type_t conn_type)
{
    return te_enum_map_from_value(tapi_perftet_conn_map, conn_type);
}

/**
 * Build command line arguments to run RDMA perf app.
 *
 * @param[in]  path       Program location.
 * @param[in]  opts       RDMA perf options.
 * @param[in]  is_client  Whether perftest is running in a client mode.
 * @param[out] argv       Vector with command line arguments.
 *
 * @return Status code.
 */
static te_errno
build_argv(const char *path, const tapi_rdma_perf_opts *opts,
           bool is_client, te_vec *argv)
{
    te_errno rc = 0;
    const tapi_job_opt_bind common_opt_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_UINT_T("--port=", true, NULL,
                            tapi_rdma_perf_common_opts, port),
        TAPI_JOB_OPT_UINT_T("--mtu=", true, NULL,
                            tapi_rdma_perf_common_opts, mtu),
        TAPI_JOB_OPT_ENUM("--connection=", true,
                          tapi_rdma_perf_common_opts, conn_type,
                          tapi_perftet_conn_map),
        TAPI_JOB_OPT_STRING("--ib-dev=", true,
                            tapi_rdma_perf_common_opts, ib_dev),
        TAPI_JOB_OPT_UINT_T("--ib-port=", true, NULL,
                            tapi_rdma_perf_common_opts, ib_port),
        TAPI_JOB_OPT_UINT_T("--gid-index=", true, NULL,
                            tapi_rdma_perf_common_opts, gid_idx),
        TAPI_JOB_OPT_UINT_T("--use_rocm=", true, NULL,
                            tapi_rdma_perf_common_opts, use_rocm),
        TAPI_JOB_OPT_UINTMAX_T("--size=", true, NULL,
                               tapi_rdma_perf_common_opts, msg_size),
        TAPI_JOB_OPT_UINTMAX_T("--iters=", true, NULL,
                               tapi_rdma_perf_common_opts, iter_num),
        TAPI_JOB_OPT_UINT_T("--rx-depth=", true, NULL,
                            tapi_rdma_perf_common_opts, rx_depth),
        TAPI_JOB_OPT_UINT_T("--duration=", true, NULL,
                            tapi_rdma_perf_common_opts, duration_s),
        TAPI_JOB_OPT_BOOL("--all", tapi_rdma_perf_common_opts, all_sizes)
    );

    if (is_client && opts->server_ip == NULL)
        return TE_EINVAL;

    rc = tapi_job_opt_build_args(path, common_opt_binds, &opts->common, argv);
    if (rc != 0)
        return rc;

    switch (opts->tst_type)
    {
        case TAPI_RDMA_PERF_TEST_LAT:
        {
            const tapi_job_opt_bind lat_opt_binds[] = TAPI_JOB_OPT_SET(
                TAPI_JOB_OPT_BOOL("--report-cycles",
                                  tapi_rdma_perf_lat_opts,
                                  report_cycles),
                TAPI_JOB_OPT_BOOL("--report-histogram",
                                  tapi_rdma_perf_lat_opts,
                                  report_histogram),
                TAPI_JOB_OPT_BOOL("--report-unsorted",
                                  tapi_rdma_perf_lat_opts,
                                  report_unsorted)
            );

            rc = tapi_job_opt_append_args(lat_opt_binds, &opts->lat, argv);
            break;
        }
        case TAPI_RDMA_PERF_TEST_BW:
        {
            const tapi_job_opt_bind bw_opt_binds[] = TAPI_JOB_OPT_SET(
                TAPI_JOB_OPT_BOOL("--bidirectional", tapi_rdma_perf_bw_opts,
                                  bi_dir),
                TAPI_JOB_OPT_UINT_T("--tx-depth=", true, NULL,
                                    tapi_rdma_perf_bw_opts, tx_depth),
                TAPI_JOB_OPT_BOOL("--dualport",
                                  tapi_rdma_perf_bw_opts, dualport),
                TAPI_JOB_OPT_UINT_T("--duration=", true, NULL,
                                    tapi_rdma_perf_bw_opts, duration_s),
                TAPI_JOB_OPT_UINT_T("--qp=", true, NULL,
                                    tapi_rdma_perf_bw_opts, qp_num),
                TAPI_JOB_OPT_UINT_T("--cq-mod=", true, NULL,
                                    tapi_rdma_perf_bw_opts, cq_mod)
            );

            rc = tapi_job_opt_append_args(bw_opt_binds, &opts->bw, argv);
            break;
        }

        default:
            rc = TE_EINVAL;
            break;
    }

    if (rc != 0)
        return rc;

    switch (opts->op_type)
    {
        case TAPI_RDMA_PERF_OP_SEND:
        {
            const tapi_job_opt_bind send_opt_binds[] = TAPI_JOB_OPT_SET(
                TAPI_JOB_OPT_UINT_T("--rx-depth=", true, NULL,
                                    tapi_rdma_perf_send_opts, rx_depth),
                TAPI_JOB_OPT_UINT_T("--mcg=", true, NULL,
                                    tapi_rdma_perf_send_opts, mcast_qps_num),
                TAPI_JOB_OPT_UINT_T("--MGID=", true, NULL,
                                    tapi_rdma_perf_send_opts, mcast_gid)
            );

            rc = tapi_job_opt_append_args(send_opt_binds, &opts->send, argv);
            break;
        }
        case TAPI_RDMA_PERF_OP_WRITE_IMM:
            /*@fallthrough@*/
        case TAPI_RDMA_PERF_OP_WRITE:
        {
            const tapi_job_opt_bind write_opt_binds[] = TAPI_JOB_OPT_SET(
                TAPI_JOB_OPT_BOOL("--write_with_imm",
                                  tapi_rdma_perf_write_opts, write_with_imm)
            );

            rc = tapi_job_opt_append_args(write_opt_binds, &opts->write, argv);
            break;
        }
        case TAPI_RDMA_PERF_OP_READ:
        {
            const tapi_job_opt_bind read_opt_binds[] = TAPI_JOB_OPT_SET(
                TAPI_JOB_OPT_UINT_T("--outs=", true, NULL,
                                    tapi_rdma_perf_read_opts, outs_num)
            );

            rc = tapi_job_opt_append_args(read_opt_binds, &opts->read, argv);
            break;
        }
        case TAPI_RDMA_PERF_OP_ATOMIC:
        {
            const tapi_job_opt_bind atomic_opt_binds[] = TAPI_JOB_OPT_SET(
                TAPI_JOB_OPT_ENUM("--atomic_type=", true,
                                  tapi_rdma_perf_atomic_opts, type,
                                  tapi_perftet_atomic_type_map)
            );

            rc = tapi_job_opt_append_args(atomic_opt_binds, &opts->atomic,
                                          argv);
            break;
        }

        default:
            rc = TE_EINVAL;
            break;
    }

    if (rc != 0)
        return rc;

    if (is_client)
    {
        const tapi_job_opt_bind client_opt_binds[] = TAPI_JOB_OPT_SET(
            TAPI_JOB_OPT_SOCKADDR_PTR("", true, tapi_rdma_perf_opts,
                                      server_ip)
        );

        rc = tapi_job_opt_append_args(client_opt_binds, opts, argv);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/* See description in tapi_rdma_perf.h */
te_errno
tapi_rdma_perf_app_start(tapi_rdma_perf_app *app)
{
    return tapi_job_start(app->job);
}

/* See description in tapi_rdma_perf.h */
te_errno
tapi_rdma_perf_app_wait(tapi_rdma_perf_app *app, int timeout_s)
{
    tapi_job_status_t   status = {0};
    te_errno            rc = 0;

    rc = tapi_job_wait(app->job, TE_SEC2MS(timeout_s), &status);
    if (rc != 0)
        return rc;

    if (status.type == TAPI_JOB_STATUS_SIGNALED)
    {
        WARN("RDMA %s app was terminated by a signal", app->name);
        return 0;
    }
    else if (status.type == TAPI_JOB_STATUS_UNKNOWN)
    {
        ERROR("RDMA %s app terminated by unknown reason", app->name);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    else if (status.value != 0)
    {
        ERROR("RDMA %s app failed with return code %d", app->name,
              status.value);
        return TE_RC(TE_TAPI, TE_ESHCMD);
    }

    return 0;
}

/* Mapping of RDMA perf transaction types to its string representation. */
static const te_enum_map op_type_map[] = {
    {.name = "send",   .value = TAPI_RDMA_PERF_OP_SEND},
    {.name = "write",  .value = TAPI_RDMA_PERF_OP_WRITE_IMM},
    {.name = "write",  .value = TAPI_RDMA_PERF_OP_WRITE},
    {.name = "read",   .value = TAPI_RDMA_PERF_OP_READ},
    {.name = "atomic", .value = TAPI_RDMA_PERF_OP_ATOMIC},
    TE_ENUM_MAP_END
};

/* Mapping of RDMA perf types to its string representation. */
static const te_enum_map test_type_map[] = {
    {.name = "lat", .value = TAPI_RDMA_PERF_TEST_LAT},
    {.name = "bw", .value = TAPI_RDMA_PERF_TEST_BW},
    TE_ENUM_MAP_END
};

/* See description in tapi_rdma_perf.h */
te_errno
tapi_rdma_perf_app_init(tapi_job_factory_t *factory,
                        tapi_rdma_perf_opts *opts,
                        bool is_client,
                        tapi_rdma_perf_app **app)
{
    te_errno                rc = 0;
    tapi_job_simple_desc_t  job_descr;
    tapi_rdma_perf_app     *handle = NULL;
    te_vec                  argv;

    if (factory == NULL || opts == NULL || app == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    handle = TE_ALLOC(sizeof(*handle));
    handle->factory = factory;

    TE_SPRINTF(handle->name, "ib_%s_%s",
               te_enum_map_from_value(op_type_map, opts->op_type),
               te_enum_map_from_value(test_type_map, opts->tst_type));

    if (opts->op_type == TAPI_RDMA_PERF_OP_WRITE_IMM &&
        !opts->write.write_with_imm)
    {
        opts->write.write_with_imm = true;
    }

    rc = build_argv(handle->name, opts, is_client, &argv);
    if (rc != 0)
    {
        ERROR("Failed to initialize RDMA perf app options: %r", rc);
        free(handle);
        return TE_RC(TE_TAPI, rc);
    }

    job_descr.spawner = NULL;
    job_descr.name = NULL;
    job_descr.program = TE_VEC_GET(char *, &argv, 0);
    job_descr.argv = (const char **)argv.data.ptr;
    job_descr.env = NULL;
    job_descr.job_loc = &handle->job;
    job_descr.stdin_loc = NULL;
    job_descr.stdout_loc = &handle->out_chs[0];
    job_descr.stderr_loc = &handle->out_chs[1];
    job_descr.filters = TAPI_JOB_SIMPLE_FILTERS(
        {
            .use_stderr = true,
            .log_level = TE_LL_ERROR,
            .readable = true,
            .filter_name = is_client ? "perf_client_stderr" :
                                       "perf_server_stderr",
        },
        {
            .use_stdout = true,
            .log_level = TE_LL_RING,
            .readable = false,
            .filter_name = is_client ? "perf_client_stdout" :
                                       "perf_server_stdout",
        },
        {
            .use_stdout = true,
            .readable = true,
            .re = "local address: LID .+? QPN (0x\\w+) PSN .+?$",
            .extract = 1,
            .filter_var = &handle->qp,
        }
    );

    rc = tapi_job_simple_create(handle->factory, &job_descr);
    if (rc == 0)
    {
        *app = handle;
    }
    else
    {
        ERROR("Initialization of RDMA %s app job context failed: %r",
              handle->name, rc);
        free(handle);
    }

    te_vec_deep_free(&argv);

    return rc;
}
