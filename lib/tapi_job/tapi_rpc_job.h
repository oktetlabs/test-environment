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
#include "tapi_job.h"

#include "tarpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_RPC_JOB_BIG_TIMEOUT_MS 600000

/**
 * Create a job controlled by the RPC server @p rpcs.
 * The job will be managed by @p spawner plugin.
 *
 * @param[inout] job    Job instance handle.
 *                      On input, job factory must be set.
 *                      On output, job ID will be set.
 * @param[in] spawner   Spawner plugin name
 *                      (may be @c NULL for the default plugin)
 * @param[in] tool      Program path to run
 * @param[in] argv      Program arguments (last item is @c NULL)
 * @param[in] env       Program environment (last item is @c NULL).
 *                      May be @c NULL to keep the current environment.
 *
 * @return              Status code
 */
extern te_errno rpc_job_create(tapi_job_t *job, const char *spawner,
                               const char *tool, const char **argv,
                               const char **env);

/**
 * Start a job
 *
 * @param job           Job instance handle
 *
 * @return              Status code
 */
extern te_errno rpc_job_start(const tapi_job_t *job);

/**
 * Allocate @p n_channels channels.
 * If the @p input_channels is @c TRUE,
 * the first channel is expected to be connected to the job's stdin;
 * If the @p input_channels is @c FALSE,
 * The first and the second output channels are expected to be
 * connected to stdout and stderr respectively;
 * The wiring of not mentioned channels is spawner-dependant.
 *
 * @param job             Job instance handle
 * @param n_channels      Number of channels
 * @param[out] channels   A vector of obtained channel handles
 *                        (may be @c NULL if the caller is not interested
 *                        in the handles)
 *
 * @return          Status code
 */
extern te_errno rpc_job_allocate_channels(const tapi_job_t *job,
                                          te_bool input_channels,
                                          unsigned int n_channels,
                                          unsigned int *channels);

/**
 * Deallocate @p n_channels channels.
 *
 * @param rpcs            RPC server
 * @param n_channels      Number of channels to deallocate
 * @param channels        Channels to deallocate
 *
 * @return                Status code
 */
extern te_errno rpc_job_deallocate_channels(rcf_rpc_server *rpcs,
                                            unsigned int n_channels,
                                            unsigned int *channels);

/**
 * Create a secondary output channel applying a filter to an existing
 * channel.
 *
 * @note Filters can be attached only to primary channels
 *
 * @param rpcs          RPC server
 * @param filter_name   Name of the filter
 * @param n_channels    Count of @p channels
 * @param channels      Output channels to attach the filter to.
 * @param readable      If @c TRUE, the output of the filter can be
 *                      read with rpc_job_receive(); otherwise, it is discarded,
 *                      possibly after being logged.
 * @param log_level     If non-zero, the output of the filter is
 *                      logged with a given log level
 * @param[out] filter   Filter channel (may be @c NULL for trivial filters)
 *
 * @return              Status code
 * @retval TE_EPERM     Some of @p channels are input channels
 * @retval TE_EINVAL    Some of @p channels are output filters
 */
extern te_errno rpc_job_attach_filter(rcf_rpc_server *rpcs,
                                      const char *filter_name,
                                      unsigned int n_channels,
                                      unsigned int *channels,
                                      te_bool readable,
                                      te_log_level log_level,
                                      unsigned int *filter);

/**
 * Add a regular expression for filter
 *
 * @param rpcs     RPC server
 * @param filter   Filter handle
 * @param re       PCRE-style regular expression to match
 * @param extract  A substring to extract as an output of the filter
 *                 (0 meaning the whole match)
 *
 * @return          Status code
 */
extern te_errno rpc_job_filter_add_regexp(rcf_rpc_server *rpcs,
                                          unsigned int filter, const char *re,
                                          unsigned int extract);

/**
 * Attach an existing filter to additional output channels
 *
 * @param rpcs        RPC server
 * @param filter      Filter to attach
 * @param n_channels  Count of @p channels
 * @param channels    Output channels to attach the filter to
 *
 * @return            Status code
 */
extern te_errno rpc_job_filter_add_channels(rcf_rpc_server *rpcs,
                                            unsigned int filter,
                                            unsigned int n_channels,
                                            unsigned int *channels);

/**
 * Remove filter from specified output channels
 *
 * @param rpcs        RPC server
 * @param filter      Filter to remove
 * @param n_channels  Count of @p channels
 * @param channels    Output channels to remove the filter from
 *
 * @return            Status code
 */
extern te_errno rpc_job_filter_remove_channels(rcf_rpc_server *rpcs,
                                               unsigned int filter,
                                               unsigned int n_channels,
                                               unsigned int *channels);

/**
 * Read the next message from one of the available filters.
 *
 * @param rpcs        RPC server
 * @param n_filters   Count of @p filters
 * @param filters     Set of filters to read from.
 * @param timeout_ms  Timeout to wait (negative means
 *                    #TAPI_RPC_JOB_BIG_TIMEOUT_MS)
 * @param buffer      Data buffer pointer. If @c NULL, the message is
 *                    silently discarded.
 *
 * @return            Status code
 * @retval TE_ETIMEDOUT     if there's no data available within @p timeout
 * @retval TE_EPERM   if some of the @p filters are input channels or
 *                    primary output channels
 * @retval TE_EXDEV   if @p filters are on different RPC servers
 */
extern te_errno rpc_job_receive(rcf_rpc_server *rpcs, unsigned int n_filters,
                                unsigned int *filters, int timeout_ms,
                                tarpc_job_buffer *buffer);
/**
 * Read the last non-eos message from one of the available filters.
 * The message is not removed from the queue, it can still be read with
 * rpc_job_receive().
 *
 * @param rpcs        RPC server
 * @param n_filters   Count of @p filters
 * @param filters     Set of filters to read from.
 * @param timeout_ms  Timeout to wait (negative means
 *                    #TAPI_RPC_JOB_BIG_TIMEOUT_MS)
 * @param buffer      Data buffer pointer. If @c NULL, the message is
 *                    silently discarded.
 *
 * @return            Status code
 * @retval TE_ETIMEDOUT     if there's no data available within @p timeout
 * @retval TE_EPERM   if some of the @p filters are input channels or
 *                    primary output channels
 * @retval TE_EXDEV   if @p filters are on different RPC servers
 */
extern te_errno rpc_job_receive_last(rcf_rpc_server *rpcs,
                                     unsigned int n_filters,
                                     unsigned int *filters,
                                     int timeout_ms,
                                     tarpc_job_buffer *buffer);

/**
 * Read multiple messages at once from the specified filters.
 * This function may retrieve some messages and return error if attempt to
 * read the next message failed.
 *
 * @param rpcs        RPC server
 * @param n_filters   Count of @p filters
 * @param filters     Set of filters to read from
 * @param timeout_ms  Timeout to wait (negative means
 *                    #TAPI_RPC_JOB_BIG_TIMEOUT_MS)
 * @param buffers     Where to save pointer to array of message buffers.
 *                    It should be released by the caller with
 *                    tarpc_job_buffers_free()
 * @param count       On input, maximum number of messages to retrieve.
 *                    If zero, all available messages will be retrieved.
 *                    On output - number of actually retrieved messages
 *
 * @return Status code.
 */
extern te_errno rpc_job_receive_many(rcf_rpc_server *rpcs,
                                     unsigned int n_filters,
                                     unsigned int *filters, int timeout_ms,
                                     tarpc_job_buffer **buffers,
                                     unsigned int *count);

/**
 * Release array of message buffers.
 *
 * @param buffers     Pointer to the array
 * @param count       Number of buffers in the array
 */
extern void tarpc_job_buffers_free(tarpc_job_buffer *buffers,
                                   unsigned int count);

/**
 * Remove all pending messages from filters, they are lost completely.
 *
 * @param rpcs        RPC server
 * @param n_filters   Count of @p filters
 * @param filters     Set of filters to clear.
 *
 * @return            Status code
 * @retval TE_EPERM   if some of the @p filters are input channels or
 *                    primary output channels
 */
extern te_errno rpc_job_clear(rcf_rpc_server *rpcs, unsigned int n_filters,
                              unsigned int *filters);

/**
 * Send data to a job input channel.
 *
 * @param rpcs     RPC server
 * @param channel  Input channel handle
 * @param buf      A pointer to data buffer
 * @param count    Size of data buffer
 *
 * @return          Status code
 * @retval TE_EPERM if @p channel is not an input channel
 */
extern te_errno rpc_job_send(rcf_rpc_server *rpcs, unsigned int channel,
                             const void *buf, size_t count);

/**
 * Poll the job's channels/filters for readiness.
 *
 * @param rpcs          RPC server
 * @param n_channels    Count of @p channels
 * @param channels      Set of channels to wait
 * @param timeout_ms    Timeout in ms to wait (negative means
 *                      #TAPI_RPC_JOB_BIG_TIMEOUT_MS)
 *
 * @return              Status code
 * @retval TE_EPERM     if some channels from @p wait_set are neither input
 *                      channels nor pollable output channels
 */
extern te_errno rpc_job_poll(rcf_rpc_server *rpcs, unsigned int n_channels,
                             unsigned int *channels, int timeout_ms);

/**
 * Send a signal to the job
 *
 * @param job           Job instance handle
 * @param signo         Signal number
 *
 * @return              Status code
 */
extern te_errno rpc_job_kill(const tapi_job_t *job, int signo);

/**
 * Send a signal to the proccess group
 *
 * @param job           Job instance handle
 * @param signo         Signal number
 *
 * @return              Status code
 */
extern te_errno rpc_job_killpg(const tapi_job_t *job, int signo);

/**
 * Wait for the job completion (or check its status if @p timeout is zero)
 *
 * @param job           Job instance handle
 * @param timeout_ms    Timeout in ms (negative means
 *                      #TAPI_RPC_JOB_BIG_TIMEOUT_MS)
 * @param[out] status   Exit status
 *
 * @return          Status code
 * @retval TE_EINPROGRESS   Job is still running
 * @retval TE_ECHILD        Job was never started
 */
extern te_errno rpc_job_wait(const tapi_job_t *job, int timeout_ms,
                             tapi_job_status_t *status);

/**
 * Stop a job. It can be started over with tapi_job_start().
 * The function tries to terminate the job with the specified signal.
 * If the signal fails to terminate the job, the function will send @c SIGKILL.
 *
 * @param job               Job instance handle
 * @param signo             Signal to be sent at first. If signo is @c SIGKILL,
 *                          it will be sent only once.
 * @param term_timeout_ms   The timeout of graceful termination of a job,
 *                          if it has been running. After the timeout expiration
 *                          the job will be killed with @c SIGKILL.
 *                          (negative means default timeout)
 *
 * @return              Status code
 */
extern te_errno rpc_job_stop(const tapi_job_t *job, int signo,
                             int term_timeout_ms);

/**
 * Destroy the job instance. If the job has started, it is terminated
 * as gracefully as possible. All resources of the instance are freed;
 * all unread data on all filters are lost.
 *
 * @param job               Job instance handle
 * @param term_timeout_ms   The timeout of graceful termination of a job,
 *                          if it has been running. After the timeout expiration
 *                          the job will be killed with SIGKILL. (negative means
 *                          default timeout)
 *
 * @return                  Status code
 */
extern te_errno rpc_job_destroy(const tapi_job_t *job, int term_timeout_ms);

/**
 * Add a wrapper for the specified job.
 *
 * @param[in] job         Job instance handle
 * @param[in] tool        Path to the wrapper tool
 * @param[in] argv        Wrapper arguments (last item is @c NULL)
 * @param[in] priority    Wrapper priority
 * @param[out] wrapper_id Wrapper handle
 *
 * @return                Status code
 */
extern te_errno rpc_job_wrapper_add(const tapi_job_t *job,
                                    const char *tool, const char **argv,
                                    tapi_job_wrapper_priority_t priority,
                                    unsigned int *wrapper_id);

/**
 * Delete the wrapper instance handle.
 *
 * @param job               Job instance handle
 * @param wrapper_id        Wrapper instance handle
 *
 * @return                  Status code
 */
extern te_errno rpc_job_wrapper_delete(const tapi_job_t *job,
                                       unsigned int wrapper_id);

/**
 * Add a scheduling parameters for the specified job.
 *
 * @param job             Job instance handle
 * @param sched_param     Array of scheduling parameters.
 * @param sched_param_len Size of array of scheduling parameters.
 *
 * @return Status code
 */
extern te_errno rpc_job_add_sched_param(const tapi_job_t *job,
                                        tapi_job_sched_param *sched_param);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_RPC_JOB_H__ */
