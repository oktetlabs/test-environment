/** @file
 * @brief General functions of netconf library
 *
 * Copyright (C) 2008 OKTET Labs, St.-Petersburg, Russia
 *
 * @author Maxim Alyutov <Maxim.Alyutov@oktetlabs.ru>
 *
 * $Id: $
 */

#include "netconf.h"
#include "netconf_internal.h"

/**
 * Free memory used by node.
 *
 * @param node          Node to free
 */
static void netconf_node_free(netconf_node *node);

int
netconf_open(netconf_handle *nh)
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
    netlink_socket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
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
                     netconf_recv_cb_t recv_cb)
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

    if (netconf_talk(nh, &req, sizeof(req), recv_cb, list) != 0)
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

    NETCONF_ASSERT(data != NULL);
    NETCONF_ASSERT(h != NULL);
    NETCONF_ASSERT(len > 0);

    rta = (struct rtattr *)((char *)h + NLMSG_ALIGN(h->nlmsg_len));
    rta->rta_type = rta_type;
    rta->rta_len = RTA_LENGTH(len);
    memcpy(RTA_DATA(rta), data, len);
    h->nlmsg_len = NLMSG_ALIGN(h->nlmsg_len) + RTA_LENGTH(len);
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
             netconf_recv_cb_t recv_cb, netconf_list *list)
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
                        if (recv_cb(h, list) != 0)
                            return -1;
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
