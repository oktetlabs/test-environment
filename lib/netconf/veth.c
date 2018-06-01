/** @file
 * @brief Virtual Ethernet (veth) interfaces management using netconf library
 *
 * Implementation of VETH interfaces configuration commands.
 *
 * Copyright (C) 2003-2018 OKTET Labs.
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#define TE_LGR_USER "Netconf VETH"

#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "te_config.h"
#include "te_alloc.h"

/* Assume the linux is modern enough to have veth.h, otherwise - break the
 * build. */
#include <linux/veth.h>

/* Veth link kind to pass in IFLA_INFO_KIND. */
#define NETCONF_LINK_KIND_VETH "veth"

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
veth_init_nlmsghdr(char *req, netconf_handle nh, uint16_t nlmsg_type,
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
veth_parse_link(struct nlmsghdr *h, struct rtattr **rta_arr, int max)
{
    struct rtattr      *rta_link;
    int                 len;

    rta_link = (struct rtattr *)((char *)h +
                                 NLMSG_SPACE(sizeof(struct ifinfomsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ifinfomsg));
    netconf_parse_rtattr(rta_link, len, rta_arr, max);
}

/**
 * Check if link is veth.
 *
 * @param linkgen   The general link attributes array
 *
 * @return @c TRUE if link is veth.
 */
static te_bool
veth_link_is_veth(struct rtattr **linkgen)
{
    struct rtattr *linkinfo[IFLA_INFO_MAX + 1];

    netconf_parse_rtattr_nested(linkgen[IFLA_LINKINFO], linkinfo,
                                IFLA_INFO_MAX);

    if (linkinfo[IFLA_INFO_KIND] == NULL ||
        strcmp(RTA_DATA(linkinfo[IFLA_INFO_KIND]),
               NETCONF_LINK_KIND_VETH) != 0)
        return FALSE;

    return TRUE;
}

/**
 * Extract veth peer name.
 *
 * @param linkgen   The general link attributes array
 * @param peer_name The peer name location
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static int
veth_extract_peer(struct rtattr **linkgen, char **peer_name)
{
    int32_t peer_ifidx = 0;
    char    peer[IFNAMSIZ];

    if (linkgen[IFLA_LINK] != NULL)
        peer_ifidx = netconf_get_rta_u32(linkgen[IFLA_LINK]);

    /* Peer index can be not set on old linux kernels, e.g. 3.16.0-4-amd64.
     * But it is specified in modern kernels - tested with 4.10.0-27-generic,
     * 3.10.0-514.26.2.el7.x86_64. */
    if (peer_ifidx != 0)
    {
        if (if_indextoname(peer_ifidx, peer) == NULL)
        {
            /* No such device in the current namespace - return nothing but
             * don't fail. */
            if (errno == ENXIO)
            {
                *peer_name = NULL;
                return 0;
            }

            ERROR("Failed to convert interface index to name: %s",
                  strerror(errno));
            return -1;
        }

        *peer_name = strdup(peer);
        if (*peer_name == NULL)
        {
            ERROR("Cannot dup the peer interface name");
            return -1;
        }
    }
    else
    {
        *peer_name = NULL;
    }

    return 0;
}

/**
 * Decode veth link data from a netlink message.
 *
 * @param h         The netlink message header
 * @param list      Netconf list to keep the data
 * @param get_peer  Extract veth peer name if @c TRUE
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static int
veth_link_gen_cb(struct nlmsghdr *h, netconf_list *list, te_bool get_peer)
{
    netconf_veth        veth;
    struct rtattr      *linkgen[IFLA_MAX + 1];

    memset(&veth, 0, sizeof(veth));

    veth_parse_link(h, linkgen, IFLA_MAX);

    if (linkgen[IFLA_LINKINFO] == NULL || linkgen[IFLA_IFNAME] == NULL ||
        veth_link_is_veth(linkgen) == FALSE)
        return 0;

    if (get_peer)
    {
        if (veth_extract_peer(linkgen, &veth.peer) != 0)
            return -1;
    }

    veth.ifname = netconf_dup_rta(linkgen[IFLA_IFNAME]);
    if (veth.ifname == NULL)
    {
        free(veth.peer);
        return -1;
    }

    if (netconf_list_extend(list, NETCONF_NODE_VETH) != 0)
    {
        free(veth.ifname);
        free(veth.peer);
        return -1;
    }

    memcpy(&list->tail->data.veth, &veth, sizeof(veth));

    return 0;
}

/**
 * Callback function to decode veth link data with the peer name.
 *
 * @param h         The netlink message header
 * @param list      Netconf list to keep the data
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static int
veth_peer_cb(struct nlmsghdr *h, netconf_list *list)
{
    return veth_link_gen_cb(h, list, TRUE);
}

/**
 * Callback function to decode veth link data without the peer name.
 *
 * @param h         The netlink message header
 * @param list      Netconf list to keep the data
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static int
veth_list_cb(struct nlmsghdr *h, netconf_list *list)
{
    return veth_link_gen_cb(h, list, FALSE);
}

/* See netconf_internal.h */
void
netconf_veth_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.veth.ifname);
    free(node->data.veth.peer);
    free(node);
}

/* See netconf.h */
te_errno
netconf_veth_add(netconf_handle nh, const char *ifname, const char *peer)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct rtattr      *linkinfo;
    struct rtattr      *peerinfo;
    struct rtattr      *data;

    memset(req, 0, sizeof(req));
    veth_init_nlmsghdr(req, nh, RTM_NEWLINK,
                       NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL,
                       &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);
    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);

    netconf_append_rta(h, NETCONF_LINK_KIND_VETH,
                       strlen(NETCONF_LINK_KIND_VETH) + 1, IFLA_INFO_KIND);

    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);

    netconf_append_rta_nested(h, VETH_INFO_PEER, &peerinfo);
    h->nlmsg_len += sizeof(struct ifinfomsg);

    netconf_append_rta(h, peer, strlen(peer) + 1, IFLA_IFNAME);

    netconf_append_rta_nested_end(h, peerinfo);
    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See netconf.h */
te_errno
netconf_veth_del(netconf_handle nh, const char *ifname)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;

    memset(req, 0, sizeof(req));
    veth_init_nlmsghdr(req, nh, RTM_DELLINK, NLM_F_REQUEST | NLM_F_ACK, &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See netconf.h */
te_errno
netconf_veth_get_peer(netconf_handle nh, const char *ifname, char *peer,
                      size_t peer_len)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    netconf_list       *list;
    int                 rc;

    memset(req, 0, sizeof(req));
    veth_init_nlmsghdr(req, nh, RTM_GETLINK, NLM_F_REQUEST | NLM_F_ACK, &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);

    list = TE_ALLOC(sizeof(*list));
    if (list == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    rc = netconf_talk(nh, req, sizeof(req), veth_peer_cb, list);
    if (rc != 0)
    {
        netconf_list_free(list);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if (list->head != NULL && list->head->data.veth.peer != NULL)
    {
        size_t len = strlen(list->head->data.veth.peer) + 1;
        if (len > peer_len)
        {
            netconf_list_free(list);
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }
        memcpy(peer, list->head->data.veth.peer, len);
    }
    else
        peer[0] = '\0';

    netconf_list_free(list);

    return 0;
}

/* See netconf.h */
te_errno
netconf_veth_list(netconf_handle nh, netconf_veth_list_filter_func filter_cb,
                  void *filter_opaque, char **list)
{
    netconf_list *nlist;
    netconf_node *node;
    te_string     str = TE_STRING_INIT;
    te_errno      rc = 0;

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC, veth_list_cb);
    if (nlist == NULL)
    {
        ERROR("Failed to get veth interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.veth.ifname != NULL)
        {
            if (filter_cb != NULL &&
                filter_cb(node->data.veth.ifname, filter_opaque) == FALSE)
                continue;

            rc = te_string_append(&str, "%s ", node->data.veth.ifname);
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
