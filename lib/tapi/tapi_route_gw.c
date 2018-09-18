/** @file
 * @brief Test GateWay network configuring API
 *
 * Implementation of functions for gateway configuration to be
 * used in tests. "Gateway" here is the third host which forwards
 * packets between two testing hosts not connected directly.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"
#include "logger_api.h"
#include "rcf_api.h"

#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg.h"
#include "tapi_test.h"

#include "tapi_route_gw.h"

/**
 * If expression evaluates to non-zero,
 * return its value.
 *
 * @param expr_     Expression to check.
 */
#define RETURN_ON_ERROR(expr_) \
    do {                        \
        te_errno rc_;           \
                                \
        rc_ = expr_;            \
        if (rc_ != 0)           \
            return rc_;         \
    } while (0)

/* See description in tapi_route_gw.h */
te_errno
tapi_update_arp(const char *ta_src,
                const char *ifname_src,
                const char *ta_dest,
                const char *ifname_dest,
                const struct sockaddr *addr_dest,
                const struct sockaddr *link_addr_dest,
                te_bool is_static)
{
    const unsigned int      max_attempts = 10;
    unsigned int            i;
    static struct sockaddr  link_addr;
    te_errno                rc;

    if (link_addr_dest != NULL)
    {
        memcpy(&link_addr, link_addr_dest, sizeof(link_addr));
    }
    else if (ta_dest != NULL && ifname_dest != NULL)
    {
        RETURN_ON_ERROR(tapi_cfg_base_if_get_link_addr(
                                            ta_dest,
                                            ifname_dest,
                                            &link_addr));
    }
    else
    {
        ERROR("Wrong options combination to change arp table");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    for (i = 0; i < max_attempts; i++)
    {
        RETURN_ON_ERROR(tapi_cfg_del_neigh_entry(
                                  ta_src, ifname_src,
                                  addr_dest));

        rc = tapi_cfg_add_neigh_entry(ta_src, ifname_src,
                                      addr_dest, link_addr.sa_data,
                                      is_static);
        if (rc == 0)
            break;
        else if (rc != TE_RC(TE_CS, TE_EEXIST))
            return rc;
    }

    return rc;
}

/* See description in tapi_route_gw.h */
te_errno
tapi_remove_arp(const char *ta,
                const char *if_name,
                const struct sockaddr *net_addr)
{
    const unsigned int max_attempts = 10;
    unsigned int       i;
    te_errno           rc = 0;

    for (i = 0; i < max_attempts; i++)
    {
        rc = tapi_cfg_del_neigh_entry(ta, if_name, net_addr);
        if (rc != 0)
            return rc;

        TAPI_WAIT_NETWORK;

        rc = tapi_cfg_get_neigh_entry(ta, if_name, net_addr,
                                      NULL, NULL, NULL);
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            return 0;
        else if (rc != 0)
            return rc;
    }

    ERROR("Failed to ensure that removed ARP entry does not reappear");
    return TE_RC(TE_TAPI, TE_EFAIL);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_init(
                  tapi_route_gateway *gw,
                  const char *iut_ta,
                  const char *tst_ta,
                  const char *gw_ta,
                  const struct if_nameindex *iut_if,
                  const struct if_nameindex *tst_if,
                  const struct if_nameindex *gw_iut_if,
                  const struct if_nameindex *gw_tst_if,
                  const struct sockaddr *iut_addr,
                  const struct sockaddr *tst_addr,
                  const struct sockaddr *gw_iut_addr,
                  const struct sockaddr *gw_tst_addr,
                  const struct sockaddr *alien_link_addr)
{
    if (snprintf(gw->iut_ta, RCF_MAX_NAME, "%s", iut_ta) >= RCF_MAX_NAME ||
        snprintf(gw->tst_ta, RCF_MAX_NAME, "%s", tst_ta) >= RCF_MAX_NAME ||
        snprintf(gw->gw_ta, RCF_MAX_NAME, "%s", gw_ta) >= RCF_MAX_NAME)
    {
        ERROR("%s(): TA name is too long", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    gw->iut_if = iut_if;
    gw->tst_if = tst_if;
    gw->gw_iut_if = gw_iut_if;
    gw->gw_tst_if = gw_tst_if;
    gw->iut_addr = iut_addr;
    gw->tst_addr = tst_addr;
    gw->gw_iut_addr = gw_iut_addr;
    gw->gw_tst_addr = gw_tst_addr;
    gw->alien_link_addr = alien_link_addr;

    return 0;
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_configure(tapi_route_gateway *gw)
{
    RETURN_ON_ERROR(
            tapi_cfg_add_route_via_gw(
               gw->iut_ta,
               gw->tst_addr->sa_family,
               te_sockaddr_get_netaddr(gw->tst_addr),
               te_netaddr_get_size(gw->tst_addr->sa_family) * 8,
               te_sockaddr_get_netaddr(gw->gw_iut_addr)));

    /*
     * We need to add IPv6 neighbors entries manually because there are cases
     * when Linux can not re-resolve FAILED entries for gateway routes.
     * See bug 9774.
     */
    if (gw->iut_addr->sa_family == AF_INET6)
    {
        RETURN_ON_ERROR(tapi_update_arp(gw->iut_ta, gw->iut_if->if_name,
                                        gw->gw_ta, gw->gw_iut_if->if_name,
                                        gw->gw_iut_addr, NULL, FALSE));
        RETURN_ON_ERROR(tapi_update_arp(gw->gw_ta, gw->gw_iut_if->if_name,
                                        gw->iut_ta, gw->iut_if->if_name,
                                        gw->iut_addr, NULL, FALSE));
    }

    RETURN_ON_ERROR(
            tapi_cfg_add_route_via_gw(
               gw->tst_ta,
               gw->iut_addr->sa_family,
               te_sockaddr_get_netaddr(gw->iut_addr),
               te_netaddr_get_size(gw->iut_addr->sa_family) * 8,
               te_sockaddr_get_netaddr(gw->gw_tst_addr)));

    if (gw->tst_addr->sa_family == AF_INET6)
    {
        RETURN_ON_ERROR(tapi_update_arp(gw->tst_ta, gw->tst_if->if_name,
                                        gw->gw_ta, gw->gw_tst_if->if_name,
                                        gw->gw_tst_addr, NULL, FALSE));
        RETURN_ON_ERROR(tapi_update_arp(gw->gw_ta, gw->gw_tst_if->if_name,
                                        gw->tst_ta, gw->tst_if->if_name,
                                        gw->tst_addr, NULL, FALSE));
    }

    return tapi_route_gateway_set_forwarding(gw, TRUE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_set_forwarding(tapi_route_gateway *gw,
                                  te_bool enabled)
{
    if (gw->gw_iut_addr->sa_family == AF_INET)
    {
        return tapi_cfg_base_ipv4_fw(gw->gw_ta, enabled);
    }
    else if (gw->gw_iut_addr->sa_family == AF_INET6)
    {
        return tapi_cfg_base_ipv6_fw(gw->gw_ta, enabled);
    }
    else
    {
        ERROR("Unsupported address family");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_break_gw_iut(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->gw_ta, gw->gw_iut_if->if_name, NULL, NULL,
                           gw->iut_addr, gw->alien_link_addr, TRUE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_repair_gw_iut(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->gw_ta, gw->gw_iut_if->if_name,
                           gw->iut_ta, gw->iut_if->if_name,
                           gw->iut_addr, NULL, FALSE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_break_gw_tst(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->gw_ta, gw->gw_tst_if->if_name, NULL, NULL,
                           gw->tst_addr, gw->alien_link_addr, TRUE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_repair_gw_tst(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->gw_ta, gw->gw_tst_if->if_name,
                           gw->tst_ta, gw->tst_if->if_name,
                           gw->tst_addr, NULL, FALSE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_break_iut_gw(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->iut_ta, gw->iut_if->if_name, NULL, NULL,
                           gw->gw_iut_addr, gw->alien_link_addr, TRUE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_repair_iut_gw(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->iut_ta, gw->iut_if->if_name, gw->gw_ta,
                           gw->gw_iut_if->if_name,
                           gw->gw_iut_addr, NULL, FALSE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_break_tst_gw(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->tst_ta, gw->tst_if->if_name, NULL, NULL,
                           gw->gw_tst_addr, gw->alien_link_addr, TRUE);
}

/* See description in tapi_route_gw.h */
te_errno
tapi_route_gateway_repair_tst_gw(tapi_route_gateway *gw)
{
    return tapi_update_arp(gw->tst_ta, gw->tst_if->if_name, gw->gw_ta,
                           gw->gw_tst_if->if_name,
                           gw->gw_tst_addr, NULL, FALSE);
}
