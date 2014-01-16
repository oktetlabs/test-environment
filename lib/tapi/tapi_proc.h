/** @file
 * @brief API to configure some system options via /proc/sys
 *
 * API declaration for access to some system options via /proc/sys
 *
 *
 * Copyright (C) 2013 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_PROC_H__
#define __TE_TAPI_PROC_H__

#include "te_errno.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Flush network routes.
 *
 * @param rpcs      RPC server
 *
 * @return @c 0 on success or @c -1 in case of failure
 */
extern int tapi_cfg_net_route_flush(rcf_rpc_server *rpcs);

/**
 * Set a new TCP syncookies value.
 *
 * @param rpcs      RPC server
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_syncookies_set(rcf_rpc_server *rpcs, int value,
                                            int *old_value);

/**
 * Get TCP syncookies value.
 *
 * @param rpcs      RPC server
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_syncookies_get(rcf_rpc_server *rpcs,
                                            int *value);

/**
 * Set a new TCP timestamps value.
 *
 * @param rpcs      RPC server
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_timestamps_set(rcf_rpc_server *rpcs, int value,
                                            int *old_value);

/**
 * Get TCP timestamps value.
 *
 * @param rpcs      RPC server
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_timestamps_get(rcf_rpc_server *rpcs,
                                            int *value);

/**
 * Get RPF filtering value of TA interface.
 *
 * @param rpcs      RPC server
 * @param ifname    Interface name
 * @param rp_filter location for RPF filtering value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rp_filter_get(rcf_rpc_server *rpcs,
                                          const char *ifname,
                                          int *rp_filter);

/**
 * Set RPF filtering value of TA interface.
 *
 * @param rpcs      RPC server
 * @param ifname    Interface name
 * @param rp_filter New RPF filtering value
 * @param old_value Location for previous RPF filtering value or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rp_filter_set(rcf_rpc_server *rpcs,
                                          const char *ifname,
                                          int rp_filter, int *old_value);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_PROC_H__ */
