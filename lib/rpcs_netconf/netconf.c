/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPC for Agent netconf daemon control
 *
 * RPC routines implementation to call Agent netconf client control functions
 */

#define TE_LGR_USER     "RPC NETCONF"

#include "te_config.h"

#include <nc_client.h>
#include <libyang/libyang.h>

#include "rpc_server.h"

#include "agentlib.h"

/*
 * libnetconf2/log.h
 */
TARPC_FUNC(nc_libssh_thread_verbosity, {},
{
    MAKE_CALL(func(in->level));
})

/*
 * libnetconf2/session.h
 */
/* nc_session_free() */
TARPC_FUNC(nc_session_free, {},
{
    MAKE_CALL(func_ptr((struct nc_session *)(in->session), NULL));
})

/*
 * libnetconf2/session_client.h
 */
/* nc_client_init() */
TARPC_FUNC(nc_client_init, {},
{
    MAKE_CALL(func_void());
})

/* nc_client_destroy() */
TARPC_FUNC(nc_client_destroy, {},
{
    MAKE_CALL(func_void());
})

/* nc_client_ssh_set_username() */
TARPC_FUNC(nc_client_ssh_set_username, {},
{
    MAKE_CALL(out->retval=func_ptr(in->username));
})

/* nc_client_ssh_add_keypair() */
TARPC_FUNC(nc_client_ssh_add_keypair, {},
{
    MAKE_CALL(out->retval=func_ptr(in->pub_key, in->priv_key));
})

/* nc_connect_ssh() */
TARPC_FUNC(nc_connect_ssh, {},
{
    MAKE_CALL(out->session = (tarpc_nc_session_ptr)func_ptr_ret_ptr(in->host,
                                                            (uint16_t)in->port,
                                                            NULL));
})

/* nc_send_rpc() */
TARPC_FUNC(nc_send_rpc, {},
{
    MAKE_CALL(out->msg_type =
                func_ptr((struct nc_session *)(in->session),
                         (struct nc_rpc *)(in->rpc),
                         in->timeout, &out->msgid));
})

/* nc_recv_reply() */
TARPC_FUNC(nc_recv_reply, {},
{
    struct lyd_node *envp = NULL;
    struct lyd_node *op = NULL;
    char *print_mem;

    MAKE_CALL(out->msg_type =
                func_ptr((struct nc_session *)(in->session),
                         (struct nc_rpc *)(in->rpc),
                         in->msgid, in->timeout, &envp, &op));

    if (envp != NULL)
    {
        lyd_print_mem(&print_mem, envp, LYD_XML, 0);
        out->envp = strdup(print_mem);
        lyd_free_all(envp);
    }
    else
    {
        out->envp = strdup("");
    }


    if (op != NULL)
    {
        lyd_print_mem(&print_mem, op, LYD_XML, 0);
        out->op = strdup(print_mem);
        lyd_free_all(op);
    }
    else
    {
        out->op = strdup("");
    }
})

/*
 * libnetconf2/messages_client.h
 */
/* nc_rpc_get() */
static void
ta_nc_rpc_get(tarpc_nc_rpc_get_in *in, tarpc_nc_rpc_get_out *out)
{
    if (*(in->filter) != '\0')
    {
        out->rpc = (tarpc_nc_rpc_ptr)nc_rpc_get(in->filter, in->wd_mode,
                                                NC_PARAMTYPE_DUP_AND_FREE);
    }
    else
    {
        ERROR("%s() Parameter 'filter' is mandatory for nc_rpc_get()",
              __FUNCTION__);
        out->rpc = (tarpc_nc_rpc_ptr)NULL;
    }
}
TARPC_FUNC_STANDALONE(nc_rpc_get, {},
{
    MAKE_CALL(ta_nc_rpc_get(in, out));
})

/* nc_rpc_getconfig() */
TARPC_FUNC(nc_rpc_getconfig, {},
{
    const char *filter;

    filter = (*(in->filter) != '\0') ? in->filter : NULL;

    MAKE_CALL(out->rpc = (tarpc_nc_rpc_ptr)func_ret_ptr(
                                                in->source,
                                                filter, in->wd_mode,
                                                NC_PARAMTYPE_DUP_AND_FREE));
})

/* nc_rpc_edit() */
static void
ta_nc_rpc_edit(tarpc_nc_rpc_edit_in *in, tarpc_nc_rpc_edit_out *out)
{
    if (*(in->edit_content) != '\0')
    {
        out->rpc = (tarpc_nc_rpc_ptr)nc_rpc_edit(
                                      in->target, in->default_op,
                                      in->test_opt, in->error_opt,
                                      in->edit_content,
                                      NC_PARAMTYPE_DUP_AND_FREE);
    }
    else
    {
        ERROR("%s() Parameter 'edit_content' is mandatory for nc_rpc_edit()",
              __FUNCTION__);
        out->rpc = (tarpc_nc_rpc_ptr)NULL;
    }
}
TARPC_FUNC_STANDALONE(nc_rpc_edit, {},
{
    MAKE_CALL(ta_nc_rpc_edit(in, out));
})

/* nc_rpc_copy() */
TARPC_FUNC(nc_rpc_copy, {},
{
    const char *url_trg;
    const char *url_or_config_src;

    url_trg = (*(in->url_trg) != '\0') ? in->url_trg : NULL;
    url_or_config_src = (*(in->url_or_config_src) != '\0') ?
                            in->url_or_config_src : NULL;

    MAKE_CALL(out->rpc = (tarpc_nc_rpc_ptr)func_ret_ptr(
                                                in->target, url_trg, in->source,
                                                url_or_config_src, in->wd_mode,
                                                NC_PARAMTYPE_DUP_AND_FREE));
})

/* nc_rpc_free() */
TARPC_FUNC(nc_rpc_free, {},
{
    MAKE_CALL(func_ptr((struct nc_rpc *)(in->rpc)));
})
