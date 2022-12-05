/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @prief redis-benchmark tool TAPI
 *
 * @defgroup tapi_redis_benchmark redis-benchmark tool TAPI (tapi_redis_benchmark)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle redis-benchmark tool.
 *
 * Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_REDIS_BENCHMARK_H__
#define __TE_TAPI_REDIS_BENCHMARK_H__

#include "tapi_job_opt.h"
#include "tapi_job.h"
#include "tapi_rpc_stdio.h"
#include "te_errno.h"
#include "te_mi_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_REDIS_BENCHMARK_RECEIVE_TIMEOUT_MS     1000
#define TAPI_REDIS_BENCHMARK_TIMEOUT_MS             10000

/** Redis-benchmark tool information. */
typedef struct tapi_redis_benchmark_app {
    /** TAPI job handle */
    tapi_job_t         *job;
    /** output channel handles (stdout and stderr) */
    tapi_job_channel_t *out_chs[2];
    /** Filters list: */
    /** filter for test names */
    tapi_job_channel_t *filter_test_name;
    /** filter for test duration */
    tapi_job_channel_t *filter_time;
    /** filter for test RPS */
    tapi_job_channel_t *filter_rps;
} tapi_redis_benchmark_app;

/** Specific redis-benchmark options. */
typedef struct tapi_redis_benchmark_opt {
    /** IP and port of the server under test */
    const struct sockaddr  *server;
    /** Server socket. */
    const char             *socket;
    /** Number of parallel connections. */
    tapi_job_opt_uint_t     clients;
    /** Total number of requests. */
    tapi_job_opt_uint_t     requests;
    /** Data size of SET/GET value, in bytes. */
    tapi_job_opt_uint_t     size;
    /** SELECT the specified db number. */
    tapi_job_opt_uint_t     dbnum;
    /** Keep alive or reconnect. */
    tapi_job_opt_uint_t     keep_alive;
    /**
     * Use random keys for SET/GET/INCR, random values for SADD.
     * Using this option the benchmark will expand the string __rand_int__
     * inside an argument with a @c 12 digits number in the specified range
     * from 0 to keyspacelen-1. The substitution changes every time a command
     * is executed.
     */
    tapi_job_opt_uint_t     keyspacelen;
    /** Number of pipline requests. */
    tapi_job_opt_uint_t     pipelines;
    /** If server replies with errors, show them on stdout. */
    te_bool                 show_srv_errors;
    /** Number of threads to use. */
    tapi_job_opt_uint_t     threads;
    /** Only run the comma separated list of tests. */
    const char             *tests;
    /** Idle mode. */
    te_bool                 idle;
    /** Path to redis-benchmark exec (if @c NULL then "redis-benchmark"). */
    const char             *exec_path;
} tapi_redis_benchmark_opt;

/** List node with redis-benchmark statistics. */
typedef struct tapi_redis_benchmark_stat {
    SLIST_ENTRY(tapi_redis_benchmark_stat) next;
    /** test name */
    char *test_name;
    /** requests per second */
    double rps;
    /** execution time */
    double time;
} tapi_redis_benchmark_stat;

/** Test stat list entry. */
typedef SLIST_HEAD(tapi_redis_benchmark_report,
                   tapi_redis_benchmark_stat) tapi_redis_benchmark_report;

/** Default redis-benchmark options initializer. */
extern const tapi_redis_benchmark_opt tapi_redis_benchmark_default_opt;

/**
 * Create redis-benchmark app.
 *
 * @param[in]  factory      job factory
 * @param[in]  opt          redis-benchmark options
 * @param[out] app          redis-benchmark app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_create(tapi_job_factory_t *factory,
                                            const tapi_redis_benchmark_opt *opt,
                                            tapi_redis_benchmark_app **app);

/**
 * Start redis-benchmark.
 *
 * @param app   redis-benchmark app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_start(const tapi_redis_benchmark_app *app);

/**
 * Wait for redis-benchmark completion.
 *
 * @param app           redis-benchmark app handle
 * @param timeout_ms    wait timeout in milliseconds
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_wait(const tapi_redis_benchmark_app *app,
                                          int timeout_ms);

/**
 * Stop redis-benchmark. It can be started over with
 * @ref tapi_redis_benchmark_start.
 *
 * @param app   redis-benchmark app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_stop(const tapi_redis_benchmark_app *app);

/**
 * Send a signal to redis-benchmark.
 *
 * @param app       redis-benchmark app handle
 * @param signum    signal to send
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_kill(const tapi_redis_benchmark_app *app,
                                          int signum);

/**
 * Destroy redis-benchmark.
 *
 * @param app   redis-benchmark app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_destroy(tapi_redis_benchmark_app *app);

/**
 * Empty redis-benchmark report list and free it entries.
 *
 * @param entry     report list entry
 */
extern void tapi_redis_benchmark_destroy_report(
                tapi_redis_benchmark_report *entry);

/**
 * Get redis-benchmark report.
 *
 * @param[in]  app      redis-benchmark app handle
 * @param[out] report   redis-benchmark statistics report
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_get_report(
                    tapi_redis_benchmark_app *app,
                    tapi_redis_benchmark_report *report);

/**
 * Get redis-benchmark statistics for a test name.
 *
 * @param report    redis-benchmark report
 * @param test_name test name
 *
 * @return redis-benchmark statistic or @c NULL in case of failure
 */
extern tapi_redis_benchmark_stat *
tapi_redis_benchmark_report_get_stat(tapi_redis_benchmark_report *report,
                                     const char *test_name);
/**
 * Add redis-benchmark report to MI logger.
 *
 * @param report        redis-benchmark statistics report
 *
 * @return Status code.
 */
extern te_errno tapi_redis_benchmark_report_mi_log(
                    te_mi_logger *logger,
                    tapi_redis_benchmark_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_REDIS_BENCHMARK_H__ */

/**@} <!-- END tapi_redis --> */
