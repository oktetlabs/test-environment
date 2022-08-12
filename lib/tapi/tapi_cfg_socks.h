/** @file
 * @brief Test API to control Socks configurator tree.
 *
 * Definition of TAPI to configure Socks.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Svetlana Fishchuk <Svetlana.Fishchuk@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_SOCKS_H__
#define __TE_TAPI_CFG_SOCKS_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <stdint.h>
#include <netinet/in.h>

#include "conf_api.h"
#include "te_queue.h"
#include "te_rpc_sys_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Definition of a type for Socks instance ID */
typedef const char * tapi_socks_id;

/** Definition of a type for Socks user instance ID */
typedef const char * tapi_socks_user_id;

/** Definition of a type for Socks proto instance ID */
typedef const char * tapi_socks_proto_id;

/** Definition of a type for Socks cipher instance ID */
typedef const char * tapi_socks_cipher_id;

/** Definition of a type for Socks interface instance ID */
typedef const char * tapi_socks_interface_id;

/** Enumeration of SOCKS server implementations */
typedef enum te_socks_impl {
    TE_SOCKS_IMPL_SRELAY,   /**< Srelay-based implementation */
} te_socks_impl;

/**
 * Add Socks instance.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_add(const char *ta, tapi_socks_id id);

/**
 * Delete Socks instance.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_del(const char *ta, tapi_socks_id id);

/**
 * Enable Socks instance.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_enable(const char         *ta,
                                      tapi_socks_id       id);

/**
 * Disable Socks instance.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_disable(const char         *ta,
                                       tapi_socks_id       id);

/**
 * Get Socks status.
 *
 * @param       ta        Test Agent.
 * @param       id        Instance ID.
 * @param[out]  value     Current state. @c TRUE if enabled, @c FALSE if
 *                        disabled.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_status_get(const char      *ta,
                                          tapi_socks_id    id,
                                          te_bool         *value);

/**
 * Set Socks status.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 * @param value     New value to set. @c TRUE to enable, @c FALSE to disable.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_status_set(const char      *ta,
                                          tapi_socks_id    id,
                                          te_bool          value);

/**
 * Obtain used SOCKS server implementation.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Pointer to variable which will received used SOCKS
 *              implementation.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_impl_get(const char        *ta,
                                        tapi_socks_id      id,
                                        te_socks_impl     *value);

/**
 * Set used SOCKS server implementation.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Used SOCKS server implementation, element of te_sock_impl
 *              enumeration.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_impl_set(const char    *ta,
                                        tapi_socks_id  id,
                                        te_socks_impl  value);

/**
 * Add protocol to be used in SOCKS operations.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 * @param proto_id  Protocol instance id.
 * @param proto     Target protocol, such as RPC_IPPROTO_TCP.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_proto_add(const char         *ta,
                                         tapi_socks_id       id,
                                         tapi_socks_proto_id proto_id,
                                         int                 proto);

/**
 * Remove protocol from being used in SOCKS operations.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 * @param proto_id  Protocol instance id.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_proto_del(const char         *ta,
                                         tapi_socks_id       id,
                                         tapi_socks_proto_id proto_id);

/**
 * Obtain protocol used in SOCKS operations.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 * @param proto_id  Protocol instance id.
 * @param value     Pointer to variable which will received used protocol.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_proto_get(const char            *ta,
                                         tapi_socks_id          id,
                                         tapi_socks_proto_id    proto_id,
                                         int                   *value);
/**
 * Set protocol to use in SOCKS operations.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 * @param proto_id  Protocol instance id.
 * @param value     Valid protocol such as RPC_IPPROTO_TCP.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_proto_set(const char          *ta,
                                         tapi_socks_id        id,
                                         tapi_socks_proto_id  proto_id,
                                         int                  value);

/**
 * Add interface to listen at.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 * @param value         Interface name to assign, e.g. "eth1".
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_add(const char *ta,
    tapi_socks_id              id,
    tapi_socks_interface_id    interface_id,
    const char                *value);

/**
 * Remove interface from list of listened interfaces.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_del(const char *ta,
    tapi_socks_id              id,
    tapi_socks_interface_id    interface_id);

/**
 * Obtain interface instance listens at.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 * @param value         Pointer to a string which will receive the obtained
 *                      interface name. The string should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_get(const char *ta,
    tapi_socks_id              id,
    tapi_socks_interface_id    interface_id,
    char                     **value);

/**
 * Set interface to listen at.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 * @param value         Interface name to assign, e.g. "eth1".
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_set(const char *ta,
    tapi_socks_id              id,
    tapi_socks_interface_id    interface_id,
    const char                *value);

/**
 * Get port to listen at.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 * @param value         Pointer to a variable which will receive the port
 *                      associated with given interface.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_port_get(const char *ta,
    tapi_socks_id           id,
    tapi_socks_interface_id interface_id,
    uint16_t               *value);

/**
 * Set port to listen at.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 * @param value         Port number which will be associated with the given
 *                      interface.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_port_set(const char *ta,
    tapi_socks_id           id,
    tapi_socks_interface_id interface_id,
    uint16_t                value);

/**
 * Get address family used when binding to interface address.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 * @param value         Pointer to a variable which will receive the address
 *                      family associated with given interface.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_addr_family_get(const char *ta,
    tapi_socks_id             id,
    tapi_socks_interface_id   interface_id,
    int                      *value);

/**
 * Set address family which will be used when binding to interface address.
 *
 * @param ta            Test Agent.
 * @param id            Instance ID.
 * @param interface_id  Interface instance ID.
 * @param value         Target address family, i.e. RPC_AF_INET6.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_interface_addr_family_set(const char *ta,
    tapi_socks_id             id,
    tapi_socks_interface_id   interface_id,
    int                       value);

/**
 * Obtain interface used to send traffic after receiving.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Pointer to a string which will receive the obtained interface
 *              name. The string should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_outbound_interface_get(const char      *ta,
                                                      tapi_socks_id    id,
                                                      char           **value);

/**
 * Set interface to let traffic out from.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Interface name to assign, e.g. "eth1".
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_outbound_interface_set(const char     *ta,
                                                      tapi_socks_id   id,
                                                      const char     *value);

/**
 * Get cipher used when passing encrypted traffic.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Pointer to a string which will receive the obtained cipher.
 *              The string should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_cipher_get(const char   *ta,
                                          tapi_socks_id id,
                                          char        **value);

/**
 * Set cipher used when passing encrypted traffic.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Cipher to assign, must be supported by selected implementation.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_cipher_set(const char   *ta,
                                          tapi_socks_id id,
                                          const char   *value);

/**
 * Get authentication type used when verifying users.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Pointer to a string which will receive the obtained
 *              authentication type.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_auth_get(const char     *ta,
                                        tapi_socks_id   id,
                                        char          **value);

/**
 * Set authentication type used when verifying users.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param value Authentication type to assign.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_auth_set(const char     *ta,
                                        tapi_socks_id   id,
                                        const char     *value);

/**
 * Add user to instance's allowed user list.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_add(const char         *ta,
                                        tapi_socks_id       id,
                                        tapi_socks_user_id  user);

/**
 * Delete user from instance's allowed user list.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_del(const char         *ta,
                                        tapi_socks_id       id,
                                        tapi_socks_user_id  user);

/**
 * Get user's next server to be used (relay request to next server for that
 * user)
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param value Pointer to variable which will receive actual user's next hop.
 *              The string should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_next_hop_get(const char        *ta,
                                                 tapi_socks_id      id,
                                                 tapi_socks_user_id user,
                                                 char             **value);

/**
 * Set user's next server to use (for that user requests will be relayed to
 * @p value server).
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param value New user's next hop.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_next_hop_set(const char        *ta,
                                                 tapi_socks_id      id,
                                                 tapi_socks_user_id user,
                                                 const char        *value);

/**
 * Get user's internal username (as opposed to user-friendly name used in
 * TAPI).
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param value Pointer to variable which will receive actual user's username.
 *              The variable should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_username_get(const char        *ta,
                                                 tapi_socks_id      id,
                                                 tapi_socks_user_id user,
                                                 char             **value);

/**
 * Set user's internal username.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param value New user's internal username.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_username_set(const char        *ta,
                                                 tapi_socks_id      id,
                                                 tapi_socks_user_id user,
                                                 const char        *value);

/**
 * Get password associated with given user.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param value Pointer to variable which will receive the actual password.
 *              The variable should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_password_get(const char          *ta,
                                                 tapi_socks_id        id,
                                                 tapi_socks_user_id   user,
                                                 char               **value);

/**
 * Set password associated with given user.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param value New user's password.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_socks_user_password_set(const char          *ta,
                                                 tapi_socks_id        id,
                                                 tapi_socks_user_id   user,
                                                 const char          *value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_SOCKS_H__ */
