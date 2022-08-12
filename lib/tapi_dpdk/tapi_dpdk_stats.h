/** @file
 * @brief DPDK statistics helper functions TAPI
 *
 * @defgroup tapi_dpdk_stats DPDK statistics helper functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle DPDK-related operations with statistics
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TAPI_DPDK_STATS_H__
#define __TAPI_DPDK_STATS_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_meas_stats.h"
#include "te_string.h"
#include "te_mi_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* prefix and gather_rc are supposed to be declared outside of this macro */
#define TAPI_DPDK_STATS_GATHERED_RING(title, N, fmt_str, ...)                 \
    do {                                                                      \
        unsigned int index;                                                   \
        te_string gather_str = TE_STRING_INIT;                                \
                                                                              \
        gather_rc = te_string_append(&gather_str, "%s%s%s\n",                 \
                                     empty_string_if_null(prefix),            \
                                     prefix == NULL ? "" : ": ", title);      \
        if (gather_rc != 0)                                                   \
        {                                                                     \
            te_string_free(&gather_str);                                      \
            break;                                                            \
        }                                                                     \
                                                                              \
        for (index = 0; index < N; index++)                                   \
        {                                                                     \
            gather_rc = te_string_append(&gather_str, fmt_str, __VA_ARGS__);  \
            if (gather_rc != 0)                                               \
                break;                                                        \
        }                                                                     \
                                                                              \
        if (gather_rc == 0)                                                   \
            RING("%s", gather_str.ptr);                                       \
                                                                              \
        te_string_free(&gather_str);                                          \
    } while (0)

/**
 * Report packet per second statistics as a test artifact.
 *
 * @param logger    MI logger entity to add artifact into (may be @c NULL)
 * @param pps       Packets per second
 * @param prefix    Prefix of the artifact message (may be @c NULL)
 *
 * @return  Status code
 */
void tapi_dpdk_stats_pps_artifact(te_mi_logger *logger, int64_t pps,
                                  const char *prefix);

/**
 * Calculate layer 1 bits per second from PPS and packet size.
 *
 * @param pps          Packets per second
 * @param packet_size  Packet size in bytes (without l1 and FCS)
 *
 * @return  Layer 1 bits per second
 */
uint64_t tapi_dpdk_stats_calculate_l1_bitrate(uint64_t pps,
                                              unsigned int packet_size);

/**
 * Calculate layer 1 link usage from layer 1 bitrate and link speed.
 *
 * @param      l1_bitrate     Layer 1 bits per second
 * @param      link_speed     Link speed in Mbps
 * @param[out] l1_link_usage  Layer 1 link usage ratio
 *
 * @return  Status code
 */
te_errno tapi_dpdk_stats_calculate_l1_link_usage(uint64_t l1_bitrate,
                                                 unsigned int link_speed,
                                                 double *l1_link_usage);

/**
 * Report packet per second statistics as a test artifact.
 *
 * @param logger        MI logger entity to add artifact into (may be @c NULL)
 * @param l1_bitrate    Layer 1 bits per second
 * @param prefix        Prefix of the artifact message (may be @c NULL)
 *
 * @return  Status code
 */
void tapi_dpdk_stats_l1_bitrate_artifact(te_mi_logger *logger,
                                         uint64_t l1_bitrate,
                                         const char *prefix);

/**
 * Report layer 1 link usage statistics as a test artifact.
 *
 * @param logger           MI logger entity to add artifact into
 *                         (may be @c NULL)
 * @param l1_link_usage    Layer 1 link usage ratio
 * @param prefix           Prefix of the artifact message (may be @c NULL)
 *
 * @return  Status code
 */
void tapi_dpdk_stats_l1_link_usage_artifact(te_mi_logger *logger,
                                            double l1_link_usage,
                                            const char *prefix);

/**
 * Report CV of packer per second statistics as a test artifact
 *
 * @param logger    MI logger entity to add artifact into (may be @c NULL)
 * @param cv       Coefficient of variation
 * @param prefix   Prefix of the artifact message (may be @c NULL)
 *
 * @return  Status code
 */
void tapi_dpdk_stats_cv_artifact(te_mi_logger *logger, double cv,
                                 const char *prefix);

/**
 * Report statistics provided by te_meas_stats_summary_t.
 *
 * @param meas_stats    Pointer to te_meas_stats_t structure
 * @param prefix        Prefix of the artifact message (may be @c NULL)
 *
 * @return      0 on success
 */
te_errno tapi_dpdk_stats_summary_artifact(const te_meas_stats_t *meas_stats,
                                          const char *prefix);

/**
 * Report statistics provided by te_meas_stats_stab_t.
 *
 * @param logger        MI logger entity to add artifact into (may be @c NULL)
 * @param meas_stats    Pointer to te_meas_stats_t structure
 * @param prefix        Prefix of the artifact message (may be @c NULL)
 */
void tapi_dpdk_stats_stab_artifact(te_mi_logger *logger,
                                   const te_meas_stats_t *meas_stats,
                                   const char *prefix);

/**
 * Report rates corresponding to PPS, packet_size and link speed
 * as test artifacts.
 *
 * @param tool              Tool used for measurement gathering
 *                          (must not be @c NULL)
 * @param meas_stats        Pointer to te_meas_stats_t structure
 * @param packet_size       Packet size in bytes (without l1 and FCS)
 * @param link_speed        Link speed in Mbps
 * @param prefix            Prefix of artifact messages (may be @c NULL)
 *
 * @return      0 on success
 */
te_errno tapi_dpdk_stats_log_rates(const char *tool,
                                   const te_meas_stats_t *meas_stats,
                                   unsigned int packet_size,
                                   unsigned int link_speed, const char *prefix);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_DPDK_STATS_H__ */

/**@} <!-- END tapi_dpdk_stats --> */
