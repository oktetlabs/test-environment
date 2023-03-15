/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI to manage dnsperf tool
 *
 * @defgroup tapi_dnsperf TAPI to manage dnsperf tool
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to manage *dnsperf*.
 */

#ifndef __TE_TAPI_DNSPERF_H__
#define __TE_TAPI_DNSPERF_H__

#include "te_errno.h"
#include "te_queue.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** dnsperf tool information. */
typedef struct tapi_dnsperf_app {
    /** TAPI job handle */
    tapi_job_t *job;
    /** Test agent name. */
    const char *ta;
    /** arguments that are used when running the tool */
    te_vec cmd;
    /** output channel handles */
    tapi_job_channel_t *out_chs[2];

    /** Filters list: */
    /** queries completed */
    tapi_job_channel_t *flt_queries_sent;
    /** queries completed */
    tapi_job_channel_t *flt_queries_completed;
    /** queries lost */
    tapi_job_channel_t *flt_queries_lost;
    /** average request size */
    tapi_job_channel_t *flt_avg_request_size;
    /** average response size */
    tapi_job_channel_t *flt_avg_response_size;
    /** run time */
    tapi_job_channel_t *flt_run_time;
    /** queries per second */
    tapi_job_channel_t *flt_rps;

    /** Name of temporary configuration file. */
    char *tmp_fname;
} tapi_dnsperf_app;

/** dnsperf information from the stdout. */
typedef struct tapi_dnsperf_report {
    /** queries sent */
    unsigned int queries_sent;

    /** queries completed */
    unsigned int queries_completed;

    /** queries lost */
    unsigned int queries_lost;

    /** queries lost (%) */
    double queries_lost_percent;

    /** average request packet size */
    double avg_request_size;

    /** average response packet size */
    double avg_response_size;

    /** run time (s) */
    double run_time;

    /** throughput, queries/second */
    double rps;

    /** Net_rate in Mibps (calculated value) */
    double net_rate;

    /** command line used to start the dnsperf job */
    char *cmd;
} tapi_dnsperf_report;

/** Representation of possible values for dnsperf family option. */
typedef enum tapi_dnsperf_addr_family {
    TAPI_DNSPERF_ADDR_FAMILY_UNDEF = TAPI_JOB_OPT_ENUM_UNDEF,
    TAPI_DNSPERF_ADDR_FAMILY_INET,
    TAPI_DNSPERF_ADDR_FAMILY_INET6,
    TAPI_DNSPERF_ADDR_FAMILY_ANY,
} tapi_dnsperf_addr_family;

/** Representation of possible values for dnsperf mode option. */
typedef enum tapi_dnsperf_transport_mode {
    TAPI_DNSPERF_TRANSPORT_MODE_UNDEF = TAPI_JOB_OPT_ENUM_UNDEF,
    TAPI_DNSPERF_TRANSPORT_MODE_UDP,
    TAPI_DNSPERF_TRANSPORT_MODE_TCP,
    TAPI_DNSPERF_TRANSPORT_MODE_DOT,
    TAPI_DNSPERF_TRANSPORT_MODE_DOH,
} tapi_dnsperf_transport_mode;

/** dnsperf specific options. */
typedef struct tapi_dnsperf_opt {
    /** the local address from which to send requests */
    const char                 *local_addr;

    /** socket send/receive buffer size in kilobytes */
    tapi_job_opt_uint_t         bufsize;

    /** the number of clients to act as */
    tapi_job_opt_uint_t         clients;

    /** the input data file (default: stdin) */
    const char                 *datafile;

    /** set the DNSSEC OK bit (implies EDNS) */
    te_bool                     enable_dnssec_ok;

    /** enable EDNS 0 */
    te_bool                     enable_edns0;

    /** send EDNS option */
    const char                 *edns_opt;

    /** address family of DNS transport, inet, inet6 or any */
    tapi_dnsperf_addr_family    addr_family;

    /** run for at most this many seconds */
    tapi_job_opt_uint_t         limit;

    /** run through input at most N times */
    tapi_job_opt_uint_t         runs_through_file;

    /** the port on which to query the server */
    tapi_job_opt_uint_t         port;

    /** the maximum number of queries outstanding */
    tapi_job_opt_uint_t         num_queries;

    /** limit the number of queries per second */
    tapi_job_opt_uint_t         max_qps;

    /** set transport mode: udp, tcp, dot or doh */
    tapi_dnsperf_transport_mode transport_mode;

    /** the server to query */
    const char                  *server;

    /** print qps statistics every N seconds */
    tapi_job_opt_uint_t         stats_interval;

    /** the timeout for query completion in seconds */
    tapi_job_opt_uint_t         timeout;

    /** the number of threads to run */
    tapi_job_opt_uint_t         threads;

    /** verbose: report each query and additional information to stdout */
    te_bool                     verbose;

    /** log warnings and errors to stdout instead of stderr */
    te_bool                     stdout_only;

    /** the local port from which to send queries */
    tapi_job_opt_uint_t         local_port;

    /** Suppress various messages and warnings */
    const char                 *ext_opt;

    /*
     * Noe: the following options are not supported as they are not needed yet:
     * -u send dynamic updates instead of queries
     * -y the TSIG algorithm, name and secret (base64)
     */

    /** list of hosts to query (see @ref tapi_dnsperf_opt_query_add_a) */
    te_vec                      queries;

    /** Path to dnsperf exec (if @c NULL then "dnsperf"). */
    const char                 *dnsperf_path;
} tapi_dnsperf_opt;

/** Default dnsperf options initializer. */
extern const tapi_dnsperf_opt tapi_dnsperf_default_opt;

/**
 * Add new DNS query with @c A type.
 *
 * @param opt      dnsperf options
 * @param host     host name
 */
extern void tapi_dnsperf_opt_query_add_a(tapi_dnsperf_opt *opt,
                                         const char *host);

/**
 * Add new DNS query with @c AAAA type.
 *
 * @param opt      dnsperf options
 * @param host     host name
 */
extern void tapi_dnsperf_opt_query_add_aaaa(tapi_dnsperf_opt *opt,
                                            const char *host);

/**
 * Release memory used by queries.
 *
 * @param opt      dnsperf options
 */
extern void tapi_dnsperf_opt_queries_free(tapi_dnsperf_opt *opt);

/**
 * Create dnsperf app.
 *
 * @param[in]  factory      job factory
 * @param[in]  opt          dnsperf options
 * @param[out] app          dnsperf app handle
 *
 * @return Status code.
 */
extern te_errno tapi_dnsperf_create(tapi_job_factory_t *factory,
                                    tapi_dnsperf_opt *opt,
                                    tapi_dnsperf_app **app);

/**
 * Start dnsperf.
 *
 * @param app   dnsperf app handle
 *
 * @return Status code.
 */
extern te_errno tapi_dnsperf_start(const tapi_dnsperf_app *app);

/**
 * Wait for dnsperf completion.
 *
 * @param app          dnsperf app handle
 * @param timeout_ms   wait timeout in milliseconds
 *
 * @return Status code.
 */
extern te_errno tapi_dnsperf_wait(const tapi_dnsperf_app *app,
                                  int timeout_ms);

/**
 * Stop dnsperf. It can be started over with tapi_dnsperf_start().
 *
 * @param app   dnsperf app handle
 *
 * @return Status code.
 */
extern te_errno tapi_dnsperf_stop(const tapi_dnsperf_app *app);

/**
 * Send a signal to dnsperf.
 *
 * @param app       dnsperf app handle
 * @param signum    signal to send
 *
 * @return Status code.
 */
extern te_errno tapi_dnsperf_kill(const tapi_dnsperf_app *app, int signum);

/**
 * Destroy dnsperf.
 *
 * @param app   dnsperf app handle
 *
 * @return Status code.
 */
extern te_errno tapi_dnsperf_destroy(tapi_dnsperf_app *app);

/**
 * Get dnsperf report.
 *
 * @note Field 'arguments' in the @p report is freed
 * in extern te_errno tapi_dnsperf_destroy_report(...).
 *
 * @param[in]  app          dnsperf app handle
 * @param[out] report       dnsperf statistics report
 */
extern te_errno tapi_dnsperf_get_report(tapi_dnsperf_app *app,
                                        tapi_dnsperf_report *report);

/**
 * Add dnsperf report to MI logger.
 *
 * @param report    dnsperf statistics report
 *
 * @return Status code.
 */
extern te_errno tapi_dnsperf_report_mi_log(const tapi_dnsperf_report *report);

/**
 * Destroy dnsperf report to MI logger and freed memory.
 *
 * @note Field 'arguments' in the @p report is freed here.
 *
 * @param report    dnsperf statistics report
 */
extern void tapi_dnsperf_destroy_report(tapi_dnsperf_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_DNSPERF_H__ */
/**@} <!-- END tapi_dnsperf --> */
