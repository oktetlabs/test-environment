/** @file
 * @brief RPC client API for DPDK EAL
 *
 * RPC client API for DPDK EAL functions.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#include "te_config.h"
#include "te_ethernet.h"
#include "te_alloc.h"

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_env.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte_eal.h"
#include "tapi_rpc_rte_ethdev.h"
#include "tapi_rpc_rte_mempool.h"
#include "tapi_cfg_pci.h"

#include "tarpc.h"

#include "rpcc_dpdk.h"

static te_errno
tapi_eal_rpcs_set_cached_eal_args(const rcf_rpc_server *rpcs,
                                  const char *eal_args)
{
    return cfg_set_instance_fmt(CVT_STRING, eal_args,
                                "/agent:%s/rpcserver:%s/config:",
                                rpcs->ta, rpcs->name);
}

static te_errno
tapi_eal_rpcs_get_cached_eal_args(const rcf_rpc_server *rpcs, char **eal_args)
{
    cfg_val_type val_type = CVT_STRING;

    return cfg_get_instance_fmt(&val_type, eal_args,
                                "/agent:%s/rpcserver:%s/config:",
                                rpcs->ta, rpcs->name);
}

static te_errno
tapi_eal_rpcs_reset_cached_eal_args(const rcf_rpc_server *rpcs)
{
    te_bool already_reset = FALSE;
    char *old_args;
    te_errno rc;

    /*
     * Cached EAL arguments are get before resetting to avoid resetting already
     * empty value. This way the log and configurator dynamic history will not
     * be polluted with the same set requests.
     */
    rc = tapi_eal_rpcs_get_cached_eal_args(rpcs, &old_args);
    if (rc != 0)
        return rc;

    already_reset = (strlen(old_args) == 0);
    free(old_args);

    if (already_reset)
        return 0;

    return tapi_eal_rpcs_set_cached_eal_args(rpcs, "");
}


te_errno
tapi_rte_get_dev_args(const char *ta, const char *vendor, const char *device,
                      char **arg_list)
{
    char *generic_args = NULL;
    char *vendor_args = NULL;
    char *device_args = NULL;
    char **args[] = { &generic_args, &vendor_args, &device_args };
    te_string result = TE_STRING_INIT;
    unsigned int i;
    te_bool first;
    te_errno rc;

    rc = cfg_get_instance_fmt(NULL, &generic_args,
                              "/local:%s/dpdk:/dev_args:pci_fn:::", ta);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
        return rc;

    if (vendor != NULL && vendor[0] != '\0')
    {
        rc = cfg_get_instance_fmt(NULL, &vendor_args,
                                  "/local:%s/dpdk:/dev_args:pci_fn:%s::",
                                  ta, vendor);
        if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
            return rc;


        if (device != NULL && device[0] != '\0')
        {
            rc = cfg_get_instance_fmt(NULL, &device_args,
                                      "/local:%s/dpdk:/dev_args:pci_fn:%s:%s:",
                                      ta, vendor, device);
            if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
                return rc;
        }
    }

    rc = 0;

    for (first = TRUE, i = 0; i < TE_ARRAY_LEN(args) && rc == 0; i++)
    {
        if (*args[i] == NULL || (*args[i])[0] == '\0')
            continue;

        rc = te_string_append(&result, "%s%s", first ? "" : ",", *args[i]);
        first = FALSE;
    }

    for (i = 0; i < TE_ARRAY_LEN(args) && rc == 0; i++)
        free(*args[i]);

    if (rc != 0)
    {
        ERROR("Failed to get dev args: %r", rc);
        te_string_free(&result);
        return rc;
    }

    if (result.len == 0)
    {
        te_string_free(&result);
        *arg_list = NULL;
    }
    else
    {
        *arg_list = result.ptr;
    }

    return 0;
}

te_errno
tapi_rte_get_dev_args_by_pci_addr(const char *ta, const char *pci_addr,
                                  char **arg_list)
{
    char *vendor;
    char *device;
    te_errno rc;

    rc = tapi_cfg_pci_get_pci_vendor_device(ta, pci_addr, &vendor, &device);
    if (rc != 0)
        return rc;

    rc = tapi_rte_get_dev_args(ta, vendor, device, arg_list);

    free(vendor);
    free(device);

    return rc;
}

te_errno
tapi_rte_eal_hotplug_add(rcf_rpc_server *rpcs, const char *busname,
                         const char *devname, const char *devargs)
{
    te_errno rc;

    rc = rpc_rte_eal_hotplug_add(rpcs, busname, devname, devargs);
    if (rc != 0)
        return rc;

    return tapi_eal_rpcs_reset_cached_eal_args(rpcs);
}

te_errno
tapi_rte_eal_hotplug_remove(rcf_rpc_server *rpcs, const char *busname,
                            const char *devname)
{
    te_errno rc;

    rc = rpc_rte_eal_hotplug_remove(rpcs, busname, devname);
    if (rc != 0)
        return rc;

    return tapi_eal_rpcs_reset_cached_eal_args(rpcs);
}

int
rpc_rte_eal_init(rcf_rpc_server *rpcs,
                 int argc, char **argv)
{
    tarpc_rte_eal_init_in   in;
    tarpc_rte_eal_init_out  out;
    int                     i;
    te_log_buf             *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.argc = argc;
    in.argv.argv_len = argc;
    in.argv.argv_val = calloc(argc, sizeof(*in.argv.argv_val));
    for (i = 0; i < argc; ++i)
        in.argv.argv_val[i].str = argv[i];

    rcf_rpc_call(rpcs, "rte_eal_init", &in, &out);

    free(in.argv.argv_val);

    /*
     * Number of processed arguments cannot be greater that number of
     * passed arguments.
     */
    CHECK_RETVAL_VAR_ERR_COND(rte_eal_init, out.retval,
                              out.retval > argc, RETVAL_ECORRUPTED,
                              out.retval < 0);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eal_init, "%d, %p(%s)", NEG_ERRNO_FMT,
                 in.argc, argv,
                 te_args2log_buf(tlbp, in.argc, (const char **)argv),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_INT(rte_eal_init, out.retval);
}

/**
 * Append a new argument to the argv array.
 *
 * Argument string is allocated and should be freed.
 */
static void
append_arg(int *argc_p, char ***argv_p, const char *fmt, ...)
{
    int         argc = *argc_p;
    char      **argv = *argv_p;
    va_list     ap;

    argv = realloc(argv, (argc + 1) * sizeof(*argv));

    va_start(ap, fmt);
    te_vasprintf(&(argv[argc]), fmt, ap);
    va_end(ap);

    *argc_p = argc + 1;
    *argv_p = argv;
}

static te_errno tapi_eal_get_vdev_slaves(tapi_env               *env,
                                         const tapi_env_ps_if   *ps_if,
                                         char                 ***slavespp,
                                         unsigned int           *nb_slavesp);

static te_errno
tapi_eal_close_bond_slave_pci_devices(tapi_env             *env,
                                      const tapi_env_ps_if *ps_if,
                                      rcf_rpc_server       *rpcs)
{
    char         **slaves = NULL;
    unsigned int   nb_slaves;
    te_errno       rc;
    unsigned int   i;

    rc = tapi_eal_get_vdev_slaves(env, ps_if, &slaves, &nb_slaves);
    if (rc != 0)
        goto out;

    for (i = 0; i < nb_slaves; ++i)
    {
        size_t name_len = strlen(slaves[i]);

        if (name_len > 1 && slaves[i][name_len - 1 - 1] == '.')
        {
            char *dev_args;
            uint16_t port_id;

            RPC_AWAIT_ERROR(rpcs);
            rc = rpc_rte_eth_dev_get_port_by_name(rpcs, slaves[i], &port_id);
            if (rc == 0)
                rpc_rte_eth_dev_close(rpcs, port_id);
            else if (rc != -TE_RC(TE_RPC, TE_ENODEV))
                goto out;

            /* If ethdev is removed, its PCI device can still be plugged in. */
            RPC_AWAIT_ERROR(rpcs);
            rc = rpc_rte_eal_hotplug_remove(rpcs, "pci", slaves[i]);
            if (rc != 0 && rc != -TE_RC(TE_RPC, TE_ENODEV) &&
                /* -EINVAL is returned in the case of no PCI device */
                rc != -TE_RC(TE_RPC, TE_EINVAL))
                goto out;

            rc = tapi_rte_get_dev_args_by_pci_addr(rpcs->ta, slaves[i],
                                                   &dev_args);
            if (rc != 0)
                goto out;

            rc = rpc_rte_eal_hotplug_add(rpcs, "pci", slaves[i], dev_args);
            free(dev_args);
            if (rc != 0)
                goto out;
        }
    }

out:
    free(slaves);

    return rc;
}

static te_errno
tapi_reuse_eal(tapi_env         *env,
               rcf_rpc_server   *rpcs,
               tapi_env_ps_ifs  *ifsp,
               int               argc,
               char             *argv[],
               te_bool          *need_init,
               char            **eal_args_out)
{
    size_t                eal_args_len;
    char                 *eal_args = NULL;
    char                 *eal_args_pos;
    char                 *eal_args_cfg = NULL;
    const tapi_env_ps_if *ps_if;
    char                 *da_alloc = NULL;
    const char           *da_generic = NULL;
    te_errno              rc = 0;
    unsigned int          i;

    if ((argc <= 0) || (argv == NULL))
    {
        rc = TE_EINVAL;
        goto out;
    }

    for (i = 0, eal_args_len = 1;
         i < (unsigned int)argc;
         eal_args_len += strlen(argv[i++]) + 1);

    eal_args = TE_ALLOC(eal_args_len);
    if (eal_args == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    eal_args_pos = eal_args;
    for (i = 0; i < (unsigned int)argc; ++i)
    {
        size_t len = strlen(argv[i]);

        eal_args_pos[0] = ' ';
        memcpy(eal_args_pos + 1, argv[i], len);
        eal_args_pos += len + 1;
    }
    /* Buffer is filled in with zeros anyway, but anyway */
    eal_args_pos[0] = '\0';

    rc = tapi_eal_rpcs_get_cached_eal_args(rpcs, &eal_args_cfg);
    if (rc != 0)
        goto out;

    if (strlen(eal_args_cfg) == 0 || strcmp(eal_args, eal_args_cfg) != 0)
    {
        rc = rcf_rpc_server_restart(rpcs);
        if (rc == 0)
        {
            *need_init = TRUE;
            *eal_args_out = eal_args;
        }
        goto out;
    }

    STAILQ_FOREACH(ps_if, ifsp, links)
    {
        const char *bus_name = NULL;
        const char *dev_name = ps_if->iface->if_info.if_name;
        uint16_t    port_id;
        const char *net_bonding_prefix = "net_bonding";

        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
            bus_name = "pci";
        else if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
            bus_name = "vdev";
        else
            continue;

        RPC_AWAIT_ERROR(rpcs);
        rc = rpc_rte_eth_dev_get_port_by_name(rpcs, dev_name, &port_id);
        if (rc == 0)
            rpc_rte_eth_dev_close(rpcs, port_id);
        else if (rc != -TE_RC(TE_RPC, TE_ENODEV))
            goto out;

        /* If ethdev is removed, its PCI device can still be plugged in. */
        RPC_AWAIT_ERROR(rpcs);
        rc = rpc_rte_eal_hotplug_remove(rpcs, bus_name, dev_name);
        if (rc != 0 && rc != -TE_RC(TE_RPC, TE_ENODEV) &&
            /* -EINVAL is returned in the case of no PCI device */
            rc != -TE_RC(TE_RPC, TE_EINVAL))
            goto out;

        if (strncmp(dev_name, net_bonding_prefix,
                    strlen(net_bonding_prefix)) == 0)
        {
            rc = tapi_eal_close_bond_slave_pci_devices(env, ps_if, rpcs);
            if (rc != 0)
                goto out;
        }
    }

    rpc_rte_mempool_free_all(rpcs);

    STAILQ_FOREACH(ps_if, ifsp, links)
    {
        const char *bus_name = NULL;
        const char *dev_name = ps_if->iface->if_info.if_name;
        const char *da_empty = "";

        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
        {
            bus_name = "pci";

            free(da_alloc);
            da_alloc = NULL;
            rc = tapi_rte_get_dev_args_by_pci_addr(rpcs->ta, dev_name,
                                                   &da_alloc);
            if (rc != 0)
                goto out;

            da_generic = da_alloc;

        }
        else if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
        {
            bus_name = "vdev";
            da_generic = NULL;

            for (i = 0; i < (unsigned int)argc; ++i)
            {
                if (strncmp(argv[i], dev_name, strlen(dev_name)) == 0)
                {
                    da_generic = strchr(argv[i], ',') + 1;
                    break;
                }
            }
        }
        else
        {
            continue;
        }

        da_generic = (da_generic == NULL) ? da_empty : da_generic;
        rc = rpc_rte_eal_hotplug_add(rpcs, bus_name, dev_name, da_generic);
        if (rc != 0)
            goto out;
    }

    *need_init = FALSE;
    *eal_args_out = NULL;

    free(eal_args);

out:
    free(da_alloc);
    free(eal_args_cfg);

    if (rc != 0)
        free(eal_args);

    return rc;
}

static te_errno
tapi_eal_get_vdev_slave_pci_addr(cfg_handle   slave_handle,
                                 char       **pci_addr_strp)
{
    cfg_val_type  val_type;
    char         *slave_inst_val = NULL;
    char         *slave_pci_inst_val = NULL;
    te_errno      rc = 0;

    val_type = CVT_STRING;
    rc = cfg_get_instance(slave_handle, &val_type, &slave_inst_val);
    if (rc != 0)
        return rc;

    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(&val_type, &slave_pci_inst_val,
                              "%s", slave_inst_val);
    if (rc != 0)
        goto out;

    rc = cfg_get_ith_inst_name(slave_pci_inst_val, 4, pci_addr_strp);

out:
    free(slave_pci_inst_val);
    free(slave_inst_val);

    return rc;
}

static te_errno
tapi_eal_get_vdev_slaves(tapi_env               *env,
                         const tapi_env_ps_if   *ps_if,
                         char                 ***slavespp,
                         unsigned int           *nb_slavesp)
{
    cfg_nets_t      *cfg_nets = &env->cfg_nets;
    tapi_env_net    *net = ps_if->iface->net;
    cfg_net_node_t  *node;
    cfg_val_type     val_type;
    char            *node_val = NULL;
    unsigned int     nb_slaves = 0;
    cfg_handle      *slave_handles = NULL;
    char           **slaves = NULL;
    te_errno         rc = 0;
    unsigned int     i;

    node = &cfg_nets->nets[net->i_net].nodes[ps_if->iface->i_node];

    val_type = CVT_STRING;
    rc = cfg_get_instance(node->handle, &val_type, &node_val);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern_fmt(&nb_slaves, &slave_handles,
                              "%s/slave:*", node_val);
    if (rc != 0)
        goto out;

    slaves = TE_ALLOC(nb_slaves * sizeof(*slaves));
    if (slaves == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < nb_slaves; ++i)
    {
        rc = tapi_eal_get_vdev_slave_pci_addr(slave_handles[i], &slaves[i]);
        if (rc != 0)
        {
            unsigned int j;

            for (j = 0; j < i; ++j)
                free(slaves[i]);

            free(slaves);
            goto out;
        }
    }

    *slavespp = slaves;
    *nb_slavesp = nb_slaves;

out:
    free(slave_handles);
    free(node_val);

    return rc;
}

static te_errno
tapi_eal_get_vdev_properties(tapi_env              *env,
                             const tapi_env_ps_if  *ps_if,
                             char                 **namep,
                             char                 **modep,
                             te_bool               *is_bondingp)
{
    cfg_nets_t     *cfg_nets = &env->cfg_nets;
    tapi_env_net   *net = ps_if->iface->net;
    cfg_net_node_t *node;
    cfg_val_type    val_type;
    char           *node_val = NULL;
    char           *name = NULL;
    char           *mode = NULL;
    const char      name_prefix_bonding[] = "net_bonding";
    te_errno        rc = 0;

    node = &cfg_nets->nets[net->i_net].nodes[ps_if->iface->i_node];

    val_type = CVT_STRING;
    rc = cfg_get_instance(node->handle, &val_type, &node_val);
    if (rc != 0)
        return rc;

    rc = cfg_get_ith_inst_name(node_val, 3, &name);
    if (rc != 0)
        goto out;

    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(&val_type, &mode, "%s/mode:", node_val);
    if (rc != 0)
        goto out;

    *namep = name;
    *modep = mode;

    if (strncmp(name, name_prefix_bonding, strlen(name_prefix_bonding)) == 0)
        *is_bondingp = TRUE;
    else
        *is_bondingp = FALSE;

out:
    if (rc != 0)
        free(name);

    free(node_val);

    return rc;
}

static te_errno
tapi_eal_mk_vdev_slave_list_str(tapi_env              *env,
                                const tapi_env_ps_if  *ps_if,
                                te_bool                is_bonding,
                                char                 **slave_list_strp,
                                const char            *ta)
{
    const char     prefix_bonding[] = ",slave=";
    const char     prefix_failsafe[] = ",dev(";
    const char     postfix_bonding[] = "";
    const char     postfix_failsafe[] = ")";
    const char    *prefix = NULL;
    const char    *postfix = NULL;
    char         **slaves = NULL;
    unsigned int   nb_slaves = 0;
    size_t         slave_list_str_len = 0;
    char          *slave_list_str = NULL;
    te_errno       rc = 0;
    unsigned int   i;

    prefix = (is_bonding) ? prefix_bonding : prefix_failsafe;
    postfix = (is_bonding) ? postfix_bonding : postfix_failsafe;

    rc = tapi_eal_get_vdev_slaves(env, ps_if, &slaves, &nb_slaves);
    if (rc != 0)
        return rc;

    for (i = 0; i < nb_slaves; ++i)
    {
        char *dev_args_failsafe = NULL;
        char   *slave_list_str_new;
        size_t  slave_list_str_len_new = slave_list_str_len;
        size_t  len;
        int     ret;

        slave_list_str_len_new += strlen(prefix);
        slave_list_str_len_new += strlen(slaves[i]);
        if (!is_bonding)
        {
            rc = tapi_rte_get_dev_args_by_pci_addr(ta, slaves[i],
                                                   &dev_args_failsafe);
            if (rc != 0)
                goto out;
        }
        if (dev_args_failsafe != NULL)
        {
            ++slave_list_str_len_new;
            slave_list_str_len_new += strlen(dev_args_failsafe);
        }
        slave_list_str_len_new += strlen(postfix);

        slave_list_str_new = realloc(slave_list_str,
                                     slave_list_str_len_new + 1);
        if (slave_list_str_new == NULL)
        {
            rc = TE_ENOMEM;
            free(dev_args_failsafe);
            goto out;
        }
        slave_list_str = slave_list_str_new;

        len = slave_list_str_len_new - slave_list_str_len;
        ret = snprintf(slave_list_str + slave_list_str_len, len + 1,
                       "%s%s%s%s%s", prefix, slaves[i],
                       (dev_args_failsafe != NULL) ? "," : "",
                       (dev_args_failsafe != NULL) ? dev_args_failsafe : "",
                       postfix);
        free(dev_args_failsafe);
        if (ret < 0 || (size_t)ret != len)
        {
            rc = TE_EINVAL;
            goto out;
        }

        slave_list_str_len = slave_list_str_len_new;
    }

    *slave_list_strp = slave_list_str;

out:
    if (rc != 0)
        free(slave_list_str);

    for (i = 0; i < nb_slaves; ++i)
        free(slaves[i]);

    free(slaves);

    return rc;
}

static te_errno
tapi_eal_add_vdev_args(tapi_env          *env,
                       tapi_env_ps_ifs   *ifsp,
                       int               *argcp,
                       char            ***argvpp,
                       const char        *ta)
{
    const tapi_env_ps_if *ps_if;
    te_errno              rc = 0;

    STAILQ_FOREACH(ps_if, ifsp, links)
    {
        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
        {
            char    *name = NULL;
            char    *mode = NULL;
            te_bool  is_bonding;
            char    *slave_list_str = NULL;

            rc = tapi_eal_get_vdev_properties(env, ps_if, &name, &mode,
                                              &is_bonding);
            if (rc != 0)
                return rc;

            rc = tapi_eal_mk_vdev_slave_list_str(env, ps_if, is_bonding,
                                                 &slave_list_str, ta);
            if (rc != 0)
            {
                free(mode);
                free(name);
                return rc;
            }

            append_arg(argcp, argvpp, "--vdev");
            append_arg(argcp, argvpp, "%s%s%s%s", name,
                       (is_bonding) ? ",mode=" : "",
                       (is_bonding) ? mode : "",
                       slave_list_str);

           free(slave_list_str);
           free(mode);
           free(name);
        }
    }

    return 0;
}

static te_errno
tapi_eal_whitelist_vdev_slaves(tapi_env               *env,
                               const tapi_env_ps_if   *ps_if,
                               char                   *ta,
                               int                    *argcp,
                               char                 ***argvpp)
{
    char         **slaves = NULL;
    unsigned int   nb_slaves = 0;
    char          *dev_args;
    te_errno       rc = 0;
    unsigned int   i;

    rc = tapi_eal_get_vdev_slaves(env, ps_if, &slaves, &nb_slaves);
    if (rc != 0)
        return rc;

    for (i = 0; i < nb_slaves; ++i)
    {
        rc = tapi_rte_get_dev_args_by_pci_addr(ta, slaves[i], &dev_args);
        if (rc != 0)
            return rc;

        append_arg(argcp, argvpp, "--pci-whitelist=%s%s%s", slaves[i],
                   (dev_args == NULL) ? "" : ",",
                   (dev_args == NULL) ? "" : dev_args);

        free(dev_args);
        free(slaves[i]);
    }

    free(slaves);

    return 0;
}

static te_errno
tapi_eal_get_vdev_port_ids(rcf_rpc_server  *rpcs,
                           tapi_env_ps_ifs *ifsp)
{
    const tapi_env_ps_if *ps_if;

    STAILQ_FOREACH(ps_if, ifsp, links)
    {
        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
        {
            const char *name = ps_if->iface->if_info.if_name;
            uint16_t    port_id;
            int         ret = 0;

            ret = rpc_rte_eth_dev_get_port_by_name(rpcs, name, &port_id);
            if (ret != 0)
                return TE_RC(TE_TAPI, -ret);

            ps_if->iface->if_info.if_index = port_id;
        }
    }

    return 0;
}

te_errno
tapi_rte_make_eal_args(tapi_env *env, rcf_rpc_server *rpcs,
                       const char *program_name,
                       int argc, const char **argv,
                       int *out_argc, char ***out_argv)
{
    te_errno                rc = 0;
    const tapi_env_pco     *pco;
    char                  **my_argv = NULL;
    int                     my_argc = 0;
    const tapi_env_ps_if   *ps_if;
    int                     i;
    cfg_val_type            val_type;
    int                     mem_channels;
    int                     mem_amount = 0;
    char                   *app_prefix = NULL;
    char                   *extra_eal_args = NULL;

    if (env == NULL || rpcs == NULL)
        return TE_EINVAL;

    pco = tapi_env_rpcs2pco(env, rpcs);
    if (pco == NULL)
        return TE_EINVAL;

    /* Use RPC server name as a program name if it is not provided */
    append_arg(&my_argc, &my_argv, "%s",
               program_name == NULL ? rpcs->name : program_name);

    /* Append vdev-related arguments should the need arise */
    rc = tapi_eal_add_vdev_args(env, &pco->process->ifs, &my_argc, &my_argv,
                                rpcs->ta);
    if (rc != 0)
        goto cleanup;

     /* Specify PCI whitelist or virtual device information */
    STAILQ_FOREACH(ps_if, &pco->process->ifs, links)
    {
        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
        {
            char *dev_args;

            rc = tapi_rte_get_dev_args_by_pci_addr(rpcs->ta,
                                                   ps_if->iface->if_info.if_name,
                                                   &dev_args);
            if (rc != 0)
                goto cleanup;

            append_arg(&my_argc, &my_argv, "--pci-whitelist=%s%s%s",
                       ps_if->iface->if_info.if_name,
                       (dev_args == NULL) ? "" : ",",
                       (dev_args == NULL) ? "" : dev_args);
            free(dev_args);
        }
        else if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
        {
            rc = tapi_eal_whitelist_vdev_slaves(env, ps_if, rpcs->ta,
                                                &my_argc, &my_argv);
            if (rc != 0)
                goto cleanup;
        }
    }

    /* Add memory channels information */
    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type, &mem_channels,
                              "/local:%s/mem_channels:", rpcs->ta);
    if (rc != 0)
        goto cleanup;

    append_arg(&my_argc, &my_argv, "-n");
    append_arg(&my_argc, &my_argv, "%d", mem_channels);

    /* Set the amount of memory (in Megabytes) to be booked within hugetables */
    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type, &mem_amount,
                              "/local:%s/dpdk:/mem_amount:", rpcs->ta);
    if (rc == 0 && mem_amount > 0)
    {
        append_arg(&my_argc, &my_argv, "-m");
        append_arg(&my_argc, &my_argv, "%d", mem_amount);
    }

    /* Specify DPDK app prefix */
    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(&val_type, &app_prefix,
                              "/local:%s/dpdk:/app_prefix:", rpcs->ta);
    if (rc == 0 && app_prefix != NULL)
    {
        append_arg(&my_argc, &my_argv, "--file-prefix");
        append_arg(&my_argc, &my_argv, "%s%s", app_prefix, rpcs->ta);

        free(app_prefix);
    }

    /* Append extra EAL args */
    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(&val_type, &extra_eal_args,
                              "/local:%s/dpdk:/extra_eal_args:", rpcs->ta);
    if (rc == 0)
    {
        char *sp = NULL;
        char *arg = NULL;

        if (extra_eal_args[0] != '\0')
        {
            char *str = extra_eal_args;

            while ((arg = strtok_r(str, " ", &sp)) != NULL)
            {
                append_arg(&my_argc, &my_argv, arg);
                str = NULL;
            }
        }

        free(extra_eal_args);
    }
    else if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        goto cleanup;
    }
    rc = 0;

    /* Append arguments provided by caller */
    for (i = 0; i < argc; ++i)
    {
        if (argv[i] == NULL)
        {
            my_argv = realloc(my_argv, (my_argc + 1) * sizeof(*my_argv));
            my_argv[my_argc] = NULL;
            my_argc++;
        }
        else
        {
            append_arg(&my_argc, &my_argv, "%s", argv[i]);
        }
    }

    *out_argc = my_argc;
    *out_argv = my_argv;

cleanup:
    if (rc != 0)
    {
        for (i = 0; i < my_argc; ++i)
            free(my_argv[i]);
        free(my_argv);
    }

    return rc;
}

te_errno
tapi_rte_eal_init(tapi_env *env, rcf_rpc_server *rpcs,
                  int argc, const char **argv)
{
    te_errno                rc;
    const tapi_env_pco     *pco;
    char                  **my_argv = NULL;
    int                     my_argc = 0;
    int                     ret;
    int                     i;
    te_bool                 need_init = TRUE;
    char                   *eal_args_new = NULL;

    if (env == NULL || rpcs == NULL)
        return TE_EINVAL;

    pco = tapi_env_rpcs2pco(env, rpcs);
    if (pco == NULL)
        return TE_EINVAL;

    rc = tapi_rte_make_eal_args(env, rpcs, NULL, argc, argv, &my_argc, &my_argv);
    if (rc != 0)
        return rc;

    if (dpdk_reuse_rpcs())
    {
        rc = tapi_reuse_eal(env, rpcs, &pco->process->ifs,
                            my_argc, my_argv,
                            &need_init, &eal_args_new);
        if (rc != 0)
            goto cleanup;
    }

    if (need_init)
    {
        ret = rpc_rte_eal_init(rpcs, my_argc, my_argv);

        rc = (ret < 0) ? TE_EFAULT : 0;
        if (rc != 0)
            goto cleanup;

        if (eal_args_new != NULL)
        {
            rc = tapi_eal_rpcs_set_cached_eal_args(rpcs, eal_args_new);
            if (rc != 0)
                goto cleanup;
        }
    }

    /* Obtain port IDs for RTE vdev interfaces. */
    rc = tapi_eal_get_vdev_port_ids(rpcs, &pco->process->ifs);

cleanup:
    free(eal_args_new);

    for (i = 0; i < my_argc; ++i)
        free(my_argv[i]);

    free(my_argv);

    return rc;
}

static te_bool
tarpc_rte_proc_type_t_valid(enum tarpc_rte_proc_type_t val)
{
    return val == TARPC_RTE_PROC_AUTO ||
           val == TARPC_RTE_PROC_PRIMARY ||
           val == TARPC_RTE_PROC_SECONDARY;
}

const char *
tarpc_rte_proc_type_t2str(enum tarpc_rte_proc_type_t val)
{
    switch (val)
    {
        case TARPC_RTE_PROC_AUTO:
            return "auto";
        case TARPC_RTE_PROC_PRIMARY:
            return "primary";
        case TARPC_RTE_PROC_SECONDARY:
            return "secondary";
        case TARPC_RTE_PROC_INVALID:
            return "invalid";
        case TARPC_RTE_PROC__UNKNOWN:
            return "<UNKNOWN>";
        default:
            return "<GARBAGE>";
    }
}

enum tarpc_rte_proc_type_t
rpc_rte_eal_process_type(rcf_rpc_server *rpcs)
{
    tarpc_rte_eal_process_type_in   in;
    tarpc_rte_eal_process_type_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "rte_eal_process_type", &in, &out);

    CHECK_RETVAL_VAR(rte_eal_process_type, out.retval,
                     !tarpc_rte_proc_type_t_valid(out.retval) &&
                     out.retval != TARPC_RTE_PROC_INVALID,
                     TARPC_RTE_PROC__UNKNOWN);

    TAPI_RPC_LOG(rpcs, rte_eal_process_type, "", "%s",
                 tarpc_rte_proc_type_t2str(out.retval));

    TAPI_RPC_OUT(rte_eal_process_type,
                 !tarpc_rte_proc_type_t_valid(out.retval));

    return out.retval;
}

int
rpc_dpdk_get_version(rcf_rpc_server *rpcs)
{
    tarpc_dpdk_get_version_in  in;
    tarpc_dpdk_get_version_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "dpdk_get_version", &in, &out);

    TAPI_RPC_LOG(rpcs, dpdk_get_version, "", "%d.%02d.%d-%d",
                 out.year, out.month, out.minor, out.release);

    return TAPI_RTE_VERSION_NUM(out.year, out.month, out.minor, out.release);
}

int
rpc_rte_eal_hotplug_add(rcf_rpc_server *rpcs,
                        const char     *busname,
                        const char     *devname,
                        const char     *devargs)
{
    tarpc_rte_eal_hotplug_add_in  in;
    tarpc_rte_eal_hotplug_add_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.busname = tapi_strdup(busname);
    in.devname = tapi_strdup(devname);
    in.devargs = tapi_strdup(devargs == NULL ? "" : devargs);

    rcf_rpc_call(rpcs, "rte_eal_hotplug_add", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eal_hotplug_add, out.retval);

    TAPI_RPC_LOG(rpcs, rte_eal_hotplug_add, "%s; %s; %s", NEG_ERRNO_FMT,
                 in.busname, in.devname, in.devargs,
                 NEG_ERRNO_ARGS(out.retval));

    free(in.busname);
    free(in.devname);
    free(in.devargs);

    RETVAL_ZERO_INT(rte_eal_hotplug_add, out.retval);
}

int
rpc_rte_eal_hotplug_remove(rcf_rpc_server *rpcs,
                           const char     *busname,
                           const char     *devname)
{
    tarpc_rte_eal_hotplug_remove_in  in;
    tarpc_rte_eal_hotplug_remove_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.busname = tapi_strdup(busname);
    in.devname = tapi_strdup(devname);

    rcf_rpc_call(rpcs, "rte_eal_hotplug_remove", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eal_hotplug_remove, out.retval);

    TAPI_RPC_LOG(rpcs, rte_eal_hotplug_remove, "%s; %s", NEG_ERRNO_FMT,
                 in.busname, in.devname, NEG_ERRNO_ARGS(out.retval));

    free(in.busname);
    free(in.devname);

    RETVAL_ZERO_INT(rte_eal_hotplug_remove, out.retval);
}

static const char *
tarpc_rte_epoll_data2str(te_log_buf *tlbp,
                         struct tarpc_rte_epoll_data *epoll_data)
{
    te_log_buf_append(tlbp, "{ event=%u, data=%llu }",
                      epoll_data->event, epoll_data->data);

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_epoll_event2str(te_log_buf *tlbp,
                          struct tarpc_rte_epoll_event *event)
{
    te_log_buf_append(tlbp, "{ status=%u, fd=%d, epfd=%d, epdata=",
                      event->status, event->fd, event->epfd);
    tarpc_rte_epoll_data2str(tlbp, &event->epdata);
    te_log_buf_append(tlbp, " }");

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_epoll_events2str(te_log_buf *tlbp,
                           struct tarpc_rte_epoll_event *events,
                           int event_count)
{
    int i;

    for (i = 0; i < event_count; i++)
    {
        if (i != 0)
            te_log_buf_append(tlbp, ", ");

        tarpc_rte_epoll_event2str(tlbp, &events[i]);
    }

    return te_log_buf_get(tlbp);
}

int
rpc_rte_epoll_wait(rcf_rpc_server *rpcs,
                   int epfd,
                   struct tarpc_rte_epoll_event *events,
                   int maxevents,
                   int timeout)
{
    tarpc_rte_epoll_wait_in     in;
    tarpc_rte_epoll_wait_out    out;
    te_log_buf                 *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.epfd = epfd;
    in.maxevents = maxevents;
    in.timeout = timeout;

    if (events != NULL)
    {
        in.events.events_val = tapi_calloc(maxevents, sizeof(*events));
        in.events.events_len = maxevents;
    }

    rcf_rpc_call(rpcs, "rte_epoll_wait", &in, &out);
    CHECK_RETVAL_VAR_ERR_COND(rte_epoll_wait, out.retval, FALSE,
                              -TE_RC(TE_TAPI, TE_ECORRUPTED), (out.retval < 0));

    if (events != NULL && out.events.events_len != 0)
    {
        memcpy(events, out.events.events_val, out.events.events_len);
    }

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_epoll_wait, "%d, {}, %d, %d",
                 NEG_ERRNO_FMT "; events: { %s }", in.epfd,
                 in.maxevents, in.timeout,
                 NEG_ERRNO_ARGS(out.retval),
                 (out.retval > 0) ?
                 tarpc_rte_epoll_events2str(tlbp, out.events.events_val,
                                            out.retval) : "n/a");
    te_log_buf_free(tlbp);
    free(in.events.events_val);

    RETVAL_INT(rte_epoll_wait, out.retval);
}
