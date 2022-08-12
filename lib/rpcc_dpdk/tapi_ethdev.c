/** @file
 * @brief TAPI for DPDK Ethernet Device API
 *
 * Helper functions to simplify RTE Ethernet Devices API operations
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#include "te_config.h"

#include "conf_api.h"
#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_test_log.h"

#include "tarpc.h"

#include "tapi_rpc_rte_ethdev.h"
#include "rpc_dpdk_offloads.h"

/**
 * Get default link speeds from Configurator if any.
 *
 * @param link_speeds   Location of the link speeds field; cannot be @c NULL
 *
 * @return Status code.
 */
static te_errno
tapi_rpc_rte_eth_conf_get_link_speeds(const char *ta, uint32_t *link_speeds)
{
    te_errno  rc;
    char     *val;

    rc = cfg_get_instance_string_fmt(&val, "/local:%s/dpdk:/link_speeds:", ta);
    if (rc == TE_RC(TE_CS, TE_ENOENT))
        return 0;
    if (rc != 0)
        return rc;

    if (strlen(val) > 0)
    {
        char *sp = NULL;
        char *token;

        for (token = strtok_r(val, ",", &sp);
             token != NULL;
             token = strtok_r(NULL, ",", &sp))
        {
            uint32_t mask = tapi_rpc_rte_eth_link_speeds_str2val(token);

            if (mask == 0)
            {
                ERROR("Unknown link speed value '%s'", token);
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                break;
            }
            *link_speeds |= mask;
        }
    }

    free(val);
    return rc;
}

/**
 * Discover fixed Tx offloads on device level from Configurator
 * and set appropriate flags to enable them.
 *
 * @param offloadsp Location of the offload field; cannot be @c NULL
 *
 * @return Status code.
 */
static te_errno
tapi_rpc_rte_eth_conf_set_fixed_dev_tx_offloads(uint64_t *offloadsp)
{
    unsigned int  nb_instances = 0;
    cfg_handle   *instance_handles = NULL;
    uint64_t      offloads = 0;
    te_errno      rc = 0;
    unsigned int  i;

    rc = cfg_find_pattern_fmt(&nb_instances, &instance_handles,
                              "/local:/dpdk:/offloads:/dev:/tx:/fixed:*");
    if (rc != 0)
        return rc;

    for (i = 0; i < nb_instances; ++i)
    {
        cfg_oid      *oid = NULL;
        const char   *name = NULL;
        unsigned int  j;

        rc = cfg_get_oid(instance_handles[i], &oid);
        if (rc != 0)
            goto out;

        name = CFG_OID_GET_INST_NAME(oid, 6);

        for (j = 0; j < rpc_dpdk_tx_offloads_num; ++j)
        {
            if (strcmp(name, rpc_dpdk_tx_offloads[j].name) == 0)
                offloads |= (1ULL << rpc_dpdk_tx_offloads[j].bit);
        }

        free(oid);
    }

    *offloadsp = offloads;

out:
    free(instance_handles);

    return rc;
}

struct tarpc_rte_eth_conf *
tapi_rpc_rte_eth_make_eth_conf(rcf_rpc_server *rpcs, uint16_t port_id,
                               struct tarpc_rte_eth_conf *eth_conf)
{
    char dev_name[RPC_RTE_ETH_NAME_MAX_LEN];
    char vdev_prefix[] = "net_";
    uint64_t *offloadsp = &eth_conf->txmode.offloads;
    te_errno rc = 0;
    int ret = 0;

    UNUSED(rpcs);
    UNUSED(port_id);

    memset(eth_conf, 0, sizeof(*eth_conf));

    rc = tapi_rpc_rte_eth_conf_get_link_speeds(rpcs->ta,
                                               &eth_conf->link_speeds);
    if (rc != 0)
        return NULL;

    rc = tapi_rpc_rte_eth_conf_set_fixed_dev_tx_offloads(offloadsp);
    if (rc != 0)
        return NULL;

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
