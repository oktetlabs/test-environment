/** @file
 * @brief ACSE
 *
 * ACSE user interface declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_LIB_ACSE_USER_H__
#define __TE_LIB_ACSE_USER_H__
#include "te_config.h"

#include "te_cwmp.h"
#include "cwmp_data.h"
#include "acse_epc.h"
/**
 * Main ACSE loop: waiting events from all channels, processing them.
 */
extern void acse_loop(void);

/**
 * Init EPC dispatcher.
 *
 * @param listen_sock           socket for listening EPC connection
 *
 * @return              status code
 */
extern te_errno acse_epc_disp_init(int listen_sock, epc_site_t *s);


/**
 * Init EPC CWMP-operations user.
 */
extern te_errno acse_epc_user_init(epc_site_t *s);

/**
 * Perform EPC configuration method and wait for result,
 * which should come ASAP.
 * Note: here is hardcoded timeout to wait response.
 * TODO: think, maybe, make this timeout configurable?
 *
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 * @param oid           Object identifier
 * @param value         The value of the instance to get (OUT)
 * @param fun           Code of ACSE config operation
 * @param cfg_result    Location for pointer to result of config operation,
 *                      it will points to the static memory area,
 *                      and should NOT be freed.
 *
 * @return              Local execution status code
 */
extern te_errno acse_conf_op(const char *acs, const char *cpe,
                             const char *oid,
                             const char *value, acse_cfg_op_t fun,
                             acse_epc_config_data_t **cfg_result);
/**
 * Prepare internal data for send EPC config operation ACSE.
 * This function MUST be called before acse_conf_call().
 *
 * @param fun           Code of ACSE config operation
 * @param cfg_data      Location for pointer to operation-specific data,
 *                      to be filled by user specifically.
 *                      It will points to the static memory area,
 *                      and should NOT never be freed.
 *                      May be NULL.
 *
 * @return              Local execution status code
 */
extern te_errno acse_conf_prepare(acse_cfg_op_t fun,
                                  acse_epc_config_data_t **cfg_data);

/**
 * Prepare internal data for send EPC config operation ACSE.
 * This function MUST be called before acse_conf_call().
 *
 * @param cfg_result    Location for pointer to result of config operation,
 *                      it will points to the static memory area,
 *                      and should NOT be freed.
 *
 * @return              Local execution status code
 */
extern te_errno acse_conf_call(acse_epc_config_data_t **cfg_result);

/**
 * Prepare internal data for send CWMP operation request to ACSE.
 * This function MUST be called before acse_cwmp_call().
 *
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 * @param fun           Code of ACSE config operation
 * @param cwmp_data     Location for pointer to operation-specific data,
 *                      to be filled by user specifically.
 *                      It will points to the static memory area,
 *                      and should NOT never be freed.
 *                      May be NULL.
 *
 * @return              Local execution status code
 */
extern te_errno acse_cwmp_prepare(const char *acs, const char *cpe,
                                  acse_epc_cwmp_op_t fun,
                                  acse_epc_cwmp_data_t **cwmp_data);

/**
 * Call CWMP operation request to ACSE, and wait for result.
 * This function MUST be called after acse_cwmp_prepare(), and
 * if necessary, subsequent additions into cwmp_data.
 *
 * @param status        Location for result status of operation.
 * @param data_len      Location for length of received response.
 * @param cwmp_data     Location for pointer to operation-specific data,
 *                      with result of operation, or NULL.
 *                      It will points to the static memory area,
 *                      and should NOT never be freed.
 *
 * @return              Local execution status code
 */
extern te_errno acse_cwmp_call(size_t *data_len,
                               acse_epc_cwmp_data_t **cwmp_data);

/**
 * Call CWMP ConnectionRequest
 *
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              status code
 */
extern te_errno acse_cwmp_connreq(const char *acs, const char *cpe,
                                  acse_epc_cwmp_data_t **cwmp_data);

/**
 * Call CWMP CPE RPC operation.
 *
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 * @param request_id    Location for request id.
 * @param rpc_cpe       Code of RPC
 * @param to_cpe        CWMP function-specific data for CPE.
 *
 * @return              status code
 */
extern te_errno acse_cwmp_rpc_call(const char *acs, const char *cpe,
                                   acse_request_id_t *request_id,
                                   te_cwmp_rpc_cpe_t  rpc_cpe,
                                   cwmp_data_to_cpe_t to_cpe);

/**
 * Check CWMP CPE RPC operation status or get ACS RPC.
 *
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              status code
 */
extern te_errno acse_cwmp_rpc_check(const char *acs, const char *cpe,
                                    acse_request_id_t  request_id,
                                    te_cwmp_rpc_cpe_t  rpc_cpe,
                                    te_cwmp_rpc_acs_t  rpc_acs,
                                    cwmp_data_from_cpe_t from_cpe);

/**
 * Order send particular HTTP message, and wait EPC answer
 * from ACSE.
 * While active CWMP session, issue specified HTTP response
 * immediately. If there is no active CWMP session,
 * specified HTTP response will be answer to the next Inform,
 * received from that CPE.
 *
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 * @param http_code     HTTP code.
 * @param location      Direction for redirect codes, or NULL.
 *
 * @return              Local execution status code
 */
extern te_errno acse_http_code(const char *acs, const char *cpe,
                               int http_code, const char *location);

/**
 */
extern te_errno acse_send_set_parameter_values(int *request_id,
                                               ...);

#endif /* __TE_LIB_ACSE_USER_H__ */
