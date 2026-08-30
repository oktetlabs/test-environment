/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * XEN configuring support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf XEN"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* XEN support */
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h> /** For 'struct stat' */

#define XEN_SUPPORT 1
#else
#define XEN_SUPPORT 0
#endif /* HAVE_SYS_STAT_H */

#include "te_alloc.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_str.h"
#include "logger_api.h"
#include "rcf_pch.h"
#include "te_shell_cmd.h"
#include "unix_internal.h"
#include "conf_common.h"

static char buf[4096];

/* XEN stuff interface */
static te_errno xen_path_get(unsigned int, char const *, char *);
static te_errno xen_path_set(unsigned int, char const *, char const *);

static te_errno xen_subpath_get(unsigned int, char const *, char *);
static te_errno xen_subpath_set(unsigned int, char const *, char const *);

static te_errno xen_kernel_get(unsigned int, char const *, char *);
static te_errno xen_kernel_set(unsigned int, char const *, char const *);

static te_errno xen_initrd_get(unsigned int, char const *, char *);
static te_errno xen_initrd_set(unsigned int, char const *, char const *);

static te_errno xen_dsktpl_get(unsigned int, char const *, char *);
static te_errno xen_dsktpl_set(unsigned int, char const *, char const *);

static te_errno xen_rcf_port_get(unsigned int, char const *, char *);
static te_errno xen_rcf_port_set(unsigned int, char const *,
                                 char const *);

static te_errno xen_rpc_br_get(unsigned int, char const *, char *);
static te_errno xen_rpc_br_set(unsigned int, char const *, char const *);

static te_errno xen_rpc_if_get(unsigned int, char const *, char *);
static te_errno xen_rpc_if_set(unsigned int, char const *, char const *);

static te_errno xen_base_mac_addr_get(unsigned int, char const *, char *);
static te_errno xen_base_mac_addr_set(unsigned int, char const *,
                                      char const *);

static te_errno xen_accel_get(unsigned int, char const *, char *);
static te_errno xen_accel_set(unsigned int, char const *, char const *);

static te_errno xen_init_set(unsigned int, char const *, char const *);

static te_errno xen_interface_add(unsigned int, char const *, char const *,
                                  char const *, char const *);
static te_errno xen_interface_del(unsigned int, char const *, char const *,
                                  char const *);
static te_errno xen_interface_list(unsigned int, char const *,
                                   const char *, char **);
static te_errno xen_interface_get(unsigned int, char const *, char *,
                                  char const *, char const *);
static te_errno xen_interface_set(unsigned int, char const *, char const *,
                                  char const *, char const *);

static te_errno xen_interface_bridge_get(unsigned int, char const *,
                                         char *, char const *,
                                         char const *);
static te_errno xen_interface_bridge_set(unsigned int, char const *,
                                         char const *, char const *,
                                         char const *);

static te_errno dom_u_add(unsigned int, char const *, char const *,
                          char const *, char const *);
static te_errno dom_u_del(unsigned int, char const *, char const *,
                          char const *);
static te_errno dom_u_list(unsigned int, char const *,
                           const char *, char **);
static te_errno dom_u_get(unsigned int, char const *, char *,
                          char const *, char const *);
static te_errno dom_u_set(unsigned int, char const *, char const *,
                          char const *, char const *);

static te_errno dom_u_status_get(unsigned int, char const *, char *,
                                 char const *, char const *);
static te_errno dom_u_status_set(unsigned int, char const *,
                                 char const *, char const *,
                                 char const *);

static te_errno dom_u_memory_get(unsigned int, char const *, char *,
                                 char const *, char const *);
static te_errno dom_u_memory_set(unsigned int, char const *,
                                 char const *, char const *,
                                 char const *);

static te_errno dom_u_ip_addr_get(unsigned int, char const *, char *,
                                  char const *, char const *);
static te_errno dom_u_ip_addr_set(unsigned int, char const *,
                                  char const *, char const *,
                                  char const *);

static te_errno dom_u_mac_addr_get(unsigned int, char const *, char *,
                                   char const *, char const *);
static te_errno dom_u_mac_addr_set(unsigned int, char const *,
                                   char const *, char const *,
                                   char const *);

static te_errno dom_u_bridge_add(unsigned int, char const *, char const *,
                                 char const *, char const *, char const *);
static te_errno dom_u_bridge_del(unsigned int, char const *, char const *,
                                 char const *, char const *);
static te_errno dom_u_bridge_list(unsigned int, char const *,
                                  const char *, char **,
                                  char const *, char const *);
static te_errno dom_u_bridge_get(unsigned int, char const *, char *,
                                 char const *, char const *, char const *);
static te_errno dom_u_bridge_set(unsigned int, char const *, char const *,
                                 char const *, char const *, char const *);

static te_errno dom_u_bridge_ip_addr_get(unsigned int, char const *,
                                         char *, char const *,
                                         char const *, char const *);
static te_errno dom_u_bridge_ip_addr_set(unsigned int, char const *,
                                         char const *, char const *,
                                         char const *, char const *);

static te_errno dom_u_bridge_mac_addr_get(unsigned int, char const *,
                                          char *, char const *,
                                          char const *, char const *);
static te_errno dom_u_bridge_mac_addr_set(unsigned int, char const *,
                                          char const *, char const *,
                                          char const *, char const *);

static te_errno dom_u_bridge_accel_get(unsigned int, char const *,
                                       char *, char const *,
                                       char const *, char const *);
static te_errno dom_u_bridge_accel_set(unsigned int, char const *,
                                       char const *, char const *,
                                       char const *, char const *);

static te_errno dom_u_migrate_set(unsigned int, char const *,
                                  char const *, char const *,
                                  char const *);

static te_errno dom_u_migrate_kind_get(unsigned int, char const *, char *,
                                       char const *, char const *);
static te_errno dom_u_migrate_kind_set(unsigned int, char const *,
                                       char const *, char const *,
                                       char const *);

/* XEN stuff tree */
RCF_PCH_CFG_NODE_RW(node_dom_u_migrate_kind, "kind",
                    NULL, NULL,
                    &dom_u_migrate_kind_get, &dom_u_migrate_kind_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_migrate, "migrate",
                    &node_dom_u_migrate_kind, NULL,
                    NULL, &dom_u_migrate_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_bridge_accel, "accel",
                    NULL, NULL,
                    &dom_u_bridge_accel_get,
                    &dom_u_bridge_accel_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_bridge_mac_addr, "mac_addr",
                    NULL, &node_dom_u_bridge_accel,
                    &dom_u_bridge_mac_addr_get,
                    &dom_u_bridge_mac_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_bridge_ip_addr, "ip_addr",
                    NULL, &node_dom_u_bridge_mac_addr,
                    &dom_u_bridge_ip_addr_get,
                    &dom_u_bridge_ip_addr_set);

static rcf_pch_cfg_object node_dom_u_bridge =
    { "bridge", 0, &node_dom_u_bridge_ip_addr, &node_dom_u_migrate,
      (rcf_ch_cfg_get)&dom_u_bridge_get,
      (rcf_ch_cfg_set)&dom_u_bridge_set,
      (rcf_ch_cfg_add)&dom_u_bridge_add,
      (rcf_ch_cfg_del)&dom_u_bridge_del,
      (rcf_ch_cfg_list)&dom_u_bridge_list, NULL, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_dom_u_mac_addr, "mac_addr",
                    NULL, &node_dom_u_bridge,
                    &dom_u_mac_addr_get, &dom_u_mac_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_ip_addr, "ip_addr",
                    NULL, &node_dom_u_mac_addr,
                    &dom_u_ip_addr_get, &dom_u_ip_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_memory, "memory",
                    NULL, &node_dom_u_ip_addr,
                    &dom_u_memory_get, &dom_u_memory_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_status, "status",
                    NULL, &node_dom_u_memory,
                    &dom_u_status_get, &dom_u_status_set);

static rcf_pch_cfg_object node_dom_u =
    { "dom_u", 0, &node_dom_u_status, NULL,
      (rcf_ch_cfg_get)&dom_u_get, (rcf_ch_cfg_set)&dom_u_set,
      (rcf_ch_cfg_add)&dom_u_add, (rcf_ch_cfg_del)&dom_u_del,
      (rcf_ch_cfg_list)&dom_u_list, NULL, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_xen_interface_bridge, "bridge",
                    NULL, NULL,
                    &xen_interface_bridge_get, &xen_interface_bridge_set);

static rcf_pch_cfg_object node_xen_interface =
    { "interface", 0, &node_xen_interface_bridge, &node_dom_u,
      (rcf_ch_cfg_get)&xen_interface_get,
      (rcf_ch_cfg_set)&xen_interface_set,
      (rcf_ch_cfg_add)&xen_interface_add,
      (rcf_ch_cfg_del)&xen_interface_del,
      (rcf_ch_cfg_list)&xen_interface_list, NULL, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_xen_init, "init",
                    NULL, &node_xen_interface,
                    NULL, &xen_init_set);

RCF_PCH_CFG_NODE_RW(node_xen_accel, "accel",
                    NULL, &node_xen_init,
                    &xen_accel_get, &xen_accel_set);

RCF_PCH_CFG_NODE_RW(node_base_mac_addr, "base_mac_addr",
                    NULL, &node_xen_accel,
                    &xen_base_mac_addr_get, &xen_base_mac_addr_set);

RCF_PCH_CFG_NODE_RW(node_rpc_if, "rpc_if",
                    NULL, &node_base_mac_addr,
                    &xen_rpc_if_get, &xen_rpc_if_set);

RCF_PCH_CFG_NODE_RW(node_rpc_br, "rpc_br",
                    NULL, &node_rpc_if,
                    &xen_rpc_br_get, &xen_rpc_br_set);

RCF_PCH_CFG_NODE_RW(node_rcf_port, "rcf_port",
                    NULL, &node_rpc_br,
                    &xen_rcf_port_get, &xen_rcf_port_set);

RCF_PCH_CFG_NODE_RW(node_dsktpl, "dsktpl",
                    NULL, &node_rcf_port,
                    &xen_dsktpl_get, &xen_dsktpl_set);

RCF_PCH_CFG_NODE_RW(node_initrd, "initrd",
                    NULL, &node_dsktpl,
                    &xen_initrd_get, &xen_initrd_set);

RCF_PCH_CFG_NODE_RW(node_kernel, "kernel",
                    NULL, &node_initrd,
                    &xen_kernel_get, &xen_kernel_set);

RCF_PCH_CFG_NODE_RW(node_subpath, "subpath",
                    NULL, &node_kernel,
                    &xen_subpath_get, &xen_subpath_set);

RCF_PCH_CFG_NODE_RW(node_xen, "xen",
                    &node_subpath, NULL,
                    &xen_path_get, &xen_path_set);

/* See the description in conf_common.h */
te_errno
ta_unix_conf_xen_init(void)
{
    return rcf_pch_add_node("/agent", &node_xen);
}

/* XEN stuff implementation */

#if XEN_SUPPORT

/** Maximal number of maintained domUs and bridges in every domU */
enum { MAX_DOM_U_NUM = 256, MAX_BRIDGE_NUM = 16, MAX_INTERFACE_NUM = 16 };

/** DomU statuses */
typedef enum { DOM_U_STATUS_NON_RUNNING,
               DOM_U_STATUS_RUNNING,
               DOM_U_STATUS_SAVED,
               DOM_U_STATUS_MIGRATED_RUNNING,
               DOM_U_STATUS_MIGRATED_SAVED,
               DOM_U_STATUS_ERROR } status_t;

/**
 * Path to accessible across network storage for
 * XEN kernel and templates of XEN config/VBD images.
 */
static char xen_path[PATH_MAX]    = { '\0' };

/** Subpath to XEN storage for dynamically created/destroyed domUs */
static char xen_subpath[PATH_MAX] = { '\0' };

/** Kernel, initial ramdisk and VBD image files */
static char xen_kernel[PATH_MAX]  = { '\0' };
static char xen_initrd[PATH_MAX]  = { '\0' };
static char xen_dsktpl[PATH_MAX]  = { '\0' };

/** RCF port number */
static unsigned int xen_rcf_port  = 0;

/** XEN dom0 RPC bridge and interface */
static char xen_rpc_br[PATH_MAX]  = { '\0' };
static char xen_rpc_if[PATH_MAX]  = { '\0' };

/** XEN domU base MAC address */
static char xen_base_mac_addr[] = "00:00:00:00:00:00";

/** Values that are used to initialize addresses */
static char const init_ip_addr[]  = "0.0.0.0";
static char const init_mac_addr[] = "00:00:00:00:00:00";

/* Names of the cloned disk image, swap image and temporary directory */
static char const *const xen_dskimg = "disk.img";
static char const *const xen_swpimg = "swap.img";
static char const *const xen_tmpdir = "tmpdir";

/** Status name to status and vice versa translation array */
static struct {
    char const *name; /**< Status name */
    status_t status;  /**< Status      */
} const statuses[] = {
    { "non-running",      DOM_U_STATUS_NON_RUNNING },
    { "running",          DOM_U_STATUS_RUNNING },
    { "saved",            DOM_U_STATUS_SAVED },
    { "migrated-running", DOM_U_STATUS_MIGRATED_RUNNING },
    { "migrated-saved",   DOM_U_STATUS_MIGRATED_SAVED } };

/** XEN vertual tested interface internal representation */
static struct {
    char const *if_name; /**< XEN virtual tested interface name       */
    char const *ph_name; /**< XEN realp hysical tested interface name */
    char const *br_name; /**< XEN bridge, which both interfaces
                              are connected to*/
} interface_slot[MAX_INTERFACE_NUM];

/** DomU internal representation */
static struct {
    char const   *name;          /**< DomU name (also serves as slot
                                      is empty sign if it is NULL)       */
    status_t     status;         /**< DomU state                         */
    unsigned int memory;         /**< DomU state                         */
    char         ip_addr[16];    /**< DomU IP address                    */
    char         mac_addr[18];   /**< DomU MAC address                   */

    struct {
       char const *br_name;      /**< Name of the bridge where dom0
                                      tested interface is added to       */
       char const *if_name;      /**< DomU testing interface name        */
       char        ip_addr[16];  /**< DomU testing interface IP address  */
       char        mac_addr[18]; /**< DomU testing interface MAC address */
       bool accel;        /**< Accelerated spec-tion in config    */
    }
    bridge_slot[MAX_BRIDGE_NUM]; /**< DomU bridges where dom0 tested
                                      interfaces are added to            */
    int          migrate_kind;   /**< Migrate kind (non-live/live)       */
} dom_u_slot[MAX_DOM_U_NUM];

/**
 * Get the whole number of domU slots.
 *
 * @return              The whole number of domU slots
 */
static inline unsigned int
dom_u_limit(void)
{
    return sizeof(dom_u_slot) / sizeof(*dom_u_slot);
}

/**
 * Find domU.
 *
 * @param dom_u         The name of the domU to find
 *
 * @return              domU index (from 0 to DOM_U_MAX_NUM - 1) if
 *                      found, otherwise - 'sizeof(dom_u_list) /
 *                      sizeof(*dom_u_list)' (which is equivlent to
 *                      MAX_DOM_U_NUM)
 */
static unsigned
find_dom_u(char const *dom_u)
{
    unsigned int u;
    unsigned int limit = dom_u_limit();

    for (u = 0; u < limit; u++)
    {
        char const *name = dom_u_slot[u].name;

        if (name != NULL && strcmp(name, dom_u) == 0)
            break;
    }

    return u;
}

/* Try to find domU and initialize its index */
#define FIND_DOM_U(dom_u_name_, dom_u_index_) \
    do {                                                               \
        if (((dom_u_index_) =                                          \
                  find_dom_u(dom_u_name_)) >= dom_u_limit())           \
        {                                                              \
            ERROR("DomU '%s' does NOT exist", (dom_u_name_));          \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                       \
        }                                                              \
    } while(0)


/**
 * Get the whole number of bridge slots.
 *
 * @return              The whole number of bridge slots
 */
static inline unsigned int
bridge_limit(void)
{
    return sizeof(dom_u_slot[0].bridge_slot) /
               sizeof(*dom_u_slot[0].bridge_slot);
}

/**
 * Find bridge.
 *
 * @param bridge        The name of the bridge to find
 *
 * @return              domU index (from 0 to MAX_BRIDGE_NUM - 1) if
 *                      found, otherwise - 'sizeof(bridge_slot) /
 *                      sizeof(*bridge_slot)' (which is equivlent to
 *                      MAX_BRIDGE_NUM)
 */
static unsigned
find_bridge(char const *bridge, unsigned int u)
{
    unsigned int v;
    unsigned int limit = bridge_limit();

    for (v = 0; v < limit; v++)
    {
        char const *name = dom_u_slot[u].bridge_slot[v].br_name;

        if (name != NULL && strcmp(name, bridge) == 0)
            break;
    }

    return v;
}

/* Try to find bridge and initialize its index */
#define FIND_BRIDGE(bridge_name_, dom_u_index_, bridge_index_) \
    do {                                                               \
        if (((bridge_index_) =                                         \
                 find_bridge((bridge_name_),                           \
                             (dom_u_index_))) >= bridge_limit())       \
        {                                                              \
            ERROR("Bridge '%s' in DomU '%s' does NOT exist",           \
                  (bridge_name_), dom_u_slot[dom_u_index_].name);      \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                       \
        }                                                              \
    } while(0)


/**
 * Get the whole number of interface slots.
 *
 * @return              The whole number of domU slots
 */
static inline unsigned int
interface_limit(void)
{
    return sizeof(interface_slot) / sizeof(*interface_slot);
}

/**
 * Find interface.
 *
 * @param interface     The name of the interface to find
 *
 * @return              interface index (from 0 to MAX_INTERFACE_NUM - 1)
 *                      if found, otherwise - 'sizeof(interface_slot) /
 *                      sizeof(*interface_slot)' (which is equivlent to
 *                      MAX_INTERFACE_NUM)
 */
static unsigned
find_interface(char const *interface)
{
    unsigned int u;
    unsigned int limit = interface_limit();

    for (u = 0; u < limit; u++)
    {
        char const *name = interface_slot[u].if_name;

        if (name != NULL && strcmp(name, interface) == 0)
            break;
    }

    return u;
}

/**
 * Find interface.
 *
 * @param bridge        The name of the bridge physical interface
 *                      is connected to
 *
 * @return              Physical interface name or NULL if not found
 */
static char const *
find_physical_interface(char const *bridge)
{
    unsigned int u;
    unsigned int limit = interface_limit();

    if (bridge == NULL)
        return NULL;

    for (u = 0; u < limit; u++)
    {
        if (interface_slot[u].if_name != NULL &&
            strcmp(interface_slot[u].br_name, bridge) == 0)
        {
            return interface_slot[u].ph_name;
        }
    }

    return NULL;
}

/* Try to find interface and initialize its index */
#define FIND_INTERFACE(interface_name_, interface_index_) \
    do {                                                                 \
        if (((interface_index_) =                                        \
                  find_interface(interface_name_)) >= interface_limit()) \
        {                                                                \
            ERROR("Interface '%s' does NOT exist", (interface_name_));   \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                         \
        }                                                                \
    } while(0)


/**
 * Converts status to its string representation.
 *
 * @param status        Status to be converted to string representation
 *
 * @return              String representation or NULL in case of an error
 */
static char const *
dom_u_status_to_string(status_t status)
{
    unsigned int u;

    for (u = 0; u < sizeof(statuses) / sizeof(*statuses); u++)
        if (statuses[u].status == status)
            return statuses[u].name;

    return NULL;
}

/**
 * Converts status string representation to status.
 *
 * @param status_string Status string representation
 *
 * @return              Converted status value or
 *                      DOM_U_STATUS_ERROR in case of an error
 */
static status_t
dom_u_status_string_to_status(char const *status_string)
{
    unsigned int u;

    for (u = 0; u < sizeof(statuses) / sizeof(*statuses); u++)
        if (strcmp(statuses[u].name, status_string) == 0)
            return statuses[u].status;

    return DOM_U_STATUS_ERROR;
}

/**
 * Checks whether the agent runs within dom0 or not
 *
 * @return              @c true if the agent runs within dom0,
 *                      otherwise - @c false
 */
static bool
is_within_dom0(void)
{
    struct stat st;

    /* Probably there is better mean do detect we are within dom0, eh? */
    return stat("/usr/sbin/xm", &st) == 0 &&
           (S_ISLNK(st.st_mode) || S_ISREG(st.st_mode));
}

/**
 * Removes directory and all its subdirectories
 *
 * @param dir           Directory path
 *
 * @return              Status code
 */
static te_errno
xen_rmfr(char const *dir)
{
    /* FIXME: Non "ta_system" implementation is needed*/
    char const* const cmd = "rm -fr ";
    char *const cmdline = TE_ALLOC(strlen(cmd) + strlen(dir) + 1);

    strcpy(cmdline, cmd);
    strcat(cmdline, dir);

    if (ta_system(cmdline) != 0)
    {
        free(cmdline);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    free(cmdline);
    return 0;
}

/**
 * Forms full path to domU disk image dynamic storage
 *
 * @param dom_u         domU
 * @param fname         File name inside disk image (path from root /)
 * @param fdata         Data string (zero ended)
 *
 * @return              Status code
 */
static char const *
get_dom_u_path(char const *dom_u)
{
    size_t      xen_path_len    = strlen(xen_path);
    size_t      xen_subpath_len = strlen(xen_subpath);
    size_t      dom_u_len       = strlen(dom_u);
    static char dom_u_path[PATH_MAX];
    char       *ptr             = dom_u_path;

    if (xen_path_len + 1 +
            xen_subpath_len + (xen_subpath_len > 0 ? 1 : 0) +
            dom_u_len + 1 > sizeof(dom_u_path))
    {
        *dom_u_path = '\0';
    }
    else
    {
        memcpy(ptr, xen_path, xen_path_len);
        ptr   += xen_path_len;
        *ptr++ = '/';

        if (xen_subpath_len > 0)
        {
            memcpy(ptr, xen_subpath, xen_subpath_len);
            ptr   += xen_subpath_len;
            *ptr++ = '/';
        }

        strcpy(ptr, dom_u);
    }

    return dom_u_path;
}

/**
 * (Re)creates file inside disk image and fills it with supplied data
 *
 * @param dom_u         domU
 * @param fname         File name inside disk image (path from root /)
 * @param fdata         Data string (zero ended)
 *
 * @return              Status code
 */
static te_errno
xen_fill_file_in_disk_image(char const *dom_u, char const *fname,
                            char const *fdata)
{
    char              buffer[PATH_MAX];
    char const *const dom_u_path = get_dom_u_path(dom_u);
    struct stat       st;
    te_errno          rc = 0;
    FILE             *f;
    int               sys;

    TE_SPRINTF(buffer, "%s/%s", dom_u_path, xen_tmpdir);

    if (stat(buffer, &st) == 0)
        goto cleanup2;

    if (mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to create temporary %s directory", buffer);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup0;
    }

    if (chmod(buffer, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to chmod temporary %s directory", buffer);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buffer, "mount -o loop %s/%s %s/%s",
               dom_u_path, xen_dskimg, dom_u_path, xen_tmpdir);

    if ((sys = ta_system(buffer)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buffer, "%s/%s%s", dom_u_path, xen_tmpdir, fname);

    if ((f = fopen(buffer, "w")) == NULL)
    {
        ERROR("Failed to open %s file for writing", buffer);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup2;
    }

    if ((size_t)fprintf(f, "%s", fdata) != strlen(fdata))
    {
        ERROR("Failed to write %s file with data:\n%s", buffer, fdata);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (fclose(f) != 0)
    {
        ERROR("Failed to close %s file after writing", buffer);

        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup2:
    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buffer, "umount %s/%s", dom_u_path, xen_tmpdir);

    if ((sys = ta_system(buffer)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup1:
    TE_SPRINTF(buffer, "%s/%s", dom_u_path, xen_tmpdir);

    if (rmdir(buffer) == -1)
    {
        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup0:
    return rc;
}

/**
 * Checks that all attributes of initialized
 * domU interfaces are also initialized properly
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
check_dom_u_is_initialized_properly(unsigned int u)
{
    unsigned int v;
    unsigned int limit = bridge_limit();

    if (dom_u_slot[u].memory == 0)
    {
        ERROR("Memory amount for '%s' domU is UNspecified",
              dom_u_slot[u].name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (*xen_rpc_br == '\0')
    {
        ERROR("The name of the bridge that is used for RCF/RPC "
              "communication ('/agent/xen/rpc_br') is NOT initialized");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (*xen_rpc_if == '\0')
    {
        ERROR("The name of the interface that is used for RCF/RPC "
              "communication ('/agent/xen/rpc_if') is NOT initialized");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strcmp(dom_u_slot[u].ip_addr, init_ip_addr) == 0)
    {
        ERROR("The IP address of the interface that is used for "
              "RCF/RPC communication ('/agent/xen/dom_u/ip_addr') "
              "is NOT initialized for '%s' domU", dom_u_slot[u].name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strcmp(dom_u_slot[u].mac_addr, init_mac_addr) == 0)
    {
        ERROR("The MAC address of the interface that is used for "
              "RCF/RPC communication ('/agent/xen/dom_u/mac_addr') "
              "is NOT initialized for '%s' domU", dom_u_slot[u].name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    for (v = 0; v < limit; v++)
    {
        char const *br_name = dom_u_slot[u].bridge_slot[v].br_name;

        if (br_name != NULL)
        {
            char const *if_name = dom_u_slot[u].bridge_slot[v].if_name;

            if (*if_name == '\0')
            {
                ERROR("The name of the interface that is used for "
                      "testing communication over '%s' bridge (the "
                      "value of '/agent/xen/dom_u/bridge') is NOT "
                      "initialized for '%s' domU", br_name,
                      dom_u_slot[u].name);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }

            if (strcmp(dom_u_slot[u].bridge_slot[v].ip_addr,
                       init_ip_addr) == 0)
            {
                ERROR("The IP address of the '%s' interface that is "
                      "used for testing communication over '%s' "
                      "bridge ('/agent/xen/dom_u/bridge/ip_addr') "
                      "is NOT initialized for '%s' domU", if_name,
                      br_name, dom_u_slot[u].name);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }

            if (strcmp(dom_u_slot[u].bridge_slot[v].mac_addr,
                       init_mac_addr) == 0)
            {
                ERROR("The MAC address of the '%s' interface that is "
                      "used for testing communication over '%s' "
                      "bridge ('/agent/xen/dom_u/bridge/mac_addr') "
                      "is NOT initialized for '%s' domU", if_name,
                      br_name, dom_u_slot[u].name);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
        }
    }

    return 0;
}

/**
 * Fills the next part of the 'buf' or resets buffer pointer
 *
 * @param fmt           Format (resets buffer pointer if NULL)
 *
 * @return              Status code
 */
static te_errno
update_buf(char const *fmt, ...)
{
    int     num;
    va_list ap;

    static char *ptr = buf;

    if (fmt == NULL)
    {
        *(ptr = buf) = '\0';
        return 0;
    }

    va_start(ap, fmt);
    num = vsnprintf(ptr, buf + sizeof(buf) - ptr, fmt, ap);
    va_end(ap);

    if (num < 0)
    {
        *(ptr = buf) = '\0';
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /* Check whether truncation has occurred */
    if (num >= buf + sizeof(buf) - ptr)
    {
        ERROR("Buffer size (%u) is too small", sizeof(buf));
        *(ptr = buf) = '\0';
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    ptr += num;

    return 0;
}

/**
 * Fills 'buf' with the network interfaces data for domU config file
 *
 * @param u             DomU slot number
 * @param i             DomU bridge slot number
 *
 * @return              Status code
 */
static te_errno
add_dom_u_interfaces_config(unsigned int u, int i)
{
    if (i < 0)
    {
        return update_buf("vif  = [ 'bridge=%s,mac=%s'",
                          xen_rpc_br, dom_u_slot[u].mac_addr);
    }

    if (i >= (int) bridge_limit())
        return update_buf(" ]\n");

    {
        char const *brd = dom_u_slot[u].bridge_slot[i].br_name;
        char const *mac = dom_u_slot[u].bridge_slot[i].mac_addr;
        char const *phy = find_physical_interface(brd);

        if (brd != NULL)
        {
            if (phy == NULL)
            {
                ERROR("Internal error: cannot find "
                      "physical interface by bridge name");
                return TE_RC(TE_TA_UNIX, TE_EFAIL);
            }

            return dom_u_slot[u].bridge_slot[i].accel ?
                       update_buf(",'bridge=%s,accel=%s,mac=%s'",
                                  brd, phy, mac) :
                       update_buf(",'bridge=%s,mac=%s'", brd, mac);
        }
    }

    return 0;
}

/**
 * Fills 'buf' with the network interfaces data for domU config file
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
prepare_dom_u_interfaces_config(unsigned int u)
{
    int i;
    int limit = (int)bridge_limit();

    update_buf(NULL); /** Restart buffer pointer (see update_conf) */

    /* Prepare interfaces */
    for (i = -1; i <= limit; i++)
    {
        te_errno rc = add_dom_u_interfaces_config(u, i);

        if (rc != 0)
            return rc;
    }

    return 0;
}

/**
 * Fills 'buf' with the data for domU
 * '/etc/udev/rules.d/z25_persistent-net.rules'
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
prepare_persistent_net_rules(unsigned int u)
{
    int   i;
    int   limit = (int) bridge_limit();

    update_buf(NULL); /** Restart buffer pointer (see update_conf) */

    /* Prepare interfaces */
    for (i = -1; i < limit; i++)
    {
        if (i < 0 || dom_u_slot[u].bridge_slot[i].br_name != NULL)
        {
            char const *mac = i < 0 ?
                           dom_u_slot[u].mac_addr :
                           dom_u_slot[u].bridge_slot[i].mac_addr;
            char const *ifn = i < 0 ?
                           xen_rpc_if :
                           dom_u_slot[u].bridge_slot[i].if_name;

            te_errno    rc = update_buf("\n"
                                        "# Xen virtual device (vif)\n"
                                        "SUBSYSTEM==\"net\", "
                                        "DRIVERS==\"?*\", "
                                        "ATTRS{address}==\"%s\", "
                                        "NAME=\"%s\"\n", mac, ifn);

            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/**
 * Fills 'buf' with the data for domU '/etc/network/interfaces'
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
prepare_network_interfaces_config(unsigned int u)
{
    int   i;
    int   limit = (int) bridge_limit();

    update_buf(NULL); /** Restart buffer pointer (see update_conf) */

    /* Prepare interfaces */
    for (i = -1; i < limit; i++)
    {
        if (i < 0 || dom_u_slot[u].bridge_slot[i].br_name != NULL)
        {
            char const *hdr = i < 0 ?
                           "auto lo\niface lo inet loopback\n" :
                           "";
            char const *ifn = i < 0 ?
                           xen_rpc_if :
                           dom_u_slot[u].bridge_slot[i].if_name;
            char const *ipa = i < 0 ?
                           dom_u_slot[u].ip_addr :
                           dom_u_slot[u].bridge_slot[i].ip_addr;

            te_errno    rc = update_buf("%s\nauto %s\n"
                                        "iface %s inet static\n"
                                        "    address %s\n"
                                        "    netmask 255.255.255.0\n",
                                        hdr, ifn, ifn, ipa);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}
#endif /* XEN_SUPPORT */

/**
 * Get path to accessible across network storage for
 * XEN kernel and templates of XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for path to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_path_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_path);
    return 0;
#else
#warning '/agent/xen' 'get' access method is not implemented
    ERROR("'/agent/xen' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set path to accessible across network storage for
 * XEN kernel and templates of XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         path to set
 *
 * @return              Status code
 */
static te_errno
xen_path_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN path: domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN path fits XEN path storage */
    if (len >= sizeof(xen_path))
    {
        ERROR("XEN path is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN path perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        if (*value != '/')
        {
            ERROR("XEN path must be absolute (starting from \"/\")");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if (stat(value, &st) == -1)
        {
            ERROR("Path specified for XEN does NOT exist");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISDIR(st.st_mode))
        {
            ERROR("Path specified for XEN is not a directory");
            return TE_RC(TE_TA_UNIX, TE_ENOTDIR);
        }
    }

    memcpy(xen_path, value, len + 1);
    return 0;
#else
#warning '/agent/xen' 'set' access method is not implemented
    UNUSED(value);

    ERROR("'/agent/xen' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get subpath to accessible across network storage for
 * XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for path to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_subpath_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_subpath);
    return 0;
#else
#warning '/agent/xen/subpath' 'get' access method is not implemented
    ERROR("'/agent/xen/subpath' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set subpath to accessible across network storage for
 * XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         path to set
 *
 * @return              Status code
 */
static te_errno
xen_subpath_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    size_t len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* Check whether XEN subpath fits XEN subpath storage */
    if (len >= sizeof(xen_subpath))
    {
        ERROR("XEN subpath is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_subpath, value, len + 1);
    return 0;
#else
#warning '/agent/xen/subpath' 'set' access method is not implemented
    UNUSED(value);

    ERROR("'/agent/xen/subpath' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN kernel file name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for kernel file name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_kernel_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_kernel);
    return 0;
#else
#warning '/agent/xen/kernel' 'get' access method is not implemented
    ERROR("'/agent/xen/kernel' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN kernel file name (XEN path must be set properly previously).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         kernel file name to set
 *
 * @return              Status code
 */
static te_errno
xen_kernel_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* XEN path must be previously properly set */
    if (*xen_path == '\0')
    {
        ERROR("Failed to set XEN kernel file name because "
              "XEN path is NOT set properly yet");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN kernel file name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN kernel file name fits XEN path storage */
    if (len >= sizeof(xen_kernel))
    {
        ERROR("XEN kernel file name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN kernel file name perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        TE_SPRINTF(buf, "%s/%s", xen_path, value);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN kernel does NOT exist on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN kernel specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }

    memcpy(xen_kernel, value, len + 1);
    return 0;
#else
#warning '/agent/xen/kernel' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/kernel' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN initial ramdisk file name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for initrd file name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_initrd_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_initrd);
    return 0;
#else
#warning '/agent/xen/initrd' 'get' access method is not implemented
    ERROR("'/agent/xen/initrd' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN initrd file name (XEN path must be set properly previously).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         initrd file name to set
 *
 * @return              Status code
 */
static te_errno
xen_initrd_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* XEN path must be previously properly set */
    if (*xen_path == '\0')
    {
        ERROR("Failed to set XEN initrd file name because "
              "XEN path is NOT set properly yet");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN initrd file name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN initrd file name fits XEN path storage */
    if (len >= sizeof(xen_initrd))
    {
        ERROR("XEN initrd file name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN initrd file name perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        TE_SPRINTF(buf, "%s/%s", xen_path, value);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN initrd does NOT exist on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN initrd specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }

    memcpy(xen_initrd, value, len + 1);
    return 0;
#else
#warning '/agent/xen/initrd' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/initrd' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN dsktpl file name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for dsktpl file name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_dsktpl_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_dsktpl);
    return 0;
#else
#warning '/agent/xen/dsktpl' 'get' access method is not implemented
    ERROR("'/agent/xen/dsktpl' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN dsktpl file name (XEN path must be set properly previously).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         dsktpl file name to set
 *
 * @return              Status code
 */
static te_errno
xen_dsktpl_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* XEN path must be previously properly set */
    if (*xen_path == '\0')
    {
        ERROR("Failed to set XEN dsktpl file name because "
              "XEN path is NOT set properly yet");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN dsktpl file name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN dsktpl file name fits XEN path storage */
    if (len >= sizeof(xen_dsktpl))
    {
        ERROR("XEN dsktpl file name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN dsktpl file name perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        TE_SPRINTF(buf, "%s/%s", xen_path, value);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN dsktpl does NOT exist on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN dsktpl specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }

    memcpy(xen_dsktpl, value, len + 1);
    return 0;
#else
#warning '/agent/xen/dsktpl' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/dsktpl' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get RCF port number.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for RCF port number to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_rcf_port_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    sprintf(value, "%u", xen_rcf_port);
    return 0;
#else
#warning '/agent/xen/rcf_port' 'get' access method is not implemented
    ERROR("'/agent/xen/rcf_port' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set RCF port numer (restrictions are applied).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         RCF port number to set
 *
 * @return              Status code
 */
static te_errno
xen_rcf_port_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    int          port  = atoi(value); /** Relying on value validity */
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not 0 then the agent must run within dom0 */
    if (port != 0 && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change RCF port number: domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* For non-0 RCF port number perform all necessary checks */
    if (port != 0 && port < 1024 && port > 65535)
    {
        ERROR("RCF port number is neither 0 "
              "nor in the range from 1024 to 65535");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    xen_rcf_port = port;
    return 0;
#else
#warning '/agent/xen/rcf_port' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/rcf_port' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN RPC bridge name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for RPC bridge name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_rpc_br_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_rpc_br);
    return 0;
#else
#warning '/agent/xen/rpc_br' 'get' access method is not implemented
    ERROR("'/agent/xen/rpc_br' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN RPC bridge name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         RPC bridge name to set
 *
 * @return              Status code
 */
static te_errno
xen_rpc_br_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN RPC bridge name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN RPC bridge name fits XEN path storage */
    if (len >= sizeof(xen_rpc_br))
    {
        ERROR("XEN RPC bridge name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_rpc_br, value, len + 1);
    return 0;
#else
#warning '/agent/xen/rpc_br' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/rpc_br' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN RPC interface name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for RPC interface name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_rpc_if_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_rpc_if);
    return 0;
#else
#warning '/agent/xen/rpc_if' 'get' access method is not implemented
    ERROR("'/agent/xen/rpc_if' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN RPC interface name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         RPC interface name to set
 *
 * @return              Status code
 */
static te_errno
xen_rpc_if_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN RPC interface name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN RPC interface name fits XEN path storage */
    if (len >= sizeof(xen_rpc_if))
    {
        ERROR("XEN RPC interface name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_rpc_if, value, len + 1);
    return 0;
#else
#warning '/agent/xen/rpc_if' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/rpc_if' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN domU base MAC address template.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for base MAC address to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_base_mac_addr_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    strcpy(value, xen_base_mac_addr);
    return 0;
#else
    UNUSED(value);
#warning '/agent/xen/base_mac_addr' 'get' \
access method is not implemented
    ERROR("'/agent/xen/base_mac_addr' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN domU base MAC address template.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         Base MAC address to set
 *
 * @return              Status code
 */
static te_errno
xen_base_mac_addr_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN base MAC address template: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN base MAC address fits XEN path storage */
    if (len >= sizeof(xen_rpc_if))
    {
        ERROR("XEN base MAC address template is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_base_mac_addr, value, len + 1);
    return 0;
#else
#warning '/agent/xen/base_mac_addr' 'set' \
access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/base_mac_addr' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

#if XEN_SUPPORT
static te_errno
xen_executive(char const *cmd)
{
    int        fd;
    int        st;
    ssize_t    rd  = 0;
    pid_t      pid = te_shell_cmd(cmd, -1, NULL, &fd, NULL);
    te_errno   rc  = 0;

    if (pid == -1)
        return TE_OS_RC(TE_TA_UNIX, errno);

    ta_waitpid(pid, &st, 0);

    if (st != 0 || (rd = read(fd, buf, sizeof(buf) - 1)) < 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);

    close(fd);

    while (rd > 0 && buf[rd - 1] == '\n')
        rd--;

    buf[rd] = '\0';
    return rc;
}

static te_errno
xen_accel_get_executive(bool *status)
{
    char const pt[] = "sfc_netback";
    te_errno   rc   = xen_executive("lsmod | grep -w ^sfc_netback "
                                    "2> /dev/null | awk '{print$1}'");

    if (rc == 0)
        *status = strncmp(buf, pt, sizeof(pt) - 1) == 0 ? true : false;

    return rc;
}
#endif

/**
 * Get XEN dom0 acceleration status
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for acceleration status
 *
 * @return              Status code
 */
static te_errno
xen_accel_get(unsigned int gid, char const *oid, char *value)
{
#if XEN_SUPPORT
    bool status;
    te_errno rc;
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* The agent must run within dom0 */
    if (!is_within_dom0())
    {
        value = "0";
        return 0;
    }

    if ((rc = xen_accel_get_executive(&status)) == 0)
        strcpy(value, status ? "1" : "0");

    return rc;
#else
    UNUSED(value);
#warning '/agent/xen/accel' 'get' \
access method is not implemented
    ERROR("'/agent/xen/accel' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN dom0 acceleration status
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         acceleration status
 *
 * @return              Status code
 */
static te_errno
xen_accel_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    bool status;
    bool needed_status = strcmp(value, "0") == 0 ? false : true;
    te_errno    rc;
    char const *cmd = NULL;
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* The agent must run within dom0 */
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if ((rc = xen_accel_get_executive(&status)) == 0)
    {
        if (status)
        {
            if (!needed_status)
                cmd = "/sbin/rmmod sfc_netback";
        }
        else
        {
            if (needed_status)
                cmd = "/sbin/modprobe sfc_netback";
        }

        if (cmd != NULL)
        {
            if (ta_system(cmd) != 0)
            {
                rc = TE_OS_RC(TE_TA_UNIX, errno);
            }
            else if ((rc = xen_accel_get_executive(&status)) == 0)
            {
                if ((needed_status && !status) ||
                    (!needed_status && status))
                {
                    ERROR("Failed to set acceleration %s",
                          status ? "ON" : "OFF");
                    rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
                }
            }
        }
    }

    return rc;
#else
#warning '/agent/xen/accel' 'set' \
access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/accel' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Perform XEN dom0 initialization/cleanup
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         init "command"
 *
 * @return              Status code
 */
static te_errno
xen_init_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    char const  *cmd_list = "/usr/sbin/xm list | awk '{print$1}' | "
                            "grep -v 'Name' | grep -v 'Domain-0'";
    char const  *cmd_shut = "for dom_u in "
                            "`/usr/sbin/xm list | awk '{print $1}' | "
                            "grep -v 'Name' | grep -v 'Domain-0'`; "
                            "do /usr/sbin/xm shutdown $dom_u; done";
    char const  *cmd_dest = "for dom_u in "
                            "`/usr/sbin/xm list | awk '{print $1}' | "
                            "grep -v 'Name' | grep -v 'Domain-0'`; "
                            "do /usr/sbin/xm destroy $dom_u; done";

    te_errno     rc;
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    /* The agent must run within dom0 */
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (*xen_path == '\0')
    {
        ERROR("XEN path is NOT set");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if ((rc =  xen_executive(cmd_list)) != 0)
        return rc;

    if (*buf == '\0')
        goto clear_xen_sub_path;

    RING("Shutting down domUs:\n%s", buf);

    if ((rc =  xen_executive(cmd_shut)) != 0)
        return rc;

    for (u = 0; u < 9; u++)
    {
        if ((rc =  xen_executive(cmd_list)) != 0)
            return rc;

        if (*buf == '\0')
            goto clear_xen_sub_path;

        sleep(3);
    }

    RING("Destroying domUs:\n%s", buf);

    if ((rc =  xen_executive(cmd_dest)) != 0)
        return rc;

    for (u = 0; u < 9; u++)
    {
        if ((rc =  xen_executive(cmd_list)) != 0)
            return rc;

        if (*buf == '\0')
            goto clear_xen_sub_path;

        sleep(3);
    }

    ERROR("Failed to shutdown and then destroy all domUs");
    return TE_RC(TE_TA_UNIX, TE_EFAIL);

clear_xen_sub_path:

    TE_SPRINTF(buf, "%s/%s/*", xen_path, xen_subpath);

    if ((rc = xen_rmfr(buf)) != 0)
        ERROR("Failed to clear XEN subpath '%s'", buf);

    return rc;
#else
#warning '/agent/xen/init' 'set' \
init method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/init' 'set' "
          "init method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get real physical interface name by the name of the virtual one.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for real interface name to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the virtual interface
 *
 * @return              Status code
 */
static te_errno
xen_interface_get(unsigned int gid, char const *oid, char *value,
                  char const *xen, char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_INTERFACE(interface, u);

    strcpy(value, interface_slot[u].ph_name);
    return 0;
#else
#warning '/agent/xen/interface' 'get' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set real physical interface name by the name of the virtual one.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         real interface name to set
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the virtual interface
 *
 * @return              Status code
 */
static te_errno
xen_interface_set(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *interface)
{
#if XEN_SUPPORT
    char const  *ph_name;
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN bridge name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    FIND_INTERFACE(interface, u);

    if ((ph_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free((void *)interface_slot[u].ph_name);
    interface_slot[u].ph_name = ph_name;
    return 0;
#else
#warning '/agent/xen/interface' 'set' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Add new XEN virtual tested interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         initializing value (not used)
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the XEN virtual tested interface to add
 *
 * @return              Status code
 */
static te_errno
xen_interface_add(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to delete XEN virtual tested interface: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    if ((u = find_interface(interface)) < interface_limit())
    {
        ERROR("Failed to add interface %s: it already exists", interface);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    /* Find an empty slot */
    for (u = 0, limit = interface_limit(); u < limit; u++)
        if (interface_slot[u].if_name == NULL)
            break;

    /* If an empty slot is NOT found */
    if (u == limit)
    {
        ERROR("Failed to add interface %s: all interface slots are taken",
              interface);
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    if ((interface_slot[u].br_name = strdup("")) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((interface_slot[u].ph_name = strdup(value)) == NULL)
    {
        free((void *)interface_slot[u].br_name);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if ((interface_slot[u].if_name = strdup(interface)) == NULL)
    {
        free((void *)interface_slot[u].ph_name);
        free((void *)interface_slot[u].br_name);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    return 0;
#else
#warning '/agent/xen/interface' 'add' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'add' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Delete XEN virtual tested interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the XEN virtual tested interface to delete
 *
 * @return              Status code
 */
static te_errno
xen_interface_del(unsigned int gid, char const *oid, char const *xen,
                  char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to delete XEN virtual tested interface: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    FIND_INTERFACE(interface, u);

    free((void *)interface_slot[u].br_name);
    free((void *)interface_slot[u].ph_name);
    free((void *)interface_slot[u].if_name);
    interface_slot[u].if_name = NULL;
    return 0;
#else
#warning '/agent/xen/interface' 'del' i\
access method is not implemented
    ERROR("'/agent/xen/interface' 'del' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * List XEN virtual tested interfaces.
 *
 * @param gid           group identifier (unused)
 * @param oid           full parent object instance identifier (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          address of a pointer to storage allocated
 *                      for the list pointer is initialized with
 *
 * @return              Status code
 */
static te_errno
xen_interface_list(unsigned int gid, char const *oid,
                   const char *sub_id, char **list)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = interface_limit();
    unsigned int len = 0;
    char        *ptr;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#if XEN_SUPPORT
    /* Count the whole length of interface names plus one per name */
    for (u = 0; u < limit; u++)
        if (interface_slot[u].if_name != NULL)
            len += strlen(interface_slot[u].if_name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    ptr = TE_ALLOC(len);

    if (list != NULL)
        *(*list = ptr) = '\0';

    /**
     * Fill in the list with existing domU names
     * separated with spaces except the last one
     */
    for (u = 0; u < limit; u++)
    {
        char const *name = interface_slot[u].if_name;

        if (name != NULL)
        {
            size_t len = strlen(name);

            if (ptr != *list)
                *ptr++ = ' ';

            memcpy(ptr, name, len);
            *(ptr += len) = '\0';
        }
    }

    return 0;
#else
#warning '/agent/xen/interface' 'list' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'list' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get the name of the XEN bridge, which
 * virtual tested interface is connected to.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for XEN bridge name to be filled in
 * @param interface     virtual tested interface name
 *
 * @return              Status code
 */
static te_errno
xen_interface_bridge_get(unsigned int gid, char const *oid, char *value,
                         char const *xen, char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_INTERFACE(interface, u);

    strcpy(value, interface_slot[u].br_name);
    return 0;
#else
#warning '/agent/xen/interface/bridge' 'get' \
access method is not implemented
    ERROR("'/agent/xen/interface/bridge' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set the name of the XEN bridge, which
 * virtual tested interface is connected to.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         XEN bridge name to set
 * @param interface     virtual tested interface name
 *
 * @return              Status code
 */
static te_errno
xen_interface_bridge_set(unsigned int gid, char const *oid,
                         char const *value, char const *xen,
                         char const *interface)
{
#if XEN_SUPPORT
    char const  *br_name;
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN bridge name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    FIND_INTERFACE(interface, u);

    if ((br_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free((void *)interface_slot[u].br_name);
    interface_slot[u].br_name = br_name;
    return 0;
#else
#warning '/agent/xen/interface/bridge' 'set' \
access method is not implemented
    UNUSED(value);
    UNUSED(interface);
    ERROR("'/agent/xen/interface/bridge' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get presence of directory/images state of domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_get(unsigned int gid, char const *oid, char *value,
          char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    struct stat  st;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, stat(get_dom_u_path(dom_u), &st) == 0 ? "1" : "0");
    return 0;
#else
#warning '/agent/xen/dom_u' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'get' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) presence of directory/images state of domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_set(unsigned int gid, char const *oid, char const *value,
           char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    struct stat  st;
    int          sys;
    bool to_set = strcmp(value, "1") == 0;
    bool is_set;
    te_errno     rc = 0;

    char const *const dom_u_path = get_dom_u_path(dom_u);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    is_set = stat(dom_u_path, &st) == 0;

    /* If desired state is already exists, do nothing */
    if ((is_set && to_set) || (!is_set && !to_set))
        return 0;

    /* If not to set then remove domU directory and disk images */
    if (!to_set)
        goto cleanup1;

    /* Otherwise, create domU directory and all necessary images */
    if (mkdir(dom_u_path, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to create domU directory %s", dom_u_path);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup0;
    }

    if (chmod(dom_u_path, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to chmod domU directory %s", dom_u_path);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "cp --sparse=always %s/%s %s/%s",
               xen_path, xen_dsktpl, dom_u_path, xen_dskimg);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buf, "%s/%s", dom_u_path, xen_dskimg);

    if (chmod(buf, S_IRUSR | S_IWUSR |
                   S_IRGRP | S_IWGRP |
                   S_IROTH | S_IWOTH) == -1)
    {
        ERROR("Failed to chmod domU disk image %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "dd if=/dev/zero of=%s/%s bs=1k seek=131071 "
               "count=1 2>/dev/null", dom_u_path, xen_swpimg);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buf, "%s/%s", dom_u_path, xen_swpimg);

    if (chmod(buf, S_IRUSR | S_IWUSR |
                   S_IRGRP | S_IWGRP |
                   S_IROTH | S_IWOTH) == -1)
    {
        ERROR("Failed to chmod domU swap image %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "/sbin/mkswap %s/%s > /dev/null",
               dom_u_path, xen_swpimg);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    if ((rc = xen_fill_file_in_disk_image(dom_u, "/etc/udev/rules.d/"
                                          "z25_persistent-net.rules",
                                          "")) == 0)
    {
        goto cleanup0;
    }

cleanup1:
    /* Erase domU directory and disk images unconditionally */
    xen_rmfr(dom_u_path);

cleanup0:
    return rc;
#else
#warning '/agent/xen/dom_u' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'set' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Add new domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         initializing value (not used)
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to add
 *
 * @return              Status code
 */
static te_errno
dom_u_add(unsigned int gid, char const *oid, char const *value,
          char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = dom_u_limit();
    te_errno     rc    = 0;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (*xen_path == '\0')
    {
        ERROR("Failed to add '%s' domU since XEN path is not set", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (*dom_u == '\0')
    {
        ERROR("Failed to add '%s' domU: domU name is empty", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    if ((u = find_dom_u(dom_u)) < dom_u_limit())
    {
        ERROR("Failed to add domU %s: it already exists", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    /* Find an empty slot */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name == NULL)
            break;

    /* If an empty slot is NOT found */
    if (u == limit)
    {
        ERROR("Failed to add domU %s: all domU slots are taken", dom_u);
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    if ((dom_u_slot[u].name = strdup(dom_u)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    /* Assign here initial values (modified later from within TAPI) */
    dom_u_slot[u].status = DOM_U_STATUS_NON_RUNNING;
    dom_u_slot[u].memory = 0;

    rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);

    strcpy(dom_u_slot[u].ip_addr, init_ip_addr);
    strcpy(dom_u_slot[u].mac_addr, init_mac_addr);

    for (v = 0, limit = bridge_limit(); v < limit; v++)
        dom_u_slot[u].bridge_slot[v].br_name = NULL;

    dom_u_slot[u].migrate_kind = 0;

    /* Try to set requested presence of directory/images state of domU */
    if ((rc = dom_u_set(gid, oid, value, xen, dom_u)) == 0)
        return 0;

    free((void *)dom_u_slot[u].name);
    dom_u_slot[u].name = NULL;
    return rc;
#else
#warning '/agent/xen/dom_u' 'add' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'add' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Delete domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to delete
 *
 * @return              Status code
 */
static te_errno
dom_u_del(unsigned int gid, char const *oid, char const *xen,
          char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = bridge_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    for (v = 0; v < limit; v++)
    {
        if (dom_u_slot[u].bridge_slot[v].br_name != NULL)
        {
            free((void *)dom_u_slot[u].bridge_slot[v].if_name);
            free((void *)dom_u_slot[u].bridge_slot[v].br_name);
        }
    }

    free((void *)dom_u_slot[u].name);
    dom_u_slot[u].name = NULL;
    return 0;
#else
#warning '/agent/xen/dom_u' 'del' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'del' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * List domUs.
 *
 * @param gid           group identifier (unused)
 * @param oid           full parent object instance identifier (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          address of a pointer to storage allocated
 *                      for the list pointer is initialized with
 *
 * @return              Status code
 */
static te_errno
dom_u_list(unsigned int gid, char const *oid,
           const char *sub_id, char **list)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    unsigned int len = 0;
    char        *ptr;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#if XEN_SUPPORT
    /* Count the whole length of domU names plus one per name */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
            len += strlen(dom_u_slot[u].name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    ptr = TE_ALLOC(len);

    if (list != NULL)
        *(*list = ptr) = '\0';

    /**
     * Fill in the list with existing domU names
     * separated with spaces except the last one
     */
    for (u = 0; u < limit; u++)
    {
        char const *name = dom_u_slot[u].name;

        if (name != NULL)
        {
            size_t len = strlen(name);

            if (ptr != *list)
                *ptr++ = ' ';

            memcpy(ptr, name, len);
            *(ptr += len) = '\0';
        }
    }

    return 0;
#else
#warning '/agent/xen/dom_u' 'list' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'list' access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU status.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_status_get(unsigned int gid, char const *oid, char *value,
                 char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    char const  *s;
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if ((s = dom_u_status_to_string(dom_u_slot[u].status)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strcpy(value, s);
    return 0;
#else
#warning '/agent/xen/dom_u/status' 'get' access method is not implemented
     ERROR("'/agent/xen/dom_u/status' 'get' access method is not " \
           "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU status; business logic is moved to TAPI.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_status_set(unsigned int gid, char const *oid, char const *value,
                 char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    FILE        *f;
    status_t     status = dom_u_status_string_to_status(value);
    te_errno     rc     = 0;

    char const *const dom_u_path = get_dom_u_path(dom_u);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    if (status == DOM_U_STATUS_ERROR)
    {
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    FIND_DOM_U(dom_u, u);

    /* If nothing to do */
    if ((dom_u_slot[u].status == status))
        goto cleanup0;

    /* "Non-running" -> "<another status>" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        (rc = check_dom_u_is_initialized_properly(u)) != 0)
    {
        goto cleanup0;
    }

    /* "Non-running" -> "migrated-saved" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        status == DOM_U_STATUS_MIGRATED_SAVED)
    {
        struct stat st;

        TE_SPRINTF(buf, "%s/%s", dom_u_path, xen_dskimg);

        if (stat(buf, &st) != 0 || !S_ISREG(st.st_mode))
        {
            ERROR("Failed to accept migrated saved '%s' domU", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Non-running" -> "migrated-running" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        status == DOM_U_STATUS_MIGRATED_RUNNING)
    {
        size_t  len = strlen(dom_u);

        /* FIXME: Non "popen" implementation is needed*/
        TE_SPRINTF(buf, "xm list | awk '{print$1}' 2>/dev/null");

        if ((f = popen(buf, "r")) == NULL)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("popen(%s) failed with errno %d", buf, rc);
            goto cleanup0;
        }

        while (fgets(buf, sizeof(buf), f) != NULL)
        {
            if (strncmp(buf, dom_u, len) == 0)
            {
                dom_u_slot[u].status = DOM_U_STATUS_MIGRATED_RUNNING;
                break;
            }
        }

        pclose(f);

        if (dom_u_slot[u].status != DOM_U_STATUS_MIGRATED_RUNNING)
        {
            ERROR("Failed to accept migrated running '%s' domU", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Non-running" -> "running" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        status == DOM_U_STATUS_RUNNING)
    {
        /* IP address must be set for domU */
        if (strcmp(dom_u_slot[u].ip_addr, "0.0.0.0") == 0)
        {
            ERROR("DomU %s IP address is not set", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* Memory size must be set for domU */
        if (dom_u_slot[u].memory == 0)
        {
            ERROR("DomU %s memory size is not set", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* Create XEN domU configuration file */
        TE_SPRINTF(buf, "%s/conf.cfg", dom_u_path);

        if ((f = fopen(buf, "w")) == NULL)
        {
            ERROR("Failed to (re)create domU %s configuration file %s",
                  dom_u, buf);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        if (fprintf(f, "kernel='%s/%s'\n", xen_path, xen_kernel)  < 0 ||
            fprintf(f, "ramdisk='%s/%s'\n", xen_path, xen_initrd) < 0 ||
            fprintf(f, "memory='%u'\n", dom_u_slot[u].memory)     < 0 ||
            fprintf(f, "root='/dev/sda1 ro'\n")                   < 0 ||
            fprintf(f, "disk=[ 'file:%s/%s,sda1,w', "
                       "'file:%s/%s,sda2,w' ]\n",
                       dom_u_path, xen_dskimg,
                       dom_u_path, xen_swpimg)               < 0 ||
            fprintf(f, "name='%s'\n", dom_u)                      < 0 ||
            (rc = prepare_dom_u_interfaces_config(u)) != 0 ||
            fprintf(f, "%s", buf) < 0 ||
            fprintf(f, "on_poweroff = 'destroy'\n") < 0 ||
            fprintf(f, "on_reboot   = 'restart'\n") < 0 ||
            fprintf(f, "on_crash    = 'restart'\n") < 0 ||
            fflush(f) != 0)
        {
            if (rc == 0)
                rc = TE_RC(TE_TA_UNIX, TE_EFAIL);

            goto cleanup2;
        }

cleanup2:
        if (fclose(f) != 0)
        {
            if (rc == 0)
                rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        }

        if (rc != 0)
            goto cleanup0;

        /* Create list of interfaces */
        if ((rc = prepare_persistent_net_rules(u)) != 0 ||
            (rc = xen_fill_file_in_disk_image(dom_u, "/etc/udev/rules.d/"
                                              "z25_persistent-net.rules",
                                              buf)) != 0)
        {
            goto cleanup0;
        }

        /* Create domU "/etc/network/interfaces" file */
        if ((rc = prepare_network_interfaces_config(u)) != 0 ||
            (rc = xen_fill_file_in_disk_image(dom_u,
                                              "/etc/network/interfaces",
                                              buf)) != 0)
        {
            goto cleanup0;
        }

        /* Starting domU */
        TE_SPRINTF(buf, "xm create %s/conf.cfg", dom_u_path);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* Really domU is stil booting here */
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Running/migrated-running" -> "non-running" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_RUNNING ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_RUNNING) &&
        status == DOM_U_STATUS_NON_RUNNING)
    {
        TE_SPRINTF(buf, "xm shutdown %s", dom_u);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(15);
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Running/migrated-running" -> "saved" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_RUNNING ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_RUNNING) &&
        status == DOM_U_STATUS_SAVED)
    {
        TE_SPRINTF(buf, "xm save %s %s/saved.img", dom_u, dom_u_path);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(10);
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Saved/migrated-saved" -> "running" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_SAVED ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_SAVED) &&
        status == DOM_U_STATUS_RUNNING)
    {
        TE_SPRINTF(buf, "xm restore %s/saved.img", dom_u_path);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(25);
        goto entry1; /** Imitation of absence of the 'break' statement */
    }

    /* "Saved/migrated-saved" -> "non-running" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_SAVED ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_SAVED) &&
        status == DOM_U_STATUS_NON_RUNNING)
    {
entry1:
        TE_SPRINTF(buf, "%s/saved.img", dom_u_path);

        /* Error here is not critical */
        if (unlink(buf) == -1)
            ERROR("Failed to unlink %s/saved.img", dom_u_path);

        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* All still unserviced transitions are erroneous */
    rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
    goto cleanup0;

cleanup1: /** This label is used in case of success */
    dom_u_slot[u].status = status;

cleanup0: /** This label is used in case of an error */
    return rc;
#else
#warning '/agent/xen/dom_u_status' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u_status' 'set' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU memory size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for memory size to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get memory size of
 *
 * @return              Status code
 */
static te_errno
dom_u_memory_get(unsigned int gid, char const *oid, char *value,
                 char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    sprintf(value, "%u", dom_u_slot[u].memory);
    return 0;
#else
#warning '/agent/xen/dom_u/memory' 'get' access method is not implemented
     ERROR("'/agent/xen/dom_u/memory' 'get' access method is not " \
           "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU memory size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         memory size to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set memory size of
 *
 * @return              Status code
 */
static te_errno
dom_u_memory_set(unsigned int gid, char const *oid, char const *value,
                 char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    int          mem = atoi(value);
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if (mem < 0)
    {
        ERROR("Invalid memory size value = %d", mem);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    dom_u_slot[u].memory = mem;
    return 0;
#else
#warning '/agent/xen/dom_u/memory' 'get' access method is not implemented
     ERROR("'/agent/xen/dom_u/memory' 'get' access method is not " \
           "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU IP address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for IP address to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get IP address of
 *
 * @return              Status code
 */
static te_errno
dom_u_ip_addr_get(unsigned int gid, char const *oid, char *value,
                  char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].ip_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/ip_addr' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u/ip_addr' 'get' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU IP address (possible only in non-running state).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_ip_addr_set(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if (len >= sizeof(dom_u_slot[u].ip_addr))
    {
        ERROR("Too long IP address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * IP address will be really changed in domU disk image
     * only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].ip_addr, value);
    return 0;
#else
#warning '/agent/xen/dom_u/ip_addr' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/ip_addr' 'set' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU MAC address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_mac_addr_get(unsigned int gid, char const *oid, char *value,
                   char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].mac_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/mac_addr' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u/mac_addr' 'get' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU MAC address (possible only in non-running state).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_mac_addr_set(unsigned int gid, char const *oid, char const *value,
                   char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    enum { ether_bytes = 6 };

    unsigned int u;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if (len >= sizeof(dom_u_slot[u].mac_addr))
    {
        ERROR("Too long MAC address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * MAC address will be really changed in domU configuration
     * file only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].mac_addr, value);
    return 0;
#else
#warning '/agent/xen/dom_u/mac_addr' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/mac_addr' 'set' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get presence of directory/images state of domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_get(unsigned int gid, char const *oid,
                 char *value, char const *xen,
                 char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].if_name);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) presence of directory/images state of domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_set(unsigned int gid, char const *oid,
                 char const *value, char const *xen,
                 char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    char const  *if_name;
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    if ((if_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free((void *)dom_u_slot[u].bridge_slot[v].if_name);
    dom_u_slot[u].bridge_slot[v].if_name = if_name;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Add new domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         initializing value (not used)
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to add
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_add(unsigned int gid, char const *oid,
                 char const *value, char const *xen,
                 char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = bridge_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if ((v = find_bridge(bridge, u)) < limit)
    {
        ERROR("Failed to add '%s' bridge on '%s' domU: it already "
              "exists", bridge, dom_u);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    /* Find an empty slot */
    for (v = 0; v < limit; v++)
        if (dom_u_slot[u].bridge_slot[v].br_name == NULL)
            break;

    /* If an empty slot is NOT found */
    if (v == limit)
    {
        ERROR("Failed to add '%s' bridge on '%s' domU: all bridge "
              "slots are taken", bridge, dom_u);
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    if ((dom_u_slot[u].bridge_slot[v].if_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((dom_u_slot[u].bridge_slot[v].br_name = strdup(bridge)) == NULL)
    {
        free((void *)dom_u_slot[u].bridge_slot[v].if_name);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    strcpy(dom_u_slot[u].bridge_slot[v].ip_addr, init_ip_addr);
    strcpy(dom_u_slot[u].bridge_slot[v].mac_addr, init_mac_addr);
    dom_u_slot[u].bridge_slot[v].accel = false;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'add' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'add' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Delete domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to delete
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_del(unsigned int gid, char const *oid,
                 char const *xen, char const *dom_u,
                 char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    free((void *)dom_u_slot[u].bridge_slot[v].if_name);
    free((void *)dom_u_slot[u].bridge_slot[v].br_name);
    dom_u_slot[u].bridge_slot[v].br_name = NULL;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'del' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'del' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * List domUs.
 *
 * @param gid           group identifier (unused)
 * @param oid           full parent object instance identifier (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          address of a pointer to storage allocated
 *                      for the list pointer is initialized with
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_list(unsigned int gid, char const *oid,
                  const char *sub_id, char **list,
                  char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = bridge_limit();
    unsigned int len = 0;
    char        *ptr;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    /* Count the whole length of domU names plus one per name */
    for (v = 0; v < limit; v++)
        if (dom_u_slot[u].bridge_slot[v].br_name != NULL)
            len += strlen(dom_u_slot[u].bridge_slot[v].br_name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    ptr = TE_ALLOC(len);

    if (list != NULL)
        *(*list = ptr) = '\0';

    /**
     * Fill in the list with existing domU names
     * separated with spaces except the last one
     */
    for (v = 0; v < limit; v++)
    {
        char const *br_name = dom_u_slot[u].bridge_slot[v].br_name;

        if (br_name != NULL)
        {
            size_t len = strlen(br_name);

            if (ptr != *list)
                *ptr++ = ' ';

            memcpy(ptr, br_name, len);
            *(ptr += len) = '\0';
        }
    }

    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'list' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'list' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU IP address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for IP address to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get IP address of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_ip_addr_get(unsigned int gid, char const *oid,
                         char *value, char const *xen,
                         char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].ip_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/ip_addr' 'get' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/ip_addr' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU IP address (possible only in non-running state).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_ip_addr_set(unsigned int gid, char const *oid,
                         char const *value, char const *xen,
                         char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    if (len >= sizeof(dom_u_slot[u].bridge_slot[v].ip_addr))
    {
        ERROR("Too long IP address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * IP address will be really changed in domU disk image
     * only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].bridge_slot[v].ip_addr, value);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/ip_addr' 'set' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/ip_addr' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU MAC address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_mac_addr_get(unsigned int gid, char const *oid,
                          char *value, char const *xen,
                          char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].mac_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/mac_addr' 'get' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/mac_addr' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU MAC address (possible only in non-running state).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_mac_addr_set(unsigned int gid, char const *oid,
                          char const *value, char const *xen,
                          char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    if (len >= sizeof(dom_u_slot[u].bridge_slot[v].mac_addr))
    {
        ERROR("Too long MAC address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * MAC address will be really changed in domU configuration
     * file only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].bridge_slot[v].mac_addr, value);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/mac_addr' 'set' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/mac_addr' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU acceleration sign.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for acceleration sign to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_accel_get(unsigned int gid, char const *oid,
                       char *value, char const *xen,
                       char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].accel ? "1" : "0");
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/accel' 'get' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/accel' 'get' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU MAC address (possible only in non-running state).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_accel_set(unsigned int gid, char const *oid,
                       char const *value, char const *xen,
                       char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    dom_u_slot[u].bridge_slot[v].accel = *value != '0' ? true : false;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/accel' 'set' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/accel' 'set' "
          "access method is not implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Migrate to another XEN dom0.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         name of the agent running within XEN dom0
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to migrate
 *
 * @return              Status code
 */
static te_errno
dom_u_migrate_set(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    TE_SPRINTF(buf, "xm migrate %s %s %s",
               dom_u_slot[u].migrate_kind ? "--live" : "", dom_u, value);

    if (ta_system(buf) != 0)
    {
        ERROR("Failed to migrate domU %s", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    return 0;
#else
#warning '/agent/xen/dom_u/migrate' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/migrate' 'set' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Migrate to another XEN dom0.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         name of the agent running within XEN dom0
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to migrate
 *
 * @return              Status code
 */
static te_errno
dom_u_migrate_kind_get(unsigned int gid, char const *oid, char *value,
                       char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].migrate_kind ? "1" : "0");
    return 0;
#else
#warning '/agent/xen/dom_u/migrate/kind' 'get' access method is not \
         implemented
    ERROR("'/agent/xen/dom_u/migrate/kind' 'get' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Migrate to another XEN dom0.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         name of the agent running within XEN dom0
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to migrate
 *
 * @return              Status code
 */
static te_errno
dom_u_migrate_kind_set(unsigned int gid, char const *oid,
                       char const *value, char const *xen,
                       char const *dom_u)
{
#if XEN_SUPPORT
    unsigned u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    dom_u_slot[u].migrate_kind = (strcmp(value, "0") == 0 ? 0 : 1);
    return 0;
#else
#warning '/agent/xen/dom_u/migrate/kind' 'set' access method is not \
         implemented
    ERROR("'/agent/xen/dom_u/migrate/kind' 'set' access method is not " \
          "implemented");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}
