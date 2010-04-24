/** @file
 * @brief Test API for ACSE usage
 *
 * Declarations of Test API to ACS Emulator on Test Agent.
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

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"

#include "te_cwmp.h"
#include "cwmp_soapStub.h"

#ifdef __cplusplus
extern "C" {
#endif


/** End of var-args list. */
extern void * const va_end_list_ptr;
#define VA_END_LIST (va_end_list_ptr)

/*
 * ================= Configuring of ACSE ===================
 */

typedef enum {
    ACSE_OP_ADD,
    ACSE_OP_DEL,
    ACSE_OP_MODIFY,
    ACSE_OP_OBTAIN,
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
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param opcode        code of operation to be performed with 
 *                      specified ACS object: 
 *                          if @c ACSE_OP_ADD, object should not exist,
 *                          if @c ACSE_OP_MODIFY, object should exist;
 * @param ...           list of pairs (@p name, @p value), where @p name
 *                      should match with name of leaf under 
 *                      $c /agent/acse/acs/ in ConfiguratorModel, and 
 *                      #value is new value or place for current value;
 *                      list should be finished
 *                      with macro @c VA_END_LIST.
 *                      Type of @p value should correspond to type of 
 *                      @p name leaf in Conḟ.Model.
 *                      List should be empty if @p opcode is 
 *                      @c ACSE_OP_DEL.
 *                      List may be non-empty if @p opcode is 
 *                      @c ACSE_OP_ADD, in this case values are set 
 *                      after add.
 *
 * @return status code
 */
extern te_errno tapi_acse_manage_acs(const char *ta,
                                     const char *acs_name,
                                     acse_op_t opcode,
                                     ...);


/**
 * Manage CPE record on the ACSE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 * @param opcode        code of operation to be performed with 
 *                      specified CPE record;
 * @param ...           list of parameters, with same semantic 
 *                      as for function tapi_acse_manage_acs().
 *
 * @return status code
 */
extern te_errno tapi_acse_manage_cpe(const char *ta,
                                     const char *acs_name,
                                     const char *cpe_name,
                                     acse_op_t opcode,
                                     ...);

    
/*
 * ================= CWMP processing ===================
 */

/**
 * Get last Inform from particular CPE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 * @param inform_descr  Inform data (OUT);
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_last_inform(const char *ta,
                                     const char *acs_name,
                                     const char *cpe_name,
                                     _cwmp__Inform *inform_descr);

/**
 * Get state of CWMP session with particular CPE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 * @param state         Session state (OUT);
 *
 * @return status code
 */
extern te_errno tapi_acse_session_state(const char *ta,
                                     const char *acs_name,
                                     const char *cpe_name,
                                     cwmp_sess_state_t *state);

/**
 * Issue CWMP ConnectionRequest to particular CPE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_connect(const char *ta,
                                     const char *acs_name,
                                     const char *cpe_name);

/**
 * Finish CWMP session with particular CPE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_disconnect(const char *ta,
                                         const char *acs_name,
                                         const char *cpe_name);


typedef uint32_t tapi_acse_call_handle_t;

/* TODO: think, maybe use TE common te_errno instead special 
 * call status? Or encode here some [TR-069] error codes? */
typedef enum {
    ACSE_RPC_PENDING,
    ACSE_RPC_SUCCESS,
    ACSE_RPC_TIMEDOUT,
} tapi_acse_call_status_t;

extern te_errno tapi_acse_cpe_rpc_call(const char *ta,
                                     const char *acs_name,
                                     const char *cpe_name,
                                     te_cwmp_rpc_cpe_t cpe_rpc_code,
                                     tapi_acse_call_handle_t *call,
                                     ...);

extern te_errno tapi_acse_cpe_rpc_call_response(const char *ta,
                                     const char *acs_name,
                                     const char *cpe_name,
                                     tapi_acse_call_handle_t call,
                                     tapi_acse_call_status_t *status);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __TE_TAPI_ACSE_H__ */
