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
 * Get IP address of 'eth0' of domU.
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
 * Set IP address of 'eth0' of domU.
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


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_XEN_H__ */
