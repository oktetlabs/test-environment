/** @file
 * @brief Unix Test Agent
 *
 * Unix TA Open vSwitch deployment.
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER "TA Unix OVS"

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "te_alloc.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_ethernet.h"
#include "te_shell_cmd.h"
#include "te_sleep.h"
#include "te_str.h"
#include "te_string.h"
#include "te_kvpair.h"

#include "agentlib.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "unix_internal.h"

#include "conf_common.h"
#include "conf_ovs.h"

#define OVS_SLEEP_MS_MAX 256

typedef struct log_module_s {
    char *name;
    char *level;
} log_module_t;

typedef enum interface_tunnel_option {
    INTERFACE_TUNNEL_REMOTE_IP = 0,
    INTERFACE_TUNNEL_LOCAL_IP,
    INTERFACE_TUNNEL_IN_KEY,
    INTERFACE_TUNNEL_OUT_KEY,
    INTERFACE_TUNNEL_DST_PORT,

    INTERFACE_TUNNEL_END,
} interface_tunnel_option;

static const char * interface_tunnel_option_names[] = {
    [INTERFACE_TUNNEL_REMOTE_IP] = "remote_ip",
    [INTERFACE_TUNNEL_LOCAL_IP] = "local_ip",
    [INTERFACE_TUNNEL_IN_KEY] = "in_key",
    [INTERFACE_TUNNEL_OUT_KEY] = "out_key",
    [INTERFACE_TUNNEL_DST_PORT] = "dst_port",
};

typedef struct interface_entry {
    /* Links in ovs interfaces list */
    SLIST_ENTRY(interface_entry)  links;
    /* Links in port interfaces list */
    SLIST_ENTRY(interface_entry)  port_links;
    char                         *name;
    char                         *type;
    te_bool                       temporary;
    te_bool                       active;

    te_bool                       mac_requested;
    char                         *mac_request;

    te_bool                       ofport_requested;
    char                         *ofport_request;

    te_bool                       mtu_requested;
    char                         *mtu_request;

    char                         *dpdk_devargs;
    char                         *vhost_server_path;
    char                         *tunnel_options[INTERFACE_TUNNEL_END];
} interface_entry;

typedef SLIST_HEAD(interface_list_t, interface_entry) interface_list_t;

#define PORT_MAX_TRUNKS 4096

typedef struct port_vlan {
    char *type;
    te_bool trunks[PORT_MAX_TRUNKS];
    int tag;
} port_vlan;

typedef struct port_entry {
    SLIST_ENTRY(port_entry)  links;
    char                    *name;
    char                    *value;
    interface_list_t         interfaces;
    te_bool                  bridge_local;
    const char              *parent_bridge_name;
    port_vlan                vlan;
} port_entry;

typedef SLIST_HEAD(port_list_t, port_entry) port_list_t;

typedef struct bridge_entry {
    SLIST_ENTRY(bridge_entry)  links;
    char                      *datapath_type;
    interface_entry           *interface;
    port_list_t                ports;
} bridge_entry;

typedef SLIST_HEAD(bridge_list_t, bridge_entry) bridge_list_t;

typedef struct ovs_ctx_s {
    te_bool           dpdk_init;
    te_kvpair_h       other_cfg;

    te_string         root_path;
    te_string         conf_db_lock_path;
    te_string         conf_db_schema;
    te_string         conf_db_path;
    te_string         env;

    te_string         dbtool_cmd;
    te_string         dbserver_cmd;
    te_string         vswitchd_cmd;
    te_string         vlog_list_cmd;
    te_string         ovs_cfg_cmd;

    pid_t             dbserver_pid;
    pid_t             vswitchd_pid;

    unsigned int      nb_log_modules;
    log_module_t     *log_modules;

    interface_list_t  interfaces;
    bridge_list_t     bridges;
} ovs_ctx_t;

static ovs_ctx_t ovs_ctx = {
    .dpdk_init = FALSE,
    .other_cfg = TAILQ_HEAD_INITIALIZER(ovs_ctx.other_cfg),

    .root_path = TE_STRING_INIT,
    .conf_db_lock_path = TE_STRING_INIT,
    .conf_db_schema = TE_STRING_INIT,
    .conf_db_path = TE_STRING_INIT,
    .env =TE_STRING_INIT,

    .dbtool_cmd = TE_STRING_INIT,
    .dbserver_cmd = TE_STRING_INIT,
    .vswitchd_cmd = TE_STRING_INIT,
    .vlog_list_cmd = TE_STRING_INIT,
    .ovs_cfg_cmd = TE_STRING_INIT,

    .dbserver_pid = -1,
    .vswitchd_pid = -1,

    .nb_log_modules = 0,
    .log_modules = NULL,

    .interfaces = SLIST_HEAD_INITIALIZER(interfaces),
    .bridges = SLIST_HEAD_INITIALIZER(bridges),
};

static const char *log_levels[] = { "EMER", "ERR", "WARN",
                                    "INFO", "DBG", NULL };

static const char *interface_types[] = { "system", "internal", "dpdk",
                                         "dpdkvhostuserclient", NULL };

static const char *interface_tunnel_types[] = {"gre", "vxlan", "geneve", NULL};

static const char *bridge_datapath_types[] = { "system", "netdev", NULL };

static const char *other_config_types[] = {
    "dpdk-alloc-mem", "dpdk-socket-mem", "dpdk-lcore-mask",
    "dpdk-hugepage-dir", "dpdk-socket-limit", "dpdk-extra", "hw-offload",
    NULL
};

static const char *vlan_types[] = { "trunk", "access", "dot1q-tunnel",
    "native-tagged", "native-untagged", NULL
};

static te_errno
ovs_interface_tunnel_option_by_name(const char *name,
                                    interface_tunnel_option *option)
{
    interface_tunnel_option result;

    for (result = 0; result < INTERFACE_TUNNEL_END; result++)
    {
        if (strcmp(interface_tunnel_option_names[result], name) == 0)
        {
            *option = result;
            return 0;
        }
    }

    ERROR("Failed to find tunnel option with name '%s'", name);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

static ovs_ctx_t *
ovs_ctx_get(const char *ovs)
{
    return (ovs[0] == '\0') ? &ovs_ctx : NULL;
}

static te_bool
ovs_value_is_valid(const char **supported_values,
                   const char  *test_value)
{
    char **valuep = (char **)supported_values;

    for (; *valuep != NULL; ++valuep)
    {
        if (strcmp(test_value, *valuep) == 0)
            return TRUE;
    }

    return FALSE;
}

static te_bool
ovs_value_is_valid_mac(const char *value)
{
    unsigned int x[ETHER_ADDR_LEN];
    char         c;
    int          n;

    n = sscanf(value, "%02x:%02x:%02x:%02x:%02x:%02x%c",
               &x[0], &x[1], &x[2], &x[3], &x[4], &x[5], &c);

    return (n == ETHER_ADDR_LEN) ? TRUE : FALSE;
}

static te_errno
ovs_value_get_effective(ovs_ctx_t   *ctx,
                        const char  *facility_name,
                        const char  *instance_name,
                        const char  *property_name,
                        char       **bufp)
{
    te_string  cmd = TE_STRING_INIT;
    int        out_fd = -1;
    FILE      *f = NULL;
    int        ret;
    te_errno   rc;

    INFO("Querying the effective '%s' property value of the %s '%s'",
         property_name, facility_name, instance_name);

    rc = te_string_append(&cmd, "%s ovs-vsctl get %s %s %s", ctx->env.ptr,
                          facility_name, instance_name, property_name);
    if (rc != 0)
    {
        ERROR("Failed to construct ovs-vsctl invocation command line");
        return rc;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, &out_fd, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-vsctl");
        rc = TE_ECHILD;
        goto out;
    }

    f = fdopen(out_fd, "r");
    if (f == NULL)
    {
        ERROR("Failed to access ovs-vsctl output");
        rc = TE_EBADF;
        goto out;
    }

    if (fscanf(f, "%m[^\n]", bufp) == EOF)
    {
        int error_code_set = ferror(f);

        ERROR("Failed to read out ovs-vsctl output");
        rc = (error_code_set != 0) ? te_rc_os2te(errno) : TE_ENODATA;
    }

out:
    if (f != NULL)
        fclose(f);
    else if (out_fd != -1)
        close(out_fd);

    if (ret != -1)
        ta_waitpid(ret, NULL, 0);

    te_string_free(&cmd);

    return rc;
}

static te_errno
ovs_log_get_nb_modules(ovs_ctx_t    *ctx,
                       unsigned int *nb_modules_out)
{
    unsigned int  nb_modules = 0;
    int           out_fd;
    te_errno      rc = 0;
    int           ret;
    FILE         *f;

    INFO("Querying the number of log modules");

    ret = te_shell_cmd(ctx->vlog_list_cmd.ptr, -1, NULL, &out_fd, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        return TE_ECHILD;
    }

    f = fdopen(out_fd, "r");
    if (f == NULL)
    {
        ERROR("Failed to access ovs-appctl output");
        rc = TE_EBADF;
        goto out;
    }

    while (fscanf(f, "%*[^\n]\n") != EOF)
        ++nb_modules;

    *nb_modules_out = nb_modules;

out:
    if (f != NULL)
        fclose(f);
    else
        close(out_fd);

    ta_waitpid(ret, NULL, 0);

    return rc;
}

static te_errno
ovs_log_init_modules(ovs_ctx_t *ctx)
{
    unsigned int  nb_modules = 0;
    log_module_t *modules = NULL;
    unsigned int  entry_idx = 0;
    int           out_fd = -1;
    FILE         *f = NULL;
    int           ret;
    te_errno      rc;

    INFO("Constructing log module context");

    assert(ctx->nb_log_modules == 0);
    assert(ctx->log_modules == NULL);

    rc = ovs_log_get_nb_modules(ctx, &nb_modules);
    if (rc != 0)
    {
        ERROR("Failed to query the number of log modules");
        return rc;
    }

    modules = TE_ALLOC(nb_modules * sizeof(*modules));
    if (modules == NULL)
    {
        ERROR("Failed to allocate log module context storage");
        return TE_ENOMEM;
    }

    ret = te_shell_cmd(ctx->vlog_list_cmd.ptr, -1, NULL, &out_fd, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        rc = TE_ECHILD;
        goto out;
    }

    f = fdopen(out_fd, "r");
    if (f == NULL)
    {
        ERROR("Failed to access ovs-appctl output");
        rc = TE_EBADF;
        goto out;
    }

    while (entry_idx < nb_modules)
    {
        if (fscanf(f, "%ms %ms %*[^\n]\n", &modules[entry_idx].name,
                   &modules[entry_idx].level) == EOF)
        {
            int error_code_set = ferror(f);

            ERROR("Failed to read entry no. %u from ovs-appctl output",
                  entry_idx);
            rc = (error_code_set != 0) ? te_rc_os2te(errno) : TE_ENODATA;
            goto out;
        }

        ++entry_idx;
    }

    ctx->nb_log_modules = nb_modules;
    ctx->log_modules = modules;

out:
    if (rc != 0)
    {
        unsigned int i;

        for (i = 0; i < entry_idx; ++i)
        {
            free(modules[i].level);
            free(modules[i].name);
        }
    }

    if (f != NULL)
        fclose(f);
    else if (out_fd != -1)
        close(out_fd);

    if (ret != -1)
        ta_waitpid(ret, NULL, 0);

    if (rc != 0)
        free(modules);

    return rc;
}

static void
ovs_log_fini_modules(ovs_ctx_t *ctx)
{
    unsigned int i;

    INFO("Dismantling log module context");

    for (i = 0; i < ctx->nb_log_modules; ++i)
    {
        free(ctx->log_modules[i].level);
        free(ctx->log_modules[i].name);
    }

    free(ctx->log_modules);
    ctx->log_modules = NULL;
    ctx->nb_log_modules = 0;
}

static interface_entry *
ovs_interface_alloc(const char *name,
                    const char *type,
                    te_bool     temporary)
{
    interface_entry *interface;

    INFO("Allocating the interface list entry for '%s'", name);

    interface = TE_ALLOC(sizeof(*interface));
    if (interface == NULL)
    {
        ERROR("Failed to allocate memory for the interface context");
        return NULL;
    }

    interface->name = strdup(name);
    if (interface->name == NULL)
    {
        ERROR("Failed to allocate memory for the interface name");
        goto fail;
    }

    interface->type = (type != NULL && type[0] != '\0') ? strdup(type) :
                                                          strdup("system");
    if (interface->type == NULL)
    {
        ERROR("Failed to allocate memory for the interface type");
        goto fail;
    }

    interface->mac_requested = FALSE;
    interface->ofport_requested = FALSE;
    interface->ofport_request = strdup("0");
    if (interface->ofport_request == NULL)
    {
        ERROR("Failed to allocate the interface ofport empty request");
        goto fail;
    }

    interface->mtu_requested = FALSE;
    interface->mtu_request = strdup("0");
    if (interface->mtu_request == NULL)
    {
        ERROR("Failed to allocate the interface MTU empty request");
        goto fail;
    }

    interface->temporary = temporary;
    interface->active = FALSE;

    return interface;

fail:
    free(interface->ofport_request);
    free(interface->type);
    free(interface->name);
    free(interface);

    return NULL;
}

static void
ovs_interface_free(interface_entry *interface)
{
    unsigned int i;

    INFO("Freeing the interface list entry for '%s'", interface->name);

    for (i = 0; i < INTERFACE_TUNNEL_END; i++)
        free(interface->tunnel_options[i]);

    free(interface->vhost_server_path);
    free(interface->dpdk_devargs);
    free(interface->mtu_request);
    free(interface->ofport_request);
    free(interface->mac_request);
    free(interface->type);
    free(interface->name);
    free(interface);
}

static interface_entry *
ovs_interface_find(ovs_ctx_t  *ctx,
                   const char *name)
{
    interface_entry *interface;

    SLIST_FOREACH(interface, &ctx->interfaces, links)
    {
        if (strcmp(name, interface->name) == 0)
            return interface;
    }

    return NULL;
}

static port_entry *
ovs_bridge_port_find(bridge_entry *bridge,
                     const char   *port_name)
{
    port_entry *port;

    SLIST_FOREACH(port, &bridge->ports, links)
    {
        if (strcmp(port_name, port->name) == 0)
            return port;
    }

    return NULL;
}

static te_errno
ovs_interface_init(ovs_ctx_t        *ctx,
                   const char       *name,
                   const char       *type,
                   te_bool           activate,
                   interface_entry **interface_out)
{
    interface_entry *interface = NULL;

    INFO("Initialising the interface list entry for '%s'", name);

    if (activate)
        interface = ovs_interface_find(ctx, name);

    if (!activate || interface == NULL)
    {
        interface = ovs_interface_alloc(name, type, activate);
        if (interface == NULL)
        {
            ERROR("Failed to allocate the interface list entry");
            return TE_ENOMEM;
        }

        SLIST_INSERT_HEAD(&ctx->interfaces, interface, links);
    }

    if (interface->active)
    {
        ERROR("The interface is already in use");
        return TE_EBUSY;
    }

    interface->active = activate;

    if (interface_out != NULL)
        *interface_out = interface;

    return 0;
}

static void
ovs_interface_fini(ovs_ctx_t       *ctx,
                   interface_entry *interface)
{
    INFO("Finalising the interface list entry for '%s'", interface->name);

    if (interface->temporary == interface->active)
    {
        SLIST_REMOVE(&ctx->interfaces, interface, interface_entry, links);
        ovs_interface_free(interface);
    }
    else if (interface->active)
    {
        interface->active = FALSE;
    }
    else
    {
        assert(FALSE);
    }
}

static void
ovs_interface_fini_all(ovs_ctx_t *ctx)
{
    interface_entry *interface_tmp;
    interface_entry *interface;

    INFO("Finalising the interface list entries");

    SLIST_FOREACH_SAFE(interface, &ctx->interfaces, links, interface_tmp)
    {
        assert(!interface->active);
        ovs_interface_fini(ctx, interface);
    }
}

static bridge_entry *
ovs_bridge_find(ovs_ctx_t  *ctx,
                const char *name)
{
    bridge_entry *bridge;

    SLIST_FOREACH(bridge, &ctx->bridges, links)
    {
        if (strcmp(name, bridge->interface->name) == 0)
            return bridge;
    }

    return NULL;
}

static te_errno
ovs_interface_check_error(ovs_ctx_t        *ctx,
                          interface_entry  *interface,
                          te_bool          *error_setp,
                          char            **error_bufp)
{
    char     *buf = NULL;
    int       ret;
    te_errno  rc;

    INFO("Checking the interface '%s' for errors", interface->name);

    rc = ovs_value_get_effective(ctx, "interface", interface->name,
                                 "error", &buf);
    if (rc != 0)
    {
        ERROR("Failed to query the error property");
        return rc;
    }

    ret = sscanf(buf, "\"%m[^\"]", error_bufp);
    switch (ret)
    {
        case 0:
            *error_setp = FALSE;
            break;

        case 1:
            *error_setp = TRUE;
            break;

        default:
            ERROR("Failed to parse the response");
            rc = TE_ENODATA;
            break;
    }

    free(buf);

    return rc;
}

static te_errno
ovs_check_dpdk_init(ovs_ctx_t *ctx, const char *requestor)
{
    if (!ctx->dpdk_init)
    {
        ERROR("%s requires initialized DPDK", requestor);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

static te_errno
ovs_cmd_vsctl_append_if_dpdk(interface_entry   *interface,
                                    te_string  *cmdp,
                                    ovs_ctx_t  *ctx)
{
    te_errno rc;

    rc = ovs_check_dpdk_init(ctx, "dpdk interface");
    if (rc != 0)
        return rc;

    if (interface->dpdk_devargs == NULL)
    {
        ERROR("dpdk_devargs is required option for dpdk interface type");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = te_string_append(cmdp, " options:dpdk-devargs=");
    if (rc == 0)
        te_string_append_shell_arg_as_is(cmdp, interface->dpdk_devargs);

    return rc;
}

static te_errno
ovs_cmd_vsctl_append_if_dpdkvhostuserclient(interface_entry *interface,
                                            te_string       *cmdp,
                                            ovs_ctx_t       *ctx)
{
    te_errno rc;

    rc = ovs_check_dpdk_init(ctx, "dpdkvhostuserclient interface");
    if (rc != 0)
        return rc;

    if (interface->vhost_server_path == NULL)
    {
        ERROR("vhost_server_path is required for dpdkvhostuserclient interface");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = te_string_append(cmdp, " options:vhost-server-path=");
    if (rc == 0)
    {
        rc = te_string_append_shell_arg_as_is(cmdp,
                                              interface->vhost_server_path);
    }

    return rc;
}

static te_errno
ovs_cmd_vsctl_append_if_tunnel(interface_entry *interface,
                               te_string       *cmdp)
{
    unsigned int i;
    te_errno rc;

    if (interface->tunnel_options[INTERFACE_TUNNEL_REMOTE_IP] == NULL)
    {
        ERROR("remote IP is required for tunnel '%s' interface",
              interface->type);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    for (i = 0; i < INTERFACE_TUNNEL_END; i++)
    {
        if (interface->tunnel_options[i] == NULL)
            continue;

        rc = te_string_append(cmdp, " options:%s=%s",
                              interface_tunnel_option_names[i],
                              interface->tunnel_options[i]);
        if (rc != 0)
        {
            ERROR("Failed to append option '%s'", interface->tunnel_options[i]);
            return rc;
        }
    }

    return 0;
}

static te_errno
ovs_cmd_vsctl_append_interface_arguments(interface_entry *interface,
                                         te_string       *cmdp,
                                         ovs_ctx_t       *ctx)
{
    te_errno rc;

    INFO("Appending ovs-vsctl arguments for the interface '%s'",
         interface->name);

    rc = te_string_append(cmdp, " -- set interface %s type=%s",
                          interface->name, interface->type);
    if (rc != 0)
    {
        ERROR("Failed to append the type argument");
        return rc;
    }

    rc = 0;
    if (strcmp(interface->type, "dpdk") == 0)
        rc = ovs_cmd_vsctl_append_if_dpdk(interface, cmdp, ctx);
    else if (strcmp(interface->type, "dpdkvhostuserclient") == 0)
        rc = ovs_cmd_vsctl_append_if_dpdkvhostuserclient(interface, cmdp, ctx);
    else if (ovs_value_is_valid(interface_tunnel_types, interface->type))
        rc = ovs_cmd_vsctl_append_if_tunnel(interface, cmdp);

    if (rc != 0)
    {
        ERROR("Failed to append the interface type-specific arguments");
        return rc;
    }

    if (interface->temporary)
        return 0;

    if (interface->mac_requested)
    {
        rc = te_string_append(cmdp, " mac=\\\"%s\\\"", interface->mac_request);
        if (rc != 0)
        {
            ERROR("Failed to append the MAC request argument");
            return rc;
        }
    }

    if (interface->ofport_requested)
    {
        rc = te_string_append(cmdp, " ofport_request=%s",
                              interface->ofport_request);
        if (rc != 0)
        {
            ERROR("Failed to append the ofport request argument");
            return rc;
        }
    }

    if (interface->mtu_requested)
    {
        rc = te_string_append(cmdp, " mtu_request=%s", interface->mtu_request);
        if (rc != 0)
        {
            ERROR("Failed to append the MTU request argument");
            return rc;
        }
    }

    return 0;
}

static port_entry *
ovs_bridge_port_alloc(const char       *parent_bridge_name,
                      const char       *name,
                      const char       *value,
                      te_bool           bridge_local,
                      interface_entry **interfaces,
                      unsigned int      nb_interfaces)
{
    port_entry   *port;
    unsigned int  i;

    INFO("Allocating the port '%s' list entry", name);

    port = TE_ALLOC(sizeof(*port));
    if (port == NULL)
    {
        ERROR("Failed to allocate memory for the port context");
        return NULL;
    }

    port->name = strdup(name);
    if (port->name == NULL)
    {
        ERROR("Failed to allocate memory for the port name");
        goto fail;
    }

    port->value = strdup(value);
    if (port->value == NULL)
    {
        ERROR("Failed to allocate memory for the port value");
        goto fail;
    }

    SLIST_INIT(&port->interfaces);

    for (i = 0; i < nb_interfaces; ++i)
        SLIST_INSERT_HEAD(&port->interfaces, interfaces[i], port_links);

    port->bridge_local = bridge_local;
    port->parent_bridge_name = parent_bridge_name;
    port->vlan.tag = -1;

    return port;

fail:
    free(port->name);
    free(port);

    return NULL;
}

static void
ovs_bridge_port_free(ovs_ctx_t  *ctx,
                     port_entry *port)
{
    interface_entry *interface_tmp;
    interface_entry *interface;

    INFO("Freeing the port '%s' list entry", port->name);

    SLIST_FOREACH_SAFE(interface, &port->interfaces, port_links, interface_tmp)
    {
        SLIST_REMOVE(&port->interfaces, interface, interface_entry, port_links);
        ovs_interface_fini(ctx, interface);
    }

    free(port->value);
    free(port->name);
    free(port);
}

static te_errno
ovs_bridge_port_activate(ovs_ctx_t  *ctx,
                         port_entry *port)
{
    te_string        cmd = TE_STRING_INIT;
    interface_entry *interface;
    int              ret;
    te_errno         rc;

    INFO("Activating the port '%s'", port->name);

    rc = te_string_append(&cmd, "%s ovs-vsctl add-port %s %s",
                          ctx->env.ptr, port->parent_bridge_name, port->name);
    if (rc != 0)
    {
        ERROR("Failed to construct ovs-vsctl invocation command line");
        goto out;
    }

    SLIST_FOREACH(interface, &port->interfaces, port_links)
    {
        rc = ovs_cmd_vsctl_append_interface_arguments(interface, &cmd, ctx);
        if (rc != 0)
        {
            ERROR("Failed to append interface-specific arguments to "
                  "ovs-vsctl invocation command line");
            goto out;
        }
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-vsctl");
        rc = TE_ECHILD;
    }
    else
    {
        ta_waitpid(ret, NULL, 0);
    }

out:
    te_string_free(&cmd);

    return rc;
}

static void
ovs_bridge_port_deactivate(ovs_ctx_t  *ctx,
                           port_entry *port)
{
    te_string cmd = TE_STRING_INIT;
    int       ret;

    INFO("Deactivating the port '%s'", port->name);

    if (te_string_append(&cmd, "%s ovs-vsctl del-port %s %s", ctx->env.ptr,
                         port->parent_bridge_name, port->name) != 0)
    {
        ERROR("Failed to construct ovs-vsctl invocation command line");
        goto out;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-vsctl");
        goto out;
    }

    ta_waitpid(ret, NULL, 0);

out:
    te_string_free(&cmd);
}

static te_errno
ovs_bridge_port_init_bonded(ovs_ctx_t    *ctx,
                            bridge_entry *bridge,
                            const char   *port_name,
                            const char   *interface_list)
{
#if 0
    INFO("Initialising the bonded port '%s' comprising the interfaces '%s'",
         port_name, interface_list);

    /*
     * TODO: Pay attention to require that the port name
     *       doesn't coincide with that of any interface.
     */
#else
    UNUSED(ctx);
    UNUSED(bridge);
    UNUSED(port_name);
    UNUSED(interface_list);

    ERROR("There is no support for bonded ports as of now");
    return TE_EOPNOTSUPP;
#endif
}

static te_errno
ovs_bridge_port_init_regular(ovs_ctx_t    *ctx,
                             bridge_entry *bridge,
                             const char   *port_name)
{
    char            *error_message;
    te_bool          error_set;
    interface_entry *interface;
    port_entry      *port;
    te_errno         rc;

    INFO("Initialising the regular port '%s'", port_name);

    rc = ovs_interface_init(ctx, port_name, "system", TRUE, &interface);
    if (rc != 0)
    {
        ERROR("Failed to initialise the port interface list entry");
        goto fail_interface_init;
    }

    port = ovs_bridge_port_alloc(bridge->interface->name, port_name, "",
                                 FALSE, &interface, 1);
    if (port == NULL)
    {
        ERROR("Failed to allocate the port list entry");
        rc = TE_ENOMEM;
        goto fail_bridge_port_alloc;
    }

    rc = ovs_bridge_port_activate(ctx, port);
    if (rc != 0)
    {
        ERROR("Failed to activate the port");
        goto fail_bridge_port_activate;
    }

    rc = ovs_interface_check_error(ctx, interface, &error_set, &error_message);
    if (rc != 0)
    {
        ERROR("Failed to check the port interface for errors");
        goto fail_interface_check_error;
    }

    if (error_set)
    {
        ERROR("Failed to configure the port interface: '%s'", error_message);
        free(error_message);
        rc = TE_EFAULT;
        goto fail_interface_check_error;
    }

    SLIST_INSERT_HEAD(&bridge->ports, port, links);

    return 0;

fail_interface_check_error:
    ovs_bridge_port_deactivate(ctx, port);

fail_bridge_port_activate:
    ovs_bridge_port_free(ctx, port);
    interface = NULL;

fail_bridge_port_alloc:
    if (interface != NULL)
        ovs_interface_fini(ctx, interface);

fail_interface_init:

    return rc;
}

static void
ovs_bridge_port_fini(ovs_ctx_t    *ctx,
                     bridge_entry *bridge,
                     port_entry   *port)
{
    INFO("Finalising the port '%s'", port->name);

    assert(!port->bridge_local);

    SLIST_REMOVE(&bridge->ports, port, port_entry, links);
    ovs_bridge_port_deactivate(ctx, port);
    ovs_bridge_port_free(ctx, port);
}

static void
ovs_bridge_port_fini_all(ovs_ctx_t    *ctx,
                         bridge_entry *bridge)
{
    port_entry *port;
    port_entry *port_tmp;

    INFO("Finalising all ports but the local one at the bridge '%s'",
         bridge->interface->name);

    SLIST_FOREACH_SAFE(port, &bridge->ports, links, port_tmp)
    {
        if (!port->bridge_local)
            ovs_bridge_port_fini(ctx, bridge, port);
    }
}

static bridge_entry *
ovs_bridge_alloc(const char      *datapath_type,
                 interface_entry *interface)
{
    bridge_entry *bridge;
    port_entry   *port;

    INFO("Allocating the bridge list entry for '%s'", interface->name);

    bridge = TE_ALLOC(sizeof(*bridge));
    if (bridge == NULL)
    {
        ERROR("Failed to allocate memory for the bridge context");
        return NULL;
    }

    bridge->datapath_type = (datapath_type != NULL &&
                             datapath_type[0] != '\0') ?
                            strdup(datapath_type) : strdup("system");
    if (bridge->datapath_type == NULL)
    {
        ERROR("Failed to allocate memory for the bridge datapath type");
        goto fail;
    }

    bridge->interface = interface;

    SLIST_INIT(&bridge->ports);

    port = ovs_bridge_port_alloc(interface->name, interface->name,
                                 "", TRUE, &interface, 1);
    if (port == NULL)
    {
        ERROR("Failed to initialise the bridge local port list entry");
        goto fail;
    }

    SLIST_INSERT_HEAD(&bridge->ports, port, links);

    return bridge;

fail:
    free(bridge->datapath_type);
    free(bridge);

    return NULL;
}

static void
ovs_bridge_free(ovs_ctx_t    *ctx,
                bridge_entry *bridge)
{
    port_entry *port = SLIST_FIRST(&bridge->ports);

    INFO("Freeing the bridge list entry for '%s'", bridge->interface->name);

    assert(port->bridge_local);
    SLIST_REMOVE(&bridge->ports, port, port_entry, links);
    ovs_bridge_port_free(ctx, port);
    free(bridge->datapath_type);
    free(bridge);
}

static te_errno
ovs_bridge_activate(ovs_ctx_t    *ctx,
                    bridge_entry *bridge)
{
    te_string cmd = TE_STRING_INIT;
    int       ret;
    te_errno  rc;

    INFO("Activating the bridge '%s'", bridge->interface->name);

    rc = te_string_append(&cmd, "%s ovs-vsctl add-br %s -- set bridge %s "
                          "datapath_type=%s", ctx->env.ptr,
                          bridge->interface->name, bridge->interface->name,
                          bridge->datapath_type);
    if (rc != 0)
    {
        ERROR("Failed to construct ovs-vsctl invocation command line");
        goto out;
    }

    rc = ovs_cmd_vsctl_append_interface_arguments(bridge->interface, &cmd, ctx);
    if (rc != 0)
    {
        ERROR("Failed to append interface-specific arguments to "
              "ovs-vsctl invocation command line");
        goto out;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-vsctl");
        rc = TE_ECHILD;
    }
    else
    {
        ta_waitpid(ret, NULL, 0);
    }

out:
    te_string_free(&cmd);

    return rc;
}

static void
ovs_bridge_deactivate(ovs_ctx_t    *ctx,
                      bridge_entry *bridge)
{
    te_string cmd = TE_STRING_INIT;
    int       ret;

    INFO("Deactivating the bridge '%s'", bridge->interface->name);

    if (te_string_append(&cmd, "%s ovs-vsctl del-br %s",
                         ctx->env.ptr, bridge->interface->name) != 0)
    {
        ERROR("Failed to construct ovs-vsctl invocation command line");
        goto out;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-vsctl");
        goto out;
    }

    ta_waitpid(ret, NULL, 0);

out:
    te_string_free(&cmd);
}

static te_errno
ovs_bridge_init(ovs_ctx_t  *ctx,
                const char *name,
                const char *datapath_type)
{
    interface_entry *interface = NULL;
    char            *error_message;
    te_bool          error_set;
    bridge_entry    *bridge;
    te_errno         rc;

    INFO("Initialising the bridge '%s'", name);

    rc = ovs_interface_init(ctx, name, "internal", TRUE, &interface);
    if (rc != 0)
    {
        ERROR("Failed to initialise the bridge local interface list entry");
        goto fail_interface_init;
    }

    bridge = ovs_bridge_alloc(datapath_type, interface);
    if (bridge == NULL)
    {
        ERROR("Failed to allocate the bridge list entry");
        rc = TE_ENOMEM;
        goto fail_bridge_alloc;
    }

    rc = ovs_bridge_activate(ctx, bridge);
    if (rc != 0)
    {
        ERROR("Failed to activate the bridge");
        goto fail_bridge_activate;
    }

    rc = ovs_interface_check_error(ctx, interface, &error_set, &error_message);
    if (rc != 0)
    {
        ERROR("Failed to check the bridge local interface for errors");
        goto fail_interface_check_error;
    }

    if (error_set)
    {
        ERROR("Failed to configure the bridge local interface: '%s'",
              error_message);
        free(error_message);
        rc = TE_EFAULT;
        goto fail_interface_check_error;
    }

    SLIST_INSERT_HEAD(&ctx->bridges, bridge, links);

    return 0;

fail_interface_check_error:
    ovs_bridge_deactivate(ctx, bridge);

fail_bridge_activate:
    ovs_bridge_free(ctx, bridge);
    interface = NULL;

fail_bridge_alloc:
    if (interface != NULL)
        ovs_interface_fini(ctx, interface);

fail_interface_init:

    return rc;
}

static void
ovs_bridge_fini(ovs_ctx_t    *ctx,
                bridge_entry *bridge)
{
    interface_entry *interface = bridge->interface;

    INFO("Finalising the bridge '%s'", interface->name);

    ovs_bridge_port_fini_all(ctx, bridge);
    ovs_bridge_deactivate(ctx, bridge);
    SLIST_REMOVE(&ctx->bridges, bridge, bridge_entry, links);
    ovs_bridge_free(ctx, bridge);
}

static void
ovs_bridge_fini_all(ovs_ctx_t *ctx)
{
    bridge_entry *bridge_tmp;
    bridge_entry *bridge;

    INFO("Finalising the bridge(s)");

    SLIST_FOREACH_SAFE(bridge, &ctx->bridges, links, bridge_tmp)
        ovs_bridge_fini(ctx, bridge);
}


static void
ovs_daemon_stop(ovs_ctx_t  *ctx,
                const char *name)
{
    te_string cmd = TE_STRING_INIT;
    int       ret;

    INFO("Trying to stop the daemon '%s'", name);

    if (te_string_append(&cmd, "%s ovs-appctl -t %s exit",
                         ctx->env.ptr, name) != 0)
    {
        ERROR("Failed to construct ovs-appctl invocation command line");
        goto out;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        goto out;
    }

    ta_waitpid(ret, NULL, 0);

out:
    te_string_free(&cmd);
}

static te_bool
ovs_dbserver_is_running(ovs_ctx_t *ctx)
{
    return (ctx->dbserver_pid == -1) ?
           FALSE : (ta_waitpid(ctx->dbserver_pid, NULL, WNOHANG) == 0);
}

static te_bool
ovs_vswitchd_is_running(ovs_ctx_t *ctx)
{
    return (ctx->vswitchd_pid == -1) ?
           FALSE : (ta_waitpid(ctx->vswitchd_pid, NULL, WNOHANG) == 0);
}

static te_errno
ovs_dbserver_start(ovs_ctx_t *ctx)
{
    int ret;

    INFO("Starting the database server");

    ret = te_shell_cmd(ctx->dbtool_cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovsdb-tool");
        return TE_ECHILD;
    }

    ta_waitpid(ret, NULL, 0);

    ctx->dbserver_pid = te_shell_cmd(ctx->dbserver_cmd.ptr,
                                     -1, NULL, NULL, NULL);
    if (ctx->dbserver_pid == -1)
    {
        ERROR("Failed to invoke ovsdb-server");
        return TE_ECHILD;
    }

    return 0;
}

static void
ovs_dbserver_stop(ovs_ctx_t *ctx)
{
    unsigned int total_sleep_ms = 0;
    unsigned int sleep_ms = 1;

    INFO("Stopping the database server");

    ovs_daemon_stop(ctx, "ovsdb-server");

    do {
        te_msleep(sleep_ms);

        total_sleep_ms += sleep_ms;
        sleep_ms <<= 1;

        if (!ovs_dbserver_is_running(ctx))
            goto done;
    } while (total_sleep_ms < OVS_SLEEP_MS_MAX);

    ERROR("Failed to perform stop gracefully");
    WARN("Killing the parent process");
    ta_kill_death(ctx->dbserver_pid);

done:
    if (unlink(ctx->conf_db_lock_path.ptr) == -1)
        ERROR("Failed to unlink the database lock file (%r)",
              te_rc_os2te(errno));

    if (unlink(ctx->conf_db_path.ptr) == -1)
        ERROR("Failed to unlink the database file (%r)", te_rc_os2te(errno));

    ctx->dbserver_pid = -1;
}

static te_errno
ovs_shell_cmd_wait(const char *cmd)
{
    pid_t pid;

    pid = te_shell_cmd(cmd, -1, NULL, NULL, NULL);
    if (pid < 0)
    {
        ERROR("Failed to invoke '%s'", cmd);
        return TE_ECHILD;
    }

    if (ta_waitpid(pid, NULL, 0) < 0)
    {
        ERROR("Failed to wait '%s'", cmd);
        return TE_ECHILD;
    }

    return 0;
}

static te_errno
ovs_dpdk_configure_other_cfg(const ovs_ctx_t *ctx)
{
    te_kvpair *p;
    te_errno rc;

    TAILQ_FOREACH(p, &ctx->other_cfg, links)
    {
        te_string cfg = TE_STRING_INIT;

        rc = te_string_append(&cfg, "%sother_config:%s=",
                              ctx->ovs_cfg_cmd.ptr, p->key);
        if (rc == 0)
            rc = te_string_append_shell_arg_as_is(&cfg, p->value);

        if (rc == 0)
            rc = ovs_shell_cmd_wait(cfg.ptr);

        te_string_free(&cfg);
        if (rc != 0)
        {
            ERROR("Failed to create config command and execute it");
            return rc;
        }
    }

    return 0;
}

static te_errno
ovs_configure(ovs_ctx_t *ctx)
{
    te_string dpdk_init = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_append(&dpdk_init, "%sother_config:dpdk-init=%s",
                          ctx->ovs_cfg_cmd.ptr,
                          ctx->dpdk_init ? "true" : "false");
    if (rc != 0)
    {
        ERROR("Failed to create dpdk_init command");
        return TE_EINVAL;
    }

    rc = ovs_shell_cmd_wait(dpdk_init.ptr);
    te_string_free(&dpdk_init);

    if (rc != 0)
        return rc;

    return ovs_dpdk_configure_other_cfg(ctx);
}

static te_errno
ovs_vswitchd_start(ovs_ctx_t *ctx)
{
    te_errno rc;

    /* Configure OVS before starting switch daemon */
    rc = ovs_configure(ctx);
    if (rc != 0)
        return rc;

    INFO("Starting the switch daemon");

    ctx->vswitchd_pid = te_shell_cmd(ctx->vswitchd_cmd.ptr,
                                     -1, NULL, NULL, NULL);
    if (ctx->vswitchd_pid == -1)
    {
        ERROR("Failed to invoke ovs-vswitchd");
        return TE_ECHILD;
    }

    return 0;
}

static void
ovs_vswitchd_stop(ovs_ctx_t *ctx)
{
    unsigned int total_sleep_ms = 0;
    unsigned int sleep_ms = 1;

    INFO("Stopping the switch daemon");

    ovs_daemon_stop(ctx, "ovs-vswitchd");

    do {
        te_msleep(sleep_ms);

        total_sleep_ms += sleep_ms;
        sleep_ms <<= 1;

        if (!ovs_vswitchd_is_running(ctx))
            goto done;
    } while (total_sleep_ms < OVS_SLEEP_MS_MAX);

    ERROR("Failed to perform stop gracefully");
    WARN("Killing the parent process");
    ta_kill_death(ctx->vswitchd_pid);

done:
    ctx->vswitchd_pid = -1;
}

static te_errno
ovs_await_resource(ovs_ctx_t  *ctx,
                   const char *resource_name)
{
    te_string    resource_path = TE_STRING_INIT;
    unsigned int total_sleep_ms = 0;
    unsigned int sleep_ms = 1;
    te_errno     rc;

    INFO("Waiting for '%s' to get ready", resource_name);

    rc = te_string_append(&resource_path, "%s/%s",
                          ctx->root_path.ptr, resource_name);
    if (rc != 0)
    {
        ERROR("Failed to construct the resource path");
        return rc;
    }

    do {
        te_msleep(sleep_ms);

        total_sleep_ms += sleep_ms;
        sleep_ms <<= 1;

        if (access(resource_path.ptr, F_OK) == 0)
        {
            te_string_free(&resource_path);
            return 0;
        }
    } while (total_sleep_ms < OVS_SLEEP_MS_MAX);

    ERROR("Failed to wait for the resource to get ready");

    te_string_free(&resource_path);

    return TE_EIO;
}

static te_errno
ovs_start(ovs_ctx_t *ctx)
{
    te_errno rc;

    INFO("Starting the facility");

    rc = ovs_dbserver_start(ctx);
    if (rc != 0)
    {
        ERROR("Failed to start the database server");
        return rc;
    }

    rc = ovs_await_resource(ctx, "ovsdb-server.pid");
    if (rc != 0)
    {
        ERROR("Failed to check the database server responsiveness");
        ovs_dbserver_stop(ctx);
        return rc;
    }

    rc = ovs_await_resource(ctx, "db.sock");
    if (rc != 0)
    {
        ERROR("Failed to check the database socket presence");
        ovs_dbserver_stop(ctx);
        return rc;
    }

    rc = ovs_vswitchd_start(ctx);
    if (rc != 0)
    {
        ERROR("Failed to start the switch daemon");
        ovs_dbserver_stop(ctx);
        return rc;
    }

    rc = ovs_await_resource(ctx, "ovs-vswitchd.pid");
    if (rc != 0)
    {
        ERROR("Failed to check the switch daemon responsiveness");
        ovs_vswitchd_stop(ctx);
        ovs_dbserver_stop(ctx);
        return rc;
    }

    rc = ovs_log_init_modules(ctx);
    if (rc != 0)
    {
        ERROR("Failed to construct log module context");
        ovs_vswitchd_stop(ctx);
        ovs_dbserver_stop(ctx);
        return rc;
    }

    return 0;
}

static te_errno
ovs_stop(ovs_ctx_t *ctx)
{
    INFO("Stopping the facility");

    ovs_bridge_fini_all(ctx);
    ovs_interface_fini_all(ctx);
    ovs_log_fini_modules(ctx);
    ovs_vswitchd_stop(ctx);
    ovs_dbserver_stop(ctx);

    return 0;
}

static int
ovs_is_running(ovs_ctx_t *ctx)
{
    te_bool dbserver_is_running = ovs_dbserver_is_running(ctx);
    te_bool vswitchd_is_running = ovs_vswitchd_is_running(ctx);

    if (dbserver_is_running != vswitchd_is_running)
    {
        WARN("One of the compulsory services was not found running. Stopping.");
        ovs_stop(ctx);
        return 0;
    }
    else
    {
        return (vswitchd_is_running) ? 1 : 0;
    }
}

static te_errno
ovs_status_get(unsigned int  gid,
               const char   *oid,
               char         *value,
               const char   *ovs)
{
    te_errno   rc = 0;
    ovs_ctx_t *ctx;
    int        ret;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying the facility status");

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    ret = snprintf(value, RCF_MAX_VAL, "%d", ovs_is_running(ctx));
    if (ret < 0)
    {
        ERROR("Failed to indicate status");
        rc = TE_EFAULT;
    }

out:
    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_status_set(unsigned int  gid,
               const char   *oid,
               const char   *value,
               const char   *ovs)
{
    te_bool    enable = !!atoi(value);
    ovs_ctx_t *ctx;
    te_errno   rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Toggling the facility status");

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    if (enable == ovs_is_running(ctx))
    {
        INFO("The facility status does not need to be updated");
        return 0;
    }

    rc = (enable) ? ovs_start(ctx) : ovs_stop(ctx);
    if (rc != 0)
        ERROR("Failed to change status");

out:
    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_dpdk_init_get(unsigned int  gid,
                  const char   *oid,
                  char         *value,
                  const char   *ovs)
{
    ovs_ctx_t *ctx;

    UNUSED(gid);
    UNUSED(oid);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    snprintf(value, RCF_MAX_VAL, "%d", ctx->dpdk_init);

    return 0;
}

static te_errno
ovs_dpdk_init_set(unsigned int  gid,
                  const char   *oid,
                  const char   *value,
                  const char   *ovs)
{
    te_bool    enable = !!atoi(value);
    ovs_ctx_t *ctx;

    UNUSED(gid);
    UNUSED(oid);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (enable == ctx->dpdk_init)
        return 0;

    if (ovs_is_running(ctx))
    {
        ERROR("The facility is in use, cannot change dpdk_init");
        return TE_RC(TE_TA_UNIX, TE_EBUSY);
    }

    ctx->dpdk_init = enable;

    return 0;
}

static te_errno
ovs_other_config_add(unsigned int  gid,
                     const char   *oid,
                     const char   *value,
                     const char   *ovs,
                     const char   *name)
{
    ovs_ctx_t       *ctx;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Adding the OVS other_config entry '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (name[0] == '\0' || value[0] == '\0')
    {
        ERROR("The OVS other config name or value is empty");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (!ovs_value_is_valid(other_config_types, name))
    {
        ERROR("Invalid other config entry");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return te_kvpair_add(&ctx->other_cfg, name, "%s", value);
}

static te_errno
ovs_other_config_del(unsigned int  gid,
                     const char   *oid,
                     const char   *ovs,
                     const char   *name)
{
    ovs_ctx_t       *ctx;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Removing the other_config '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return te_kvpairs_del(&ctx->other_cfg, name);
}

static te_errno
ovs_other_config_get(unsigned int  gid,
                     const char   *oid,
                     char         *value,
                     const char   *ovs,
                     const char   *name)
{
    ovs_ctx_t       *ctx;
    const char *cfg;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Querying the type of the other_config '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    cfg = te_kvpairs_get(&ctx->other_cfg, name);
    if (cfg == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(value, cfg, RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_other_config_list(unsigned int   gid,
                      const char    *oid_str,
                      const char    *sub_id,
                      char         **list)
{
    te_string        list_container = TE_STRING_INIT;
    te_errno         rc = 0;
    cfg_oid         *oid;
    const char      *ovs;
    ovs_ctx_t       *ctx;
    te_kvpair *p;

    UNUSED(gid);
    UNUSED(sub_id);

    INFO("Constructing the list of other_config entries");

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert the OID string to native OID handle");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ovs = CFG_OID_GET_INST_NAME(oid, 2);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    TAILQ_FOREACH(p, &ctx->other_cfg, links)
    {
        rc = te_string_append(&list_container, "%s ", p->key);
        if (rc != 0)
        {
            ERROR("Failed to construct the list");
            te_string_free(&list_container);
            goto out;
        }
    }

    *list = list_container.ptr;

out:
    cfg_free_oid(oid);

    return TE_RC(TE_TA_UNIX, rc);
}

static log_module_t *
ovs_log_module_find(ovs_ctx_t  *ctx,
                    const char *name)
{
    unsigned int i;

    for (i = 0; i < ctx->nb_log_modules; ++i)
    {
        if (strcmp(name, ctx->log_modules[i].name) == 0)
            return &ctx->log_modules[i];
    }

    return NULL;
}

static te_errno
ovs_log_get(unsigned int  gid,
            const char   *oid,
            char         *value,
            const char   *ovs,
            const char   *name)
{
    log_module_t *module;
    ovs_ctx_t    *ctx;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying log level word for the module '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    module = ovs_log_module_find(ctx, name);
    if (module == NULL)
    {
        ERROR("The log module does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    te_strlcpy(value, module->level, RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_log_set(unsigned int  gid,
            const char   *oid,
            const char   *value,
            const char   *ovs,
            const char   *name)
{
    te_string     cmd = TE_STRING_INIT;
    log_module_t *module;
    char         *level;
    ovs_ctx_t    *ctx;
    int           ret;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Setting log level word '%s' for the module '%s'", value, name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    module = ovs_log_module_find(ctx, name);
    if (module == NULL)
    {
        ERROR("The log module does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (!ovs_value_is_valid(log_levels, value))
    {
        ERROR("The log level word is illicit");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    level = strdup(value);
    if (level == NULL)
    {
        ERROR("Failed to copy the log level word");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = te_string_append(&cmd, "%s ovs-appctl -t ovs-vswitchd vlog/set %s:%s",
                          ctx->env.ptr, module->name, level);
    if (rc != 0)
    {
        ERROR("Failed to construct ovs-appctl invocation command line");
        goto out;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        rc = TE_ECHILD;
        goto out;
    }

    ta_waitpid(ret, NULL, 0);

    free(module->level);
    module->level = level;

out:
    te_string_free(&cmd);

    if (rc != 0)
        free(level);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_log_list(unsigned int   gid,
             const char    *oid_str,
             const char    *sub_id,
             char         **list)
{
    te_string    list_container = TE_STRING_INIT;
    te_errno     rc = 0;
    cfg_oid     *oid;
    const char  *ovs;
    ovs_ctx_t   *ctx;
    unsigned int i;

    UNUSED(gid);
    UNUSED(sub_id);

    INFO("Constructing the list of log modules");

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert the OID string to native OID handle");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ovs = CFG_OID_GET_INST_NAME(oid, 2);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    for (i = 0; i < ctx->nb_log_modules; ++i)
    {
        rc = te_string_append(&list_container, "%s ", ctx->log_modules[i].name);
        if (rc != 0)
        {
            ERROR("Failed to construct the list");
            te_string_free(&list_container);
            goto out;
        }
    }

    *list = list_container.ptr;

out:
    free(oid);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_pick(const char       *ovs,
                   const char       *interface_name,
                   te_bool           writable,
                   ovs_ctx_t       **ctx_out,
                   interface_entry **interface_out)
{
    ovs_ctx_t       *ctx;
    interface_entry *interface;

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    interface = ovs_interface_find(ctx, interface_name);
    if (interface == NULL)
    {
        ERROR("The interface does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (writable && interface->active)
    {
        ERROR("The interface is in use");
        return TE_RC(TE_TA_UNIX, TE_EBUSY);
    }

    if (ctx_out != NULL)
        *ctx_out = ctx;

    if (interface_out != NULL)
        *interface_out = interface;

    return 0;
}

static te_errno
ovs_interface_link_state_get(unsigned int  gid,
                             const char   *oid,
                             char         *value,
                             const char   *ovs,
                             const char   *interface_name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying (requested) link state of the interface '%s'",
         interface_name);

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (interface->active)
    {
        char     *resp;
        te_errno  rc;

        rc = ovs_value_get_effective(ctx, "interface", interface->name,
                                     "link_state", &resp);
        if (rc != 0)
        {
            ERROR("Failed to query the effective value");
            return TE_RC(TE_TA_UNIX, rc);
        }

        if (strcmp(resp, "down") == 0)
            value[0] = '0';
        else if (strcmp(resp, "up") == 0)
            value[0] = '1';
        else
            assert(FALSE);

        free(resp);
    }
    else
    {
        value[0] = '0';
    }

    value[1] = '\0';

    return 0;
}

static te_errno
ovs_interface_dpdk_devargs_get(unsigned int  gid,
                               const char   *oid,
                               char         *value,
                               const char   *ovs,
                               const char   *interface_name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_strlcpy(value, te_str_empty_if_null(interface->dpdk_devargs),
               RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_interface_dpdk_devargs_set(unsigned int  gid,
                               const char   *oid,
                               const char   *value,
                               const char   *ovs,
                               const char   *interface_name)
{
    interface_entry *interface;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_interface_pick(ovs, interface_name, TRUE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    return string_replace(&interface->dpdk_devargs, value);
}

static te_errno
ovs_interface_tunnel_option_get(unsigned int  gid,
                                const char   *oid,
                                char         *value,
                                const char   *ovs,
                                const char   *interface_name,
                                const char   *option_name)
{
    interface_tunnel_option option;
    interface_entry *interface;
    ovs_ctx_t *ctx;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_interface_tunnel_option_by_name(option_name, &option);
    if (rc != 0)
        return rc;

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_strlcpy(value, te_str_empty_if_null(interface->tunnel_options[option]),
               RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_interface_tunnel_option_add(unsigned int  gid,
                                const char   *oid,
                                char         *value,
                                const char   *ovs,
                                const char   *interface_name,
                                const char   *option_name)
{
    interface_tunnel_option option;
    interface_entry *interface;
    ovs_ctx_t *ctx;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_interface_tunnel_option_by_name(option_name, &option);
    if (rc != 0)
        return rc;

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    return string_replace(&interface->tunnel_options[option], value);
}

static te_errno
ovs_interface_tunnel_option_del(unsigned int  gid,
                                const char   *oid,
                                const char   *ovs,
                                const char   *interface_name,
                                const char   *option_name)
{
    interface_tunnel_option option;
    interface_entry *interface;
    ovs_ctx_t *ctx;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_interface_tunnel_option_by_name(option_name, &option);
    if (rc != 0)
        return rc;

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    return string_replace(&interface->tunnel_options[option], NULL);
}

static te_errno
ovs_interface_tunnel_option_list(unsigned int  gid,
                                 const char   *oid,
                                 const char   *sub_id,
                                 char        **list,
                                 const char   *ovs,
                                 const char   *interface_name)
{
    te_string        list_container = TE_STRING_INIT;
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;
    unsigned int     i;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    for (i = 0; i < INTERFACE_TUNNEL_END; i++)
    {
        if (interface->tunnel_options[i] == NULL)
            continue;

        rc = te_string_append(&list_container, "%s ",
                              interface_tunnel_option_names[i]);
        if (rc != 0)
        {
            ERROR("Failed to construct the list");
            te_string_free(&list_container);
            return rc;
        }
    }

    *list = list_container.ptr;

    return 0;
}

static te_errno
ovs_interface_vhost_server_path_get(unsigned int  gid,
                                    const char   *oid,
                                    char         *value,
                                    const char   *ovs,
                                    const char   *interface_name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_strlcpy(value, te_str_empty_if_null(interface->vhost_server_path),
               RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_interface_vhost_server_path_set(unsigned int  gid,
                                    const char   *oid,
                                    const char   *value,
                                    const char   *ovs,
                                    const char   *interface_name)
{
    interface_entry *interface;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_interface_pick(ovs, interface_name, TRUE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    return string_replace(&interface->vhost_server_path, value);
}

static te_errno
ovs_interface_mtu_get(unsigned int  gid,
                      const char   *oid,
                      char         *value,
                      const char   *ovs,
                      const char   *interface_name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying (requested) MTU of the interface '%s'", interface_name);

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (interface->active)
    {
        char     *resp;
        te_errno  rc;

        rc = ovs_value_get_effective(ctx, "interface", interface->name,
                                     "mtu", &resp);
        if (rc != 0)
        {
            ERROR("Failed to query the effective value");
            return TE_RC(TE_TA_UNIX, rc);
        }

        te_strlcpy(value, (strcmp(resp, "[]") == 0) ? "0" : resp, RCF_MAX_VAL);
        free(resp);
    }
    else
    {
        te_strlcpy(value, interface->mtu_request, RCF_MAX_VAL);
    }

    return 0;
}

static te_errno
ovs_interface_mtu_set(unsigned int  gid,
                      const char   *oid,
                      const char   *value,
                      const char   *ovs,
                      const char   *interface_name)
{
    interface_entry *interface;
    int              test;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Requesting the custom MTU '%s' for the interface '%s'",
         value, interface_name);

    rc = ovs_interface_pick(ovs, interface_name, TRUE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (sscanf(value, "%d", &test) != 1 || test < 0 || test > UINT16_MAX)
    {
        ERROR("The value is not a valid MTU");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = string_replace(&interface->mtu_request, value);
    if (rc != 0)
        ERROR("Failed to store the new interface custom MTU value");
    else
        interface->mtu_requested = (test != 0) ? TRUE : FALSE;

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_ofport_get(unsigned int  gid,
                         const char   *oid,
                         char         *value,
                         const char   *ovs,
                         const char   *interface_name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying (requested) ofport of the interface '%s'", interface_name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (interface->active)
    {
        char     *resp;
        te_errno  rc;

        rc = ovs_value_get_effective(ctx, "interface", interface->name,
                                     "ofport", &resp);
        if (rc != 0)
        {
            ERROR("Failed to query the effective value");
            return TE_RC(TE_TA_UNIX, rc);
        }

        te_strlcpy(value, resp, RCF_MAX_VAL);
        free(resp);
    }
    else
    {
        te_strlcpy(value, interface->ofport_request, RCF_MAX_VAL);
    }

    return 0;
}

static te_errno
ovs_interface_ofport_set(unsigned int  gid,
                         const char   *oid,
                         const char   *value,
                         const char   *ovs,
                         const char   *interface_name)
{
    interface_entry *interface;
    int              test;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Requesting the custom ofport '%s' for the interface '%s'",
         value, interface_name);

    rc = ovs_interface_pick(ovs, interface_name, TRUE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (sscanf(value, "%d", &test) != 1 || test < 0 || test > UINT16_MAX)
    {
        ERROR("The value is not a valid ofport");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = string_replace(&interface->ofport_request, value);
    if (rc != 0)
        ERROR("Failed to store the new interface custom ofport value");
    else
        interface->ofport_requested = (test != 0) ? TRUE : FALSE;

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_mac_get_effective(ovs_ctx_t       *ctx,
                                interface_entry *interface,
                                char            *buf)
{
    char     *resp;
    te_errno  rc;

    INFO("Querying effective MAC of the interface '%s'", interface->name);

    rc = ovs_value_get_effective(ctx, "interface", interface->name,
                                 "mac_in_use", &resp);
    if (rc != 0)
    {
        ERROR("Failed to perform the query");
        return rc;
    }

    if (strlen(resp) > RCF_MAX_VAL + 1)
    {
        ERROR("The response does not fit in the available buffer");
        rc = TE_ENOBUFS;
        goto out;
    }

    if (sscanf(resp, "\"%[^\"]", buf) != 1)
    {
        ERROR("Failed to parse the response");
        rc = TE_ENODATA;
    }

out:
    free(resp);

    return rc;
}

static te_errno
ovs_interface_mac_get(unsigned int  gid,
                      const char   *oid,
                      char         *value,
                      const char   *ovs,
                      const char   *interface_name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying (requested) MAC of the interface '%s'", interface_name);

    rc = ovs_interface_pick(ovs, interface_name, FALSE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (interface->active)
    {
        te_errno rc;

        rc = ovs_interface_mac_get_effective(ctx, interface, value);
        if (rc != 0)
        {
            ERROR("Failed to query effective MAC for the interface");
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    else
    {
        te_strlcpy(value, te_str_empty_if_null(interface->mac_request),
                   RCF_MAX_VAL);
    }

    return 0;
}

static te_errno
ovs_interface_mac_set(unsigned int  gid,
                      const char   *oid,
                      const char   *value,
                      const char   *ovs,
                      const char   *interface_name)
{
    te_bool          mac_requested = FALSE;
    interface_entry *interface;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Requesting the custom MAC '%s' for the interface '%s'",
         value, interface_name);

    rc = ovs_interface_pick(ovs, interface_name, TRUE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (value[0] != '\0')
    {
        if (!ovs_value_is_valid_mac(value))
        {
            ERROR("The value is not a valid MAC");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        mac_requested = TRUE;
    }

    rc = string_replace(&interface->mac_request, value);
    if (rc != 0)
        ERROR("Failed to store the new interface custom MAC value");
    else
        interface->mac_requested = mac_requested;

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_add(unsigned int  gid,
                  const char   *oid,
                  const char   *value,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Adding the interface '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (name[0] == '\0')
    {
        ERROR("The interface name is empty");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    interface = ovs_interface_find(ctx, name);
    if (interface != NULL)
    {
        ERROR("The interface already exists");
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    if (value[0] != '\0')
    {
        if (!ovs_value_is_valid(interface_types, value) &&
            !ovs_value_is_valid(interface_tunnel_types, value))
        {
            ERROR("The interface type is unsupported");
            return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        }
    }

    rc = ovs_interface_init(ctx, name, value, FALSE, NULL);
    if (rc != 0)
        ERROR("Failed to initialise the interface list entry");

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_del(unsigned int  gid,
                  const char   *oid,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Removing the interface '%s'", name);

    rc = ovs_interface_pick(ovs, name, TRUE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    ovs_interface_fini(ctx, interface);

    return 0;
}

static te_errno
ovs_interface_get(unsigned int  gid,
                  const char   *oid,
                  char         *value,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Querying the type of the interface '%s'", name);

    rc = ovs_interface_pick(ovs, name, FALSE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_strlcpy(value, interface->type, RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_interface_set(unsigned int  gid,
                  const char   *oid,
                  const char   *value,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Setting the type '%s' for the interface '%s'", value, name);

    rc = ovs_interface_pick(ovs, name, TRUE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (!ovs_value_is_valid(interface_types, value) &&
        !ovs_value_is_valid(interface_tunnel_types, value))
    {
        ERROR("The interface type is unsupported");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    rc = string_replace(&interface->type, value);
    if (rc != 0)
        ERROR("Failed to store the new interface type value");

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_list(unsigned int   gid,
                   const char    *oid_str,
                   const char    *sub_id,
                   char         **list)
{
    te_string        list_container = TE_STRING_INIT;
    interface_entry *interface;
    te_errno         rc = 0;
    cfg_oid         *oid;
    const char      *ovs;
    ovs_ctx_t       *ctx;

    UNUSED(gid);
    UNUSED(sub_id);

    INFO("Constructing the list of interfaces");

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert the OID string to native OID handle");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ovs = CFG_OID_GET_INST_NAME(oid, 2);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    SLIST_FOREACH(interface, &ctx->interfaces, links)
    {
        rc = te_string_append(&list_container, "%s ", interface->name);
        if (rc != 0)
        {
            ERROR("Failed to construct the list");
            te_string_free(&list_container);
            goto out;
        }
    }

    *list = list_container.ptr;

out:
    free(oid);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_bridge_pick(const char    *ovs,
                const char    *bridge_name,
                ovs_ctx_t    **ctx_out,
                bridge_entry **bridge_out)
{
    ovs_ctx_t    *ctx;
    bridge_entry *bridge;

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    bridge = ovs_bridge_find(ctx, bridge_name);
    if (bridge == NULL)
    {
        ERROR("The interface does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ctx_out != NULL)
        *ctx_out = ctx;

    if (bridge_out != NULL)
        *bridge_out = bridge;

    return 0;
}

static te_errno
ovs_bridge_add(unsigned int  gid,
               const char   *oid,
               const char   *value,
               const char   *ovs,
               const char   *name)
{
    bridge_entry *bridge;
    ovs_ctx_t    *ctx;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Adding the bridge '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (name[0] == '\0')
    {
        ERROR("The bridge name is empty");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    bridge = ovs_bridge_find(ctx, name);
    if (bridge != NULL)
    {
        ERROR("The bridge already exists");
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    if (value[0] != '\0')
    {
        if (!ovs_value_is_valid(bridge_datapath_types, value))
        {
            ERROR("The bridge datapath type is unsupported");
            return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        }
    }

    rc = ovs_bridge_init(ctx, name, value);
    if (rc != 0)
        ERROR("Failed to initialise the bridge");

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_bridge_del(unsigned int  gid,
               const char   *oid,
               const char   *ovs,
               const char   *name)
{
    bridge_entry *bridge;
    ovs_ctx_t    *ctx;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Removing the bridge '%s'", name);

    rc = ovs_bridge_pick(ovs, name, &ctx, &bridge);
    if (rc != 0)
    {
        ERROR("Failed to pick the bridge entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    ovs_bridge_fini(ctx, bridge);

    return 0;
}

static te_errno
ovs_bridge_get(unsigned int  gid,
               const char   *oid,
               char         *value,
               const char   *ovs,
               const char   *name)
{
    bridge_entry *bridge;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying the datapath type of the bridge '%s'", name);

    rc = ovs_bridge_pick(ovs, name, NULL, &bridge);
    if (rc != 0)
    {
        ERROR("Failed to pick the bridge entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_strlcpy(value, bridge->datapath_type, RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_bridge_list(unsigned int   gid,
                const char    *oid_str,
                const char    *sub_id,
                char         **list)
{
    te_string     list_container = TE_STRING_INIT;
    bridge_entry *bridge;
    te_errno      rc = 0;
    cfg_oid      *oid;
    const char   *ovs;
    ovs_ctx_t    *ctx;

    UNUSED(gid);
    UNUSED(sub_id);

    INFO("Constructing the list of bridges");

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert the OID string to native OID handle");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ovs = CFG_OID_GET_INST_NAME(oid, 2);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    SLIST_FOREACH(bridge, &ctx->bridges, links)
    {
        rc = te_string_append(&list_container, "%s ", bridge->interface->name);
        if (rc != 0)
        {
            ERROR("Failed to construct the list of bridges");
            te_string_free(&list_container);
            goto out;
        }
    }

    *list = list_container.ptr;

out:
    free(oid);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_bridge_port_pick(const char    *ovs,
                     const char    *bridge_name,
                     const char    *port_name,
                     ovs_ctx_t    **ctx_out,
                     bridge_entry **bridge_out,
                     port_entry   **port_out)
{
    ovs_ctx_t    *ctx;
    bridge_entry *bridge;
    port_entry   *port;
    te_errno      rc;

    rc = ovs_bridge_pick(ovs, bridge_name, &ctx, &bridge);
    if (rc != 0)
    {
        ERROR("Failed to pick the bridge entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    port = ovs_bridge_port_find(bridge, port_name);
    if (port == NULL)
    {
        ERROR("The port does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ctx_out != NULL)
        *ctx_out = ctx;

    if (bridge_out != NULL)
        *bridge_out = bridge;

    if (port_out != NULL)
        *port_out = port;

    return 0;
}

static te_errno
ovs_bridge_port_add(unsigned int  gid,
                    const char   *oid,
                    const char   *value,
                    const char   *ovs,
                    const char   *bridge_name,
                    const char   *port_name)
{
    bridge_entry *bridge;
    port_entry   *port;
    ovs_ctx_t    *ctx;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Adding the port '%s' to the bridge '%s'", port_name, bridge_name);

    rc = ovs_bridge_pick(ovs, bridge_name, &ctx, &bridge);
    if (rc != 0)
    {
        ERROR("Failed to pick the bridge entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (port_name[0] == '\0')
    {
        ERROR("The port name is empty");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    port = ovs_bridge_port_find(bridge, port_name);
    if (port != NULL)
    {
        ERROR("The port already exists");
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    rc = (value[0] == '\0') ?
         ovs_bridge_port_init_regular(ctx, bridge, port_name) :
         ovs_bridge_port_init_bonded(ctx, bridge, port_name, value);
    if (rc != 0)
        ERROR("Failed to initialise the port");

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_bridge_port_del(unsigned int  gid,
                    const char   *oid,
                    const char   *ovs,
                    const char   *bridge_name,
                    const char   *port_name)
{
    bridge_entry *bridge;
    port_entry   *port;
    ovs_ctx_t    *ctx;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Freeing the bridge '%s' port '%s'", bridge_name, port_name);

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (!port->bridge_local)
        ovs_bridge_port_fini(ctx, bridge, port);

    return 0;
}

static te_errno
ovs_bridge_port_get(unsigned int  gid,
                    const char   *oid,
                    char         *value,
                    const char   *ovs,
                    const char   *bridge_name,
                    const char   *port_name)
{
    port_entry *port;
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying the bridge '%s' port '%s' value", bridge_name, port_name);

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name, NULL, NULL, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_strlcpy(value, port->value, RCF_MAX_VAL);

    return 0;
}

static te_errno
ovs_bridge_port_list(unsigned int   gid,
                     const char    *oid_str,
                     const char    *sub_id,
                     char         **list)
{
    te_string     list_container = TE_STRING_INIT;
    const char   *bridge_name;
    bridge_entry *bridge;
    port_entry   *port;
    cfg_oid      *oid;
    const char   *ovs;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(sub_id);

    INFO("Constructing the port list of the bridge by the OID '%s'", oid_str);

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert the OID string to native OID handle");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ovs = CFG_OID_GET_INST_NAME(oid, 2);
    bridge_name = CFG_OID_GET_INST_NAME(oid, 3);

    rc = ovs_bridge_pick(ovs, bridge_name, NULL, &bridge);
    if (rc != 0)
    {
        ERROR("Failed to pick the bridge entry");
        goto out;
    }

    SLIST_FOREACH(port, &bridge->ports, links)
    {
        rc = te_string_append(&list_container, "%s ", port->name);
        if (rc != 0)
        {
            ERROR("Failed to construct the port list");
            te_string_free(&list_container);
            goto out;
        }
    }

    *list = list_container.ptr;

out:
    free(oid);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_bridge_port_vlan_get(unsigned int  gid,
                         const char   *oid,
                         char         *value,
                         const char   *ovs,
                         const char   *bridge_name,
                         const char   *port_name)
{
    port_entry      *port;
    bridge_entry    *bridge;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_strlcpy(value, te_str_empty_if_null(port->vlan.type), RCF_MAX_VAL);

    return 0;
}

typedef enum ovs_cfg_action {
    OVS_CFG_SET,
    OVS_CFG_REMOVE,
    OVS_CFG_ADD,
} ovs_cfg_action;

static te_errno
ovs_configure_value_va(ovs_ctx_t *ctx, const char *entity, const char *name,
                      const char *cfg, ovs_cfg_action action, const char *fmt,
                      va_list ap)
{
    te_string cmd = TE_STRING_INIT;
    te_errno rc;
    int ret;
    const char *action_str;

    switch (action)
    {
        case OVS_CFG_SET:
            action_str = "set";
            break;
        case OVS_CFG_REMOVE:
            action_str = "remove";
            break;
        case OVS_CFG_ADD:
            action_str = "add";
            break;
        default:
            ERROR("Invalid configure action");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = te_string_append(&cmd, "%s ovs-vsctl %s %s %s %s%s",
                          ctx->env.ptr, action_str, entity, name, cfg,
                          (action == OVS_CFG_SET) ? "=" : " ");
    if (rc == 0)
        rc = te_string_append_va(&cmd, fmt, ap);

    if (rc != 0)
    {
        ERROR("Failed to create configuration command");
        te_string_free(&cmd);
        return rc;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    te_string_free(&cmd);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-vsctl");
        return TE_RC(TE_TA_UNIX, TE_ECHILD);
    }
    else
    {
        ta_waitpid(ret, NULL, 0);
    }

    return 0;
}

static te_errno
ovs_configure_value(ovs_ctx_t *ctx, const char *entity, const char *name,
                    const char *cfg, ovs_cfg_action action, const char *fmt, ...)
{
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = ovs_configure_value_va(ctx, entity, name, cfg, action, fmt, ap);
    va_end(ap);

    return rc;
}

static te_errno
ovs_bridge_port_vlan_set(unsigned int  gid,
                         const char   *oid,
                         const char   *value,
                         const char   *ovs,
                         const char   *bridge_name,
                         const char   *port_name)
{
    port_entry      *port;
    bridge_entry    *bridge;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    if (*value != '\0' && !ovs_value_is_valid(vlan_types, value))
    {
        ERROR("Invalid vlan type");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((*value == '\0' && port->vlan.type == NULL) ||
        strcmp_null(value, port->vlan.type) == 0)
    {
        return 0;
    }

    rc = ovs_configure_value(ctx, "port", port->name, "vlan_mode",
                             *value == '\0' ? OVS_CFG_REMOVE : OVS_CFG_SET,
                             "%s", *value == '\0' ? port->vlan.type : value);
    if (rc != 0)
    {
        ERROR("Failed to configure port's vlan mode");
        return rc;
    }

    return string_replace(&port->vlan.type, value);
}

static te_errno
ovs_bridge_port_vlan_tag_get(unsigned int  gid,
                             const char   *oid,
                             char         *value,
                             const char   *ovs,
                             const char   *bridge_name,
                             const char   *port_name)
{
    port_entry      *port;
    bridge_entry    *bridge;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    te_snprintf(value, RCF_MAX_VAL, "%d", port->vlan.tag);

    return 0;
}

static te_errno
ovs_bridge_port_vlan_tag_set(unsigned int  gid,
                             const char   *oid,
                             const char   *value,
                             const char   *ovs,
                             const char   *bridge_name,
                             const char   *port_name)
{
    port_entry      *port;
    bridge_entry    *bridge;
    ovs_ctx_t       *ctx;
    te_errno         rc;
    int              tag;
    te_bool          set;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strtoi(value, 0, &tag);
    if (rc != 0)
        return rc;

    if (tag < -1 || tag >= PORT_MAX_TRUNKS)
    {
        ERROR("Invalid VLAN tag: %d", tag);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    set = tag >= 0;

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (set && strcmp_null(port->vlan.type, "trunk") == 0)
    {
        ERROR("Cannot set vlan tag in trunk mode");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (port->vlan.tag == tag)
        return 0;

    rc = ovs_configure_value(ctx, "port", port->name, "tag",
                             set ? OVS_CFG_SET : OVS_CFG_REMOVE, "%d",
                             set ? tag : port->vlan.tag);
    if (rc != 0)
        return rc;

    port->vlan.tag = tag;

    return 0;
}

static te_errno
ovs_bridge_port_vlan_trunks_list(unsigned int   gid,
                                 const char    *oid,
                                 const char    *sub_id,
                                 char         **list,
                                 const char    *ovs,
                                 const char    *bridge_name,
                                 const char    *port_name)
{
    te_string     list_container = TE_STRING_INIT;
    port_entry   *port;
    bridge_entry *bridge;
    ovs_ctx_t    *ctx;
    te_errno      rc;
    size_t        i;

    UNUSED(gid);
    UNUSED(sub_id);
    UNUSED(oid);

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    for (i = 0; i < TE_ARRAY_LEN(port->vlan.trunks); i++)
    {
        if (!port->vlan.trunks[i])
            continue;

        rc = te_string_append(&list_container, "%d ", i);
        if (rc != 0)
        {
            ERROR("Failed to construct the port trunks list");
            te_string_free(&list_container);
            return rc;
        }
    }

    *list = list_container.ptr;

    return 0;
}

static te_errno
ovs_bridge_port_vlan_trunks_add(unsigned int  gid,
                                const char   *oid,
                                const char   *value,
                                const char   *ovs,
                                const char   *bridge_name,
                                const char   *port_name,
                                const char   *unused,
                                const char   *tag_str)
{
    port_entry      *port;
    bridge_entry    *bridge;
    ovs_ctx_t       *ctx;
    te_errno         rc;
    int              tag;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused);
    UNUSED(value);

    rc = te_strtoi(tag_str, 0, &tag);
    if (rc != 0)
        return rc;

    if (tag < 0 || tag >= PORT_MAX_TRUNKS)
    {
        ERROR("Invalid VLAN trunk: %d", tag);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (port->vlan.trunks[tag])
    {
        ERROR("VLAN trunk with tag %d is already added", tag);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    rc = ovs_configure_value(ctx, "port", port->name, "trunks",
                             OVS_CFG_ADD, "%d", tag);
    if (rc != 0)
        return rc;

    port->vlan.trunks[tag] = TRUE;

    return 0;
}

static te_errno
ovs_bridge_port_vlan_trunks_del(unsigned int  gid,
                                const char   *oid,
                                const char   *ovs,
                                const char   *bridge_name,
                                const char   *port_name,
                                const char   *unused,
                                const char   *tag_str)
{
    port_entry      *port;
    bridge_entry    *bridge;
    ovs_ctx_t       *ctx;
    te_errno         rc;
    int              tag;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused);

    rc = te_strtoi(tag_str, 0, &tag);
    if (rc != 0)
        return rc;

    if (tag < 0 || tag >= PORT_MAX_TRUNKS)
    {
        ERROR("Invalid VLAN trunk: %d", tag);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = ovs_bridge_port_pick(ovs, bridge_name, port_name,
                              &ctx, &bridge, &port);
    if (rc != 0)
    {
        ERROR("Failed to pick the port entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (!port->vlan.trunks[tag])
    {
        ERROR("VLAN trunk with tag %d is not present", tag);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = ovs_configure_value(ctx, "port", port->name, "trunks",
                             OVS_CFG_REMOVE, "%d", tag);
    if (rc != 0)
        return rc;

    port->vlan.trunks[tag] = FALSE;

    return 0;
}

RCF_PCH_CFG_NODE_COLLECTION(node_ovs_bridge_port_vlan_trunks, "trunk",
                            NULL, NULL,
                            ovs_bridge_port_vlan_trunks_add,
                            ovs_bridge_port_vlan_trunks_del,
                            ovs_bridge_port_vlan_trunks_list, NULL);

RCF_PCH_CFG_NODE_RW(node_ovs_bridge_port_vlan_tag, "tag",
                    NULL, &node_ovs_bridge_port_vlan_trunks,
                    ovs_bridge_port_vlan_tag_get, ovs_bridge_port_vlan_tag_set);

RCF_PCH_CFG_NODE_RW(node_ovs_bridge_port_vlan, "vlan",
                    &node_ovs_bridge_port_vlan_tag, NULL,
                    ovs_bridge_port_vlan_get, ovs_bridge_port_vlan_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ovs_bridge_port, "port",
                               &node_ovs_bridge_port_vlan, NULL,
                               ovs_bridge_port_get, NULL,
                               ovs_bridge_port_add, ovs_bridge_port_del,
                               ovs_bridge_port_list, NULL);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ovs_bridge, "bridge",
                               &node_ovs_bridge_port, NULL,
                               ovs_bridge_get, NULL,
                               ovs_bridge_add, ovs_bridge_del,
                               ovs_bridge_list, NULL);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ovs_interface_tunnel_options, "tunnel_option",
                    NULL, NULL,
                    ovs_interface_tunnel_option_get, NULL,
                    ovs_interface_tunnel_option_add,
                    ovs_interface_tunnel_option_del,
                    ovs_interface_tunnel_option_list, NULL);

RCF_PCH_CFG_NODE_RW(node_ovs_interface_dpdk_devargs, "dpdk_devargs",
                    NULL, &node_ovs_interface_tunnel_options,
                    ovs_interface_dpdk_devargs_get,
                    ovs_interface_dpdk_devargs_set);

RCF_PCH_CFG_NODE_RW(node_ovs_interface_vhost_server_path, "vhost_server_path",
                    NULL, &node_ovs_interface_dpdk_devargs,
                    ovs_interface_vhost_server_path_get,
                    ovs_interface_vhost_server_path_set);

RCF_PCH_CFG_NODE_RW(node_ovs_interface_link_state, "link_state",
                    NULL, &node_ovs_interface_vhost_server_path,
                    ovs_interface_link_state_get, NULL);

RCF_PCH_CFG_NODE_RW(node_ovs_interface_mtu, "mtu",
                    NULL, &node_ovs_interface_link_state,
                    ovs_interface_mtu_get, ovs_interface_mtu_set);

RCF_PCH_CFG_NODE_RW(node_ovs_interface_ofport, "ofport",
                    NULL, &node_ovs_interface_mtu,
                    ovs_interface_ofport_get, ovs_interface_ofport_set);

RCF_PCH_CFG_NODE_RW(node_ovs_interface_mac, "mac",
                    NULL, &node_ovs_interface_ofport,
                    ovs_interface_mac_get, ovs_interface_mac_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ovs_interface, "interface",
                               &node_ovs_interface_mac, &node_ovs_bridge,
                               ovs_interface_get, ovs_interface_set,
                               ovs_interface_add, ovs_interface_del,
                               ovs_interface_list, NULL);

static rcf_pch_cfg_object node_ovs_log = {
    "log", 0, NULL, &node_ovs_interface,
    (rcf_ch_cfg_get)ovs_log_get, (rcf_ch_cfg_set)ovs_log_set,
    NULL, NULL, (rcf_ch_cfg_list)ovs_log_list, NULL, NULL, NULL
};

RCF_PCH_CFG_NODE_RW(node_ovs_status, "status",
                    NULL, &node_ovs_log, ovs_status_get, ovs_status_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ovs_other_config, "other_config",
                               NULL, &node_ovs_status,
                               ovs_other_config_get, NULL,
                               ovs_other_config_add, ovs_other_config_del,
                               ovs_other_config_list, NULL);

RCF_PCH_CFG_NODE_RW(node_ovs_dpdk_init, "dpdk_init",
                    NULL, &node_ovs_other_config,
                    ovs_dpdk_init_get, ovs_dpdk_init_set);

RCF_PCH_CFG_NODE_NA(node_ovs, "ovs", &node_ovs_dpdk_init, NULL);

static void
ovs_cleanup_static_ctx(void)
{
    INFO("Clearing the facility static context");

    te_kvpair_fini(&ovs_ctx.other_cfg);
    te_string_free(&ovs_ctx.ovs_cfg_cmd);
    te_string_free(&ovs_ctx.vlog_list_cmd);
    te_string_free(&ovs_ctx.vswitchd_cmd);
    te_string_free(&ovs_ctx.dbserver_cmd);
    te_string_free(&ovs_ctx.dbtool_cmd);
    te_string_free(&ovs_ctx.env);
    te_string_free(&ovs_ctx.conf_db_path);
    te_string_free(&ovs_ctx.conf_db_schema);
    te_string_free(&ovs_ctx.conf_db_lock_path);
    te_string_free(&ovs_ctx.root_path);
}

static te_errno
ovs_create_env_shell_arguments(te_string *env, const char *ta_dir)
{
    const char *env_vars[] = {"OVS_RUNDIR", "OVS_DBDIR", "OVS_PKGDATADIR"};
    te_string str = TE_STRING_INIT;
    unsigned int i;
    te_errno rc;

    for (i = 0; i < TE_ARRAY_LEN(env_vars); i++)
    {
        rc = te_string_append(&str, "%s=", env_vars[i]);
        if (rc == 0)
            rc = te_string_append_shell_arg_as_is(&str, ta_dir);

        if (rc == 0)
            rc = te_string_append(&str, " ");

        if (rc != 0)
            goto out;
    }

    rc = te_string_append(&str, "PATH=");
    if (rc == 0)
        rc = te_string_append_shell_arg_as_is(&str, ta_dir);

    if (rc == 0)
        rc = te_string_append(&str, "\":${PATH}\"");

    if (rc == 0)
        rc = te_string_append(env, "%s", str.ptr);

out:
    te_string_free(&str);

    return rc;
}

te_errno
ta_unix_conf_ovs_init(void)
{
    te_errno rc;

    INFO("Initialising the facility static context");

    rc = te_string_append(&ovs_ctx.root_path, "%s", ta_dir);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.conf_db_lock_path,
                          "%s/.conf.db.~lock~", ta_dir);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.conf_db_schema, "%s/vswitch.ovsschema",
                          ta_dir);
    if (rc != 0)
        goto fail;
    if (access(ovs_ctx.conf_db_schema.ptr, R_OK) != 0)
    {
        const char *def_schema = "/usr/share/openvswitch/vswitch.ovsschema";

        if (access(def_schema, R_OK) != 0)
        {
            WARN("No DB schema available: neither %s nor %s - drop OvS support",
                 ovs_ctx.conf_db_schema.ptr, def_schema);
            goto fail;
        }

        te_string_reset(&ovs_ctx.conf_db_schema);
        rc = te_string_append(&ovs_ctx.conf_db_schema, def_schema);
        if (rc != 0)
            goto fail;
    }

    rc = te_string_append(&ovs_ctx.conf_db_path, "%s/conf.db", ta_dir);
    if (rc != 0)
        goto fail;

    rc = ovs_create_env_shell_arguments(&ovs_ctx.env, ta_dir);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.dbtool_cmd,
                          "%s ovsdb-tool create %s %s", ovs_ctx.env.ptr,
                          ovs_ctx.conf_db_path.ptr,
                          ovs_ctx.conf_db_schema.ptr);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.dbserver_cmd, "%s ovsdb-server "
                          "--remote=punix:db.sock --pidfile", ovs_ctx.env.ptr);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.vswitchd_cmd,
                          "%s ovs-vswitchd --pidfile", ovs_ctx.env.ptr);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.vlog_list_cmd,
                          "%s ovs-appctl -t ovs-vswitchd "
                          "vlog/list | tail -n +3", ovs_ctx.env.ptr);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.ovs_cfg_cmd,
                          "%s ovs-vsctl --no-wait set Open_vSwitch . ",
                          ovs_ctx.env.ptr);

    if (rc == 0)
        return rcf_pch_add_node("/agent", &node_ovs);

fail:
    if (rc != 0)
        ERROR("Failed to initialise the facility");
    ovs_cleanup_static_ctx();

    return TE_RC(TE_TA_UNIX, rc);
}
