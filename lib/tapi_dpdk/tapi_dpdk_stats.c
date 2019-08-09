/** @file
 * @brief DPDK statistics helper functions TAPI
 *
 * TAPI to handle DPDK-related operations with statistics (implementation)
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#include "te_config.h"

#include "te_ethernet.h"
#include "tapi_dpdk_stats.h"
#include "tapi_test_log.h"

static const char *
empty_string_if_null(const char *string)
{
    return string == NULL ? "" : string;
}

void
tapi_dpdk_stats_pps_artifact(uint64_t pps, const char *prefix)
{
    TEST_ARTIFACT("%sPPS: %lu", empty_string_if_null(prefix), pps);
}

uint64_t
tapi_dpdk_stats_calculate_l1_bitrate(uint64_t pps, unsigned int packet_size)
{
    /*
     * Assume the overhead size: 20 (Preamble + SOF + IPG) + 4 (FCS) = 24 bytes
     * per packet. IPG could be different, but the information is not present.
     */
    unsigned int overhead_size = 24;

    /* Packet is padded to the min ethernet frame not including CRC */
    packet_size = MAX(packet_size, ETHER_MIN_LEN - ETHER_CRC_LEN);

    return (packet_size + overhead_size) * 8U * pps;
}

void
tapi_dpdk_stats_l1_bitrate_artifact(uint64_t l1_bitrate, const char *prefix)
{
    TEST_ARTIFACT("%sL1 bit rate: %lu bit/s", empty_string_if_null(prefix),
                  l1_bitrate);
}

te_errno
tapi_dpdk_stats_calculate_l1_link_usage(uint64_t l1_bitrate,
                                        unsigned int link_speed,
                                        double *l1_link_usage)
{
    if (link_speed == 0)
    {
        ERROR("Link usage cannot be calculated when link speed is zero");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *l1_link_usage = (double)l1_bitrate /
                     (double)((uint64_t)link_speed * 1000000ULL);

    return 0;
}

void
tapi_dpdk_stats_l1_link_usage_artifact(double l1_link_usage, const char *prefix)
{
    TEST_ARTIFACT("%sL1 rate percent: %.3f", empty_string_if_null(prefix),
                  l1_link_usage * 100.0);
}

void
tapi_dpdk_stats_cv_artifact(double cv, const char *prefix)
{
    TEST_ARTIFACT("%sCV: %.3f%%", empty_string_if_null(prefix), cv * 100);
}

te_errno
tapi_dpdk_stats_summary_artifact(const te_meas_stats_t *meas_stats,
                                 const char *prefix)
{
    const te_meas_stats_data_t *data;
    const te_meas_stats_summary_t *summary;
    unsigned int freq_size;
    unsigned int bin_edges_num;
    te_string str = TE_STRING_INIT;
    int gather_rc = 0;
    unsigned int i;
    unsigned int j;
    char *fmt_str;

    data = &meas_stats->data;
    summary = &meas_stats->summary;
    if (summary->sample_deviation == NULL)
        return TE_EFAULT;

    gather_rc = te_string_append(&str, "%s%s",
                                empty_string_if_null(prefix),
                                "Datapoints and ratios of theirs devations "
                                "from prefixed subsample mean to subsample "
                                "deviation\n");
    if (gather_rc != 0)
        goto out;

    for (i = 0; i < data->num_datapoints; i++)
    {
        gather_rc = te_string_append(&str, "%u. %.0f\n{ ",
                                     i + 1, data->sample[i]);
        if (gather_rc != 0)
            goto out;

        for (j = 0; j < data->num_datapoints - i; j++)
        {
            fmt_str = "%u: %.2f, ";
            if (j == data->num_datapoints - i - 1)
                fmt_str = "%u: %.3f }\n";

            gather_rc = te_string_append(&str, fmt_str,
                                         j + i + 1,
                                         summary->sample_deviation[i][j + i]);
            if (gather_rc != 0)
                goto out;
        }
    }

    RING("%s", str.ptr);

    freq_size = summary->freq_size;
    bin_edges_num = summary->bin_edges_num;

    if (freq_size == bin_edges_num)
    {
        TAPI_DPDK_STATS_GATHERED_RING("Histogram",
                                      bin_edges_num,
                                      "%.f(%.3f%%) : %.3f%%\n",
                                      summary->bin_edges[index],
                                      te_meas_stats_value_deviation(
                                            summary->bin_edges[index],
                                            data->mean),
                                      summary->freq[index] * 100);
        if (gather_rc != 0)
            goto out;
    }
    else
    {
        TAPI_DPDK_STATS_GATHERED_RING("Histogram",
                                      bin_edges_num - 1,
                                      "%.f(%.3f%%) - %.f(%.3f%%) : %.3f%%\n",
                                      summary->bin_edges[index],
                                      te_meas_stats_value_deviation(
                                               summary->bin_edges[index],
                                               data->mean),
                                      summary->bin_edges[index + 1],
                                      te_meas_stats_value_deviation(
                                               summary->bin_edges[index + 1],
                                               data->mean),
                                      summary->freq[index] * 100);
        if (gather_rc != 0)
            goto out;
    }

out:
    te_string_free(&str);

    return gather_rc;
}

void
tapi_dpdk_stats_stab_artifact(const te_meas_stats_t *meas_stats,
                              const char *prefix)
{
    if (te_meas_stats_stab_is_stable(&meas_stats->stab,
                                     &meas_stats->data))
        TEST_ARTIFACT(
            "%sStabilization reached on datapoint (+ leading zero datapoints): %d (+ %d)",
            empty_string_if_null(prefix),
            meas_stats->data.num_datapoints,
            meas_stats->num_zeros);
    else
        TEST_ARTIFACT("%sStabilization not reached",
                      empty_string_if_null(prefix));
}

te_errno
tapi_dpdk_stats_log_rates(const te_meas_stats_t *meas_stats,
                          unsigned int packet_size, unsigned int link_speed,
                          const char *prefix)
{
    uint64_t l1_bitrate;
    uint64_t pps;
    double cv;
    te_errno rc;

    pps = meas_stats->stab_required ?
          meas_stats->stab.correct_data.mean :
          meas_stats->data.mean;

    l1_bitrate = tapi_dpdk_stats_calculate_l1_bitrate(pps, packet_size);

    tapi_dpdk_stats_pps_artifact(pps, prefix);
    tapi_dpdk_stats_l1_bitrate_artifact(l1_bitrate, prefix);

    cv = meas_stats->stab_required ?
         meas_stats->stab.correct_data.cv :
         meas_stats->data.cv;

    tapi_dpdk_stats_cv_artifact(cv, prefix);

    if (link_speed == 0)
    {
        WARN_VERDICT("%sLink speed is zero: link usage report is skipped",
                     empty_string_if_null(prefix));
    }
    else
    {
        double l1_link_usage;

        tapi_dpdk_stats_calculate_l1_link_usage(l1_bitrate, link_speed,
                                                &l1_link_usage);
        tapi_dpdk_stats_l1_link_usage_artifact(l1_link_usage, prefix);
    }

    if (meas_stats->summary_required)
    {
        if ((rc = tapi_dpdk_stats_summary_artifact(meas_stats, prefix)) != 0)
            return rc;
    }

    if (meas_stats->stab_required)
        tapi_dpdk_stats_stab_artifact(meas_stats, prefix);

    return 0;
}
