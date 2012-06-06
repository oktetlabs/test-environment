/** @file
 * @brief Test API to configure XEN.
 *
 * Definition of API to configure XEN.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id:
 */

#ifndef __TE_TAPI_CFG_XEN_H__
#define __TE_TAPI_CFG_XEN_H__

#include "te_errno.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sockaddr; /** Forward declaration to avoid header inclusion */

/**
 * @defgroup tapi_conf_xen XEN configuration
 * @ingroup tapi_conf
 * @{
 */

/**
 * Get XEN storage path for templates of domU
 * disk images and where domUs are cloned.
 *
 * @param ta            Test Agent running withing dom0
 * @param path          Storage to accept path
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_get_path(char const *ta, char *path);

/**
 * Set XEN storage path for templates of domU
 * disk images and where domUs are cloned.
 *
 * @param ta            Test Agent running withing dom0
 * @param path          New XEN path
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_set_path(char const *ta, char const *path);

/**
 * Get IP address of 'eth0' of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get status of
 * @param size          Storage to accept domU 'eth0' memory size
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_get_rcf_port(char const   *ta,
                                          unsigned int *port);

/**
 * Set IP address of 'eth0' of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get memory size of
 * @param size          New domU 'eth0' memory size
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_set_rcf_port(char const  *ta,
                                          unsigned int port);

/**
 * Get the name of the bridge that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get bridge name of
 * @param br_name       Storage to accept bridge name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_get_rpc_br(char const *ta,
                                        char       *br_name);

/**
 * Set the name of the bridge that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param br_name       New domU bridge name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_set_rpc_br(char const *ta,
                                        char const *br_name);

/**
 * Get the name of the interface that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param if_name       Storage to accept interface name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_get_rpc_if(char const *ta,
                                        char       *if_name);

/**
 * Set the name of the interface that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param if_name       New domU interface name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_set_rpc_if(char const *ta,
                                        char const *if_name);

/**
 * Get MAC address that is used as base one for domUs.
 *
 * @param ta            Test Agent running withing dom0
 * @param mac           Storage to accept base MAC address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_get_base_mac_addr(char const *ta,
                                               uint8_t    *mac);

/**
 * Set MAC address that is used as base one for domUs.
 *
 * @param ta            Test Agent running withing dom0
 * @param mac           New base MAC address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_set_base_mac_addr(char const    *ta,
                                               uint8_t const *mac);

/**
 * Get dom0 acceleration.
 *
 * @param ta            Test Agent running withing dom0
 * @param accel         Storage to accept dom0 acceleration
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_get_accel(char const *ta, te_bool *accel);

/**
 * Set dom0 acceleration.
 *
 * @param ta            Test Agent running withing dom0
 * @param accel         New acceleration value
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_set_accel(char const *ta, te_bool accel);

/**
 * Perform dom0 initialization/cleanup.
 *
 * @param ta            Test Agent running withing dom0
 * @param init          Initialization (TRUE) or clean up (FALSE)?
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_set_init(char const *ta, te_bool init);

/**
 * Create new domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to create
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_create_dom_u(char const *ta,
                                          char const *dom_u);

/**
 * Destroy new domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to destroy
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_destroy_dom_u(char const *ta,
                                           char const *dom_u);

/**
 * Get status of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get status of
 * @param status        Storage to accept status
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_get_status(char const *ta,
                                              char const *dom_u,
                                              char       *status);

/**
 * Set status of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get status of
 * @param status        New domU status
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_set_status(char const *ta,
                                              char const *dom_u,
                                              char const *status);

/**
 * Get memory size that wiil be specified in creation of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get status of
 * @param size          Storage to accept domU memory size
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_get_memory_size(char const   *ta,
                                                   char const   *dom_u,
                                                   unsigned int *size);

/**
 * Set memory size that wiil be specified in creation of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get memory size of
 * @param size          New domU memory size
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_set_memory_size(char const  *ta,
                                                   char const  *dom_u,
                                                   unsigned int size);

/**
 * Get IP address of the interface that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get IP address of
 * @param ip_addr       Storage to accept domU 'eth0' IP address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_get_ip_addr(char const      *ta,
                                               char const      *dom_u,
                                               struct sockaddr *ip_addr);

/**
 * Set IP address of the interface that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get IP address of
 * @param ip_addr       New domU 'eth0' IP address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_set_ip_addr(
                        char const            *ta,
                        char const            *dom_u,
                        struct sockaddr const *ip_addr);

/**
 * Get MAC address of 'eth0' of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get MAC address of
 * @param mac           Storage to accept domU 'eth0' MAC address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_get_mac_addr(char const *ta,
                                                char const *dom_u,
                                                uint8_t    *mac);

/**
 * Set MAC address of 'eth0' of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get MAC address of
 * @param mac           New domU 'eth0' MAC address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_set_mac_addr(char const    *ta,
                                                char const    *dom_u,
                                                uint8_t const *mac);

/**
 * Add new bridge to domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to create
 * @param bridge        Bridge name
 * @param if_name       Interface name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_add_bridge(char const *ta,
                                              char const *dom_u,
                                              char const *bridge,
                                              char const *if_name);

/**
 * Delete bridge from domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to destroy
 * @param bridge        Bridge name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_del_bridge(char const *ta,
                                              char const *dom_u,
                                              char const *bridge);

/**
 * Get the name of the interface that is used for testing communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get bridge name of
 * @param bridge        DomU bridge name
 * @param if_name       Storage to accept interface name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_get_if_name(char const *ta,
                                                      char const *dom_u,
                                                      char const *bridge,
                                                      char       *if_name);

/**
 * Set the name of the interface that is used for testing communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get bridge name of
 * @param bridge        DomU bridge name
 * @param if_name       Interface name
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_set_if_name(char const *ta,
                                                      char const *dom_u,
                                                      char const *bridge,
                                                      char const *if_name);

/**
 * Get IP address of the interface that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU
 * @param bridge        Name of the bridge interface is added to
 * @param ip_addr       Storage to accept IP address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_get_ip_addr(
                                           char const      *ta,
                                           char const      *dom_u,
                                           char const      *bridge,
                                           struct sockaddr *ip_addr);

/**
 * Set IP address of the interface that is used for RCF/RPC communication.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get IP address of
 * @param ip_addr       New domU 'eth0' IP address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_set_ip_addr(
                                   char const            *ta,
                                   char const            *dom_u,
                                   char const            *bridge,
                                   struct sockaddr const *ip_addr);

/**
 * Get MAC address of 'eth0' of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get MAC address of
 * @param mac           Storage to accept domU 'eth0' MAC address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_get_mac_addr(
                                                char const *ta,
                                                char const *dom_u,
                                                char const *bridge,
                                                uint8_t    *mac);

/**
 * Set MAC address of 'eth0' of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get MAC address of
 * @param mac           New domU 'eth0' MAC address
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_set_mac_addr(
                                                char const    *ta,
                                                char const    *dom_u,
                                                char const    *bridge,
                                                uint8_t const *mac);

/**
 * Get acceleration specification sign of a tested interface of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get MAC address of
 * @param accel         Storage to accept domU tested iface accel sign
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_get_accel(
                                                char const *ta,
                                                char const *dom_u,
                                                char const *bridge,
                                                te_bool    *accel);

/**
 * Set acceleration specification sign of a tested interface of domU.
 *
 * @param ta            Test Agent running withing dom0
 * @param dom_u         Name of domU to get tested iface accel sign of
 * @param accel         New domU tested iface accel sign
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_bridge_set_accel(
                                                char const *ta,
                                                char const *dom_u,
                                                char const *bridge,
                                                te_bool     accel);

/**
 * Set MAC address of 'eth0' of domU.
 *
 * @param from_ta       Test Agent running withing source dom0
 * @param to_ta         Test Agent running withing target dom0
 * @param dom_u         Name of domU to migrate
 * @param host          Host name or IP address to migrate to
 * @param live          Kind of migration to perform (live/non-live)
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_xen_dom_u_migrate(char const *from_ta,
                                           char const *to_ta,
                                           char const *dom_u,
                                           char const *host,
                                           te_bool     live);

/**@} <!-- END tapi_conf_xen --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_XEN_H__ */
