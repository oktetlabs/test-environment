/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Machine interface data logging
 *
 * @defgroup te_tools_te_mi_log Machine interface data logging
 * @ingroup te_tools
 * @{
 *
 * Definition of API for machine interface data logging.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TOOLS_TE_MI_LOG_H__
#define __TE_TOOLS_TE_MI_LOG_H__

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Type of a MI data. First enum element must have value @c 0. */
typedef enum te_mi_type {
    /** Logging of measurement results */
    TE_MI_TYPE_MEASUREMENT = 0,

    /** One past last valid MI data type */
    TE_MI_TYPE_END,
} te_mi_type;

/**
 * MI logger entity that is responsible for internally storing MI data,
 * and logging it.
 */
typedef struct te_mi_logger te_mi_logger;

/**
 * Key-value pair that represents a comment to any MI data.
 * Comment is considered an additional MI data.
 * The struct also used to represent a measurement key.
 *
 * `te_kvpair` is not used here since it is a tail queue element which
 * is meant to be dynamically allocated. But te_mi_log_meas() API
 * is more convenient to use with an on-stack allocated array argument,
 * which can be easily constructed using `te_mi_log_kvpair`.
 */
typedef struct te_mi_log_kvpair {
    const char *key;
    const char *value;
} te_mi_log_kvpair;

/** Convenience kvpair vector constructor */
#define TE_MI_LOG_KVPAIRS(...) \
    (te_mi_log_kvpair []){__VA_ARGS__, { .key = NULL }}

/** Convenience comment vector constructor */
#define TE_MI_COMMENTS(...) TE_MI_LOG_KVPAIRS(__VA_ARGS__)

/**
 * Add a comment to a MI logger.
 *
 * @param           logger      MI logger
 * @param           retval      Return code. If @c NULL is passed, @p logger
 *                              is not @c NULL and error occures, error flag
 *                              is stored in @p logger, which will fail the
 *                              next te_mi_logger_flush().
 *                              TE_EEXIST means the name already exists.
 * @param[in]       name        Name of the comment
 * @param[in]       value_fmt   Format string for the value of the comment
 * @param[in]       ...         Format string arguments
 */
extern void te_mi_logger_add_comment(te_mi_logger *logger, te_errno *retval,
                                     const char *name,
                                     const char *value_fmt, ...)
                                     __attribute__((format(printf, 4, 5)));

/**
 * Purge the logger's MI data. The data is lost completely.
 * The flag that indicates that previously called logger manipulation functions
 * errors were ignored is cleared.
 *
 * @param           logger      MI logger
 */
extern void te_mi_logger_reset(te_mi_logger *logger);

/**
 * - If there were any ignored logger internal failures - return an error.
 * - Log the MI data of a logger as an ARTIFACT with deficated MI log level.
 * - Remove the data from the logger (calls te_mi_logger_reset()).
 *
 * If the logger does not have any MI data, nothing is logged.
 * Logger is ready to add new data after the call to the function.
 *
 * @note    The format of the MI log is JSON RFC 8259. The API passes
 *          the JSON string to logger without indentation.
 *
 * JSON-schema: see doc/drafts/mi-schema.json
 *
 * Example of a valid JSON:
 * {
 *   "type":"measurement",
 *   "version":1,
 *   "tool":"testpmd",
 *   "results":[
 *     {
 *       "type":"pps",
 *       "entries":[
 *         {
 *           "aggr":"mean",
 *           "value":7040400.000,
 *           "base_units":"pps",
 *           "multiplier":"1"
 *         },
 *         {
 *           "aggr":"cv",
 *           "value":0.000,
 *           "base_units":"",
 *           "multiplier":"1"
 *         }
 *       ]
 *     },
 *     {
 *       "type":"throughput",
 *       "entries":[
 *         {
 *           "aggr":"mean",
 *           "value":4731.149,
 *           "base_units":"bps",
 *           "multiplier":"1e+6"
 *         }
 *       ]
 *     },
 *     {
 *       "type":"bandwidth-usage",
 *       "entries":[
 *         {
 *           "aggr":"mean",
 *           "value":0.473,
 *           "base_units":"",
 *           "multiplier":"1"
 *         }
 *       ]
 *     }
 *   ],
 *   "keys":{
 *     "Side":"Rx"
 *   },
 *   "comments":{
 *     "Stabilizaton":"reached on datapoint (+ leading zero datapoints): 10 (+ 1)"
 *   }
 * }
 *
 * @param           logger      MI logger
 *
 * @return                      Status code
 * @retval TE_EFAIL             There were ignored internal logger failures
 */
extern te_errno te_mi_logger_flush(te_mi_logger *logger);

/**
 * Flush (te_mi_logger_flush()) the MI data of a logger and free the logger
 * itself.
 *
 * @param           logger      MI logger
 */
extern void te_mi_logger_destroy(te_mi_logger *logger);

/**
 * @defgroup te_tools_te_mi_log_meas MI measurement-specific declarations
 *
 * @{
 */

/**
 * Type of a measurement aggregation. The units of the measurement are
 * defined by measurement type, unless different units are specified by
 * aggregation explicilty.
 *
 * First enum element must have value @c 0.
 */
typedef enum te_mi_meas_aggr {
    /** Start of the specified values */
    TE_MI_MEAS_AGGR_START = 0,
    /** Single measurement */
    TE_MI_MEAS_AGGR_SINGLE,
    /** Measurement with the minimal value */
    TE_MI_MEAS_AGGR_MIN,
    /** Measurement with the maximum value */
    TE_MI_MEAS_AGGR_MAX,
    /** Average value of a measurement */
    TE_MI_MEAS_AGGR_MEAN,
    /** Median value of a measurements */
    TE_MI_MEAS_AGGR_MEDIAN,
    /**
     * Coefficient of Variation of measurements. The aggregation is
     * unit-independend and defined as the ratio of standard deviation to
     * the mean.
     */
    TE_MI_MEAS_AGGR_CV,
    /** Standard deviation of measurements */
    TE_MI_MEAS_AGGR_STDEV,
    /**
     * Number of measurements that are out of range. The aggregation is
     * unit-independend.
     */
    TE_MI_MEAS_AGGR_OUT_OF_RANGE,
    /**
     * N-th percentile of a measurements.
     * N must be specified in the measurement name.
     */
    TE_MI_MEAS_AGGR_PERCENTILE,

    /** One past last valid measurement aggregation type */
    TE_MI_MEAS_AGGR_END,

    /* Special values for aggregation enumeration. */

    /** The start of the special values */
    TE_MI_MEAS_AGGR_SV_START,
    /** Unspecified value. */
    TE_MI_MEAS_AGGR_SV_UNSPECIFIED,
    /** The end of the special values */
    TE_MI_MEAS_AGGR_SV_END ,
} te_mi_meas_aggr;

/** Type of a measurement. First enum element must have value @c 0. */
typedef enum te_mi_meas_type {
    TE_MI_MEAS_PPS = 0, /**< Packets per second */
    TE_MI_MEAS_LATENCY, /**< Latency in seconds */
    TE_MI_MEAS_THROUGHPUT, /**< Throughput in bits per second */
    TE_MI_MEAS_BANDWIDTH_USAGE, /**< Bandwidth usage ratio */
    TE_MI_MEAS_TEMP, /**< Temperature in degrees Celsius */
    TE_MI_MEAS_RPS, /**< Requests per second */
    TE_MI_MEAS_RTT, /**< Rount trip time in seconds */
    TE_MI_MEAS_RETRANS, /**< TCP retransmissons */
    TE_MI_MEAS_FREQ, /**< Events per seconds (Hz) */
    TE_MI_MEAS_EPE, /**< Events per another event */
    TE_MI_MEAS_IOPS, /**< Input/Output operations per second */

    TE_MI_MEAS_END, /**< End marker for a measurement vector.
                         Also is one past last valid type */
} te_mi_meas_type;

/**
 * Scale of a measurement result. The measurement value should be multiplied by
 * this to get value in base units.
 *
 * First enum element must have value @c 0.
 */
typedef enum te_mi_meas_multiplier {
    TE_MI_MEAS_MULTIPLIER_NANO = 0, /**< 10^(-9) */
    TE_MI_MEAS_MULTIPLIER_MICRO, /**< 10^(-6) */
    TE_MI_MEAS_MULTIPLIER_MILLI, /**< 10^(-3) */
    TE_MI_MEAS_MULTIPLIER_PLAIN, /**< 1 */
    TE_MI_MEAS_MULTIPLIER_KILO, /**< 10^(3) */
    TE_MI_MEAS_MULTIPLIER_KIBI, /**< 2^(10) */
    TE_MI_MEAS_MULTIPLIER_MEGA, /**< 10^(6) */
    TE_MI_MEAS_MULTIPLIER_MEBI, /**< 2^(20) */
    TE_MI_MEAS_MULTIPLIER_GIGA, /**< 10^(9) */
    TE_MI_MEAS_MULTIPLIER_GIBI, /**< 2^(30) */

    /** One past last valid multiplier */
    TE_MI_MEAS_MULTIPLIER_END,
} te_mi_meas_multiplier;

/**
 * Measurement. Base units of a measurement are defined by
 * measurement type and measurement aggregation.
 */
typedef struct te_mi_meas {
    /** Measurement type */
    te_mi_meas_type type;
    /** Measurement name */
    const char *name;
    /** Measurement aggregation */
    te_mi_meas_aggr aggr;
    /** Measurement value */
    double val;
    /** Scale of a measurement result */
    te_mi_meas_multiplier multiplier;
} te_mi_meas;

/**
 * Create a MI measurements logger entity.
 *
 * @param[in]  tool     Tool that was used for gathering measurements.
 * @param[out] logger   Created logger
 *
 * @return              Status code
 */
extern te_errno te_mi_logger_meas_create(const char *tool,
                                         te_mi_logger **logger);

/** Convenience te_mi_meas constructor */
#define TE_MI_MEAS(_type, _name, _aggr, _val, _multiplier)   \
    (te_mi_meas){ TE_MI_MEAS_ ## _type, (_name),        \
                  TE_MI_MEAS_AGGR_ ## _aggr, (_val),    \
                  TE_MI_MEAS_MULTIPLIER_ ## _multiplier }

/** Convenience te_mi_meas vector constructor for te_mi_log_meas() */
#define TE_MI_MEAS_V(...) \
    (te_mi_meas []){__VA_ARGS__, { .type = TE_MI_MEAS_END }}

/** Convenience measurement key vector constructor for te_mi_log_meas() */
#define TE_MI_MEAS_KEYS(...) TE_MI_LOG_KVPAIRS(__VA_ARGS__)

/**
 * Log MI measurements in one call. Logging steps:
 * - create a logger;
 * - add @p measurements, @p keys and @p comments to the logger;
 * - log the MI data;
 * - destroy the logger;
 *
 * usage example:
 * @code{.c}
 * te_mi_log_meas("mytool",
 *                TE_MI_MEAS_V(TE_MI_MEAS(PPS, "pps", MIN, 42.4, PLAIN),
 *                             TE_MI_MEAS(PPS, "pps", MEAN, 300, PLAIN),
 *                             TE_MI_MEAS(PPS, "pps", CV, 10, PLAIN),
 *                             TE_MI_MEAS(PPS, "pps", MAX, 5.4, KILO),
 *                             TE_MI_MEAS(TEMP, "mem", SINGLE, 42.4, PLAIN),
 *                             TE_MI_MEAS(LATENCY, "low", MEAN, 54, MICRO)),
 *                TE_MI_MEAS_KEYS({"key", "value"}),
 *                TE_MI_COMMENTS({"comment", "comment_value"}));
 * @endcode
 *
 * @param[in]  tool             Tool that was used for gathering measurements.
 * @param[in]  measurements     Measurements vector. The last element must have
 *                              type @c TE_MI_MEAS_END.
 * @param[in]  keys             Measurement keys vector. The last element must
 *                              have key @c NULL. May be @c NULL to not add any
 *                              keys.
 * @param[in]  comments         Comments vector. The last element must have
 *                              key @c NULL. May be @c NULL to not add any
 *                              comments.
 *
 * @return                      Status code
 */
extern te_errno te_mi_log_meas(const char *tool,
                               const te_mi_meas *measurements,
                               const te_mi_log_kvpair *keys,
                               const te_mi_log_kvpair *comments);

/**
 * Add a measurement result to a MI logger. Results are aggregated by
 * @p type - @p name pair. Multiple results with the the same
 * @p type - @p name pair are allowed.
 *
 * @param           logger      MI logger
 * @param           retval      Return code. If @c NULL is passed, @p logger
 *                              is not @c NULL and error occures, error flag
 *                              is stored in @p logger, which will fail the
 *                              next te_mi_logger_flush().
 * @param[in]       type        The type of the measurement result
 * @param[in]       name        The name of the measurement result used to
 *                              identify measurements in logs, may be @c NULL
 * @param[in]       aggr        Aggregation type of the measurement
 * @param[in]       val         Value of the measurement
 * @param[in]       multiplier  Scale of the measurement value
 */
extern void te_mi_logger_add_meas(te_mi_logger *logger, te_errno *retval,
                                  te_mi_meas_type type, const char *name,
                                  te_mi_meas_aggr aggr, double val,
                                  te_mi_meas_multiplier multiplier);

/**
 * Variation of te_mi_logger_add_result() function that accepts
 * measurement as a struct.
 *
 * @param           logger      MI logger
 * @param           retval      Return code. If @c NULL is passed, @p logger
 *                              is not @c NULL and error occures, error flag
 *                              is stored in @p logger, which will fail the
 *                              next te_mi_logger_flush().
 * @param[in]       meas        Measurement
 *
 * @return                      Status code
 */
extern void te_mi_logger_add_meas_obj(te_mi_logger *logger, te_errno *retval,
                                      const te_mi_meas *meas);

/**
 * Add an vector of measurements to a MI logger.
 * Insertion stops on the first error (the rest of the measurements are not
 * added and the successful insertions persist in logger).
 *
 * @param           logger          MI logger
 * @param           retval          Return code. If @c NULL is passed, @p logger
 *                                  is not @c NULL and error occures, error flag
 *                                  is stored in @p logger, which will fail the
 *                                  next te_mi_logger_flush().
 * @param[in]       measurements    Measurements vector. The last element must
 *                                  have type @c TE_MI_MEAS_END.
 *
 * @return                      Status code
 */
extern void te_mi_logger_add_meas_vec(te_mi_logger *logger, te_errno *retval,
                                      const te_mi_meas *measurements);

/**
 * Add a measurement key to a MI logger. Measurement key is a key-value pair
 * that represents extra measurement information.
 *
 * @param           logger      MI logger.
 * @param           retval      Return code. If @c NULL is passed, @p logger
 *                              is not @c NULL and error occurs, error flag
 *                              is stored in @p logger, which will fail the
 *                              next te_mi_logger_flush().
 * @param[in]       key         Measurement key
 * @param[in]       value_fmt   Format string for the value of the
 *                              measurement key
 * @param[in]       ...         Format string arguments
 */
extern void te_mi_logger_add_meas_key(te_mi_logger *logger, te_errno *retval,
                                      const char *key,
                                      const char *value_fmt, ...)
                                      __attribute__((format(printf, 4, 5)));

/** Types of MI measurement views */
typedef enum te_mi_meas_view_type {
    TE_MI_MEAS_VIEW_LINE_GRAPH = 0,    /**< 2D graph with lines */
    TE_MI_MEAS_VIEW_POINT,             /**< A single "point" representing
                                            a given MI artifact (like mean
                                            value) */
    TE_MI_MEAS_VIEW_MAX,               /**< One past last valid type */
} te_mi_meas_view_type;

/**
 * Add a view for MI measurement.
 *
 * @param logger      MI logger
 * @param retval      Return code. If @c NULL is passed, @p logger
 *                    is not @c NULL and error occures, error flag
 *                    is stored in @p logger, which will fail the
 *                    next te_mi_logger_flush().
 * @param type        View type.
 * @param name        View name (must be unique for a given type;
 *                    used for identifying a view when calling other
 *                    functions of this API).
 * @param title       View title (may be an empty string; used as a title
 *                    when displaying a graph).
 */
extern void te_mi_logger_add_meas_view(te_mi_logger *logger,
                                       te_errno *retval,
                                       te_mi_meas_view_type type,
                                       const char *name,
                                       const char *title);

/** Types of MI measurement view axis */
typedef enum te_mi_graph_axis {
    TE_MI_GRAPH_AXIS_X = 0,     /**< X-axis */
    TE_MI_GRAPH_AXIS_Y,         /**< Y-axis */
    TE_MI_GRAPH_AXIS_MAX,       /**< One past last valid value */
} te_mi_graph_axis;

/**
 * Special name meaning that sequence number should be used instead of
 * values of some measurement on a given graph axis.
 */
#define TE_MI_GRAPH_AUTO_SEQNO "auto-seqno"

/**
 * Add a measurement to a graph axis.
 *
 * @param logger      MI logger.
 * @param retval      Return code. If @c NULL is passed, @p logger
 *                    is not @c NULL and error occurs, error flag
 *                    is stored in @p logger, which will fail the
 *                    next te_mi_logger_flush().
 * @param view_type   View type.
 * @param view_name   Name of the graph view.
 * @param axis        Graph axis. For X axis only a single measurement
 *                    may (and must) be specified - or a special
 *                    @c TE_MI_GRAPH_AUTO_SEQNO keyword must be used as
 *                    described below.
 *                    Measurements specified for Y axis will each be
 *                    drawn as a separate line. All measurements must
 *                    have the same number of values as the measurement
 *                    specified for X axis.
 *                    If no measurement is specified for Y axis,
 *                    it behaves as if all measurements are assigned to
 *                    it except the one assigned to X axis.
 * @param meas_type   Measurement type. Using @c TE_MI_MEAS_END means
 *                    that no type is specified; in that case measurement
 *                    name must be unique (i.e. used only for a single
 *                    type). Also @c TE_MI_MEAS_END can be used for
 *                    @c TE_MI_GRAPH_AUTO_SEQNO (see @p meas_name).
 * @param meas_name   Name of the measurement. May be empty or @c NULL
 *                    if @p meas_type is specified, in which case
 *                    there should be the single parameter of a given
 *                    type.
 *                    For X axis @c TE_MI_GRAPH_AUTO_SEQNO can be used
 *                    instead of a measurement name. In that case
 *                    sequence number in array of values will be used
 *                    as X-coordinate on a graph.
 */
extern void te_mi_logger_meas_graph_axis_add(te_mi_logger *logger,
                                             te_errno *retval,
                                             te_mi_meas_view_type view_type,
                                             const char *view_name,
                                             te_mi_graph_axis axis,
                                             te_mi_meas_type meas_type,
                                             const char *meas_name);

/**
 * Wrapper for te_mi_logger_meas_graph_axis_add() which accepts only
 * measurement name (which must be unique among all measurements of
 * different types).
 */
static inline void te_mi_logger_meas_graph_axis_add_name(
                                             te_mi_logger *logger,
                                             te_errno *retval,
                                             te_mi_meas_view_type view_type,
                                             const char *view_name,
                                             te_mi_graph_axis axis,
                                             const char *meas_name)
{
    te_mi_logger_meas_graph_axis_add(logger, retval, view_type,
                                     view_name, axis,
                                     TE_MI_MEAS_END, meas_name);
}

/**
 * Wrapper for te_mi_logger_meas_graph_axis_add() which accepts only
 * measurement type (there must be a single parameter with NULL name
 * for a given type in such case).
 */
static inline void te_mi_logger_meas_graph_axis_add_type(
                                             te_mi_logger *logger,
                                             te_errno *retval,
                                             te_mi_meas_view_type view_type,
                                             const char *view_name,
                                             te_mi_graph_axis axis,
                                             te_mi_meas_type meas_type)
{
    te_mi_logger_meas_graph_axis_add(logger, retval, view_type,
                                     view_name, axis,
                                     meas_type, NULL);
}

/**
 * Add a point view representing MI measurement by a single point
 * (mean, maximum value, etc). This view can be used for performance
 * history graphs showing such 'points' taken from multiple logs
 * on a single graph.
 *
 * Example:
 * @code
 *
 * te_mi_logger *logger;
 *
 * CHECK_RC(te_mi_logger_meas_create("mytool", &logger));
 * te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, "MeasName",
 *                       TE_MI_MEAS_AGGR_MEAN, 10,
 *                       TE_MI_MEAS_MULTIPLIER_NANO);
 *
 * te_mi_logger_add_meas_view(logger, NULL, TE_MI_MEAS_VIEW_POINT, "ViewName",
 *                            "ViewTitile");
 * te_mi_logger_meas_point_add(logger, NULL, "ViewName", TE_MI_MEAS_LATENCY,
 *                              "MeasName", TE_MI_MEAS_AGGR_MEAN);
 *
 * te_mi_logger_destroy(logger);
 *
 * @endcode
 *
 * @param logger[in]    MI logger.
 * @param retval[out]   Return code. If @c NULL is passed, @p logger
 *                      is not @c NULL and error occurs, error flag
 *                      is stored in @p logger, which will fail the
 *                      next te_mi_logger_flush().
 * @param view_name[in] Name of the point view.
 * @param meas_type[in] Measurement type. Using @c TE_MI_MEAS_END means
 *                      that no type is specified; in that case measurement
 *                      name must be unique (i.e. used only for a single
 *                      type).
 * @param meas_name[in] Name of the measurement. May be empty or @c NULL
 *                      if @p meas_type is specified, in which case
 *                      there should be the single parameter of a given
 *                      type.
 * @param meas_aggr[in] Type of a measurement aggregation. Using @c
 *                      TE_MI_MEAS_AGGR_SF_UNPECIFIED means that aggr is not
 *                      specified, in that case the measurement that is
 *                      descibed by @p meas_name and @p meas_type must contain
 *                      only one value.
 */
extern void te_mi_logger_meas_point_add(te_mi_logger *logger,
                                        te_errno *retval,
                                        const char *view_name,
                                        te_mi_meas_type meas_type,
                                        const char *meas_name,
                                        te_mi_meas_aggr meas_aggr);

/**
 * @page te_tools_te_mi_log_scenarios Usage scenarios
 *
 * Log all performance measurements of a tool
 * ------------------------------------------
 * - Create a logger
 * - Add performance measurements
 * - Flush the logger data
 *
 * @code{.c}
 * te_mi_logger *logger;
 *
 * CHECK_RC(te_mi_logger_meas_create("mytool", &logger));
 * for (i = 0; i < 10; i++)
 * {
 *    double pps;
 *
 *    sleep(1);
 *    pps = tool_get_pps();
 *    tool_reset_pps();
 *    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_PPS, "pps",
 *                          TE_MI_MEAS_AGGR_SINGLE, pps,
 *                          TE_MI_MEAS_MULTIPLIER_PLAIN));
 * }
 *
 * te_mi_logger_add_meas_key(logger, NULL, "ver", "%s", tool_get_version_str());
 * te_mi_logger_add_comment(logger, NULL, "fake", "true");
 *
 * CHECK_RC(te_mi_logger_flush(logger));
 *
 * @endcode
 */

/**@} <!-- END te_tools_te_mi_log_meas --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TOOLS_TE_MI_LOG_H__ */
/**@} <!-- END te_tools_te_mi_log --> */
