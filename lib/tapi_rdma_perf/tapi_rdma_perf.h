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

/** Statistics for BW tests. */
typedef struct tapi_rdma_perf_bw_stats {
    /** BW peak in MB/sec.*/
    double peak;
    /** BW average in MB/sec. */
    double average;
    /** MsgRate in Mpps. */
    double msg_rate;
} tapi_rdma_perf_bw_stats;

/** Statistics for LAT tests. */
typedef struct tapi_rdma_perf_lat_stats {
    float min_usec; /**< Minimal latency. */
    float max_usec; /**< Maximum latency. */
    float typical_usec; /**< Typical latency. */
    float avg_usec; /**< Average latency. */
    float stdev_usec; /**< Standard deviation. */
    float percent_99_00; /**< 99.00 percentile. */
    float percent_99_90; /**< 99.90 percentile. */
} tapi_rdma_perf_lat_stats;

/** Statistics for LAT tests when duration option is set. */
typedef struct tapi_rdma_perf_lat_dur_stats {
    float avg_usec; /**< Average latency. */
    float avg_tps; /**< Average transactions per second. */
} tapi_rdma_perf_lat_dur_stats;

/** Common structure to hold perftest statistics. */
typedef struct tapi_rdma_perf_stats
{
    /** Number of bytes that was sent per each iteration. */
    unsigned long bytes;
    /* Number of iterations that was performed. */
    uint64_t iterations;
    union {
        /** BW-specific test stats. */
        tapi_rdma_perf_bw_stats bw;
        /** LAT-specific test stats. */
        tapi_rdma_perf_lat_stats lat;
        /** LAT test stats when duration option is set. */
        tapi_rdma_perf_lat_dur_stats lat_dur;
    };
    /** Whether some error happened during the statistics parsing. */
    bool parse_error;
} tapi_rdma_perf_stats;

/** Performance test results structure. */
typedef struct tapi_rdma_perf_results {
    tapi_rdma_perf_stats stats;        /**< Perftest stats report. */
} tapi_rdma_perf_results;

#define TAPI_RDMA_PERF_RESULTS_INIT { .stats = { 0 } }

/** Type of perftest stats report. */
typedef enum {
    /** Report for BW tests. */
    TAPI_RDMA_PERF_REPORT_BW,
    /** Report for LAT tests. */
    TAPI_RDMA_PERF_REPORT_LAT,
    /** Report for LAT tests when duration option is set. */
    TAPI_RDMA_PERF_REPORT_LAT_DUR,
} tapi_rdma_perf_report_type_t;

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
    /** Type of perftest stats report. */
    tapi_rdma_perf_report_type_t   report_type;
    /** Arguments that are used when running the tool. */
    te_vec                         args;
    /** Channel for Queue Pair Number retrieval. */
    tapi_job_channel_t            *qp;
    /** Channel to collect stats. */
    tapi_job_channel_t            *stats;

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
 *
 * If the timeout expires, kill the app and receive all available results.
 *
 * @param[in]  app             RDMA perf app context.
 * @param[in]  timeout_s       Time to wait for app results.
 * @param[out] results         Where app results should be stored.
 *
 * @return Status code.
 */
extern te_errno tapi_rdma_perf_app_wait(tapi_rdma_perf_app *app,
                                        int timeout_s,
                                        tapi_rdma_perf_results *results);

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
