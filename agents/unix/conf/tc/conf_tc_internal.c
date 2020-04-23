/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support Traffic Control
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf"

#include "te_queue.h"
#include "te_config.h"
#include "te_alloc.h"
#include "logger_api.h"

#include "conf_tc_internal.h"
#include "conf_net_if_wrapper.h"
#include "conf_qdisc_params.h"

#include <netlink/errno.h>
#include <netlink/route/link.h>
#include <netlink/route/qdisc.h>

static struct nl_sock *netlink_socket = NULL;

static struct nl_cache *qdisc_cache = NULL;

static uint32_t last_id = 0;

typedef struct qdisc_context_node {
    SLIST_ENTRY(qdisc_context_node) links;
    struct rtnl_qdisc         *qdisc;
    uint32_t id;
} qdisc_context_node;

static SLIST_HEAD(qdisc_contexts, qdisc_context_node) qdiscs;

static te_errno
netlink_socket_init(void)
{
    int rc;
    netlink_socket = nl_socket_alloc();

    if (netlink_socket == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    rc = nl_connect(netlink_socket, NETLINK_ROUTE);
    return conf_tc_internal_nl_error2te_errno(rc);
}

static void
netlink_socket_fini(void)
{
    nl_close(netlink_socket);
    nl_socket_free(netlink_socket);
}

static te_errno
netlink_qdisc_cache_init(void)
{
    int rc;
    struct nl_sock *sock = conf_tc_internal_get_sock();

    rc = rtnl_qdisc_alloc_cache(sock, &qdisc_cache);
    return conf_tc_internal_nl_error2te_errno(rc);
}

static void
netlink_qdisc_cache_fini(void)
{
    nl_cache_free(qdisc_cache);
}

/* See the description in conf_tc_internal.h */
te_errno
conf_tc_internal_init(void)
{
    te_errno rc;

    SLIST_INIT(&qdiscs);

    if ((rc = netlink_socket_init()) != 0)
        return rc;

    if ((rc = netlink_qdisc_cache_init()) != 0)
        return rc;

    return 0;
}

/* See the description in conf_tc_internal.h */
void
conf_tc_internal_fini(void)
{
    qdisc_context_node *qdisc_cxt, *tqdisc_cxt;

    netlink_socket_fini();
    netlink_qdisc_cache_fini();

    SLIST_FOREACH_SAFE(qdisc_cxt, &qdiscs, links, tqdisc_cxt) {
        rtnl_qdisc_put(qdisc_cxt->qdisc);
        free(qdisc_cxt);
    }

    conf_qdisc_tbf_params_free();
    conf_qdisc_clsact_params_free();
}

/* See the description in conf_tc_internal.h */
struct nl_sock *
conf_tc_internal_get_sock(void)
{
    return netlink_socket;
}

/* See the description in conf_tc_internal.h */
struct rtnl_qdisc *
conf_tc_internal_try_get_qdisc(const char *if_name)
{
    struct rtnl_qdisc *qdisc = NULL;
    int if_index = conf_net_if_wrapper_if_nametoindex(if_name);

    qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, if_index, TC_H_ROOT);

    if (qdisc != NULL && rtnl_tc_get_handle(TC_CAST(qdisc)) == 0) {
        rtnl_qdisc_put(qdisc);
        qdisc = NULL;
    }

    return qdisc;
}

/* See the description in conf_tc_internal.h */
static struct rtnl_qdisc *
new_qdisc(const char *if_name)
{
    struct rtnl_qdisc *qdisc = NULL;
    struct nl_object *obj = NULL;

    qdisc = conf_tc_internal_try_get_qdisc(if_name);
    if (qdisc != NULL) {
        obj = nl_object_clone((struct nl_object*) qdisc);
        rtnl_qdisc_put(qdisc);
        return (struct rtnl_qdisc *) obj;
    }

    if ((qdisc = rtnl_qdisc_alloc()) == NULL)
        return NULL;

    rtnl_tc_set_ifindex(TC_CAST(qdisc),
                        conf_net_if_wrapper_if_nametoindex(if_name));
    return qdisc;
}

/* See the description in conf_tc_internal.h */
static struct rtnl_qdisc *
add_qdisc(const char *if_name)
{
    struct rtnl_qdisc *qdisc = NULL;
    qdisc_context_node *qdisc_cxt;

    qdisc = new_qdisc(if_name);
    if (qdisc == NULL)
        return NULL;

    qdisc_cxt = TE_ALLOC(sizeof(*qdisc_cxt));
    if (qdisc_cxt == NULL)
        return NULL;

    qdisc_cxt->qdisc = qdisc;
    qdisc_cxt->id = ++last_id;
    SLIST_INSERT_HEAD(&qdiscs, qdisc_cxt, links);

    return qdisc;
}

/* See the description in conf_tc_internal.h */
static struct rtnl_qdisc *
find_qdisc(const char *if_name)
{
    int qdisc_if_index;
    int if_index = conf_net_if_wrapper_if_nametoindex(if_name);
    qdisc_context_node *qdisc_ctx = NULL;

    SLIST_FOREACH(qdisc_ctx, &qdiscs, links)
    {
        qdisc_if_index = rtnl_tc_get_ifindex(TC_CAST(qdisc_ctx->qdisc));
        if (qdisc_if_index == if_index)
            return qdisc_ctx->qdisc;
    }

    return NULL;
}

/* See the description in conf_tc_internal.h */
struct rtnl_qdisc *
conf_tc_internal_get_qdisc(const char *if_name)
{
    struct rtnl_qdisc * qdisc = find_qdisc(if_name);

    return qdisc == NULL ? add_qdisc(if_name) : qdisc;
}

/* See the description in conf_tc_internal.h */
te_errno
conf_tc_internal_nl_error2te_errno(int nl_error)
{
    /* libnl function return negative error */
    if (nl_error < 0)
        nl_error = -nl_error;

    switch (nl_error) {
        case NLE_SUCCESS:       return 0;
        case NLE_EXIST:         return TE_RC(TE_TA_UNIX, TE_EEXIST);
        case NLE_NOMEM:         return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        case NLE_INVAL:         return TE_RC(TE_TA_UNIX, TE_EINVAL);

        default:
            WARN("Cannot convert libnl error to TE error: %s",
                 nl_geterror(nl_error));
            return TE_RC(TE_TA_UNIX, TE_EUNKNOWN);
    }
}

/* See the description in conf_tc_internal.h */
te_errno
conf_tc_internal_qdisc_enable(const char *if_name)
{
    int rc;
    struct rtnl_qdisc *qdisc = NULL;

    if ((qdisc = conf_tc_internal_get_qdisc(if_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strcmp(rtnl_tc_get_kind(TC_CAST(qdisc)), "clsact") == 0)
    {
#ifdef WITH_BPF
        rtnl_tc_set_handle(TC_CAST(qdisc), TC_HANDLE(TC_H_CLSACT, 0));
        rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_CLSACT);
#else
        ERROR("clsact qdisc is not supported");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif /* WITH_BPF */
    }
    else
    {
        rtnl_tc_set_handle(TC_CAST(qdisc), TC_HANDLE(1, 0));
        rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_ROOT);
    }

    rc = rtnl_qdisc_add(conf_tc_internal_get_sock(), qdisc,
                        NLM_F_CREATE | NLM_F_REPLACE);

    return conf_tc_internal_nl_error2te_errno(rc);
}

/* See the description in conf_tc_internal.h */
te_errno
conf_tc_internal_qdisc_disable(const char *if_name)
{
    int rc;
    struct rtnl_qdisc *qdisc = NULL;

    if ((qdisc = conf_tc_internal_get_qdisc(if_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rtnl_tc_set_handle(TC_CAST(qdisc), 0);
    rc = rtnl_qdisc_delete(conf_tc_internal_get_sock(), qdisc);

    return conf_tc_internal_nl_error2te_errno(rc);
}
