/** @file
 * @brief TAPI for DPDK Ethernet Device API
 *
 * Helper functions to simplify RTE Ethernet Devices API operations
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

#include "te_config.h"

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_test_log.h"

#include "tarpc.h"

#include "tapi_rpc_rte_ethdev.h"


struct tarpc_rte_eth_conf *
tapi_rpc_rte_eth_make_eth_conf(rcf_rpc_server *rpcs, uint8_t port_id,
                               struct tarpc_rte_eth_conf *eth_conf)
{
    UNUSED(rpcs);
    UNUSED(port_id);

    memset(eth_conf, 0, sizeof(*eth_conf));

    eth_conf->rxmode.flags |= (1 << TARPC_RTE_ETH_RXMODE_HW_STRIP_CRC_BIT);

    return eth_conf;
}

int
tapi_rpc_rte_eth_dev_configure_def(rcf_rpc_server *rpcs, uint8_t port_id,
                                   uint16_t nb_rx_queue, uint16_t nb_tx_queue)
{
    struct tarpc_rte_eth_conf   eth_conf;

    return rpc_rte_eth_dev_configure(rpcs, port_id, nb_rx_queue, nb_tx_queue,
               tapi_rpc_rte_eth_make_eth_conf(rpcs, port_id, &eth_conf));
}
