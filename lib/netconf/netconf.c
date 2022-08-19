/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief General functions of netconf library
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "netconf.h"
#include "netconf_internal.h"
#include "logger_api.h"
#include "te_alloc.h"

/**
 * Free memory used by node.
 *
 * @param node          Node to free
 */
static void netconf_node_free(netconf_node *node);

int
netconf_open(netconf_handle *nh, int netlink_family)
{
    int                 netlink_socket;
    struct sockaddr_nl  local_addr;
    int                 sndbuf = NETCONF_SOCK_SNDBUF;
    int                 rcvbuf = NETCONF_SOCK_RCVBUF;
    unsigned int        len;

    if (nh == NULL)
    {
        NETCONF_ASSERT(0);
        errno = EINVAL;
        return -1;
    }

    /* Create netlink socket */
    netlink_socket = socket(AF_NETLINK, SOCK_RAW, netlink_family);
    if (netlink_socket < 0)
    {
        return -1;
    }

    if (setsockopt(netlink_socket, SOL_SOCKET, SO_SNDBUF,
                   &sndbuf, sizeof(sndbuf)) < 0)
    {
        int err = errno;

        close(netlink_socket);
        errno = err;
        return -1;
    }

    if (setsockopt(netlink_socket, SOL_SOCKET, SO_RCVBUF,
                   &rcvbuf, sizeof(rcvbuf)) < 0)
    {
        int err = errno;

        close(netlink_socket);
        errno = err;
        return -1;
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.nl_family = AF_NETLINK;
    if (bind(netlink_socket, (struct sockaddr *)&local_addr,
             sizeof(local_addr)) < 0)
    {
        int err = errno;

        close(netlink_socket);
        errno = err;
        return -1;
    }

    /* Get nl_pid of socket to filter with it when receiving messages */
    len = sizeof(local_addr);
    if (getsockname(netlink_socket,
                    (struct sockaddr *)&local_addr, &len) < 0)
    {
        int err = errno;

        close(netlink_socket);
        errno = err;
        return -1;
    }

    NETCONF_ASSERT(len == sizeof(local_addr));
    NETCONF_ASSERT(local_addr.nl_family == AF_NETLINK);

    *nh = malloc(sizeof(**nh));
    if (*nh == NULL)
    {
        int err = errno;

        close(netlink_socket);
        errno = err;
        return -1;
    }

    memset(*nh, 0, sizeof(**nh));
    (*nh)->socket = netlink_socket;
    (*nh)->local_addr = local_addr;

    /* Some random number */
    (*nh)->seq = time(NULL);

    return 0;
}

void
netconf_close(netconf_handle nh)
{
    if ((nh != NULL) && (nh->socket >= 0))
    {
        close(nh->socket);
        nh->socket = -1;
        free(nh);
    }
}

uint16_t
netconf_cmd_to_flags(netconf_cmd cmd)
{
    uint16_t f = NLM_F_REQUEST | NLM_F_ACK;

    switch (cmd)
    {
        case NETCONF_CMD_ADD:
            f |= NLM_F_CREATE | NLM_F_EXCL;
            break;

        case NETCONF_CMD_DEL:
            break;

        case NETCONF_CMD_CHANGE:
            f |= NLM_F_REPLACE;
            break;

        case NETCONF_CMD_REPLACE:
            f |= NLM_F_CREATE | NLM_F_REPLACE;
            break;

        default:
            /* Unrecognized command */
            f = 0;
            break;
    }

    return f;
}

netconf_list *
netconf_dump_request(netconf_handle nh, uint16_t type,
                     unsigned char family,
                     netconf_recv_cb_t *recv_cb,
                     void *cookie)
{
    char                req[NLMSG_SPACE(sizeof(struct rtgenmsg))];
    struct nlmsghdr    *h;
    struct rtgenmsg    *g;
    netconf_list       *list;

    if (nh == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    NETCONF_ASSERT(recv_cb != NULL);

    /* Generate request */
    memset(&req, 0, sizeof(req));

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    h->nlmsg_type = type;
    h->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    h->nlmsg_seq = ++nh->seq;

    g = NLMSG_DATA(h);
    g->rtgen_family = family;

    /* Prepare list */
    list = malloc(sizeof(*list));
    if (list == NULL)
        return NULL;

    memset(list, 0, sizeof(netconf_list));

    if (netconf_talk(nh, &req, sizeof(req), recv_cb, cookie, list) != 0)
    {
        int err = errno;

        netconf_list_free(list);
        errno = err;
        return NULL;
    }
    return list;
}

int
netconf_list_extend(netconf_list *list, netconf_node_type type)
{
    netconf_node **node;

    node = (list->head == NULL) ? &(list->head) : &(list->tail->next);

    *node = malloc(sizeof(**node));
    if (*node == NULL)
        return -1;

    memset(*node, 0, sizeof(netconf_node));
    (*node)->type = type;
    (*node)->prev = list->tail;

    list->tail = (list->tail == NULL) ? (list->head) : (list->tail->next);
    list->length++;
    return 0;
}

void
netconf_append_rta(struct nlmsghdr *h, const void *data, int len,
                   unsigned short rta_type)
{
    struct rtattr *rta;
    int rta_len = RTA_LENGTH(len);

    NETCONF_ASSERT(h != NULL);

    rta = NETCONF_NLMSG_TAIL(h);
    rta->rta_type = rta_type;
    rta->rta_len = rta_len;
    memcpy(RTA_DATA(rta), data, len);
    h->nlmsg_len = NLMSG_ALIGN(h->nlmsg_len) + RTA_ALIGN(rta_len);
}

void *
netconf_dup_rta(const struct rtattr *rta)
{
    void *data;

    data = malloc(RTA_PAYLOAD(rta));
    if (data == NULL)
        return NULL;

    memcpy(data, RTA_DATA(rta), RTA_PAYLOAD(rta));

    return data;
}

void
netconf_list_filter(netconf_list *list,
                    netconf_node_filter_t filter,
                    void *user_data)
{
    netconf_node *t;
    netconf_node *next = NULL;
    netconf_node *prev = NULL;

    if ((list == NULL) || (filter == NULL))
        return;

    t = list->head;
    while (t != NULL)
    {
        next = t->next;
        prev = t->prev;

        if (!filter(t, user_data))
        {
            /* Remove this node */
            if (prev == NULL)
                list->head = next;
            else
                prev->next = next;

            if (next == NULL)
                list->tail = prev;
            else
                next->prev = prev;

            list->length--;
            netconf_node_free(t);
        }

        t = next;
    }
}

static void
netconf_node_free(netconf_node *node)
{
    switch (node->type)
    {
        case NETCONF_NODE_LINK:
            netconf_link_node_free(node);
            break;

        case NETCONF_NODE_NET_ADDR:
            netconf_net_addr_node_free(node);
            break;

        case NETCONF_NODE_ROUTE:
            netconf_route_node_free(node);
            break;

        case NETCONF_NODE_NEIGH:
            netconf_neigh_node_free(node);
            break;

        case NETCONF_NODE_RULE:
            netconf_rule_node_free(node);
            break;

        case NETCONF_NODE_MACVLAN:
            netconf_macvlan_node_free(node);
            break;

        case NETCONF_NODE_IPVLAN:
            netconf_ipvlan_node_free(node);
            break;

        case NETCONF_NODE_VLAN:
            netconf_vlan_node_free(node);
            break;

        case NETCONF_NODE_VETH:
            netconf_veth_node_free(node);
            break;

        case NETCONF_NODE_GENEVE:
            netconf_geneve_node_free(node);
            break;

        case NETCONF_NODE_VXLAN:
            netconf_vxlan_node_free(node);
            break;

        case NETCONF_NODE_BRIDGE:
            netconf_bridge_node_free(node);
            break;

        case NETCONF_NODE_BRIDGE_PORT:
            netconf_port_node_free(node);
            break;

        case NETCONF_NODE_DEVLINK_INFO:
            netconf_devlink_info_node_free(node);
            break;

        case NETCONF_NODE_DEVLINK_PARAM:
            netconf_devlink_param_node_free(node);
            break;

        default:
            NETCONF_ASSERT(0);
            free(node);
    }
}

void
netconf_list_free(netconf_list *list)
{
    netconf_node       *node;
    netconf_node       *next;

    if (list == NULL)
        return;

    node = list->head;
    while (node != NULL)
    {
        next = node->next;
        netconf_node_free(node);
        node = next;
    }

    free(list);
}

int
netconf_talk(netconf_handle nh, void *req, int len,
             netconf_recv_cb_t *recv_cb, void *cookie, netconf_list *list)
{
    struct msghdr       msg;
    char                buf[NETCONF_RCV_BUF_LEN];
    struct sockaddr_nl  nladdr;
    struct iovec        iov;
    int                 sent;

    NETCONF_ASSERT(req != NULL);
    NETCONF_ASSERT(len > 0);

    if ((nh == NULL) || (nh->socket < 0))
    {
        errno = EINVAL;
        return -1;
    }

    /* Send message to netlink socket */
    memset(&nladdr, 0, sizeof(nladdr));
    memset(&iov, 0, sizeof(iov));
    memset(&msg, 0, sizeof(msg));
    nladdr.nl_family = AF_NETLINK;
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    iov.iov_base = req;
    iov.iov_len = len;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sent = sendmsg(nh->socket, &msg, 0);
    if (sent < 0)
    {
        return -1;
    }

    memset(&nladdr, 0, sizeof(nladdr));
    memset(&iov, 0, sizeof(iov));
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    while (1)
    {
        struct nlmsghdr        *h;
        int                     rcvd;

        /* Receive response from netlink socket */
        memset(&buf, 0, sizeof(buf));
        rcvd = recvmsg(nh->socket, &msg, 0);
        if (rcvd < 0)
        {
            /* TODO: is it necessary thing? */
            if (errno == EINTR)
                continue;
            return -1;
        }

        for (h = (struct nlmsghdr *)buf;
             NLMSG_OK(h, (unsigned int)rcvd);
             h = NLMSG_NEXT(h, rcvd))
        {
            if ((nladdr.nl_pid != 0) ||
                (h->nlmsg_pid != nh->local_addr.nl_pid) ||
                (h->nlmsg_seq != nh->seq))
            {
                /*
                 * We received data intended to some other socket.
                 * This is strange.
                 */
                NETCONF_ASSERT(0);
                continue;
            }

            switch (h->nlmsg_type)
            {
                case NLMSG_DONE:
                    return 0;

                case NLMSG_NOOP:
                    /*
                     * In fact we can receive NOOP only while monitoring
                     * events, so this case indicates strange things.
                     */
                    NETCONF_ASSERT(0);
                    break;

                case NLMSG_ERROR:
                {
                    struct nlmsgerr *errmsg = NLMSG_DATA(h);

                    if (errmsg->error != 0)
                    {
                        errno = -errmsg->error;
                        return -1;
                    }
                    else
                    {
                        /* This is ACK, all is ok */
                        return 0;
                    }
                }

                default:
                    if (recv_cb != NULL)
                    {
                        if (recv_cb(h, list, cookie) != 0)
                            return -1;

                        if (!(h->nlmsg_flags & NLM_F_MULTI))
                            return 0;
                    }
                    else
                    {
                        NETCONF_ASSERT(0);
                    }
            }
        }
    }

    /* Never reached */
    return 0;
}

void
netconf_append_rta_nested(struct nlmsghdr *h, unsigned short rta_type,
                          struct rtattr **rta)
{
    *rta = NETCONF_NLMSG_TAIL(h);
    netconf_append_rta(h, NULL, 0, rta_type);
}

void
netconf_append_rta_nested_end(struct nlmsghdr *h, struct rtattr *rta)
{
    rta->rta_len = (void *)NETCONF_NLMSG_TAIL(h) - (void *)rta;
}

void
netconf_parse_rtattr(struct rtattr *rta, int len, struct rtattr **rta_arr,
                     int max)
{
    memset(rta_arr, 0, sizeof(*rta_arr) * (max + 1));

    while (RTA_OK(rta, len))
    {
        if (rta->rta_type <= max)
            rta_arr[rta->rta_type] = rta;

        rta = RTA_NEXT(rta, len);
    }

    NETCONF_ASSERT(len == 0);
}

void
netconf_parse_rtattr_nested(struct rtattr *rta, struct rtattr **rta_arr,
                            int max)
{
    netconf_parse_rtattr(RTA_DATA(rta), RTA_PAYLOAD(rta), rta_arr, max);
}

uint32_t
netconf_get_rta_u32(struct rtattr *rta)
{
    return *(uint32_t *)RTA_DATA(rta);
}

void
netconf_init_nlmsghdr(char *req, netconf_handle nh, uint16_t nlmsg_type,
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

void
netconf_parse_link(struct nlmsghdr *h, struct rtattr **rta_arr, int max)
{
    struct rtattr      *rta_link;
    int                 len;

    rta_link = (struct rtattr *)((char *)h +
                                 NLMSG_SPACE(sizeof(struct ifinfomsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ifinfomsg));
    netconf_parse_rtattr(rta_link, len, rta_arr, max);
}

/* See description in netconf_internal.h */
te_errno
netconf_append_attr(char *req, size_t max_len,
                    uint16_t attr_type, const void *data, size_t len)
{
    struct nlmsghdr *h;
    struct nlattr *na;
    size_t attr_len;
    size_t new_len;

    h = (struct nlmsghdr *)req;
    na = (struct nlattr *)((void *)h + h->nlmsg_len);

    attr_len = NLA_HDRLEN + len;
    new_len = NLMSG_LENGTH((void *)na - NLMSG_DATA(h) +
                           NLA_ALIGN(attr_len));
    if (max_len < new_len)
    {
        ERROR("%s(): not enough space for a new attribute", __FUNCTION__);
        return TE_ENOBUFS;
    }

    memset(na, 0, sizeof(*na));
    na->nla_len = attr_len;
    na->nla_type = attr_type;

    if (len > 0)
        memcpy((void *)na + NLA_HDRLEN, data, len);

    h->nlmsg_len = new_len;
    return 0;
}

/* See description in netconf_internal.h */
te_errno
netconf_get_str_attr(struct nlattr *na, char **value)
{
    int len;

    if (na->nla_len < NLA_HDRLEN)
    {
        ERROR("%s(): attribute of type %d has incorrect length",
              __FUNCTION__, (int)(na->nla_type));
        return TE_EINVAL;
    }
    else if (na->nla_len == NLA_HDRLEN)
    {
        *value = strdup("");
        if (*value == NULL)
        {
            ERROR("%s(): not enough memory", __FUNCTION__);
            return TE_ENOMEM;
        }

        return 0;
    }

    len = na->nla_len - NLA_HDRLEN;
    /*
     * Include '\0' at the end for the case when attribute is not
     * null-terminated
     */
    len++;

    *value = TE_ALLOC(len);
    if (*value == NULL)
        return TE_ENOMEM;

    memcpy(*value, (void *)na + NLA_HDRLEN, len - 1);
    return 0;
}

/* Get value of an attribute of integer type */
#define GET_INT_ATTR(_res, _na, _type) \
    do {                                                                \
        if (_na == NULL)                                                \
        {                                                               \
            ERROR("%s(): attribute pointer cannot be NULL",             \
                  __FUNCTION__);                                        \
            return TE_EINVAL;                                           \
        }                                                               \
        if (_na->nla_len < NLA_HDRLEN + sizeof(_type))                  \
        {                                                               \
            ERROR("%s(): attribute of type %d has incorrect length",    \
                  __FUNCTION__, (int)(_na->nla_type));                  \
            return TE_EINVAL;                                           \
        }                                                               \
                                                                        \
        *_res = *(_type *)((void *)_na + NLA_HDRLEN);                   \
    } while (0)

/* See description in netconf_internal.h */
te_errno
netconf_get_uint8_attr(struct nlattr *na, uint8_t *value)
{
    GET_INT_ATTR(value, na, uint8_t);
    return 0;
}

/* See description in netconf_internal.h */
te_errno
netconf_get_uint16_attr(struct nlattr *na, uint16_t *value)
{
    GET_INT_ATTR(value, na, uint16_t);
    return 0;
}

/* See description in netconf_internal.h */
te_errno
netconf_get_uint32_attr(struct nlattr *na, uint32_t *value)
{
    GET_INT_ATTR(value, na, uint32_t);
    return 0;
}

/* See description in netconf_internal.h */
te_errno
netconf_get_uint64_attr(struct nlattr *na, uint64_t *value)
{
    GET_INT_ATTR(value, na, uint64_t);
    return 0;
}

/* See description in netconf_internal.h */
const char *
netconf_nla_type2str(netconf_nla_type nla_type)
{
    switch (nla_type)
    {
        case NETCONF_NLA_U8:
            return "u8";

        case NETCONF_NLA_U16:
            return "u16";

        case NETCONF_NLA_U32:
            return "u32";

        case NETCONF_NLA_U64:
            return "u64";

        case NETCONF_NLA_STRING:
            return "string";

        case NETCONF_NLA_FLAG:
            return "flag";

        default:
            return "<UNKNOWN>";
    }
}

/* See description in netconf_internal.h */
te_errno
netconf_process_attrs(void *first_attr,
                      size_t attrs_size,
                      netconf_attr_cb cb,
                      void *cb_data)
{
    struct nlattr *na;
    size_t aligned_len;
    te_errno rc;

    if (first_attr == NULL)
    {
        ERROR("%s(): first_attr cannot be NULL", __FUNCTION__);
        return TE_EINVAL;
    }

    na = first_attr;
    while (attrs_size > 0)
    {
        if (sizeof(*na) > attrs_size)
            break;
        if (na->nla_len > attrs_size)
        {
            ERROR("%s(): the last attribute has too big length",
                  __FUNCTION__);
            return TE_EINVAL;
        }

        rc = cb(na, cb_data);
        if (rc != 0)
            return rc;

        aligned_len = NLA_ALIGN(na->nla_len);
        na = (void *)na + aligned_len;
        if (attrs_size >= aligned_len)
            attrs_size -= aligned_len;
        else
            break;
    }

    return 0;
}

/* See description in netconf_internal.h */
te_errno
netconf_process_hdr_attrs(struct nlmsghdr *h, size_t hdr_len,
                          netconf_attr_cb cb, void *cb_data)
{
    void *first_attr;
    size_t attrs_size;

    first_attr = NLMSG_DATA(h) + hdr_len;
    attrs_size = (void *)h + h->nlmsg_len - first_attr;

    return netconf_process_attrs(first_attr, attrs_size,
                                 cb, cb_data);
}

/* See description in netconf_internal.h */
te_errno
netconf_process_nested_attrs(struct nlattr *na_parent, netconf_attr_cb cb,
                             void *cb_data)
{
    return netconf_process_attrs((void *)na_parent + NLA_HDRLEN,
                                 na_parent->nla_len - NLA_HDRLEN,
                                 cb, cb_data);
}
