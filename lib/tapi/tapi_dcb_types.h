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

/* Mapping between DCBX flags and their canonical names. */
static const te_enum_map tapi_dcb_dcbx_flag_map[] = {
    { .name = "host", .value = TAPI_DCB_DCBX_HOST, },
    { .name = "lld-managed", .value = TAPI_DCB_DCBX_LLD_MANAGED, },
    { .name = "cee", .value = TAPI_DCB_DCBX_VER_CEE, },
    { .name = "ieee", .value = TAPI_DCB_DCBX_VER_IEEE, },
    { .name = "static", .value = TAPI_DCB_DCBX_STATIC, },
    TE_ENUM_MAP_END
};

/** DCBX configuration bitmask. */
typedef uint8_t tapi_dcb_dcbx_config;

/** Complete DCBX state bitmask. */
typedef tapi_dcb_dcbx_config tapi_dcb_dcbx_state;

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_DCB_TYPES_H__ */
