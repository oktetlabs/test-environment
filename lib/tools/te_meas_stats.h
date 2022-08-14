/** @file
 * @brief Tools for statistics
 *
 * Tools for collecting statistical characteristics and
 * stabilization of samples.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_MEAS_STATS_H__
#define __TE_MEAS_STATS_H__

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TE_MEAS_STATS_DEFAULT_DEVIATION_COEFF 3

#define TE_MEAS_STATS_CONTINUE(meas_stats)                                     \
        ((meas_stats) != NULL &&                                               \
         ((meas_stats)->data.num_datapoints + (meas_stats)->num_zeros) <       \
         (meas_stats)->data.max_num_datapoints &&                              \
         (!(meas_stats)->stab_required ||                                      \
         !te_meas_stats_stab_is_stable(&(meas_stats)->stab,                    \
                                       &(meas_stats)->data)))

#define TE_MEAS_STATS_UPDATE_FAILED(uc)               \
        ((uc) == TE_MEAS_STATS_UPDATE_OUT_OF_RANGE || \
         (uc) == TE_MEAS_STATS_UPDATE_NOMEM)


/** Statistics collector behavior configuration flags */
typedef enum {
    TE_MEAS_STATS_INIT_SUMMARY_REQUIRED = 1,      /**< Summary structure is
                                                       required */
    TE_MEAS_STATS_INIT_STAB_REQUIRED    = 1 << 1, /**< Stabilization structure
                                                       is required */
    TE_MEAS_STATS_INIT_IGNORE_ZEROS     = 1 << 2  /**< Ignore leading zero
                                                       datapoints */
} te_meas_stats_init_flags;

typedef int te_meas_stats_update_code;

/** Codes returned after updating te_meas_stats structures with new datapoint */
typedef enum {
    TE_MEAS_STATS_UPDATE_SUCCESS,
    TE_MEAS_STATS_UPDATE_OUT_OF_RANGE,
    TE_MEAS_STATS_UPDATE_STABLE,
    TE_MEAS_STATS_UPDATE_NOT_STABLE,
    TE_MEAS_STATS_UPDATE_NOMEM
} te_meas_stats_update_codes;

/** Structure with main statistical characteristics of sample */
typedef struct te_meas_stats_data_t {
    unsigned int num_datapoints;
    unsigned int max_num_datapoints;
    double mean;
    double tss;                      /**< Total sum of squares of differences
                                          from mean */
    double cv;
    double *sample;
} te_meas_stats_data_t;

/** Data for stabilization of sample */
typedef struct te_meas_stats_stab_t {
    te_meas_stats_data_t correct_data; /**< Contains sample with skipped
                                            incorrect datapoints */
    double required_cv;
    unsigned int min_num_datapoints;
    unsigned int allowed_skips;
    double deviation_coeff;            /** Used to determine incorrect
                                           sample datapoint */
} te_meas_stats_stab_t;

/** Summary of sample, e.g. its histogram */
typedef struct te_meas_stats_summary_t {
    double *freq;
    double *bin_edges;           /**< Histogram bin edges or in case of small
                                      number of sample unique datapoints -
                                      sample's each unique datapoint */
    double **sample_deviation;   /**< For each pair of datapoint and prefixed
                                      subsample contains ratio of datapoint
                                      deviation from subsample mean to
                                      subsample standard deviation */
    unsigned int bin_edges_num;
    unsigned int freq_size;
} te_meas_stats_summary_t;

/** Structure for providing both summary and stabilization by request */
typedef struct te_meas_stats_t {
    te_bool stab_required;
    te_bool summary_required;
    te_bool ignore_zeros;
    te_bool nonzero_reached;
    unsigned int num_zeros;
    te_meas_stats_data_t data;
    te_meas_stats_stab_t stab;
    te_meas_stats_summary_t summary;
} te_meas_stats_t;

/**
 * Update te_meas_stats_data_t sample mean and sum of squares of
 * differences from mean with new datapoint
 *
 * @param data             Pointer to te_meas_stats_data_t structure
 * @param new_datapoint    New datapoint
 *
 * @alg     see Welford's algorithm
 */
static inline void
te_meas_stats_update_mean_and_tss(te_meas_stats_data_t *data,
                                 double new_datapoint)
{
    double delta1;
    double delta2;

    data->num_datapoints++;
    delta1 = new_datapoint - data->mean;
    data->mean += delta1 / data->num_datapoints;
    delta2 = new_datapoint - data->mean;
    data->tss += delta1 * delta2;
}

/**
 * Calculate variance of te_meas_stats_data_t structure sample
 *
 * @param data     Pointer to te_meas_stats_data_t structure
 *
 * @return      Variance of te_meas_stats_data_t structure sample
 *
 * @alg     see Welford's algorithm
 */
static inline double
te_meas_stats_get_var(const te_meas_stats_data_t *data)
{
    double var = data->tss / data->num_datapoints;

    /*
     * Theoritically variance is always non-negative but due to possible loss of
     * precision with variance close to zero it might be evaluated to negative
     * value as long as long subtraction is envolved.
     */

    return (var > 0) ? var : 0;
}

/**
 * Calculate deviation of te_meas_stats_data_t sample
 *
 * @param data     Pointer to te_meas_stats_data_t structure
 *
 * @return      deviation of te_meas_stats_data_t sample
 */
static inline double
te_meas_stats_get_deviation(const te_meas_stats_data_t *data)
{
    return sqrt(te_meas_stats_get_var(data));
}

/**
 * Update CV of te_meas_stats_data_t structure
 *
 * @param data     Pointer to te_meas_stats_data_t structure
 */
static inline void
te_meas_stats_update_cv(te_meas_stats_data_t *data)
{
    data->cv = te_meas_stats_get_deviation(data) / data->mean;
}

/**
 * Check if te_meas_stats_stab_t sample is stable
 *
 * @param stab      Pointer to te_meas_stats_stab_t structure
 * @param data      Pointer to te_meas_stats_data_t structure
 *
 * @return      TRUE if te_meas_stats_stab_t sample is stable, FALSE otherwise
 */
static inline te_bool
te_meas_stats_stab_is_stable(const te_meas_stats_stab_t *stab,
                             const te_meas_stats_data_t *data)
{
    return (data->num_datapoints >= stab->min_num_datapoints &&
            stab->required_cv > stab->correct_data.cv) ?
            TRUE : FALSE;
}

/**
 * Check if sample datapoint may be skipped
 *
 * @param datapoint         sample datapoint
 * @param mean              sample mean
 * @param deviation         sample deviation
 * @param deviation_coeff   deviation coefficient
 *
 * @return     TRUE if datapoint may be skipped, FALSE otherwise
 *
 * @alg     Check if deviation of datapoint within deviation_coeff
 *          deviations from mean (see Chebyshev's inequality)
 */
static inline te_bool
te_meas_stats_is_datapoint_correct(double datapoint, double mean,
                                   double deviation, double deviation_coeff)
{
    return fabs(mean - datapoint) < deviation_coeff * deviation;
}

/**
 * Calculate number of bins by Sturge's rule
 *
 * @param num_datapoints    Size of sample
 *
 * @return     Number of bins
 */
static inline unsigned int
te_meas_stats_sturges_rule(unsigned int num_datapoints)
{
    return log2(num_datapoints) + 1;
}

/**
 * Calculate deviation of x from y in percentage
 *
 * @param x    deviating value
 * @param y    fixed value
 *
 * @return     deviation
 */
static inline double
te_meas_stats_value_deviation(double x, double y)
{
    return (x - y) * 100.0 / y;
}

/**
 * Allocates and initialize all fields of te_meas_stats_data_t structure.
 *
 * @param data                  Pointer to structure to initialize
 * @param max_num_datapoints    Maximum allowed datapoints
 *
 * @return      0 on success
 */
te_errno te_meas_stats_data_init(te_meas_stats_data_t *data,
                                 unsigned int max_num_datapoints);

/**
 * Free te_meas_stats_data_t structure's resources
 *
 * @param data      Pointer to te_meas_stats_data_t structure
 */
void te_meas_stats_data_free(te_meas_stats_data_t *data);

/**
 * Initialize te_meas_stats_stab_t structure.
 *
 * @param stab                  Pointer to structure to initialize
 * @param data                  Pointer to te_meas_stats_data_t structure
 * @param min_num_datapoints    Minimum datapoints for stabilization
 * @param req_cv                CV required for stabilization
 * @param allowed_skips         Incorrect datapoints allowed to skip
 * @param deviation_coeff       Coefficient used to determine incorrect
 *                              datapoints, if less than zero then the
 *                              corresponding field initialized with
 *                              TE_MEAS_STATS_DEFAULT_DEVIATION_COEFF
 *
 * @return      0 on success
 */
te_errno te_meas_stats_stab_init(te_meas_stats_stab_t *stab,
                                 te_meas_stats_data_t *data,
                                 unsigned int min_num_datapoints, double req_cv,
                                 unsigned int allowed_skips,
                                 double deviation_coeff);

/**
 * Free te_meas_stats_stab_t structure's resources
 *
 * @param stab     Pointer to te_meas_stats_stab_t structure
 */
void te_meas_stats_stab_free(te_meas_stats_stab_t *stab);

/**
 * Initialize te_meas_stats_t structure, allocate and initialize its stats_stab
 * and stats_summary fields if corresponding flags are specified.
 *
 * @param meas_stats            Pointer to structure to initialize
 * @param max_num_datapoints    Maximum allowed iterations
 * @param flags                 Bitwise-ORed values of te_meas_stats_init_flags
 * @param min_num_datapoints    Minimum datapoints for stabilization, ignored
 *                              without TE_MEAS_STATS_INIT_STAB_REQUIRED
 *                              flag specified
 * @param req_cv                CV required for stabilization, ignored
 *                              without TE_MEAS_STATS_INIT_STAB_REQUIRED
 *                              flag specified
 * @param allowed_skips         Incorrect datapoints allowed to skip, ignored
 *                              without TE_MEAS_STATS_INIT_STAB_REQUIRED
 *                              flag specified
 * @param deviation_coeff       Coefficient used to determine incorrect
 *                              datapoints, if less than zero then the
 *                              corresponding field initialized with
 *                              TE_MEAS_STATS_DEFAULT_DEVIATION_COEFF. Ignored
 *                              without TE_MEAS_STATS_INIT_STAB_REQUIRED
 *                              flag specified
 *
 * @return      0 on success
 */
te_errno te_meas_stats_init(te_meas_stats_t *meas_stats,
                            unsigned int max_num_datapoints, int flags,
                            unsigned int min_num_datapoints, double req_cv,
                            unsigned int allowed_skips,
                            double deviation_coeff);

/**
 * Free te_meas_stats_t structure's resources
 *
 * @param meas_stats    Pointer to te_meas_stats_t structure
 */
void te_meas_stats_free(te_meas_stats_t *meas_stats);

/**
 * Free te_meas_stats_summary_t structure's resources
 *
 * @param summary     Pointer to te_meas_summary_stab_t structure
 */
void te_meas_stats_summary_free(te_meas_stats_summary_t *summary);

/**
 * Update te_meas_stats_stab_t structure's stats_data and correct_stats_data
 * with new datapoint
 *
 * @param stab              Pointer to te_meas_stats_stab_t structure
 * @param data              Pointer to stats_data structure of stats_stab
 * @param new_datapoint     New datapoint
 *
 * @return      status of update
 */
te_meas_stats_update_code te_meas_stats_stab_update(te_meas_stats_stab_t *stab,
                                                    te_meas_stats_data_t *data,
                                                    double new_datapoint);

/**
 * Update te_meas_stats_t structure with new datapoint.
 * Note that in worst cases square of new datapoint may be calculated
 * inside of this function call.
 *
 * @param meas_stats        Pointer to te_meas_stats_t structure
 * @param new_datapoint     New datapoint
 *
 * @return      status of update
 */
te_meas_stats_update_code te_meas_stats_update(te_meas_stats_t *meas_stats,
                                               double new_datapoint);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
