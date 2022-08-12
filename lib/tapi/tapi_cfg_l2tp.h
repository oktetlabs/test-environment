/** @file
 * @brief Test API to configure L2TP.
 *
 * @defgroup tapi_conf_l2tp L2TP configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure L2TP.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_L2TP_H__
#define __TE_TAPI_CFG_L2TP_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <stdint.h>
#include "conf_api.h"
#include <netinet/in.h>
#include "te_queue.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Authentication type */
enum l2tp_auth_prot {
    L2TP_AUTH_PROT_CHAP,         /**< CHAP authentication */
    L2TP_AUTH_PROT_PAP,          /**< PAP authentication */
    L2TP_AUTH_PROT_REST_AUTH     /**< Remote peer authentication */
};

/** The law defining the type of the ip range */
enum l2tp_policy {
    L2TP_POLICY_DENY,     /**< For ipv4 addresses which must be denied*/
    L2TP_POLICY_ALLOW     /**< For ipv4 addresses which must be allowed */
};

/** Type of the request for the auth_type */
enum l2tp_auth_policy {
    L2TP_AUTH_POLICY_REFUSE,    /**< Refuse CHAP|PAP|REST_AUTH */
    L2TP_AUTH_POLICY_REQUIRE    /**< Require CHAP|PAP|REST_AUTH */
};

/** Subset to which the certain ip range addresses belongs */
enum l2tp_iprange_class {
    L2TP_IP_RANGE_CLASS_IP,    /**< ip range*/
    L2TP_IP_RANGE_CLASS_LAC    /**< lac */
};

/** Bit options */
enum l2tp_bit {
    L2TP_BIT_HIDDEN,    /**< hidden bit option */
    L2TP_BIT_LENGTH,    /**< flow bits option */
};

/** Structure for the IP-address pool */
typedef struct l2tp_ipaddr_range {
    struct sockaddr  *start;    /**< The left boundary of the pool */
    struct sockaddr  *end;      /**< The right boundary of the pool */
    enum l2tp_policy  type;     /**< Above pool can be allowed or denied */
} l2tp_ipaddr_range;

/** CHAP|PAP secret structure */
typedef struct l2tp_ppp_secret {
    enum l2tp_auth_prot  is_chap;  /**< CHAP|PAP secret */
    const char          *client;   /**< Client name */
    const char          *server;   /**< Server name */
    const char          *secret;   /**< Secret name */
    const char          *sipv4;    /**< IP address */
} l2tp_ppp_secret;


/** Structure for desired authentication */
typedef struct l2tp_auth {
    enum l2tp_auth_prot    protocol; /**< CHAP|PAP|REST_AUTH */
    enum l2tp_auth_policy  type;     /**< REQUIRE|REFUSE */
} l2tp_auth;

/**
 * Set a l2tp server status.
 *
 * @param ta            Test Agent.
 * @param status        Status: @c 1 to start, @c 0 to stop l2tp server.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_server_set(const char *ta, int status);

/**
 * Get a l2tp server status.
 *
 * @param ta            Test Agent.
 * @param status        Status.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_server_get(const char *ta, int *status);


/**
 * Add LNS.
 *
 * @param ta            Test Agent.
 * @param lns           LNS name to add.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_add(const char *ta, const char *lns);

/**
 * Delete LNS.
 *
 * @param ta            Test Agent.
 * @param lns           Name of LNS to delete.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_add(const char *ta, const char *lns);

/**
 * Get tunnel IP address.
 *
 * @param[in]  ta       Test Agent.
 * @param[in]  lns      Name of LNS.
 * @param[out] addr     Current tunnel IP address.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_tunnel_ip_get(const char *ta, const char *lns,
                                            struct sockaddr **addr);

/**
 * Set tunnel IP address.
 *
 * @param ta            Test Agent.
 * @param lns           Name of LNS.
 * @param addr          Tunnel IP address to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_tunnel_ip_set(const char *ta, const char *lns,
                                            const struct sockaddr *addr);

/**
 * Get global listen IP address.
 *
 * @param[in]  ta       Test Agent.
 * @param[out] addr     Listen IP address.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_listen_ip_get(const char *ta,
                                            struct sockaddr **addr);

/**
 * Set global listen IP address.
 *
 * @param ta            Test Agent.
 * @param addr          Listen IP address to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_listen_ip_set(const char *ta,
                                            const struct sockaddr *addr);

/**
 * Get a global port.
 *
 * @param ta            Test Agent.
 * @param port          Port.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_port_get(const char *ta, int *port);

/**
 * Set a global port.
 *
 * @param ta            Test Agent.
 * @param port          Port to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_port_set(const char *ta, int port);


/**
 * Add ip range to the configuration.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section to modify.
 * @param iprange       IP address pool.
 * @param kind          The class of the added ip range IP|LAC.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_range_add(const char *ta, const char *lns,
                                            const l2tp_ipaddr_range *iprange,
                                            enum l2tp_iprange_class kind);

/**
 * Delete specified ip range from the configuration.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section to modify.
 * @param iprange       IP address pool.
 * @param kind          The class of the removed ip range IP|LAC.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_range_del(const char *ta, const char *lns,
                                            const l2tp_ipaddr_range *iprange,
                                            enum l2tp_iprange_class kind);

/**
 * Get the list of connected clients.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param connected     The connected ip addresses (array of pointers).
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_connected_get(const char *ta, const char *lns,
                                                struct sockaddr ***connected);

/**
 * Add a bit option value for the specified LNS.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param bit           Desired bit option to change.
 * @param value         Bit value.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_bit_add(const char *ta, const char *lns,
                                          enum l2tp_bit bit, te_bool value);

/**
 * Delete the bit option value of the specified LNS.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param bit           Desired bit option to delete.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_bit_del(const char *ta, const char *lns,
                                          enum l2tp_bit *bit);

/**
 * Get the bit parameter's value for the specified LNS.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param bit           Name of the parameter(e.g. hidden, length, flow).
 * @param selector      Returned pointer to the current bit parameter value.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_bit_get(const char *ta, const char *lns,
                                          enum l2tp_bit *bit_name, char *selector);

/**
 * Set the instance value to yes or no for "/auth/refuse|require".
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param param         Desired authentication.
 * @param value         @c TRUE(1) or @c FALSE(0).
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_add_auth(const char *ta, const char *lns,
                                           l2tp_auth param, te_bool value);

/**
 * Delete the instance value "/auth/refuse|require".
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param param         Desired authentication.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_del_auth(const char *ta, const char *lns,
                                           l2tp_auth param);

/**
 * Add chap|pap secret.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param secret        Secret to add.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_secret_add(const char *ta, const char *lns,
                                             const l2tp_ppp_secret *secret);

/**
 * Delete chap|pap secret.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param secret        Secret to delete.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_secret_delete(const char *ta, const char *lns,
                                                const l2tp_ppp_secret *secret);

/**
 * Set the instance value to yes or no for "/use_challenge:".
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_set_use_challenge(const char *ta,
                                                    const char *lns,
                                                    te_bool value);

/**
 * Get the instance value "/use_challenge:".
 *
 * @param[in]  ta       Test Agent.
 * @param[in]  lns      The name of the section.
 * @param[out] value    Value of use challenge.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_get_use_challenge(const char *ta,
                                                    const char *lns,
                                                    te_bool *value);

/**
 * Set the instance value to yes or no for "/unix_auth:".
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_set_unix_auth(const char *ta, const char *lns,
                                                te_bool value);

/**
 * Get the instance value "/unix_auth:".
 *
 * @param[in]  ta       Test Agent.
 * @param[in]  lns      The name of the section.
 * @param[out] value    Value of auth.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_get_unix_auth(const char *ta, const char *lns,
                                                te_bool *value);

/**
 * Set MTU size.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_mtu_set(const char *ta, const char *lns,
                                          int value);

/**
 * Get MTU size.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value of MTU.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_mtu_get(const char *ta, const char *lns,
                                          int *value);

/**
 * Set MRU size.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_mru_set(const char *ta, const char *lns,
                                          int value);

/**
 * Get MRU size.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value of MRU.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_mru_get(const char *ta, const char *lns,
                                          int *value);

/**
 * Set lcp-echo-failure.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_lcp_echo_failure_set(const char *ta,
                                                       const char *lns,
                                                       int value);

/**
 * Get lcp-echo-failure.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         lcp-echo-failure value.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_lcp_echo_failure_get(const char *ta,
                                                       const char *lns,
                                                       int *value);

/**
 * Set lcp-echo-interval.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         Value to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_lcp_echo_interval_set(const char *ta,
                                                        const char *lns,
                                                        int value);

/**
 * Get lcp-echo-interval.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param value         lcp-echo-interval value.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_lcp_echo_interval_get(const char *ta,
                                                        const char *lns,
                                                        int *value);

/**
 * Add an option to pppd.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param opt           Option to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_pppopt_add(const char *ta, const char *lns,
                                             const char *pparam);

/**
 * Delete an option from pppd.
 *
 * @param ta            Test Agent.
 * @param lns           The name of the section.
 * @param opt           Option to delete.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_l2tp_lns_pppopt_del(const char *ta, const char *lns,
                                             const char *pparam);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_L2TP_H__ */

/**@} <!-- END tapi_conf_l2tp --> */
