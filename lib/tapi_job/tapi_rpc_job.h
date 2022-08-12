/** @file
 * @brief RPC client API for Agent job control
 *
 * RPC client API for Agent job control functions
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_JOB_H__
#define __TE_TAPI_RPC_JOB_H__

#include "rcf_rpc.h"
#include "tapi_job.h"
#include "tapi_job_methods.h"

#include "tarpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_RPC_JOB_BIG_TIMEOUT_MS 600000

/** Methods for jobs created by RPC factory */
extern const tapi_job_methods_t rpc_job_methods;

/**
 * Create a job.
 *
 * @note Job ID is set on successful function call
 *
 * @sa tapi_job_method_create
 */
extern tapi_job_method_create rpc_job_create;

/**
 * Start a job
 *
 * @sa tapi_job_method_start
 */
extern tapi_job_method_start rpc_job_start;

/**
 * Allocate channels for a job
 *
 * @sa tapi_job_method_allocate_channels
 */
extern tapi_job_method_allocate_channels rpc_job_allocate_channels;

/**
 * Send a signal to the job
 *
 * @sa tapi_job_method_kill
 */
extern tapi_job_method_kill rpc_job_kill;

/**
 * Send a signal to the proccess group
 *
 * @sa tapi_job_method_killpg
 */
extern tapi_job_method_killpg rpc_job_killpg;

/**
 * Wait for the job completion
 *
 * @note Negative @p timeout_ms means #TAPI_RPC_JOB_BIG_TIMEOUT_MS.
 *       @c TE_ECHILD return value means that the job was never started.
 *
 * @sa tapi_job_method_wait
 */
extern tapi_job_method_wait rpc_job_wait;

/**
 * Stop a job
 *
 * @sa tapi_job_method_stop
 */
extern tapi_job_method_stop rpc_job_stop;

/**
 * Destroy a job
 *
 *
 * @sa tapi_job_method_destroy
 */
extern tapi_job_method_destroy rpc_job_destroy;

/**
 * Add a wrapper for a job
 *
 * @sa tapi_job_method_wrapper_add
 */
extern tapi_job_method_wrapper_add rpc_job_wrapper_add;

/**
 * Delete a wrapper
 *
 * @sa tapi_job_method_wrapper_delete
 */
extern tapi_job_method_wrapper_delete rpc_job_wrapper_delete;

/**
 * Add a scheduling parameters for a job
 *
 * @sa tapi_job_method_add_sched_param
 */
extern tapi_job_method_add_sched_param rpc_job_add_sched_param;

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_RPC_JOB_H__ */
