/** @file
 * @brief Test API for ACSE usage
 *
 * Declarations of Test API to ACS Emulator on Test Agent.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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

// #include "tapi_rpc.h"
#include "rcf_rpc.h"
#include "te_cwmp.h"
#include "cwmp_soapStub.h"
#include "cwmp_utils.h"

#ifdef __cplusplus
extern "C" {
#endif




#define CWMP_FAULT(__p) ((cwmp_fault_t *)(__p))

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
            cwmp_fault_t *f = (cwmp_fault_t *) var_;                    \
            TEST_FAIL("CWMP Fault received: %s(%s)",                    \
                        f->FaultCode, f->FaultString);                  \
        }                                                               \
        else                                                            \
            TEST_FAIL("%s returns 0x%X (%r), but expected 0",  \
                       # expr_, rc_, rc_);                     \
    } while (0)

/**
 * Macro for usage at start of CWMP tests to init context.
 * Assume existing TA with name 'agt_acse' for ACSE.
 */
#define TAPI_ACSE_CTX_INIT(ctx_var_) \
    do {                                                        \
        const char *ta_acse = "agt_acse";                       \
        if (NULL == (ctx_var_ = tapi_acse_ctx_init(ta_acse)))   \
            TEST_FAIL("Init ACSE TAPI context failed");         \
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
                                  User is free to change it.
                                  After each operation is automatically
                                  set to value of def_timeout. */

    int             def_timeout; /*< Default timeout in seconds */

    acse_request_id_t req_id;/*< CWMP request ID.  This field is filled
                                  by 'call' methods, and is used by
                                  'get_response' methods.
                                  Thus, if user change it before get
                                  response, he will get response for some
                                  other operation then the last one. */

    int             prev_usleep; /*< Internal: previous sleep period */
    int             next_usleep; /*< Internal: next sleep period */
    te_bool         change_sync; /*< Internal: do we changed 'sync_mode'
                                  during connect, before establish
                                  CWMP session. */

} tapi_acse_context_t;


/**
 * Init ACSE TAPI context, according with its description.
 * This method expects defined environment variable CPE_NAME
 * with CS name of CPE which is interesing for running configuration.
 * By the way, while context is just inited and no operations
 * was performed, user is free to modify ACS and CPE names,
 * but in this case he is responsible for presence of wanted CPE cfg
 * parameters in TA acse CS subtree (use tapi_acse_ta_cs_init() ).
 *
 * @param ta            name of TA where ACSE is started.
 *
 * @return New allocated and correctly initialized context, or NULL
 *         if there is some test configuration errors.
 */
extern tapi_acse_context_t *tapi_acse_ctx_init(const char *ta);



/**
 * Copy parameters for ACS and CPE from local static CS subtree
 * to the run-time subtree on specified TA.
 *
 * @param ctx           ACSE TAPI context.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_ta_cs_init(tapi_acse_context_t *ctx);

/*
 * ================= Configuring of ACSE ===================
 */

/**
 * Operation types for configuring ACSE.
 */
typedef enum {
    ACSE_OP_ADD,        /**< Add new ACS/CPE record. */
    ACSE_OP_DEL,        /**< Remove ACS/CPE record. */
    ACSE_OP_MODIFY,     /**< Modify some parameter for ACS/CPE record. */
    ACSE_OP_OBTAIN,     /**< Obtain some parameter for ACS/CPE record. */
} acse_op_t;

/**
 * Start ACSE process under specified Test Agent.
 *
 * @param ta            Test Agent name;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_start(const char *ta);

/**
 * Stop ACSE process under specified Test Agent.
 *
 * @param ta            Test Agent name;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_stop(const char *ta);

/**
 * Manage ACS object on the ACSE.
 *
 * @param ctx           ACSE TAPI context.
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
 * @return Status code.
 */
extern te_errno tapi_acse_manage_acs(tapi_acse_context_t *ctx,
                                     acse_op_t opcode,
                                     ...);


/**
 * Manage CPE record on the ACSE.
 *
 * @param ctx           ACSE TAPI context.
 * @param opcode        code of operation to be performed with
 *                      specified CPE record;
 * @param ...           list of parameters, with same semantic
 *                      as for function tapi_acse_manage_acs().
 *
 * @return Status code.
 */
extern te_errno tapi_acse_manage_cpe(tapi_acse_context_t *ctx,
                                     acse_op_t opcode,
                                     ...);

/*
 * ==================== Useful config ACSE methods =====================
 */

/**
 * Clear CWMP activity and its cache on ACS object on the ACSE:
 * stop CWMP session, if any, and remove all requests in queue and
 * received responses
 *
 * @param ctx           ACSE TAPI context.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_clear_acs(tapi_acse_context_t *ctx);

/**
 * Clear CWMP activity and its cache on CPE object on the ACSE.
 *
 * @param ctx           ACSE TAPI context.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_clear_cpe(tapi_acse_context_t *ctx);


static inline te_errno
tapi_acse_get_cwmp_state(tapi_acse_context_t *ctx,
                         cwmp_sess_state_t *state)
{
    return tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                "cwmp_state", state, VA_END_LIST);
}

/**
 * Wait for particular state of CWMP session with specified CPE on ACSE.
 *
 * @param ctx           ACSE TAPI context.
 * @param want_state    State to wait.
 *
 * @return Status code; TE_ETIMEDOUT if timeout was expired and state
 *              was not occured.
 */
extern te_errno tapi_acse_wait_cwmp_state(tapi_acse_context_t *ctx,
                                          cwmp_sess_state_t want_state);

/**
 * Wait for particular state of ConnectionRequest to specified CPE on ACSE.
 *
 * @param ctx           ACSE TAPI context.
 * @param want_state    State to wait.
 *
 * @return Status code; TE_ETIMEDOUT if timeout was expired and state
 *              was not occured.
 */
extern te_errno tapi_acse_wait_cr_state(tapi_acse_context_t *ctx,
                                        acse_cr_state_t want_state);
/*
 * ================= CWMP processing ===================
 */


/**
 * Initiate CWMP session with CPE, that includes:
 * turn on sync_mode on ACSE for this CPE,
 * issue CWMP ConnectionRequest to particular CPE;
 * check its status, wait while CWMP session will be established.
 *
 * @se This method changes ACSE parameter 'sync_mode' to the TRUE.
 *
 * @param ctx           ACSE TAPI context.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_cpe_connect(tapi_acse_context_t *ctx);

/**
 * Issue CWMP ConnectionRequest to particular CPE.
 *
 * @param ctx           ACSE TAPI context.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_cpe_conn_request(tapi_acse_context_t *ctx);

/**
 * Finish CWMP session with particular CPE.
 * Really this util just initiates send of empty HTTP response
 * and turn off sync mode on the CPE, since this is designed as
 * dual for tapi_acse_cpe_connect().
 *
 * @param ctx           ACSE TAPI context.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_cpe_disconnect(tapi_acse_context_t *ctx);


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
 * @param ctx           ACSE TAPI context;
 * @param cpe_rpc_code  Code of RPC method;
 * @param to_cpe        CWMP RPC parameters.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_cpe_rpc_call(tapi_acse_context_t *ctx,
                                       te_cwmp_rpc_cpe_t cpe_rpc_code,
                                       cwmp_data_to_cpe_t to_cpe);

/**
 * Check status of queued CWMP RPC on ACSE and get response.
 *
 * @param ctx           ACSE TAPI context;
 * @param cpe_rpc_code  location for type of RPC response, may be NULL;
 * @param from_cpe      location for RPC response parameters.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_cpe_rpc_response(tapi_acse_context_t *ctx,
                                           te_cwmp_rpc_cpe_t *cpe_rpc_code,
                                           cwmp_data_from_cpe_t *from_cpe);


/**
 * Get particular received ACS RPC.
 * Return ENOENT if there was not such RPC caught from specified CPE.
 *
 * @param ctx           ACSE TAPI context.
 * @param rpc_acs       expected type of ACS RPC.
 * @param from_cpe      location for RPC response parameters.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_rpc_acs(tapi_acse_context_t *ctx,
                                      te_cwmp_rpc_acs_t rpc_acs,
                                      cwmp_data_from_cpe_t *from_cpe);




/*
 * ================= Particular CWMP RPC methods =======================
 */



/**
 * Call CPE GetRPCMethods method.
 *
 * @param ctx         current TAPI ACSE context
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_rpc_methods(tapi_acse_context_t *ctx);

/**
 * Get CPE GetRPCMethods response.
 * If there was CWMP Fault received as response, then TE_CWMP_FAULT
 * returned, and ptr to fault struct is stored in #resp place.
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     location for received list of supported RPC methods.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_rpc_methods_resp(tapi_acse_context_t *ctx,
                                               string_array_t **resp);
/**
 * Call CPE SetParameterValues method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param par_key  Parameter key for Set CWMP request;
 * @param req      Array of values for the SetParameterValues method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_set_parameter_values(tapi_acse_context_t *ctx,
                                               const char *par_key,
                                               cwmp_values_array_t *req);

/**
 * Get CPE SetParameterValues response.
 * If there was CWMP Fault received as response, then TE_CWMP_FAULT
 * returned, and ptr to fault struct is stored in #resp place.
 *
 * @param ctx      current TAPI ACSE context;
 * @param status   location for status of SetParameterValues, unchanged
 *                  if CWMP Fault was received.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_set_parameter_values_resp(
                                        tapi_acse_context_t *ctx,
                                        int *status);

/**
 * Call CPE SetParameterValues method and wait for response,
 * works good only in the sync mode during active CWMP session.
 * Response may contain more values then request, if some of names
 * is not leaf.
 *
 * @param ctx      current TAPI ACSE context;
 * @param par_key  Parameter key for Set CWMP request;
 * @param req      Array of values for the SetParameterValues method
 * @param status   location for status of SetParameterValues, unchanged
 *                  if CWMP Fault was received.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_set_pvalues_sync(tapi_acse_context_t *ctx,
                                           const char *par_key,
                                           cwmp_values_array_t *req,
                                           int *status);

/**
 * Call CPE GetParameterValues method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param names    Array of names which values are needed.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_parameter_values(tapi_acse_context_t *ctx,
                                               string_array_t *names);

/**
 * Get CPE GetParameterValues response
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     location for array of received values.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_parameter_values_resp(
                                    tapi_acse_context_t *ctx,
                                    cwmp_values_array_t **resp);

/**
 * Call CPE GetParameterValues method and wait for response,
 * works good only in the sync mode during active CWMP session.
 * Response may contain more values then request, if some of names
 * is not leaf.
 *
 * @param ctx      current TAPI ACSE context;
 * @param names    Array of names which values are needed.
 * @param resp     location for array of received values.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_pvalues_sync(tapi_acse_context_t *ctx,
                                           string_array_t *names,
                                           cwmp_values_array_t **resp);


/**
 * Call CPE GetParameterValues method and wait for response,
 * works good only in the sync mode during active CWMP session.
 * Userful routine for single parameter name.
 * Response may contain more then one value, if @p name is not leaf.
 *
 * @param ctx      current TAPI ACSE context;
 * @param name     names of wanted value.
 * @param resp     location for array of received values.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_pvalue_sync(tapi_acse_context_t *ctx,
                                          const char *name,
                                          cwmp_values_array_t **resp);

/**
 * Call CPE GetParameterNames method.
 *
 * @param ctx        current TAPI ACSE context;
 * @param next_level Boolean flag whether got only next level names,
 *                      see detailed description in TR-069 standard.
 * @param fmt        sprintf-like format string for parameter name.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_parameter_names(tapi_acse_context_t *ctx,
                                              te_bool next_level,
                                              const char *fmt, ...);

/**
 * Get CPE GetParameterNames response.
 * NB! This user-friendly method looses 'Writable' flag of variables
 * in response; to get full  GetParameterNamesResponse use generic
 * method: tapi_acse_cpe_rpc_call()
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     array with received parameter names.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_parameter_names_resp(tapi_acse_context_t *ctx,
                                                   string_array_t **resp);

/**
 * Call CPE GetParameterNames method and wait for response,
 * works good only in the sync mode during active CWMP session.
 *
 * @param ctx        current TAPI ACSE context;
 * @param next_level Boolean flag whether got only next level names,
 *                      see detailed description in TR-069 standard.
 * @param name       parameter name, should NOT contain any printf-like
 *                      format marks.
 * @param resp       array with received parameter names (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_pnames_sync(tapi_acse_context_t *ctx,
                                          te_bool next_level,
                                          const char *name,
                                          string_array_t **resp);

/**
 * Call CPE GetParameterAttributes method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param names    List of parameter names whose attributes are wanted.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_parameter_attributes(
                                tapi_acse_context_t *ctx,
                                string_array_t *names);

/**
 * Get CPE GetParameterAttributes response.
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     CPE response to the GetParameterAttributes method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_parameter_attributes_resp(
                tapi_acse_context_t *ctx,
                cwmp_get_parameter_attributes_response_t **resp);

/**
 * Call CPE AddObject method.
 *
 * @param ctx        ACSE TAPI context.
 * @param obj_name   name of object for AddObject method.
 * @param param_key  ParameterKey for AddObject method.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_add_object(tapi_acse_context_t *ctx,
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
 * @return Status code.
 */
extern te_errno tapi_acse_add_object_resp(tapi_acse_context_t *ctx,
                                          int *obj_index,
                                          int *add_status);

/**
 * Call CPE DeleteObject method.
 *
 * @param ctx        current TAPI ACSE context;
 * @param obj_name   name of object for DeleteObject method.
 * @param param_key  ParameterKey for DeleteObject method.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_delete_object(tapi_acse_context_t *ctx,
                                        const char *obj_name,
                                        const char *param_key);
/**
 * Get CPE DeleteObject response.
 *
 * @param ctx        current TAPI ACSE context;
 * @param del_status location for Status from DeleteObjectResponse.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_delete_object_resp(tapi_acse_context_t *ctx,
                                             int *del_status);

/**
 * Call on CPE CWMP Reboot method.
 *
 * @param ctx          current TAPI ACSE context;
 * @param command_key  string for CommandKey parameter of Reboot method.
 *
 * @return Status code.
 */
extern te_errno tapi_acse_reboot(tapi_acse_context_t *ctx,
                                 const char *command_key);

/**
 * Get CPE Reboot response, that is simply check status.
 *
 * @param ctx          current TAPI ACSE context;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_reboot_resp(tapi_acse_context_t *ctx);

/**
 * Call CPE Download method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param req      ACS request for the Download method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_download(tapi_acse_context_t *ctx,
                                   cwmp_download_t *req);

/**
 * Get CPE Download response.
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     CPE response to the Download method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_download_resp(tapi_acse_context_t *ctx,
                                        cwmp_download_response_t **resp);

/**
 * Call CPE Upload method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param req      ACS request for the Upload method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_upload(tapi_acse_context_t *ctx,
                                 cwmp_upload_t *req);
/**
 * Get CPE Upload response.
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     CPE response to the Upload method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_upload_resp(tapi_acse_context_t *ctx,
                                      cwmp_upload_response_t **resp);

/**
 * Call CPE FactoryReset method.
 *
 * @param ctx      current TAPI ACSE context;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_factory_reset(tapi_acse_context_t *ctx);

/**
 * Get CPE FactoryReset response.
 *
 * @param ctx      current TAPI ACSE context;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_factory_reset_resp(tapi_acse_context_t *ctx);

/**
 * Call CPE GetQueuedTransfers method.
 *
 * @param ctx      current TAPI ACSE context;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_queued_transfers(tapi_acse_context_t *ctx);
/**
 * Get CPE GetQueuedTransfers response.
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     CPE response to the GetQueuedTransfers method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_queued_transfers_resp(
                            tapi_acse_context_t *ctx,
                            cwmp_get_queued_transfers_response_t **resp);

/**
 * Call CPE GetAllQueuedTransfers method.
 *
 * @param ctx      current TAPI ACSE context;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_all_queued_transfers(
                tapi_acse_context_t *ctx);

/**
 * Get CPE GetAllQueuedTransfers response.
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     CPE response to the GetAllQueuedTransfers method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_all_queued_transfers_resp(
                tapi_acse_context_t *ctx,
                cwmp_get_all_queued_transfers_response_t **resp);

/**
 * Call CPE ScheduleInform method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param req      ACS request for the ScheduleInform method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_schedule_inform(tapi_acse_context_t *ctx,
                                          cwmp_schedule_inform_t *req);

/**
 * Get CPE ScheduleInform response.
 *
 * @param ctx      current TAPI ACSE context;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_schedule_inform_resp(tapi_acse_context_t *ctx);

/**
 * Call CPE SetVouchers method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param req      ACS request for the SetVouchers method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_set_vouchers(tapi_acse_context_t *ctx,
                                       cwmp_set_vouchers_t *req);
/**
 * Get CPE SetVouchers response.
 *
 * @param ctx      current TAPI ACSE context;
 *
 * @return Status code.
 */
extern te_errno tapi_acse_set_vouchers_resp(tapi_acse_context_t *ctx);

/**
 * Call CPE GetOptions method.
 *
 * @param ctx      current TAPI ACSE context;
 * @param req      ACS request for the GetOptions method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_options(tapi_acse_context_t *ctx,
                                      cwmp_get_options_t *req);
/**
 * Get CPE GetOptions response.
 *
 * @param ctx      current TAPI ACSE context;
 * @param resp     CPE response to the GetOptions method
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_options_resp(tapi_acse_context_t *ctx,
               cwmp_get_options_response_t **resp);




/**
 * Get ACS full URL for establish CWMP session.
 *
 * @param ctx     ACSE TAPI context.
 * @param addr    Network address of ACS, when it will accept connection.
 * @param str     location for URL
 * @param buflen  size of buffer for URL
 *
 * @return Status code.
 */
extern te_errno tapi_acse_get_full_url(tapi_acse_context_t *ctx,
                                       struct sockaddr *addr,
                                       char *str, size_t buflen);




#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __TE_TAPI_ACSE_H__ */
