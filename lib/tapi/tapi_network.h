/** @file
 * @brief High level test API to configure tested network
 *
 * @defgroup ts_tapi_network High level TAPI to configure network
 * @ingroup te_ts_tapi
 * @{
 *
 * This API can be used to set up network configurations like resources
 * reservation, assigning IP addresses etc with minimum efforts.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 */

#ifndef __TE_TAPI_NETWORK_H_
#define __TE_TAPI_NETWORK_H_

#include "te_defs.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reserve resources, set IP addresses and static ARP (if required) in
 * accordance to the current Configurator configuration.
 *
 * @note The function jumps to @b cleanup in case of failure.
 *
 * @param ipv6_supp  @c TRUE if IPv6 addressing is supported.
 */
extern void tapi_network_setup(te_bool ipv6_supp);

/**
 * Flush ARP table for the interface @p ifname.
 *
 * @param rpcs      RPC server handle
 * @param ifname    Interface name
 *
 * @return Status code.
 */
extern te_errno tapi_neight_flush(rcf_rpc_server *rpcs, const char *ifname);

/**
 * Flush ARP table for all interfaces on test agent @p rpcs->ta.
 *
 * @param rpcs      RPC server handle
 *
 * @return Status code.
 */
extern te_errno tapi_neight_flush_ta(rcf_rpc_server *rpcs);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_NETWORK_H_ */

/**@} <!-- END ts_tapi_network --> */
