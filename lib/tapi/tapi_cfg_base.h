/** @file
 * @brief Network Configuration Model TAPI
 *
 * Definition of test API for basic configuration model
 * (storage/cm/cm_base.xml).
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

#ifndef __TE_TAPI_CFG_BASE_H__
#define __TE_TAPI_CFG_BASE_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Enable/disable IPv4 forwarding on the Test Agent.
 *
 * @param ta        TA name
 * @param enabled   IN: TRUE - enable, FALSE - disable;
 *                  OUT: previous state of the forwarding
 *
 * @return Status code.
 */
extern int tapi_cfg_base_ipv4_fw(const char *ta, te_bool *enabled);

/**
 * Get MAC address of TA interface.
 *
 * @param oid       instance OID of TA interface
 * @param mac       location for MAC address (at least ETHER_ADDR_LEN)
 *
 * @return Status code.
 */
extern int tapi_cfg_base_if_get_mac(const char *oid, uint8_t *mac);

/**
 * Get MTU (layer 2 payload) of the Test Agent interface.
 *
 * @param oid       TA interface oid, e.g. /agent:A/interface:eth0
 * @param p_mtu     location for MTU
 *
 * @return Status code.
 */
extern int tapi_cfg_base_if_get_mtu(const char *oid, unsigned int *p_mtu);

/**
 * Add network address (/net_addr:).
 *
 * @param oid       TA interface oid, e.g. /agent:A/interface:eth0
 * @param addr      Address to add
 * @param prefix    Address prefix length (0 - default, -1 - do not set)
 * @param set_bcast Set broadcast address or not
 * @param cfg_hndl  Configurator handle of the new address
 *
 * @return Status code.
 * @retval EAFNOSUPPORT     Address family is not supported
 * @retval EEXIST           Address already exist
 */
extern int tapi_cfg_base_add_net_addr(const char            *oid,
                                      const struct sockaddr *addr,
                                      int                    prefix,
                                      te_bool                set_bcast,
                                      cfg_handle            *cfg_hndl);

/**
 * Wrapper over tapi_cfg_base_add_net_addr() function.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name on the Agent
 * @param addr      Address to add
 * @param prefix    Address prefix length (0 - default, -1 - do not set)
 * @param set_bcast Set broadcast address or not
 * @param cfg_hndl  Configurator handle of the new address
 *
 * @return See return value of tapi_cfg_base_add_net_addr()
 */
static inline int
tapi_cfg_base_if_add_net_addr(const char *ta, const char *ifname,
                              const struct sockaddr *addr,
                              int prefix, te_bool set_bcast,
                              cfg_handle *cfg_hndl)
{
    char inst_name[CFG_OID_MAX];

    snprintf(inst_name, sizeof(inst_name),
             "/agent:%s/interface:%s", ta, ifname);
    return tapi_cfg_base_add_net_addr(inst_name, addr, prefix, set_bcast,
                                      cfg_hndl);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_BASE_H__ */
