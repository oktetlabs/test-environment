/** @file
 * @brief ethtool tool TAPI
 *
 * @defgroup tapi_ethtool ethtool tool TAPI (tapi_ethtool)
 * @ingroup te_ts_tapi
 * @{
 *
 * Declarations for TAPI to handle ethtool tool.
 *
 * WARNING: do not use this TAPI unless you really need to check
 * ethtool application itself. Normally configuration tree should
 * be used to read and change network interface settings.
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_ETHTOOL_H__
#define __TE_TAPI_ETHTOOL_H__

#include "te_errno.h"
#include "tapi_job.h"
#include "te_kvpair.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum string length in ethtool output */
#define TAPI_ETHTOOL_MAX_STR 256

/** Supported ethtool commands */
typedef enum tapi_ethtool_cmd {
    TAPI_ETHTOOL_CMD_NONE, /**< No command (shows interface properties) */
    TAPI_ETHTOOL_CMD_STATS, /**< Interface statistics */
    TAPI_ETHTOOL_CMD_SHOW_PAUSE, /**< Show pause parameters (--show-pause
                                      command) */
} tapi_ethtool_cmd;

/** Command line options for ethtool */
typedef struct tapi_ethtool_opt {
    tapi_ethtool_cmd cmd; /**< Ethtool command */

    const char *if_name;  /**< Interface name */
} tapi_ethtool_opt;

/** Default options initializer */
extern const tapi_ethtool_opt tapi_ethtool_default_opt;

/** Main structure describing ethtool command */
typedef struct tapi_ethtool_app tapi_ethtool_app;

/** Interface properties parsed in case of @c TAPI_ETHTOOL_CMD_NONE */
typedef struct tapi_ethtool_if_props {
    te_bool link;     /**< Link status */
    te_bool autoneg;  /**< Auto-negotiation state */
} tapi_ethtool_if_props;

/** Pause parameters parsed in case of @c TAPI_ETHTOOL_CMD_SHOW_PAUSE */
typedef struct tapi_ethtool_pause {
    te_bool autoneg; /**< Pause auto-negotiation state */
    te_bool rx; /**< Whether reception of pause frames is enabled */
    te_bool tx; /**< Whether transmission of pause frames is enabled */
} tapi_ethtool_pause;

/** Structure for storing parsed data from ethtool output */
typedef struct tapi_ethtool_report {
    tapi_ethtool_cmd cmd; /**< Ethtool command */

    union {
        tapi_ethtool_if_props if_props; /**< Interface properties printed
                                             when no command is supplied */
        te_kvpair_h stats; /**< Interface statistics */
        tapi_ethtool_pause pause; /**< Pause parameters */
    } data; /**< Parsed data */
} tapi_ethtool_report;

/** Default report initializer */
extern const tapi_ethtool_report tapi_ethtool_default_report;

/**
 * Create a job to run ethtool application.
 *
 * @param factory       Job factory
 * @param opt           Ethtool options
 * @param app           Where to save pointer to application structure
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_create(tapi_job_factory_t *factory,
                                    const tapi_ethtool_opt *opt,
                                    tapi_ethtool_app **app);

/**
 * Start ethtool application.
 *
 * @param app       Pointer to application structure
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_start(tapi_ethtool_app *app);

/**
 * Wait for termination of ethtool application.
 *
 * @param app           Pointer to application structure
 * @param timeout_ms    Timeout (negative means tapi_job_get_timeout())
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_wait(tapi_ethtool_app *app, int timeout_ms);

/**
 * Send a signal to ethtool application.
 *
 * @param app       Pointer to application structure
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_kill(tapi_ethtool_app *app, int signum);

/**
 * Stop ethtool application.
 *
 * @param app       Pointer to application structure
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_stop(tapi_ethtool_app *app);

/**
 * Release resources allocated for ethtool application.
 *
 * @param app       Pointer to application structure
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_destroy(tapi_ethtool_app *app);

/**
 * Check whether something was printed to stderr.
 *
 * @param app       Pointer to application structure
 *
 * @return @c TRUE if something was printed, @c FALSE otherwise.
 */
extern te_errno tapi_ethtool_check_stderr(tapi_ethtool_app *app);

/**
 * Get data parsed from ethtool output.
 *
 * @param app       Pointer to application structure
 * @param report    Where to save parsed data
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_get_report(tapi_ethtool_app *app,
                                        tapi_ethtool_report *report);

/**
 * Get a single statistic from parsed ethtool output.
 *
 * @param report      Pointer to parsed data storage
 * @param name        Name of the statistic
 * @param value       Where to save parsed value
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool_get_stat(tapi_ethtool_report *report,
                                      const char *name,
                                      long int *value);

/**
 * Release resources allocated for ethtool output data.
 *
 * @param report    Structure storing parsed data
 */
extern void tapi_ethtool_destroy_report(tapi_ethtool_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_ETHTOOL_H__ */

/** @} <!-- END tapi_ethtool --> */
