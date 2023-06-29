/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC for DPDK
 *
 * Definitions necessary for RPC implementation
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LIB_RPCS_DPDK_H__
#define __TE_LIB_RPCS_DPDK_H__

#include "te_rpc_errno.h"
#include "te_enum.h"
#include "rpc_dpdk_defs.h"

#define RPC_TYPE_NS_RTE_MEMPOOL "rte_mempool"
#define RPC_TYPE_NS_RTE_MBUF    "rte_mbuf"
#define RPC_TYPE_NS_RTE_RING    "rte_ring"
#define RPC_TYPE_NS_RTE_FLOW    "rte_flow"

/**
 * Translate negative errno from host to RPC.
 */
static inline void
neg_errno_h2rpc(int *retval)
{
    if (*retval < 0)
        *retval = -errno_h2rpc(-*retval);
}

/**
 * Convert RPC 64-bit bitmask to/from RTE.
 *
 * @param[in]  from_bm    Bitmask to convert from.
 * @param[in]  conv_map   Conversion map.
 * @param[in]  rte2rpc    Whether conversion is from RTE to RPC.
 * @param[out] to_bm      Bitmask to convert to.
 *
 * @return Status code.
 * @retval TE_EINVAL      @p conv_map contains zero values or overlapped bits.
 * @retval TE_ERANGE      Conversion was done, but some bits of @p bm can not
 *                        be converted using @p conv_map.
 */
static inline te_errno
rpc_dpdk_bitmask64_convert(uint64_t from_bm,
                           const te_enum_bitmask_conv conv_map[],
                           te_bool rte2rpc, uint64_t *to_bm)
{
    return te_enum_bitmask_convert(conv_map, from_bm, rte2rpc, to_bm);
}

/**
 * Convert RPC 64-bit RPC bitmask to RTE.
 *
 * @param[in]  rpc_bm     RPC 64-bit bitmask to convert.
 * @param[in]  conv_map   Conversion map.
 * @param[out] rte_bm     Converted RTE 64-bit bitmask.
 *
 * @return Status code.
 * @retval TE_EINVAL      @p conv_map contains zero values or overlapped bits.
 * @retval TE_ERANGE      Conversion was done, but some bits of @p bm can not
 *                        be converted using @p conv_map.
 */
static inline te_errno
rpc_dpdk_bitmask64_rpc2rte(uint64_t rpc_bm,
                           const te_enum_bitmask_conv conv_map[],
                           uint64_t *rte_bm)
{
    return rpc_dpdk_bitmask64_convert(rpc_bm, conv_map, FALSE, rte_bm);
}

/**
 * Convert RPC 64-bit RTE bitmask to RPC.
 *
 * @param[in] rte_bm       RPC 64-bit bitmask to convert.
 * @param[in] conv_map     Conversion map.
 * @param[in] unknown_bit  Bit to set if conversion was failed or incomleted.
 *
 * @return Converted RPC 64-bit bitmask.
 */
static inline uint64_t
rpc_dpdk_bitmask64_rte2rpc(uint64_t rte_bm,
                           const te_enum_bitmask_conv conv_map[],
                           unsigned int unknown_bit)
{
    uint64_t rpc_bm = 0;
    te_errno rc;

    rc = rpc_dpdk_bitmask64_convert(rte_bm, conv_map, TRUE, &rpc_bm);
    if (rc == TE_ERANGE)
        rpc_bm |= UINT64_C(1) << unknown_bit;
    else if (rc == TE_EINVAL)
        rpc_bm = UINT64_C(1) << unknown_bit;

    return rpc_bm;
}

/**
 * Convert RPC 32-bit RPC bitmask to RTE.
 *
 * @param[in]  rpc_bm     RPC 32-bit bitmask to convert.
 * @param[in]  conv_map   Conversion map.
 * @param[out] rte_bm     Converted RTE 32-bit bitmask.
 *
 * @return Status code.
 * @retval TE_EINVAL      @p conv_map contains zero values or overlapped bits.
 * @retval TE_ERANGE      Conversion was done, but some bits of @p bm can not
 *                        be converted using @p conv_map.
 */
static inline te_errno
rpc_dpdk_bitmask32_rpc2rte(uint32_t rpc_bm,
                           const te_enum_bitmask_conv conv_map[],
                           uint32_t *rte_bm)
{
    uint64_t rte_bm64;
    te_errno rc;

    rc = rpc_dpdk_bitmask64_rpc2rte(rpc_bm, conv_map, &rte_bm64);
    if (rc == 0 && (rte_bm64 & ~UINT32_MAX) == 0)
        *rte_bm = rte_bm64;

    return rc;
}

/**
 * Convert RPC 32-bit RTE bitmask to RPC.
 *
 * @param[in] rte_bm       RPC 32-bit bitmask to convert.
 * @param[in] conv_map     Conversion map.
 * @param[in] unknown_bit  Bit to set if conversion was failed or incomleted.
 *
 * @return Converted RPC 32-bit bitmask.
 */
static inline uint32_t
rpc_dpdk_bitmask32_rte2rpc(uint32_t rte_bm,
                           const te_enum_bitmask_conv conv_map[],
                           unsigned int unknown_bit)
{
    uint64_t rpc_bm64 = 0;

    rpc_bm64 = rpc_dpdk_bitmask64_rte2rpc(rte_bm, conv_map, unknown_bit);
    if ((rpc_bm64 & ~UINT32_MAX) == 0)
        return rpc_bm64;
    else
        return (UINT32_C(1) << unknown_bit);
}

/**
 * Convert RPC 16-bit RPC bitmask to RTE.
 *
 * @param[in]  rpc_bm     RPC 16-bit bitmask to convert.
 * @param[in]  conv_map   Conversion map.
 * @param[out] rte_bm     Converted RTE 16-bit bitmask.
 *
 * @return Status code.
 * @retval TE_EINVAL      @p conv_map contains zero values or overlapped bits.
 * @retval TE_ERANGE      Conversion was done, but some bits of @p bm can not
 *                        be converted using @p conv_map.
 */
static inline te_errno
rpc_dpdk_bitmask16_rpc2rte(uint16_t rpc_bm,
                           const te_enum_bitmask_conv conv_map[],
                           uint16_t *rte_bm)
{
    uint64_t rte_bm64;
    te_errno rc = 0;

    rc = rpc_dpdk_bitmask64_rpc2rte(rpc_bm, conv_map, &rte_bm64);
    if (rc == 0 && (rte_bm64 & ~UINT16_MAX) == 0)
        *rte_bm = rte_bm64;

    return rc;
}

/**
 * Convert RPC 16-bit RTE bitmask to RPC.
 *
 * @param[in] rte_bm       RPC 16-bit bitmask to convert.
 * @param[in] conv_map     Conversion map.
 * @param[in] unknown_bit  Bit to set if conversion was failed or incomleted.
 *
 * @return Converted RPC 16-bit bitmask.
 */
static inline uint32_t
rpc_dpdk_bitmask16_rte2rpc(uint16_t rte_bm,
                           const te_enum_bitmask_conv conv_map[],
                           unsigned int unknown_bit)
{
    uint64_t rpc_bm64 = 0;

    rpc_bm64 = rpc_dpdk_bitmask64_rte2rpc(rte_bm, conv_map, unknown_bit);
    if ((rpc_bm64 & ~UINT16_MAX) == 0)
        return rpc_bm64;
    else
        return (UINT16_C(1) << unknown_bit);
}

#endif /* __TE_LIB_RPCS_DPDK_H__ */
