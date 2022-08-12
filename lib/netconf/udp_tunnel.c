/** @file
 * @brief Generic UDP Tunnel (VXLAN and Geneve)
 * interfaces management using netconf library
 *
 * Implementation of UDP Tunnel interfaces configuration commands.
 *
 * Copyright (C) 2019-2022 OKTET Labs.
 *
 */

#define TE_LGR_USER "Netconf UDP Tunnel"

#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"

/* See netconf_internal.h */
void
netconf_udp_tunnel_free(netconf_udp_tunnel *udp_tunnel)
{
    NETCONF_ASSERT(udp_tunnel != NULL);

    free(udp_tunnel->ifname);
}

/* See netconf.h */
te_errno
netconf_udp_tunnel_del(netconf_handle nh, const char *ifname)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;

    memset(req, 0, sizeof(req));
    netconf_init_nlmsghdr(req, nh, RTM_DELLINK, NLM_F_REQUEST | NLM_F_ACK,
                          &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See netconf.h */
te_errno
netconf_udp_tunnel_list(netconf_handle nh,
                        netconf_udp_tunnel_list_filter_func filter_cb,
                        void *filter_opaque, char **list, char *link_kind)
{
    netconf_list   *nlist;
    netconf_node   *node;
    te_string       str = TE_STRING_INIT;
    char           *ifname;
    te_errno        rc = 0;

    if (strcmp(link_kind, "geneve") == 0)
        nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                     geneve_list_cb, NULL);
    else if (strcmp(link_kind, "vxlan") == 0)
        nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                     vxlan_list_cb, NULL);
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (nlist == NULL)
    {
        ERROR("Failed to get %s interfaces list", link_kind);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (strcmp(link_kind, "geneve") == 0)
            ifname = node->data.geneve.generic.ifname;
        else if (strcmp(link_kind, "vxlan") == 0)
            ifname = node->data.vxlan.generic.ifname;
        else
            ifname = NULL;

        if (ifname != NULL)
        {
            if (filter_cb != NULL &&
                filter_cb(ifname, filter_opaque) == FALSE)
                continue;

            rc = te_string_append(&str, "%s ", ifname);
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
