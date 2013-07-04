/** @file
 * @brief Test Environment
 *
 * A test eth configuration
 * 
 * Copyright (C) 2004-20013 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_TAPI_CFG_ETH_EXT_H__
#define __TE_TAPI_CFG_ETH_EXT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get link number of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param link      Location for the link number
 * 
 * @return Status code
 */
extern te_errno tapi_eth_link_get(const char *ta, const char *ifname,
                                 int *link);

/**
 * Get GRO value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gro       Location for the GRO value
 * 
 * @return Status code
 */
extern te_errno tapi_eth_gro_get(const char *ta, const char *ifname,
                                 int *gro);

/**
 * Get GSO value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gso       Location for the GSO value
 * 
 * @return Status code
 */
extern te_errno tapi_eth_gso_get(const char *ta, const char *ifname,
                                 int *gso);

/**
 * Get TSO value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tso       The TSO value location
 * 
 * @return Status code
 */
extern te_errno tapi_eth_tso_get(const char *ta, const char *ifname,
                                 int *tso);

/**
 * Get flags value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param flags     Location for the eth flags
 * 
 * @return Status code
 */
extern te_errno tapi_eth_flags_get(const char *ta, const char *ifname,
                                 int *flags);

/**
 * Set GRO value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gro       A new GRO value
 * 
 * @return Status code
 */
extern te_errno tapi_eth_gro_set(const char *ta, const char *ifname,
                                 int gro);

/**
 * Set GSO value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gso       A new GSO value
 * 
 * @return Status code
 */
extern te_errno tapi_eth_gso_set(const char *ta, const char *ifname,
                                 int gso);

/**
 * Set TSO value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tso       The TSO value
 * 
 * @return Status code
 */
extern te_errno tapi_eth_tso_set(const char *ta, const char *ifname,
                                 int tso);

/**
 * Set flags value of an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param flags     A new eth flags value
 * 
 * @return Status code
 */
extern te_errno tapi_eth_flags_set(const char *ta, const char *ifname,
                                 int flags);

/**
 * Reset an ethernet interface
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * 
 * @return Status code
 */
extern te_errno tapi_eth_reset(const char *ta, const char *ifname);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_ETH_EXT_H__ */
