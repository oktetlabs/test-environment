/** @file
 * @brief Test API to configure L2TP.
 *
 * Definition of TAPI to configure L2TP.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file REST_AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Albert Podusenko <albert.podusenko@oktetlabs.ru>
 *
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

enum l2tp_bit {
    L2TP_BIT_HIDDEN,    /**< hidden bit option */
    L2TP_BIT_LENGTH,    /**< flow bits option */
    L2TP_BIT_FLOW  ,    /**< hidden bit option */
} l2tp_bit;

/** Structure for the IP-address pool */
typedef struct l2tp_ipv4_range {
    struct sockaddr_in *start;   /**< The left boundary of the pool */
    struct sockaddr_in *end;     /**< The right boundary of the pool */
    enum l2tp_policy    type;    /**< Above pool can be allowed or denied */
} l2tp_ipv4_range;

/** CHAP|PAP secret structure */
typedef struct l2tp_ppp_secret {
    enum l2tp_auth_prot  is_chap;  /**< CHAP|PAP secret */
    char                *client;   /**< Client name */
    char                *server;   /**< Server name */
    char                *secret;   /**< Secret name */
    struct sockaddr_in   sipv4;    /**< IP address */
} l2tp_ppp_secret;


/** Structure for desired authentication */
typedef struct l2tp_auth {
    enum l2tp_auth_prot    protocol; /**< CHAP|PAP|REST_AUTH */
    enum l2tp_auth_policy  type;     /**< REQUIRE|REFUSE */
} l2tp_auth;

typedef struct l2tp_range {
    SLIST_ENTRY(l2tp_range)   list;
    SLIST_HEAD(, l2tp_range)  l2tp_range;
    char                     *range; /**< range itself */
    char                     *local_ip;   /**< lns name */
    char                     *lns;   /**< lns name */
} l2tp_range;

/**
 * Set a l2tp server status.
 *
 * @param ta            Test Agent
 * @param status        1/0
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_server_set(const char *ta, int status);

/**
 * Get a l2tp server status.
 *
 * @param ta            Test Agent
 * @param status        Returned value
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_server_get(const char *ta, int *status);

/**
 * Set a listen/local ip.
 *
 * @param ta            Test Agent
 * @param lns           If NULL the global listen ip will be set,
 otherwise it will set local ip to the specified lns
 * @param local         New IP address
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_ip_set(const char *ta, const char *lns,
                     struct sockaddr_in *local);

/**
 * Get a current listen/local ip.
 *
 * @param ta            Test Agent
 * @param is_lnc        If NULL the global listen ip will be got,
 otherwise it will got local ip to the specified lns
 * @param local         Returned pointer to the current local ip
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_ip_get(const char *ta, const char *lns,
                     struct sockaddr_in *local_ip);

/**
 * Add ip range to the configuration.
 *
 * @param ta            Test Agent
 * @param lns           The name of the section to modify
 * @param iprange       IP address pool
 * @param kind          The class of the added ip range
 IP|LAC
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_range_add(const char *ta, const char *lns,
                            const l2tp_ipv4_range *iprange,
                            enum l2tp_iprange_class kind);

/**
 * Delete specified ip range from the configuration.
 *
 * @param ta            Test Agent
 * @param lns           The name of the section to modify
 * @param iprange       IP address pool
 * @param kind          The class of the removed ip range
 IP|LAC
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_range_del(const char *ta, const char *lns,
                            const l2tp_ipv4_range *iprange,
                            enum l2tp_iprange_class kind);

/**
 * Get the list of connected clients.
 *
 * @param ta         Test Agent
 * @param lns        The name of the section
 * @param connected  The connected ip addresses
 *                   As it is neccassary to return
 *                   an array of pointers to sockaddr_in *,
 *                   triple pointer is used
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_connected_get(const char *ta, const char *lns,
                                struct sockaddr_in ***connected);

/**
 * Set the bit parameter's value for the specified LNS.
 *
 * @param ta        Test Agent
 * @param lns       The name of the section
 * @param bit       Desired bit option to change
 * @param selector  If true the bit_name value will be "yes"
 otherwise "no"
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_bit_set(const char *ta, const char *lns,
                          enum l2tp_bit bit, const char *selector);

/**
 * Get the bit parameter's value for the specified LNS.
 *
 * @param ta        Test Agent
 * @param lns       The name of the section
 * @param bit_name  Name of the parameter(e.g. hidden, length, flow)
 * @param selector  Returned pointer to the current bit parameter value
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_bit_get(const char *ta, const char *lns,
                          enum l2tp_bit *bit_name, char *selector);

/**
 * Set the instance value to yes or no for "/auth/refuse|require".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param param       Desired authentication
 * @param value       true(1) or false(0)
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_set_auth(const char *ta, const char *lns,
                           l2tp_auth param, const char *value);
/**
 * Get the instance value "/auth/refuse|require".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param param       Desired authentication
 * @param value       Returned pointer to the authentication parameter,
 like "true" or "false"
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_get_auth(const char *ta, const char *lns,
                           l2tp_auth param, char *value);

/**
 * Add chap|pap secret.
 *
 * @param ta            Test Agent
 * @param lns           The name of the section
 * @param new_secret    Secret to add
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_secret_add(const char *ta, const char *lns,
                             const l2tp_ppp_secret *new_secret);
/**
 * Delete chap|pap secret.
 *
 * @param ta            Test Agent
 * @param lns           The name of the section
 * @param new_secret    Secret to delete
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_secret_delete(const char *ta, const char *lns,
                                const l2tp_ppp_secret *prev_secret);

/**
 * Set the instance value to yes or no for "/use_challenge:".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param value       yes or no
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_set_use_challenge(const char *ta, const char *lns,
                                    char *value);

/**
 * Get the instance value "/use_challenge:".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param value       Returned pointer to the current value
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_get_use_challenge(const char *ta, const char *lns,
                                    char *value);

/**
 * Set the instance value to yes or no for "/unix_auth:".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param value       true(1) or false(0)
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_set_unix_auth(const char *ta, const char *lns,
                                char *value);

/**
 * Get the instance value "/unix_auth:".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param value       Returned pointer to the current value
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_lns_get_unix_auth(const char *ta, const char *lns,
                                char *value);

/**
 * Set MTU size
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    New value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_mtu_set(const char *ta, const char *lns, int value);

/**
 * Get MTU size
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    Returned pointer to the current value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_mtu_get(const char *ta, const char *lns, int *value);

/**
 * Set MRU size
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    New value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_mru_set(const char *ta, const char *lns, int value);

/**
 * Get MRU size
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    Returned pointer to the current value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_mru_get(const char *ta, const char *lns, int *value);

/**
 * Set lcp-echo-failure.
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    New value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_lcp_echo_failure_set(const char *ta, const char *lns,
                                       int value);

/**
 * Get lcp-echo-failure.
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    Returned pointer to the current value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_lcp_echo_failure_get(const char *ta, const char *lns,
                                       int *value);

/**
 * Set lcp-echo-interval.
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    New value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_lcp_echo_interval_set(const char *ta, const char *lns,
                                        int value);

/**
 * Get lcp-echo-interval.
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param value    Returned pointer to the current value
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_lcp_echo_interval_get(const char *ta, const char *lns,
                                        int *value);

/**
 * Add an option to pppd.
 *
 * @param ta        Test Agent
 * @param lns       The name of the section
 * @param pparam    New param
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_pppopt_add(const char *ta, const char *lns,
                             const char *pparam);

/**
 * Delete an option from pppd.
 *
 * @param ta        Test Agent
 * @param lns       The name of the section
 * @param pparam    Param to delete
 *
 * @return Status code
 */

extern te_errno
tapi_cfg_l2tp_lns_pppopt_del(const char *ta, const char *lns,
                             const char *pparam);

#endif /* !__TE_TAPI_CFG_L2TP_H__ */