/** @file
 * @brief RPC client API for DPDK mbuf library
 *
 * RPC client API for DPDK mbuf library functions
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

#ifndef __TE_TAPI_RPC_RTE_MBUF_H__
#define __TE_TAPI_RPC_RTE_MBUF_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"
#include "log_bufs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_mbuf TAPI for RTE MBUF API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * @b rte_pktmbuf_pool_create() RPC
 *
 * @param name            The name of the mbuf pool
 * @param n               The number of elements in the mbuf pool
 * @param cache_size      Size of the per-core object cache
 * @param priv_size       Size of application private are between the rte_mbuf
 *                        structure and the data buffer
 * @param data_room_size  Size of data buffer in each mbuf, including
 *                        RTE_PKTMBUF_HEADROOM
 * @param socket_id       The socket identifier where the memory should
 *                        be allocated
 *
 * @return RTE mempool pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_mempool_p rpc_rte_pktmbuf_pool_create(rcf_rpc_server *rpcs,
                                                     const char *name,
                                                     uint32_t n,
                                                     uint32_t cache_size,
                                                     uint16_t priv_size,
                                                     uint16_t data_room_size,
                                                     int socket_id);

/**
 * @b rte_pktmbuf_alloc() RPC
 *
 * @param mp              RTE mempool pointer
 *
 * @return RTE mbuf pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_mbuf_p rpc_rte_pktmbuf_alloc(rcf_rpc_server *rpcs,
                                            rpc_rte_mempool_p mp);

/**
 * @b rte_pktmbuf_free() RPC
 *
 * @param m               RTE mbuf pointer
 */
extern void rpc_rte_pktmbuf_free(rcf_rpc_server *rpcs,
                                 rpc_rte_mbuf_p m);

/**
 * Append data to an mbuf
 *
 * @param m               RTE mbuf pointer
 * @param buf             Pointer to a buffer containing data
 * @param len             Buffer length
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_pktmbuf_append_data(rcf_rpc_server *rpcs,
                                       rpc_rte_mbuf_p m, uint8_t *buf,
                                       size_t len);

/**
 * Read data from an mbuf with a particular offset
 *
 * @param m               RTE mbuf pointer
 * @param offset          Offset into mbuf data
 * @param count           Amount of data to be read (bytes)
 * @param buf             Pointer to a buffer for data to be read in
 * @param rbuflen         Buffer length
 *
 * @return Amount of data actually read (bytes); jumps out on error
 */
extern int rpc_rte_pktmbuf_read_data(rcf_rpc_server *rpcs,
                                     rpc_rte_mbuf_p m, size_t offset,
                                     size_t count, uint8_t *buf,
                                     size_t rbuflen);

/**
 * @b rte_pktmbuf_clone() RPC
 *
 * @param m               RTE mbuf pointer
 * @param mp              RTE mempool pointer
 *
 * @return RTE mbuf pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_mbuf_p rpc_rte_pktmbuf_clone(rcf_rpc_server *rpcs,
                                            rpc_rte_mbuf_p m,
                                            rpc_rte_mempool_p mp);

/**
 * Prepend data to an mbuf
 *
 * @param m               RTE mbuf pointer
 * @param buf             Pointer to a buffer containing data
 * @param len             Buffer length
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_pktmbuf_prepend_data(rcf_rpc_server *rpcs,
                                        rpc_rte_mbuf_p m, uint8_t *buf,
                                        size_t len);

/**
 * Get pointer to the next mbuf segment in an mbuf chain
 *
 * @param m               RTE mbuf pointer
 *
 * @return RTE mbuf pointer; doesn't jump out when pointer is @c NULL
 */
extern rpc_rte_mbuf_p rpc_rte_pktmbuf_get_next(rcf_rpc_server *rpcs,
                                               rpc_rte_mbuf_p m);

/**
 * Get packet length
 *
 * @param m               RTE mbuf pointer
 *
 * @return Packet length (bytes)
 */
extern uint32_t rpc_rte_pktmbuf_get_pkt_len(rcf_rpc_server *rpcs,
                                            rpc_rte_mbuf_p m);

/**
 * Auxiliary function for logging mbuf pointers;
 * it should be used by RPCs in @b TAPI_RPC_LOG() statements
 */
const char *rpc_rte_mbufs2str(te_log_buf *tlbp, const rpc_rte_mbuf_p *mbufs,
                              unsigned int count, rcf_rpc_server *rpcs);

/**
 * @b rte_pktmbuf_alloc_bulk() RPC
 *
 * @param mp              RTE mempool pointer
 * @param bulk            Pointer for the resulting array of pointers
 * @param count           The number of mbufs to allocate
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_pktmbuf_alloc_bulk(rcf_rpc_server *rpcs,
                                      rpc_rte_mempool_p mp,
                                      rpc_rte_mbuf_p *bulk,
                                      unsigned int count);

/**
 * @b rte_pktmbuf_chain() RPC
 *
 * @param head            RTE mbuf pointer (head)
 * @param tail            RTE mbuf pointer (tail)
 *
 * @return @c 0 on success; doesn't jump out in case of error (i.e., -EOVERFLOW)
 */
extern int rpc_rte_pktmbuf_chain(rcf_rpc_server *rpcs,
                                 rpc_rte_mbuf_p head, rpc_rte_mbuf_p tail);

/**@} <!-- END te_lib_rpc_rte_mbuf --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_MBUF_H__ */
