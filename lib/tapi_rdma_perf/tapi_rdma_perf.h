/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Generic Test API for RDMA perf tests
 *
 * @defgroup tapi_rdma_perf Test API to run RDMA perf tests
 * @ingroup te_ts_tapi
 * @{
 *
 * Generic API to run RDMA perf tests.
 */

#ifndef __TAPI_RDMA_PERF_H__
#define __TAPI_RDMA_PERF_H__

#include "te_errno.h"
#include "rcf_rpc.h"
#include "te_string.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of RDMA test name */
#define TAPI_RDMA_PERF_APP_NAME_LEN 32

/** RDMA transaction type to test. */
typedef enum {
    TAPI_RDMA_PERF_OP_SEND,
    TAPI_RDMA_PERF_OP_WRITE,
    TAPI_RDMA_PERF_OP_WRITE_IMM,
    TAPI_RDMA_PERF_OP_READ,
    TAPI_RDMA_PERF_OP_ATOMIC,
} tapi_rdma_perf_op_type_t;

/** RDMA test type. */
typedef enum {
    TAPI_RDMA_PERF_TEST_LAT,
    TAPI_RDMA_PERF_TEST_BW,
} tapi_rdma_perf_test_type_t;

/** RDMA connection type. */
typedef enum {
    TAPI_RDMA_PERF_CONN_RC,
    TAPI_RDMA_PERF_CONN_UC,
    TAPI_RDMA_PERF_CONN_UD,
    TAPI_RDMA_PERF_CONN_XRC,
    TAPI_RDMA_PERF_CONN_DC,
    TAPI_RDMA_PERF_CONN_SRD,
} tapi_rdma_perf_conn_type_t;

/** RDMA atomic operations type. */
typedef enum {
    TAPI_RDMA_PERF_ATOMIC_CMP_AND_SWAP,
    TAPI_RDMA_PERF_ATOMIC_FETCH_AND_ADD,
} tapi_rdma_perf_atomic_type;

/** Common options to all tests. */
typedef struct tapi_rdma_perf_common_opts {
    tapi_job_opt_uint_t         port;       /**< Listen on/connect to port. */
    tapi_job_opt_uint_t         mtu;        /**< QP MTU size. */
    tapi_rdma_perf_conn_type_t  conn_type;  /**< Connection type. */
    const char                 *ib_dev;     /**< IB device name. */
    tapi_job_opt_uint_t         ib_port;    /**< IB device network port. */
    tapi_job_opt_uint_t         gid_idx;    /**< IB GID index. */
    tapi_job_opt_uint_t         use_rocm;   /**< GPU index. */
    te_optional_uintmax_t       msg_size;   /**< Size of message to
                                                 exchange. */
    te_optional_uintmax_t       iter_num;   /**< Number of exchanges. */
    tapi_job_opt_uint_t         rx_depth;   /**< Receive queue depth. */
    tapi_job_opt_uint_t         duration_s; /**< Test duration, seconds. */
    bool                        all_sizes;  /**< Run sizes from 2 till 2^23. */
} tapi_rdma_perf_common_opts;

/** Options for latency tests. */
typedef struct tapi_rdma_perf_lat_opts {
    bool report_cycles;    /**< Report times in CPU cycle units. */
    bool report_histogram; /**< Print out all results. */
    bool report_unsorted;  /**< Print out unsorted results. */
} tapi_rdma_perf_lat_opts;

/** Options for bandwidth tests. */
typedef struct tapi_rdma_perf_bw_opts {
    bool             bi_dir;        /**< Measure bidirectional bandwidth. */
    tapi_job_opt_uint_t tx_depth;   /**< Size of Tx queue. */
    bool             dualport;      /**< Use dual-port mode. */
    tapi_job_opt_uint_t duration_s; /**< Test duration, seconds. */
    tapi_job_opt_uint_t qp_num;     /**< Num of QPs running in the process. */
    tapi_job_opt_uint_t cq_mod;     /**< Completion num after which Cqe will
                                         be generated */
} tapi_rdma_perf_bw_opts;

/** Options for test with SEND transactions. */
typedef struct tapi_rdma_perf_send_opts {
    tapi_job_opt_uint_t rx_depth;      /**< Size of receive queue. */
    tapi_job_opt_uint_t mcast_qps_num; /**< Send messages to multicast group
                                            with qps number attached to it. */
    tapi_job_opt_uint_t mcast_gid;     /**< Group MGID for multicast case. */
} tapi_rdma_perf_send_opts;

/** Options for test with WRITE transactions. */
typedef struct tapi_rdma_perf_write_opts {
    bool write_with_imm; /**< Use WRITE-WITH-IMMEDIATE verb
                              instead of WRITE. */
} tapi_rdma_perf_write_opts;

/** Options for test with READ transactions. */
typedef struct tapi_rdma_perf_read_opts {
    tapi_job_opt_uint_t outs_num; /**< Number of outstanding requests. */
} tapi_rdma_perf_read_opts;

/** Options for test with atomic transactions. */
typedef struct tapi_rdma_perf_atomic_opts {
    tapi_rdma_perf_atomic_type type;     /**< Type of atomic operation. */
    tapi_job_opt_uint_t        outs_num; /**< Number of outstanding requests. */
} tapi_rdma_perf_atomic_opts;

/** RDMA perf tool options */
typedef struct tapi_rdma_perf_opts {
    tapi_rdma_perf_common_opts      common;    /**< Common options to
                                                    all tests. */
    union {
        tapi_rdma_perf_lat_opts     lat;       /**< Options for latency
                                                    tests. */
        tapi_rdma_perf_bw_opts      bw;        /**< Options for BW tests. */
    };
    union {
        tapi_rdma_perf_send_opts    send;      /**< SEND-specific options. */
        tapi_rdma_perf_write_opts   write;     /**< WRITE-specific options. */
        tapi_rdma_perf_read_opts    read;      /**< READ-specific options. */
        tapi_rdma_perf_atomic_opts  atomic;    /**< ATOMIC-specific options. */
    };
    const struct sockaddr          *server_ip; /**< Server IP address. */
    tapi_rdma_perf_test_type_t      tst_type;  /**< Test latency or BW. */
    tapi_rdma_perf_op_type_t        op_type;   /**< Type of RDMA operation
                                                    to test. */
} tapi_rdma_perf_opts;

/** Default values for common options of RDMA perf. */
extern const tapi_rdma_perf_common_opts tapi_rdma_perf_cmn_opts_def;

/** Default values for options of latency RDMA perf tests. */
extern const tapi_rdma_perf_lat_opts tapi_rdma_perf_lat_opts_def;

/** Default values for options of BW RDMA perf tests. */
extern const tapi_rdma_perf_bw_opts tapi_rdma_perf_bw_opts_def;

/** Default values for options of RDMA perf tests with SEND transactions. */
extern const tapi_rdma_perf_send_opts tapi_rdma_perf_send_opts_def;

/** Default values for options of RDMA perf tests with WRITE transactions. */
extern const tapi_rdma_perf_write_opts tapi_rdma_perf_write_opts_def;

/** Default values for options of RDMA perf tests with READ transactions. */
extern const tapi_rdma_perf_read_opts tapi_rdma_perf_read_opts_def;

/** Default values for options of RDMA perf tests with ATOMIC transactions. */
extern const tapi_rdma_perf_atomic_opts tapi_rdma_perf_atomic_opts_def;

/** RDMA perf context */
struct tapi_rdma_perf_app {
    /** Job instance. */
    tapi_job_t                    *job;
    /** Standart output channels. */
    tapi_job_channel_t            *out_chs[2];
    /** Factory used for the app intance. */
    tapi_job_factory_t            *factory;
    /** Name of the test application. */
    char                           name[TAPI_RDMA_PERF_APP_NAME_LEN];
    /** Arguments that are used when running the tool. */
    te_vec                         args;
    /** Channel for Queue Pair Number retrieval. */
    tapi_job_channel_t            *qp;
};
typedef struct tapi_rdma_perf_app tapi_rdma_perf_app;

/**
 * Initiate test options for RDMA perf app.
 *
 * @param[in]  opts       RDMA perf options.
 * @param[in]  tst_type   Type of RDMA perf.
 * @param[in]  op_type    Type of RDMA operation to test.
 * @param[in]  server_ip  Server IP address (requied if @p is_client is @c true).
 *
 * @return Status code.
 */
static inline te_errno
tapi_rdma_perf_def_opts_init(tapi_rdma_perf_opts *opts,
                             tapi_rdma_perf_test_type_t tst_type,
                             tapi_rdma_perf_op_type_t op_type,
                             const struct sockaddr *server_ip)
{
    if (opts == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    opts->common = tapi_rdma_perf_cmn_opts_def;

    switch (tst_type)
    {
        case TAPI_RDMA_PERF_TEST_LAT:
            opts->lat = tapi_rdma_perf_lat_opts_def;
            break;
        case TAPI_RDMA_PERF_TEST_BW:
            opts->bw = tapi_rdma_perf_bw_opts_def;
            break;
        default:
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
    opts->tst_type = tst_type;

    switch (op_type)
    {
        case TAPI_RDMA_PERF_OP_SEND:
            opts->send = tapi_rdma_perf_send_opts_def;
            break;
        case TAPI_RDMA_PERF_OP_WRITE_IMM:
            /*@fallthrough@*/
        case TAPI_RDMA_PERF_OP_WRITE:
            opts->write = tapi_rdma_perf_write_opts_def;
            break;
        case TAPI_RDMA_PERF_OP_READ:
            opts->read = tapi_rdma_perf_read_opts_def;
            break;
        case TAPI_RDMA_PERF_OP_ATOMIC:
            opts->atomic = tapi_rdma_perf_atomic_opts_def;
            break;
        default:
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
    opts->op_type = op_type;
    opts->server_ip = server_ip;

    return 0;
}

/**
 * Get connection type in string representation.
 *
 * @param conn_type       RDMA connection type.
 *
 * @return String with connection type.
 */
extern const char * tapi_rdma_perf_conn_str_get(
                        tapi_rdma_perf_conn_type_t conn_type);

/**
 * Initiate RDMA perf app.
 *
 * @param[in]  factory    Job factory.
 * @param[in]  opts       RDMA perf options.
 * @param[in]  is_client  Are options for server or client side.
 * @param[out] app        The application handle.
 *
 * @return Status code.
 *
 * @sa tapi_rdma_perf_app_destroy
 */
extern te_errno tapi_rdma_perf_app_init(tapi_job_factory_t *factory,
                                        tapi_rdma_perf_opts *opts,
                                        bool is_client,
                                        tapi_rdma_perf_app **app);

/**
 * Destroy RDMA perf app.
 *
 * @param app             RDMA perf app context.
 *
 * @sa tapi_rdma_perf_app_init
 */
extern void tapi_rdma_perf_app_destroy(tapi_rdma_perf_app *app);

/**
 * Start RDMA perf app.
 *
 * @param app             RDMA perf app context.
 *
 * @return Status code.
 *
 * @sa tapi_rdma_perf_app_stop
 */
extern te_errno tapi_rdma_perf_app_start(tapi_rdma_perf_app *app);

/**
 * Wait until RDMA perf client-specific app finishes its work.
 * Note, function jumps to cleanup when timeout expires.
 *
 * @param app             RDMA perf app context.
 * @param timeout_s       Time to wait for app results.
 *                        It MUST be big enough to finish client normally.
 *
 * @return Status code.
 */
extern te_errno tapi_rdma_perf_app_wait(tapi_rdma_perf_app *app,
                                        int timeout_s);

/**
 * Get CMD in string representation that will be used to run RDMA perf app.
 *
 * @param[in]  app             RDMA perf app context.
 * @param[out] cmd             Resulting command line.
 *
 * @note It is expected that @p cmd is allocated.
 *       Can be called only after @b tapi_rdma_perf_app_init().
 *
 * @return Status code.
 */
te_errno tapi_rdma_perf_get_cmd_str(tapi_rdma_perf_app *app, te_string *cmd);

/**
 * Destroy structure to hold perftest results.
 *
 * @param stats    RDMA perftest results.
 */
static inline void
tapi_rdma_perf_destroy_results(tapi_rdma_perf_results *results)
{
    if (results != NULL)
        tapi_rdma_perf_destroy_stats(&results->stats);
}

/**@} <!-- END tapi_rdma_perf --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_RDMA_PERF_H__ */
