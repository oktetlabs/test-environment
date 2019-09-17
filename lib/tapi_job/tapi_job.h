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
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An opaque type to represent job instances
 */
typedef struct tapi_job_t tapi_job_t;

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
 * Create a job controlled by the RPC server @p rcps.
 * The job will be managed by @p spawner plugin.
 *
 * @note This is the only function in this API that depends
 * on the actual communication protocol used to control the agent
 * jobs, hence @c rpc in its name. All other functions should remain
 * intact, if the mechanism is ever changed.
 * @note The created job is *not* started automatically,
 *       use tapi_job_start() to actually run it.
 * @note The first element of @p args, by convention of exec family functions,
 *       should point to the filename associated with the file being executed.
 *
 * @param rpcs      RPC server
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
extern te_errno tapi_job_rpc_create(rcf_rpc_server *rpcs,
                                    const char *spawner,
                                    const char *program,
                                    const char **argv,
                                    const char **env,
                                    tapi_job_t **job);
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
 * Create a job on a given RPC served based on a description
 *
 * @param rpcs   RPC server
 * @param desc   Job description
 *
 * @return       Status code
 * @retval TE_EALREADY A job associated with @p desc already created
 */
extern te_errno tapi_job_rpc_simple_create(rcf_rpc_server *rpcs,
                                           tapi_job_simple_desc_t *desc);

/**
 * Set the path in Job TAPI environment to /agent:/env:PATH
 *
 * @note Job TAPI does not automatically inherit the environment from /agent:env
 *
 * @param rpcs   RPC server
 *
 * @return       Status code
 */
extern te_errno tapi_job_set_path(rcf_rpc_server *rpcs);

/**
 * Start a job
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
 * @param timeout_ms Timeout in ms (negative means tapi_job_get_timeout())
 * @param[out] status Exit statu
 *
 * @return                  Status code
 * @retval TE_EINPROGRESS   Job is still running
 */
extern te_errno tapi_job_wait(tapi_job_t *job, int timeout_ms,
                              tapi_job_status_t *status);

/**
 * Allocate @p n_channels input channels.
 * The first channel is expected to be connected to the job's stdin;
 * the wiring of others is spawner-dependant.
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
 * The first channel and the second output channels are expected to be
 * connected to stdout and stderr resp., the wiring of others is
 * spawner-dependant.
 *
 * @note The implementation need not support any other operations on
 *       primary output channels other than attaching filters to them
 *       (see tapi_job_attach_filter())
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
 * Attach a simple filter to a job created by tapi_job_simple_rpc_create()
 *
 * @param desc   Job description (it must have @a job_loc, @a stdin_loc and
 *               @a stderr_loc filled in)
 * @param filter Filter to attach
 *
 * @return       Status code
 * @retval TE_ENOTCONN tapi_job_simple_rpc_create() has not been called on
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
 * @return          Status code
 */
extern te_errno tapi_job_filter_add_regexp(tapi_job_channel_t *filter,
                                           const char *re,
                                           unsigned int extract);

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
 */
typedef struct tapi_job_buffer_t {
    tapi_job_channel_t *channel; /**< Last message channel */
    tapi_job_channel_t *filter; /**< Last message filter */
    unsigned int dropped; /**< Number of dropped messages */
    te_bool eos; /**< @c TRUE if the stream behind the filter has been closed */
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
 * @retval TE_ENODATA if there's no data available within @p timeout
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
 * Destroy the job instance. If the job has started, it is terminated
 * as gracefully as possible. All resources of the instance are freed;
 * all unread data on all filters are lost.
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
 * @page tapi-job-scenarios Job TAPI usage scenarios
 *
 * Run and wait
 * ------------
 * - Run a program
 * - Log all stdout and stderr via the Logger
 * - Wait for the program termination
 *
 * \code{.c}
 * tapi_job_t *job = NULL
 * tapi_job_channel_t *out_channels[2];
 *
 * CHECK_RC(tapi_job_rpc_create(pco, NULL, "/usr/bin/tool",
 *                              (const char *[]){"tool", "arg1", "arg2",
 *                              NULL}, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 2, NULL));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[0],
 *                                                      out_channels[1]),
 *                                 "Filter", FALSE, TE_LL_INFO, NULL));
 * CHECK_RC(tapi_job_start(job));
 * ...
 * CHECK_RC(tapi_job_wait(job, -1, &status));
 * if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
 *    TEST_FAIL("Tool failed");
 * CHECK_RC(tapi_job_destroy(job));
 * \endcode
 *
 * Run and read
 * ------------
 * - Run a program
 * - Read all the stdout while logging all the stderr
 * - When all the data are read, the program is assumed to terminate
 *
 * \code{.c}
 * tapi_job_t *job = NULL;
 * tapi_job_buffer buf = TAPI_JOB_BUFFER_INIT;
 * tapi_job_channel_t *out_channels[2];
 * tapi_job_channel_t *out_filter;
 *
 * CHECK_RC(tapi_job_rpc_create(pco, NULL, "/usr/bin/tool",
 *                              (const char *[]){"tool", "arg1", "arg2",
 *                              NULL}, &job));
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
 * CHECK_RC(tapi_job_destroy(job));
 * \endcode
 *
 *
 * Run and wait for
 * ------------
 * - Run a program
 * - Log the stderr line matching `ERROR` as TE errors
 * - Wait for the string `Completed` at the stdout
 *
 * \code{.c}
 * tapi_job_t *job = NULL;
 * tapi_job_channel_t *out_channels[2];
 * tapi_job_channel_t *out_filters[2];
 *
 * CHECK_RC(tapi_job_rpc_create(pco, NULL, "/usr/bin/tool",
 *                              (const char *[]){"tool", "arg1", "arg2",
 *                              NULL}, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 2, out_channels));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[0]),
 *                                 "Out filter", TRUE, 0, &out_filters[0]));
 * CHECK_RC(tapi_job_filter_add_regexp(out_filters[0], "Completed", 0));
 * CHECK_RC(tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_channels[1]),
 *                                 "Error filter", FALSE, TE_LL_ERROR,
 *                                 &out_filters[1]));
 * CHECK_RC(tapi_job_filrer_add_regexp(out_filters[1],
 *                                    "^ERROR:(.*)$", 1);
 *
 * CHECK_RC(tapi_job_start(job));
 *
 * CHECK_RC(tapi_job_poll(TAPI_JOB_CHANNEL_SET(out_filters[0]), -1));
 *
 * CHECK_RC(tapi_job_wait(job, 0, &status));
 * if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
 *    TEST_FAIL("Tool failed");
 * CHECK_RC(tapi_job_destroy(job));
 * \endcode
 *
 * Run an interactive program
 * --------------------------
 *
 * - Run an interactive program via a pseudo-terminal
 * - Wait for the prompt
 * - Send an command
 * - Repeat the previous steps, logging all the output until the program
 *   terminates
 * \code{.c}
 * tapi_job_t *job = NULL;
 * tapi_job_channel_t *in_channel;
 * tapi_job_channel_t *out_channel;
 * tapi_job_channel_t *prompt_filter;
 *
 * CHECK_RC(tapi_job_rpc_create(pco, "pty_spawner", "/usr/bin/tool",
 *                              (const char *[]){"tool", "arg1", "arg2",
 *                              NULL}, &job));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 1, &out_channel));
 * CHECK_RC(tapi_job_alloc_output_channels(job, 1, &in_channel));
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
 *     CHECK_RC(tapi_job_receive(TAPI_JOB_CHANNEL_SET(prompt_filter), - 1, NULL));
 *     CHECK_RC(tapi_job_send(job, in_channel, &command_string));
 * }
 *
 * if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
 *    TEST_FAIL("Tool failed");
 * CHECK_RC(tapi_job_destroy(job));
 * \endcode
 *
 * Terminate a job and if it hangs, forcedly kill it
 * -------------------------------------------------
 *
 * \code{.c}
 * ...
 * CHECK_RC(tapi_job_kill(job, SIGTERM));
 * if (tapi_job_wait(job, TIMEOUT, &status) == TE_INPROGRESS)
 * {
 *     CHECK_RC(tapi_job_kill(job, SIGKILL));
 *     CHECK_RC(tapi_job_wait(job, 0, &status));
 * }
 * ...
 * \endcode
 */
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_JOB_H__ */

/**@} <!-- END tapi_job --> */
