/** @file
 * @brief RPC client API for DPDK mbuf library
 *
 * RPC client API for DPDK mbuf library functions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
 * @b rte_pktmbuf_pool_create_by_ops() RPC
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
 * @param ops_name        The mempool ops name to be used for this mempool
 *                        instead of default mempool. The value can be NULL
 *                        to use default mempool
 *
 * @return RTE mempool pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_mempool_p rpc_rte_pktmbuf_pool_create_by_ops(rcf_rpc_server *rpcs,
                                                            const char *name,
                                                            uint32_t n,
                                                            uint32_t cache_size,
                                                            uint16_t priv_size,
                                                            uint16_t data_room_size,
                                                            int socket_id,
                                                            const char *ops_name);

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
                                       rpc_rte_mbuf_p m, const uint8_t *buf,
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
 * @return @c 0 on success; jumps out in case of error (i.e., -EOVERFLOW)
 */
extern int rpc_rte_pktmbuf_chain(rcf_rpc_server *rpcs,
                                 rpc_rte_mbuf_p head, rpc_rte_mbuf_p tail);

/**
 * @b rte_pktmbuf_get_nb_segs() RPC
 *
 * @param m               RTE mbuf pointer
 *
 * @return The number of segments in the mbuf chain
 */
extern uint16_t rpc_rte_pktmbuf_get_nb_segs(rcf_rpc_server *rpcs,
                                            rpc_rte_mbuf_p m);

/**
 * @b rte_pktmbuf_get_port() RPC
 *
 * @param m               RTE mbuf pointer
 *
 * @return Input port
 */
extern uint16_t rpc_rte_pktmbuf_get_port(rcf_rpc_server *rpcs,
                                         rpc_rte_mbuf_p m);

/**
 * @b rte_pktmbuf_set_port() RPC
 *
 * @param m               RTE mbuf pointer
 * @param port            Port number to set
 */
extern void rpc_rte_pktmbuf_set_port(rcf_rpc_server *rpcs,
                                     rpc_rte_mbuf_p m, uint16_t port);

/**
 * Get mbuf segment data length
 *
 * @param m               RTE mbuf pointer
 *
 * @return Data length (bytes)
 */
extern uint16_t rpc_rte_pktmbuf_get_data_len(rcf_rpc_server *rpcs,
                                             rpc_rte_mbuf_p m);

/**
 * Get VLAN TCI
 *
 * @param m               RTE mbuf pointer
 *
 * @return VLAN TCI
 */
extern uint16_t rpc_rte_pktmbuf_get_vlan_tci(rcf_rpc_server *rpcs,
                                             rpc_rte_mbuf_p m);

/**
 * Set VLAN TCI
 *
 * @param m               RTE mbuf pointer
 * @param vlan_tci        VLAN TCI
 */
extern void rpc_rte_pktmbuf_set_vlan_tci(rcf_rpc_server *rpcs,
                                         rpc_rte_mbuf_p m, uint16_t vlan_tci);

/**
 * Get VLAN TCI (outer)
 *
 * @param m               RTE mbuf pointer
 *
 * @return VLAN TCI (outer)
 */
extern uint16_t rpc_rte_pktmbuf_get_vlan_tci_outer(rcf_rpc_server *rpcs,
                                                   rpc_rte_mbuf_p m);

/**
 * Set VLAN TCI (outer)
 *
 * @param m               RTE mbuf pointer
 * @param vlan_tci_outer  VLAN TCI (outer)
 */
extern void rpc_rte_pktmbuf_set_vlan_tci_outer(rcf_rpc_server *rpcs,
                                               rpc_rte_mbuf_p m,
                                               uint16_t vlan_tci_outer);

/**
 * Get mbuf offload flags
 *
 * @param m               RTE mbuf pointer
 *
 * @return Offload flags bitmask for the particular mbuf
 */
extern uint64_t rpc_rte_pktmbuf_get_flags(rcf_rpc_server *rpcs,
                                          rpc_rte_mbuf_p m);

/**
 * Set mbuf offload flags
 *
 * @param m               RTE mbuf pointer
 * @param ol_flags        Offload flags bitmask to be set
 *
 * @return @c 0 on success; jumps out if some flags cannot be set (i.e. -EINVAL)
 */
extern int rpc_rte_pktmbuf_set_flags(rcf_rpc_server *rpcs,
                                     rpc_rte_mbuf_p m,
                                     uint64_t ol_flags);

/**
 * Get a pointer to mempool within which the particular mbuf has been allocated
 *
 * @param m               RTE mbuf pointer
 *
 * @return RTE mempool pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_mempool_p rpc_rte_pktmbuf_get_pool(rcf_rpc_server *rpcs,
                                                  rpc_rte_mbuf_p m);

/**
 * Get the headroom length in the particular mbuf
 *
 * @param m               RTE mbuf pointer
 *
 * @return Headroom length; jumps out in case of invalid value (i.e. UINT16_MAX)
 */
extern uint16_t rpc_rte_pktmbuf_headroom(rcf_rpc_server *rpcs,
                                         rpc_rte_mbuf_p m);

/**
 * Get the tailroom length in the particular mbuf
 *
 * @param m               RTE mbuf pointer
 *
 * @return Tailroom length; jumps out in case of invalid value (i.e. UINT16_MAX)
 */
extern uint16_t rpc_rte_pktmbuf_tailroom(rcf_rpc_server *rpcs,
                                         rpc_rte_mbuf_p m);

/**
 * @b rte_pktmbuf_trim() RPC
 *
 * @param m               RTE mbuf pointer
 * @param len             Amount of bytes to be trimmed
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_pktmbuf_trim(rcf_rpc_server *rpcs,
                                rpc_rte_mbuf_p m,
                                uint16_t len);

/**
 * @b rte_pktmbuf_adj() RPC
 *
 * @param m               RTE mbuf pointer
 * @param len             Amount of bytes to be removed at the beginning of mbuf
 *
 * @return The new @c data_off on success; jumps out on error (i.e., UINT16_MAX)
 */
extern uint16_t rpc_rte_pktmbuf_adj(rcf_rpc_server *rpcs,
                                    rpc_rte_mbuf_p m, uint16_t len);

/**
 * Get packet type flags
 *
 * @param m               RTE mbuf pointer
 * @param p_type          Pointer to a structure to be filled with the info
 */
extern void rpc_rte_pktmbuf_get_packet_type(rcf_rpc_server *rpcs,
                                            rpc_rte_mbuf_p m,
                                  struct tarpc_rte_pktmbuf_packet_type *p_type);

/**
 * Set packet type flags
 *
 * @param m               RTE mbuf pointer
 * @param p_type          Pointer to a structure containing info to be populated
 *
 * @return @c 0 on success; jumps out if some types cannot be set (i.e. -EINVAL)
 */
extern int rpc_rte_pktmbuf_set_packet_type(rcf_rpc_server *rpcs,
                                           rpc_rte_mbuf_p m,
                            const struct tarpc_rte_pktmbuf_packet_type *p_type);

/**
 * Get RSS hash
 *
 * @param m               RTE mbuf pointer
 *
 * @return RSS hash for the particular mbuf
 */
extern uint32_t rpc_rte_pktmbuf_get_rss_hash(rcf_rpc_server *rpcs,
                                             rpc_rte_mbuf_p m);

/**
 * Get FDIR ID, if RTE_MBUF_F_RX_FDIR_ID is in offload flags
 *
 * @param m               RTE mbuf pointer
 *
 * @return FDIR ID for the particular mbuf
 */
extern uint32_t rpc_rte_pktmbuf_get_fdir_id(rcf_rpc_server *rpcs,
                                            rpc_rte_mbuf_p m);

/** Get parameters needed by TX offload to work
 *
 * @param m               RTE mbuf pointer
 * @param tx_offload      Pointer to structure to be filled with the info
 */
extern void rpc_rte_pktmbuf_get_tx_offload(rcf_rpc_server *rpcs,
                                           rpc_rte_mbuf_p m,
                               struct tarpc_rte_pktmbuf_tx_offload *tx_offload);

/** Set parameters needed by TX offload to work
 *
 * @param m               RTE mbuf pointer
 * @param tx_offload      Pointer to a structure containing info to be populated
 */
extern void rpc_rte_pktmbuf_set_tx_offload(rcf_rpc_server *rpcs,
                                           rpc_rte_mbuf_p m,
                         const struct tarpc_rte_pktmbuf_tx_offload *tx_offload);

/**
 * @c rte_pktmbuf_refcnt_update() RPC
 *
 * @param m RTE pktmbuf pointer
 * @param v The value to be added to reference counters for
 *          the corresponding segments which @p m consists of
 */
extern void rpc_rte_pktmbuf_refcnt_update(rcf_rpc_server *rpcs,
                                          rpc_rte_mbuf_p  m,
                                          int16_t         v);

/**
 * Redistribute a packet mbuf data across some given number of segments
 * of variable length according to segmentation pattern provided by the caller
 * (an array of segment group definitions and the number of elements
 *  in the array) to produce a new mbuf chain with a different RPC pointer.
 *
 * The new segments are allocated from a subset of memory pools @p mp_multi
 * in a round-robin style; fallback to the default pool (obtained from @p m)
 * will happen if either @p mp_multi is unset or mempool shortages take place.
 *
 * @param m                 RTE mbuf pointer (in / out)
 * @param mp_multi          An array of RTE mempool pointers
 * @param mp_multi_nb_items The number of RTE mempool pointers
 * @param seg_groups        An array of segment group definitions
 * @param nb_seg_groups     The number of segment group definitions
 *
 * @return The number of segments produced or a negative errno value
 */
extern int rpc_rte_pktmbuf_redist_multi(
                            rcf_rpc_server                 *rpcs,
                            rpc_rte_mbuf_p                 *m,
                            rpc_rte_mempool_p              *mp_multi,
                            unsigned int                    mp_multi_nb_items,
                            struct tarpc_pktmbuf_seg_group *seg_groups,
                            uint8_t                         nb_seg_groups);

/**
 * Single-pool wrapper for @c rpc_rte_pktmbuf_redist_multi().
 * Default mempool is picked from @p m.
 */
extern int rpc_rte_pktmbuf_redist(rcf_rpc_server *rpcs,
                                  rpc_rte_mbuf_p *m,
                                  struct tarpc_pktmbuf_seg_group *seg_groups,
                                  uint8_t nb_seg_groups);

/**
 * @b rte_vlan_strip() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * return value is not equal to 0 or -1.
 */
extern int rpc_rte_vlan_strip(rcf_rpc_server *rpcs, rpc_rte_mbuf_p m);

/**@} <!-- END te_lib_rpc_rte_mbuf --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_MBUF_H__ */
