/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI RPC for libnetconf2
 *
 * Definitions of TAPI RPC for libnetconf2
 */

#ifndef __TE_TAPI_RPC_NETCONF_H__
#define __TE_TAPI_RPC_NETCONF_H__

#include "te_config.h"

#include "te_defs.h"
#include "rcf_rpc.h"
#include "te_rpc_netconf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_netconf TAPI for libnetconf client functions
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Set libssh verbosity level
 *
 * @param[in] rps       RPC server
 * @param[in] level     Verbosity level value
 */
extern void rpc_nc_libssh_thread_verbosity(rcf_rpc_server *rpcs,
                                           rpc_nc_verb_level level);

/*
 * libnetconf2/session.h
 */

/**
 * Free the NETCONF session object.
 *
 * @param[in] rpcs      RPC server
 * @param[in] session   Pointer to the netconf session object
 */
extern void rpc_nc_session_free(rcf_rpc_server *rpcs,
                                struct rpc_nc_session *session);

/*
 * libnetconf2/session_client.h
 */

/**
 * Initialize libssh and/or libssl/libcrypto for use in the client.
 *
 * @param[in] rpcs  RPC server
 */
extern void rpc_nc_client_init(rcf_rpc_server *rpcs);

/**
 * Destroy all libssh and/or libssl/libcrypto dynamic memory and
 * the client options, for both SSH and TLS, and for Call Home too.
 *
 * @param[in] rpcs  RPC server
 */
extern void rpc_nc_client_destroy(rcf_rpc_server *rpcs);

/**
 * Set client SSH username used for authentication.
 *
 * @param[in] rpcs      RPC server
 * @param[in] username  SSH user name.
 *
 * @return @c 0 - on success @c -1 -on fault.
 */
extern int rpc_nc_client_ssh_set_username(rcf_rpc_server *rpcs,
                                          const char *username);
/**
 * Add an SSH public and private key pair to be used for client authentication.
 * Private key can be encrypted, the passphrase will be asked for before
 * using it.
 *
 * @param[in] rpcs      RPC server
 * @param[in] pub_key   Path to the public key.
 * @param[in] priv_key  Path to the private key.
 *
 * @return @c 0 - on success @c -1 -on fault.
 */
extern int rpc_nc_client_ssh_add_keypair(rcf_rpc_server *rpcs,
                                         const char *pub_key,
                                         const char *priv_key);

/**
 * Connect to the NETCONF server using SSH transport (via libssh).
 *
 * @param[in] rpcs  RPC server
 * @param[in] host  Hostname or address (both Ipv4 and IPv6 are accepted)
 *                  of the target server. 'localhost' is used by default
 *                  if NULL is specified.
 * @param[in] port  Port number of the target server. Default value 830 is
 *                  used if 0 is specified.
 *
 * @return pointer to the created NETCONF session structure or @p NULL
 */
extern struct rpc_nc_session *rpc_nc_connect_ssh(rcf_rpc_server *rpcs,
                                                 const char *host,
                                                 uint16_t port);

/**
 * Send NETCONF RPC message via the session.
 *
 * @param[in] rpcs      TA RPC server
 * @param[in] session   NETCONF session where the RPC will be written.
 * @param[in] rpc       NETCONF RPC object to send via the specified session.
 * @param[in] timeout   Timeout for writing in milliseconds. Use negative
 *                      value for infinite waiting and 0 for return if data
 *                      cannot be sent immediately.
 * @param[out] msgid    If RPC was successfully sent, this is it's message ID.
 *
 * @retval #RPC_NC_MSG_RPC          on success,
 *         #RPC_NC_MSG_WOULDBLOCK   in case of a busy session, and
 *         #RPC_NC_MSG_ERROR        on error.
 */
extern RPC_NC_MSG_TYPE rpc_nc_send_rpc(rcf_rpc_server *rpcs,
                           struct rpc_nc_session *session,
                           struct rpc_nc_rpc *rpc,
                           int timeout,
                           uint64_t *msg_id);


/**
 * Receive NETCONF RPC reply.
 *
 * @note This function can be called in a single thread only.
 *
 * @param[in] rpcs      RPC server
 * @param[in] session   NETCONF session from which the function gets data.
 *                      It must be the client side session object.
 * @param[in] rpc       Original RPC this should be the reply to.
 * @param[in] msgid     Expected message ID of the reply.
 * @param[in] timeout   Timeout for reading in milliseconds. Use negative
 *                      value for infinite waiting and 0 for immediate return
 *                      if data are not available on the wire.
 * @param[out] envp     Location for NETCONF rpc-reply in XML form.
 *                      Optional, may be NULL.
 * @param[out] op       Location for parsed NETCONF reply data in XML form,
 *                      if any (none for \<ok\> or error replies).
 *                      Set only on #NC_MSG_REPLY and #NC_MSG_REPLY_ERR_MSGID
 *                      return. Optional, may be NULL.
 *
 * @retval #RPC_NC_MSG_REPLY            for success,
 *         #RPC_NC_MSG_WOULDBLOCK       if @p timeout has elapsed,
 *         #RPC_NC_MSG_ERROR            if reading has failed,
 *         #RPC_NC_MSG_NOTIF            if a notification was read instead (call
 *                                  this function again to get the reply), and
 *         #RPC_NC_MSG_REPLY_ERR_MSGID  if a reply with missing or wrong
 *                                  message-id was received.
 */
extern RPC_NC_MSG_TYPE rpc_nc_recv_reply(rcf_rpc_server *rpcs,
                             struct rpc_nc_session *session,
                             struct rpc_nc_rpc *rpc,
                             uint64_t msgid,
                             int timeout,
                             char **envp,
                             char **op);

/**
 * libnetconf2/messages_client.h
 *
 * Note: Originally many functions in this library have
 * argument 'paramtype' to specify what the function will
 * do with pointer-type arguments. See library header.
 * The variants are
 * NC_PARAMTYPE_CONST: use the parameter directly, do not free
 * NC_PARAMTYPE_FREE: use the parameter directly, free afterwards
 * NC_PARAMTYPE_DUP_AND_FREE: make a copy of the argument, free afterwards
 * Only the first variant looks like reasonable for TAPI RPC. So
 * the 'paramtype' argument will be omitted and all functions
 * will be called by RPC server with paramtype==NC_PARAMTYPE_CONST
 */

/**
 * Create NETCONF RPC <get> object.
 *
 * @param[in] rpcs      RPC server
 * @param[in] filter    Optional filter data, an XML subtree or
 *                      XPath expression (with JSON prefixes).
 * @param[in] wd_mode   Optional with-defaults capability mode.
 *
 * @return Pointer to NETCONF RPC object.
 */
extern struct rpc_nc_rpc *rpc_nc_rpc_get(rcf_rpc_server *rpcs,
                                     const char *filter,
                                     RPC_NC_WD_MODE wd_mode);

/**
 * Create NETCONF RPC <get-config>
 *
 * @param[in] rpcs      RPC server
 * @param[in] source    Source datastore being queried.
 * @param[in] filter    Optional filter data, an XML subtree or
 *                      XPath expression (with JSON prefixes).
 * @param[in] wd_mode   Optional with-defaults capability mode.
 *
 * @return Pointer to NETCONF RPC object.
 */
extern struct rpc_nc_rpc *rpc_nc_rpc_getconfig(rcf_rpc_server *rpcs,
                                           RPC_NC_DATASTORE source,
                                           const char *filter,
                                           RPC_NC_WD_MODE wd_mode);

/**
 * Create NETCONF RPC <edit-config>
 *
 * @param[in] rpcs          RPC server
 * @param[in] target        Target datastore being edited.
 * @param[in] default_op    Optional default operation.
 * @param[in] test_opt      Optional test option.
 * @param[in] error_opt     Optional error option.
 * @param[in] edit_content  Config or URL where the config to perform
 *                          is to be found.
 *
 * @return Pointer to NETCONF RPC object.
 */
extern struct rpc_nc_rpc *rpc_nc_rpc_edit(rcf_rpc_server *rpcs,
                                      RPC_NC_DATASTORE target,
                                      RPC_NC_RPC_EDIT_DFLTOP default_op,
                                      RPC_NC_RPC_EDIT_TESTOPT test_opt,
                                      RPC_NC_RPC_EDIT_ERROPT error_opt,
                                      const char *edit_content);

/**
 * Create NETCONF RPC <copy-config>
 *
 * @param[in] rpcs              RPC server
 * @param[in] target            Target datastore being edited.
 * @param[in] url_trg           Used instead target if the target is an URL.
 * @param[in] source            Source datastore.
 * @param[in] url_or_config_src Used instead source if the source is an URL
 *                              or a config.
 * @param[in] wd_mode           Optional with-defaults capability mode.
 *
 * @return Pointer to NETCONF RPC object.
 */
extern struct rpc_nc_rpc *rpc_nc_rpc_copy(rcf_rpc_server *rpcs,
                                      RPC_NC_DATASTORE target,
                                      const char *url_trg,
                                      RPC_NC_DATASTORE source,
                                      const char *url_or_config_src,
                                      RPC_NC_WD_MODE wd_mode);

/**
 * Free the NETCONF RPC object
 *
 * @param[in] rpcs      RPC server
 * @param[in] rpc       Pointer to RPC object to free.
 */
extern void rpc_nc_rpc_free(rcf_rpc_server *rpcs, struct rpc_nc_rpc *rpc);

/**@} <!-- END te_lib_rpc_netconf --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RPC_NETCONF_H__ */
