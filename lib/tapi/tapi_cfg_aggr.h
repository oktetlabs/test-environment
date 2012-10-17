/** @file
 * @brief Test API to configure bonding and bridging.
 *
 * Definition of API to configure linux trunks (IEEE 802.3ad) and bridges.
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_CFG_AGGR_H__
#define __TE_TAPI_CFG_AGGR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 * @param ifname    Where to place the name of created
 *                  interface
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_create_bond(const char *ta, const char *name,
                                     char **ifname);

/**
 * Destroy bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_destroy_bond(const char *ta, const char *name);

/**
 * Add a slave interface to a bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 * @param slave_if  Name of interface to be enslaved
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_bond_enslave(const char *ta, const char *name,
                                      const char *slave_if);

/**
 * Release a slave interface from a bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 * @param slave_if  Name of interface to be freed
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_bond_free_slave(const char *ta, const char *name,
                                         const char *slave_if);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_AGGR_H__ */
