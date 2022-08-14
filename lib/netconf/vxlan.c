/** @file
 * @brief Virtual eXtensible Local Area Network (vxlan)
 * interfaces management using netconf library
 *
 * Implementation of VXLAN interfaces configuration commands.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Netconf VXLAN"

#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"

/* VXLAN link kind to pass in IFLA_INFO_KIND. */
#define NETCONF_LINK_KIND_VXLAN "vxlan"

/**
 * Check whether link is VXLAN.
 *
 * @param linkgen   The general link attributes array
 *
 * @return @c TRUE if link is VXLAN.
 */
static te_bool
vxlan_link_is_vxlan(struct rtattr **linkgen)
{
    struct rtattr *linkinfo[IFLA_INFO_MAX + 1];

    netconf_parse_rtattr_nested(linkgen[IFLA_LINKINFO], linkinfo,
                                IFLA_INFO_MAX);

    if (linkinfo[IFLA_INFO_KIND] == NULL ||
        strcmp(RTA_DATA(linkinfo[IFLA_INFO_KIND]),
               NETCONF_LINK_KIND_VXLAN) != 0)
        return FALSE;

    return TRUE;
}

/* See netconf.h */
int
vxlan_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    netconf_vxlan       vxlan;
    struct rtattr      *linkgen[IFLA_MAX + 1];

    UNUSED(cookie);

    memset(&vxlan, 0, sizeof(vxlan));

    netconf_parse_link(h, linkgen, IFLA_MAX);

    if (linkgen[IFLA_LINKINFO] == NULL || linkgen[IFLA_IFNAME] == NULL ||
        vxlan_link_is_vxlan(linkgen) == FALSE)
        return 0;

    vxlan.generic.ifname = netconf_dup_rta(linkgen[IFLA_IFNAME]);
    if (vxlan.generic.ifname == NULL)
        return -1;

    if (netconf_list_extend(list, NETCONF_NODE_VXLAN) != 0)
    {
        free(vxlan.generic.ifname);
        return -1;
    }

    memcpy(&list->tail->data.vxlan, &vxlan, sizeof(vxlan));

    return 0;
}

/* See netconf_internal.h */
void
netconf_vxlan_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    netconf_udp_tunnel_free(&(node->data.vxlan.generic));

    free(node->data.vxlan.dev);
    free(node);
}

/* See netconf.h */
te_errno
netconf_vxlan_add(netconf_handle nh, const netconf_vxlan *vxlan)
{
    char                req[NETCONF_MAX_REQ_LEN];
    uint32_t            index;
    struct nlmsghdr    *h;
    struct rtattr      *linkinfo;
    struct rtattr      *data;
    uint16_t            port = htons(vxlan->generic.port);

    memset(req, 0, sizeof(req));
    netconf_init_nlmsghdr(req, nh, RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK |
                          NLM_F_CREATE | NLM_F_EXCL, &h);

    netconf_append_rta(h, vxlan->generic.ifname,
                       strlen(vxlan->generic.ifname) + 1, IFLA_IFNAME);
    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);
    netconf_append_rta(h, NETCONF_LINK_KIND_VXLAN,
                       strlen(NETCONF_LINK_KIND_VXLAN) + 1, IFLA_INFO_KIND);
    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);

    netconf_append_rta(h, &(vxlan->generic.vni), sizeof(vxlan->generic.vni),
                       IFLA_VXLAN_ID);

    switch (vxlan->generic.remote_len)
    {
        case sizeof(struct in_addr):
            netconf_append_rta(h, vxlan->generic.remote,
                               vxlan->generic.remote_len, IFLA_VXLAN_GROUP);
            break;

#if HAVE_DECL_IFLA_VXLAN_GROUP6
        case sizeof(struct in6_addr):
            netconf_append_rta(h, vxlan->generic.remote,
                               vxlan->generic.remote_len, IFLA_VXLAN_GROUP6);
            break;
#endif

        case 0:
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    switch (vxlan->local_len)
    {
        case sizeof(struct in_addr):
            netconf_append_rta(h, vxlan->local, vxlan->local_len,
                               IFLA_VXLAN_LOCAL);
            break;

#if HAVE_DECL_IFLA_VXLAN_LOCAL6
        case sizeof(struct in6_addr):
            netconf_append_rta(h, vxlan->local, vxlan->local_len,
                               IFLA_VXLAN_LOCAL6);
            break;
#endif

        case 0:
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    netconf_append_rta(h, &port, sizeof(port), IFLA_VXLAN_PORT);

    if (vxlan->dev != NULL && strlen(vxlan->dev) != 0)
    {
        IFNAME_TO_INDEX(vxlan->dev, index);
        netconf_append_rta(h, &index, sizeof(index), IFLA_VXLAN_LINK);
    }

    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL, NULL) != 0) {
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/* See netconf.h */
te_errno
netconf_vxlan_list(netconf_handle nh,
                   netconf_udp_tunnel_list_filter_func filter_cb,
		   void *filter_opaque, char **list)
{
    return netconf_udp_tunnel_list(nh, filter_cb, filter_opaque, list,
                                   NETCONF_LINK_KIND_VXLAN);
}
