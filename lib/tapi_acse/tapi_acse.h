/** @file
 * @brief Test API 
 *
 * Declaration of Test API
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_ACSE_H__
#define __TE_TAPI_ACSE_H__

#include "te_stdint.h"
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * ================= Configuring of ACSE ===================
 */

typedef enum {
    ACSE_OP_ADD,
    ACSE_OP_REMOVE,
    ACSE_OP_CHANGE,
} acse_op_t;

/**
 * Start ACSE process under specified Test Agent.
 *
 * @param ta            Test Agent name;
 *
 * @return status code
 */
extern te_errno tapi_acse_start(const char *ta);

/**
 * Stop ACSE process under specified Test Agent.
 *
 * @param ta            Test Agent name;
 *
 * @return status code
 */
extern te_errno tapi_acse_stop(const char *ta);

/**
 * Manage ACS object on the ACSE.
 *
 * @param ta            Test Agent name;
 * @param acs_handle    Name of ACS object within @p ta ACSE;
 * @param opcode        code of operation to be performed with 
 *                      specified ACS object: 
 *                          if @c ACSE_OP_ADD, object should not exist,
 *                          if @c ACSE_OP_CHANGE, object should exist, and
 *                              only non-NULL parameters will changed;
 * @param url           HTTP URL for @p acs_handle or NULL;
 * @param login         HTTP login name for @p acs_handle or NULL;
 * @param password      HTTP password for @p acs_handle or NULL;
 * @param ssl_cert      SSL certificate for @p acs_handle or NULL;
 *
 * @return status code
 */
extern te_errno tapi_acse_manage_acs(const char *ta,
                                     const char *acs_handle,
                                     acse_op_t opcode,
                                     const char *url,
                                     const char *login,
                                     const char *password,
                                     const char *ssl_cert);


/**
 * Manage CPE record on the ACSE.
 *
 * @param ta            Test Agent name;
 * @param acs_handle    Name of ACS object within @p ta ACSE;
 * @param cpe_handle    Name of CPE object within @p ta ACSE;
 * @param opcode        code of operation to be performed with 
 *                      specified CPE object;
 * @param url           HTTP URL for Connection Request to this 
 *                      CPE, or NULL;
 * @param login         HTTP login name for CPE or NULL;
 * @param password      HTTP password for CPE or NULL;
 * @param ssl_cert      SSL certificate for CPE or NULL;
 *
 * @return status code
 */
extern te_errno tapi_acse_manage_cpe(const char *ta,
                                     const char *acs_handle,
                                     const char *cpe_handle,
                                     acse_op_t opcode,
                                     const char *url,
                                     const char *login,
                                     const char *password,
                                     const char *ssl_cert);

    
/*
 * ================= CWMP processing ===================
 */

extern te_errno tapi_acse_cpe_last_inform(const char *ta,
                                     const char *acs_handle,
                                     const char *cpe_handle,
                                     te_cwmp_inform_t *inform_descr,
);

extern te_errno tapi_acse_session_state(const char *ta,
                                     const char *acs_handle,
                                     const char *cpe_handle,
                                     te_cwmp_session_state_t *state,
);

extern te_errno tapi_acse_cpe_connect(const char *ta,
                                     const char *acs_handle,
                                     const char *cpe_handle,
);

extern te_errno tapi_acse_cpe_disconnect(const char *ta,
                                     const char *acs_handle,
                                     const char *cpe_handle,
);


typedef uint32_t tapi_acse_call_handle_t;

/* TODO: think, maybe use TE common te_errno instead special 
 * call status? Or encode here some [TR-069] error codes? */
typedef enum {
    ACSE_RPC_SUCCESS,
    ACSE_RPC_TIMEDOUT,
} tapi_acse_call_status_t;

extern te_errno tapi_acse_cpe_rpc_call(const char *ta,
                                     const char *acs_handle,
                                     const char *cpe_handle,
                                     te_cwmp_rpc_cpe_t cpe_rpc_code,
                                     tapi_acse_call_handle_t *call,
                                     ...
);

extern te_errno tapi_acse_cpe_rpc_call_response(const char *ta,
                                     const char *acs_handle,
                                     const char *cpe_handle,
                                     tapi_acse_call_handle_t call,
                                     tapi_acse_call_status_t *status
);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __TE_TAPI_ACSE_H__ */
