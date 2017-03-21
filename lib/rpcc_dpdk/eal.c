/** @file
 * @brief RPC client API for DPDK EAL
 *
 * RPC client API for DPDK EAL functions.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#include "te_config.h"
#include "te_ethernet.h"

#include "log_bufs.h"
#include "tapi_env.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte_ethdev.h"
#include "tapi_rpc_rte_mempool.h"

#include "tarpc.h"

#include "rpcc_dpdk.h"


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
    CHECK_RETVAL_VAR(rte_eal_init, out.retval, out.retval > argc,
                     RETVAL_ECORRUPTED);

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
    vasprintf(&(argv[argc]), fmt, ap);
    va_end(ap);

    *argc_p = argc + 1;
    *argv_p = argv;
}

static te_errno
tapi_reuse_eal(rcf_rpc_server *rpcs,
               int             argc,
               char           *argv[],
               te_bool        *need_init,
               char          **eal_args_out,
               te_bool        *need_conf_instance)
{
    unsigned int                i;
    size_t                      eal_args_len = 1;
    char                       *eal_args = NULL;
    char                       *eal_args_new;
    char                       *eal_args_cfg = NULL;
    cfg_val_type                val_type = CVT_STRING;
    uint8_t                     dev_count;
    te_errno                    rc = 0;

    if ((argc <= 0) || (argv == NULL))
    {
        rc = TE_EINVAL;
        goto out;
    }

    for (i = 0; i < (unsigned int)argc; ++i)
    {
        size_t arg_len = strlen(argv[i]);

        eal_args_len += arg_len;

        eal_args_new = realloc(eal_args, eal_args_len);
        if (eal_args_new == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        eal_args = eal_args_new;

        eal_args_new = strncat(eal_args, argv[i], arg_len);
        if (eal_args_new == NULL)
        {
            rc = TE_ENOBUFS;
            goto out;
        }

        eal_args = eal_args_new;
    }

    rc = cfg_get_instance_fmt(&val_type, &eal_args_cfg,
                              "/local:%s/dpdk:/eal_args:%s",
                              rpcs->ta, rpcs->name);
    if (rc == TE_RC(TE_CS, TE_ENOENT))
    {
        *need_init = TRUE;
        *eal_args_out = eal_args;
        *need_conf_instance = TRUE;

        rc = 0;
        goto out;
    }
    else if (rc != 0)
    {
        goto out;
    }

    dev_count = rpc_rte_eth_dev_count(rpcs);

    for (i = 0; i < dev_count; ++i)
    {
        if (rpc_rte_eth_dev_is_valid_port(rpcs, i))
        {
            rpc_rte_eth_dev_close(rpcs, i);

            /* Repeal any changes possibly made by the previous iteration */
            rc = rpc_rte_eth_dev_set_mtu(rpcs, i, ETHER_DATA_LEN);
            if (rc != 0)
                goto out;

            /* TODO: reset mc addr. list taking device state into account */
        }
    }

    rpc_rte_mempool_free_all(rpcs);

    if (strcmp(eal_args, eal_args_cfg) != 0)
    {
        rc = rcf_rpc_server_restart(rpcs);
        if (rc == 0)
        {
            *need_init = TRUE;
            *eal_args_out = eal_args;
            *need_conf_instance = FALSE;
        }

        goto out;
    }

    *need_init = FALSE;
    *eal_args_out = NULL;
    *need_conf_instance = FALSE;

    free(eal_args);

out:
    free(eal_args_cfg);

    if (rc != 0)
        free(eal_args);

    return rc;
}

te_errno
tapi_rte_eal_init(tapi_env *env, rcf_rpc_server *rpcs,
                  int argc, char **argv)
{
    te_errno                rc;
    const tapi_env_pco     *pco;
    char                  **my_argv;
    int                     my_argc;
    const tapi_env_ifs     *ps_ifs;
    const tapi_env_if      *p;
    int                     ret;
    int                     i;
    cfg_val_type            val_type;
    char                   *dev_args = NULL;
    int                     mem_channels;
    int                     mem_amount = 0;
    char                   *app_prefix = NULL;
    te_bool                 need_init = TRUE;
    char                   *eal_args_new = NULL;
    te_bool                 need_conf_instance = FALSE;

    if (env == NULL || rpcs == NULL)
        return TE_EINVAL;

    pco = tapi_env_rpcs2pco(env, rpcs);
    if (pco == NULL)
        return TE_EINVAL;

    /* Reserve space for argumnets provided by caller */
    my_argv = calloc(argc, sizeof(*my_argv));
    my_argc = 0;

    /* Use RPC server name as a program name */
    append_arg(&my_argc, &my_argv, "%s", rpcs->name);

    /* Get device arguments to be specified in whitelist option */
    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(&val_type, &dev_args,
                              "/local:%s/dpdk:/dev_args:", rpcs->ta);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        dev_args = NULL;
    }
    else if (rc != 0)
    {
        goto cleanup;
    }
    else if (dev_args[0] == '\0')
    {
        free(dev_args);
        dev_args = NULL;
    }

     /* Specify PCI whitelist or virtual device information */
    ps_ifs = &pco->process->ifs;
    for (p = ps_ifs->cqh_first; p != (void *)ps_ifs; p = p->ps_links.cqe_next)
    {
        if (p->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
        {
            append_arg(&my_argc, &my_argv, "--pci-whitelist=%s%s%s",
                       p->if_info.if_name, (dev_args == NULL) ? "" : ",",
                       (dev_args == NULL) ? "" : dev_args);
        }
    }

    free(dev_args);

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

    /* Append arguments provided by caller */
    memcpy(&my_argv[my_argc], argv, argc * sizeof(*my_argv));

    if (dpdk_reuse_rpcs())
    {
        rc = tapi_reuse_eal(rpcs, my_argc + argc, my_argv,
                            &need_init, &eal_args_new,
                            &need_conf_instance);
        if (rc != 0)
            goto cleanup;
    }

    if (need_init)
    {
        ret = rpc_rte_eal_init(rpcs, my_argc + argc, my_argv);

        rc = (ret < 0) ? TE_EFAULT : 0;
        if (rc != 0)
            goto cleanup;

        if (eal_args_new == NULL)
            goto cleanup;

        if (need_conf_instance)
        {
            rc = cfg_add_instance_fmt(NULL, CVT_STRING, eal_args_new,
                                      "/local:%s/dpdk:/eal_args:%s",
                                      rpcs->ta, rpcs->name);
        }
        else
        {
            rc = cfg_set_instance_fmt(CVT_STRING, eal_args_new,
                                      "/local:%s/dpdk:/eal_args:%s",
                                      rpcs->ta, rpcs->name);
        }
    }

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
