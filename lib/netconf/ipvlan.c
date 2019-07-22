/** @file
 * @brief IP VLAN interfaces management using netconf library
 *
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Damir Mansurov <dnman@oktetlabs.ru>
 */

#define TE_LGR_USER "Netconf IP VLAN"

#include "te_config.h"
#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"
#include <linux/if_link.h>

#define NETCONF_LINK_KIND_IPVLAN "ipvlan"

/* Linux > 3.18 */
#ifdef IFLA_IPVLAN_MAX

/* See description in netconf.h */
te_errno
netconf_ipvlan_modify(netconf_handle nh, netconf_cmd cmd,
                      const char *link, const char *ifname,
                      uint32_t mode, uint32_t flag)
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
    h->nlmsg_type = (cmd == NETCONF_CMD_DEL) ? RTM_DELLINK : RTM_NEWLINK;

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
    netconf_append_rta(h, NETCONF_LINK_KIND_IPVLAN,
                       strlen(NETCONF_LINK_KIND_IPVLAN) + 1,
                       IFLA_INFO_KIND);

    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);

    netconf_append_rta(h, &mode, sizeof(mode), IFLA_IPVLAN_MODE);

#ifdef IPVLAN_F_PRIVATE
    netconf_append_rta(h, &flag, sizeof(flag), IFLA_IPVLAN_FLAGS);
#else
    /* flag: default value is 0 */
    if (flag != 0)
        WARN("The argument flag was ignored: IPVLAN_F_* is not supported");
#endif
    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, &req, sizeof(req), NULL, NULL, NULL) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/**
 * Callback of IP VLAN interfaces dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 * @param cookie        Extra parameters (unused)
 *
 * @return @c 0 on success, @c -1 on error (check errno for details).
 */
static int
ipvlan_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    struct ifinfomsg   *ifla = NLMSG_DATA(h);
    netconf_ipvlan      ipvlan;
    struct rtattr      *rta_arr[IFLA_MAX + 1];
    struct rtattr      *rta;
    struct rtattr      *linkinfo[IFLA_INFO_MAX + 1];
    struct rtattr      *ipvlan_data[IFLA_IPVLAN_MAX + 1];
    int                 len;

    memset(&ipvlan, 0, sizeof(ipvlan));

    rta = (struct rtattr *)((char *)h + NLMSG_SPACE(sizeof(*ifla)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(*ifla));

    netconf_parse_rtattr(rta, len, rta_arr, IFLA_MAX);

    if (rta_arr[IFLA_LINKINFO] == NULL)
        return 0;

    netconf_parse_rtattr_nested(rta_arr[IFLA_LINKINFO], linkinfo,
                                IFLA_INFO_MAX);

    if (linkinfo[IFLA_INFO_KIND] == NULL ||
        strcmp(RTA_DATA(linkinfo[IFLA_INFO_KIND]),
               NETCONF_LINK_KIND_IPVLAN) != 0)
    {
        return 0;
    }

    if (linkinfo[IFLA_INFO_DATA] == NULL)
        return 0;
    netconf_parse_rtattr_nested(linkinfo[IFLA_INFO_DATA], ipvlan_data,
                                IFLA_IPVLAN_MAX);

    if (ipvlan_data[IFLA_IPVLAN_MODE] == NULL)
        return 0;
    ipvlan.mode = netconf_get_rta_u32(ipvlan_data[IFLA_IPVLAN_MODE]);

#ifdef IPVLAN_F_PRIVATE
    if (ipvlan_data[IFLA_IPVLAN_FLAGS] == NULL)
        return 0;
    ipvlan.flag = netconf_get_rta_u32(ipvlan_data[IFLA_IPVLAN_FLAGS]);
#else
    ipvlan.flag = 0; /* bridge (default) */
#endif

    if (netconf_list_extend(list, NETCONF_NODE_IPVLAN) != 0)
        return -1;

    ipvlan.ifindex = ifla->ifi_index;
    if (rta_arr[IFLA_LINK] != NULL)
        ipvlan.link = netconf_get_rta_u32(rta_arr[IFLA_LINK]);
    if (rta_arr[IFLA_IFNAME] != NULL)
        ipvlan.ifname = netconf_dup_rta(rta_arr[IFLA_IFNAME]);

    memcpy(&list->tail->data.ipvlan, &ipvlan, sizeof(ipvlan));

    return 0;
}

/* See description in netconf.h */
te_errno
netconf_ipvlan_list(netconf_handle nh, const char *link, char **list)
{
    netconf_list *nlist;
    netconf_node *node;
    te_string     str = TE_STRING_INIT;
    int           index;
    te_errno      rc = 0;

    if (link == NULL || list == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    IFNAME_TO_INDEX(link, index);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 ipvlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get IP VLAN intefaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.ipvlan.link == index)
        {
            rc = te_string_append(&str, "%s ", node->data.ipvlan.ifname);
            if (rc != 0)
                break;
        }
    }

    if (rc == 0)
        *list = str.ptr;
    else
        te_string_free(&str);

    netconf_list_free(nlist);
    return rc;
}

/* See description in netconf.h */
te_errno
netconf_ipvlan_get_mode(netconf_handle nh, const char *ifname,
                        uint32_t *mode, uint32_t *flag)
{
    netconf_list *nlist;
    netconf_node *node;

    if (ifname == NULL || mode == NULL || flag == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 ipvlan_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get IP VLAN intefaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (strcmp(ifname, node->data.ipvlan.ifname) == 0)
        {
            *mode = node->data.ipvlan.mode;
            *flag = node->data.ipvlan.flag;

            netconf_list_free(nlist);
            return 0;
        }
    }

    netconf_list_free(nlist);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

#else /* IFLA_IPVLAN_MAX */

#define IPVLAN_NOT_SUPPORTED                                    \
    do {                                                        \
        ERROR("%s: IP VLAN is not supported", __FUNCTION__);    \
        return TE_RC(TE_TA_UNIX, TE_ENOSYS);                    \
    } while (0)

/* See description in netconf.h */
te_errno
netconf_ipvlan_modify(netconf_handle nh, netconf_cmd cmd,
                      const char *link, const char *ifname,
                      uint32_t mode, uint32_t flag)
{
    UNUSED(nh);
    UNUSED(cmd);
    UNUSED(link);
    UNUSED(ifname);
    UNUSED(mode);
    UNUSED(flag);

    IPVLAN_NOT_SUPPORTED;
}

/* See description in netconf.h */
te_errno
netconf_ipvlan_list(netconf_handle nh, const char *link, char **list)
{
    UNUSED(nh);
    UNUSED(link);
    UNUSED(list);

    IPVLAN_NOT_SUPPORTED;
}

/* See description in netconf.h */
te_errno
netconf_ipvlan_get_mode(netconf_handle nh, const char *ifname,
                        uint32_t *mode, uint32_t *flag)
{
    UNUSED(nh);
    UNUSED(ifname);
    UNUSED(mode);
    UNUSED(flag);

    IPVLAN_NOT_SUPPORTED;
}
#endif /* !IFLA_IPVLAN_MAX */

/* See description in netconf.h */
void
netconf_ipvlan_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.ipvlan.ifname);
    free(node);
}

