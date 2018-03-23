/** @file
 * @brief Auxiliary functions to performance TAPI
 *
 * Auxiliary functions for internal use in performance TAPI
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __PERFORMANCE_INTERNAL_H__
#define __PERFORMANCE_INTERNAL_H__

#include "tapi_performance.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Perf application error messages mapping. */
typedef struct tapi_perf_error_map {
    tapi_perf_error code;       /**< Error code. */
    const char *msg;            /**< Error message. */
} tapi_perf_error_map;


/**
 * Close perf application opened descriptors.
 *
 * @param app               Application context.
 */
extern void perf_app_close_descriptors(tapi_perf_app *app);

/**
 * Start perf application. Note, @b perf_app_stop should be called to stop the
 * application.
 *
 * @param[in]    rpcs       RPC server handle.
 * @param[in]    cmd        Command to execute (start) application. Note, in
 *                          case of failure @p cmd will be free, just to make
 *                          @ref client_start and @ref server_start simple.
 * @param[inout] app        Application context.
 *
 * @return Status code.
 *
 * @sa perf_app_stop
 */
extern te_errno perf_app_start(rcf_rpc_server *rpcs, char *cmd,
                               tapi_perf_app *app);

/**
 * Stop perf application.
 *
 * @param[inout] app        Application context.
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
 * Get application options and results as a string of name-value pairs.
 *
 * @note Return value should be freed with free(3) when it is no longer needed.
 *
 * @param server            Server context.
 * @param client            Client context.
 * @param report            Report.
 *
 * @return Tuple of application options and results.
 */
extern char *perf_get_tool_tuple(const tapi_perf_server *server,
                                 const tapi_perf_client *client,
                                 const tapi_perf_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __PERFORMANCE_INTERNAL_H__ */
