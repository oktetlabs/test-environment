/** @file
 * @brief MAC VLAN interfaces management using netconf library
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#define TE_LGR_USER "Netconf MAC VLAN"

#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "te_config.h"

#define NETCONF_LINK_KIND_MACVLAN "macvlan"

typedef struct macvlan_mode_map {
    uint32_t    val;
    const char *str;
} macvlan_mode_map;

static macvlan_mode_map macvlan_modes[] =
{
    {MACVLAN_MODE_PRIVATE, "private"},
    {MACVLAN_MODE_VEPA, "vepa"},
    {MACVLAN_MODE_BRIDGE, "bridge"},
    {MACVLAN_MODE_PASSTHRU, "passthru"}
};

/**
 * Convert MAC VLAN mode string @p str to value @p val.
 *
 * @return Status code
 */
static te_errno
macvlan_mode_str2val(const char *str, uint32_t *val)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(macvlan_modes); i++)
    {
        if (strcmp(macvlan_modes[i].str, str) == 0)
        {
            *val = macvlan_modes[i].val;
            return 0;
        }
    }

    ERROR("Unknown MAC VLAN mode '%s'", str);
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/**
 * Convert MAC VLAN mode value @p val to string @p str.
 *
 * @return Status code
 */
static te_errno
macvlan_mode_val2str(uint32_t val, const char **str)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(macvlan_modes); i++)
    {
        if (val == macvlan_modes[i].val)
        {
            *str = macvlan_modes[i].str;
            return 0;
        }
    }

    ERROR("Unknown MAC VLAN mode %"TE_PRINTF_32"u", val);
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

te_errno
netconf_macvlan_modify(netconf_handle nh, netconf_cmd cmd,
                       const char *link, const char *ifname,
                       const char *mode_str)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct ifinfomsg   *ifmsg;
    struct rtattr      *linkinfo;
    struct rtattr      *data;

    if (ifname == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

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

    if ((h->nlmsg_flags & NLM_F_CREATE) == 0)
        IFNAME_TO_INDEX(ifname, ifmsg->ifi_index);

    if (link != NULL)
    {
        uint32_t link_index;

        IFNAME_TO_INDEX(link, link_index);
        netconf_append_rta(h, &link_index, sizeof(link_index), IFLA_LINK);
    }

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);

    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);
    netconf_append_rta(h, NETCONF_LINK_KIND_MACVLAN,
                       strlen(NETCONF_LINK_KIND_MACVLAN) + 1,
                       IFLA_INFO_KIND);

    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);

    /* Don't specify MAC VLAN mode explicitly if empty line is passed. */
    if (mode_str != NULL && strlen(mode_str) > 0)
    {
        uint32_t mode;
        te_errno rc;

        rc = macvlan_mode_str2val(mode_str, &mode);
        if (rc != 0)
            return rc;
        netconf_append_rta(h, &mode, sizeof(mode), IFLA_MACVLAN_MODE);
    }

    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, &req, sizeof(req), NULL, NULL, NULL) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/**
 * Callback of MAC VLAN interfaces dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 * @param cookie        Extra parameters (unused)
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
static netconf_recv_cb_t macvlan_list_cb;
static int
macvlan_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    struct ifinfomsg   *ifla = NLMSG_DATA(h);
    netconf_macvlan     macvlan;
    struct rtattr      *rta_arr[IFLA_MAX + 1];
    struct rtattr      *rta;
    int                 len;

    UNUSED(cookie);

    memset(&macvlan, 0, sizeof(macvlan));

    rta = (struct rtattr *)((char *)h + NLMSG_SPACE(sizeof(*ifla)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(*ifla));

    netconf_parse_rtattr(rta, len, rta_arr, IFLA_MAX);

    if (rta_arr[IFLA_LINKINFO] != NULL)
    {
        struct rtattr *linkinfo[IFLA_INFO_MAX + 1];
        struct rtattr *macvlan_data[IFLA_MACVLAN_MAX + 1];

        netconf_parse_rtattr_nested(rta_arr[IFLA_LINKINFO], linkinfo,
                                    IFLA_INFO_MAX);

        if (linkinfo[IFLA_INFO_KIND] == NULL ||
            strcmp(RTA_DATA(linkinfo[IFLA_INFO_KIND]),
                   NETCONF_LINK_KIND_MACVLAN) != 0)
            return 0;

        if (linkinfo[IFLA_INFO_DATA] == NULL)
            return 0;
        netconf_parse_rtattr_nested(linkinfo[IFLA_INFO_DATA], macvlan_data,
                                    IFLA_MACVLAN_MAX);

        if (macvlan_data[IFLA_MACVLAN_MODE] == NULL)
            return 0;
        macvlan.mode = netconf_get_rta_u32(macvlan_data[IFLA_MACVLAN_MODE]);
    }
    else
        return 0;

    macvlan.ifindex = ifla->ifi_index;
    if (rta_arr[IFLA_LINK] != NULL)
        macvlan.link = netconf_get_rta_u32(rta_arr[IFLA_LINK]);
    if (rta_arr[IFLA_IFNAME] != NULL)
        macvlan.ifname = netconf_dup_rta(rta_arr[IFLA_IFNAME]);

    if (netconf_list_extend(list, NETCONF_NODE_MACVLAN) != 0)
        return -1;

    memcpy(&list->tail->data.macvlan, &macvlan, sizeof(macvlan));

    return 0;
}

void
netconf_macvlan_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.macvlan.ifname);
    free(node);
}

te_errno
netconf_macvlan_list(netconf_handle nh, const char *link, char **list)
{
    netconf_list *nlist;
    netconf_node *node;
    te_string     str = TE_STRING_INIT;
    int           index;

    if (link == NULL || list == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    IFNAME_TO_INDEX(link, index);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 macvlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get MAC VLAN intefaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.macvlan.link == index)
            te_string_append(&str, "%s ", node->data.macvlan.ifname);
    }

    netconf_list_free(nlist);

    *list = str.ptr;

    return 0;
}

te_errno
netconf_macvlan_get_mode(netconf_handle nh, const char *ifname,
                         const char **mode_str)
{
    netconf_list *nlist;
    netconf_node *node;
    te_errno      rc;

    if (ifname == NULL || mode_str == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 macvlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get MAC VLAN intefaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (strcmp(ifname, node->data.macvlan.ifname) == 0)
        {
            rc = macvlan_mode_val2str(node->data.macvlan.mode, mode_str);

            netconf_list_free(nlist);
            return rc;
        }
    }

    netconf_list_free(nlist);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}
