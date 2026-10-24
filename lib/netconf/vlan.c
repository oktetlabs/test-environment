/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief VLAN interfaces management using netconf library
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Netconf VLAN"

#include "te_config.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"

#define NETCONF_LINK_KIND_VLAN "vlan"

/* See description in netconf.h */
te_errno
netconf_vlan_modify(netconf_handle nh, netconf_cmd cmd,
                    const char *link, const char *ifname,
                    unsigned int vid)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct ifinfomsg   *ifmsg;
    struct rtattr      *linkinfo;
    struct rtattr      *data;
    char                vlan_ifname[IFNAMSIZ];
    uint16_t            vid_attr;
    int                 rc;

    if (ifname == NULL || ifname[0] == '\0')
    {
        if (cmd == NETCONF_CMD_ADD)
        {
            rc = snprintf(vlan_ifname, IFNAMSIZ, "%s.%u",
                          link, vid);

            if (rc >= IFNAMSIZ)
            {
                WARN("VLAN interface name was truncated to '%s'",
                     vlan_ifname);
            }
        }
        else
        {
            rc = netconf_vlan_get_ifname(nh, link, vid,
                                         vlan_ifname, IFNAMSIZ);
            if (rc != 0)
                return rc;
        }
    }
    else
    {
        rc = snprintf(vlan_ifname, IFNAMSIZ, "%s",
                      ifname);

        if (rc >= IFNAMSIZ)
            WARN("VLAN interface name was truncated to '%s'", vlan_ifname);
    }

    /* Generate request */
    memset(&req, 0, sizeof(req));

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(*ifmsg));
    h->nlmsg_type = (cmd == NETCONF_CMD_DEL) ?
                    RTM_DELLINK : RTM_NEWLINK;

    h->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    if (cmd == NETCONF_CMD_ADD)
        h->nlmsg_flags |= NLM_F_CREATE | NLM_F_EXCL;

    h->nlmsg_seq = ++nh->seq;

    ifmsg = NLMSG_DATA(h);

    if (cmd == NETCONF_CMD_DEL)
        IFNAME_TO_INDEX(vlan_ifname, ifmsg->ifi_index);

    if (link != NULL)
    {
        uint32_t link_index;

        IFNAME_TO_INDEX(link, link_index);
        netconf_append_rta(h, &link_index, sizeof(link_index), IFLA_LINK);
    }

    netconf_append_rta(h, vlan_ifname, strlen(vlan_ifname) + 1,
                       IFLA_IFNAME);

    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);
    netconf_append_rta(h, NETCONF_LINK_KIND_VLAN,
                       strlen(NETCONF_LINK_KIND_VLAN) + 1,
                       IFLA_INFO_KIND);

    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);
    vid_attr = vid;
    netconf_append_rta(h, &vid_attr, sizeof(vid_attr), IFLA_VLAN_ID);

    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, &req, sizeof(req), NULL, NULL, NULL) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See description in netconf.h */
void
netconf_vlan_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.vlan.ifname);
    free(node->data.vlan.egress_qos);
    free(node);
}

/**
 * Parse the egress QoS map attribute of a VLAN.
 *
 * @param rta       @c IFLA_VLAN_EGRESS_QOS attribute
 * @param vlan      Where to store the mappings
 */
static void
vlan_parse_egress_qos(struct rtattr *rta, netconf_vlan *vlan)
{
    struct rtattr *map;
    int len;
    unsigned int n = 0;

    for (map = RTA_DATA(rta), len = RTA_PAYLOAD(rta); RTA_OK(map, len);
         map = RTA_NEXT(map, len))
    {
        if (map->rta_type == IFLA_VLAN_QOS_MAPPING &&
            RTA_PAYLOAD(map) >= sizeof(struct ifla_vlan_qos_mapping))
            n++;
    }
    if (n == 0)
        return;

    vlan->egress_qos = TE_ALLOC(n * sizeof(*vlan->egress_qos));
    for (map = RTA_DATA(rta), len = RTA_PAYLOAD(rta); RTA_OK(map, len);
         map = RTA_NEXT(map, len))
    {
        const struct ifla_vlan_qos_mapping *m = RTA_DATA(map);

        if (map->rta_type != IFLA_VLAN_QOS_MAPPING ||
            RTA_PAYLOAD(map) < sizeof(*m))
            continue;
        vlan->egress_qos[vlan->n_egress_qos].from = m->from;
        vlan->egress_qos[vlan->n_egress_qos].to = m->to;
        vlan->n_egress_qos++;
    }
}

/**
 * Callback for getting VLAN interfaces dump.
 *
 * @param h             Message header
 * @param list          Where to store interfaces data
 * @param cookie        Extra parameters (unused)
 *
 * @return @c 0 on success, @c -1 on error.
 */
static netconf_recv_cb_t vlan_list_cb;
static int
vlan_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    struct ifinfomsg   *ifla = NLMSG_DATA(h);
    netconf_vlan        vlan;
    struct rtattr      *rta_arr[IFLA_MAX + 1];
    struct rtattr      *rta;
    int                 len;

    UNUSED(cookie);

    memset(&vlan, 0, sizeof(vlan));

    rta = (struct rtattr *)((char *)h + NLMSG_SPACE(sizeof(*ifla)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(*ifla));

    netconf_parse_rtattr(rta, len, rta_arr, IFLA_MAX);

    if (rta_arr[IFLA_LINKINFO] != NULL)
    {
        struct rtattr *linkinfo[IFLA_INFO_MAX + 1];
        struct rtattr *vlan_data[IFLA_VLAN_MAX + 1];

        netconf_parse_rtattr_nested(rta_arr[IFLA_LINKINFO], linkinfo,
                                    IFLA_INFO_MAX);

        if (linkinfo[IFLA_INFO_KIND] == NULL ||
            strcmp(RTA_DATA(linkinfo[IFLA_INFO_KIND]),
                   NETCONF_LINK_KIND_VLAN) != 0)
            return 0;

        if (linkinfo[IFLA_INFO_DATA] == NULL)
            return 0;

        netconf_parse_rtattr_nested(linkinfo[IFLA_INFO_DATA], vlan_data,
                                    IFLA_VLAN_MAX);

        if (vlan_data[IFLA_VLAN_ID] == NULL)
            return 0;
        vlan.vid = netconf_get_rta_u32(vlan_data[IFLA_VLAN_ID]);
        if (vlan_data[IFLA_VLAN_EGRESS_QOS] != NULL)
            vlan_parse_egress_qos(vlan_data[IFLA_VLAN_EGRESS_QOS], &vlan);
    }
    else
    {
        return 0;
    }

    vlan.ifindex = ifla->ifi_index;
    if (rta_arr[IFLA_LINK] != NULL)
        vlan.link = netconf_get_rta_u32(rta_arr[IFLA_LINK]);
    if (rta_arr[IFLA_IFNAME] != NULL)
        vlan.ifname = netconf_dup_rta(rta_arr[IFLA_IFNAME]);

    if (netconf_list_extend(list, NETCONF_NODE_VLAN) != 0)
    {
        free(vlan.ifname);
        free(vlan.egress_qos);
        return -1;
    }

    memcpy(&list->tail->data.vlan, &vlan, sizeof(vlan));

    return 0;
}

/* See description in netconf.h */
te_errno
netconf_vlan_list(netconf_handle nh, const char *link, te_vec *names)
{
    netconf_list *nlist;
    netconf_node *node;
    int           index;

    if (link == NULL || names == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    IFNAME_TO_INDEX(link, index);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 vlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get VLAN interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.vlan.link == index)
        {
            char *name = te_string_fmt("%u",
                                       (unsigned int)node->data.vlan.vid);

            TE_VEC_APPEND(names, name);
        }
    }

    netconf_list_free(nlist);

    return 0;
}

/* See description in netconf.h */
te_errno
netconf_vlan_get_ifname(netconf_handle nh, const char *link, unsigned int vid,
                        char *ifname, size_t len)
{
    netconf_list *nlist;
    netconf_node *node;
    int           index;
    te_errno      rc = 0;

    if (link == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    IFNAME_TO_INDEX(link, index);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 vlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get VLAN interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.vlan.link == index &&
            node->data.vlan.vid == vid)
        {
            if (te_strlcpy(ifname, node->data.vlan.ifname, len) >= len)
            {
                ERROR("Interface name '%s' is too long to fit "
                      "into provided buffer", node->data.vlan.ifname);
                rc = TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
            }
            break;
        }
    }

    netconf_list_free(nlist);

    if (rc == 0 && node == NULL)
    {
        ERROR("Failed to find VLAN ID %u on %s", vid, link);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return rc;
}

/**
 * Find a VLAN in a dump.
 *
 * @param nlist     Dump of the VLAN interfaces
 * @param index     Index of the link interface
 * @param vid       VLAN ID
 *
 * @return The node, or @c NULL
 */
static netconf_node *
vlan_find(netconf_list *nlist, int index, unsigned int vid)
{
    netconf_node *node;

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.vlan.link == index && node->data.vlan.vid == vid)
            return node;
    }

    return NULL;
}

/* See description in netconf.h */
te_errno
netconf_vlan_egress_qos_set(netconf_handle nh, const char *link,
                            unsigned int vid, uint32_t from, uint32_t to)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct ifinfomsg   *ifmsg;
    struct rtattr      *linkinfo;
    struct rtattr      *data;
    struct rtattr      *qos;
    struct ifla_vlan_qos_mapping map = { .from = from, .to = to };
    netconf_list       *nlist;
    netconf_node       *node;
    int                 index;
    int                 ifindex;

    if (link == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    IFNAME_TO_INDEX(link, index);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 vlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get VLAN interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    node = vlan_find(nlist, index, vid);
    if (node == NULL)
    {
        netconf_list_free(nlist);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    ifindex = node->data.vlan.ifindex;
    netconf_list_free(nlist);

    memset(&req, 0, sizeof(req));
    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(*ifmsg));
    h->nlmsg_type = RTM_NEWLINK;
    h->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    h->nlmsg_seq = ++nh->seq;

    ifmsg = NLMSG_DATA(h);
    ifmsg->ifi_index = ifindex;

    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);
    netconf_append_rta(h, NETCONF_LINK_KIND_VLAN,
                       strlen(NETCONF_LINK_KIND_VLAN) + 1,
                       IFLA_INFO_KIND);
    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);
    netconf_append_rta_nested(h, IFLA_VLAN_EGRESS_QOS, &qos);
    netconf_append_rta(h, &map, sizeof(map), IFLA_VLAN_QOS_MAPPING);
    netconf_append_rta_nested_end(h, qos);
    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, &req, sizeof(req), NULL, NULL, NULL) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See description in netconf.h */
te_errno
netconf_vlan_egress_qos_get(netconf_handle nh, const char *link,
                            unsigned int vid, te_vec *maps)
{
    netconf_list *nlist;
    netconf_node *node;
    unsigned int  i;
    int           index;
    te_errno      rc = 0;

    if (link == NULL || maps == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    IFNAME_TO_INDEX(link, index);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 vlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get VLAN interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    node = vlan_find(nlist, index, vid);
    if (node == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    else
    {
        for (i = 0; i < node->data.vlan.n_egress_qos; i++)
            TE_VEC_APPEND(maps, node->data.vlan.egress_qos[i]);
    }

    netconf_list_free(nlist);

    return rc;
}
