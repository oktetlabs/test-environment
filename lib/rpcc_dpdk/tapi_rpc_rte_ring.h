/** @file
 * @brief RPC client API for DPDK ring library
 *
 * RPC client API for DPDK ring library functions (declarations)
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_RING_H__
#define __TE_TAPI_RPC_RTE_RING_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"
#include "log_bufs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_ring TAPI for RTE ring API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * @b rte_ring_create() RPC
 *
 * @param name            The name of the ring
 * @param count           The size of the ring (must be a power of 2)
 * @param socket_id       The socket identifier where the memory should
 *                        be allocated
 * @param flags           An OR of the following:
 *                        (1U << TARPC_RTE_RING_F_SP_ENQ),
 *                        (1U << TARPC_RTE_RING_F_SC_DEQ)
 *
 * @return RTE ring pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_ring_p rpc_rte_ring_create(rcf_rpc_server *rpcs,
                                          const char     *name,
                                          unsigned        count,
                                          int             socket_id,
                                          unsigned        flags);

/**
 * @b rte_ring_free() RPC
 *
 * @param ring            RTE ring pointer
 */
extern void rpc_rte_ring_free(rcf_rpc_server *rpcs,
                              rpc_rte_ring_p  ring);

/**
 * Enqueue an mbuf to RTE ring
 *
 * @param ring            RTE ring pointer
 * @param m               RTE mbuf pointer
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rte_ring_enqueue_mbuf(rcf_rpc_server *rpcs,
                                 rpc_rte_ring_p  ring,
                                 rpc_rte_mbuf_p  m);

/**
 * Dequeue an mbuf from RTE ring
 *
 * @param ring            RTE ring pointer
 *
 * @return RTE mbuf pointer; doesn't jump out when pointer is @c NULL
 */
extern rpc_rte_mbuf_p rte_ring_dequeue_mbuf(rcf_rpc_server *rpcs,
                                            rpc_rte_ring_p  ring);

/**@} <!-- END te_lib_rpc_rte_ring --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_RING_H__ */
