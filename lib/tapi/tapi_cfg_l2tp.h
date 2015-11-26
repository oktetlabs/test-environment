/** @file
 * @brief Test API to configure L2TP.
 *
 * Definition of TAPI to configure L2TP.
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
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

#include <stdbool.h>
#include <stdint.h>
#include "conf_api.h"

/** The following MACROS are needed for API */
#define REQUIRE true  /**< See the 'struct auth_prot' */
#define REFUSE false  /**< See the 'struct auth_prot' */

/** Authentication type */
enum auth_type {
    CHAP   = 0,          /**< CHAP authentication */
    PAP    = 1,          /**< PAP authentication */
    AUTH   = 2,          /**< remote peer authentication */
} ;

/** Structure for the IP-address pool */
typedef struct ipv4_range {
    struct sockaddr_in start; /**< The left boundary of the pool */
    struct sockaddr_in end;   /**< The right boundary of the pool */
    bool               is_lac /**< This field describes the pool appointment.
                                   Pool may belong to the ip range or 
                                   to the lac*/
} ipv4_range;

/** Node of the single-linked list for the connected clients */
typedef struct l2tp_connected {
    struct l2tp_connected  *next;   /**< Next node */
    struct sockaddr_in      cipv4;  /**< Connected client IP */

} l2tp_connected;

/** CHAP|PAP secret structure */
typedef struct ppp_secret {
    int8_t              is_chap;  /**< CHAP|PAP secret */
    char               *client;   /**< Client name */
    char               *server;   /**< Server name */
    char               *secret;   /**< Secret name */
    struct sockaddr_in  sipv4;    /**< IP address */
} ppp_secret;

/** CHAP|PAP single-linked list */
typedef struct secret_list {
    struct secret_list *next; /**< Next node */
    ppp_secret          snode;
} secret_list;

/** Structure for desired authentication */
typedef struct auth_prot {
    int8_t  auth_prot; /**< CHAP|PAP|AUTH */
    bool    demand;    /**< REQUIRE|REFUSE */
} auth_prot;

/**
 * Set a listen/local ip.
 *
 * @param ta            Test Agent
 * @param lns           If NULL the global listen ip will be set,
                        otherwise it will set local ip to the specified lns
 * @param local         New IP adress
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
                      struct sockaddr_in **local);

/**
 * Add ip range to the configuration.
 *
 * @param ta            Test Agent
 * @param lns           The name of the section to modify
 * @param ipv4_range    IP address pool
 *
 * @return Status code
 */

extern te_errno 
tapi_cfg_l2tp_lns_range_add(const char *ta, const char *lns, 
                            const ipv4_range *iprange);

/**
 * Delete specified ip range from the configuration.
 *
 * @param ta            Test Agent
 * @param lns           The name of the section to modify
 * @param ipv4_range    IP address pool
 *
 * @return Status code
 */
extern te_errno 
tapi_cfg_l2tp_lns_range_del(const char *ta, const char *lns,
                            const ipv4_range *iprange);

/**
 * Get the list of connected clients.
 *
 * @param ta       Test Agent
 * @param lns      The name of the section
 * @param local    Returned pointer to the list of connected ip
 *
 * @return Status code
 */

 extern te_errno
 tapi_cfg_l2tp_lns_connected_get(const char *ta, const char *lns, 
                                 l2tp_connected *connected);

/**
 * Set the bit parameter's value for the specified LNS.
 *
 * @param ta        Test Agent
 * @param lns       The name of the section
 * @param bit_name  Name of the parameter(e.g. hidden, length, flow)
 * @param selector  If true the bit_name value will be "yes"
                    otherwise "no"
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_lns_bit_set(const char *ta, const char *lns, 
                           const char *bit_name, bool selector);

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
                           const char *bit_name, bool *selector);

/**
 * Set the instance to yes or no for "/authentication/refuse|require".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param param       Desired authentication
 * @param ins_name    true(yes) or false(no)
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_lns_set_auth(const char *ta, const char *lns, 
                            auth_prot param, bool ins_name);
 /**
 * Get the instance to yes or no for "/authentication/refuse|require".
 *
 * @param ta          Test Agent
 * @param lns         The name of the section
 * @param param       Desired authentication
 * @param instance    Returned pointer to the authentication parameter,
                      like "yes" or "no"
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_lns_get_auth(const char *ta, const char *lns, 
                            auth_prot param, char *instance);

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
                             const ppp_secret *new_secret);
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
                                const ppp_secret *prev_secret);

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
