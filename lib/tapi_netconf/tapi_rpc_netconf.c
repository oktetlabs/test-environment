/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI RPC for netconf
 *
 * Implementation of TAPI RPC for netconf
 */

#include "te_defs.h"
#include "rcf_rpc.h"
#include "tarpc.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_netconf.h"

/*
 * libnetconf2/log.h
 */
void
rpc_nc_libssh_thread_verbosity(rcf_rpc_server *rpcs,
                               rpc_nc_verb_level level)
{
    tarpc_nc_libssh_thread_verbosity_in in;
    tarpc_nc_libssh_thread_verbosity_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.level = level;

    rcf_rpc_call(rpcs, "nc_libssh_thread_verbosity", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_libssh_thread_verbosity, "%s", "",
                 nc_verb_level_rpc2str(level));

    RETVAL_VOID(nc_libssh_thread_verbosity);
}

/*
 * libnetconf2/session.h
 */

/* See description in tapi_rpc_netconf.h */
void
rpc_nc_session_free(rcf_rpc_server *rpcs, struct rpc_nc_session *session)
{
    tarpc_nc_session_free_in in;
    tarpc_nc_session_free_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.session = (tarpc_nc_session_ptr)session;

    rcf_rpc_call(rpcs, "nc_session_free", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_session_free, "0x%x", "", session);

    RETVAL_VOID(nc_session_free);
}

/*
 * libnetconf2/session_client.h
 */

/* See description in tapi_rpc_netconf.h */
void
rpc_nc_client_init(rcf_rpc_server *rpcs)
{
    tarpc_nc_client_init_in in;
    tarpc_nc_client_init_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "nc_client_init", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_client_init, "%s", "", "void");

    RETVAL_VOID(nc_client_init);
}

/* See description in tapi_rpc_netconf.h */
void
rpc_nc_client_destroy(rcf_rpc_server *rpcs)
{
    tarpc_nc_client_destroy_in in;
    tarpc_nc_client_destroy_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "nc_client_destroy", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_client_destroy, "%s", "", "void");

    RETVAL_VOID(nc_client_destroy);
}

/* See description in tapi_rpc_netconf.h */
int
rpc_nc_client_ssh_set_username(rcf_rpc_server *rpcs, const char *username)
{
    tarpc_nc_client_ssh_set_username_in in;
    tarpc_nc_client_ssh_set_username_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.username = username;

    rcf_rpc_call(rpcs, "nc_client_ssh_set_username", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_client_ssh_set_username, "'%s'", "%d",
                 username, out.retval);

    RETVAL_INT(nc_client_ssh_set_username, out.retval);
}

/* See description in tapi_rpc_netconf.h */
int
rpc_nc_client_ssh_add_keypair(rcf_rpc_server *rpcs, const char *pub_key,
                              const char *priv_key)
{
    tarpc_nc_client_ssh_add_keypair_in in;
    tarpc_nc_client_ssh_add_keypair_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.pub_key = pub_key;
    in.priv_key = priv_key;

    rcf_rpc_call(rpcs, "nc_client_ssh_add_keypair", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_client_ssh_add_keypair, "'%s', '%s'", "%d",
                 pub_key, priv_key, out.retval);

    RETVAL_INT(nc_client_ssh_add_keypair, out.retval);
}

/* See description in tapi_rpc_netconf.h */
struct rpc_nc_session *
rpc_nc_connect_ssh(rcf_rpc_server *rpcs, const char *host, uint16_t port)
{
    tarpc_nc_connect_ssh_in in;
    tarpc_nc_connect_ssh_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.host = host;
    in.port = port;

    rcf_rpc_call(rpcs, "nc_connect_ssh", &in, &out);

    if (!RPC_IS_CALL_OK(rpcs))
        RETVAL_PTR(nc_connect_ssh, NULL);

    TAPI_RPC_LOG(rpcs, nc_connect_ssh, "'%s', %u", "0x%x",
                 host, port, out.session);

    RETVAL_PTR64(nc_connect_ssh, out.session);
}

/* See description in tapi_rpc_netconf.h */
RPC_NC_MSG_TYPE
rpc_nc_send_rpc(rcf_rpc_server *rpcs, struct rpc_nc_session *session,
                struct rpc_nc_rpc *rpc, int timeout, uint64_t *msg_id)
{
    tarpc_nc_send_rpc_in in;
    tarpc_nc_send_rpc_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.session = (tarpc_nc_session_ptr)session;
    in.rpc = (tarpc_nc_rpc_ptr)rpc;
    in.timeout = timeout;

    rcf_rpc_call(rpcs, "nc_send_rpc", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_send_rpc, "0x%x, 0x%x, %u, 0x%x", "'%s'",
                 session, rpc, timeout, msg_id,
                 nc_msg_type_rpc2str(nc_msg_type_h2rpc(out.msg_type)));

    if (msg_id != NULL)
        *msg_id = out.msgid;

    RETVAL_INT(nc_send_rpc, nc_msg_type_h2rpc(out.msg_type));
}

/* See description in tapi_rpc_netconf.h */
RPC_NC_MSG_TYPE
rpc_nc_recv_reply(rcf_rpc_server *rpcs, struct rpc_nc_session *session,
                  struct rpc_nc_rpc *rpc, uint64_t msgid, int timeout,
                  char **envp, char **op)
{
    tarpc_nc_recv_reply_in in;
    tarpc_nc_recv_reply_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.session = (tarpc_nc_session_ptr)session;
    in.rpc = (tarpc_nc_rpc_ptr)rpc;
    in.msgid = msgid;
    in.timeout = timeout;

    rcf_rpc_call(rpcs, "nc_recv_reply", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_recv_reply, "0x%x, 0x%x, %u, %u", "'%s'",
                 session, rpc, msgid, timeout,
                 nc_msg_type_rpc2str(nc_msg_type_h2rpc(out.msg_type)));

    if (envp != NULL && out.envp != NULL)
        *envp = strdup(out.envp);

    if (op != NULL && out.op != NULL)
        *op = strdup(out.op);

    RETVAL_INT(nc_recv_reply, nc_msg_type_h2rpc(out.msg_type));
}

/*
 * libnetconf2/messages_client.h
 */

/* See description in tapi_rpc_netconf.h */
struct rpc_nc_rpc *
rpc_nc_rpc_get(rcf_rpc_server *rpcs, const char *filter, RPC_NC_WD_MODE wd_mode)
{
    tarpc_nc_rpc_get_in in;
    tarpc_nc_rpc_get_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.wd_mode = nc_wd_mode_rpc2h(wd_mode);
    in.filter = (filter != NULL) ? filter : "";

    rcf_rpc_call(rpcs, "nc_rpc_get", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_rpc_get, "'%s', %s", "0x%x",
                 filter, nc_wd_mode_rpc2str(wd_mode), out.rpc);

    RETVAL_PTR64(nc_rpc_get, out.rpc);
}

/* See description in tapi_rpc_netconf.h */
struct rpc_nc_rpc *
rpc_nc_rpc_getconfig(rcf_rpc_server *rpcs, RPC_NC_DATASTORE source,
                     const char *filter, RPC_NC_WD_MODE wd_mode)
{
    tarpc_nc_rpc_getconfig_in in;
    tarpc_nc_rpc_getconfig_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.source = nc_datastore_rpc2h(source);
    in.wd_mode = nc_wd_mode_rpc2h(wd_mode);

    in.filter = (filter != NULL) ? filter : "";

    rcf_rpc_call(rpcs, "nc_rpc_getconfig", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_rpc_getconfig, "%s, '%s', %s", "0x%x",
                 nc_datastore_rpc2str(source), filter,
                 nc_wd_mode_rpc2str(wd_mode), out.rpc);

    RETVAL_PTR64(nc_rpc_getconfig, out.rpc);
}

/* See description in tapi_rpc_netconf.h */
struct rpc_nc_rpc *
rpc_nc_rpc_edit(rcf_rpc_server *rpcs,
            RPC_NC_DATASTORE target, RPC_NC_RPC_EDIT_DFLTOP default_op,
            RPC_NC_RPC_EDIT_TESTOPT test_opt, RPC_NC_RPC_EDIT_ERROPT error_opt,
            const char *edit_content)
{
    tarpc_nc_rpc_edit_in in;
    tarpc_nc_rpc_edit_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.target = nc_datastore_rpc2h(target);
    in.default_op = nc_rpc_edit_dfltop_rpc2h(default_op);
    in.test_opt = nc_rpc_edit_testopt_rpc2h(test_opt);
    in.error_opt = nc_rpc_edit_erropt_rpc2h(error_opt);
    in.edit_content = edit_content;

    rcf_rpc_call(rpcs, "nc_rpc_edit", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_rpc_edit, "%s, %s, %s, %s, '%s'", "0x%x",
                 nc_datastore_rpc2str(target),
                 nc_rpc_edit_dfltop_rpc2str(default_op),
                 nc_rpc_edit_testopt_rpc2str(test_opt),
                 nc_rpc_edit_erropt_rpc2str(error_opt),
                 edit_content, out.rpc);

    RETVAL_PTR64(nc_rpc_edit, out.rpc);
}

/* See description in tapi_rpc_netconf.h */
struct rpc_nc_rpc *
rpc_nc_rpc_copy(rcf_rpc_server *rpcs,
                RPC_NC_DATASTORE target, const char *url_trg,
                RPC_NC_DATASTORE source, const char *url_or_config_src,
                RPC_NC_WD_MODE wd_mode)
{
    tarpc_nc_rpc_copy_in in;
    tarpc_nc_rpc_copy_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.target = nc_datastore_rpc2h(target);
    in.source = nc_datastore_rpc2h(source);
    in.wd_mode = nc_wd_mode_rpc2h(wd_mode);

    in.url_trg = (url_trg != NULL) ? url_trg : "";
    in.url_or_config_src = (url_or_config_src != NULL) ?
                                url_or_config_src : "";

    rcf_rpc_call(rpcs, "nc_rpc_copy", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_rpc_copy, "%s, '%s', %s, '%s', %s", "0x%x",
                 nc_datastore_rpc2str(target), url_trg,
                 nc_datastore_rpc2str(source), url_or_config_src,
                 nc_wd_mode_rpc2str(wd_mode), out.rpc);

    RETVAL_PTR64(nc_rpc_copy, out.rpc);
}

/* See description in tapi_rpc_netconf.h */
void
rpc_nc_rpc_free(rcf_rpc_server *rpcs, struct rpc_nc_rpc *rpc)
{
    tarpc_nc_rpc_free_in in;
    tarpc_nc_rpc_free_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.rpc = (tarpc_nc_rpc_ptr)rpc;

    rcf_rpc_call(rpcs, "nc_rpc_free", &in, &out);

    TAPI_RPC_LOG(rpcs, nc_rpc_free, "0x%x", "void", rpc);

    RETVAL_VOID(nc_rpc_free);
}
