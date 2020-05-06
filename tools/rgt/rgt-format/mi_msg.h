/** @file
 * @brief Declaration of API for logging MI messages
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky  <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_RGT_MI_MSG_H__
#define __TE_RGT_MI_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDDEF_H
#include <stddef.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif

#include "te_defs.h"
#include "te_errno.h"

/** Maximum length of error description from JSON parser */
#define TE_RGT_MI_MAX_ERR_LEN 1024

/** Value of a measured parameter or statistic obtained from JSON. */
typedef struct te_rgt_mi_meas_value {
    te_bool defined;            /**< If @c TRUE, the value is defined */
    te_bool specified;          /**< If @c TRUE, the numeric value was
                                     specified */
    double value;               /**< Value of "value" field */
    const char *multiplier;     /**< Value of "multiplier" field */
    const char *base_units;     /**< Value of "base_units" field */
} te_rgt_mi_meas_value;

/** Initializer for te_rgt_mi_meas_value structure */
#define TE_RGT_MI_MEAS_VALUE_INIT { FALSE, FALSE, 0, NULL, NULL }

/** Description of measured parameter */
typedef struct te_rgt_mi_meas_param {
    const char *name;               /**< Parameter name */

    te_bool stats_present;          /**< @c TRUE if some of the
                                         statistics are set */
    te_rgt_mi_meas_value min;       /**< Minimum value */
    te_rgt_mi_meas_value max;       /**< Maximum value */
    te_rgt_mi_meas_value mean;      /**< Mean */
    te_rgt_mi_meas_value median;    /**< Median */
    te_rgt_mi_meas_value stdev;     /**< Standard deviation */
    te_rgt_mi_meas_value cv;        /**< Coefficient of variation */

    te_rgt_mi_meas_value *values;   /**< Array of parameter values */
    size_t values_num;              /**< Number of elements in the
                                         array of values */
} te_rgt_mi_meas_param;

/** Key-value pair */
typedef struct te_rgt_mi_kv {
    const char *key;      /**< Key */
    const char *value;    /**< Value */
} te_rgt_mi_kv;

/** Description of MI message of type "measurement" */
typedef struct te_rgt_mi_meas {
    const char *tool;               /**< Tool name */
    const char *version;            /**< Version */

    te_rgt_mi_meas_param *params;   /**< Array of measured parameters */
    size_t params_num;              /**< Number of measured parameters */

    te_rgt_mi_kv *keys;             /**< Array of keys and associated
                                         values */
    size_t keys_num;                /**< Number of keys */

    te_rgt_mi_kv *comments;         /**< Array of comments */
    size_t comments_num;            /**< Number of comments */
} te_rgt_mi_meas;

/** Types of MI message */
typedef enum {
    TE_RGT_MI_TYPE_MEASUREMENT = 0,   /**< Measurement */
    TE_RGT_MI_TYPE_UNKNOWN            /**< Unknown type */
} te_rgt_mi_type;

/** Parsed MI message */
typedef struct te_rgt_mi {
    te_rgt_mi_type type;    /**< MI message type */

    te_errno rc;      /**< Error occurred when parsing and
                           filling the structure. @c TE_EOPNOTSUPP
                           means there is no libjansson. */
    te_bool parse_failed;   /**< Will be set to @c TRUE if it could
                                 not parse JSON */
    char parse_err[TE_RGT_MI_MAX_ERR_LEN]; /**< Error message from
                                                JSON parser */

    void *json_obj; /**< Pointer to parsed JSON object */

    union {
        te_rgt_mi_meas measurement; /**< Data for measurement MI
                                         message */
    } data; /**< Data obtained from JSON object */
} te_rgt_mi;

/**
 * Parse MI message.
 *
 * @note Currently only "measurement" MI message is fully supported,
 *       for other types this function only tries to parse JSON without
 *       extracting any data from it.
 *
 * @param json_buf          Buffer with JSON from MI message.
 * @param buf_len           Length of the buffer.
 * @param mi                Where to store obtained data.
 */
extern void te_rgt_parse_mi_message(const char *json_buf,
                                    size_t buf_len,
                                    te_rgt_mi *mi);

/**
 * Release memory allocated for storing MI message data.
 *
 * @param mi      Pointer to te_rgt_mi structure.
 */
extern void te_rgt_mi_clean(te_rgt_mi *mi);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_MI_MSG_H__ */
