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
#include "te_vector.h"

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
    const char *name;                   /**< Parameter name */
    const char *type;                   /**< Parameter type */
    const char *descr;                  /**< Parameter description */

    te_bool stats_present;              /**< @c TRUE if some of the
                                             statistics are set */
    te_rgt_mi_meas_value min;           /**< Minimum value */
    te_rgt_mi_meas_value max;           /**< Maximum value */
    te_rgt_mi_meas_value mean;          /**< Mean */
    te_rgt_mi_meas_value median;        /**< Median */
    te_rgt_mi_meas_value stdev;         /**< Standard deviation */
    te_rgt_mi_meas_value cv;            /**< Coefficient of variation */
    te_rgt_mi_meas_value out_of_range;  /**< Number of out of range values */
    te_rgt_mi_meas_value percentile;    /**< N-th percentile */

    te_rgt_mi_meas_value *values;       /**< Array of parameter values */
    size_t values_num;                  /**< Number of elements in the
                                             array of values */

    te_bool in_graph;                   /**< @c TRUE if this measured
                                             parameter is part of some
                                             graph view */
} te_rgt_mi_meas_param;

/** Key-value pair */
typedef struct te_rgt_mi_kv {
    const char *key;      /**< Key */
    const char *value;    /**< Value */
} te_rgt_mi_kv;

/** Sequence number is specified on a graph axis */
#define TE_RGT_MI_GRAPH_AXIS_AUTO_SEQNO -1

/** Line-graph view */
typedef struct te_rgt_mi_meas_view_line_graph {
    ssize_t axis_x;     /**< Measured parameter on axis X */
    ssize_t *axis_y;    /**< Measured parameters on axis Y */
    size_t axis_y_num;  /**< Number of measured parameters on
                             axis Y */
} te_rgt_mi_meas_view_line_graph;

/** View (graph, etc) */
typedef struct te_rgt_mi_meas_view {
    const char *name;   /**< Name of the view */
    const char *type;   /**< Type of the view */
    const char *title;  /**< Title of the view */

    union {
        te_rgt_mi_meas_view_line_graph line_graph; /**< Line-graph related
                                                        data */
    } data;
} te_rgt_mi_meas_view;

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

    te_rgt_mi_meas_view *views;     /**< Array of views */
    size_t views_num;               /**< Number of views */
} te_rgt_mi_meas;

/** Personal information */
typedef struct te_rgt_mi_person {
    const char *name;  /**< Full name */
    const char *email; /**< Email address */
} te_rgt_mi_person;

/** Description of MI message of type "test_start" */
typedef struct te_rgt_mi_test_start {
    int               node_id;    /**< Node ID */
    int               parent_id;  /**< Parent ID */
    int               plan_id;    /**< Plan ID */
    const char       *node_type;  /**< PACKAGE, SESSION or TEST */
    const char       *name;       /**< Name */
    te_rgt_mi_kv     *params;     /**< Array of parameters */
    size_t            params_num; /**< Number of parameters */
    te_rgt_mi_person *authors;    /**< Array of authors */
    size_t            authors_num;/**< Number of authors */
    const char       *objective;  /**< Objective */
    const char       *page;       /**< Page */
    int               tin;        /**< Test Iteration Number */
    const char       *hash;       /**< Hash */
} te_rgt_mi_test_start;

/** Description of a test result */
typedef struct te_rgt_mi_test_result {
    const char  *status;       /**< Status code */
    const char **verdicts;     /**< Array of verdicts */
    size_t       verdicts_num; /**< Number of verdicts */
    const char  *notes;        /**< Additional notes */
    const char  *key;          /**< Result key, e.g. bug reference */
} te_rgt_mi_test_result;

/** Description of MI message of type "test_end" */
typedef struct te_rgt_mi_test_end {
    int                    node_id;      /**< Node ID */
    int                    parent_id;    /**< Parent ID */
    int                    plan_id;      /**< Plan ID */
    const char            *error;        /**< TRC error message */
    const char            *tags_expr;    /**< Matched tag expression */
    te_rgt_mi_test_result  obtained;     /**< Obtained result */
    te_rgt_mi_test_result *expected;     /**< Array of expected results */
    size_t                 expected_num; /**< Number of expected results */
} te_rgt_mi_test_end;

/** Description of a TRC tag */
typedef struct te_rgt_mi_trc_tag_entry {
    char *name;   /**< Tag name */
    char *value;  /**< Tag value */
} te_rgt_mi_trc_tag_entry;

/** Description of MI message of type "trc_tags" */
typedef struct te_rgt_mi_trc_tags {
    te_vec tags; /**< Vector of TRC tags */
} te_rgt_mi_trc_tags;

/** Types of MI message */
typedef enum {
    TE_RGT_MI_TYPE_MEASUREMENT = 0,   /**< Measurement */
    TE_RGT_MI_TYPE_TEST_START,        /**< Package/Session/Test start */
    TE_RGT_MI_TYPE_TEST_END,          /**< Package/Session/Test end */
    TE_RGT_MI_TYPE_TRC_TAGS,          /**< TRC tags */
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
        te_rgt_mi_test_start test_start; /**< Data for test_start MI message */
        te_rgt_mi_test_end   test_end;   /**< Data for test_end MI message */

        te_rgt_mi_trc_tags trc_tags; /**< Data for trc_tags MI message */
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

/**
 * Get measurement parameter name (possibly derived from its type if
 * name property was left empty).
 *
 * @param param       Parameter.
 *
 * @return Pointer to constant string.
 */
extern const char *te_rgt_mi_meas_param_name(te_rgt_mi_meas_param *param);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_MI_MSG_H__ */
