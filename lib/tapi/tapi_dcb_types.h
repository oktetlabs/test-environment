/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/**
 * @file
 * @brief Common Data Center Bridging types
 *
 * Common types and constants used by DCB APIs.
 */

#ifndef __TE_TAPI_DCB_TYPES_H__
#define __TE_TAPI_DCB_TYPES_H__

#include "te_defs.h"
#include "te_enum.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name DCB limits
 * @{
 */

/** Maximum number of IEEE 802.1p priorities. */
#define TAPI_DCB_NUM_PRIORITIES 8
/**
 * Maximum number of IEEE 802.1Qaz Traffic Classes.
 *
 * IEEE 802.1Qaz defines at most one Traffic Class per user priority.
 */
#define TAPI_DCB_NUM_TCS_MAX TAPI_DCB_NUM_PRIORITIES

/** @} */

/**
 * @name DCBX configuration flags
 * @{
 */

/** DCBX negotiation is performed by the host LLDP agent (OS). */
#define TAPI_DCB_DCBX_HOST              0x01
/** DCBX configuration is managed by the device LLDP agent (NIC). */
#define TAPI_DCB_DCBX_LLD_MANAGED       0x02
/** CEE DCBX protocol flavor. */
#define TAPI_DCB_DCBX_VER_CEE           0x04
/** IEEE DCBX protocol flavor. */
#define TAPI_DCB_DCBX_VER_IEEE          0x08
/** Static configuration, i.e. no actual negotiation is performed. */
#define TAPI_DCB_DCBX_STATIC            0x10

/** @} */

/**
 * @name Transmission Selection Algorithm values
 * @{
 */

/** Strict-priority Transmission Selection Algorithm. */
#define TAPI_DCB_TSA_STRICT  0
/** Credit-Based Shaper Transmission Selection Algorithm. */
#define TAPI_DCB_TSA_CBS     1
/** Enhanced Transmission Selection Algorithm. */
#define TAPI_DCB_TSA_ETS     2
/** Vendor-specific Transmission Selection Algorithm. */
#define TAPI_DCB_TSA_VENDOR  UINT8_MAX

/** @} */

/* Mapping between DCBX flags and their canonical names. */
static const te_enum_map tapi_dcb_dcbx_flag_map[] = {
    { .name = "host", .value = TAPI_DCB_DCBX_HOST, },
    { .name = "lld-managed", .value = TAPI_DCB_DCBX_LLD_MANAGED, },
    { .name = "cee", .value = TAPI_DCB_DCBX_VER_CEE, },
    { .name = "ieee", .value = TAPI_DCB_DCBX_VER_IEEE, },
    { .name = "static", .value = TAPI_DCB_DCBX_STATIC, },
    TE_ENUM_MAP_END
};

/** Mapping between TSA values and their canonical names. */
static const te_enum_map tapi_dcb_tsa_map[] = {
    { .name = "strict", .value = TAPI_DCB_TSA_STRICT, },
    { .name = "cbs", .value = TAPI_DCB_TSA_CBS, },
    { .name = "ets", .value = TAPI_DCB_TSA_ETS, },
    { .name = "vendor", .value = TAPI_DCB_TSA_VENDOR, },
    TE_ENUM_MAP_END
};

/** Transmission Selection Algorithm value. */
typedef uint8_t tapi_dcb_tsa;

/** DCBX configuration bitmask. */
typedef uint8_t tapi_dcb_dcbx_config;

/** Complete DCBX state bitmask. */
typedef tapi_dcb_dcbx_config tapi_dcb_dcbx_state;

/** Configurable ETS parameters. */
typedef struct tapi_dcb_ets_config {
    /** Whether the interface is willing to accept ETS configuration. */
    bool willing;
    /** Traffic Class bandwidths. */
    uint8_t tc_bw[TAPI_DCB_NUM_TCS_MAX];
    /** Priority Group bandwidths. */
    uint8_t pg_bw[TAPI_DCB_NUM_TCS_MAX];
    /** Transmission Selection Algorithms for Traffic Classes. */
    tapi_dcb_tsa tc_tsa[TAPI_DCB_NUM_TCS_MAX];
    /** Traffic Class assignments for user priorities. */
    uint8_t prio_tc[TAPI_DCB_NUM_PRIORITIES];
    /** Recommended Traffic Class bandwidths. */
    uint8_t reco_tc_bw[TAPI_DCB_NUM_TCS_MAX];
    /** Recommended Transmission Selection Algorithms for Traffic Classes. */
    tapi_dcb_tsa reco_tc_tsa[TAPI_DCB_NUM_TCS_MAX];
    /** Recommended Traffic Class assignments for user priorities. */
    uint8_t reco_prio_tc[TAPI_DCB_NUM_PRIORITIES];
} tapi_dcb_ets_config;

/** Complete ETS state. */
typedef struct tapi_dcb_ets_state {
    /** Read/write ETS configuration. */
    tapi_dcb_ets_config config;

    /** Number of Traffic Classes supported by the interface (read-only). */
    uint8_t ets_cap;
    /** Credit-Based Shaper TSA capability (read-only). */
    bool cbs;
} tapi_dcb_ets_state;

/** Configurable PFC parameters. */
typedef struct tapi_dcb_pfc_config {
    /** MACsec bypass state. */
    bool mbc;
    /** Round-trip link propagation delay allowance, in link bits. */
    uint16_t delay;
    /** PFC enablement states. */
    bool pfc_en[TAPI_DCB_NUM_PRIORITIES];
} tapi_dcb_pfc_config;

/** Complete PFC state. */
typedef struct tapi_dcb_pfc_state {
    /** Read/write PFC configuration. */
    tapi_dcb_pfc_config config;

    /**
     * Maximum number of TCs for which PFC may be enabled simultaneously
     * (read-only).
     */
    uint8_t pfc_cap;
    /** Number of transmitted PFC frames, indexed by priority. */
    uint64_t requests[TAPI_DCB_NUM_PRIORITIES];
    /** Number of received PFC frames, indexed by priority. */
    uint64_t indications[TAPI_DCB_NUM_PRIORITIES];
} tapi_dcb_pfc_state;

/**
 * Convert a single DCBX flag to its textual representation.
 *
 * @param[in] flag  DCBX flag.
 *
 * @return DCBX flag name, or @c "unknown" if @p flag is not a known flag.
 */
static inline const char *
tapi_dcb_dcbx_flag2str(tapi_dcb_dcbx_config flag)
{
    return te_enum_map_from_any_value(tapi_dcb_dcbx_flag_map, flag,
                                      "unknown");
}

/**
 * Convert TSA to its textual representation.
 *
 * @param[in] tsa   TSA value.
 *
 * @return TSA name, or @c "unknown" if @p tsa has no name (a value from
 *         3 to 254).
 */
static inline const char *
tapi_dcb_tsa2str(tapi_dcb_tsa tsa)
{
    return te_enum_map_from_any_value(tapi_dcb_tsa_map, tsa, "unknown");
}

/**
 * Compare the ETS parameters that tests set: willing, Traffic Class
 * bandwidths and TSAs, and Traffic Class assignments for user priorities.
 * Priority Group bandwidths and the recommended parameters are not
 * compared.
 *
 * @param[in] a     ETS configuration.
 * @param[in] b     ETS configuration to compare with.
 *
 * @return @c true if the parameters are equal.
 */
static inline bool
tapi_dcb_ets_config_equal(const tapi_dcb_ets_config *a,
                          const tapi_dcb_ets_config *b)
{
    return a->willing == b->willing &&
           memcmp(a->tc_bw, b->tc_bw, sizeof(a->tc_bw)) == 0 &&
           memcmp(a->tc_tsa, b->tc_tsa, sizeof(a->tc_tsa)) == 0 &&
           memcmp(a->prio_tc, b->prio_tc, sizeof(a->prio_tc)) == 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_DCB_TYPES_H__ */
