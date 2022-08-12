/** @file
 * @brief Configuration TAPI to work with /local/host subtree
 *
 * Implementation of test API to manage the configurator subtree `/local/host`
 * which describes relations between agents, namespaces and network interfaces
 * on a host.
 * (@path{storage/cm/cm_local.xml})
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "Host NS TAPI"

#include "te_config.h"
#include "te_alloc.h"
#include "te_string.h"

#include "logger_api.h"
#include "tapi_cfg_base.h"
#include "tapi_host_ns.h"
#include "tapi_namespaces.h"

#define TAPI_HOST_NS_TREE_AGENT "/local:/host:%s/agent:%s"
#define TAPI_HOST_NS_TREE_IF TAPI_HOST_NS_TREE_AGENT "/interface:%s"
#define TAPI_HOST_NS_TREE_IF_PARENT TAPI_HOST_NS_TREE_IF "/parent:%d"

/* See description in tapi_host_ns.h */
te_bool
tapi_host_ns_enabled(void)
{
    te_errno    rc;
    cfg_handle  handle = CFG_HANDLE_INVALID;

    rc = cfg_find_str("/local/host", &handle);
    if (rc != 0 || handle == CFG_HANDLE_INVALID)
        return FALSE;

    return TRUE;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_get_host(const char *ta, char **host)
{
    te_errno      rc;
    unsigned int  count;
    cfg_handle   *agt_handles;
    cfg_handle    host_handle;

    rc = cfg_find_pattern_fmt(&count, &agt_handles,
                              TAPI_HOST_NS_TREE_AGENT, "*", ta);
    if (rc != 0 || count == 0)
    {
        if (rc == 0)
            rc = TE_RC(TE_TAPI, TE_ENOENT);
        ERROR("Cannot find host name of the agent '%s': %r", ta, rc);
        return rc;
    }

    rc = cfg_get_father(agt_handles[0], &host_handle);
    if (rc != 0)
    {
        ERROR("Failed to get the host handle");
        free(agt_handles);
        return rc;
    }

    rc = cfg_get_inst_name(host_handle, host);
    if (rc != 0)
        ERROR("Failed to get the host name");
    free(agt_handles);

    return rc;
}

/**
 * Make string with link to an interface.
 *
 * @param host      Hostname or @c NULL
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param link      Pointer to the link location
 *
 * @return Status code.
 */
static te_errno
tapi_host_ns_if_make_link(const char *host, const char *ta,
                          const char *ifname, char **link)
{
    te_errno  rc;
    char     *host_loc = NULL;
    te_string str = TE_STRING_INIT;

    if (host == NULL)
    {
        rc = tapi_host_ns_get_host(ta, &host_loc);
        if (rc != 0)
            return rc;
        host = host_loc;
    }

    rc = te_string_append(&str, TAPI_HOST_NS_TREE_IF, host, ta, ifname);
    if (rc == 0)
        *link = str.ptr;
    free(host_loc);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_agent_add(const char *host, const char *ta, const char *netns)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                              TAPI_HOST_NS_TREE_AGENT, host, ta);
    if (rc == 0 && netns != NULL)
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, netns),
                                  TAPI_HOST_NS_TREE_AGENT "/netns:",
                                  host, ta);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_agent_del(const char *ta)
{
    te_errno rc;
    char    *host;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = cfg_del_instance_fmt(FALSE, TAPI_HOST_NS_TREE_AGENT, host, ta);
    free(host);

    return rc;
}

/**
 * Get new instance index to add parent reference.
 *
 * @param host      Hostname
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param index     Index value
 *
 * @return Status code.
 */
static te_errno
tapi_host_ns_if_new_parent_index(const char *host, const char *ta,
                                 const char *ifname, int *index)
{
    unsigned int  count;
    unsigned int  i;
    cfg_handle   *parent_handles;
    te_errno      rc;
    int           tmp;
    int           max = -1;
    char         *name;

    rc = cfg_find_pattern_fmt(&count, &parent_handles, TAPI_HOST_NS_TREE_IF
                              "/parent:*", host, ta, ifname);
    if (rc != 0)
    {
        ERROR("Cannot get parents list: %r", rc);
        return rc;
    }

    for (i = 0; i < count; i++)
    {
        rc = cfg_get_inst_name(parent_handles[i], &name);
        if (rc != 0)
            break;

        tmp = atoi(name);
        free(name);

        if (tmp > max)
            max = tmp;
    }
    if (rc == 0)
        *index = max + 1;

    if (*index < 0)
    {
        ERROR("You got a fish trophy! Index counter limit is reached.");
        rc = TE_RC(TE_TAPI, TE_ERANGE);
    }

    free(parent_handles);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_parent_add(const char *ta, const char *ifname,
                           const char *parent_ta, const char *parent_ifname)
{
    te_errno rc;
    char    *host;
    char    *link = NULL;
    int      index;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = tapi_host_ns_if_make_link(host, parent_ta, parent_ifname, &link);
    if (rc != 0)
    {
        free(host);
        return rc;
    }

    rc = tapi_host_ns_if_new_parent_index(host, ta, ifname, &index);
    if (rc == 0)
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, link),
                                  TAPI_HOST_NS_TREE_IF_PARENT,
                                  host, ta, ifname, index);
    }
    free(host);
    free(link);

    if (rc != 0)
        ERROR("Failed to add parent interface link %s/%s to "
              "interface %s/%s: %r", parent_ta, parent_ifname, ta, ifname,
              rc);

    return rc;
}

/**
 * Remove parent link if @p handle value matches.
 *
 * @param handle    Parent instance handle
 * @param link      Link to parent interface
 * @param all       Return @c TE_EOK if the parent link is found and
 *                  successfully removed
 *
 * @return Status code.
 */
static te_errno
rm_parent_link(cfg_handle handle, char *link, te_bool all)
{
    te_errno rc;
    char    *val;

    rc = cfg_get_instance(handle, NULL, &val);
    if (rc != 0)
    {
        ERROR("Cannot get a parent link: %r", rc);
        return rc;
    }

    if (strcmp(link, val) == 0)
    {
        rc = cfg_del_instance(handle, FALSE);
        if (rc == 0 && !all)
            rc = TE_RC(TE_TAPI, TE_EOK);
    }

    free(val);

    return rc;
}

/**
 * Callback function to remove parent link. It stops iterating as soon
 * appropriate link is met.
 *
 * @param handle    Parent instance handle
 * @param opaque    Parent interface link
 *
 * @return Status code:
 *  - @c EOK if the parent link is found and removed;
 *  - @c 0 no matched links.
 */
static te_errno
rm_parent_link_cb(cfg_handle handle, void *opaque)
{
    return rm_parent_link(handle, (char *)opaque, FALSE);
}

/**
 * Callback function to remove all parent links.
 *
 * @param handle    Parent instance handle
 * @param opaque    Parent interface link
 *
 * @return Status code.
 */
static te_errno
rm_parent_link_all_cb(cfg_handle handle, void *opaque)
{
    return rm_parent_link(handle, (char *)opaque, TRUE);
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_parent_del(const char *ta, const char *ifname,
                           const char *parent_ta, const char *parent_ifname)
{
    te_errno rc;
    char    *host;
    char    *link;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = tapi_host_ns_if_make_link(host, parent_ta, parent_ifname, &link);
    if (rc != 0)
    {
        free(host);
        return rc;
    }

    rc = cfg_find_pattern_iter_fmt(&rm_parent_link_cb, (void *)link,
                                   TAPI_HOST_NS_TREE_IF "/parent:*",
                                   host, ta, ifname);
    free(host);
    free(link);

    if (rc == TE_RC(TE_TAPI, TE_EOK))
        return 0;
    else if (rc == 0)
        return TE_RC(TE_TAPI, TE_ENOENT);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_add(const char *ta, const char *ifname,
                    const char *parent_ifname)
{
    te_errno rc;
    char    *host;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                              TAPI_HOST_NS_TREE_IF, host, ta, ifname);
    if (rc == 0 && parent_ifname != NULL)
        rc = tapi_host_ns_if_parent_add(ta, ifname, ta, parent_ifname);

    free(host);
    if (rc != 0)
        ERROR("Failed to add interface %s/%s to the agent subtree: %r",
              ta, ifname, rc);

    return rc;
}

/**
 * Remove parent all references to interface.
 *
 * @param host      Hostname
 * @param ta        Test agent name
 * @param ifname    Interface name
 *
 * @return Status code.
 */
static te_errno
tapi_host_ns_if_refs_del(const char *host, const char *ta, const char *ifname)
{
    te_errno    rc;
    char       *link;

    rc = tapi_host_ns_if_make_link(host, ta, ifname, &link);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern_iter_fmt(&rm_parent_link_all_cb, (void *)link,
                                   TAPI_HOST_NS_TREE_IF "/parent:*",
                                   host, "*", "*");
    free(link);

    if (rc != 0)
        ERROR("Failed to delete references interface %s/%s: %r",
              ta, ifname, rc);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_del(const char *ta, const char *ifname, te_bool del_refs)
{
    te_errno rc;
    te_errno rc2;
    char    *host;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    if (del_refs)
        rc = tapi_host_ns_if_refs_del(host, ta, ifname);

    rc2 = cfg_del_instance_fmt(TRUE, TAPI_HOST_NS_TREE_IF, host, ta, ifname);
    free(host);
    if (rc == 0)
        rc = rc2;

    if (rc != 0)
        ERROR("Failed to delete interface %s/%s: %r", ta, ifname, rc);

    return rc;
}

/**
 * Callback function to copy parent links to specified interface.
 *
 * @param handle    Parent instance handle
 * @param opaque    String with the target interface instance
 *
 * @return Status code.
 */
static te_errno
cp_parent_cb(cfg_handle handle, void *opaque)
{
    te_errno rc;
    char    *target_if = (char *)opaque;
    char    *val;
    char    *index;

    rc = cfg_get_instance(handle, NULL, &val);
    if (rc != 0)
    {
        ERROR("Cannot get a parent link: %r", rc);
        return rc;
    }

    rc = cfg_get_inst_name(handle, &index);
    if (rc != 0)
    {
        ERROR("Cannot get a instance name: %r", rc);
        free(val);
        return rc;
    }

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, val),
                              "%s/parent:%s", target_if, index);
    free(index);
    free(val);

    return rc;
}

/* Context to update parent references. */
typedef struct update_link_context {
    char *old;
    char *new;
} update_link_context;

/**
 * Callback function to update parent links.
 *
 * @param handle    Parent instance handle
 * @param opaque    @b update_link_context
 *
 * @return Status code.
 */
static te_errno
update_parent_cb(cfg_handle handle, void *opaque)
{
    update_link_context *link = (update_link_context *)opaque;
    te_errno             rc;
    char                *val;

    rc = cfg_get_instance(handle, NULL, &val);
    if (rc != 0)
    {
        ERROR("Cannot get a parent link: %r", rc);
        return rc;
    }

    if (strcmp(link->old, val) == 0)
        rc = cfg_set_instance(handle, CFG_VAL(STRING, link->new));
    free(val);

    return rc;
}

/**
 * Update all parent references of and to interface after moving it to another
 * test agent.
 *
 * @param host      Hostname or @c NULL
 * @param ta        Test agent name
 * @param ns_ta     New owner name of the interface
 * @param ifname    Interface name
 *
 * @return Status code.
 */
static te_errno
update_parents(const char *host, const char *ta, const char *ns_ta,
               const char *ifname)
{
    update_link_context link = {NULL, NULL};
    te_errno            rc;

    rc = tapi_host_ns_if_make_link(host, ns_ta, ifname, &link.new);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern_iter_fmt(&cp_parent_cb, (void *)link.new,
                                   TAPI_HOST_NS_TREE_IF "/parent:*",
                                   host, ta, ifname);
    if (rc == 0)
    {
        rc = tapi_host_ns_if_make_link(host, ta, ifname, &link.old);
        if (rc == 0)
            rc = cfg_find_pattern_iter_fmt(&update_parent_cb, (void *)&link,
                                           TAPI_HOST_NS_TREE_IF "/parent:*",
                                           host, "*", "*");
    }

    free(link.old);
    free(link.new);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_change_ns(const char *ta, const char *ifname,
                          const char *ns_name, const char *ns_ta)
{
    te_errno rc;
    char    *host = NULL;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

#define CHRC(_rc, _err...) \
    do {                 \
        if (_rc != 0)    \
        {                \
            free(host);  \
            ERROR(_err); \
            return _rc;  \
        }                \
    } while (0);

    rc = tapi_netns_if_set(ta, ns_name, ifname);
    CHRC(rc, "Failed to move interface %s/%s to net namespace %s: %r",
         ta, ifname, ns_name, rc);

    rc = tapi_cfg_base_if_add_rsrc(ns_ta, ifname);
    CHRC(rc, "Failed to grab interface %s/%s resource: %r",
         ns_ta, ifname, rc);

    rc = tapi_host_ns_if_add(ns_ta, ifname, NULL);
    CHRC(rc, "Failed to add interface %s to the agent %s: %r",
         ifname, ns_ta, rc);

    rc = update_parents(host, ta, ns_ta, ifname);
    CHRC(rc, "Failed to update parent links of the interface %s/%s: %r",
         ns_ta, ifname, rc);

    rc = tapi_host_ns_if_del(ta, ifname, FALSE);
    CHRC(rc, "Cannot delete moved interface %s/%s: %r", ta, ifname, rc);

#undef CHRC

    return 0;
}

/* Context to pass to parent and child iterator callbacks. */
typedef struct iterate_if_ctx {
    tapi_host_ns_if_cb_func cb;
    char                   *link;
    void                   *opaque;
} iterate_if_ctx;

/**
 * Callback function to iterate interface children.
 *
 * @param handle    Parent instance handle
 * @param opaque    Iterator context (@b iterate_if_ctx)
 *
 * @return Status code.
 */
static te_errno
iterate_child_cb(cfg_handle handle, void *opaque)
{
    iterate_if_ctx *ctx = (iterate_if_ctx *)opaque;
    te_errno        rc;
    char           *ifname = NULL;
    char           *val = NULL;
    char           *ta = NULL;

    rc = cfg_get_instance(handle, NULL, &val);
    if (rc != 0)
    {
        ERROR("Cannot get a parent link: %r", rc);
        return rc;
    }

#define CHRC(_rc, _err...) \
    do {                    \
        if (_rc != 0)       \
        {                   \
            free(ifname);   \
            free(val);      \
            free(ta);       \
            ERROR(_err);    \
            return _rc;     \
        }                   \
    } while (0);

    if (strcmp(ctx->link, val) == 0)
    {
        cfg_handle phandle;

        rc = cfg_get_father(handle, &phandle);
        CHRC(rc, "Failed to get interface handle");
        rc = cfg_get_inst_name(phandle, &ifname);
        CHRC(rc, "Failed to get interface instance name");
        rc = cfg_get_father(phandle, &phandle);
        CHRC(rc, "Failed to get test agent handle");
        rc = cfg_get_inst_name(phandle, &ta);
        CHRC(rc, "Failed to get test agent instance name");

        rc = ctx->cb(ta, ifname, ctx->opaque);

        free(ifname);
        free(ta);
    }
    free(val);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_child_iter(const char *ta, const char *ifname,
                           tapi_host_ns_if_cb_func cb, void *opaque)
{
    iterate_if_ctx ctx = {.cb = cb, .link = NULL, .opaque = opaque};
    char          *host = NULL;
    te_errno       rc;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = tapi_host_ns_if_make_link(host, ta, ifname, &ctx.link);
    if (rc == 0)
    {
        rc = cfg_find_pattern_iter_fmt(&iterate_child_cb, (void *)&ctx,
                                       TAPI_HOST_NS_TREE_IF "/parent:*",
                                       host, "*", "*");
        free(ctx.link);
    }
    free(host);

    return rc;
}

/**
 * Callback function to iterate interface parents.
 *
 * @param handle    Parent instance handle
 * @param opaque    Iterator context (@b iterate_if_ctx)
 *
 * @return Status code.
 */
static te_errno
iterate_parent_cb(cfg_handle handle, void *opaque)
{
    iterate_if_ctx *ctx = (iterate_if_ctx *)opaque;
    te_errno rc;
    char    *link = NULL;
    char    *ifname = NULL;
    char    *ta = NULL;
    char    *saveptr = NULL;
    char    *saveptr2 = NULL;
    char    *token = NULL;

    rc = cfg_get_instance(handle, NULL, &link);
    if (rc != 0)
    {
        ERROR("Cannot get a parent link: %r", rc);
        return rc;
    }

    for (token = strtok_r(link, "/", &saveptr);
         token != NULL;
         token = strtok_r(NULL, "/", &saveptr))
    {
        token = strtok_r(token, ":", &saveptr2);
        if (token != NULL)
        {
            if (strcmp(token, "interface") == 0)
            {
                ifname = strtok_r(NULL, ":", &saveptr2);
                break;
            }
            else if (strcmp(token, "agent") == 0)
            {
                ta = strtok_r(NULL, ":", &saveptr2);
            }
        }
    }

    if (ifname == NULL || ta == NULL)
    {
        ERROR("Failed to parse interface link: ta %s, ifname %s", ta, ifname);
        rc = TE_RC(TE_TAPI, TE_EFMT);
    }
    else
    {
        rc = ctx->cb(ta, ifname, ctx->opaque);
    }

    free(link);
    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_parent_iter(const char *ta, const char *ifname,
                            tapi_host_ns_if_cb_func cb, void *opaque)
{
    iterate_if_ctx ctx = {.cb = cb, .opaque = opaque};
    char          *host = NULL;
    te_errno       rc;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern_iter_fmt(&iterate_parent_cb, (void *)&ctx,
                                   TAPI_HOST_NS_TREE_IF "/parent:*",
                                   host, ta, ifname);
    free(host);

    return rc;
}

/**
 * Callback function to find test agent in default netns. It stops iterating
 * as soon the first appropriate agent is met.
 *
 * @param handle    `netns` instance handle
 * @param opaque    Pointer to save agent name
 *
 * @return Status code:
 * @retval EOK if test agent is found;
 * @retval 0 agent is in a net namespace, continue the search.
 */
static te_errno
get_default_netns_ta_cb(cfg_handle handle, void *opaque)
{
    cfg_handle  ta_handle;
    te_errno    rc;
    char       *netns;
    char      **ta = (char **)opaque;

    rc = cfg_get_instance(handle, NULL, &netns);
    if (rc != 0)
    {
        ERROR("Failed to get netns instance value: %r", rc);
        return rc;
    }

    /* Default netns is empty string. */
    if (*netns != '\0')
    {
        free(netns);
        return 0;
    }
    free(netns);

    rc = cfg_get_father(handle, &ta_handle);
    if (rc != 0)
    {
        ERROR("Failed to get test agent handle");
        return rc;
    }

    rc = cfg_get_inst_name(ta_handle, ta);
    if (rc != 0)
        ERROR("Failed to get the host name");
    else
        rc = TE_RC(TE_TAPI, TE_EOK);

    return rc;
}


/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_agent_default(const char *ta, char **ta_default)
{
    te_errno rc;
    char    *host;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern_iter_fmt(&get_default_netns_ta_cb,
                                   (void *)ta_default,
                                   TAPI_HOST_NS_TREE_AGENT "/netns:",
                                   host, "*");
    free(host);

    if (rc == TE_RC(TE_TAPI, TE_EOK))
        return 0;
    else if (rc == 0)
        return TE_RC(TE_TAPI, TE_ENOENT);

    return rc;
}

/**
 * Callback function to iterate all interfaces on a test agent.
 *
 * @param handle    Parent instance handle
 * @param opaque    Iterator context (@b iterate_if_ctx)
 *
 * @return Status code.
 */
static te_errno
iterate_ta_cb(cfg_handle handle, void *opaque)
{
    iterate_if_ctx *ctx = (iterate_if_ctx *)opaque;
    cfg_handle      ta_h = CFG_HANDLE_INVALID;

    te_errno rc;
    char    *ifname = NULL;
    char    *ta = NULL;

    rc = cfg_get_inst_name(handle, &ifname);
    if (rc != 0)
    {
        ERROR("Failed to get interface instance name");
        return rc;
    }

    if ((rc = cfg_get_father(handle, &ta_h)) != 0 ||
        (rc = cfg_get_inst_name(ta_h, &ta)) != 0 )
        ERROR("Failed to get agent name");
    else
        rc = ctx->cb(ta, ifname, ctx->opaque);

    free(ifname);
    free(ta);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_ta_iter(const char *ta, tapi_host_ns_if_cb_func cb,
                        void *opaque)
{
    iterate_if_ctx ctx = {.cb = cb, .opaque = opaque};
    char          *host = NULL;
    te_errno       rc;

    rc = tapi_host_ns_get_host(ta, &host);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern_iter_fmt(&iterate_ta_cb, (void *)&ctx,
                                   TAPI_HOST_NS_TREE_IF, host, ta, "*");
    free(host);

    return rc;
}

/* See description in tapi_host_ns.h */
te_errno
tapi_host_ns_if_host_iter(const char *host, tapi_host_ns_if_cb_func cb,
                          void *opaque)
{
    iterate_if_ctx ctx = {.cb = cb, .opaque = opaque};

    return cfg_find_pattern_iter_fmt(&iterate_ta_cb, (void *)&ctx,
                                     TAPI_HOST_NS_TREE_IF, host, "*", "*");
}
