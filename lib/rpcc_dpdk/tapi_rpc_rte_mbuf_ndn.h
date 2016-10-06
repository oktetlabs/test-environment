/** @file
 * @brief RPC client API for RTE mbuf CSAP layer
 *
 * RPC client API to be used for traffic matching and conversion
 * between ASN.1 and RTE mbuf(s) representation
 * (declarations)
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_MBUF_NDN_H__
#define __TE_TAPI_RPC_RTE_MBUF_NDN_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"
#include "log_bufs.h"
#include "asn_usr.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_mbuf_ndn TAPI for RTE mbuf layer API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Convert an ASN.1 traffic template to RTE mbuf(s)
 *
 * @param[in]  template    ASN.1 traffic template
 * @param[in]  mp          RTE mempool pointer
 * @param[out] mbufs       Location for RTE mbuf pointer(s)
 * @param[out] count       Location for the number of mbufs prepared
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_mk_mbuf_from_template(rcf_rpc_server   *rpcs,
                                         const asn_value  *template,
                                         rpc_rte_mempool_p mp,
                                         rpc_rte_mbuf_p  **mbufs,
                                         unsigned int     *count);

/**@} <!-- END te_lib_rpc_rte_mbuf_ndn --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_MBUF_NDN_H__ */
