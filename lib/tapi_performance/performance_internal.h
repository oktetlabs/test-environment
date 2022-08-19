/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Auxiliary functions to performance TAPI
 *
 * Auxiliary functions for internal use in performance TAPI
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __PERFORMANCE_INTERNAL_H__
#define __PERFORMANCE_INTERNAL_H__

#include "tapi_performance.h"
#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Perf application error messages mapping. */
typedef struct tapi_perf_error_map {
    tapi_perf_error code;       /**< Error code. */
    const char *msg;            /**< Error message. */
} tapi_perf_error_map;

/**
 * Create job for perf application with specified arguments.
 *
 * @param factory   Job factory.
 * @param args      List with command and its arguments to create
 *                  application.
 * @param app       Application context.
 *
 * @return Status code.
 *
 * @sa perf_app_stop
 */
extern te_errno perf_app_create_job_from_args(tapi_job_factory_t *factory,
                                              te_vec *args, tapi_perf_app *app);

/**
 * Create server perf tool.
 *
 * @param server    Server context.
 * @param factory   Job factory.
 *
 * @return Status code.
 */
extern te_errno perf_server_create(tapi_perf_server *server,
                                   tapi_job_factory_t *factory);

/**
 * Create client perf tool.
 *
 * @param client    Client context.
 * @param factory   Job factory.
 *
 * @return Status code.
 */
extern te_errno perf_client_create(tapi_perf_client *client,
                                   tapi_job_factory_t *factory);

/**
 * Start perf application. Note, @b perf_app_stop should be called to stop the
 * application.
 *
 * @param[in] app        Application context.
 *
 * @return Status code.
 *
 * @sa perf_app_stop
 */
extern te_errno perf_app_start(tapi_perf_app *app);

/**
 * Read data from filter to string.
 *
 * @param[in]  out_filter   Filter for reading output and error messages.
 * @param[out] str          String with filtered messages perf tool.
 *
 * @return Status code.
 */
extern te_errno perf_app_read_output(tapi_job_channel_t *out_filter,
                                     te_string *str);

/**
 * Stop perf application.
 *
 * @param[in,out] app       Application context.
 *
 * @return Status code.
 *
 * @sa perf_app_start
 */
extern te_errno perf_app_stop(tapi_perf_app *app);

/**
 * Wait while application finishes his work.
 *
 * @param app           Application context.
 * @param timeout       Time to wait for application results (seconds).
 *
 * @return Status code.
 * @retval 0            No errors.
 * @retval TE_ESHCMD    Application returns non-zero exit code.
 * @retval TE_EFAIL     Critical error, application should be stopped.
 */
extern te_errno perf_app_wait(tapi_perf_app *app, int16_t timeout);

/**
 * Check application report for errors. The function prints verdicts in case of
 * errors are presents in the @p report.
 *
 * @param app               Application context.
 * @param report            Application report.
 * @param tag               Tag to print in verdict message.
 *
 * @return Status code. It returns non-zero code if there are errors in the
 * report.
 */
extern te_errno perf_app_check_report(tapi_perf_app *app,
                                      tapi_perf_report *report,
                                      const char *tag);

/**
 * Dump application output (both stdout and stderr).
 *
 * @param app               Application context.
 * @param tag               Tag to print in dump message.
 */
extern void perf_app_dump_output(tapi_perf_app *app, const char *tag);

/**
 * Get application options as a string of name-value pairs.
 *
 * @param server            Server context.
 * @param client            Client context.
 * @param buf               Where to log the results
 */
extern void perf_get_tool_input_tuple(const tapi_perf_server *server,
                                      const tapi_perf_client *client,
                                      te_string *buf);

/**
 * Get application results as a string of name-value pairs.
 *
 * @param report            Report.
 * @param buf               Where to log the results
 */
extern void perf_get_tool_result_tuple(const tapi_perf_report *report,
                                       te_string *buf);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __PERFORMANCE_INTERNAL_H__ */
