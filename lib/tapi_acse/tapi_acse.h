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

#include "tapi_rpc.h"
#include "te_cwmp.h"
#include "cwmp_soapStub.h"
#include "cwmp_utils.h"

#ifdef __cplusplus
extern "C" {
#endif




#define CWMP_FAULT(__p) ((_cwmp__Fault *)(__p))

/**
 * Check status of CWMP RPC response in main test body.
 * If the expression is something else than zero, test fails.
 * If there was CWMP Fault detected, it will be print in log, and
 * test fails.
 * 
 * @param expr_  Expression with TAPI call to get CWMP RPC response.
 * @param var_   Variable where pointer to response should be put.
 */
#define CHECK_CWMP_RESP_RC(expr_, var_) \
    do {                                                                \
        int rc_;                                                        \
                                                                        \
        if (0 == (rc_ = (expr_)))                                       \
            break;                                                      \
        if (TE_CWMP_FAULT == TE_RC_GET_ERROR(rc_))                      \
        {                                                               \
            _cwmp__Fault *f = (_cwmp__Fault *) var_;                    \
            TEST_FAIL("CWMP Fault received: %s(%s)",                    \
                        f->FaultCode, f->FaultString);                  \
        }                                                               \
        else                                                            \
            TEST_FAIL("line %d: %s returns 0x%X (%r), but expected 0",  \
                      __LINE__, # expr_, rc_, rc_);                     \
    } while (0)



/**
 * Descriptor of TAPI context for work with ACSE.
 */
typedef struct tapi_acse_context_s {
    const char     *ta;      /*< Name of TA, which is connected with
                                  ACSE. Init from test argument 'ta_acse'.
                                  If user change it, he must ensure,
                                  that new TA has started its ACSE, and
                                  RPC server is actual. 
                                  It is highly recommended do NOT change
                                  this field, but init new context for
                                  another TA. */
    rcf_rpc_server *rpc_srv; /*< TA RCF RPC server for connection with
                                  ACSE.
                                  Init: started server with name
                                  'acse_ctl' on TA. */

    const char     *acs_name;/*< Name of used ACS object on ACSE.
                                  Init: first presend ACS object in 
                                  CS subtree on the ACSE. 
                                  If user change it, he must ensure
                                  presense of that ACS object. */
    const char     *cpe_name;/*< Name of used CPE record on ACSE.
                                  Init: first presend ACS object in CS
                                  subtree on the ACSE. 
                                  If user change it, he must ensure
                                  presense of that CPE record. */
    int             timeout; /*< Timeout in seconds of operation. 
                                  Has sense only for get response from ACSE.
                                  Because poll is absent in communication
                                  with ACSE, such methods ask periodically
                                  whether response was got on ACSE. 
                                  User is free to change it. */

    acse_request_id_t req_id;/*< CWMP request ID.  This field is filled
                                  by 'call' methods, and is used by
                                  'get_response' methods. 
                                  Thus, if user change it before get
                                  response, he will get response for some
                                  other operation then the last one. */
} tapi_acse_context_t;


/**
 * Init ACSE TAPI context, according with its description.
 *
 * @param ta            name of TA where ACSE is started.
 *
 * @return new allocated and correctly initialized context, or NULL
 *         if there is some test configuration errors. 
 */
extern tapi_acse_context_t *tapi_acse_ctx_init(const char *ta);


/**
 * Macro for usage at start of CWMP tests to init context.
 * Assume test paremeter 'ta_acse' with name of TA with ACSE.
 */
#define TAPI_ACSE_CTX_INIT(ctx_var_) \
    do {                                                        \
        const char *ta_acse;                                    \
        TEST_GET_STRING_PARAM(ta_acse);                         \
        if (NULL == (ctx_var_ = tapi_acse_ctx_init(ta_acse)))   \
            TEST_FAIL("Init ACSE TAPI context failed");         \
    } while (0)

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
 * ==================== Useful config ACSE methods =====================
 */
    
/**
 * Clear CWMP activity and its cache on ACS object on the ACSE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 *
 * @return status code
 */
extern te_errno tapi_acse_clear_acs(const char *ta, const char *acs_name);

/**
 * Clear CWMP activity and its cache on CPE object on the ACSE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 *
 * @return status code
 */
extern te_errno tapi_acse_clear_cpe(const char *ta,
                                    const char *acs_name,
                                    const char *cpe_name);

/**
 * Wait for particular state of CWMP session with specified CPE on ACSE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 * @param want_state    State to wait.
 * @param timeout       Timeout in seconds.
 *
 * @return status code; TE_ETIMEDOUT if timeout was expired and state
 *              was not occured.
 */
extern te_errno tapi_acse_wait_cwmp_state(const char *ta,
                                          const char *acs_name,
                                          const char *cpe_name,
                                          cwmp_sess_state_t want_state,
                                          int timeout);

/**
 * Wait for particular state of ConnectionRequest to specified CPE on ACSE.
 *
 * @param ta            Test Agent name;
 * @param acs_name      Name of ACS object within @p ta ACSE;
 * @param cpe_name      Name of CPE record within @p ta ACSE;
 * @param want_state    State to wait.
 * @param timeout       Timeout in seconds.
 *
 * @return status code; TE_ETIMEDOUT if timeout was expired and state
 *              was not occured.
 */
extern te_errno tapi_acse_wait_cr_state(const char *ta,
                                        const char *acs_name,
                                        const char *cpe_name,
                                        acse_cr_state_t want_state,
                                        int timeout);
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
extern te_errno tapi_acse_cpe_connect(rcf_rpc_server *acse_rpcs,
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
extern te_errno tapi_acse_cpe_disconnect(rcf_rpc_server *acse_rpcs,
                                         const char *acs_name,
                                         const char *cpe_name);


/* TODO: think, maybe use TE common te_errno instead special 
 * call status? Or encode here some [TR-069] error codes? */
typedef enum {
    ACSE_RPC_PENDING,
    ACSE_RPC_SUCCESS,
    ACSE_RPC_TIMEDOUT,
} tapi_acse_call_status_t;



/**
 * Call CPE RPC method.
 *
 * @param rpcs          TE RPC server handle
 * @param acs_name      name of ACS object on ACSE.
 * @param cpe_name      name of CPE record on ACSE.
 * @param call_index    location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_rpc_call(rcf_rpc_server *rpcs,
                                       const char *acs_name,
                                       const char *cpe_name,
                                       te_cwmp_rpc_cpe_t cpe_rpc_code,
                                       acse_request_id_t *request_id,
                                       cwmp_data_to_cpe_t to_cpe);

/**
 * Check status of queued CWMP RPC on ACSE.
 * 
 * @param rpcs          TE rpc server.
 * @param acs_name      name of ACS object on ACSE.
 * @param cpe_name      name of CPE record on ACSE.
 * @param timeout       interval for wait response,
 *                              value <=0 for infinite wait.
 * @param call_index    queue index of CWMP call on ACSE.
 * @param cpe_rpc_code  location for type of RPC response, may be NULL.
 * @param buf           location for ptr to response data; 
 *                              user have to free() it.
 * @param buflen        location for size of response data.
 *
 * @return status
 */
extern te_errno tapi_acse_cpe_rpc_response(rcf_rpc_server *rpcs,
                                           const char *acs_name,
                                           const char *cpe_name,
                                           int timeout, int call_index,
                                           te_cwmp_rpc_cpe_t *cpe_rpc_code,
                                           cwmp_data_from_cpe_t *from_cpe);


/**
 * Check status of queued CWMP RPC on ACSE or get received ACS RPC.
 * Return ENOENT if there was not such RPC caught from specified CPE.
 * 
 * @param rpcs          TE rpc server.
 * @param acs_name      name of ACS object on ACSE.
 * @param cpe_name      name of CPE record on ACSE.
 * @param timeout       interval for wait response,
 *                              value <=0 for infinite wait.
 * @param rpc_acs       type of ACS RPC.
 * @param buf           location for ptr to response data; 
 *                              user have to free() it.
 * @param buflen        location for size of response data.
 *
 * @return status
 */
extern te_errno tapi_acse_get_rpc_acs(rcf_rpc_server *rpcs,
                                   const char *acs_name,
                                   const char *cpe_name,
                                   int timeout,
                                   te_cwmp_rpc_acs_t rpc_acs,
                                   cwmp_data_from_cpe_t *from_cpe);




/*
 * ==================== CWMP RPC methods =========================
 */



/**
 * Call CPE GetRPCMethods method.
 *
 * @param rpcs        TE RPC server handle
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_rpc_methods(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int *call_index);

/**
 * Get CPE GetRPCMethods response.
 * If there was CWMP Fault received as response, then TE_CWMP_FAULT
 * returned, and ptr to fault struct is stored in #resp place.
 *
 * @param rpcs     TE RPC server handle.
 * @param acs_name Name of ACS object on ACSE.
 * @param cpe_name Name of CPE object on ACSE.
 * @param timeout  Timeout in seconds to wait response,
 *                      zero for return immediately, just check,
 *                      negative value for wait forever.
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the GetRPCMethods method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_rpc_methods_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout, int call_index,
               _cwmp__GetRPCMethodsResponse **resp);
/**
 * Call CPE SetParameterValues method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the SetParameterValues method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_set_parameter_values(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__SetParameterValues *req,
               int *call_index);

/**
 * Get CPE SetParameterValues response.
 * If there was CWMP Fault received as response, then TE_CWMP_FAULT
 * returned, and ptr to fault struct is stored in #resp place.
 *
 * @param rpcs     RPC server handle
 * @param acs_name Name of ACS object on ACSE.
 * @param cpe_name Name of CPE object on ACSE.
 * @param timeout  Timeout in seconds to wait response,
 *                    zero for return immediately, just check,
 *                      negative value for wait forever.
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the SetParameterValues method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_set_parameter_values_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout, int call_index,
               _cwmp__SetParameterValuesResponse **resp);


/**
 * Call CPE GetParameterValues method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetParameterValues method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_parameter_values(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__GetParameterValues *req,
               int *call_index);

/**
 * Get CPE GetParameterValues response
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the GetParameterValues method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_parameter_values_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               _cwmp__GetParameterValuesResponse **resp);


/**
 * Call CPE GetParameterNames method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetParameterNames method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_parameter_names(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__GetParameterNames *req,
               int *call_index);

/**
 * Get CPE GetParameterNames response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the GetParameterNames method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_parameter_names_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               _cwmp__GetParameterNamesResponse **resp);


/**
 * Call CPE SetParameterAttributes method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the SetParameterAttributes method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_set_parameter_attributes(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__SetParameterAttributes *req,
               int *call_index);

/**
 * Get CPE SetParameterAttributes response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_set_parameter_attributes_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index);


/**
 * Call CPE GetParameterAttributes method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetParameterAttributes method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_parameter_attributes(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__GetParameterAttributes *req,
               int *call_index);

/**
 * Get CPE GetParameterAttributes response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the GetParameterAttributes method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_parameter_attributes_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               _cwmp__GetParameterAttributesResponse **resp);

/**
 * Call CPE AddObject method.
 *
 * @param ctx        ACSE TAPI context.
 * @param obj_name   name of object for AddObject method.
 * @param param_key  ParameterKey for AddObject method.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_add_object(tapi_acse_context_t *ctx,
                                         const char *obj_name,
                                         const char *param_key);

/**
 * Get CPE AddObject response.
 *
 * @param ctx        ACSE TAPI context.
 * @param obj_index  location for index of created object,
 *                              unchanged on error.
 * @param add_status location for Status from AddObjectResponse.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_add_object_resp(tapi_acse_context_t *ctx,
                                              int *obj_index,
                                              int *add_status);

/**
 * Call CPE DeleteObject method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the DeleteObject method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_delete_object(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               const char *obj_name,
               const char *param_key,
               int *call_index);
/**
 * Get CPE DeleteObject response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the DeleteObject method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_delete_object_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               int *add_status);

/**
 * Call CPE Reboot method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the Reboot method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_reboot(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__Reboot *req,
               int *call_index);

/**
 * Get CPE Reboot response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_reboot_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index);

/** 
 * Call CPE Download method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the Download method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_download(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__Download *req,
               int *call_index);

/** 
 * Get CPE Download response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the Download method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_download_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout, int call_index,
               _cwmp__DownloadResponse **resp);

/**
 * Call CPE Upload method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the Upload method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_upload(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__Upload *req,
               int *call_index);
/**
 * Get CPE Upload response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the Upload method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_upload_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               _cwmp__UploadResponse **resp);

/**
 * Call CPE FactoryReset method.
 *
 * @param rpcs     RPC server handle
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_factory_reset(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int *call_index);

/**
 * Get CPE FactoryReset response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_factory_reset_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index);

/**
 * Call CPE GetQueuedTransfers method.
 *
 * @param rpcs     RPC server handle
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_queued_transfers(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int *call_index);
/**
 * Get CPE GetQueuedTransfers response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the GetQueuedTransfers method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_queued_transfers_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               _cwmp__GetQueuedTransfersResponse **resp);

/**
 * Call CPE GetAllQueuedTransfers method.
 *
 * @param rpcs     RPC server handle
 * @param call_index  location for index of TR RPC.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_all_queued_transfers(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int *call_index);

/**
 * Get CPE GetAllQueuedTransfers response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the GetAllQueuedTransfers method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_all_queued_transfers_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               _cwmp__GetAllQueuedTransfersResponse **resp);

/**
 * Call CPE ScheduleInform method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the ScheduleInform method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_schedule_inform(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__ScheduleInform *req,
               int *call_index);

/**
 * Get CPE ScheduleInform response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_schedule_inform_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index);

/**
 * Call CPE SetVouchers method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the SetVouchers method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_set_vouchers(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__SetVouchers *req,
               int *call_index);
/**
 * Get CPE SetVouchers response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_set_vouchers_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index);

/**
 * Call CPE GetOptions method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetOptions method
 * @param call_index  location for index of TR RPC (OUT).
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_options(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               _cwmp__GetOptions *req,
               int *call_index);
/**
 * Get CPE GetOptions response.
 *
 * @param rpcs     RPC server handle
 * @param call_index  index of TR RPC.
 * @param resp     CPE response to the GetOptions method
 *
 * @return status code
 */
extern te_errno tapi_acse_cpe_get_options_resp(
               rcf_rpc_server *rpcs,
               const char *acs_name,
               const char *cpe_name,
               int timeout,
               int call_index,
               _cwmp__GetOptionsResponse **resp);





/*
 * ============= Useful routines for prepare CWMP RPC params =============
 */

/**
 * Construct GetParameterNames argument.
 */
extern _cwmp__GetParameterNames *cwmp_get_names_alloc(const char *name,
                                                      te_bool next_level);

/**
 * Free GetParameterNames argument.
 */
extern void cwmp_get_names_free(_cwmp__GetParameterNames *arg);

/**
 * Iterate over GetParameterNames response.
 */
#define GET_NAMES_RESP_FOREACH(resp_, item_) \
    for ()

/**
 * Free GetParameterNames response, which is got from regular TAPI.
 */
extern void cwmp_get_names_resp_free(_cwmp__GetParameterNamesResponse *r);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __TE_TAPI_ACSE_H__ */
