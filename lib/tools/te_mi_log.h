/** @file
 * @brief Machine interface data logging
 *
 * @defgroup te_tools_te_mi_log Machine interface data logging
 * @ingroup te_tools
 * @{
 *
 * Definition of API for machine interface data logging.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
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
 * Create a MI logger entity.
 *
 * @param[in]  type     The type of the logger
 * @param[out] logger   Created logger
 *
 * @return              Status code
 */
extern te_errno te_mi_logger_create(te_mi_type type, te_mi_logger **logger);

/**
 * Add a comment to a MI logger.
 *
 * @param           logger      MI logger
 * @param[in]       name        Name of the comment
 * @param[in]       value_fmt   Format string for the value of the comment
 * @param[in]       ...         Format string arguments
 *
 * @return                      Status code
 * @retval TE_EEXIST            The name already exists
 */
extern te_errno te_mi_logger_add_comment(te_mi_logger *logger, const char *name,
                                         const char *value_fmt, ...)
                                         __attribute__((format(printf, 3, 4)));

/**
 * Purge the logger's MI data. The data is lost completely.
 *
 * @param           logger      MI logger
 */
extern void te_mi_logger_reset(te_mi_logger *logger);

/**
 * Log the MI data of a logger as an ARTIFACT with deficated MI log
 * level and remove the data from the logger.
 * If the logger does not have any MI data, nothing is logged.
 * Logger is ready to add new data after the call to the function.
 *
 * @param           logger      MI logger
 *
 * @return                      Status code
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
 * Type of a measurement aggregation.
 * First enum element must have value @c 0.
 */
typedef enum te_mi_meas_aggr {
    /** Single measurement */
    TE_MI_MEAS_AGGR_SINGLE = 0,
    /** Measurement with the minimal value */
    TE_MI_MEAS_AGGR_MIN,
    /** Measurement with the maximum value */
    TE_MI_MEAS_AGGR_MAX,
    /** Average value of a measurement */
    TE_MI_MEAS_AGGR_MEAN,
    /** Coefficient of Variation of measurements */
    TE_MI_MEAS_AGGR_CV,

    /** One past last valid measurement aggregation type */
    TE_MI_MEAS_AGGR_END,
} te_mi_meas_aggr;

/** Type of a measurement. First enum element must have value @c 0. */
typedef enum te_mi_meas_type {
    TE_MI_MEAS_PPS = 0, /**< Packets per second */
    TE_MI_MEAS_LATENCY, /**< Latency in seconds */
    TE_MI_MEAS_THROUGHPUT, /**< Throughput in bits per second */
    TE_MI_MEAS_TEMP, /**< Temperature in degrees Celsius */

    TE_MI_MEAS_END, /**< End marker for a measurement vector.
                         Also is one past last valid type */
} te_mi_meas_type;

/** Scale of units of a result. First enum element must have value @c 0. */
typedef enum te_mi_meas_units {
    TE_MI_MEAS_UNITS_NANO = 0, /**< 10^(-9) */
    TE_MI_MEAS_UNITS_MICRO, /**< 10^(-6) */
    TE_MI_MEAS_UNITS_MILLI, /**< 10^(-3) */
    TE_MI_MEAS_UNITS_PLAIN, /**< 1 */
    TE_MI_MEAS_UNITS_KILO, /**< 10^(3) */
    TE_MI_MEAS_UNITS_KIBI, /**< 2^(10) */
    TE_MI_MEAS_UNITS_MEGA, /**< 10^(6) */
    TE_MI_MEAS_UNITS_MEBI, /**< 2^(20) */
    TE_MI_MEAS_UNITS_GIGA, /**< 10^(9) */
    TE_MI_MEAS_UNITS_GIBI, /**< 2^(30) */

    /** One past last valid unit */
    TE_MI_MEAS_UNITS_END,
} te_mi_meas_units;

/** Measurement */
typedef struct te_mi_meas {
    /** Measurement type */
    te_mi_meas_type type;
    /** Measurement name */
    const char *name;
    /** Measurement aggregation */
    te_mi_meas_aggr aggr;
    /** Measurement value */
    double val;
    /** Units of the measurement */
    te_mi_meas_units units;
} te_mi_meas;

/** Convenience te_mi_meas constructor */
#define TE_MI_MEAS(_type, _name, _aggr, _val, _units)   \
    (te_mi_meas){ TE_MI_MEAS_ ## _type, (_name),        \
                  TE_MI_MEAS_AGGR_ ## _aggr, (_val),    \
                  TE_MI_MEAS_UNITS_ ## _units }

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
 * te_mi_log_meas(TE_MI_MEAS_V(TE_MI_MEAS(PPS, "pps", MIN, 42.4, PLAIN),
 *                             TE_MI_MEAS(PPS, "pps", MEAN, 300, PLAIN),
 *                             TE_MI_MEAS(PPS, "pps", CV, 10, PLAIN),
 *                             TE_MI_MEAS(PPS, "pps", MAX, 5.4, KILO),
 *                             TE_MI_MEAS(TEMP, "mem", SINGLE, 42.4, PLAIN),
 *                             TE_MI_MEAS(LATENCY, "low", MEAN, 54, MICRO)),
 *                TE_MI_MEAS_KEYS({"key", "value"}),
 *                TE_MI_COMMENTS({"comment", "comment_value"}));
 * @endcode
 *
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
extern te_errno te_mi_log_meas(const te_mi_meas *measurements,
                               const te_mi_log_kvpair *keys,
                               const te_mi_log_kvpair *comments);

/**
 * Add a measurement result to a MI logger. Results are aggregated by
 * @p type - @p name pair. Multiple results with the the same
 * @p type - @p name pair are allowed.
 *
 * @param           logger      MI logger
 * @param[in]       type        The type of the measurement result
 * @param[in]       name        The name of the measurement result used to
 *                              identify measurements in logs, may be @c NULL
 * @param[in]       aggr        Aggregation type of the measurement
 * @param[in]       val         Value of the measurement
 * @param[in]       units       Units of the measurement value
 *
 * @return                      Status code
 */
extern te_errno te_mi_logger_add_meas(te_mi_logger *logger,
                                      te_mi_meas_type type, const char *name,
                                      te_mi_meas_aggr aggr, double val,
                                      te_mi_meas_units units);

/**
 * Variation of te_mi_logger_add_result() function that accepts
 * measurement as a struct.
 *
 * @param           logger      MI logger
 * @param[in]       meas        Measurement
 *
 * @return                      Status code
 */
extern te_errno te_mi_logger_add_meas_obj(te_mi_logger *logger,
                                          const te_mi_meas *meas);

/**
 * Add a measurement key to a MI logger. Measurement key is a key-value pair
 * that represents extra measurement information.
 *
 * @param           logger      MI logger
 * @param[in]       key         Measurement key
 * @param[in]       value_fmt   Format string for the value of the
 *                              measurement key
 * @param[in]       ...         Format string arguments
 *
 * @return                      Status code
 * @retval TE_EEXIST            The key already exists
 */
extern te_errno te_mi_logger_add_meas_key(te_mi_logger *logger, const char *key,
                                          const char *value_fmt, ...)
                                         __attribute__((format(printf, 3, 4)));
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
 * CHECK_RC(te_mi_logger_create(TE_MI_TYPE_MEASUREMENT, &logger));
 * for (i = 0; i < 10; i++)
 * {
 *    double pps;
 *
 *    sleep(1);
 *    pps = tool_get_pps();
 *    tool_reset_pps();
 *    CHECK_RC(te_mi_logger_add_meas(logger, TE_MI_MEAS_PPS, "pps",
 *                                   TE_MI_MEAS_AGGR_SINGLE, pps,
 *                                   TE_MI_MEAS_UNITS_PLAIN));
 * }
 *
 * CHECK_RC(te_mi_logger_add_meas_key(logger, "ver", "%s",
 *                                    tool_get_version_str()));
 * CHECK_RC(te_mi_logger_add_comment(logger, "fake", "true"));
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
