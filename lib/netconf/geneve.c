/** @file
 * @brief GEneric NEtwork Virtualization Encapsulation (Geneve)
 * interfaces management using netconf library
 *
 * Implementation of Geneve interfaces configuration commands.
 *
 * Copyright (c) 2019-2022 OKTET Labs.
 */

#define TE_LGR_USER "Netconf Geneve"

#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "te_alloc.h"

/* Geneve link kind to pass in IFLA_INFO_KIND. */
#define NETCONF_LINK_KIND_GENEVE "geneve"

static te_bool
geneve_link_is_geneve(struct rtattr **linkgen)
{
    struct rtattr *linkinfo[IFLA_INFO_MAX + 1];

    netconf_parse_rtattr_nested(linkgen[IFLA_LINKINFO], linkinfo,
                                IFLA_INFO_MAX);

    return (linkinfo[IFLA_INFO_KIND] != NULL &&
            strcmp(RTA_DATA(linkinfo[IFLA_INFO_KIND]),
                   NETCONF_LINK_KIND_GENEVE) == 0);
}

/* See netconf_internal.h */
int
geneve_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    netconf_geneve      geneve;
    struct rtattr      *linkgen[IFLA_MAX + 1];

    UNUSED(cookie);

    memset(&geneve, 0, sizeof(geneve));

    netconf_parse_link(h, linkgen, IFLA_MAX);

    if (linkgen[IFLA_LINKINFO] == NULL || linkgen[IFLA_IFNAME] == NULL ||
        geneve_link_is_geneve(linkgen) == FALSE)
        return 0;

    geneve.generic.ifname = netconf_dup_rta(linkgen[IFLA_IFNAME]);
    if (geneve.generic.ifname == NULL)
        return -1;

    if (netconf_list_extend(list, NETCONF_NODE_GENEVE) != 0)
    {
        free(geneve.generic.ifname);
        return -1;
    }

    memcpy(&list->tail->data.geneve, &geneve, sizeof(geneve));

    return 0;
}

/* See netconf_internal.h */
void
netconf_geneve_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    netconf_udp_tunnel_free(&(node->data.geneve.generic));

    free(node);
}

/* See netconf.h */
te_errno
netconf_geneve_add(netconf_handle nh, const netconf_geneve *geneve)
{
#if HAVE_DECL_IFLA_GENEVE_ID
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct rtattr      *linkinfo;
    struct rtattr      *data;
    uint16_t            port = htons(geneve->generic.port);

    memset(req, 0, sizeof(req));
    netconf_init_nlmsghdr(req, nh, RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK |
                          NLM_F_CREATE | NLM_F_EXCL, &h);

    netconf_append_rta(h, geneve->generic.ifname,
                       strlen(geneve->generic.ifname) + 1, IFLA_IFNAME);
    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);
    netconf_append_rta(h, NETCONF_LINK_KIND_GENEVE,
                       strlen(NETCONF_LINK_KIND_GENEVE) + 1, IFLA_INFO_KIND);
    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);

    netconf_append_rta(h, &(geneve->generic.vni), sizeof(geneve->generic.vni),
                       IFLA_GENEVE_ID);

    switch (geneve->generic.remote_len)
    {
        case sizeof(struct in_addr):
            netconf_append_rta(h, geneve->generic.remote,
                               geneve->generic.remote_len, IFLA_GENEVE_REMOTE);
            break;

#if HAVE_DECL_IFLA_GENEVE_REMOTE6
        case sizeof(struct in6_addr):
            netconf_append_rta(h, geneve->generic.remote,
                               geneve->generic.remote_len,
                               IFLA_GENEVE_REMOTE6);
            break;
#endif

        case 0:
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    netconf_append_rta(h, &port, sizeof(port), IFLA_GENEVE_PORT);

    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL, NULL) != 0) {
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
#else
    UNUSED(nh);
    UNUSED(geneve);
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
}

/* See netconf.h */
te_errno
netconf_geneve_list(netconf_handle nh,
                    netconf_udp_tunnel_list_filter_func filter_cb,
                    void *filter_opaque, char **list)
{
#if HAVE_DECL_IFLA_GENEVE_ID
    return netconf_udp_tunnel_list(nh, filter_cb, filter_opaque, list,
                                   NETCONF_LINK_KIND_GENEVE);
#else
    UNUSED(nh);
    UNUSED(filter_cb);
    UNUSED(filter_opaque);

    *list = TE_ALLOC(sizeof(char));
    if (*list == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    **list = '\0';

    return 0;
#endif
}


