/** @file
 * @brief Auxiliary functions for internal use in TAPI Job
 *
 * @defgroup tapi_job_internal TAPI Job internal functions (tapi_job_internal)
 * @ingroup tapi_job
 * @{
 *
 * Auxiliary functions for internal use in TAPI Job
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TAPI_JOB_INTERNAL_H__
#define __TAPI_JOB_INTERNAL_H__

#include "rcf_rpc.h"
#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get RPC server on which @p job was created.
 * The job must have been created using RPC job factory, otherwise
 * the function calls TEST_FAIL().
 *
 * The function is primarily intended for internal usage.
 *
 * @param job       Job instance handle
 *
 * @return          RPC server on which @p job was created
 * @exception       TEST_FAIL
 */
extern rcf_rpc_server *tapi_job_get_rpcs(const tapi_job_t *job);

/**
 * Get internal ID of the @p job.
 * The job must have been created using RPC job factory, otherwise
 * the function calls TEST_FAIL().
 *
 * The function is primarily intended for internal usage.
 *
 * @param job       Job instance handle
 *
 * @return          Internal ID of the @p job
 * @exception       TEST_FAIL
 */
extern unsigned int tapi_job_get_id(const tapi_job_t *job);

/**
 * Set internal ID of the @p job.
 * The job must have been created using RPC job factory, otherwise
 * the function calls TEST_FAIL().
 *
 * The function is primarily intended for internal usage.
 *
 * @param job       Job instance handle
 * @param id        ID to set to the @p job
 *
 * @exception       TEST_FAIL
 */
extern void tapi_job_set_id(tapi_job_t *job, unsigned int id);

/**
 * Get name of the Test Agent on which @p job was created.
 * The job must have been created using CFG job factory, otherwise
 * the function calls TEST_FAIL().
 *
 * The function is primarily intended for internal usage.
 *
 * @param job       Job instance handle
 *
 * @return          Name of the Test Agent on which @p job was created
 * @exception       TEST_FAIL
 */
extern const char *tapi_job_get_ta(const tapi_job_t *job);

/**
 * Get name of the @p job.
 * The job must have been created using CFG job factory, otherwise
 * the function calls TEST_FAIL().
 *
 * The function is primarily intended for internal usage.
 *
 * @param job       Job instance handle
 *
 * @return          Name of the @p job
 * @exception       TEST_FAIL
 */
extern const char *tapi_job_get_name(const tapi_job_t *job);

/**
 * Set name of the @p job.
 * The corresponding member of tapi_job_t should be freed when the job is not
 * needed anymore.
 * The job must have been created using CFG job factory, otherwise
 * the function calls TEST_FAIL().
 *
 * The function is primarily intended for internal usage.
 *
 * @param job       Job instance handle
 * @param name      Name to set to the @p job
 *
 * @exception       TEST_FAIL
 */
extern void tapi_job_set_name(tapi_job_t *job, const char *name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_JOB_INTERNAL_H__ */

/** @{ <!-- END tapi_job_internal --> */
