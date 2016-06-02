/** @file
 * @brief RPC client API for DPDK Ethernet Device API
 *
 * RPC client API for DPDK Ethernet Device API functions.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_ETHDEV_H__
#define __TE_TAPI_RPC_RTE_ETHDEV_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_ethdev TAPI for RTE Ethernet Device API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * rte_eth_dev_info() RPC.
 *
 * Caller must free memory allocated for driver_name using free().
 */
extern void rpc_rte_eth_dev_info_get(rcf_rpc_server *rpcs,
                                     uint8_t port_id,
                                     struct tarpc_rte_eth_dev_info *dev_info);

/**
 * rte_eth_dev_configure() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_dev_configure(rcf_rpc_server *rpcs,
                                     uint8_t port_id,
                                     uint16_t nb_rx_queue,
                                     uint16_t nb_tx_queue,
                                     const struct tarpc_rte_eth_conf *eth_conf);

/**@} <!-- END te_lib_rpc_rte_ethdev --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_ETHDEV_H__ */
