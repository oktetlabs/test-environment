/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Neighbour tables management in netconf library
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "netconf.h"
#include "netconf_internal.h"

/**
 * Convert TE neighbor entry flags to native neighbor entry flags.
 *
 * @param flags     Flags to convert.
 *
 * @return Converted flags.
 */
static unsigned int
neigh_flags_te2h(unsigned int flags)
{
    unsigned int result = 0;

#ifdef NTF_USE
    if (flags & NETCONF_NTF_USE)
        result = result | NTF_USE;
#endif
#ifdef NTF_SELF
    if (flags & NETCONF_NTF_SELF)
        result = result | NTF_SELF;
#endif
#ifdef NTF_MASTER
    if (flags & NETCONF_NTF_MASTER)
        result = result | NTF_MASTER;
#endif
#ifdef NTF_PROXY
    if (flags & NETCONF_NTF_PROXY)
        result = result | NTF_PROXY;
#endif
#ifdef NTF_EXT_LEARNED
    if (flags & NETCONF_NTF_EXT_LEARNED)
        result = result | NTF_EXT_LEARNED;
#endif
#ifdef NTF_ROUTER
    if (flags & NETCONF_NTF_ROUTER)
        result = result | NTF_ROUTER;
#endif

    return result;
}

/**
 * Convert native neighbor entry flags to TE neighbor entry flags.
 *
 * @param flags     Flags to convert.
 *
 * @return Converted flags.
 */
static unsigned int
neigh_flags_h2te(unsigned int flags)
{
    unsigned int result = 0;

#ifdef NTF_USE
    if (flags & NTF_USE)
        result = result | NETCONF_NTF_USE;
#endif
#ifdef NTF_SELF
    if (flags & NTF_SELF)
        result = result | NETCONF_NTF_SELF;
#endif
#ifdef NTF_MASTER
    if (flags & NTF_MASTER)
        result = result | NETCONF_NTF_MASTER;
#endif
#ifdef NTF_PROXY
    if (flags & NTF_PROXY)
        result = result | NETCONF_NTF_PROXY;
#endif
#ifdef NTF_EXT_LEARNED
    if (flags & NTF_EXT_LEARNED)
        result = result | NETCONF_NTF_EXT_LEARNED;
#endif
#ifdef NTF_ROUTER
    if (flags & NTF_ROUTER)
        result = result | NETCONF_NTF_ROUTER;
#endif

    return result;
}

/**
 * Callback of neighbours dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 * @param cookie        Extra parameters (unused)
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
static netconf_recv_cb_t neigh_list_cb;
static int
neigh_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    struct ndmsg       *ndm = NLMSG_DATA(h);
    struct rtattr      *rta;
    int                 len;
    netconf_neigh      *neigh;

    UNUSED(cookie);

    if (netconf_list_extend(list, NETCONF_NODE_NEIGH) != 0)
        return -1;

    neigh = &(list->tail->data.neigh);

    neigh->family = ndm->ndm_family;
    neigh->ifindex = ndm->ndm_ifindex;
    neigh->state = ndm->ndm_state;
    neigh->flags = neigh_flags_h2te(ndm->ndm_flags);

    rta = (struct rtattr *)((char *)h +
                            NLMSG_SPACE(sizeof(struct ndmsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ndmsg));

    while (RTA_OK(rta, len))
    {
        switch (rta->rta_type)
        {
            case NDA_DST:
                neigh->dst = netconf_dup_rta(rta);
                break;

            case NDA_LLADDR:
                neigh->lladdr = netconf_dup_rta(rta);
                neigh->addrlen = RTA_PAYLOAD(rta);
                break;
        }

        rta = RTA_NEXT(rta, len);
    }

    return 0;
}

netconf_list *
netconf_neigh_dump(netconf_handle nh, unsigned char family)
{
    return netconf_dump_request(nh, RTM_GETNEIGH, family, neigh_list_cb, NULL);
}

void
netconf_neigh_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    if (node->data.neigh.dst != NULL)
        free(node->data.neigh.dst);
    if (node->data.neigh.lladdr != NULL)
        free(node->data.neigh.lladdr);

    free(node);
}

void
netconf_neigh_init(netconf_neigh *neigh)
{
    memset(neigh, 0 , sizeof(netconf_neigh));
    neigh->family = AF_INET;
    neigh->state = NETCONF_NUD_UNSPEC;
}

int
netconf_neigh_modify(netconf_handle nh, netconf_cmd cmd,
                     const netconf_neigh *neigh)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct ndmsg       *ndm;
    unsigned int        addrlen;
    unsigned int        hwaddrlen;

    /* Check required fields */
    if ((neigh == NULL) ||
        ((neigh->family != AF_INET) && (neigh->family != AF_INET6)) ||
        (neigh->dst == NULL) ||
        (neigh->ifindex == 0))
    {
        errno = EINVAL;
        return -1;
    }

    /* Generate request */
    memset(&req, 0, sizeof(req));

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
    h->nlmsg_type = (cmd == NETCONF_CMD_DEL) ?
                    RTM_DELNEIGH : RTM_NEWNEIGH;
    h->nlmsg_flags = netconf_cmd_to_flags(cmd);
    if (h->nlmsg_flags == 0)
    {
        errno = EINVAL;
        return -1;
    }
    h->nlmsg_seq = ++nh->seq;

    /* Set parameters or some defaults if unspecified */

    ndm = NLMSG_DATA(h);
    ndm->ndm_family = neigh->family;
    ndm->ndm_ifindex = neigh->ifindex;

    ndm->ndm_state = (neigh->state == NETCONF_NUD_UNSPEC) ?
                     NUD_PERMANENT : neigh->state;

    ndm->ndm_flags = neigh_flags_te2h(neigh->flags);

    NETCONF_ASSERT((neigh->family == AF_INET) ||
                   (neigh->family == AF_INET6));
    addrlen = (neigh->family == AF_INET) ?
              sizeof(struct in_addr) : sizeof(struct in6_addr);

    /* This is ethernet address by default */
    hwaddrlen = (neigh->addrlen == 0) ?
                ETHER_ADDR_LEN : neigh->addrlen;

    NETCONF_ASSERT(neigh->dst != NULL);
    netconf_append_rta(h, neigh->dst, addrlen, NDA_DST);

    if (neigh->lladdr != NULL)
        netconf_append_rta(h, neigh->lladdr, hwaddrlen, NDA_LLADDR);

    /* Send request and receive ACK */
    return netconf_talk(nh, &req, sizeof(req), NULL, NULL, NULL);
}
