/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI to manage memtier_benchmark
 *
 * @defgroup tapi_memtier TAPI to manage memtier_benchmark
 * @ingroup te_ts_tapi
 * @{
 */

#ifndef __TE_TAPI_MEMTIER_H__
#define __TE_TAPI_MEMTIER_H__

#include "tapi_job_opt.h"
#include "te_mi_log.h"
#include "tapi_job.h"
#include "te_units.h"
#include "te_errno.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** memtier_benchmark tool information */
typedef struct tapi_memtier_app {
    /** TAPI job handle */
    tapi_job_t *job;
    /** Output channel handles: one for stdout, second for stderr */
    tapi_job_channel_t *out_chs[2];
    /** Command line used to start the memtier job */
    te_vec cmd;

    /** Filter used to extract some statistics from stdout */
    tapi_job_channel_t *stats_filter;
} tapi_memtier_app;

/** Statistics for specific operation */
typedef struct tapi_memtier_op_stats {
    /** Throughput, operations/second */
    double tps;
    /** The rate of network, Mbit/sec */
    double net_rate;
    /** Set to TRUE if statistics were parsed */
    te_bool parsed;
} tapi_memtier_op_stats;

/** memtier_benchmark information from stdout */
typedef struct tapi_memtier_report {
    /** Statistics for set operations */
    tapi_memtier_op_stats sets;
    /** Statistics for get operations */
    tapi_memtier_op_stats gets;
    /** Statistics for all operations */
    tapi_memtier_op_stats totals;

    /** Command line used to start the memtier job */
    char *cmd;
} tapi_memtier_report;

/** Default report initializer */
extern const tapi_memtier_report tapi_memtier_default_report;

/** Possible values for --protocol option */
typedef enum tapi_memtier_proto {
    /** Option is omitted */
    TAPI_MEMTIER_PROTO_NONE = TAPI_JOB_OPT_ENUM_UNDEF,

    TAPI_MEMTIER_PROTO_REDIS, /**< "redis" */
    TAPI_MEMTIER_PROTO_RESP2, /**< "resp2" */
    TAPI_MEMTIER_PROTO_RESP3, /**< "resp3" */
    TAPI_MEMTIER_PROTO_MEMCACHE_TEXT, /**< "memcache_text" */
    TAPI_MEMTIER_PROTO_MEMCACHE_BINARY, /**< "memcache_binary" */
} tapi_memtier_proto;

/** memtier_benchmark command line options */
typedef struct tapi_memtier_opt {
    /** Tested server address and port */
    const struct sockaddr *server;

    /** Protocol to use */
    tapi_memtier_proto protocol;

    /** Number of full-test iterations to perform */
    tapi_job_opt_uint_t run_count;
    /** Total requests per client */
    tapi_job_opt_uint_t requests;
    /** Clients per thread */
    tapi_job_opt_uint_t clients;
    /** Number of threads */
    tapi_job_opt_uint_t threads;
    /** Number of concurrent pipelined requests */
    tapi_job_opt_uint_t pipeline;
    /** Number of seconds to run the test */
    tapi_job_opt_uint_t test_time;

    /** Object data size in bytes */
    tapi_job_opt_uint_t data_size;
    /** Indicate that data should be randomized */
    te_bool random_data;

    /** Set:Get ratio (for example, "1:10") */
    const char *ratio;

    /** Prefix for keys (default: "memtier-") */
    const char *key_prefix;

    /**
     * Set-Get key pattern. For example, "S:R" means
     * that sets are sequential and gets are random.
     * Supported options for sets and gets:
     * G - Gaussian distribution
     * R - uniform Random
     * S - Sequential
     * P - Parallel (sequential where every client has a subset of the
     *     key range)
     */
    const char *key_pattern;

    /** Minimum key ID */
    tapi_job_opt_uint_t key_minimum;
    /** Maximum key ID */
    tapi_job_opt_uint_t key_maximum;

    /** Don't print detailed latency histogram */
    te_bool hide_histogram;

    /** Print debug output */
    te_bool debug;

    /** Path to memtier_benchmark executable */
    const char *memtier_path;
} tapi_memtier_opt;

/** Default memtier_benchmark options initializer */
extern const tapi_memtier_opt tapi_memtier_default_opt;

/**
 * Create memtier_benchmark app.
 *
 * @param[in]  factory      Job factory.
 * @param[in]  opt          Application options.
 * @param[out] app          Application handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_create(tapi_job_factory_t *factory,
                                    const tapi_memtier_opt *opt,
                                    tapi_memtier_app **app);

/**
 * Start memtier_benchmark app.
 *
 * @param[in] app          memtier_benchmark app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_start(const tapi_memtier_app *app);

/**
 * Wait for memtier_benchmark completion.
 *
 * @param[in] app          Application handle.
 * @param[in] timeout_ms   Timeout in milliseconds.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_wait(const tapi_memtier_app *app,
                                  int timeout_ms);

/**
 * Stop memtier_benchmark. It can be started over with tapi_memtier_start().
 *
 * @param[in] app          Application handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_stop(const tapi_memtier_app *app);

/**
 * Send a signal to memtier_benchmark.
 *
 * @param[in] app          Application handle.
 * @param[in] signum       Signal to send.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_kill(const tapi_memtier_app *app, int signum);

/**
 * Destroy memtier_benchmark.
 *
 * @param[in] app          Application handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_destroy(tapi_memtier_app *app);

/**
 * Get memtier_benchmark report.
 *
 * @note Resources allocated for the report can be released with
 *       tapi_memtier_destroy_report().
 *
 * @param[in]  app          Application handle.
 * @param[out] report       Statistics report.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_get_report(tapi_memtier_app *app,
                                        tapi_memtier_report *report);

/**
 * Print MI log for memtier_benchmark report.
 *
 * @param[in] report       Report.
 *
 * @return Status code.
 */
extern te_errno tapi_memtier_report_mi_log(
                                    const tapi_memtier_report *report);

/**
 * Release resources allocated for memtier_benchmark report.
 *
 * @param[in] report    Report
 */
extern void tapi_memtier_destroy_report(tapi_memtier_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_MEMTIER_H__ */

/**@} <!-- END tapi_memtier --> */
