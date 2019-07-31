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

/* VXLAN link kind to pass in IFLA_INFO_KIND. */
#define NETCONF_LINK_KIND_VXLAN "vxlan"

/**
 * Initialize @p hdr.
 *
 * @param req           Request buffer
 * @param nh            Netconf session handle
 * @param nlmsg_type    Netlink message type
 * @param nlmsg_flags   Netlink message flags
 * @param hdr           Pointer to the netlink message header
 */
static void
vxlan_init_nlmsghdr(char *req, netconf_handle nh, uint16_t nlmsg_type,
                   uint16_t nlmsg_flags, struct nlmsghdr **hdr)
{
    struct nlmsghdr *h;

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    h->nlmsg_type = nlmsg_type;
    h->nlmsg_flags = nlmsg_flags;
    h->nlmsg_seq = ++nh->seq;

    *hdr = h;
}

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
        return -1;

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

/* See netconf_internal.h */
void
netconf_vxlan_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.vxlan.ifname);
    free(node);
}

/* See netconf.h */
te_errno
netconf_vxlan_add(netconf_handle nh, const netconf_vxlan *vxlan)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct rtattr      *linkinfo;
    struct rtattr      *data;

    memset(req, 0, sizeof(req));
    vxlan_init_nlmsghdr(req, nh, RTM_NEWLINK,
                        NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL,
                        &h);

    netconf_append_rta(h, vxlan->ifname, strlen(vxlan->ifname) + 1,
                       IFLA_IFNAME);
    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);
    netconf_append_rta(h, NETCONF_LINK_KIND_VXLAN,
                       strlen(NETCONF_LINK_KIND_VXLAN) + 1, IFLA_INFO_KIND);
    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);

    netconf_append_rta(h, &(vxlan->vni), sizeof(vxlan->vni), IFLA_VXLAN_ID);

    switch (vxlan->remote_len)
    {
        case sizeof(struct in_addr):
            netconf_append_rta(h, vxlan->remote, vxlan->remote_len,
                               IFLA_VXLAN_GROUP);
            break;

#if HAVE_DECL_IFLA_VXLAN_GROUP6
        case sizeof(struct in6_addr):
            netconf_append_rta(h, vxlan->remote, vxlan->remote_len,
                               IFLA_VXLAN_GROUP6);
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

    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL) != 0) {
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/* See netconf.h */
te_errno
netconf_vxlan_del(netconf_handle nh, const char *ifname)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;

    memset(req, 0, sizeof(req));
    vxlan_init_nlmsghdr(req, nh, RTM_DELLINK, NLM_F_REQUEST | NLM_F_ACK, &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
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
