#ifndef __TAPI_JOB_FACTORY_RPC_H__
#define __TAPI_JOB_FACTORY_RPC_H__

#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tapi_job_factory_t;

/**
 * Create a job factory by the RPC server @p rcps.
 *
 * @param rpcs          RPC server
 * @param[out] factory  Job factory handle
 *
 * @return              Status code
 */
extern te_errno tapi_job_factory_rpc_create(rcf_rpc_server *rpcs,
                                            struct tapi_job_factory_t **factory);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_JOB_FACTORY_RPC_H__ */
