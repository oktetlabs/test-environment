/** @file
 * @brief Configurator API for Agent job control
 *
 * @defgroup tapi_cfg_job Configurator API for Agent job control (tapi_cfg_job)
 * @ingroup tapi_job
 * @{
 *
 * Configurator API for Agent job control.
 * Since the API is based on "/agent/process" Configurator subtree,
 * terms "job" and "process" are used interchangeably in this file.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Dilshod Urazov <Dilshod.Urazov@oktetlabs.ru>
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_JOB_H__
#define __TE_TAPI_CFG_JOB_H__

#include "te_defs.h"
#include "te_errno.h"

#include "tapi_job.h"
#include "tapi_job_methods.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Cause of process termination */
typedef enum cfg_job_exit_status_type_t {
    /** Process terminated normally (by exit() or return from main) */
    CFG_JOB_EXIT_STATUS_EXITED,
    /** Process was terminated by a signal */
    CFG_JOB_EXIT_STATUS_SIGNALED,
    /** The cause of process termination is not known */
    CFG_JOB_EXIT_STATUS_UNKNOWN,
} cfg_job_exit_status_type_t;

/** Structure that represents status of a terminated process */
typedef struct cfg_job_exit_status_t {
    /** Cause of process termination */
    cfg_job_exit_status_type_t type;
    /**
     * Either an exit status of the process or a number of a signal
     * which caused the process termination
     */
    int value;
} cfg_job_exit_status_t;

/** Methods for job created by CFG factory */
extern const tapi_job_methods_t cfg_job_methods;

/**
 * Create a job
 *
 * @sa tapi_job_method_create
 */
extern tapi_job_method_create cfg_job_create;

/**
 * Start a job
 *
 * @sa tapi_job_method_start
 */
extern tapi_job_method_start cfg_job_start;

/**
 * Send a signal to the job
 *
 * @sa tapi_job_method_kill
 */
extern tapi_job_method_kill cfg_job_kill;

/**
 * Send a signal to the job's proccess group
 *
 * @sa tapi_job_method_killpg
 */
extern tapi_job_method_killpg cfg_job_killpg;

/**
 * Wait for a job completion
 *
 * @note Negative @p timeout_ms blocks the test execution until the job changes
 *       its status.
 *       Zero return value may also mean that the job was never started.
 *
 * @sa tapi_job_method_wait
 */
extern tapi_job_method_wait cfg_job_wait;

/**
 * Stop a job
 *
 * @note Parameters @p signo and @p term_timeout_ms are ignored, use @c -1 to
 *       avoid warnings
 *
 * @sa tapi_job_method_stop
 */
extern tapi_job_method_stop cfg_job_stop;

/**
 * Destroy a job
 *
 * @note Parameter @p term_timeout_ms is ignored, use @c -1 to avoid warning
 *
 * @sa tapi_job_method_destroy
 */
extern tapi_job_method_destroy cfg_job_destroy;

/**
 * Set autorestart timeout for a job
 *
 * @note This function should be called before the job is started
 *
 * @sa tapi_job_method_set_autorestart
 */
extern tapi_job_method_set_autorestart cfg_job_set_autorestart;

/**
 * Get autorestart timeout of a job
 *
 * @sa tapi_job_method_get_autorestart
 */
extern tapi_job_method_get_autorestart cfg_job_get_autorestart;

/**
 * Retrieve information about a job from Configurator
 *
 * @note At this moment the function only checks that a process corresponding to
 *       the job exists, but later it might be extended to extract additional
 *       job data
 *
 * @sa tapi_job_method_recreate
 */
extern tapi_job_method_recreate cfg_job_recreate;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_JOB_H__ */

/**@} <!-- END tapi_cfg_job --> */
