/** @file
 * @brief RPC client API for DPDK EAL
 *
 * RPC client API for DPDK EAL functions.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#include "te_config.h"
#include "te_ethernet.h"
#include "te_alloc.h"
#include "te_vector.h"
#include "te_sleep.h"
#include "te_str.h"

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_env.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte_eal.h"
#include "tapi_rpc_rte_ethdev.h"
#include "tapi_rpc_rte_mempool.h"
#include "tapi_cfg_pci.h"
#include "tapi_cfg_cpu.h"

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
    return cfg_get_instance_string_fmt(eal_args,
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

static te_errno
tapi_rte_get_pci_fn_specifiers(const char *property, const char *ta,
                               const char *vendor, const char *device,
                               te_vec **instance_list)
{
    char empty = '\0';
    char *gen = &empty;
    char *ven = &empty;
    char *dev = &empty;
    char *gen_ta = &empty;
    char *ven_ta = &empty;
    char *dev_ta = &empty;
    char **instances[] = {
        &gen, &ven, &dev, &gen_ta, &ven_ta, &dev_ta
    };
    te_errno rc = 0;
    unsigned int i;
    te_vec *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_ENOMEM;

    *result = TE_VEC_INIT(char *);

    gen = te_string_fmt("/local:/dpdk:/%s:pci_fn:::", property);
    if (ta != NULL)
        gen_ta = te_string_fmt("/local:%s/dpdk:/%s:pci_fn:::", ta, property);

    if (vendor != NULL && vendor[0] != '\0')
    {
        ven = te_string_fmt("/local:/dpdk:/%s:pci_fn:%s::", property, vendor);
        if (ta != NULL)
        {
            ven_ta = te_string_fmt("/local:%s/dpdk:/%s:pci_fn:%s::",
                                   ta, property, vendor);
        }

        if (device != NULL && device[0] != '\0')
        {
            dev = te_string_fmt("/local:/dpdk:/%s:pci_fn:%s:%s:",
                                property, vendor, device);
            if (ta != NULL)
            {
                dev_ta = te_string_fmt("/local:%s/dpdk:/%s:pci_fn:%s:%s:",
                                       ta, property, vendor, device);
            }
        }
    }

    if (gen == NULL || gen_ta == NULL || ven == NULL || ven_ta == NULL ||
        dev == NULL || dev_ta == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < TE_ARRAY_LEN(instances); i++)
    {
        if (*instances[i] == &empty)
            continue;

        rc = cfg_get_instance_fmt(NULL, NULL, "%s", *instances[i]);
        if (rc == 0)
        {
            rc = TE_VEC_APPEND(result, *instances[i]);
            if (rc != 0)
                goto out;
        }
        else if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
        {
            goto out;
        }
        else
        {
            rc = 0;
        }
    }

    *instance_list = result;

out:
    for (i = 0; rc != 0 && i < TE_ARRAY_LEN(instances); i++)
    {
        if (*instances[i] == &empty)
            free(*instances[i]);
    }

    return rc;
}

static char
val2xdigit(uint8_t val)
{
    if (val < 10)
        return '0' + val;
    else
        return 'a' + val - 10;
}

static void
val2xdigits(uint8_t val, char *digits)
{
    uint8_t hi = val / 16;
    uint8_t lo = val % 16;

    digits[0] = val2xdigit(hi);
    digits[1] = val2xdigit(lo);
}

static char *
lcore_mask_to_hex(const lcore_mask_t *mask)
{
    /* Size of the mask in bytes excluding leading zero bytes */
    unsigned int mask_size = 0;
    unsigned int str_size;
    char *result;
    int str_i;
    int i;

    for (i = TE_ARRAY_LEN(mask->bytes); i > 0; i--)
    {
        if (mask->bytes[i - 1] != 0)
        {
            mask_size = i;
            break;
        }
    }

    if (mask_size == 0)
        return NULL;

    /* sizeof() string literal includes terminating '\0' */
    str_size = sizeof("0x") + (mask_size * 2);
    result = TE_ALLOC(str_size);
    if (result == NULL)
        return NULL;

    result[0] = '0';
    result[1] = 'x';

    for (i = mask_size - 1, str_i = 2; i >= 0; i--, str_i += 2)
        val2xdigits(mask->bytes[i], &result[str_i]);

    return result;
}

static te_bool
lcore_mask_is_zero(const lcore_mask_t *mask)
{
    uint8_t test = 0;
    unsigned int i;

    for (i = 0; i < TE_ARRAY_LEN(mask->bytes); ++i)
            test |= mask->bytes[i];

    return (test == 0);
}

static te_bool
lcore_mask_bit_is_set(const lcore_mask_t *mask, unsigned int bit)
{
    unsigned int index;

    if (bit >= TE_ARRAY_LEN(mask->bytes) * CHAR_BIT)
        return FALSE;

    index = bit / CHAR_BIT;

    return mask->bytes[index] & (1U << (bit % CHAR_BIT));
}

te_errno
tapi_rte_lcore_mask_set_bit(lcore_mask_t *mask, unsigned int bit)
{
    unsigned int index;

    if (bit >= TE_ARRAY_LEN(mask->bytes) * CHAR_BIT)
    {
        ERROR("lcore mask is too small for bit %u", bit);
        return TE_ENOSPC;
    }

    index = bit / CHAR_BIT;

    mask->bytes[index] |= (1U << (bit % CHAR_BIT));

    return 0;
}

te_errno
tapi_rte_get_nb_required_service_cores(const char *ta, const char *vendor,
                                       const char *device,
                                       unsigned int *nb_cores)
{
    unsigned int result = 0;
    te_vec *specifiers;
    te_errno rc;
    char **spec;

    rc = tapi_rte_get_pci_fn_specifiers("required_service_cores", ta, vendor,
                                        device, &specifiers);
    if (rc != 0)
        return rc;

    TE_VEC_FOREACH(specifiers, spec)
    {
        cfg_val_type type = CVT_INTEGER;
        unsigned int count;

        rc = cfg_get_instance_str(&type, &count, *spec);
        if (rc != 0)
        {
            te_vec_deep_free(specifiers);
            free(specifiers);
            return rc;
        }

        result = MAX(result, count);
    }

    te_vec_deep_free(specifiers);
    free(specifiers);

    *nb_cores = result;

    return 0;
}

te_errno
tapi_rte_get_dev_args(const char *ta, const char *vendor, const char *device,
                      char **arg_list)
{
    te_string result = TE_STRING_INIT;
    te_vec *specifiers;
    te_bool first;
    te_errno rc;
    char **spec;

    rc = tapi_rte_get_pci_fn_specifiers("dev_args", ta, vendor, device,
                                        &specifiers);
    if (rc != 0)
        return rc;

    first = TRUE;
    TE_VEC_FOREACH(specifiers, spec)
    {
        char *args;

        rc = cfg_get_instance_str(NULL, &args, *spec);
        if (rc != 0)
        {
            te_vec_deep_free(specifiers);
            free(specifiers);
            te_string_free(&result);
            return rc;
        }

        if (args != NULL && args[0] != '\0')
        {
            rc = te_string_append(&result, "%s%s", first ? "" : ",", args);
            first = FALSE;
        }

        free(args);
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

    te_vec_deep_free(specifiers);
    free(specifiers);

    return 0;
}

static te_errno
get_service_cores_by_pci_addr(const char *ta, const char *pci_addr,
                              unsigned int *nb_cores)
{
    char *vendor;
    char *device;
    te_errno rc;

    rc = tapi_cfg_pci_get_pci_vendor_device(ta, pci_addr, &vendor, &device);
    if (rc != 0)
        return rc;

    rc = tapi_rte_get_nb_required_service_cores(ta, vendor, device, nb_cores);

    free(vendor);
    free(device);

    return rc;
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
    char *eal_args_old = NULL;
    char *eal_args_new = NULL;
    te_errno rc;

    if (strcmp(busname, "pci") == 0)
    {
        const char *old_dev;
        char *arg;

        rc = tapi_eal_rpcs_get_cached_eal_args(rpcs, &eal_args_old);
        if (rc != 0)
            return rc;

        arg = te_string_fmt("--allow=%s", devname);
        if (arg == NULL)
        {
            free(eal_args_old);
            return TE_ENOMEM;
        }

        old_dev = strstr(eal_args_old, arg);
        if (old_dev != NULL)
        {
            const char *next_arg = strchr(old_dev, ' ');
            int start_offset = old_dev - eal_args_old;
            int old_dev_len = (next_arg == NULL) ? strlen(old_dev) :
                next_arg - old_dev;

            eal_args_new = te_string_fmt("%.*s%.*s%s%s%.*s",
                         start_offset, eal_args_old,
                         old_dev_len, old_dev,
                         devargs == NULL ? "" : ",",
                         te_str_empty_if_null(devargs),
                         (int)strlen(eal_args_old) - start_offset - old_dev_len,
                         eal_args_old + start_offset + old_dev_len);
        }
        else
        {
            eal_args_new = te_string_fmt("%s %s%s%s", eal_args_old, arg,
                                         devargs == NULL ? "" : ",",
                                         te_str_empty_if_null(devargs));
        }
        free(arg);
        if (eal_args_new == NULL)
        {
            free(eal_args_old);
            return TE_ENOMEM;
        }

        rc = tapi_eal_rpcs_set_cached_eal_args(rpcs, eal_args_new);
        free(eal_args_new);
        if (rc != 0)
        {
            free(eal_args_old);
            return rc;
        }
    }

    rc = rpc_rte_eal_hotplug_add(rpcs, busname, devname, devargs);
    if (rc != 0)
    {
        if (eal_args_old != NULL)
        {
            (void)tapi_eal_rpcs_set_cached_eal_args(rpcs, eal_args_old);
            free(eal_args_old);
        }
        return rc;
    }

    free(eal_args_old);
    return 0;
}

te_errno
tapi_rte_eal_hotplug_remove(rcf_rpc_server *rpcs, const char *busname,
                            const char *devname)
{
    char *eal_args_old = NULL;
    char *eal_args_new = NULL;
    te_errno rc;

    if (strcmp(busname, "pci") == 0)
    {
        const char *old_dev;
        char *arg;

        rc = tapi_eal_rpcs_get_cached_eal_args(rpcs, &eal_args_old);
        if (rc != 0)
            return rc;

        arg = te_string_fmt("--allow=%s", devname);
        if (arg == NULL)
        {
            free(eal_args_old);
            return TE_ENOMEM;
        }

        old_dev = strstr(eal_args_old, arg);
        if (old_dev == NULL)
        {
            free(eal_args_old);
            ERROR("Failed to find device to hotplug remove in cached EAL args");
            return TE_EINVAL;
        }

        {
            const char *next_arg = strchr(old_dev, ' ');
            int start_offset = old_dev - eal_args_old;
            int old_dev_len = (next_arg == NULL) ? strlen(old_dev) :
                next_arg - old_dev;

            eal_args_new = te_string_fmt("%.*s%.*s",
                         start_offset, eal_args_old,
                         (int)strlen(eal_args_old) - start_offset - old_dev_len,
                         eal_args_old + start_offset + old_dev_len);
        }
        free(arg);
        if (eal_args_new == NULL)
        {
            free(eal_args_old);
            return TE_ENOMEM;
        }

        rc = tapi_eal_rpcs_set_cached_eal_args(rpcs, eal_args_new);
        free(eal_args_new);
        if (rc != 0)
        {
            free(eal_args_old);
            return rc;
        }
    }

    rc = rpc_rte_eal_hotplug_remove(rpcs, busname, devname);
    if (rc != 0)
    {
        if (eal_args_old != NULL)
        {
            (void)tapi_eal_rpcs_set_cached_eal_args(rpcs, eal_args_old);
            free(eal_args_old);
        }
        return rc;
    }

    free(eal_args_old);
    return 0;
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
eal_args_to_str(int argc, char *argv[], char **eal_args_out)
{
    size_t                eal_args_len;
    char                 *eal_args = NULL;
    char                 *eal_args_pos;
    unsigned int          i;

    for (i = 0, eal_args_len = 1;
         i < (unsigned int)argc;
         eal_args_len += strlen(argv[i++]) + 1);

    eal_args = TE_ALLOC(eal_args_len);
    if (eal_args == NULL)
        return TE_ENOMEM;

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

    *eal_args_out = eal_args;

    return 0;
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
    char                 *eal_args = NULL;
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

    rc = eal_args_to_str(argc, argv, &eal_args);
    if (rc != 0)
        goto out;

    rc = tapi_eal_rpcs_get_cached_eal_args(rpcs, &eal_args_cfg);
    if (rc != 0)
        goto out;

    if (strlen(eal_args_cfg) == 0 || strcmp(eal_args, eal_args_cfg) != 0)
    {
        rc = tapi_rte_eal_fini(env, rpcs);
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
        const char *net_af_xdp_prefix = "net_af_xdp";

        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
            bus_name = "pci";
        else if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
            bus_name = "vdev";
        else
            continue;

        RPC_AWAIT_ERROR(rpcs);
        rc = rpc_rte_eth_dev_get_port_by_name(rpcs, dev_name, &port_id);
        if (rc == 0)
        {
            rpc_rte_eth_dev_stop(rpcs, port_id);
            rpc_rte_eth_dev_close(rpcs, port_id);
        }
        else if (rc != -TE_RC(TE_RPC, TE_ENODEV))
        {
            goto out;
        }

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
        else if (strncmp(dev_name, net_af_xdp_prefix,
                 strlen(net_af_xdp_prefix)) == 0)
        {
            te_motivated_msleep(100, "Wait for AF_XDP async cleanup be over");
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

    rc = cfg_get_instance_string_fmt(&slave_pci_inst_val, "%s", slave_inst_val);
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

    rc = cfg_get_instance_string_fmt(&mode, "%s/mode:", node_val);
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
    const char *dn = ps_if->iface->if_info.if_name;
    const char dn_prefix_af_xdp[] = "net_af_xdp";
    te_bool is_af_xdp = FALSE;

    prefix = (is_bonding) ? prefix_bonding : prefix_failsafe;
    postfix = (is_bonding) ? postfix_bonding : postfix_failsafe;

    if (strncmp(dn, dn_prefix_af_xdp, strlen(dn_prefix_af_xdp)) == 0)
    {
        is_af_xdp = TRUE;
        prefix = ",iface=";
        postfix = "";
    }

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
        char *sub = slaves[i];
        char *ifname_by_fn = NULL;

        if (is_af_xdp) {
            rc = cfg_get_instance_string_fmt(
                       &ifname_by_fn, "/agent:%s/hardware:/pci:/device:%s/net:",
                       ta, slaves[i]);
            if (rc != 0)
                goto out;

            sub = ifname_by_fn;
        }

        slave_list_str_len_new += strlen(prefix);
        slave_list_str_len_new += strlen(sub);
        if (!is_bonding)
        {
            rc = tapi_rte_get_dev_args_by_pci_addr(ta, slaves[i],
                                                   &dev_args_failsafe);
            if (rc != 0)
            {
                free(ifname_by_fn);
                goto out;
            }
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
            free(ifname_by_fn);
            free(dev_args_failsafe);
            goto out;
        }
        slave_list_str = slave_list_str_new;

        len = slave_list_str_len_new - slave_list_str_len;
        ret = snprintf(slave_list_str + slave_list_str_len, len + 1,
                       "%s%s%s%s%s", prefix, sub,
                       (dev_args_failsafe != NULL) ? "," : "",
                       (dev_args_failsafe != NULL) ? dev_args_failsafe : "",
                       postfix);
        free(dev_args_failsafe);
        free(ifname_by_fn);
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
tapi_eal_vdev_slaves_service_cores(tapi_env               *env,
                                   const tapi_env_ps_if   *ps_if,
                                   char                   *ta,
                                   unsigned int           *nb_cores)
{
    char         **slaves = NULL;
    unsigned int   nb_slaves = 0;
    unsigned int   result = 0;
    te_errno       rc = 0;
    unsigned int   i;

    rc = tapi_eal_get_vdev_slaves(env, ps_if, &slaves, &nb_slaves);
    if (rc != 0)
        return rc;

    for (i = 0; i < nb_slaves; i++)
    {
        unsigned int cores;

        rc = get_service_cores_by_pci_addr(ta, slaves[i], &cores);
        if (rc != 0)
            goto out;

        result = MAX(result, cores);
    }

    *nb_cores = result;

out:
    for (i = 0; i < nb_slaves; i++)
        free(slaves[i]);

    free(slaves);

    return rc;
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

        append_arg(argcp, argvpp, "--allow=%s%s%s", slaves[i],
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
tapi_eal_get_nb_required_service_cores_rpcs(tapi_env *env, rcf_rpc_server *rpcs,
                                            unsigned int *nb_cores)
{
    unsigned int service_core_count = 0;
    const tapi_env_ps_if *ps_if;
    const tapi_env_pco *pco;
    te_errno rc;

    pco = tapi_env_rpcs2pco(env, rpcs);
    if (pco == NULL)
        return TE_EINVAL;

    STAILQ_FOREACH(ps_if, &pco->process->ifs, links)
    {
        unsigned int count;

        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
        {
            rc = get_service_cores_by_pci_addr(rpcs->ta,
                                               ps_if->iface->if_info.if_name,
                                               &count);
            if (rc != 0)
                return rc;

        }
        else if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
        {
            rc = tapi_eal_vdev_slaves_service_cores(env, ps_if, rpcs->ta,
                                                    &count);
            if (rc != 0)
                return rc;
        }

        service_core_count = MAX(service_core_count, count);
    }

    *nb_cores = service_core_count;

    return 0;
}

static te_errno
grab_lcores_by_service_core_count(const char *ta,
                                  unsigned int service_core_count,
                                  lcore_mask_t *lcore_mask)
{
    /* The number of required lcores include service cores and 1 main core */
    unsigned int lcore_count = service_core_count + 1;
    tapi_cpu_index_t *indices = NULL;
    lcore_mask_t mask = {{0}};
    size_t nb_cpus;
    te_errno rc;
    size_t i;

    /*
     * It is assumed that 'ts/prologue' has already reserved cores
     * as per tapi_eal_get_nb_required_service_cores_rpcs() advice.
     * Below code simply queries reserved cores and makes the mask.
     *
     * TODO: reconsider this approach once new regressions show up.
     */
    rc = tapi_cfg_get_all_threads(ta, &nb_cpus, &indices);
    if (rc != 0)
        return rc;

    if (nb_cpus < lcore_count)
    {
        ERROR("%zu CPUs are reserved; required: %u", nb_cpus, lcore_count);
        rc = TE_ENOENT;
        goto out;
    }

    for (i = 0; i < lcore_count; i++)
    {
        rc = tapi_rte_lcore_mask_set_bit(&mask, indices[i].thread_id);
        if (rc != 0)
            goto out;
    }

    *lcore_mask = mask;

out:
    free(indices);

    return rc;
}

static te_errno
build_lcore_mask_arg(int *argc, char ***argv, const lcore_mask_t *lcore_mask)
{
    char *hex;

    if (lcore_mask_is_zero(lcore_mask))
    {
        ERROR("Provided lcore mask is zero");
        return TE_EINVAL;
    }

    hex = lcore_mask_to_hex(lcore_mask);
    if (hex == NULL)
        return TE_ENOMEM;

    append_arg(argc, argv, "-c");
    append_arg(argc, argv, "%s", hex);
    free(hex);

    return 0;
}

static te_errno
build_service_core_mask_arg(int *argc, char ***argv,
                            const lcore_mask_t *lcore_mask,
                            unsigned int n_service_cores)
{
    lcore_mask_t s_core_mask = {{0}};
    te_bool first = TRUE;
    unsigned int bit;
    unsigned int n;
    te_errno rc;
    char *hex;

    if (n_service_cores == 0)
        return 0;

    n = n_service_cores;
    for (bit = 0; bit < TE_ARRAY_LEN(s_core_mask.bytes) * CHAR_BIT; bit++)
    {
        if (lcore_mask_bit_is_set(lcore_mask, bit))
        {
            /* Skip the main lcore; it can't be treated as a service lcore. */
            if (first)
            {
                first = FALSE;
                continue;
            }

            rc = tapi_rte_lcore_mask_set_bit(&s_core_mask, bit);
            if (rc != 0)
                return rc;

            n--;
            if (n == 0)
                break;
        }
    }

    if (n > 0)
    {
        ERROR("Specified lcore mask does not allow for %u service cores",
              n_service_cores);
        return TE_EINVAL;
    }

    hex = lcore_mask_to_hex(&s_core_mask);
    if (hex == NULL)
        return TE_ENOMEM;

    append_arg(argc, argv, "-s%s", hex);
    free(hex);

    return 0;
}

static te_errno
get_numa_node_by_pci_addr(const char *ta, const char *pci_addr, int *numa_node)
{
    char *pci_oid;
    te_errno rc;

    rc = tapi_cfg_pci_oid_by_addr(ta, pci_addr, &pci_oid);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_pci_get_numa_node_id(pci_oid, numa_node);
    free(pci_oid);
    return rc;
}

static te_errno
tapi_eal_vdev_slaves_numa_node(tapi_env               *env,
                               const tapi_env_ps_if   *ps_if,
                               char                   *ta,
                               int                    *numa_node)
{
    char         **slaves = NULL;
    unsigned int   nb_slaves = 0;
    int            result = -1;
    te_errno       rc = 0;
    unsigned int   i;

    rc = tapi_eal_get_vdev_slaves(env, ps_if, &slaves, &nb_slaves);
    if (rc != 0)
        return rc;

    for (i = 0; i < nb_slaves; i++)
    {
        int node;

        rc = get_numa_node_by_pci_addr(ta, slaves[i], &node);
        if (rc != 0)
            goto out;

        if (result < 0)
        {
            result = node;
        }
        else if (node >= 0 && node != result)
        {
            result = -1;
            break;
        }
    }

    *numa_node = result;

out:
    for (i = 0; i < nb_slaves; i++)
        free(slaves[i]);

    free(slaves);

    return rc;
}

te_errno
tapi_rte_get_numa_node(tapi_env *env, rcf_rpc_server *rpcs, int *numa_node)
{
    const tapi_env_ps_if *ps_if;
    const tapi_env_pco *pco;
    int result = -1;
    te_errno rc;

    pco = tapi_env_rpcs2pco(env, rpcs);
    if (pco == NULL)
        return TE_EINVAL;

    STAILQ_FOREACH(ps_if, &pco->process->ifs, links)
    {
        int node;

        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
        {
            rc = get_numa_node_by_pci_addr(rpcs->ta,
                                           ps_if->iface->if_info.if_name,
                                           &node);
            if (rc != 0)
                return rc;
        }
        else if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_RTE_VDEV)
        {
            rc = tapi_eal_vdev_slaves_numa_node(env, ps_if, rpcs->ta,
                                                &node);
            if (rc != 0)
                return rc;
        }
        else
        {
            return TE_EINVAL;
        }

        if (result < 0)
        {
            result = node;
        }
        else if (node >= 0 && node != result)
        {
            result = -1;
            break;
        }
    }

    *numa_node = result;

    return 0;
}

te_errno
tapi_rte_make_eal_args(tapi_env *env, rcf_rpc_server *rpcs,
                       const char *program_name,
                       const lcore_mask_t *lcore_mask_override,
                       int argc, const char **argv,
                       int *out_argc, char ***out_argv)
{
    te_errno                rc = 0;
    const tapi_env_pco     *pco;
    char                  **my_argv = NULL;
    int                     my_argc = 0;
    const tapi_env_ps_if   *ps_if;
    int                     i;
    int                     mem_channels;
    int                     mem_amount = 0;
    char                   *app_prefix = NULL;
    char                   *extra_eal_args = NULL;
    unsigned int            service_core_count = 0;
    lcore_mask_t            lcore_mask;

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
        const char *dn = ps_if->iface->if_info.if_name;
        const char dn_prefix_af_xdp[] = "net_af_xdp";
        char *dev_args;

        switch (ps_if->iface->rsrc_type)
        {
            case NET_NODE_RSRC_TYPE_PCI_FN:
                rc = tapi_rte_get_dev_args_by_pci_addr(rpcs->ta,
                                                       ps_if->iface->if_info.if_name,
                                                       &dev_args);
                if (rc != 0)
                    goto cleanup;

                append_arg(&my_argc, &my_argv, "--allow=%s%s%s",
                           ps_if->iface->if_info.if_name,
                           (dev_args == NULL) ? "" : ",",
                           (dev_args == NULL) ? "" : dev_args);
                free(dev_args);
                break;

            case NET_NODE_RSRC_TYPE_RTE_VDEV:
                if (strncmp(dn, dn_prefix_af_xdp,
                            strlen(dn_prefix_af_xdp)) != 0)
                {
                    rc = tapi_eal_whitelist_vdev_slaves(env, ps_if, rpcs->ta,
                                                        &my_argc, &my_argv);
                    if (rc != 0)
                        goto cleanup;
                }
                else
                {
                    /* Do not let EAL automatically grab PCI devices */
                    append_arg(&my_argc, &my_argv, "--allow=FFFF:FF:FF.F");
                }
                break;

            default:
                rc = TE_EINVAL;
                goto cleanup;
        }
    }

    /* Add memory channels information */
    rc = cfg_get_instance_int_fmt(&mem_channels, "/local:%s/mem_channels:",
                                  rpcs->ta);
    if (rc != 0)
        goto cleanup;

    append_arg(&my_argc, &my_argv, "-n");
    append_arg(&my_argc, &my_argv, "%d", mem_channels);

    /* Set the amount of memory (in Megabytes) to be booked within hugetables */
    rc = cfg_get_instance_int_fmt(&mem_amount, "/local:%s/dpdk:/mem_amount:",
                                  rpcs->ta);
    if (rc == 0 && mem_amount > 0)
    {
        append_arg(&my_argc, &my_argv, "-m");
        append_arg(&my_argc, &my_argv, "%d", mem_amount);
    }

    /* Specify DPDK app prefix */
    rc = cfg_get_instance_string_fmt(&app_prefix, "/local:%s/dpdk:/app_prefix:",
                                     rpcs->ta);
    if (rc == 0 && app_prefix != NULL)
    {
        append_arg(&my_argc, &my_argv, "--file-prefix");
        append_arg(&my_argc, &my_argv, "%s%s", app_prefix, rpcs->ta);

        free(app_prefix);
    }

    /* Append extra EAL args */
    rc = cfg_get_instance_string_fmt(&extra_eal_args,
                                     "/local:%s/dpdk:/extra_eal_args:",
                                     rpcs->ta);
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

    rc = tapi_eal_get_nb_required_service_cores_rpcs(env, rpcs,
                                                     &service_core_count);
    if (rc != 0)
        goto cleanup;

    if (lcore_mask_override != NULL)
    {
        lcore_mask = *lcore_mask_override;
    }
    else
    {
        rc = grab_lcores_by_service_core_count(rpcs->ta, service_core_count,
                                               &lcore_mask);
        if (rc != 0)
            goto cleanup;
    }

    rc = build_lcore_mask_arg(&my_argc, &my_argv, &lcore_mask);
    if (rc != 0)
        goto cleanup;

    rc = build_service_core_mask_arg(&my_argc, &my_argv, &lcore_mask,
                                     service_core_count);
    if (rc != 0)
        goto cleanup;

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

    rc = tapi_rte_make_eal_args(env, rpcs, NULL, NULL, argc, argv,
                                &my_argc, &my_argv);
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
    else
    {
        rc = eal_args_to_str(my_argc, my_argv, &eal_args_new);
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

te_errno
tapi_rte_eal_fini(tapi_env *env, rcf_rpc_server *rpcs)
{
    te_errno rc;

    /* For the symmetry with tapi_rte_eal_init() */
    UNUSED(env);

    rc = tapi_eal_rpcs_reset_cached_eal_args(rpcs);
    if (rc != 0)
        return rc;

    /*
     * No work is done here to release CPU reservations.
     * It is assumed that the number of those that have
     * been reserved by 'ts/prologue' is sufficient for
     * any test iteration in the whole session. Release
     * will be done in the end, when all tests have run.
     */

    return rcf_rpc_server_restart(rpcs);
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
