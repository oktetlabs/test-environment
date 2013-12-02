/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support declarations.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 *
 * $Id$
 */

#ifndef __TE_TA_UNIX_CONF_ROUTE_H__
#define __TE_TA_UNIX_CONF_ROUTE_H__

#include "te_errno.h"
#include "rcf_pch_ta_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize routing configuration.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_route_init(void);


/**
 * Find route and return its attributes.
 *
 * @param[inout] rt_info    Route related information
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_route_find(ta_rt_info_t *rt_info);

/** 
 * Change route.
 *
 * @param action    What to do with this route
 * @param rt_info   Route-related information
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_route_change(ta_cfg_obj_action_e  action,
                                          ta_rt_info_t        *rt_info);

/**
 * Get instance list for object "/agent/route".
 *
 * @param list      Location for the list pointer
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_route_list(char **list);


/**
 * Get list of 'blackhole' routes.
 *
 * @param list      Location for pointer to allocated string
 * 
 * @return Status code.
 */
extern te_errno ta_unix_conf_route_blackhole_list(char **list);

/**
 * Add 'blackhole' route.
 *
 * @param rt_info   Route information
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_route_blackhole_add(ta_rt_info_t *rt_info);

/**
 * Delete 'blackhole' route.
 *
 * @param rt_info   Route information
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_route_blackhole_del(ta_rt_info_t *rt_info);


/**
 * Resolve outgoing interface for destination.
 *
 * @param rt_info   Route information with specified destination
 *
 * @return Status code.
 *
 * @attention If destination is not directly reachable it is replaced
 *            with gateway address.
 */
extern te_errno ta_unix_conf_outgoing_if(ta_rt_info_t *rt_info);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TA_UNIX_CONF_ROUTE_H__ */
