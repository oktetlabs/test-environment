/** @file
 * @brief Test API to access configuration model
 *
 * Definition of API.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_CFG_H_
#define __TE_TAPI_CFG_H_

#include "te_stdint.h"
#include "conf_api.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get son of configuration node with MAC address value.
 *
 * @param father    father's OID
 * @param subid     son subidentifier
 * @param name      name of the son or "" (empty string)
 * @param p_mac     location for MAC address (at least ETHER_ADDR_LEN)
 *
 * @return Status code.
 */
extern int tapi_cfg_get_son_mac(const char *father, const char *subid,
                                const char *name, uint8_t *p_mac);


extern int tapi_cfg_switch_add_vlan(const char *ta_name, uint16_t vid);
extern int tapi_cfg_switch_del_vlan(const char *ta_name, uint16_t vid);
extern int tapi_cfg_switch_vlan_add_port(const char *ta_name,
                                         uint16_t vid, unsigned int port);
extern int tapi_cfg_switch_vlan_del_port(const char *ta_name,
                                         uint16_t vid, unsigned int port);

/**
 * Add a new route via a gateway to some destination address
 *
 * @param ta           Test agent
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 *
 * @return Status code
 *
 * @retval 0  - on success
 */


/**
 * Delete a particular route identified by destination and gateway addresses
 *
 * @param ta           Test agent
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 *
 * @return Status code
 *
 * @retval 0  - on success
 */
extern int tapi_cfg_del_route(const char *ta, int addr_family,
                              const void *dst_addr, int prefix,
                              const void *gw_addr);

extern int tapi_cfg_add_route_my(const char *ta, int addr_family,
                                 const void *dst_addr, int prefix,
                                 const void *gw_addr, const char *dev,
                                 uint32_t flags,
                                 int metric, int mss, int win, int irtt);

extern int tapi_cfg_del_route_my(const char *ta, int addr_family,
                                 const void *dst_addr, int prefix);

/**
 * Add a new static ARP entry on specified agent
 *
 * @param ta           Test agent
 * @param net_addr     IPv4 network address
 * @param link_addr    IEEE 802.3 Link layer address
 *
 * @return Status code
 *
 * @retval 0  - on success
 *
 * @note Currently the function supports only (IPv4 -> IEEE 802.3 ethernet) 
 * entries. In the future it might be extended with an additional parameter
 * hw_type to support different classes of link layer addresses.
 */
extern int tapi_cfg_add_arp_entry(const char *ta,
                                  const void *net_addr,
                                  const void *link_addr);

/**
 * Delete a particular ARP entry from ARP table on specified agent
 *
 * @param ta           Test agent
 * @param net_addr     IPv4 network address
 * @param link_addr    IEEE 802.3 Link layer address
 *
 * @return Status code
 *
 * @retval 0  - on success
 *
 * @note Currently the function supports only (IPv4 -> IEEE 802.3 ethernet) 
 * entries. In the future it might be extended with an additional parameter
 * hw_type to support different classes of link layer addresses.
 */
extern int tapi_cfg_del_arp_entry(const char *ta,
                                  const void *net_addr,
                                  const void *link_addr);

/**
 * Returns hardware address of specified interface on a particular 
 * test agent.
 * 
 * @param ta          Test agent
 * @param ifname      Interface name whose hardware address is obtained
 * @param hwaddr      Hardware address - link-layer address (OUT)
 * @param hwaddr_len  Length of 'hwaddr' buffer (IN/OUT)
 *
 * @return Status of the oprtation
 * @retval 0         on success
 * @retval EMSGSIZE  Buffer is too short to fit the hardware address
 */
extern int tapi_cfg_get_hwaddr(const char *ta,
                               const char *ifname,
                               void *hwaddr, unsigned int *hwaddr_len);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_H_ */
