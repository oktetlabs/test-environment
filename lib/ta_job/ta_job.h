/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Library for Agent job control on agent side
 *
 * @defgroup ta_job Library for Agent job control on agent side (ta_job)
 * @ingroup tapi_job
 * @{
 *
 * Library to be used by backends for TAPI job.
 * The library provides types and functions that should be used by
 * TAPI job backends for agent job management.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TA_JOB_H__
#define __TA_JOB_H__

#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "te_exec_child.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Job manager instance which represents a particular TAPI job backend */
typedef struct ta_job_manager_t ta_job_manager_t;

/** Cause of job's completion */
typedef enum ta_job_status_type_t {
    /** Job terminated normally (by exit() or return from main) */
    TA_JOB_STATUS_EXITED,
    /** Job was terminated by a signal */
    TA_JOB_STATUS_SIGNALED,
    /** The cause of job termination is not known */
    TA_JOB_STATUS_UNKNOWN,
} ta_job_status_type_t;

/** Structure that represents status of a completed job */
typedef struct ta_job_status_t {
    /** Cause of job's completion */
    ta_job_status_type_t type;
    /**
     * Either an exit status of the job or a number of a signal
     * which caused the job's termination
     */
    int value;
} ta_job_status_t;

/** A structure to store messages produced by the job */
typedef struct ta_job_buffer_t {
    /** Channel from which the message was received */
    unsigned int channel_id;
    /** Filter from which the message was received */
    unsigned int filter_id;
    /** @c TRUE if the stream behind the filter has been closed */
    te_bool eos;
    /** Number of dropped messages */
    unsigned int dropped;
    /** Size of message content */
    size_t size;
    /*
     * We do not use te_string here since we consider the data to be just
     * a stream of bytes, not a C string
     */
    /** Message content */
    char *data;
} ta_job_buffer_t;

/** Default initializer for #ta_job_buffer_t */
#define TA_JOB_BUFFER_INIT (ta_job_buffer_t){ .data = NULL }

/**
 * Wrappers priority level
 */
/*
 * Note: New priority values must be added between
 * TA_JOB_WRAPPER_PRIORITY_MIN and TA_JOB_WRAPPER_PRIORITY_MAX.
 */
typedef enum ta_job_wrapper_priority_t {
    TA_JOB_WRAPPER_PRIORITY_MIN = 0, /**< A service item.
                                          Indicates the minimum value
                                          in this enum. */
    TA_JOB_WRAPPER_PRIORITY_LOW,     /**< The wrapper will be added to the
                                          rigth of default level according to
                                          the priority level. */
    TA_JOB_WRAPPER_PRIORITY_DEFAULT, /**< The wrapper will be added to the main
                                          tool from right to left according to
                                          the priority level. */
    TA_JOB_WRAPPER_PRIORITY_HIGH,    /**< The wrapper will be added to the
                                          left of default level according to
                                          the priority level. */
    TA_JOB_WRAPPER_PRIORITY_MAX,     /**<  A service item.
                                           Indicates the maxixmum value
                                           in this enum. */
} ta_job_wrapper_priority_t;

/**
 * Initialize a job manager. You must ensure that the function is called before
 * any backend function which uses the job manager, i.e. the manager must not be
 * used uninitialized.
 *
 * @param[out] manager         Job manager handle
 *
 * @return     Status code
 */
extern te_errno ta_job_manager_init(ta_job_manager_t **manager);

/**
 * Create a job
 *
 * @param[in]  manager         Job manager handle
 * @param[in]  spawner         Spawner plugin name
 *                             (may be @c NULL for the default plugin)
 * @param[in]  tool            Program path to run
 * @param[in]  argv            Program arguments (last item is @c NULL)
 * @param[in]  env             Program environment (last item is @c NULL).
 *                             May be @c NULL to keep the current environment.
 * @param[out] job_id          ID of the created job
 *
 * @note       @p argv and @p env are owned by the function on success and must
 *             not be modified or freed by the caller.
 *
 * @return     Status code
 */
extern te_errno ta_job_create(ta_job_manager_t *manager, const char *spawner,
                              const char *tool, char **argv, char **env,
                              unsigned int *job_id);

/**
 * Start a job
 *
 * @param      manager         Job manager handle
 * @param      id              ID of the job to be started
 *
 * @return     Status code
 */
extern te_errno ta_job_start(ta_job_manager_t *manger, unsigned int id);

/**
 * Allocate @p n_channels channels.
 * This function is supposed to be called only once for each job.
 *
 * @param[in]  manager         Job manager handle
 * @param[in]  job_id          ID of the job for which to allocate the channels
 * @param[in]  input_channels  @c TRUE to allocate input channels,
 *                             @c FALSE to allocate output channels
 * @param[in]  n_channels      Number of channels to allocate
 * @param[out] channels        Array of channels to be filled with ids of
 *                             the allocated channels
 *
 * @note       Memory for @p channels must be allocated before calling
 *             this function
 *
 * @return     Status code
 */
extern te_errno ta_job_allocate_channels(ta_job_manager_t *manager,
                                         unsigned int job_id,
                                         te_bool input_channels,
                                         unsigned int n_channels,
                                         unsigned int *channels);

/**
 * Deallocate @p n_channels channels
 *
 * @param      manager         Job manager handle
 * @param      n_channels      Number of channels to deallocate
 * @param      channels        Array of channels to be deallocated
 */
extern void ta_job_deallocate_channels(ta_job_manager_t *manager,
                                       unsigned int n_channels,
                                       unsigned int *channels);

/**
 * Attach filter to specified output channels
 *
 * @param[in]  manager         Job manager handle
 * @param[in]  filter_name     Name of the filter, may be @c NULL
 * @param[in]  n_channel       Number of channels to attach the filter to
 * @param[in]  channels        IDs of output channels to attach the filter to
 * @param[in]  readable        Whether or not it will be possible to get
 *                             an output of the filter via ta_job_receive()
 * @param[in]  log_level       If non-zero, the output of the filter is
 *                             logged with a given log level
 * @param[out] filter_id       ID of the attached filter
 *
 * @return     Status code
 */
extern te_errno ta_job_attach_filter(ta_job_manager_t *manager,
                                     const char *filter_name,
                                     unsigned int n_channels,
                                     unsigned int *channels,
                                     te_bool readable,
                                     te_log_level log_level,
                                     unsigned int *filter_id);

/**
 * Add a regular expression to a filter.
 * The filter will only store output that matches the regular expression.
 *
 * @param      manager         Job manager handle
 * @param      filter_id       ID of the filter to attach the regex to
 * @param      re              PCRE-style regular expression
 * @param      extract         Index of a capturing group to extract from
 *                             the matched output. @c 0 means to extract
 *                             the whole match.
 *
 * @note       Multi-segment matching is performed. Thus, PCRE_MULTILINE
 *             option is set.
 *
 * @return     Status code
 */
extern te_errno ta_job_filter_add_regexp(ta_job_manager_t *manager,
                                         unsigned int filter_id,
                                         char *re, unsigned int extract);

/**
 * Attach an existing filter to additional output channels
 *
 * @param      manager         Job manager handle
 * @param      filter_id       ID of the filter to attach
 * @param      n_channel       Number of channels to attach the filter to
 * @param      channels        IDs of output channels to attach the filter to
 *
 * @return     Status code
 */
extern te_errno ta_job_filter_add_channels(ta_job_manager_t *manager,
                                           unsigned int filter_id,
                                           unsigned int n_channels,
                                           unsigned int *channels);

/**
 * Remove filter from specified output channels.
 * For instance, if the filter is attached to stdout and stderr and the function
 * is called with only stdout specified, the filter will continue working
 * with stderr.
 * Either the function succeeds and the filter is removed from all specified
 * channels, or the function fails and the filter is not removed from any
 * channel.
 *
 * @note       If the filter is removed from every channel it was attached to,
 *             all associated resources will be freed, so ta_job_receive() will
 *             fail
 *
 * @param      manager         Job manager handle
 * @param      filter_id       ID of the filter to remove
 * @param      n_channels      Number of channels to remove the filter from
 * @param      channels        IDs of output channels to remove the filter from
 *
 * @return     Status code
 */
extern te_errno ta_job_filter_remove_channels(ta_job_manager_t *manager,
                                              unsigned int filter_id,
                                              unsigned int n_channels,
                                              unsigned int *channels);

/**
 * Poll channels/filters for readiness.
 * A filter is considered ready if it is readable and has some data that
 * can be got via ta_job_receive(). A channel is considered ready if it
 * is an input channel and it is ready to accept input.
 *
 * @param      manager         Job manager handle
 * @param      n_channels      Number of channels to poll
 * @param      channel_ids     IDs of channels to poll
 * @param      timeout_ms      Timeout to wait for readiness
 * @param      filter_only     @c TRUE if only channels are provided.
 *                             In this case, if one of ids is actually
 *                             an id of a channel, not a filter, TE_EPERM
 *                             will be returned.
 *
 * @return     Status code
 * @retval     0               One of the channels/filters is ready
 * @retval     TE_ETIMEDOUT    None of the channels/filters was ready
 *                             within @p timeout_ms
 */
extern te_errno ta_job_poll(ta_job_manager_t *manager, unsigned int n_channels,
                            unsigned int *channel_ids, int timeout_ms,
                            te_bool filter_only);

/**
 * Receive the first message from one of the filters and remove it from
 * the filter's message queue.
 *
 * @param[in]  manager         Job manager handle
 * @param[in]  n_filters       Number of filters to receive from
 * @param[in]  filters         ID of filters to receive from
 * @param[in]  timeout_ms      Time to wait for data to appear on some filter
 * @param[out] buffer          Buffer to store the received data
 *
 * @return     Status code
 */
extern te_errno ta_job_receive(ta_job_manager_t *manager,
                               unsigned int n_filters, unsigned int *filters,
                               int timeout_ms, ta_job_buffer_t *buffer);

/**
 * Receive the last non-eos message (if there is one) from one of the filters.
 * If the only message contains eos, read it. The message is not removed
 * from the filter's message queue, it can still be read with ta_job_receive().
 *
 * @param[in]  manager         Job manager handle
 * @param[in]  n_filters       Number of filters to receive from
 * @param[in]  filters         ID of filters to receive from
 * @param[in]  timeout_ms      Time to wait for data to appear on some filter
 * @param[out] buffer          Buffer to store the received data
 *
 * @return     Status code
 */
extern te_errno ta_job_receive_last(ta_job_manager_t *manager,
                                    unsigned int n_filters,
                                    unsigned int *filters, int timeout_ms,
                                    ta_job_buffer_t *buffer);

/**
 * Receive multiple messages at once from the specified filters.
 * This function may retrieve some messages and return error if attempt to read
 * the next message failed.
 *
 * @param[in]    manager       Job manager handle
 * @param[in]    n_filters     Number of filters to receive from
 * @param[in]    filters       ID of filters to receive from
 * @param[in]    timeout_ms    Time to wait for data to appear on some filter
 * @param[out]   buffers       Where to save pointer to array of buffers with
 *                             messages
 * @param[in,out] count        On input, maximum number of messages to retrieve.
 *                             If zero, all available messages will be
 *                             retrieved. On output - number of actually
 *                             retrieved messages.
 *
 * @return       Status code
 */
extern te_errno ta_job_receive_many(ta_job_manager_t *manager,
                                    unsigned int n_filters,
                                    unsigned int *filters, int timeout_ms,
                                    ta_job_buffer_t **buffers,
                                    unsigned int *count);

/**
 * Remove all messages from the filters' message queues
 *
 * @param      manager         Job manager handle
 * @param      n_filters       Number of filters to be cleared
 * @param      filters         IDs of filters to be cleared
 *
 * @return     Status code
 */
extern te_errno ta_job_clear(ta_job_manager_t *manager, unsigned int n_filters,
                             unsigned int *filters);

/**
 * Send data to a job through an input channel
 *
 * @param      manager         Job manager handle
 * @param      channel_id      ID of the input channel
 * @param      count           Number of bytes to be sent
 * @param      buf             Data to be sent
 *
 * @return     Status code
 */
extern te_errno ta_job_send(ta_job_manager_t *manager, unsigned int channel_id,
                            size_t count, uint8_t *buf);

/**
 * Send a signal to a job
 *
 * @param      manager         Job manager handle
 * @param      job_id          ID of the job to send signal to
 * @param      signo           Number of the signal to send
 *
 * @return     Status code
 */
extern te_errno ta_job_kill(ta_job_manager_t *manager, unsigned int job_id,
                            int signo);

/**
 * Send a signal to a job's process group
 *
 * @param      manager         Job manager handle
 * @param      job_id          ID of the job to send signal to
 * @param      signo           Number of the signal to send
 *
 * @return     Status code
 */
extern te_errno ta_job_killpg(ta_job_manager_t *manager, unsigned int job_id,
                              int signo);
/**
 * Wait for a job completion (or check its status if @p timeout_ms is zero)
 *
 * @param[in]  manager         Job manager handle
 * @param[in]  job_id          ID of the job to wait for
 * @param[in]  timeout_ms      Time to wait for the job. @c 0 means to check
 *                             current status and exit, negative value means
 *                             that the call of the function blocks until
 *                             the job changes its status.
 * @param[out] status          Job exit status location, may be @c NULL
 *
 * @return     Status code
 * @retval     0               The job completed running
 * @retval     TE_EINPROGRESS  The job is still running
 * @retval     TE_ECHILD       The job was never started
 */
extern te_errno ta_job_wait(ta_job_manager_t *manager, unsigned int job_id,
                            int timeout_ms, ta_job_status_t *status);

/**
 * Stop a job. It can be started over with ta_job_start().
 * The function tries to terminate the job with the specified signal.
 * If the signal fails to terminate the job withing @p term_timeout_ms,
 * the function will send @c SIGKILL.
 *
 * @param      manager         Job manager handle
 * @param      job             ID of the job to stop
 * @param      signo           Number of the signal to send. If @p signo is
 *                             @c SIGKILL, it will be sent only once.
 * @param      term_timeout_ms The timeout of graceful termination of a job,
 *                             if it has been running. After the timeout
 *                             expiration the job will be killed with
 *                             @c SIGKILL. Negative means default timeout.
 *
 * @return      Status code
 */
extern te_errno ta_job_stop(ta_job_manager_t *manager, unsigned int job_id,
                            int signo, int term_timeout_ms);

/**
 * Destroy a job instance. If the job has started, it is terminated
 * as gracefully as possible. All resources of the instance are freed,
 * all unread data on all filters are lost.
 *
 * @param      manager         Job manager handle
 * @param      job_id          ID of the job to destroy
 * @param      term_timeout_ms The timeout of graceful termination of a job,
 *                             if it has been running. After the timeout
 *                             expiration the job will be killed with
 *                             @c SIGKILL. Negative means default timeout.
 *
 * @return     Status code
 */
extern te_errno ta_job_destroy(ta_job_manager_t *manager, unsigned int job_id,
                               int term_timeout_ms);

/**
 * Add a wrapper for a job that is not running
 *
 * @note       The wrapper must be added after the job is created
 *
 * @note       The function can be called several times. Wrappers will be added
 *             to the main tool from right to left according to the
 *             priority levels.
 *
 * @param[in]  manager         Job manager handle
 * @param[in]  tool            Path to the wrapper tool
 * @param[in]  argv            Wrapper arguments (last item must be @c NULL)
 * @param[id]  job_id          ID of the job to add the wrapper to
 * @param[in]  priority        Wrapper priority
 * @param[out] wrapper_id      ID of the added wrapper
 *
 * @return     Status code
 */
extern te_errno ta_job_wrapper_add(ta_job_manager_t *manager, const char *tool,
                                   char **argv, unsigned int job_id,
                                   ta_job_wrapper_priority_t priority,
                                   unsigned int *wrapper_id);

/**
 * Delete a wrapper
 *
 * @note       There is no necessity to delete wrappers before job destruction.
 *             All wrappers are removed automatically together with the job.
 *
 * @param      manager         Job manager handle
 * @param      job_id          ID of a job to which the wrapper belongs
 * @param      wrapper_id      ID of a wrapper to delete
 *
 * @return     Status code
 */
extern te_errno ta_job_wrapper_delete(ta_job_manager_t *manager,
                                      unsigned int job_id,
                                      unsigned int wrapper_id);

/**
 * Add a scheduling parameter for a job
 *
 * @param      manager         Job manager handle
 * @param      job_id          ID of the job to add the parametern to
 * @param      sched_params    Array of scheduling parameters. The last element
 *                             must have the type set to @c TE_SCHED_END
 *                             and the data set to @c NULL.
 *
 * @return     Status code
 */
extern te_errno ta_job_add_sched_param(ta_job_manager_t *manager,
                                       unsigned int job_id,
                                       te_sched_param *sched_params);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TA_JOB_H__ */

/**@} <!-- END ta_job --> */
