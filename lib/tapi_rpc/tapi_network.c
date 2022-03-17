/** @file
 * @brief Implementation of high level test API to configure tested network
 *
 * Functions implementation.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Network Configuration TAPI"

#include "te_config.h"

#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#include "logger_api.h"
#include "tapi_test_log.h"
#include "tapi_network.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_net.h"
#include "tapi_cfg.h"
#include "tapi_host_ns.h"
#include "tapi_rpc_stdio.h"

/* System tool 'ip' */
#define IP_TOOL "ip"

/* See description in tapi_network.h. */
void
tapi_network_setup(te_bool ipv6_supp)
{
    unsigned int    i, j, k;
    cfg_nets_t      nets;
    cfg_oid        *oid = NULL;
    int             use_static_arp_def;
    int             use_static_arp;
    cfg_val_type    val_type;
    int rc;

    if ((rc = tapi_cfg_net_remove_empty()) != 0)
        TEST_FAIL("Failed to remove /net instances with empty interfaces");

    rc = tapi_cfg_net_reserve_all();
    if (rc != 0)
    {
        TEST_FAIL("Failed to reserve all interfaces mentioned in networks "
                  "configuration: %r", rc);
    }

    rc = tapi_cfg_net_all_up(FALSE);
    if (rc != 0)
    {
        TEST_FAIL("Failed to up all interfaces mentioned in networks "
                  "configuration: %r", rc);
    }

    rc = tapi_cfg_net_delete_all_ip4_addresses();
    if (rc != 0)
    {
        TEST_FAIL("Failed to delete all IPv4 addresses from all "
                  "interfaces mentioned in networks configuration: %r",
                  rc);
    }

    rc = tapi_cfg_net_delete_all_ip6_addresses();
    if (rc != 0)
    {
        TEST_FAIL("Failed to delete all IPv6 addresses from all "
                  "interfaces mentioned in networks configuration: %r",
                  rc);
    }

    /* Get default value for 'use_static_arp' */
    rc = cfg_get_instance_int_fmt(&use_static_arp_def,
                                  "/local:/use_static_arp:");
    if (rc != 0)
    {
        use_static_arp_def = 0;
        WARN("Failed to get /local:/use_static_arp: default value, "
             "set to %d", use_static_arp_def);
    }

    /* Get available networks configuration */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        TEST_FAIL("Failed to get networks from Configurator: %r", rc);
    }

    for (i = 0; i < nets.n_nets; ++i)
    {
        cfg_net_t  *net = nets.nets + i;


        rc = tapi_cfg_net_assign_ip(AF_INET, net, NULL);
        if (rc != 0)
        {
            ERROR("Failed to assign IPv4 subnet to net #%u: %r", i, rc);
            break;
        }

        /* Add static ARP entires */
        for (j = 0; j < net->n_nodes; ++j)
        {
            char               *node_oid = NULL;
            char               *if_oid = NULL;
            unsigned int        ip4_addrs_num;
            cfg_handle         *ip4_addrs;
            struct sockaddr    *ip4_addr = NULL;
            uint8_t             mac[ETHER_ADDR_LEN];

            /* Get IPv4 address assigned to the node */
            rc = cfg_get_oid_str(net->nodes[j].handle, &node_oid);
            if (rc != 0)
            {
                ERROR("Failed to string OID by handle: %r", rc);
                break;
            }

            /* Get IPv4 addresses of the node */
            rc = cfg_find_pattern_fmt(&ip4_addrs_num, &ip4_addrs,
                                      "%s/ip4_address:*", node_oid);
            if (rc != 0)
            {
                ERROR("Failed to find IPv4 addresses assigned to node "
                      "'%s': %r", node_oid, rc);
                free(node_oid);
                break;
            }
            if (ip4_addrs_num == 0)
            {
                ERROR("No IPv4 addresses are assigned to node '%s'",
                      node_oid);
                free(ip4_addrs);
                free(node_oid);
                rc = TE_EENV;
                break;
            }
            val_type = CVT_ADDRESS;
            rc = cfg_get_instance(ip4_addrs[0], &val_type, &ip4_addr);
            free(ip4_addrs);
            free(node_oid);
            if (rc != 0)
            {
                ERROR("Failed to get node IPv4 address: %r", rc);
                break;
            }

            /* Get MAC address of the network interface */
            val_type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[j].handle, &val_type,
                                  &if_oid);
            if (rc != 0)
            {
                ERROR("Failed to get Configurator instance by handle "
                      "0x%x: %r", net->nodes[j].handle, rc);
                free(ip4_addr);
                break;
            }
            rc = tapi_cfg_base_if_get_mac(if_oid, mac);
            if (rc != 0)
            {
                ERROR("Failed to get MAC address of %s: %r", if_oid, rc);
                free(if_oid);
                free(ip4_addr);
                break;
            }
            free(if_oid);

            /* Add static ARP entry for all other nodes */
            for (k = 0; k < net->n_nodes; ++k)
            {
                if (j == k)
                    continue;

                /* Get network node OID and agent name in it */
                val_type = CVT_STRING;
                rc = cfg_get_instance(net->nodes[k].handle, &val_type,
                                      &if_oid);
                if (rc != 0)
                {
                    ERROR("Failed to string OID by handle: %r", rc);
                    break;
                }
                oid = cfg_convert_oid_str(if_oid);
                if (oid == NULL)
                {
                    ERROR("Failed to convert OID from string '%s' to "
                          "struct", if_oid);
                    free(if_oid);
                    break;
                }
                free(if_oid);

                /* Should we use static ARP for this TA? */
                rc = cfg_get_instance_int_fmt(&use_static_arp,
                                              "/local:%s/use_static_arp:",
                                              CFG_OID_GET_INST_NAME(oid, 1));
                if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
                {
                    use_static_arp = use_static_arp_def;
                    rc = 0;
                }
                else if (rc != 0)
                {
                    ERROR("Failed to get /local:%s/use_static_arp: "
                          "value: %r", CFG_OID_GET_INST_NAME(oid, 1),
                          rc);
                    break;
                }
                if (!use_static_arp)
                {
                    cfg_free_oid(oid);
                    continue;
                }

                /* Add static ARP entry */
                rc = tapi_cfg_add_neigh_entry(CFG_OID_GET_INST_NAME(oid, 1),
                                              CFG_OID_GET_INST_NAME(oid, 2),
                                              ip4_addr, mac, TRUE);
                if (rc != 0)
                {
                    ERROR("Failed to add static ARP entry to TA '%s': %r",
                          CFG_OID_GET_INST_NAME(oid, 1), rc);
                    cfg_free_oid(oid);
                    break;
                }
                cfg_free_oid(oid);
            }
            free(ip4_addr);
            if (rc != 0)
                break;
        }
        if (rc != 0)
            break;

        if (ipv6_supp)
        {
            rc = tapi_cfg_net_assign_ip(AF_INET6, net, NULL);
            if (rc != 0)
            {
                ERROR("Failed to assign IPv6 subnet to net #%u: %r", i, rc);
                break;
            }
        }
    }
    tapi_cfg_net_free_nets(&nets);
    if (rc != 0)
    {
        TEST_FAIL("Failed to prepare testing networks");
    }
}

/* See description in tapi_network.h */
te_errno
tapi_neight_flush(rcf_rpc_server *rpcs, const char *ifname)
{
    rpc_wait_status st;

    RPC_AWAIT_IUT_ERROR(rpcs);
    st = rpc_system_ex(rpcs, IP_TOOL " neigh flush dev %s", ifname);
    if (st.flag != RPC_WAIT_STATUS_EXITED || st.value != 0)
    {
        ERROR("Failed to flush ARP table for interface %s/%s", rpcs->ta,
              ifname);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return 0;
}

/**
 * Callback function to flush ARP table on an interface.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    RPC server handle
 *
 * @return Status code. The loop is stopped if an error happens.
 */
static te_errno
flush_neight_cb(const char *ta, const char *ifname, void *opaque)
{
    UNUSED(ta);
    return tapi_neight_flush((rcf_rpc_server *)opaque, ifname);
}

/* See description in tapi_network.h */
te_errno
tapi_neight_flush_ta(rcf_rpc_server *rpcs)
{
    return tapi_host_ns_if_ta_iter(rpcs->ta, &flush_neight_cb, (void *)rpcs);
}
