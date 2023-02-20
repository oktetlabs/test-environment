/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unbound DNS server tool TAPI.
 *
 * @defgroup tapi_dns_unbound unbound DNS server tool TAPI (tapi_dns_unbound)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle unbound DNS server tool.
 */

#ifndef __TE_TAPI_DNS_UNBOUND_H__
#define __TE_TAPI_DNS_UNBOUND_H__

#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Representation of possible values for unbound::verbose option. */
typedef enum tapi_dns_unbound_verbose {
    /** No verbosity, only errors. */
    TAPI_DNS_UNBOUND_NOT_VERBOSE = TAPI_JOB_OPT_ENUM_UNDEF,

    /** Gives operational information. */
    TAPI_DNS_UNBOUND_VERBOSE = 1,

    /**
     * Gives detailed operational information including short information per
     * query.
     */
    TAPI_DNS_UNBOUND_MORE_VERBOSE,

    /** Gives query level information, output per query. */
    TAPI_DNS_UNBOUND_VERBOSE_LL_QUERY,

    /** Gives algorithm level information. */
    TAPI_DNS_UNBOUND_VERBOSE_LL_ALGO,

    /** Logs client identification for cache misses. */
    TAPI_DNS_UNBOUND_VERBOSE_LL_CACHE,
} tapi_dns_unbound_verbose;

/** Unbound DNS server specific command line options. */
typedef struct tapi_dns_unbound_opt {
    /** Path to Unbound executable. */
    const char *unbound_path;

    /**
     * Config file with settings for unbound to read instead of reading the file
     * at the default location.
     */
    const char *cfg_file;

    /** Increase verbosity. */
    tapi_dns_unbound_verbose verbose;
} tapi_dns_unbound_opt;

/** Default options initializer. */
extern const tapi_dns_unbound_opt tapi_dns_unbound_default_opt;

/** Information of a unbound DNS server tool. */
typedef struct tapi_dns_unbound_app {
    /** TAPI job handle. */
    tapi_job_t *job;
    /** Output channel handles. */
    tapi_job_channel_t *out_chs[2];
    /** Filter for out channel. */
    tapi_job_channel_t *out_filter;
    /** Filter for error messages. */
    tapi_job_channel_t *err_filter;
    /** Filter for debug info messages. */
    tapi_job_channel_t *info_filter;
} tapi_dns_unbound_app;

/**
 * Create unbound DNS server app.
 *
 * @param[in]  factory    Job factory.
 * @param[in]  opt        Unbound server tool options.
 * @param[out] app        Unbound server app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_create(tapi_job_factory_t *factory,
                                        const tapi_dns_unbound_opt *opt,
                                        tapi_dns_unbound_app **app);

/**
 * Start unbound DNS server tool.
 *
 * @param app    Unbound DNS server app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_start(tapi_dns_unbound_app *app);

/**
 * Wait for unbound DNS server tool completion.
 *
 * @param app           Unbound DNS server app handle.
 * @param timeout_ms    Wait timeout in milliseconds.
 *
 * @return    Status code.
 * @retval    TE_EINPROGRESS unbound DNS server is still running.
 */
extern te_errno tapi_dns_unbound_wait(tapi_dns_unbound_app *app,
                                      int timeout_ms);

/**
 * Send a signal to unbound DNS server tool.
 *
 * @param app       Unbound DNS server app handle.
 * @param signum    Signal to send.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_kill(tapi_dns_unbound_app *app, int signum);

/**
 * Stop unbound DNS server tool. It can be started over with
 * tapi_dns_unbound_start().
 *
 * @param app    Unbound DNS server app handle.
 *
 * @return    Status code.
 * @retval    TE_EPROTO unbound DNS server tool is stopped, but report is
 *            unavailable.
 */
extern te_errno tapi_dns_unbound_stop(tapi_dns_unbound_app *app);

/**
 * Destroy unbound DNS server app. The app cannot be used after calling this
 * function.
 *
 * @param app    Unbound DNS server app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_destroy(tapi_dns_unbound_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_DNS_UNBOUND_H__ */

/** @} <!-- END tapi_dns_unbound --> */
