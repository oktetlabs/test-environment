/** @file
 * @brief RPC client API for Agent job control
 *
 * RPC client API for Agent job control functions
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_JOB_H__
#define __TE_TAPI_RPC_JOB_H__

#include "rcf_rpc.h"

#include "tarpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a job controlled by the RPC server @p rcps.
 * The job will be managed by @p spawner plugin.
 *
 * @param rpcs          RPC server
 * @param spawner       Spawner plugin name
 *                      (may be @c NULL for the default plugin)
 * @param tool          Program path to run
 * @param argv          Program arguments (last item is @c NULL)
 * @param env           Program environment (last item is @c NULL).
 *                      May be @c NULL to keep the current environment.
 * @param[out] job_id   Job handle
 *
 * @return              Status code
 */
extern int rpc_job_create(rcf_rpc_server *rpcs, const char *spawner,
                          const char *tool, const char **argv, const char **env,
                          unsigned int *job_id);

/**
 * Start a job
 *
 * @param rpcs          RPC server
 * @param job_id        Job instance handle
 */
extern int rpc_job_start(rcf_rpc_server *rpcs, unsigned int job_id);

/**
 * Send a signal to the job
 *
 * @param rpcs          RPC server
 * @param job_id        Job instance handle
 * @param signo         Signal number
 *
 * @return              Status code
 */
extern int rpc_job_kill(rcf_rpc_server *rpcs, unsigned int job_id,
                        rpc_signum signo);

/**
 * Wait for the job completion (or check its status if @p timeout is zero)
 *
 * @param rpcs          RPC server
 * @param job_id        Job instance handle
 * @param timeout_ms    Timeout in ms (negative means infinity)
 * @param[out] status   Exit status
 *
 * @return          Status code
 */
extern int rpc_job_wait(rcf_rpc_server *rpcs, unsigned int job_id,
                        int timeout_ms, tarpc_job_status *status);

/**
 * Destroy the job instance. If the job has started, it is terminated
 * as gracefully as possible. All resources of the instance are freed;
 * all unread data on all filters are lost.
 *
 * @param rpcs          RPC server
 * @param job_id        Job instance handle
 *
 * @return              Status code
 */
extern int rpc_job_destroy(rcf_rpc_server *rpcs, unsigned int job_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_RPC_JOB_H__ */
