/** @file
 * @brief RPC client API for RTE mbuf CSAP layer
 *
 * RPC client API to be used for traffic matching and conversion
 * between ASN.1 and RTE mbuf(s) representation
 * (declarations)
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_MBUF_NDN_H__
#define __TE_TAPI_RPC_RTE_MBUF_NDN_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"
#include "log_bufs.h"
#include "asn_usr.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_mbuf_ndn TAPI for RTE mbuf layer API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Convert an ASN.1 traffic template to RTE mbuf(s)
 *
 * @param[in]  template    ASN.1 traffic template
 * @param[in]  mp          RTE mempool pointer
 * @param[out] mbufs       Location for RTE mbuf pointer(s)
 * @param[out] count       Location for the number of mbufs prepared
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_mk_mbuf_from_template(rcf_rpc_server   *rpcs,
                                         const asn_value  *template,
                                         rpc_rte_mempool_p mp,
                                         rpc_rte_mbuf_p  **mbufs,
                                         unsigned int     *count);

/**
 * Match RTE mbuf(s) to a particular pattern
 *
 * @param[in]  pattern     ASN.1 traffic pattern
 * @param[in]  mbufs       Array of RTE mbuf pointer(s)
 * @param[in]  count       The number of RTE mbuf pointers in the array
 * @param[out] packets     Location for the matching ASN.1 packets (optional)
 * @param[out] matched     Location for the number of packets matched
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_mbuf_match_pattern(rcf_rpc_server     *rpcs,
                                      const asn_value    *pattern,
                                      rpc_rte_mbuf_p     *mbufs,
                                      unsigned int        count,
                                      asn_value        ***packets,
                                      unsigned int       *matched);

/**
 * Wrapper for rte_mbuf_match_pattern() RPC
 * intended for sequence matching
 */
extern int tapi_rte_mbuf_match_pattern_seq(rcf_rpc_server    *rpcs,
                                           const asn_value   *pattern,
                                           rpc_rte_mbuf_p    *mbufs,
                                           unsigned int       count,
                                           asn_value       ***packets,
                                           unsigned int      *matched);

/**
 * Prepare Tx mbuf for comparison with Rx burst.
 *
 * @param[in] m Packet mbuf
 *
 * @note Length fields in the header must assume
 *       zero L4 payload length, but the payload
 *       itself must be present in the mbuf.
 *
 * @note VLAN tags in the outermost header must
 *       use standard QinQ and VLAN TPID values.
 *
 * @note If encapsulation is used, no VLAN tags
 *       are allowed in the inner header.
 *
 * @note IPv6 extension headers are disallowed.
 *
 * @note @p m MUST have m->[...]lX_len values
 *       originally set by TAD rte_mbuf layer.
 *
 * @note The packet header must be contiguous.
 *
 * @return @c 0 on success; jumps out on failure
 */
extern int rpc_rte_mbuf_match_tx_rx_pre(rcf_rpc_server *rpcs,
                                        rpc_rte_mbuf_p  m);

/**@} <!-- END te_lib_rpc_rte_mbuf_ndn --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_MBUF_NDN_H__ */
