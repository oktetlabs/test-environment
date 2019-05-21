/** @file
 * @brief Virtual eXtensible Local Area Network (vxlan)
 * interfaces management using netconf library
 *
 * Implementation of VXLAN interfaces configuration commands.
 *
 * Copyright (C) 2019 OKTET Labs.
 *
 * @author Ian Dolzhansky <Ian.Dolzhansky@oktetlabs.ru>
 */

#define TE_LGR_USER "Netconf VXLAN"

#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "te_config.h"
#include "te_alloc.h"

/* VXLAN link kind to pass in IFLA_INFO_KIND. */
#define NETCONF_LINK_KIND_VXLAN "vxlan"

/**
 * Parse the general link attribute.
 *
 * @param nh        Netconf session handle
 * @param rta_arr   Sorted attributes pointers array
 * @param max       Maximum index of the array
 */
static void
vxlan_parse_link(struct nlmsghdr *h, struct rtattr **rta_arr, int max)
{
    struct rtattr      *rta_link;
    int                 len;

    rta_link = (struct rtattr *)((char *)h +
                                 NLMSG_SPACE(sizeof(struct ifinfomsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ifinfomsg));
    netconf_parse_rtattr(rta_link, len, rta_arr, max);
}

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

/**
 * Decode VXLAN link data from a netlink message.
 *
 * @param h         The netlink message header
 * @param list      Netconf list to keep the data
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static int
vxlan_link_gen_cb(struct nlmsghdr *h, netconf_list *list)
{
    netconf_vxlan       vxlan;
    struct rtattr      *linkgen[IFLA_MAX + 1];

    memset(&vxlan, 0, sizeof(vxlan));

    vxlan_parse_link(h, linkgen, IFLA_MAX);

    if (linkgen[IFLA_LINKINFO] == NULL || linkgen[IFLA_IFNAME] == NULL ||
        vxlan_link_is_vxlan(linkgen) == FALSE)
        return 0;

    vxlan.ifname = netconf_dup_rta(linkgen[IFLA_IFNAME]);
    if (vxlan.ifname == NULL)
    {
        return -1;
    }

    if (netconf_list_extend(list, NETCONF_NODE_VXLAN) != 0)
    {
        free(vxlan.ifname);
        return -1;
    }

    memcpy(&list->tail->data.vxlan, &vxlan, sizeof(vxlan));

    return 0;
}

/**
 * Callback function to decode VXLAN link data.
 *
 * @param h         The netlink message header
 * @param list      Netconf list to keep the data
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static int
vxlan_list_cb(struct nlmsghdr *h, netconf_list *list)
{
    return vxlan_link_gen_cb(h, list);
}

/* See netconf.h */
te_errno
netconf_vxlan_list(netconf_handle nh, netconf_vxlan_list_filter_func filter_cb,
		   void *filter_opaque, char **list)
{
    netconf_list   *nlist;
    netconf_node   *node;
    te_string       str = TE_STRING_INIT;
    te_errno        rc = 0;

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC, vxlan_list_cb);
    if (nlist == NULL)
    {
        ERROR("Failed to get vxlan interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.vxlan.ifname != NULL)
        {
            if (filter_cb != NULL &&
                filter_cb(node->data.vxlan.ifname, filter_opaque) == FALSE)
                continue;

            rc = te_string_append(&str, "%s ", node->data.vxlan.ifname);
            if (rc != 0)
            {
                te_string_free(&str);
                rc = TE_RC(TE_TA_UNIX, rc);
                break;
            }
        }
    }

    netconf_list_free(nlist);

    if (rc == 0)
        *list = str.ptr;

    return rc;
}
