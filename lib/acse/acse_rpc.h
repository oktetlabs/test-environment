/** @file
 * @brief ACSE RPC functions
 *
 * Fuctions to process ACSE RPC calls
 *
 *
 */

#ifndef __TE_ACSE_RPC_H__
#define __TE_ACSE_RPC_H__

#include "tarpc.h"

/**
 * RPC methods to control ACS emulator
 */

/**
 * Send ACSE command to start CWMP operation.
 *
 * @return              Status
 */
extern int cwmp_op_call(tarpc_cwmp_op_call_in *in,
                        tarpc_cwmp_op_call_out *out);

/**
 * Send ACSE command to check status of CWMP operation.
 *
 * @return              Status
 */
extern int cwmp_op_check(tarpc_cwmp_op_check_in *in,
                         tarpc_cwmp_op_check_out *out);

/**
 * Send ACSE command to connect CPE.
 *
 * @return              Status
 */
extern int cwmp_conn_req(tarpc_cwmp_conn_req_in *in,
                         tarpc_cwmp_conn_req_out *out);

/**
 * Initialize and start ACSE (ACS emulator) thread on tester agent.
 *
 * @return              Status
 */
extern int cwmp_acse_start(tarpc_cwmp_acse_start_in *in,
                           tarpc_cwmp_acse_start_out *out);

#endif /* __TE_ACSE_RPC_H__ */
