/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of some standard input/output
 * functions and useful extensions.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
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
