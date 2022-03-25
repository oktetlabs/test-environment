/** @file
 * @brief Functions that TAPI Job backend might implement
 *
 * @defgroup tapi_job_methods TAPI Job Methods (tapi_job_methods)
 * @ingroup tapi_job
 * @{
 *
 * Functions that TAPI Job backend might implement.
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TAPI_JOB_METHODS__
#define __TAPI_JOB_METHODS__

#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Method that creates a job on backend side
 *
 * @param[inout] job               Job instance handle.
 *                                 On input, job factory must be set.
 *                                 On output, backend specific data for the job
 *                                 will be set.
 * @param[in]    spawner           Spawner plugin name
 *                                 (may be @c NULL for the default plugin)
 * @param[in]    tool              Program path to run
 * @param[in]    argv              Program arguments (last item is @c NULL)
 * @param[in]    env               Program environment (last item is @c NULL).
 *                                 May be @c NULL to keep the current
 *                                 environment.
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_create)(tapi_job_t *job,
                                           const char *spawner,
                                           const char *program,
                                           const char **argv,
                                           const char **env);

/**
 * Method that starts a job
 *
 * @param        job               Job instance handle
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_start)(const tapi_job_t *job);

/**
 * Method that allocates @p n_channels channels.
 * If the @p input_channels is @c TRUE,
 * the first channel is expected to be connected to the job's stdin;
 * If the @p input_channels is @c FALSE,
 * The first and the second output channels are expected to be
 * connected to stdout and stderr respectively;
 * The wiring of not mentioned channels is spawner-dependant.
 *
 * @param[in]    job               Job instance handle
 * @param[in]    input_channels    @c TRUE to allocate input channels,
 *                                 @c FALSE to allocate output channels
 * @param[in]    n_channels        Number of channels
 * @param[out]   channels          A vector of obtained channel handles
 *                                 (may be @c NULL if the caller is not
 *                                 interested in the handles)
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_allocate_channels)(const tapi_job_t *job,
                                                      te_bool input_channels,
                                                      unsigned int n_channels,
                                                      unsigned int *channels);

/**
 * Method that sends a signal to a job
 *
 * @param        job               Job instance handle
 * @param        signo             Signal number
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_kill)(const tapi_job_t *job, int signo);

/**
 * Method that sends a signal to job's process group
 *
 * @param        job               Job instance handle
 * @param        signo             Signal number
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_killpg)(const tapi_job_t *job, int signo);

/**
 * Method that waits for the job completion (or checks its status if @p timeout
 * is zero)
 *
 * @param[in]    job               Job instance handle
 * @param[in]    timeout_ms        Timeout in ms (negative means default
 *                                 timeout)
 * @param[out]   status            Exit status
 *
 * @return       Status code
 * @retval       TE_EINPROGRESS    Job is still running
 * @retval       TE_ECHILD         Job was never started
 */
typedef te_errno (*tapi_job_method_wait)(const tapi_job_t *job, int timeout_ms,
                                         tapi_job_status_t *status);

/**
 * Method that stops a job.
 * The function tries to terminate the job with the specified signal.
 * If the signal fails to terminate the job, the function will send @c SIGKILL.
 *
 * @param        job               Job instance handle
 * @param        signo             Signal to be sent at first. If signo is
 *                                 @c SIGKILL, it will be sent only once.
 * @param        term_timeout_ms   The timeout of graceful termination of a job,
 *                                 if it has been running. After the timeout
 *                                 expiration the job will be killed with
 *                                 @c SIGKILL (negative means default timeout).
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_stop)(const tapi_job_t *job, int signo,
                                         int term_timeout_ms);

/**
 * Method that destroys a job on backend side.
 * If the job has started, it is terminated as gracefully as possible.
 * All resources of the instance are freed; all unread data on all filters are
 * lost.
 *
 * @param        job               Job instance handle
 * @param        term_timeout_ms   The timeout of graceful termination of a job,
 *                                 if it has been running. After the timeout
 *                                 expiration the job will be killed with
 *                                 @c SIGKILL (negative means default timeout).
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_destroy)(const tapi_job_t *job,
                                            int term_timeout_ms);

/**
 * Method that adds a wrapper for the specified job
 *
 * @param[in]    job               Job instance handle
 * @param[in]    tool              Path to the wrapper tool
 * @param[in]    argv              Wrapper arguments (last item is @c NULL)
 * @param[in]    priority          Wrapper priority
 * @param[out]   wrapper_id        Wrapper handle
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_wrapper_add)(const tapi_job_t *job,
                                        const char *tool, const char **argv,
                                        tapi_job_wrapper_priority_t priority,
                                        unsigned int *wrapper_id);

/**
 * Method that deletes the wrapper instance handle
 *
 * @param        job               Job instance handle
 * @param        wrapper_id        Wrapper instance handle
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_wrapper_delete)(const tapi_job_t *job,
                                                   unsigned int wrapper_id);

/**
 * Method that adds a scheduling parameters for the specified job
 *
 * @param        job               Job instance handle
 * @param        sched_param       Array of scheduling parameters. The last
 *                                 element must have the type
 *                                 @c TAPI_JOB_SCHED_END and data pointer to
 *                                 @c NULL.
 *
 * @return       Status code
 */
typedef te_errno (*tapi_job_method_add_sched_param)(const tapi_job_t *job,
                                             tapi_job_sched_param *sched_param);

/** Methods to operate TAPI Job instances */
typedef struct tapi_job_methods_t {
    /** Method that creates a job on backend side */
    tapi_job_method_create create;
    /** Method that starts a job */
    tapi_job_method_start start;
    /** Method that allocates channels */
    tapi_job_method_allocate_channels allocate_channels;
    /** Method that sends a signal to a job */
    tapi_job_method_kill kill;
    /** Method that sends a signal to job's process group */
    tapi_job_method_killpg killpg;
    /** Method that waits for the job completion */
    tapi_job_method_wait wait;
    /** Method that stops a job */
    tapi_job_method_stop stop;
    /** Method that destroys a job on backend side */
    tapi_job_method_destroy destroy;
    /** Method that adds a wrapper for the specified job */
    tapi_job_method_wrapper_add wrapper_add;
    /** Method that deletes the wrapper instance handle */
    tapi_job_method_wrapper_delete wrapper_delete;
    /** Method that adds a scheduling parameters for the specified job */
    tapi_job_method_add_sched_param add_sched_param;
} tapi_job_methods_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_JOB_METHODS */

/** @} <!-- END tapi_job_methods --> */
