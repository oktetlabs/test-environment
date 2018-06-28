/** @file
 * @brief TAPI for DPDK Ethernet Device API
 *
 * Helper functions to simplify RTE Ethernet Devices API operations
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
tapi_rpc_rte_eth_make_eth_conf(rcf_rpc_server *rpcs, uint16_t port_id,
                               struct tarpc_rte_eth_conf *eth_conf)
{
    char dev_name[RPC_RTE_ETH_NAME_MAX_LEN];
    char vdev_prefix[] = "net_";
    int ret = 0;

    UNUSED(rpcs);
    UNUSED(port_id);

    memset(eth_conf, 0, sizeof(*eth_conf));

    ret = rpc_rte_eth_dev_get_name_by_port(rpcs, port_id, dev_name);
    if (ret != 0)
        return NULL;

    if (strncmp(dev_name, vdev_prefix, strlen(vdev_prefix)) == 0)
        return eth_conf;

    eth_conf->rxmode.flags |= (1 << TARPC_RTE_ETH_RXMODE_HW_STRIP_CRC_BIT);

    return eth_conf;
}

int
tapi_rpc_rte_eth_dev_configure_def(rcf_rpc_server *rpcs, uint16_t port_id,
                                   uint16_t nb_rx_queue, uint16_t nb_tx_queue)
{
    struct tarpc_rte_eth_conf   eth_conf;

    return rpc_rte_eth_dev_configure(rpcs, port_id, nb_rx_queue, nb_tx_queue,
               tapi_rpc_rte_eth_make_eth_conf(rpcs, port_id, &eth_conf));
}

/* See description in tapi_rpc_rte_ethdev.h */
te_errno
tapi_rpc_add_mac_as_octstring2kvpair(rcf_rpc_server *rpcs, uint16_t port_id,
                                     te_kvpair_h *head, const char *name)
{
    struct tarpc_ether_addr mac_addr_cmp;
    struct tarpc_ether_addr mac_addr;
    te_errno                rc;

    rpc_rte_eth_macaddr_get(rpcs, port_id, &mac_addr);

    memset(&mac_addr_cmp, 0, sizeof(mac_addr_cmp));
    if (memcmp(&mac_addr, &mac_addr_cmp, sizeof(mac_addr_cmp)) == 0)
    {
        struct tarpc_rte_eth_conf ec;
        int                       ret;

        memset(&ec, 0, sizeof(ec));

        ret = rpc_rte_eth_dev_configure(rpcs, port_id, 1, 1, &ec);
        if (ret != 0)
            return TE_RC(TE_TAPI, -ret);

        rpc_rte_eth_macaddr_get(rpcs, port_id, &mac_addr);
        rpc_rte_eth_dev_close(rpcs, port_id);
    }

    rc = te_kvpair_add(head, name, "'%02x%02x%02x%02x%02x%02x'H",
                       mac_addr.addr_bytes[0], mac_addr.addr_bytes[1],
                       mac_addr.addr_bytes[2], mac_addr.addr_bytes[3],
                       mac_addr.addr_bytes[4], mac_addr.addr_bytes[5]);

    return rc;
}
