/** @file
 * @brief Network namespaces configuration
 *
 * Implementation of configuration nodes for network namespaces management.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf NETNS"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"

#if defined(__linux__) && defined(USE_LIBNETCONF)

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "conf_netconf.h"
#include "conf_common.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "te_string.h"
#include "te_alloc.h"
#include "agentlib.h"
#include "unix_internal.h"

#include "netconf.h"

#if defined(CLONE_NEWNET) && defined(HAVE_SETNS)

/* Directory with named network namespaces file descriptors. */
#define NETNS_FDS_DIR "/var/run/netns"

/* Format string of a network namespace file descriptor. */
#define NETNS_FDS_FMT NETNS_FDS_DIR "/%s"

/* Buffer size to keep namespaces list. */
#define NETNS_LIST_BUF_SIZE 4096

/**
 * Close network namespace file descriptor @p _fd, return @b te_errno from a
 * function in case of failure.
 */
#define NETNS_CLOSE_FD(_fd) \
    do {                                                            \
        if (close(_fd) != 0)                                        \
        {                                                           \
            ERROR("%s: cannot close namespace file descriptor",     \
                  __FUNCTION__);                                    \
            return TE_OS_RC(TE_TA_UNIX, errno);                     \
        }                                                           \
    } while(0)

/* Network interface structure. */
typedef struct netns_interface {
    SLIST_ENTRY(netns_interface)  ent_l;
    char *name;
} netns_interface;

/* Network namespace structure. */
typedef struct netns_namespace {
    SLIST_ENTRY(netns_namespace)  ent_l;
    SLIST_HEAD(, netns_interface) ifs_h;
    char *name;
} netns_namespace;

/* Head of the network namespaces list. */
static SLIST_HEAD(, netns_namespace) netns_h;

/**
 * Get network namespace file descriptor @p fd by its name @p ns_name.
 *
 * @return Status code.
 */
static te_errno
netns_get_fd(const char *ns_name, int *fd)
{
    char netns_path[RCF_MAX_PATH];
    int fd_i;

    snprintf(netns_path, RCF_MAX_PATH, NETNS_FDS_FMT, ns_name);
    fd_i = open(netns_path, O_RDONLY | O_CLOEXEC);
    if (fd_i < 0)
    {
        ERROR("Failed to open namespace file descriptor %s", ns_name);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    *fd = fd_i;
    return 0;
}

/**
 * Get namespace structure by its name.
 *
 * @param name      The namespace name.
 * @param netns     Pointer to the namespace structure pointer.
 *
 * @return Status code.
 */
static te_errno
netns_namespace_find_by_name(const char *name, netns_namespace **netns)
{
    netns_namespace *netns_i;

    SLIST_FOREACH(netns_i, &netns_h, ent_l)
    {
        if (strcmp(name, netns_i->name) == 0)
        {
            *netns = netns_i;
            return 0;
        }
    }

    ERROR("Cannot find namespace %s object", name);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Get interface structure by its namespace and name.
 *
 * @param netns     Network namespace handle.
 * @param name      The interface name.
 * @param netif     The interface handle.
 *
 * @return Status code.
 */
static te_errno
netns_interface_find_by_name(netns_namespace *netns, const char *name,
                             netns_interface **netif)
{
    netns_interface *netif_i;

    SLIST_FOREACH(netif_i, &netns->ifs_h, ent_l)
    {
        if (strcmp(name, netif_i->name) == 0)
        {
            *netif = netif_i;
            return 0;
        }
    }

    ERROR("Cannot find interface %s in namespace %s", name, netns->name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Release memory allocated for a @b netns_interface object.
 */
static void
netns_interface_release(netns_interface *netif)
{
    free(netif->name);
    free(netif);
}

/**
 * Release memory allocated for a @b netns_namespace object.
 */
static void
netns_namespace_release(netns_namespace *netns)
{
    netns_interface *netif;

    while (SLIST_EMPTY(&netns->ifs_h) == FALSE)
    {
        netif = SLIST_FIRST(&netns->ifs_h);
        SLIST_REMOVE_HEAD(&netns->ifs_h, ent_l);
        netns_interface_release(netif);
    }

    free(netns->name);
    free(netns);
}

/**
 * Add new namespace object to the list @p netns_h.
 *
 * @param ns_name   The namespace name.
 *
 * @return Status code.
 */
static te_errno
netns_namespace_add(const char *ns_name)
{
    netns_namespace *netns;

    netns = TE_ALLOC(sizeof(*netns));
    if (netns == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    netns->name = strdup(ns_name);
    if (netns->name == NULL)
    {
        free(netns);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    SLIST_INIT(&netns->ifs_h);
    SLIST_INSERT_HEAD(&netns_h, netns, ent_l);

    return 0;
}

/**
 * Delete a namespace object from the list @p netns_h.
 *
 * @param ns_name   The namespace name.
 *
 * @return Status code.
 */
static te_errno
netns_namespace_del(const char *ns_name)
{
    netns_namespace *netns;
    te_errno         rc;

    rc = netns_namespace_find_by_name(ns_name, &netns);
    if (rc != 0)
        return rc;

    SLIST_REMOVE(&netns_h, netns, netns_namespace, ent_l);
    netns_namespace_release(netns);

    return 0;
}

/**
 * Move a network interface to a network namespace.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param value     The object value (unused).
 * @param ns        Namespace configurator instance (unused).
 * @param ns_name   The namespace name.
 * @param if_name   The interface name.
 *
 * @return Status code.
 */
static te_errno
netns_interface_add(unsigned int gid, const char *oid, const char *value,
                    const char *ns, const char *ns_name,
                    const char *if_name)
{
    netns_namespace *netns;
    netns_interface *netif;
    te_errno         rc;
    int              fd;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ns);
    UNUSED(value);

    rc = netns_get_fd(ns_name, &fd);
    if (rc != 0)
        return rc;

    rc = netconf_link_set_ns(nh, if_name, fd, 0);
    NETNS_CLOSE_FD(fd);
    if (rc != 0)
    {
        ERROR("Failed to move interface %s to the namespace %s", if_name,
              ns_name);
        return rc;
    }

    rc = netns_namespace_find_by_name(ns_name, &netns);
    if (rc != 0)
        return rc;

    netif = TE_ALLOC(sizeof(*netif));
    if (netif == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    netif->name = strdup(if_name);
    if (netif->name == NULL)
    {
        free(netif);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    SLIST_INSERT_HEAD(&netns->ifs_h, netif, ent_l);

    return 0;
}

/**
 * Switch network namespace of the current process to @p ns_name.
 *
 * @return Status code.
 */
static te_errno
netns_switch(const char *ns_name)
{
    te_errno rc = 0;
    int      fd;

    rc = netns_get_fd(ns_name, &fd);
    if (rc != 0)
        return rc;

    if (setns(fd, CLONE_NEWNET) != 0)
    {
        ERROR("Cannot change network namespace to %s", ns_name);
        rc = TE_OS_RC(TE_TA_UNIX, errno);
    }

    NETNS_CLOSE_FD(fd);

    return rc;
}

/**
 * Entry point to the process for moving an interface from a network
 * namespace to the parent process namespace.
 *
 * @param ns_name   The namespace name.
 * @param if_name   The interface name.
 *
 * @return Exit code.
 */
int
netns_unset_interface_process(const char *ns_name, const char *if_name)
{
    te_errno        rc;
    netconf_handle  nch;

    rc = netns_switch(ns_name);
    if (rc != 0)
    {
        ERROR("Failed to move process to namespace %s: %r", ns_name, rc);
        exit(EXIT_FAILURE);
    }

    if (netconf_open(&nch, NETLINK_ROUTE) < 0)
    {
        ERROR("Cannot open netconf session: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    rc = netconf_link_set_ns(nch, if_name, -1, getppid());
    netconf_close(nch);
    if (rc != 0)
    {
        ERROR("Failed to move interface %s to the parent namespace %s: %r",
              if_name, ns_name, rc);
        exit(EXIT_FAILURE);
    }

    return 0;
}

/**
 * Move a network interface from network namespace @p ns_name to the parent
 * process namespace.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param ns        Namespace configurator instance (unused).
 * @param ns_name   The namespace name.
 * @param if_name   The interface name.
 *
 * @return Status code.
 */
static te_errno
netns_interface_del(unsigned int gid, const char *oid, const char *ns,
                    const char *ns_name, const char *if_name)
{
    netns_namespace *netns;
    netns_interface *netif;
    te_errno         rc;
    pid_t            pid;
    int              status;
    int              argc = 2;
    const char      *params[argc];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ns);

    params[0] = ns_name;
    params[1] = if_name;

    rc = netns_namespace_find_by_name(ns_name, &netns);
    if (rc != 0)
        return rc;

    rc = netns_interface_find_by_name(netns, if_name, &netif);
    if (rc != 0)
        return rc;

    if ((rc = rcf_ch_start_process(&pid, -1,
                                   "netns_unset_interface_process", FALSE,
                                   argc, (void **)params)) != 0)
    {
        ERROR("Failed to start auxiliary process");
        return rc;
    }

    if (ta_waitpid(pid, &status, 0) <= 0)
    {
        ERROR("Failed to get status of the interface moving process");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    if (status != 0)
    {
        ERROR("Non-zero status of the interface moving process: %d", status);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    SLIST_REMOVE(&netns->ifs_h, netif, netns_interface, ent_l);
    netns_interface_release(netif);

    return 0;
}

/**
 * Get list of network interfaces moved to a namespace.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full identifier of the father instance (unused).
 * @param sub_id    ID of the object to be listed (unused).
 * @param list      Where to save the list.
 * @param ns        Namespace configurator instance (unused).
 * @param ns_name   The namespace name.
 *
 * @return Status code.
 */
static te_errno
netns_interface_list(unsigned int gid, const char *oid,
                     const char *sub_id, char **list,
                     const char *ns, const char *ns_name)
{
    netns_namespace *netns;
    netns_interface *netif;
    te_string        te_str = TE_STRING_INIT;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(ns);

    rc = netns_namespace_find_by_name(ns_name, &netns);
    if (rc != 0)
        return rc;

    SLIST_FOREACH(netif, &netns->ifs_h, ent_l)
    {
        rc = te_string_append(&te_str, "%s ", netif->name);
        if (rc != 0)
        {
            te_string_free(&te_str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = te_str.ptr;

    return 0;
}

/**
 * Create new network namespace. Current process is naturally moved to the
 * namespace.
 *
 * @param ns_name   The namespace name.
 *
 * @param Status code.
 */
static te_errno
netns_create(const char *ns_name)
{
    char netns_path[RCF_MAX_PATH];
    int fd;
    int rc;

    rc = mkdir(NETNS_FDS_DIR,
               S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (rc != 0 && errno != EEXIST)
    {
        ERROR("Failed to create the namespace directory %s", NETNS_FDS_DIR);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rc = mount("", NETNS_FDS_DIR, "none", MS_SHARED | MS_REC, NULL);
    if (rc != 0)
    {
        if (errno != EINVAL)
        {
            ERROR("Failed to share mount point %s", NETNS_FDS_DIR);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        rc = mount(NETNS_FDS_DIR, NETNS_FDS_DIR, "none", MS_BIND, NULL);
        if (rc != 0)
        {
            ERROR("Failed to bind mount point %s", NETNS_FDS_DIR);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        rc = mount("", NETNS_FDS_DIR, "none", MS_SHARED | MS_REC, NULL);
        if (rc != 0)
        {
            ERROR("Failed to share mount point %s", NETNS_FDS_DIR);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
    }

    snprintf(netns_path, RCF_MAX_PATH, NETNS_FDS_FMT, ns_name);
    fd = open(netns_path, O_RDONLY | O_CREAT | O_EXCL, 0);
    if (fd < 0)
    {
        ERROR("Cannot create network namespace file %s", netns_path);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    NETNS_CLOSE_FD(fd);

    rc = unshare(CLONE_NEWNET);
    if (rc != 0)
    {
        ERROR("Failed to unshare network namespace");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rc = mount("/proc/self/ns/net", netns_path, "none", MS_BIND, NULL);
    if (rc != 0)
    {
        ERROR("Cannot perform a bind mount to the new namespace file");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Entry point to the process to create a new network namespace.
 *
 * @param ns_name   The namespace name.
 *
 * @return Exit code.
 */
int
netns_create_process(const char *ns_name)
{
    te_errno rc;

    rc = netns_create(ns_name);
    if (rc != 0)
    {
        ERROR("Failed to create new network namespace %s: %r", ns_name, rc);
        exit(EXIT_FAILURE);
    }

    return 0;
}

/**
 * Add new network namespace.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param value     The object value (unused).
 * @param ns        Namespace configurator instance (unused).
 * @param ns_name   The namespace name.
 *
 * @return      Status code.
 */
static te_errno
netns_add(unsigned int gid, const char *oid, const char *value,
          const char *ns, const char *ns_name)
{
    te_errno         rc;
    pid_t            pid;
    int              status;
    int              argc = 1;
    const char      *params[argc];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ns);
    UNUSED(value);

    params[0] = ns_name;

    /* Create new process to avoid moving the main agent process to the new
     * namespace. */
    if ((rc = rcf_ch_start_process(&pid, -1, "netns_create_process", FALSE,
                                   argc, (void **)params)) != 0)
    {
        ERROR("Failed to start auxiliary process");
        return rc;
    }

    if (ta_waitpid(pid, &status, 0) <= 0)
    {
        ERROR("Failed to get status of the namespace creation process");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    if (status != 0)
    {
        ERROR("Non-zero status of the namespace creation process: %d",
              status);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Delete a network namespace.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param ns        Namespace configurator instance (unused).
 * @param ns_name   The namespace name.
 *
 * @return      Status code.
 */
static te_errno
netns_del(unsigned int gid, const char *oid, const char *ns,
          const char *ns_name)
{
    char             netns_path[RCF_MAX_PATH];
    int              rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ns);

    snprintf(netns_path, RCF_MAX_PATH, NETNS_FDS_FMT, ns_name);

    rc = umount2(netns_path, MNT_DETACH);
    if (rc != 0)
    {
        ERROR("Failed to unmount the namespace file %s", netns_path);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rc = unlink(netns_path);
    if (rc != 0)
    {
        ERROR("Cannot remove the namespace file %s", netns_path);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Check if a namespace is grabbed by the test agent.
 *
 * @param ns_name   Namespace name.
 * @param data      Opaque data (unused).
 *
 * @return @c TRUE if the namespace is grabbed, @c FALSE otherwise.
 */
static te_bool
netns_check_rsrc_cb(const char *ns_name, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/namespace:/net:%s",
                                   ta_name, ns_name);
}

/**
 * Get list of network namespaces.
 *
 * @param gid     Group identifier (unused).
 * @param oid     Full identifier of the father instance (unused).
 * @param sub_id  ID of the object to be listed (unused).
 * @param list    Where to save the list.
 * @param ns      Namespace configurator instance (unused).
 *
 * @return Status code.
 */
static te_errno
netns_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list, const char *ns)
{
    te_errno  rc;
    char      list_buf[NETNS_LIST_BUF_SIZE];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(ns);

    rc = get_dir_list(NETNS_FDS_DIR, list_buf, sizeof(list_buf), TRUE,
                      &netns_check_rsrc_cb, NULL);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            return 0;

        ERROR("Failed to get namespaces list %r", rc);
        return rc;
    }

    if ((*list = strdup(list_buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}


RCF_PCH_CFG_NODE_COLLECTION(node_interface, "interface", NULL, NULL,
                            netns_interface_add, netns_interface_del,
                            netns_interface_list, NULL);

RCF_PCH_CFG_NODE_COLLECTION(node_netns, "net", &node_interface, NULL,
                            netns_add, netns_del, netns_list, NULL);

/**
 * Grab a network namespace resource hook.
 *
 * @param obj   The configurator object.
 *
 * @return Status code.
 */
static te_errno
netns_rsrc_grab(const char *obj)
{
    char *ns_name = strrchr(obj, ':');

    if (ns_name == NULL)
    {
        ERROR("Unknown resource object format: %s", obj);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    ns_name++;

    return netns_namespace_add(ns_name);
}

/**
 * Release a network namespace resource hook.
 *
 * @param obj   The configurator object.
 *
 * @return Status code.
 */
static te_errno
netns_rsrc_release(const char *obj)
{
    char *ns_name = strrchr(obj, ':');

    if (ns_name == NULL)
    {
        ERROR("Unknown resource object format: %s", obj);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    ns_name++;

    return netns_namespace_del(ns_name);
}


/**
 * Initialize network namespaces subtree.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_ns_net_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent/namespace/", &node_netns);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/namespace/net",
                             netns_rsrc_grab,
                             netns_rsrc_release);
}

#else /* CLONE_NEWNET && HAVE_SETNS */
te_errno
ta_unix_conf_ns_net_init(void)
{
    INFO("Network namespaces are not supported");
    return 0;
}
#endif /* !(CLONE_NEWNET && HAVE_SETNS) */

#else /* __linux__ && USE_LIBNETCONF */
te_errno
ta_unix_conf_ns_net_init(void)
{
    INFO("Network namespaces configuration is supported only on linux and"
         "TE build must include netconf library");
    return 0;
}

#endif /* !(__linux__ && USE_LIBNETCONF) */
