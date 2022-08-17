/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tools for statistics
 *
 * Tools for collecting statistical characteristics and
 * stabilization of samples.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_meas_stats.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"

/**
 * Count number of unique values in sorted array
 *
 * @param arr          Sorted array
 * @param arr_size     Size of array
 *
 * @return     Number of unique values
 */
static unsigned int
te_meas_stats_unique_values(double *arr, unsigned int arr_size)
{
    double *val;
    unsigned int unique_values_num = 1;

    val = arr;

    while (++val < arr + arr_size)
    {
        if (*val != *(val - 1))
            unique_values_num++;
    }

    return unique_values_num;
}

static int
te_meas_stats_double_cmp(const void *a, const void *b)
{
    const double *val_a = a;
    const double *val_b = b;

    if (*val_a > *val_b)
        return 1;

    if (*val_a < *val_b)
        return -1;

    return 0;
}

/**
 * Update statistical characteristics of te_meas_stats_data_t structure
 * with new datapoint
 *
 * @param data              Pointer to te_meas_stats_data_t structure
 * @param new_datapoint     New datapoint
 *
 * @return      status of update
 */
static te_meas_stats_update_code
te_meas_stats_data_update(te_meas_stats_data_t *data, double new_datapoint)
{
    if (data->num_datapoints >= data->max_num_datapoints)
        return TE_MEAS_STATS_UPDATE_OUT_OF_RANGE;

    data->sample[data->num_datapoints] = new_datapoint;
    te_meas_stats_update_mean_and_tss(data, new_datapoint);
    te_meas_stats_update_cv(data);

    return TE_MEAS_STATS_UPDATE_SUCCESS;
}

/**
 * Allocate and initialize histogram fields of summary by current state of
 * sample in data
 *
 * @param summary       Pointer to te_meas_stats_summary_t structure
 * @param data          Pointer to te_meas_stats_data_t structure
 *
 * @return      0 on success, TE_ENOMEM if allocation failed
 */
static te_errno
te_meas_stats_fill_summary_histogram(te_meas_stats_summary_t *summary,
                                     te_meas_stats_data_t *data)
{
    unsigned int sample_size;
    double *sorted_sample = NULL;
    double *bin_edges = NULL;
    unsigned int unique_values_num;
    unsigned int bin_edges_num = 0;
    te_bool use_sample_bucket;
    double *freq = NULL;
    unsigned int freq_size = 0;
    double min_sample_val;
    double max_sample_val;
    double bin_width;
    unsigned int i;
    double *val;
    te_errno rc = 0;

    sample_size = data->num_datapoints;

    if ((sorted_sample = TE_ALLOC(sample_size * sizeof(*sorted_sample))) ==
        NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    memcpy(sorted_sample, data->sample, sample_size * sizeof(*sorted_sample));
    qsort(sorted_sample, sample_size, sizeof(*sorted_sample),
          te_meas_stats_double_cmp);

    unique_values_num = te_meas_stats_unique_values(sorted_sample, sample_size);
    use_sample_bucket = TRUE;

    if ((bin_edges_num = te_meas_stats_sturges_rule(sample_size) + 1) >
        unique_values_num)
    {
        bin_edges_num = unique_values_num;
        use_sample_bucket = FALSE;
    }

    if ((bin_edges = TE_ALLOC(bin_edges_num * sizeof(*bin_edges))) == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    freq_size = use_sample_bucket ? bin_edges_num - 1 : bin_edges_num;

    if ((freq = TE_ALLOC(freq_size * sizeof(*freq))) == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    if (use_sample_bucket)
    {
        min_sample_val = sorted_sample[0];
        max_sample_val = sorted_sample[sample_size - 1];
        bin_width = (max_sample_val - min_sample_val) / (bin_edges_num - 1);

        for (i = 0; i < bin_edges_num; i++)
            bin_edges[i] = min_sample_val + bin_width * i;

        val = sorted_sample - 1;
        i = 0;

        while (++val < sorted_sample + sample_size)
        {
            if (i == bin_edges_num - 2)
            {
                freq[i]++;
                continue;
            }

            if (bin_edges[i] <= *val && *val < bin_edges[i + 1])
                freq[i]++;
            else
            {
                i++;
                val--;
            }
        }
    }
    else
    {
        i = 0;
        freq[0] = 1;
        bin_edges[0] = sorted_sample[0];
        val = sorted_sample;

        while (++val < sorted_sample + sample_size)
        {
            if (*val != *(val - 1))
            {
                i++;
                freq[i] = 1;
                bin_edges[i] = *val;
            }
            else
            {
                freq[i]++;
            }
        }
    }

    for (i = 0; i < freq_size; i++)
        freq[i] /= sample_size;

out:
    summary->bin_edges = bin_edges;
    summary->freq = freq;
    summary->bin_edges_num = bin_edges_num;
    summary->freq_size = freq_size;

    free(sorted_sample);

    return rc;
}

/**
 * Allocate and initialize sample deviation field of summary by current state
 * of sample in data
 *
 * @param summary       Pointer to te_meas_stats_summary_t structure
 * @param data          Pointer to te_meas_stats_data_t structure
 *
 * @return      0 on success, TE_ENOMEM if allocation failed
 */
static te_errno
te_meas_stats_fill_summary_sample_deviation(te_meas_stats_summary_t *summary,
                                            te_meas_stats_data_t *data)
{
    te_meas_stats_t meas_stats;
    int rc = 0;
    unsigned int i;
    unsigned int j;

    rc = te_meas_stats_init(&meas_stats, data->num_datapoints, 0, 0, 0, 0, 0);
    if (rc != 0)
        goto out;

    summary->sample_deviation = TE_ALLOC(data->num_datapoints *
                                         sizeof(*summary->sample_deviation));
    if (summary->sample_deviation == NULL)
        goto out;

    *summary->sample_deviation = TE_ALLOC(data->num_datapoints *
                                          data->num_datapoints *
                                          sizeof(**summary->sample_deviation));
    if (*summary->sample_deviation == NULL)
        goto out;

    for (i = 0; i < data->num_datapoints; i++)
    {
        summary->sample_deviation[i] = *summary->sample_deviation +
                                       i * data->num_datapoints;
    }

    for (i = 0; i < data->num_datapoints; i++)
    {
        rc = te_meas_stats_update(&meas_stats, data->sample[i]);
        if (rc != 0)
            goto out;

        for (j = 0; j <= i; j++)
        {
            summary->sample_deviation[j][i] = (data->sample[j] -
                                               meas_stats.data.mean) /
                                              te_meas_stats_get_deviation(
                                                              &meas_stats.data);
        }
    }

out:
    te_meas_stats_free(&meas_stats);
    return rc;
}

/**
 * Allocate and initialize summary fields of summary by current state of
 * sample in data
 *
 * @param summary       Pointer to te_meas_stats_summary_t structure
 * @param data          Pointer to te_meas_stats_data_t structure
 *
 * @return      0 on success, TE_ENOMEM if allocation failed
 */
static te_errno
te_meas_stats_fill_summary(te_meas_stats_summary_t *summary,
                           te_meas_stats_data_t *data)
{
    int code;

    if ((code = te_meas_stats_fill_summary_histogram(summary, data)) != 0)
        return code;

    if ((code = te_meas_stats_fill_summary_sample_deviation(summary, data))
        != 0)
        return code;

    return 0;
}

te_meas_stats_update_code
te_meas_stats_stab_update(te_meas_stats_stab_t *stab,
                          te_meas_stats_data_t *data,
                          double new_datapoint)
{
    double mean;
    double deviation;
    unsigned int i;
    int rc;

    if (data->num_datapoints > stab->min_num_datapoints)
    {
        mean = stab->correct_data.mean;
        deviation = te_meas_stats_get_deviation(&stab->correct_data);

        if (!te_meas_stats_is_datapoint_correct(new_datapoint, mean,
                                                deviation,
                                                stab->deviation_coeff) &&
            stab->allowed_skips > 0)
        {
            stab->allowed_skips--;
        }
        else
        {
            rc = te_meas_stats_data_update(&stab->correct_data, new_datapoint);
            if (rc != TE_MEAS_STATS_UPDATE_SUCCESS)
                return rc;
        }
    }
    else if (data->num_datapoints == stab->min_num_datapoints)
    {
        deviation = te_meas_stats_get_deviation(data);
        mean = data->mean;

        for (i = 0; i < data->num_datapoints; i++)
        {
            if (!te_meas_stats_is_datapoint_correct(data->sample[i], mean,
                                                    deviation,
                                                    stab->deviation_coeff) &&
                 stab->allowed_skips > 0)
            {
                 stab->allowed_skips--;
                 continue;
            }

            rc = te_meas_stats_data_update(&stab->correct_data,
                                           data->sample[i]);
            if (rc != TE_MEAS_STATS_UPDATE_SUCCESS)
                return rc;
        }
    }

    return te_meas_stats_stab_is_stable(stab, data) ?
           TE_MEAS_STATS_UPDATE_STABLE :
           TE_MEAS_STATS_UPDATE_NOT_STABLE;
}

te_errno
te_meas_stats_data_init(te_meas_stats_data_t *data,
                        unsigned int max_num_datapoints)
{
    memset(data, 0, sizeof(*data));
    data->max_num_datapoints = max_num_datapoints;

    data->sample = TE_ALLOC(sizeof(*data->sample) * max_num_datapoints);
    if (data->sample == NULL)
       return TE_ENOMEM;

    return 0;
}

void
te_meas_stats_data_free(te_meas_stats_data_t *data)
{
    free(data->sample);
}

te_errno
te_meas_stats_stab_init(te_meas_stats_stab_t *stab,
                        te_meas_stats_data_t *data,
                        unsigned int min_num_datapoints, double req_cv,
                        unsigned int allowed_skips,
                        double deviation_coeff)
{
    memset(stab, 0, sizeof(*stab));
    stab->min_num_datapoints = min_num_datapoints;
    stab->required_cv = req_cv;
    stab->allowed_skips = allowed_skips;
    stab->deviation_coeff = deviation_coeff >= 0 ?
                            deviation_coeff :
                            TE_MEAS_STATS_DEFAULT_DEVIATION_COEFF;

    return te_meas_stats_data_init(&stab->correct_data,
                                   data->max_num_datapoints);
}

void
te_meas_stats_stab_free(te_meas_stats_stab_t *stab)
{
    te_meas_stats_data_free(&stab->correct_data);
}

te_errno
te_meas_stats_init(te_meas_stats_t *meas_stats,
                   unsigned int max_num_datapoints, int flags,
                   unsigned int min_num_datapoints, double req_cv,
                   unsigned int allowed_skips,
                   double deviation_coeff)
{
    te_errno rc;

    memset(meas_stats, 0, sizeof(*meas_stats));

    rc = te_meas_stats_data_init(&meas_stats->data, max_num_datapoints);
    if (rc != 0)
        return rc;

    meas_stats->stab_required = (flags & TE_MEAS_STATS_INIT_STAB_REQUIRED) != 0;
    meas_stats->summary_required = (flags &
                                    TE_MEAS_STATS_INIT_SUMMARY_REQUIRED) != 0;
    meas_stats->ignore_zeros = (flags & TE_MEAS_STATS_INIT_IGNORE_ZEROS) != 0;
    meas_stats->nonzero_reached = FALSE;

    if (meas_stats->stab_required)
    {
        rc = te_meas_stats_stab_init(&meas_stats->stab, &meas_stats->data,
                                     min_num_datapoints, req_cv, allowed_skips,
                                     deviation_coeff);
        if (rc != 0)
            return rc;
    }

    return 0;
}

void
te_meas_stats_free(te_meas_stats_t *meas_stats)
{
    te_meas_stats_data_free(&meas_stats->data);
    te_meas_stats_stab_free(&meas_stats->stab);
    te_meas_stats_summary_free(&meas_stats->summary);
}

void
te_meas_stats_summary_free(te_meas_stats_summary_t *summary)
{
    free(summary->freq);
    free(summary->bin_edges);
    if (summary->sample_deviation != NULL)
        free(*summary->sample_deviation);
    free(summary->sample_deviation);
}

te_meas_stats_update_code
te_meas_stats_update(te_meas_stats_t *meas_stats, double new_datapoint)
{
    te_meas_stats_update_code code1;
    te_errno code2;

    if (meas_stats->ignore_zeros && !meas_stats->nonzero_reached)
    {
        if (new_datapoint != 0)
        {
           meas_stats->nonzero_reached = TRUE;
        }
        else
        {
            meas_stats->num_zeros++;
            return TE_MEAS_STATS_UPDATE_SUCCESS;
        }
    }

    if ((code1 = te_meas_stats_data_update(&meas_stats->data, new_datapoint)) !=
         TE_MEAS_STATS_UPDATE_SUCCESS)
    {
        return code1;
    }

    if (meas_stats->stab_required)
        code1 = te_meas_stats_stab_update(&meas_stats->stab,
                                          &meas_stats->data,
                                          new_datapoint);

    if (meas_stats->summary_required &&
        (code1 == TE_MEAS_STATS_UPDATE_STABLE ||
         (meas_stats->data.num_datapoints + meas_stats->num_zeros) ==
         meas_stats->data.max_num_datapoints))
    {
        code2  = te_meas_stats_fill_summary(&meas_stats->summary,
                                            &meas_stats->data);
        if (code2 == TE_ENOMEM)
            return TE_MEAS_STATS_UPDATE_NOMEM;
    }

    return code1;
}
