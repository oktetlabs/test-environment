/** @file
 * @brief Test API to control OpenVPN configurator tree.
 *
 * Definition of TAPI to configure OpenVPN.
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 * @author Svetlana Fishchuk <Svetlana.Fishchuk@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_OPENVPN_H__
#define __TE_TAPI_CFG_OPENVPN_H__

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


#ifdef __cplusplus
extern "C" {
#endif

/** Definition of a type for OpenVPN instance ID */
typedef const char * tapi_openvpn_id;

/** Type for sub instance IDs */
typedef const char * tapi_openvpn_prop;

/** Prototype for a function setting parameter of an integer type */
typedef te_errno (*tapi_cfg_openvpn_int_param_set)(const char      *ta,
                                                   tapi_openvpn_id  id,
                                                   int              val);
/** Prototype for a function setting parameter of a string type */
typedef te_errno (*tapi_cfg_openvpn_str_param_set)(const char      *ta,
                                                   tapi_openvpn_id  id,
                                                   const char      *val);

/** Macro generating get/set accessors for property */
#define TAPI_OPENVPN_ACCESSOR(name_, prop_, c_type_, cfg_type_)                \
static inline te_errno                                                         \
tapi_cfg_openvpn_##name_##_set(const char *ta, tapi_openvpn_id id,             \
                               c_type_ val)                                    \
{                                                                              \
    return tapi_cfg_openvpn_prop_set(ta, id, prop_, CFG_VAL(cfg_type_, val));  \
}                                                                              \
static inline te_errno                                                         \
tapi_cfg_openvpn_##name_##_get(const char *ta, tapi_openvpn_id id,             \
                               c_type_ *val)                                   \
{                                                                              \
    return tapi_cfg_openvpn_prop_get(ta, id, prop_, CVT_##cfg_type_,           \
                                     (void *)val);                             \
}

/**
 * Add OpenVPN instance.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_add(const char *ta, tapi_openvpn_id id);

/**
 * Delete OpenVPN instance.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_del(const char *ta, tapi_openvpn_id id);

/**
 * Set OpenVPN property.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param prop  Name of property.
 * @param type  Type of property.
 * @param val   Value the property will receive after execution of function.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_prop_set(const char       *ta,
                                          tapi_openvpn_id   id,
                                          const char       *prop,
                                          cfg_val_type      type,
                                          const void       *val);

/**
 * Get OpenVPN property.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param prop  Name of property.
 * @param type  Type of property.
 * @param val   Pointer to value which will receive the parameter stored in the
 *              configurator.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_prop_get(const char       *ta,
                                          tapi_openvpn_id   id,
                                          const char       *prop,
                                          cfg_val_type      type,
                                          void            **val);

/*
 * Accessors for string-based properties, macros spawn two functions like this:
 * tapi_cfg_openvpn_mode_set(const char *ta, tapi_openvpn_id id, const char *)
 * tapi_cfg_openvpn_mode_get(const char *ta, tapi_openvpn_id id, const char **)
 */
TAPI_OPENVPN_ACCESSOR(mode,
                      "mode:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(key_direction,
                      "key_direction:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(cipher,
                      "cipher:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(digest,
                      "digest:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(tls_key,
                      "tls_key:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(ca,
                      "ca:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(cert,
                      "cert:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(key,
                      "key:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(proto,
                      "proto:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(interface_behind,
                      "interface_behind:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(ip_facility,
                      "ip_facility:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(server_dh,
                      "server:/dh:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(ip,
                      "server:/ip:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(subnet_mask,
                      "server:/subnet_mask:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(server_pool_start,
                      "server:/pool:/start:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(server_pool_end,
                      "server:/pool:/end:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(client_username,
                      "client:/username:",
                      const char *,
                      STRING);
TAPI_OPENVPN_ACCESSOR(client_password,
                      "client:/password:",
                      const char *,
                      STRING);
/*
 * Accessors for integer-based properties, macros spawn two functions like this:
 * tapi_cfg_openvpn_status_set(const char *ta, tapi_openvpn_id id, int)
 * tapi_cfg_openvpn_status_get(const char *ta, tapi_openvpn_id id, int *)
 */
TAPI_OPENVPN_ACCESSOR(status,
                      "status:",
                      int,
                      INTEGER);
TAPI_OPENVPN_ACCESSOR(lzo,
                      "lzo:",
                      int,
                      INTEGER);
TAPI_OPENVPN_ACCESSOR(port,
                      "port:",
                      int,
                      INTEGER);
TAPI_OPENVPN_ACCESSOR(server_dh_size,
                      "server:/dh:/size:",
                      int,
                      INTEGER);
TAPI_OPENVPN_ACCESSOR(server_require_certs,
                      "server:/require_certs:",
                      int,
                      INTEGER);
TAPI_OPENVPN_ACCESSOR(is_server,
                      "is_server:",
                      int,
                      INTEGER);

/**
 * Enable OpenVPN instance.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_enable(const char         *ta,
                                        tapi_openvpn_id     id);

/**
 * Disable OpenVPN instance.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_disable(const char         *ta,
                                        tapi_openvpn_id     id);


/**
 * Current OpenVPN endpoint IP.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param addr  Double pointer to sockaddr receiving IP.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_openvpn_endpoint_ip_get(const char        *ta,
                                                 tapi_openvpn_id    id,
                                                 struct sockaddr  **addr);

/**
 * Add peer to instance's remote list: the remote which will be used in client
 * mode during target peer selection.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param peer  Peer name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_peer_add(const char       *ta,
                                          tapi_openvpn_id   id,
                                          tapi_openvpn_prop peer);

/**
 * Delete peer from instance's remote list.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param peer  Peer name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_peer_del(const char       *ta,
                                          tapi_openvpn_id   id,
                                          tapi_openvpn_prop peer);

/**
 * Get port associated with the peer.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param peer  Peer name.
 * @param val   Pointer to a variable which will receive the associated port.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_peer_port_get(const char          *ta,
                                               tapi_openvpn_id      id,
                                               tapi_openvpn_prop    peer,
                                               uint16_t            *val);

/**
 * Set port associated with the peer.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param peer  Peer name.
 * @param val   Port number which will be associated with given remote.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_peer_port_set(const char          *ta,
                                               tapi_openvpn_id      id,
                                               tapi_openvpn_prop    peer,
                                               uint16_t             val);

/**
 * Add user to instance's allowed user list.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_user_add(const char       *ta,
                                          tapi_openvpn_id   id,
                                          tapi_openvpn_prop user);

/**
 * Delete user from instance's allowed user list.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_user_del(const char       *ta,
                                          tapi_openvpn_id   id,
                                          tapi_openvpn_prop user);

/**
 * Get user's internal username (as opposed to user-friendly name used in
 * TAPI).
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param val   Pointer to variable which will receive actual user's username.
 *              The username string should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_user_username_get(const char          *ta,
                                                   tapi_openvpn_id      id,
                                                   tapi_openvpn_prop    user,
                                                   char               **val);

/**
 * Set user's internal username.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param val   New user's internal username.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_user_username_set(const char          *ta,
                                                   tapi_openvpn_id      id,
                                                   tapi_openvpn_prop    user,
                                                   const char          *val);

/**
 * Get password associated with given user.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param val   Pointer to variable which will receive the actual password.
 *              The password string should be freed after usage.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_user_password_get(const char          *ta,
                                                   tapi_openvpn_id      id,
                                                   tapi_openvpn_prop    user,
                                                   char               **val);

/**
 * Set password associated with given user.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param val   New user's password.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_user_password_set(const char          *ta,
                                                   tapi_openvpn_id      id,
                                                   tapi_openvpn_prop    user,
                                                   const char          *val);

/**
 * Get certificate (rather path to it) associated with the user.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param val   Pointer to variable which will receive the actual certificate
 *              path. The certificate path string should be freed after usage.
 *
 * @return Status code
 *
 * @remark Certificate must be physically located at TA side.
 */
extern te_errno tapi_cfg_openvpn_user_certificate_get(const char       *ta,
                                                      tapi_openvpn_id   id,
                                                      tapi_openvpn_prop user,
                                                      char            **val);

/**
 * Set path to certificate associated with the user.
 *
 * @param ta    Test Agent.
 * @param id    Instance ID.
 * @param user  User name.
 * @param val   New user's certificate path.
 *
 * @return Status code
 *
 * @remark Certificate must be physically located at TA side.
 */
extern te_errno tapi_cfg_openvpn_user_certificate_set(const char       *ta,
                                                      tapi_openvpn_id   id,
                                                      tapi_openvpn_prop user,
                                                      const char       *val);

/**
 * Add custom option to use when generating configuration file.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 * @param option    User-friendly option name.
 * @param val       Full option, e.g. "sndbuf 65536".
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_option_add(const char         *ta,
                                            tapi_openvpn_id     id,
                                            tapi_openvpn_prop   option,
                                            const char         *val);

/**
 * Delete option.
 *
 * @param ta        Test Agent.
 * @param id        Instance ID.
 * @param option    User-friendly option name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_openvpn_option_del(const char         *ta,
                                            tapi_openvpn_id     id,
                                            tapi_openvpn_prop   option);

#undef TAPI_OPENVPN_ACCESSOR

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_OPENVPN_H__ */
