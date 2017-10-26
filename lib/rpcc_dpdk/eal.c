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
#include "te_alloc.h"

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
               const char     *dev_args,
               te_bool        *need_init,
               char          **eal_args_out)
{
    unsigned int                i;
    size_t                      eal_args_len;
    char                       *eal_args = NULL;
    char                       *eal_args_cfg = NULL;
    cfg_val_type                val_type = CVT_STRING;
    uint8_t                     dev_count;
    te_errno                    rc = 0;

    if ((argc <= 0) || (argv == NULL))
    {
        rc = TE_EINVAL;
        goto out;
    }

    for (i = 0, eal_args_len = 1;
         i < (unsigned int)argc;
         eal_args_len += strlen(argv[i++]));

    eal_args = TE_ALLOC(eal_args_len);
    if (eal_args == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < (unsigned int)argc; ++i)
    {
        char *retp;

        retp = strncat(eal_args, argv[i], strlen(argv[i]));
        if (retp == NULL)
        {
            rc = TE_ENOBUFS;
            goto out;
        }
    }

    rc = cfg_get_instance_fmt(&val_type, &eal_args_cfg,
                              "/agent:%s/rpcserver:%s/config:",
                              rpcs->ta, rpcs->name);
    if (rc != 0)
        goto out;

    if (strlen(eal_args_cfg) == 0)
    {
        *need_init = TRUE;
        *eal_args_out = eal_args;
        goto out;
    }

    dev_count = rpc_rte_eth_dev_count(rpcs);

    for (i = 0; i < dev_count; ++i)
    {
        if (rpc_rte_eth_dev_is_valid_port(rpcs, i))
        {
            char   dev_name[RPC_RTE_ETH_NAME_MAX_LEN];
            size_t dev_na_len;

            rpc_rte_eth_dev_close(rpcs, i);

            rc = rpc_rte_eth_dev_detach(rpcs, i, dev_name);
            if (rc != 0)
                goto out;

            if (dev_name[0] == '\0')
            {
                rc = TE_EINVAL;
                goto out;
            }

            dev_na_len = strlen(dev_name) + 1;
            dev_na_len += (dev_args != NULL) ? (strlen(dev_args) + 1) : 0;

            {
                char      dev_na[dev_na_len];
                char     *dev_na_generic = dev_name;
                uint16_t  port_id;

                if (dev_args != NULL)
                {
                    int ret;

                    ret = snprintf(dev_na, dev_na_len, "%s,%s",
                                   dev_name, dev_args);
                    if ((ret < 0) || (((unsigned int)ret + 1) != dev_na_len))
                    {
                        rc = TE_EINVAL;
                        goto out;
                    }

                    dev_na_generic = dev_na;
                }

                rc = rpc_rte_eth_dev_attach(rpcs, dev_na_generic, &port_id);
                if (rc != 0)
                    goto out;

                if (port_id != i)
                {
                    rc = TE_EINVAL;
                    goto out;
                }
            }
        }
    }

    rpc_rte_mempool_free_all(rpcs);

    if ((dev_count == 0) || (strcmp(eal_args, eal_args_cfg) != 0))
    {
        rc = rcf_rpc_server_restart(rpcs);
        if (rc == 0)
        {
            *need_init = TRUE;
            *eal_args_out = eal_args;
        }

        goto out;
    }

    *need_init = FALSE;
    *eal_args_out = NULL;

    free(eal_args);

out:
    free(eal_args_cfg);

    if (rc != 0)
        free(eal_args);

    return rc;
}

te_errno
tapi_rte_eal_init(tapi_env *env, rcf_rpc_server *rpcs,
                  int argc, const char **argv)
{
    te_errno                rc;
    const tapi_env_pco     *pco;
    char                  **my_argv;
    int                     my_argc;
    const tapi_env_ps_if   *ps_if;
    int                     ret;
    int                     i;
    cfg_val_type            val_type;
    char                   *dev_args = NULL;
    int                     mem_channels;
    int                     mem_amount = 0;
    char                   *app_prefix = NULL;
    te_bool                 need_init = TRUE;
    char                   *eal_args_new = NULL;

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
    STAILQ_FOREACH(ps_if, &pco->process->ifs, links)
    {
        if (ps_if->iface->rsrc_type == NET_NODE_RSRC_TYPE_PCI_FN)
        {
            append_arg(&my_argc, &my_argv, "--pci-whitelist=%s%s%s",
                       ps_if->iface->if_info.if_name,
                       (dev_args == NULL) ? "" : ",",
                       (dev_args == NULL) ? "" : dev_args);
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

    /* Append arguments provided by caller */
    for (i = 0; i < argc; ++i)
        append_arg(&my_argc, &my_argv, "%s", argv[i]);

    if (dpdk_reuse_rpcs())
    {
        rc = tapi_reuse_eal(rpcs, my_argc, my_argv, dev_args,
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

        if (eal_args_new == NULL)
            goto cleanup;

        rc = cfg_set_instance_fmt(CVT_STRING, eal_args_new,
                                  "/agent:%s/rpcserver:%s/config:",
                                  rpcs->ta, rpcs->name);
    }

cleanup:
    free(eal_args_new);

    for (i = 0; i < my_argc; ++i)
        free(my_argv[i]);

    free(dev_args);

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
