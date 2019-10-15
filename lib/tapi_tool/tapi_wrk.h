/** @file
 * @brief wrk tool TAPI
 *
 * @defgroup tapi_wrk tool functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle wrk tool.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_WRK_H__
#define __TE_TAPI_WRK_H__

#include "te_errno.h"
#include "te_string.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of entries in wrk latency distibution statistics */
#define TAPI_WRK_LATENCY_DISTR_ENTRIES 4

/** Maximum number of headers that can be set in wrk options */
#define TAPI_WRK_HEADERS_MAX 10

/** wrk tool specific command line options */
typedef struct tapi_wrk_opt {
    /** Number of connections to keep open */
    unsigned int connections;
    /** Number of threads to use */
    unsigned int n_threads;
    /** Duration of test in seconds */
    unsigned int duration_s;
    /** Socket/request timeout in milliseconds */
    unsigned int timeout_ms;
    /**
     * Load Lua script file.
     * The file is created in TA only if script content is not NULL.
     */
    const char *script_content;
    /**
     * The file path is auto-generated only if
     * the path is @c NULL AND script_content is not @c NULL.
     */
    const char *script_path;
    /** Array option for headers field */
    tapi_job_opt_array headers_array;
    /** Request headers in "Name: Value" format (note the space). */
    const char *headers[TAPI_WRK_HEADERS_MAX];
    /** Print latency statistics */
    te_bool latency;
    /** Host to connect to */
    const char *host;
} tapi_wrk_opt;

/** Default options initializer */
extern const tapi_wrk_opt tapi_wrk_default_opt;

/** Statistics for a single thread in wrk (units are not specified) */
typedef struct tapi_wrk_thread_stats {
    /** Mean value */
    double mean;
    /** Standard deviation */
    double stdev;
    /** Max value */
    double max;
    /** Percentage of values within [mean - stdev ; mean + stdev] range */
    double within_stdev;
} tapi_wrk_thread_stats;

/** Entry of latency distribution statistics */
typedef struct tapi_wrk_latency_percentile {
    /** Percentile */
    double percentile;
    /** Latency value */
    double latency;
} tapi_wrk_latency_percentile;

/** Statistics report of wrk */
typedef struct tapi_wrk_report {
    /** Latency in microseconds (for each thread) */
    tapi_wrk_thread_stats thread_latency;
    /** Requests per second (for each thread) */
    tapi_wrk_thread_stats thread_req_per_sec;
    /** Latency distribution */
    tapi_wrk_latency_percentile lat_distr[TAPI_WRK_LATENCY_DISTR_ENTRIES];
    /** Requests per second */
    double req_per_sec;
    /** Bytes per second */
    double bps;
} tapi_wrk_report;

/** Information of a wrk tool */
typedef struct tapi_wrk_app {
    /** TAPI job handle */
    tapi_job_t *job;
    /** Output channel handles */
    tapi_job_channel_t *out_chs[2];
    /** Bytes per second filter */
    tapi_job_channel_t *bps_filter;
    /** Total requests filter */
    tapi_job_channel_t *req_total_filter;
    /** Latency per thread filter */
    tapi_job_channel_t *lat_filter;
    /** Requests per thread filter */
    tapi_job_channel_t *req_filter;
    /** Latency distribution filter */
    tapi_job_channel_t *lat_distr_filter;
} tapi_wrk_app;

/**
 * Create wrk app. All needed information to run wrk is in @p opt
 *
 * @param[in]  rpcs         RPC server to create job on.
 * @param[in]  opt          Options of wrk tool.
 * @param[out] app          wrk app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_wrk_create(rcf_rpc_server *rpcs, const tapi_wrk_opt *opt,
                                tapi_wrk_app **app);

/**
 * Start wrk.
 *
 * @param[in]  app          wrk app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_wrk_start(tapi_wrk_app *app);

/**
 * Wait for wrk completion.
 *
 * @param[in]  app          wrk app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *
 * @return Status code.
 */
extern te_errno tapi_wrk_wait(tapi_wrk_app *app, int timeout_ms);

/**
 * Get wrk report.
 *
 * @param[in]  app          wrk app handle.
 * @param[out] report       wrk statistics report.
 *
 * @return Status code.
 */
extern te_errno tapi_wrk_get_report(tapi_wrk_app *app, tapi_wrk_report *report);

/**
 * Send a signal to wrk.
 *
 * @param[in]  app          wrk app handle.
 * @param[in]  signo        Signal to send to wrk.
 *
 * @return Status code.
 */
extern te_errno tapi_wrk_kill(tapi_wrk_app *app, int signo);

/**
 * Destroy wrk app.
 *
 * @param[in]  app          wrk app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_wrk_destroy(tapi_wrk_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_WRK_H__ */

/**@} <!-- END tapi_wrk --> */
