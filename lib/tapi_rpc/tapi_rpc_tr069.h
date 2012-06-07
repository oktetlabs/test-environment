/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of some standard input/output
 * functions and useful extensions.
 *
 *
 * Copyright (C) 2009-2010 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_TR069_H__
#define __TE_TAPI_RPC_TR069_H__

#include <stdio.h>

#include "rcf_rpc.h"

#include "te_cwmp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_tr069 TAPI for TR-069 ACS
 * @ingroup te_lib_rpc_tapi
 * @{
 */

extern te_errno rpc_cwmp_op_call(rcf_rpc_server *rpcs,
                                 const char *acs_name,
                                 const char *cpe_name,
                                 te_cwmp_rpc_cpe_t cwmp_rpc,
                                 uint8_t *buf, size_t buflen, 
                                 acse_request_id_t *request_id);


/**
 * Check status of queued CWMP RPC on ACSE or get received ACS RPC.
 * 
 * @param rpcs          TE rpc server.
 * @param acs_name      name of ACS object on ACSE.
 * @param cpe_name      name of CPE record on ACSE.
 * @param request_id    queue index of CWMP call on ACSE.
 * @param cwmp_rpc_acs  type of RPC message to ACS, should be zero 
 *                       for get response on issued CPE RPC.
 * @param cwmp_rpc      location for type of RPC response, may be NULL.
 * @param buf           location for ptr to response data; 
 *                              user have to free() it.
 * @param buflen        location for size of response data.
 *
 * @return status
 */
extern te_errno rpc_cwmp_op_check(rcf_rpc_server *rpcs,
                                  const char *acs_name,
                                  const char *cpe_name,
                                  acse_request_id_t request_id,
                                  te_cwmp_rpc_acs_t cwmp_rpc_acs,
                                  te_cwmp_rpc_cpe_t *cwmp_rpc,
                                  uint8_t **buf, size_t *buflen);


extern te_errno rpc_cwmp_conn_req(rcf_rpc_server *rpcs,
                                  const char *acs_name,
                                  const char *cpe_name);

extern te_errno rpc_cwmp_get_inform(rcf_rpc_server *rpcs,
                                    const char *acs_name,
                                    const char *cpe_name, 
                                    int index, uint8_t *buf,
                                    size_t *buflen);
/**@} <!-- END te_lib_rpc_tr069 --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RPC_TR069_H__ */
