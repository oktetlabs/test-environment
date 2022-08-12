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
 * It is strictly prohibited to use this TAPI for changing
 * configuration.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
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
    TAPI_ETHTOOL_CMD_SHOW_RING, /**< Show ring size (--show-ring command)*/
} tapi_ethtool_cmd;

/** Command line options for ethtool */
typedef struct tapi_ethtool_opt {
    tapi_ethtool_cmd cmd; /**< Ethtool command */

    const char *if_name;  /**< Interface name */
} tapi_ethtool_opt;

/** Default options initializer */
extern const tapi_ethtool_opt tapi_ethtool_default_opt;

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

/** Ring parameters parsed in case of @c TAPI_ETHTOOL_CMD_SHOW_RING */
typedef struct tapi_ethtool_ring {
    int rx_max; /**< RX ring size */
    int tx_max; /**< TX ring size */
    int rx; /**< RX ring size */
    int tx; /**< TX ring size */
} tapi_ethtool_ring;

/** Structure for storing parsed data from ethtool output */
typedef struct tapi_ethtool_report {
    tapi_ethtool_cmd cmd; /**< Ethtool command */

    te_bool err_out;       /**< @c TRUE if something was printed to
                                stderr */
    te_string err_data;    /**< Text printed to stderr */
    te_errno err_code;     /**< Error code determined from parsing
                                stderr output */

    union {
        tapi_ethtool_if_props if_props; /**< Interface properties printed
                                             when no command is supplied */
        te_kvpair_h stats; /**< Interface statistics */
        tapi_ethtool_pause pause; /**< Pause parameters */
        tapi_ethtool_ring ring;
    } data; /**< Parsed data */
} tapi_ethtool_report;

/** Default report initializer */
extern const tapi_ethtool_report tapi_ethtool_default_report;

/**
 * Run ethtool command, parse its output if required.
 *
 * @param factory       Job factory
 * @param opt           Ethtool command options
 * @param report        If not @c NULL, parsed data from output will be
 *                      saved here
 *
 * @return Status code.
 */
extern te_errno tapi_ethtool(tapi_job_factory_t *factory,
                             const tapi_ethtool_opt *opt,
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
