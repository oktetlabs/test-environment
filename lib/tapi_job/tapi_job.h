/** @file
 * @brief Agent job control
 *
 * @defgroup tapi_job Agent job control
 * @ingroup te_ts_tapi
 * @{
 *
 * API to manage subordinate jobs at the agent side
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#ifndef __TAPI_JOB_H__
#define __TAPI_JOB_H__

#include "te_errno.h"
#include "te_string.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An opaque type to represent factory that creates job instances
 */
typedef struct tapi_job_factory_t tapi_job_factory_t;

/**
 * An opaque type to represent job instances
 */
typedef struct tapi_job_t tapi_job_t;

/**
 * An opaque type to represent wrapper instances
 */
typedef struct tapi_job_wrapper_t tapi_job_wrapper_t;

/**
 * Job channel handle
 */
typedef struct tapi_job_channel_t tapi_job_channel_t;

/**
 * Job channel set (a @c NULL terminated vector)
 */
typedef tapi_job_channel_t *tapi_job_channel_set_t[];

/**
 * A convenience vector constructor to use with polling functions
 */
#define TAPI_JOB_CHANNEL_SET(...)               \
    (tapi_job_channel_set_t){__VA_ARGS__, NULL}

/**
 * Get name of the test agent that hosts job instances created by
 * @p factory.
 *
 * @note                The returned pointer should not be used after
 *                      factory destruction
 *
 * @param factory       Job factory
 *
 * @return              Test Agent name (or @c NULL on failure)
 */
extern const char * tapi_job_factory_ta(const tapi_job_factory_t *factory);

/**
 * Set path in factory's environment to /agent:/env:PATH. The environment
 * is inherited by created job if the environment is not specified in
 * tapi_job_create().
 *
 * @note This function supports only RPC factories
 *
 * @note Job TAPI does not automatically inherit the environment from /agent:env
 *
 * @param factory       Job factory
 *
 * @return              Status code
 */
extern te_errno tapi_job_factory_set_path(tapi_job_factory_t *factory);

/**
 * Destroy a job factory
 *
 * @param factory       Job factory
 */
extern void tapi_job_factory_destroy(tapi_job_factory_t *factory);

/**
 * Create a job controlled in the way specified by @p factory.
 * The job will be managed by @p spawner plugin.
 *
 * @note The created job is *not* started automatically,
 *       use tapi_job_start() to actually run it.
 * @note The first element of @p args, by convention of exec family functions,
 *       should point to the filename associated with the file being executed.
 *
 * @note Unnamed jobs created by CFG factory are not supposed to live longer
 *       than one test, so they should be destroyed at the end of the test where
 *       they were created. Otherwise, trying to create an unnamed job by CFG
 *       factory in a subsequent test will lead to an error.
 *
 * @param factory   Job factory
 * @param spawner   Spawner plugin name
 *                 (may be @c NULL for the default plugin)
 * @param program   Program path to run
 * @param argv      Program arguments (last item is @c NULL)
 * @param env       Program environment (last item is @c NULL).
 *                  May be @c NULL to keep the current environment.
 * @param[out] job  Job handle
 *
 * @return          Status code
 */
extern te_errno tapi_job_create(tapi_job_factory_t *factory,
                                const char *spawner,
                                const char *program,
                                const char **argv,
                                const char **env,
                                tapi_job_t **job);

/**
 * Same as tapi_job_create(), but allows to set a name for the job being created
 *
 * @param factory   Job factory
 * @param name      Name of the job.
 *                  If CFG job factory is used, this parameter is mandatory.
                    If RPC job factory is used, it should be @c NULL.
 * @param spawner   Spawner plugin name (may be @c NULL for the default plugin)
 * @param program   Program path to run
 * @param argv      Program arguments (last item is @c NULL)
 * @param env       Program environment (last item is @c NULL).
 *                  May be @c NULL to keep the current environment.
 * @param[out] job  Job handle
 *
 * @return          Status code
 *
 * @sa tapi_job_create
 */
extern te_errno tapi_job_create_named(tapi_job_factory_t *factory,
                                      const char *name,
                                      const char *spawner,
                                      const char *program,
                                      const char **argv,
                                      const char **env,
                                      tapi_job_t **job);

/**
 * Get handle of a job that was once created and hasn't been destroyed.
 *
 * @param[in]  factory     Job factory that was used when the job was first
 *                         created
 * @param[in]  identifier  Data that allows to identify the job.
 *                         For jobs created by CFG factory it must be a C-string
 *                         with a name that was assigned to the job when it was
 *                         first create.
 * @param[out] job         Location where to put the job handle
 *
 * @return     Status code
 * @retval     TE_ENOENT   The job with the @p identifier was never created
 */
extern te_errno tapi_job_recreate(tapi_job_factory_t *factory,
                                  const void *identifier, tapi_job_t **job);

/**
 * A simplified description of an output filter.
 * The caller is expected to fill the fields one is
 * interested in and leave others @c NULL or @c 0.
 */
typedef struct tapi_job_simple_filter_t {
    /** Attach to the job's stdout */
    te_bool use_stdout;
    /** Attach to the job's stderr */
    te_bool use_stderr;
    /** Filter name (default if @c NULL) */
    const char *filter_name;
    /** Whether the filter is readable by the test */
    te_bool readable;
    /** Log level (if 0, output is not logged) */
    te_log_level log_level;
    /** Regexp to match */
    const char *re;
    /** Index of matched substring (0 means whole match) */
    unsigned extract;
    /** Location of a variable to hold the filter handle */
    tapi_job_channel_t **filter_var;
} tapi_job_simple_filter_t;

/**
 * A simplified description of a job.
 * The caller is expected to fill the fields one is
 * interested in and leave others @c NULL.
 */
typedef struct tapi_job_simple_desc_t {
    /**
     * Name of the job.
     * Might be useful for jobs created by CFG factory
     * (see tapi_job_create_named()).
     */
    const char *name;
    /** Spawner type */
    const char *spawner;
    /** Program path */
    const char *program;
    /** Program argument vector */
    const char **argv;
    /** Program environment */
    const char **env;
    /** Location for a job handle var */
    tapi_job_t **job_loc;
    /** Location for a channel handle connected to stdin */
    tapi_job_channel_t **stdin_loc;
    /** Location for a channel handle connected to stdout */
    tapi_job_channel_t **stdout_loc;
    /** Location for a channel handle connected to stderr */
    tapi_job_channel_t **stderr_loc;
    /** Vector of filters.
     * If not @c NULL, the last element should have
     * @a use_stdout and @a use_stderr set to @c FALSE
     */
    tapi_job_simple_filter_t *filters;
} tapi_job_simple_desc_t;

/**
 * A helper to create vectors of job simple filter descriptions
 */
#define TAPI_JOB_SIMPLE_FILTERS(...)                \
    ((tapi_job_simple_filter_t [])                  \
    {__VA_ARGS__,                                   \
        {.use_stdout = FALSE, .use_stderr = FALSE}  \
    })

/**
 * Create a job based on a description
 *
 * @param factory   Job factory
 * @param desc      Job description
 *
 * @return       Status code
 * @retval TE_EALREADY A job associated with @p desc already created
 */
extern te_errno tapi_job_simple_create(tapi_job_factory_t *factory,
                                       tapi_job_simple_desc_t *desc);

/**
 * Start a job
 *
 * @note For autorestart jobs this function should be called only once.
 *       The following job executions will be done by the autorestart
 *       subsystem.
 *
 * @param job Job instance handle
 */
extern te_errno tapi_job_start(tapi_job_t *job);

/**
 * Send a signal to the job
 *
 * @param job Job instance handle
 * @param signo Signal number
 *
 * @return          Status code
 */
extern te_errno tapi_job_kill(tapi_job_t *job, int signo);

/**
 * Send a signal to the proccess group
 *
 * @param job Job instance handle
 * @param signo Signal number
 *
 * @return          Status code
 */
extern te_errno tapi_job_killpg(tapi_job_t *job, int signo);

/**
 * Cause of a job's completion
 */
typedef enum {
    TAPI_JOB_STATUS_EXITED,   /**< Job terminated normally
                                   (by exit() or return from main) */
    TAPI_JOB_STATUS_SIGNALED, /**< Job was terminated by a signal */
    TAPI_JOB_STATUS_UNKNOWN,  /**< The cause of job termination is not known */
} tapi_job_status_type_t;

/**
 * A structure that represents completed job status
 */
typedef struct tapi_job_status_t {
    tapi_job_status_type_t type;
    int value;
} tapi_job_status_t;

/**
 * Get timeout of tapi_job function that is set when negative timeout
 * is passed to the function
 *
 * @return Timeout in millisecond
 */
extern unsigned int tapi_job_get_timeout(void);

/**
 * Wait for the job completion (or check its status if @p timeout is zero)
 *
 * @param job Job instance handle
 * @param timeout_ms Timeout in ms. Meaning of negative timeout depends on the
 *                   factory type that was used to create the job.
 *                   See rpc_job_wait() and cfg_job_wait() for details.
 * @param[out] status Exit status (may be @c NULL)
 *
 * @return                  Status code
 * @retval TE_EINPROGRESS   Job is still running. For autorestart jobs, this
 *                          means that the autorestart subsystem is working with
 *                          the job and it will be restarted when needed.
 * @retval TE_ECHILD        Job was never started
 *
 * @note For jobs created by some factories it's possible that this function
 *       will return zero if the job was never started
 *
 * @sa tapi_job_set_autorestart
 */
extern te_errno tapi_job_wait(tapi_job_t *job, int timeout_ms,
                              tapi_job_status_t *status);

/**
 * Check if the status is a success status (i.e. the job exited successfully)
 *
 * @param _status   Job exit status
 */
#define TAPI_JOB_CHECK_STATUS(_status)                                        \
    do {                                                                      \
        if ((_status).type != TAPI_JOB_STATUS_EXITED || (_status).value != 0) \
            return TE_RC(TE_TAPI, TE_ESHCMD);                                 \
    } while (0)

/**
 * Check whether a job is running
 *
 * @param      job        Job instace handle
 *
 * @return                @c TRUE if @p job is running
 * @exception TEST_FAIL
 */
extern te_bool tapi_job_is_running(tapi_job_t *job);

/**
 * Allocate @p n_channels input channels.
 * The first channel is expected to be connected to the job's stdin;
 * the wiring of others is spawner-dependant.
 *
 * @note This function supports only jobs created by RPC factory
 *
 * @param      job        Job instace handle
 * @param      n_channels Number of channels
 * @param[out] channels   A vector of obtained channel handles
 *
 * @return          Status code
 */
extern te_errno tapi_job_alloc_input_channels(tapi_job_t *job,
                                              unsigned int n_channels,
                                              tapi_job_channel_t *
                                              channels[n_channels]);

/**
 * Allocate @p n_channels output channels.
 * The first and the second output channels are expected to be
 * connected to stdout and stderr resp., the wiring of others is
 * spawner-dependant.
 *
 * @note This function supports only jobs created by RPC factory
 *
 * @note The implementation need not support any other operations on
 *       primary output channels other than attaching and removing filters
 *       to/from them (see tapi_job_attach_filter(),
 *       tapi_job_filter_add_channels(), and tapi_job_filter_remove_channels())
 *
 * @param      job        Job instace handle
 * @param      n_channels Number of channels
 * @param[out] channels   A vector of obtained channel handles
 *
 * @return          Status code
 */
extern te_errno tapi_job_alloc_output_channels(tapi_job_t *job,
                                               unsigned int n_channels,
                                               tapi_job_channel_t *
                                               channels[n_channels]);

/**
 * Deallocate primary channels.
 *
 * @note The channels are freed by the function.
 *
 * @param channels      Channels to be deallocated.
 *
 * @return Status code
 */
extern te_errno tapi_job_dealloc_channels(tapi_job_channel_set_t channels);

/**
 * Create a secondary output channel applying a filter to an existing
 * channel.
 *
 * @note The implementation is only required to support filter channels
 * over primary channels, so filter chaining may not be supported.
 *
 * @param channels      Output channels to attach the filter to.
 * @param filter_name   Filter name (may be @c NULL,
 *                      then the default filter is used) *
 * @param readable      If @c TRUE, the output of the filter is
 *                      sent to the test, that is, it can be read with
 *                      tapi_job_receive(); otherwise, it is discarded,
 *                      possibly after being logged.
 * @param log_level     If non-zero, the output of the filter is
 *                      logged with a given log level
 * @param[out] filter   Filter channel (may be @c NULL for trivial filters)
 *
 * @return           Status code
 * @retval TE_EPERM  Some of @p channels are input channels
 * @retval TE_EINVAL Some of @p channels are filter output
 *                   channels if the implementaion does not support filters
 *                   over filters.
 * @retval TE_EXDEV  @p channels are on different RPC servers
 */
extern te_errno tapi_job_attach_filter(tapi_job_channel_set_t channels,
                                       const char *filter_name,
                                       te_bool readable,
                                       te_log_level log_level,
                                       tapi_job_channel_t **filter);

/**
 * Attach a simple filter to a job created by tapi_job_simple_create()
 *
 * @param desc   Job description (it must have @a job_loc, @a stdin_loc and
 *               @a stderr_loc filled in)
 * @param filter Filter to attach
 *
 * @return       Status code
 * @retval TE_ENOTCONN tapi_job_simple_create() has not been called on
 *                     @p desc
 */
extern te_errno tapi_job_attach_simple_filter(const tapi_job_simple_desc_t *desc,
                                              tapi_job_simple_filter_t *filter);


/**
 * Add a regular expression for filter
 *
 * @param filter   Filter handle
 * @param re       PCRE-style regular expression to match
 * @param extract  A substring to extract as an output of the filter
 *                 (0 meaning the whole match)
 *
 * @note           Multi-segment matching is performed. Thus, PCRE_MULTILINE
 *                 option is set.
 *
 * @return         Status code
 */
extern te_errno tapi_job_filter_add_regexp(tapi_job_channel_t *filter,
                                           const char *re,
                                           unsigned int extract);

/**
 * Attach an existing filter to additional output channels
 *
 * @param filter   Filter to attach
 * @param channels Output channels to attach the filter to
 *
 * @return         Status code
 */
extern te_errno tapi_job_filter_add_channels(tapi_job_channel_t *filter,
                                             tapi_job_channel_set_t channels);

/**
 * Remove filter from specified output channels.
 * For instance, if the filter is attached to stdout and stderr and the function
 * is called with only stdout specified, the filter will continue working
 * with stderr.
 * Either the function succeeds and the filter is removed from all specified
 * channels, or the function fails and the filter is not removed from any
 * channel.
 *
 * @note           If the filter is removed from every channel it was attached
 *                 to, all associated resources including the memory for
 *                 tapi_job_channel_t structure will be freed, so @p filter
 *                 variable must not be used again.
 *
 * @param filter   Filter to remove
 * @param channels Channels from which to remove the filter
 *
 * @return         Status code
 */
extern te_errno tapi_job_filter_remove_channels(tapi_job_channel_t *filter,
                                                tapi_job_channel_set_t
                                                                    channels);

/**
 * Send data to a job input channel.
 *
 * @param channel  Output channel handle
 * @param str      A pointer to string buffer
 *
 * @return          Status code
 * @retval TE_EPERM if @p channel is not an input channel
 */
extern te_errno tapi_job_send(tapi_job_channel_t *channel,
                              const te_string *str);

/**
 * The same as tapi_job_send(), but fails the test if an error
 * happens, instead of returning an error
 *
 * @param channel   Input channel handle
 * @param str      A pointer to string buffer
 *
 * @exception TEST_FAIL
 */
extern void tapi_job_simple_send(tapi_job_channel_t *channel,
                                 const te_string *str);


/**
 * Poll the job's channels/filters for readiness.
 * from @p wait_set
 *
 * @param wait_set      Set of channels to wait
 * @param timeout_ms    Timeout in ms to wait (negative means
 *                      tapi_job_get_timeout())
 *
 * @return          Status code
 * @retval TE_EPERM if some channels from @p wait_set are neither input
 *                  channels nor pollable output channels
 * @retval TE_EXDEV if channels from @p wait_set are from different RPC
 *                  servers
 */
extern te_errno tapi_job_poll(const tapi_job_channel_set_t wait_set,
                              int timeout_ms);
/**
 * The same as tapi_job_poll(), but fails the test if an error
 * happens, instead of returning an error.
 *
 * @param wait_set Set of channels to wait
 * @param timeout_ms Timeout in ms to wait (negative means
 *                   tapi_job_get_timeout())
 *
 * @exception TEST_FAIL
 */
extern void tapi_job_simple_poll(const tapi_job_channel_set_t wait_set,
                                 int timeout_ms);

/**
 * A structure to store messages read by tapi_job_receive()
 * or tapi_job_receive_last().
 */
typedef struct tapi_job_buffer_t {
    tapi_job_channel_t *channel; /**< Last message channel */
    tapi_job_channel_t *filter; /**< Last message filter */
    unsigned int dropped; /**< Number of dropped messages */
    te_bool eos; /**<
                  * @c TRUE if the stream behind the filter has been closed.
                  * If #eos is received and a job is not started again,
                  * the next tapi_job_receive() will time out.
                  * The stream behind the filter is re-opened when a job
                  * restarts. #eos is used as a separation between runs.
                  */
    te_string data; /**< Data buffer */
} tapi_job_buffer_t;

/**
 * Static initializer for #tapi_job_buffer_t
 */
#define TAPI_JOB_BUFFER_INIT {.data = TE_STRING_INIT}

/**
 * Read the next message from one of the available filters.
 *
 * The data being read are appended to @a data buffer.
 * At most one message is read.
 *
 * @param filters     Set of filters to read from.
 * @param timeout_ms  Timeout to wait (negative means tapi_job_get_timeout())
 * @param buffer      Data buffer pointer. If @c NULL, the message is
 *                    silently discarded.
 *
 * @return            Status code
 * @retval TE_ETIMEDOUT     if there's no data available within @p timeout
 * @retval TE_EPERM   if some of the @p filters are input channels or
 *                    primary output channels and the implementation does
 *                    not support reading from them or unreadable output
 *                    channels
 * @retval TE_EXDEV   if @p filters are on different RPC servers
 */
extern te_errno tapi_job_receive(const tapi_job_channel_set_t filters,
                                 int timeout_ms,
                                 tapi_job_buffer_t *buffer);

/**
 * Read the last non-eos message (if there is one) from one of the
 * available filters. If the only message contains eos, read it.
 * The message is not removed from the queue, it can still be read with
 * tapi_job_receive().
 *
 * The data being read are appended to @a data buffer.
 * At most one message is read.
 *
 * @param filters     Set of filters to read from.
 * @param timeout_ms  Timeout to wait (negative means tapi_job_get_timeout())
 * @param buffer      Data buffer pointer. If @c NULL, the message is
 *                    silently discarded.
 *
 * @return            Status code
 * @retval TE_ETIMEDOUT     if there's no data available within @p timeout
 * @retval TE_EPERM   if some of the @p filters are input channels or
 *                    primary output channels and the implementation does
 *                    not support reading from them or unreadable output
 *                    channels
 * @retval TE_EXDEV   if @p filters are on different RPC servers
 */
extern te_errno tapi_job_receive_last(const tapi_job_channel_set_t filters,
                                      int timeout_ms,
                                      tapi_job_buffer_t *buffer);

/**
 * Obtain multiple messages at once from the specified filters.
 * This function may retrieve some messages and return error if attempt to
 * read the next message failed.
 *
 * @param filters     Set of filters to read from
 * @param timeout_ms  Timeout to wait (negative means tapi_job_get_timeout())
 *                    until some messages are available
 * @param buffers     Where to save pointer to array of buffers with
 *                    messages (should be released by caller with
 *                    tapi_job_buffers_free())
 * @param count       On input, maximum number of messages to retrieve.
 *                    If zero, all available messages will be retrieved.
 *                    On output - number of actually retrieved messages
 *
 * @return Status code.
 */
extern te_errno tapi_job_receive_many(const tapi_job_channel_set_t filters,
                                      int timeout_ms,
                                      tapi_job_buffer_t **buffers,
                                      unsigned int *count);

/**
 * Release array of message buffers.
 *
 * @param buffers     Pointer to the array
 * @param count       Number of buffers in the array
 */
extern void tapi_job_buffers_free(tapi_job_buffer_t *buffers,
                                  unsigned int count);

/**
 * Check if there is at least one non-eos message on one of the available
 * filters. The message is not removed from the queue, it can still be read with
 * tapi_job_receive().
 *
 * The function is useful to check if a job produced some particular output.
 *
 * @param filters     Set of filters to read from.
 * @param timeout_ms  Timeout to wait (negative means tapi_job_get_timeout())
 *
 * @return            @c TRUE if there is a non-eos message
 */
extern te_bool tapi_job_filters_have_data(const tapi_job_channel_set_t filters,
                                          int timeout_ms);

/**
 * The same as tapi_job_receive() but fails the test if an error
 * happens. Also, unlike tapi_job_receive(), the function resets the
 * @p buffer contents.
 *
 * @param filters     Set of filters to read from.
 * @param timeout_ms  Timeout to wait (negative means tapi_job_get_timeout())
 * @param buffer      Data buffer pointer. If @c NULL, the message is
 *                    silently discarded.
 *
 * @exception TEST_FAIL
 */
extern void tapi_job_simple_receive(const tapi_job_channel_set_t filters,
                                    int timeout_ms,
                                    tapi_job_buffer_t *buffer);

/**
 * Get contents of a single message. This function will fail if
 * there is more than one message. It is useful for filters looking
 * for something unique in the output.
 *
 * @param filter      From where to read the message
 * @param val         Where to save the contents
 * @param timeout_ms  Timeout to wait (negative means tapi_job_get_timeout())
 *
 * @return Status code.
 */
extern te_errno tapi_job_receive_single(tapi_job_channel_t *filter,
                                        te_string *val,
                                        int timeout_ms);

/**
 * Remove all pending messages from filters, they are lost completely.
 *
 * @param filters     Set of filters to clear.
 *
 * @return            Status code
 * @retval TE_EPERM   if some of the @p filters are input channels or
 *                    primary output channels and the implementation does
 *                    not support reading from them or unreadable output
 *                    channels
 * @retval TE_EXDEV   if @p filters are on different RPC servers
 */
extern te_errno tapi_job_clear(const tapi_job_channel_set_t filters);

/**
 * Stop a job. It can be started over with tapi_job_start().
 * The function tries to terminate the job with the specified signal.
 * If the signal fails to terminate the job, the function will send @c SIGKILL.
 *
 * @note Parameters @p signo and @p term_timeout_ms are supported only for jobs
 *       created by RPC factory. Use @c -1 to avoid warnings.
 *
 * @note For autorestart jobs this function will stop the job and prevent the
 *       autorestart subsystem from starting the job over
 *
 * @param job               Job instance handle
 * @param signo             Signal to be sent at first. If signo is @c SIGKILL,
 *                          it will be sent only once.
 * @param term_timeout_ms   The timeout of graceful termination of a job,
 *                          if it has been running. After the timeout expiration
 *                          the job will be killed with @c SIGKILL.
 *                          (negative means default timeout)
 *
 * @return Status code
 *
 * @sa tapi_job_set_autorestart
 */
extern te_errno tapi_job_stop(tapi_job_t *job, int signo, int term_timeout_ms);

/**
 * Destroy the job instance. If the job has started, it is terminated
 * as gracefully as possible. All resources of the instance are freed;
 * all unread data on all filters are lost.
 *
 * @note Parameter @p term_timeout_ms is supported only for jobs created by RPC
 *       factory. Use @c -1 to avoid warnings.
 *
 * @param job               Job instance handle
 * @param term_timeout_ms   The timeout of graceful termination of a job,
 *                          if it has been running. After the timeout expiration
 *                          the job will be killed with SIGKILL. (negative means
 *                          defult timeout)
 *
 * @return        Status code
 */
extern te_errno tapi_job_destroy(tapi_job_t *job, int term_timeout_ms);

/**
 * Wrappers priority level
 */
typedef enum {
    TAPI_JOB_WRAPPER_PRIORITY_LOW = 0, /**< The wrapper will be added to the
                                            rigth of default level according to
                                            the priority level. */
    TAPI_JOB_WRAPPER_PRIORITY_DEFAULT, /**< The Wrappers will be
                                            added to the main tool from right
                                            to left according to the priority level. */
    TAPI_JOB_WRAPPER_PRIORITY_HIGH     /**< The wrapper will be added to the
                                            rigth of default level according to
                                            the priority level. */
} tapi_job_wrapper_priority_t;

/**
 * Add a wrapper for the specified job. We can only add a wrapper
 * to a job that hasn't started yet.
 *
 * @note This function supports only jobs created by RPC factory
 *
 * @note The wrapper must be added after the job is created.
 *
 * @note We can call this function several times. Wrappers will be
 *       added to the main tool from right to left according to the
 *       priority level.
 *
 * @param[in]  job      Job instance handle
 * @param[in]  tool     Path to the wrapper tool.
 * @param[in]  argv     Wrapper arguments (last item is @c NULL)
 * @param[in]  priority Wrapper priority.
 * @param[out] wrap     Wrapper instance handle
 *
 * @return         Status code
 */
extern te_errno tapi_job_wrapper_add(tapi_job_t *job, const char *tool,
                                     const char **argv,
                                     tapi_job_wrapper_priority_t priority,
                                     tapi_job_wrapper_t **wrap);

/**
 * Delete the wrapper instance handle.
 *
 * @note There is no necessity to delete wrappers before job destruction.
 *       All wrappers are removed automatically together with the job.
 *
 * @param wrapper Wrapper instance handle
 *
 * @return        Status code
 */
extern te_errno tapi_job_wrapper_delete(tapi_job_wrapper_t *wrapper);

/** Scheduling type */
typedef enum tapi_job_sched_param_type {
    TAPI_JOB_SCHED_AFFINITY,
    TAPI_JOB_SCHED_PRIORITY,
    TAPI_JOB_SCHED_END
} tapi_job_sched_param_type;

/** Scheduling parameters */
typedef struct tapi_job_sched_param {
    /** Type of scheduling */
    tapi_job_sched_param_type type;
    /** Data specific to the specified type */
    void *data;
} tapi_job_sched_param;

/** Data specific for CPU affinity scheduling type */
typedef struct tapi_job_sched_affinity_param {
    /** Array of CPU IDs */
    int *cpu_ids;
     /** Array size */
    int cpu_ids_len;
} tapi_job_sched_affinity_param;

/** Data specific for priority scheduling type */
typedef struct tapi_job_sched_priority_param {
     /** Process priority. */
    int priority;
} tapi_job_sched_priority_param;

/**
 * Add a scheduling parameters for the specified job.
 *
 * @note This function supports only jobs created by RPC factory
 *
 * @param job          Job instance handle
 * @param sched_param  Array of scheduling parameters. The last element must
 *                     have the type @c TAPI_JOB_SCHED_END and data pointer to
 *                     @c NULL.
 *
 * @return Status code
 */
extern te_errno tapi_job_add_sched_param(tapi_job_t *job,
                                         tapi_job_sched_param *sched_param);

/**
 * Set autorestart timeout for the job.
 * The value represents a frequency with which the autorestart subsystem
 * will check whether the process stopped running (regardless of the reason)
 * and restart it if it did.
 *
 * @note This function supports only jobs created by CFG factory
 *
 * @note For jobs created by CFG factory this function should be called before
 *       the job is started
 *
 * @param      job          Job instance handle
 * @param      value        Autorestart timeout in seconds or @c 0 to disable
 *                          autorestart for the process.
 *
 * @return     Status code
 */
extern te_errno tapi_job_set_autorestart(tapi_job_t *job, unsigned int value);

/**
 * Get autorestart timeout
 *
 * @note This function supports only jobs created by CFG factory
 *
 * @param[in]  job          Job instance handle
 * @param[out] value        Autorestart timeout in seconds. If @c 0,
 *                          the autorestart is disabled.
 *
 * @return     Status code
 */
extern te_errno tapi_job_get_autorestart(tapi_job_t *job, unsigned int *value);

/**
 * @page tapi-job-factory Creating tapi_job_t instances
 *
 * Before a job can be created, a job factory must be initialized.
 * It is done by using one of tapi_job_factory_* create functions
 * (e.g. tapi_job_factory_rpc_create()). The job factory defines
 * the methods to create and control jobs on a specific Test Agent.
 *
 * When factory is created by tapi_job_factory_rpc_create(), the job
 * instances are created on the specified RPC server and are controlled
 * by the facility that is set up on the RPC server.
 *
 * When factory is created by tapi_job_factory_cfg_create(), the job
 * instances are controlled by "/agent/process" Configurator subtree.
 *
 * Job factory API allows tapi_job_create() API to be independent from
 * a particular implementation of job control. It makes any other
 * programming logic that is build on tapi_job functionality fully
 * decoupled from job control backend. To achieve this, e.g. an API
 * should not use tapi_job_factory* create, but consume already created
 * factory to create jobs inside the API. The user of such an API will
 * decide which job control implementation to choose.
 *
 * @page tapi-job-build Building tapi_job
 *
 * tarpc_job.x.m4 must be added to the list of rpcdefs
 * (along with other required rpcdefs) using
 * TE_LIB_PARAMS directive in builder configuration file:
 *
 * @code{.c}
 * TE_LIB_PARMS([rpcxdr], [], [],
 *              [--with-rpcdefs=tarpc_job.x.m4,someother.x.m4])
 * @endcode
 *
 * @page tapi-job-scenarios Job TAPI usage scenarios
 *
 * Run and wait
 * ------------
 * - Run a program
 * - Log all stdout and stderr via the Logger
 * - Wait for the program termination
 *
 * @code{.c}
 * tapi_job_factory_t *factory = NULL;
 * tapi_job_t *job = NULL
 * tapi_job_channel_t *out_channels[2];
 *
 * CHECK_RC(tapi_job_factory_rpc_create(pco, &factory));
 * CHECK_RC(tapi_job_create(factory, NULL, "/usr/bin/tool",
 *                          (const char *[]){"tool", "arg1", "arg2",
 *                          NULL}, NULL, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 2, out_channels));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[0],
 *                                                      out_channels[1]),
 *                                 "Filter", FALSE, TE_LL_INFO, NULL));
 * CHECK_RC(tapi_job_start(job));
 * ...
 * CHECK_RC(tapi_job_wait(job, -1, &status));
 * if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
 *    TEST_FAIL("Tool failed");
 * CHECK_RC(tapi_job_destroy(job, -1));
 * tapi_job_factory_destroy(factory);
 * @endcode
 *
 * Run and read
 * ------------
 * - Run a program
 * - Read all the stdout while logging all the stderr
 * - When all the data are read, the program is assumed to terminate
 *
 * @code{.c}
 * tapi_job_factory_t *factory = NULL;
 * tapi_job_t *job = NULL;
 * tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
 * tapi_job_channel_t *out_channels[2];
 * tapi_job_channel_t *out_filter;
 *
 * CHECK_RC(tapi_job_factory_rpc_create(pco, &factory));
 * CHECK_RC(tapi_job_create(factory, NULL, "/usr/bin/tool",
 *                          (const char *[]){"tool", "arg1", "arg2",
 *                          NULL}, NULL, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 2, out_channels));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[0]),
 *                                 "Out filter", TRUE, 0, &out_filter));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[1]),
 *                                 "Error filter", FALSE, TE_LL_INFO, NULL));
 * CHECK_RC(tapi_job_start(job));
 * while (!buf.eos)
 * {
 *     te_string_reset(&buf.data);
 *     CHECK_RC(tapi_job_receive(TAPI_JOB_CHANNEL_SET(out_filter), -1, &buf));
       ...
 * }
 * CHECK_RC(tapi_job_wait(job, 0, &status));
 * if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
 *    TEST_FAIL("Tool failed");
 * CHECK_RC(tapi_job_destroy(job, -1));
 * tapi_job_factory_destroy(factory);
 * @endcode
 *
 *
 * Run and wait for
 * ------------
 * - Run a program
 * - Log the stderr line matching `ERROR` as TE errors
 * - Wait for the string `Completed` at the stdout
 *
 * @code{.c}
 * tapi_job_factory_t *factory = NULL;
 * tapi_job_t *job = NULL;
 * tapi_job_channel_t *out_channels[2];
 * tapi_job_channel_t *out_filters[2];
 *
 * CHECK_RC(tapi_job_factory_rpc_create(pco, &factory));
 * CHECK_RC(tapi_job_create(factory, NULL, "/usr/bin/tool",
 *                          (const char *[]){"tool", "arg1", "arg2",
 *                          NULL}, NULL, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 2, out_channels));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[0]),
 *                                 "Out filter", TRUE, 0, &out_filters[0]));
 * CHECK_RC(tapi_job_filter_add_regexp(out_filters[0], "Completed", 0));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[1]),
 *                                 "Error filter", FALSE, TE_LL_ERROR,
 *                                 &out_filters[1]));
 * CHECK_RC(tapi_job_filter_add_regexp(out_filters[1],
 *                                    "^ERROR:(.*)$", 1);
 *
 * CHECK_RC(tapi_job_start(job));
 *
 * CHECK_RC(tapi_job_poll(TAPI_JOB_CHANNEL_SET(out_filters[0]), -1));
 *
 * CHECK_RC(tapi_job_wait(job, 0, &status));
 * if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
 *    TEST_FAIL("Tool failed");
 * CHECK_RC(tapi_job_destroy(job, -1));
 * tapi_job_factory_destroy(factory);
 * @endcode
 *
 * Run an interactive program
 * --------------------------
 *
 * - Run an interactive program via a pseudo-terminal
 * - Wait for the prompt
 * - Send a command
 * - Repeat the previous steps, logging all the output until the program
 *   terminates
 * @code{.c}
 * tapi_job_factory_t *factory = NULL;
 * tapi_job_t *job = NULL;
 * tapi_job_channel_t *in_channel;
 * tapi_job_channel_t *out_channel;
 * tapi_job_channel_t *prompt_filter;
 *
 * CHECK_RC(tapi_job_factory_rpc_create(pco, &factory));
 * CHECK_RC(tapi_job_create(factory, "pty_spawner", "/usr/bin/tool",
 *                          (const char *[]){"tool", "arg1", "arg2",
 *                          NULL}, NULL, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 1, &out_channel));
 * CHECK_RC(tapi_job_alloc_input_channels(job, 1, &in_channel));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channel),
 *                                 "Readable filter", TRUE, 0, &prompt_filter));
 * CHECK_RC(tapi_job_filter_add_regexp(prompt_filter, "^\\$ ", 0));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channel),
 *                                 "Log filter", FALSE, TE_LL_RING, NULL));
 *
 * CHECK_RC(tapi_job_start(job));
 *
 * while (tapi_job_wait(job, 0, &status) == TE_EINPROGRESS)
 * {
 *     CHECK_RC(tapi_job_receive(TAPI_JOB_CHANNEL_SET(prompt_filter), -1, NULL));
 *     CHECK_RC(tapi_job_send(job, in_channel, &command_string));
 * }
 *
 * if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
 *    TEST_FAIL("Tool failed");
 * CHECK_RC(tapi_job_destroy(job, -1));
 * tapi_job_factory_destroy(factory);
 * @endcode
 *
 * Terminate a job and if it hangs, forcedly kill it
 * -------------------------------------------------
 *
 * @code{.c}
 * ...
 * CHECK_RC(tapi_job_kill(job, SIGTERM));
 * if (tapi_job_wait(job, TIMEOUT, &status) == TE_INPROGRESS)
 * {
 *     CHECK_RC(tapi_job_kill(job, SIGKILL));
 *     CHECK_RC(tapi_job_wait(job, 0, &status));
 * }
 * ...
 * @endcode
 *
 * Start a job several times and get its data
 * ------------------------------------------
 *
 * @code{.c}
 * unsigned int eos_count;
 * tapi_job_buffer buf = TAPI_JOB_BUFFER_INIT;
 * tapi_job_factory_t *factory = NULL;
 * tapi_job_t *job = NULL;
 * tapi_job_channel_t *out_channel;
 * tapi_job_channel_t *out_filter;
 * tapi_job_channel_t *out_filter2;
 *
 * CHECK_RC(tapi_job_factory_rpc_create(pco, &factory));
 * CHECK_RC(tapi_job_create(factory, "pty_spawner", "/usr/bin/tool",
 *                          (const char *[]){"tool", "arg1", "arg2",
 *                          NULL}, NULL, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 1, &out_channel));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channel),
 *                                 "Readable filter", TRUE, 0, &out_filter));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channel),
 *                                 "Readable filter2", TRUE, 0, &out_filter2));
 *
 * CHECK_RC(tapi_job_start(job));
 * CHECK_RC(tapi_job_wait(job, BIG_TIMEOUT, NULL));
 *
 * // Get data from the first run from both filters
 * eos_count = 0;
 * while (eos_count < 2)
 * {
 *     te_string_reset(&buf.data);
 *     CHECK_RC(tapi_job_receive(TAPI_JOB_CHANNEL_SET(out_filter, out_filter2),
 *                               -1, &buf));
 *     if (buf.eos)
 *         eos_count++;
 *     ...
 * }
 *
 * CHECK_RC(tapi_job_start(job));
 * CHECK_RC(tapi_job_wait(job, BIG_TIMEOUT, NULL));
 *
 * // Get data from the second run from both filters
 * eos_count = 0;
 * while (eos_count < 2)
 * {
 *     te_string_reset(&buf.data);
 *     CHECK_RC(tapi_job_receive(TAPI_JOB_CHANNEL_SET(out_filter, out_filter2),
 *                               -1, &buf));
 *     if (buf.eos)
 *         eos_count++;
 *     ...
 * }
 *
 * CHECK_RC(tapi_job_destroy(job, -1));
 * tapi_job_factory_destroy(factory);
 * @endcode
 */
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_JOB_H__ */

/**@} <!-- END tapi_job --> */
