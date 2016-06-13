/** @file
 * @brief Test API to work with RTE mbufs
 *
 * Definition of test API to work with RTE mbufs
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RTE_MBUF_H__
#define __TE_TAPI_RTE_MBUF_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepare an RTE mbuf with Ethernet frame containing particular data
 * (if buffer to contain the frame data is NULL, then random data will be put)
 *
 * @param mp              RTE mempool pointer
 * @param dst_addr        Destination Ethernet address (network byte order)
 * @param src_addr        Source Ethernet address (network byte order)
 * @param ether_type      Ethernet type value (host byte order)
 * @param payload         Data to be encapsulated into the frame or @c NULL
 * @param len             Data length
 *
 * @return RTE mbuf pointer on success; jumps out on failure
 */
extern rpc_rte_mbuf_p tapi_rte_mk_mbuf_eth(rcf_rpc_server *rpcs,
                                           rpc_rte_mempool_p mp,
                                           const uint8_t *dst_addr,
                                           const uint8_t *src_addr,
                                           const uint16_t ether_type,
                                           const uint8_t *payload, size_t len);

/**
 * Read the whole mbuf (chain) data (starting at a given offset)
 * into the buffer allocated internally and pass the number of bytes read
 * to the user-specified variable
 *
 * @param m               RTE mbuf pointer
 * @param offset          Offset into mbuf data
 * @param bytes_read      Amount of bytes read
 *
 * @return Pointer to a buffer containing data read from mbuf (chain)
 */
extern uint8_t *tapi_rte_get_mbuf_data(rcf_rpc_server *rpcs,
                                       rpc_rte_mbuf_p m, size_t offset,
                                       size_t *bytes_read);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RTE_MBUF_H__ */
