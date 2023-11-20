/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief HAProxy tool TAPI
 *
 * @defgroup tapi_haproxy HAProxy tool TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to manage HAProxy tool.
 */

#ifndef __TE_TAPI_HAPROXY_H__
#define __TE_TAPI_HAPROXY_H__

#include "tapi_haproxy_cfg.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_HAPROXY_PATH "haproxy"
#define TAPI_HAPROXY_CONF_FILENAME_SUFFIX "haproxy.conf"
#define TAPI_HAPROXY_TERM_TIMEOUT_MS 1000

/** HAProxy specific command line options. */
typedef struct tapi_haproxy_opt {
    /** Path to the executable. */
    const char *haproxy_path;
    /**
     * Config file for HAProxy to read.
     * Set to @c NULL to generate config file from
     * tapi_haproxy_opt::cfg_opt.
     */
    const char *cfg_file;
    /**
     * Configuration options for HAProxy.
     * This field is ignored if tapi_haproxy_opt::cfg_file is not
     * @c NULL.
     */
    const tapi_haproxy_cfg_opt *cfg_opt;
    /** Verbosity flag. */
    te_bool verbose;
} tapi_haproxy_opt;

/** Default options initializer. */
extern const tapi_haproxy_opt tapi_haproxy_default_opt;

/** HAProxy tool information. */
typedef struct tapi_haproxy_app {
    /** TAPI job handle. */
    tapi_job_t *job;
    /** Output channel handles. */
    tapi_job_channel_t *out_chs[2];
    /** Stdout filter. */
    tapi_job_channel_t *stdout_filter;
    /** Stderr filter. */
    tapi_job_channel_t *stderr_filter;
    /** Path to generated config file. */
    char *generated_cfg_file;
} tapi_haproxy_app;

/**
 * Create HAProxy app.
 *
 * @param[in]  factory    Job factory.
 * @param[in]  opt        HAProxy tool options.
 * @param[out] app        HAProxy app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_haproxy_create(tapi_job_factory_t *factory,
                                    const tapi_haproxy_opt *opt,
                                    tapi_haproxy_app **app);

/**
 * Start HAProxy tool.
 *
 * @param app   HAProxy app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_haproxy_start(tapi_haproxy_app *app);

/**
 * Wait for HAProxy tool completion.
 *
 * @param app           HAProxy app handle.
 * @param timeout_ms    Wait timeout in milliseconds.
 *
 * @return    Status code.
 * @retval    TE_EINPROGRESS HAProxy is still running.
 */
extern te_errno tapi_haproxy_wait(tapi_haproxy_app *app, int timeout_ms);

/**
 * Send a signal to HAProxy tool.
 *
 * @param app       HAProxy app handle.
 * @param signum    Signal to send.
 *
 * @return    Status code.
 */
extern te_errno tapi_haproxy_kill(tapi_haproxy_app *app, int signo);

/**
 * Destroy HAProxy app. The app cannot be used after calling this function.
 *
 * @param app   HAProxy app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_haproxy_destroy(tapi_haproxy_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_HAPROXY_H__ */
/**@} <!-- END tapi_haproxy --> */