/** @file
 * @brief TAPI to manage memaslap
 *
 * @defgroup tapi_memaslap TAPI to manage memaslap
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to manage *memaslap*.
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 *
 * @author Daniil Byshenko <Daniil.Byshenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_MEMASLAP_H__
#define __TE_TAPI_MEMASLAP_H__

#include "tapi_job_opt.h"
#include "te_mi_log.h"
#include "tapi_job.h"
#include "te_units.h"
#include "te_errno.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of servers that can be set in memaslap options */
#define TAPI_MEMASLAP_SERVERS_MAX 16

/** memaslap tool information. */
typedef struct tapi_memaslap_app {
    /** TAPI job handle. */
    tapi_job_t             *job;
    /** Output channel handles: one for stdout, second for stderr. */
    tapi_job_channel_t     *out_chs[2];
    /** Command line used to start the memaslap job. */
    te_vec                  cmd;
    /** Filters list: */
    /** Throughput filter. */
    tapi_job_channel_t     *tps_filter;
    /** Net rate filter. */
    tapi_job_channel_t     *net_rate_filter;
} tapi_memaslap_app;

/** memaslap information from the stdout. */
typedef struct tapi_memaslap_report {
    /** Throughput, operations/second. */
    unsigned int            tps;
    /** The rate of network. It's always in Mb/s. */
    double                  net_rate;
    /** Command line used to start the memaslap job. */
    char                   *cmd;
} tapi_memaslap_report;

/** memaslap specific options. */
typedef struct tapi_memaslap_opt {
    /** Number of actual servers in @p servers. */
    size_t                  n_servers;
    /**
     * List one or more servers to connect.
     * Servers count must be less than threads count.
     * e.g.: -s 192.168.31.31:1234,localhost:11211.
     */
    const struct sockaddr  *servers[TAPI_MEMASLAP_SERVERS_MAX];
    /** Number of threads to startup. */
    tapi_job_opt_uint_t     threads;
    /** The number of concurrencies memaslap runs with. */
    tapi_job_opt_uint_t     concurrency;
    /** Number of TCP socks per concurrency. */
    tapi_job_opt_uint_t     conn_sock;
    /** Number of operations (get and set) to execute for the given test. */
    tapi_job_opt_uint_t     execute_number;
    /**
     * How long the test to run, in seconds.
     * e.g.: --time=20s.
     */
    tapi_job_opt_uint_t     time;
    /**
     * Task window size of each concurrency, in Kilobytes.
     * e.g.: --win_size=10k.
    */
    tapi_job_opt_uint_t     win_size;
    /** Fixed length of value. */
    tapi_job_opt_uint_t     fixed_size;
    /** The proportion of date verification, e.g.: --verify=0.01. */
    tapi_job_opt_double_t   verify;
    /** Number of keys to multi-get once. */
    tapi_job_opt_uint_t     division;
    /**
     * Frequency of dumping statistic information, in seconds.
     * e.g.: --resp_freq=10s.
     */
    tapi_job_opt_uint_t     stat_freq;
    /**
     * The proportion of objects with expire time, e.g.: --exp_verify=0.01.
     * Default no object with expire time.
     */
    tapi_job_opt_double_t   expire_verify;
    /**
     * The proportion of objects need overwrite, e.g.: --overwrite=0.01.
     * Default never overwrite object.
     */
    tapi_job_opt_double_t   overwrite;
    /** Reconnect tests: when connection is closed it will be reconnected. */
    te_bool                 reconnect;
    /** UDP tests. TCP port and UDP port of server must be same. */
    te_bool                 udp;
    /**
     * Enable facebook test feature,
     * set with TCP and multi-get with UDP.
     */
    te_bool                 facebook;
    /** Enable binary protocol. Default with ASCII protocol. */
    te_bool                 bin_protocol;
    /**
     * Expected throughput, in operations/second.
     * e.g.: --tps=10k.
     */
    tapi_job_opt_uint_t     expected_tps;
    /** The first n-th servers can write data, e.g.: --rep_write=2. */
    tapi_job_opt_uint_t     rep_write;
    /** Output detailed information when verification fails. */
    te_bool                 verbose;
    /** Path to memaslap exec. */
    const char             *memaslap_path;
} tapi_memaslap_opt;

/** Default memaslap options initializer. */
extern const tapi_memaslap_opt tapi_memaslap_default_opt;

/**
 * Create memaslap app.
 *
 * @param[in]  factory      Job factory.
 * @param[in]  opt          memaslap options.
 * @param[out] app          memaslap app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_create(tapi_job_factory_t *factory,
                                     const tapi_memaslap_opt *opt,
                                     tapi_memaslap_app **app);

/**
 * Start memaslap.
 *
 * @param[in]  app          memaslap app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_start(const tapi_memaslap_app *app);

/**
 * Wait for memaslap completion.
 *
 * @param[in]  app          memaslap app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_wait(const tapi_memaslap_app *app,
                                   int timeout_ms);

/**
 * Stop memaslap. It can be started over with tapi_memaslap_start().
 *
 * @param[in]  app          memaslap app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_stop(const tapi_memaslap_app *app);

/**
 * Send a signal to memaslap.
 *
 * @param[in]  app          memaslap app handle.
 * @param[in]  signum       Signal to send.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_kill(const tapi_memaslap_app *app, int signum);

/**
 * Destroy memaslap.
 *
 * @param[in]  app          memaslap app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_destroy(tapi_memaslap_app *app);

/**
 * Get memaslap report.
 *
 * @note Field 'arguments' in the @p report is freed
 * in extern te_errno tapi_memaslap_destroy_report(...).
 *
 * @param[in]  app          memaslap app handle.
 * @param[out] report       memaslap statistics report.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_get_report(tapi_memaslap_app *app,
                                         tapi_memaslap_report *report);

/**
 * Add memaslap report to MI logger.
 *
 * @param[in]  report       memaslap statistics report.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_report_mi_log(const tapi_memaslap_report *report);

/**
 * Destroy memaslap report to MI logger and freed memory.
 *
 * @note Field 'arguments' in the @p report is freed here.
 *
 * @param[in]  report       memaslap statistics report.
 *
 * @return Status code.
 */
extern te_errno tapi_memaslap_destroy_report(tapi_memaslap_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_MEMASLAP_H__ */
/**@} <!-- END tapi_memaslap --> */