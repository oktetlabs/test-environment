/** @file
 *
 * Library to be used by backends for TAPI job.
 * The library provides types and functions that should be used by
 * TAPI job backends for agent job management.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TA JOB"

#include "te_config.h"

#include "ta_job.h"
#include "te_exec_child.h"
#include "te_queue.h"
#include "te_str.h"
#include "te_string.h"
#include "te_sleep.h"
#include "te_vector.h"
#include "te_file.h"
#include "agentlib.h"
#include "te_rpc_signal.h"
#include "logfork.h"

#if HAVE_PCRE_H
#include <pcre.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

/* The maximum size of log user entry (in bytes) for filter logging. */
#define MAX_LOG_USER_SIZE 128

#define PROC_WAIT_US 1000
/*
 * The amount of time needed for a process to terminate after receiving
 * a signal (that is meant to terminate the process) with default handler.
 */
#define KILL_TIMEOUT_MS 10

#define MAX_OUTPUT_CHANNELS_PER_JOB 32
#define MAX_INPUT_CHANNELS_PER_JOB 32
#define MAX_FILTERS_PER_CHANNEL 32
#define MAX_CHANNELS_AND_FILTERS_OVERALL UINT_MAX
#define MAX_JOBS UINT_MAX

#define MAX_QUEUE_SIZE (16 * 1024 * 1024)
#define MAX_MESSAGE_DATA_SIZE 8192

#define CTRL_PIPE_INITIALIZER {-1, -1}
#define CTRL_MESSAGE ("c\n")

typedef struct message_t {
    TAILQ_ENTRY(message_t) entry;

    char *data;
    size_t size;
    te_bool eos;
    unsigned int channel_id;
    unsigned int filter_id;
    size_t dropped;
} message_t;

typedef TAILQ_HEAD(message_list, message_t) message_list;

typedef struct message_queue_t {
    message_list messages;
    size_t dropped;
    size_t size;
} message_queue_t;

typedef enum queue_action_t {
    EXTRACT_FIRST,
    GET_LAST
} queue_action_t;

#define OVECCOUNT 30 /* should be a multiple of 3 */

typedef struct regexp_data_t {
    pcre *re;
    pcre_extra *sd;
    unsigned int extract;
    te_bool utf8;
    te_bool crlf_is_newline;
    int max_lookbehind;
} regexp_data_t;

typedef LIST_HEAD(filter_list, filter_t) filter_list;

typedef struct filter_t {
    LIST_ENTRY(filter_t) next;

    unsigned int id;

    /* String to which the next read segment should be appended  */
    te_string saved_string;
    int start_offset;
    te_bool line_begin;

    int ref_count;
    te_bool signal_on_data;
    te_bool readable;
    te_log_level log_level;
    char *name;
    message_queue_t queue;
    regexp_data_t *regexp_data;
} filter_t;

typedef LIST_HEAD(channel_list, channel_t) channel_list;

typedef struct channel_t {
    LIST_ENTRY(channel_t) next;

    unsigned int id;

    struct ta_job_t *job;
    te_bool closed;
    int fd;

    te_bool is_input_channel;

    union {
        /* Regarding output channel */
        struct {
            filter_t *filters[MAX_FILTERS_PER_CHANNEL];
            unsigned int n_filters;
        };
        /* Regarding input channel */
        struct {
            te_bool input_ready;
            te_bool signal_on_data;
        };
    };
} channel_t;

typedef LIST_HEAD(job_list, ta_job_t) job_list;

typedef struct wrapper_t {
    LIST_ENTRY(wrapper_t) next;

    unsigned int id;

    char *tool;
    char **argv;
    ta_job_wrapper_priority_t priority;
} wrapper_t;

typedef struct ta_job_t {
    LIST_ENTRY(ta_job_t) next;

    unsigned int id;

    channel_t *out_channels[MAX_OUTPUT_CHANNELS_PER_JOB];
    unsigned int n_out_channels;
    channel_t *in_channels[MAX_INPUT_CHANNELS_PER_JOB];
    unsigned int n_in_channels;

    pid_t pid;
    /* A job has started once */
    te_bool has_started;
    ta_job_status_t last_status;
    char *spawner;
    char *tool;
    char **argv;
    char **env;

    LIST_HEAD(wrapper_list, wrapper_t) wrappers;

    te_sched_param *sched_params;
} ta_job_t;

struct ta_job_manager_t {
    job_list all_jobs;
    channel_list all_channels;
    filter_list all_filters;
    /* Channels and filters exist in the same handler namespace */
    unsigned int channel_last_id;

    pthread_mutex_t channels_lock;
    pthread_cond_t data_cond;
    int ctrl_pipe[2];

    int abandoned_descriptors[FD_SETSIZE];
    size_t n_abandoned_descriptors;

    te_bool thread_is_running;
    pthread_t service_thread;
};

/** Default initializer for ta_job_manager_t structure */
static const ta_job_manager_t ta_job_manager_initializer = {
    .all_jobs = LIST_HEAD_INITIALIZER(all_jobs),
    .all_channels = LIST_HEAD_INITIALIZER(all_channels),
    .all_filters = LIST_HEAD_INITIALIZER(all_filters),
    .channels_lock = PTHREAD_MUTEX_INITIALIZER,
    .data_cond = PTHREAD_COND_INITIALIZER,
    .ctrl_pipe = CTRL_PIPE_INITIALIZER,
    .thread_is_running = FALSE,
};

/* See description in ta_job.h */
te_errno
ta_job_manager_init(ta_job_manager_t **manager)
{
    ta_job_manager_t *result = calloc(1, sizeof(ta_job_manager_t));

    if (result == NULL)
    {
        ERROR("Failed to allocate memory for job manager structure");
        return TE_ENOMEM;
    }

    *result = ta_job_manager_initializer;
    *manager = result;

    return 0;
}

#define DEF_GETTER(_name, _type, _head)                    \
static _type *                                             \
get_ ## _name(ta_job_manager_t *manager, unsigned int id)  \
{                                                          \
    _type *item;                                           \
                                                           \
    LIST_FOREACH(item, &manager->_head, next)              \
    {                                                      \
        if (item->id == id)                                \
            return item;                                   \
    }                                                      \
    ERROR(#_name " with id %d is not found", id);          \
    return NULL;                                           \
}                                                          \

DEF_GETTER(job, ta_job_t, all_jobs);
DEF_GETTER(channel, channel_t, all_channels);
DEF_GETTER(filter, filter_t, all_filters);

#undef DEF_GETTER

static void
get_channel_or_filter(ta_job_manager_t *manager, unsigned int id,
                      channel_t **channel, filter_t **filter)
{
    channel_t *c;
    filter_t *f;

    LIST_FOREACH(c, &manager->all_channels, next)
    {
        if (c->id == id)
        {
            *channel = c;
            return;
        }
    }

    LIST_FOREACH(f, &manager->all_filters, next)
    {
        if (f->id == id)
        {
            *filter = f;
            return;
        }
    }
    ERROR("Channel or filter with %d id is not found", id);
}

static void
regexp_data_destroy(regexp_data_t *regexp_data)
{
    if (regexp_data != NULL)
    {
        pcre_free_study(regexp_data->sd);
        pcre_free(regexp_data->re);
    }

    free(regexp_data);
}

static te_errno
regexp_data_create(char *pattern, unsigned int extract,
                   regexp_data_t **regexp_data)
{
    regexp_data_t *result = NULL;
    unsigned long int option_bits;
    int max_lookbehind;
    const char *error;
    te_errno rc = 0;
    int erroffset;
    te_bool utf8;

    if ((result = calloc(1, sizeof(*result))) == NULL)
    {
        ERROR("Failed to allocate regexp data");
        rc = TE_ENOMEM;
        goto out;
    }

    result->re = pcre_compile(pattern, PCRE_MULTILINE,
                              &error, &erroffset, NULL);
    if (result->re == NULL)
    {
        ERROR("PCRE compilation of pattern %s, failed at offset %d: %s",
              pattern, erroffset, error);
        rc = TE_EINVAL;
        goto out;
    }

    result->sd = pcre_study(result->re, 0, &error);
    if (error != NULL)
    {
        ERROR("PCRE study failed, %s", error);
        rc = TE_EPERM;
        goto out;
    }

    if (pcre_fullinfo(result->re, result->sd, PCRE_INFO_MAXLOOKBEHIND,
                      &max_lookbehind) != 0)
    {
        ERROR("PCRE fullinfo for max lookbehind failed");
        rc = TE_EPERM;
        goto out;
    }

    if (pcre_fullinfo(result->re, result->sd, PCRE_INFO_OPTIONS,
                      &option_bits) != 0)
    {
        ERROR("PCRE fullinfo failed");
        rc = TE_EPERM;
        goto out;
    }

    utf8 = (option_bits & PCRE_UTF8) != 0;

    option_bits &= PCRE_NEWLINE_CR | PCRE_NEWLINE_LF | PCRE_NEWLINE_CRLF |
                   PCRE_NEWLINE_ANY | PCRE_NEWLINE_ANYCRLF;

    if (option_bits == 0)
    {
        int d;

        if (pcre_config(PCRE_CONFIG_NEWLINE, &d) != 0)
        {
            ERROR("PCRE config failed");
            rc = TE_EINVAL;
            goto out;
        }
        /*
         * Values from specification. Note that these values are always
         * the ASCII ones, even in EBCDIC environments. CR = 13, NL = 10.
         */
        option_bits = (d == 13) ? PCRE_NEWLINE_CR :
                      (d == 10) ? PCRE_NEWLINE_LF :
                      (d == 3338) ? PCRE_NEWLINE_CRLF :
                      (d == -2) ? PCRE_NEWLINE_ANYCRLF :
                      (d == -1) ? PCRE_NEWLINE_ANY : 0;
    }

    result->crlf_is_newline = option_bits == PCRE_NEWLINE_ANY ||
                              option_bits == PCRE_NEWLINE_CRLF ||
                              option_bits == PCRE_NEWLINE_ANYCRLF;
    result->utf8 = utf8;
    result->extract = extract;
    result->max_lookbehind = max_lookbehind;

    *regexp_data = result;

out:
    if (rc != 0)
        regexp_data_destroy(result);

    return rc;
}

static te_errno
abandoned_descriptors_add(ta_job_manager_t *manager, size_t count, int *fds)
{
    size_t i;
    size_t *n_ab_descr = &manager->n_abandoned_descriptors;
    int *ab_descr = manager->abandoned_descriptors;

    if (*n_ab_descr + count > FD_SETSIZE)
    {
        ERROR("Failed to add abandoned descriptors: not enough space");
        return TE_ENOBUFS;
    }

    for (i = 0; i < count; i++)
        ab_descr[(*n_ab_descr)++] = fds[i];

    return 0;
}

static void
abandoned_descriptors_close(ta_job_manager_t *manager)
{
    size_t i;
    size_t *n_ab_descr = &manager->n_abandoned_descriptors;
    int *ab_descr = manager->abandoned_descriptors;

    for (i = 0; i < *n_ab_descr; i++)
        close(ab_descr[i]);

    *n_ab_descr = 0;
}

static te_errno
ctrl_pipe_create(ta_job_manager_t *manager)
{
    int rc;

    if (manager->ctrl_pipe[0] > -1 || manager->ctrl_pipe[1] > -1)
    {
        WARN("Control pipe already created");
        return 0;
    }

    if ((rc = pipe2(manager->ctrl_pipe, O_CLOEXEC)) != 0)
    {
        ERROR("Control pipe creation failure");
        return te_rc_os2te(rc);
    }

    return 0;
}

static void
ctrl_pipe_destroy(ta_job_manager_t *manager)
{
    if (manager->ctrl_pipe[0] > -1)
        close(manager->ctrl_pipe[0]);
    if (manager->ctrl_pipe[1] > -1)
        close(manager->ctrl_pipe[1]);

    manager->ctrl_pipe[0] = -1;
    manager->ctrl_pipe[1] = -1;
}

static int
ctrl_pipe_get_read_fd(ta_job_manager_t *manager)
{
    return manager->ctrl_pipe[0];
}

static te_errno
ctrl_pipe_send(ta_job_manager_t *manager)
{
    ssize_t write_rc;

    write_rc = write(manager->ctrl_pipe[1], CTRL_MESSAGE, sizeof(CTRL_MESSAGE));
    if (write_rc < 0)
    {
        ERROR("Control pipe write failed");
        return te_rc_os2te(errno);
    }

    if (write_rc == 0)
    {
        ERROR("Control pipe write sent 0 bytes");
        return TE_EIO;
    }

    return 0;
}

static te_errno
init_message_queue(message_queue_t *queue)
{
    queue->size = 0;
    queue->dropped = 0;
    TAILQ_INIT(&queue->messages);

    return 0;
}

static te_bool
queue_drop_oldest(message_queue_t *queue)
{
    message_t *msg = TAILQ_FIRST(&queue->messages);

    if (msg != NULL)
    {
        TAILQ_REMOVE(&queue->messages, msg, entry);

        queue->size -= (msg->size + sizeof(*msg));
        free(msg->data);
        free(msg);

        return TRUE;
    }

    return FALSE;
}

static te_errno
queue_put(const char *buf, size_t size, te_bool eos, unsigned int channel_id,
          unsigned int filter_id, message_queue_t *queue)
{
    message_t *msg;
    char *msg_str;
    size_t needed_space = size + sizeof(*msg);

    /*
     * Assert that queue size is sufficient to hold at least 1 message
     * of maximum size
     */
    TE_COMPILE_TIME_ASSERT(MAX_QUEUE_SIZE >=
                           MAX_MESSAGE_DATA_SIZE + sizeof(message_t));

    while (queue->size + needed_space > MAX_QUEUE_SIZE)
    {
        (void)queue_drop_oldest(queue);
        queue->dropped++;
    }

    msg = calloc(1, sizeof(*msg));

    if (msg == NULL)
    {
        ERROR("Message allocation failed");
        return TE_ENOMEM;
    }

    if (size != 0)
    {
        msg_str = calloc(1, size);
        if (msg_str == NULL)
        {
            ERROR("Message databuf allocation failed");
            free(msg);
            return TE_ENOMEM;
        }
    }
    else
    {
        msg_str = NULL;
    }

    memcpy(msg_str, buf, size);
    msg->data = msg_str;
    msg->size = size;
    msg->channel_id = channel_id;
    msg->filter_id = filter_id;
    msg->eos = eos;

    TAILQ_INSERT_TAIL(&queue->messages, msg, entry);

    queue->size += needed_space;

    return 0;
}

static te_bool
queue_has_data(message_queue_t *queue)
{
    return !TAILQ_EMPTY(&queue->messages);
}

static message_t *
queue_extract_first_msg(message_queue_t *queue)
{
    message_t *msg = TAILQ_FIRST(&queue->messages);

    if (msg != NULL)
    {
        msg->dropped = queue->dropped;
        queue->dropped = 0;
        TAILQ_REMOVE(&queue->messages, msg, entry);
    }

    return msg;
}

static message_t *
queue_get_last_msg(message_queue_t *queue)
{
    message_t *msg = TAILQ_LAST(&queue->messages, message_list);
    message_t *msg_prev;

    if (msg != NULL)
    {
        msg_prev = TAILQ_PREV(msg, message_list, entry);
        /* If the last msg indicates eos, try to get the second-to-last one */
        if (msg->eos && msg_prev != NULL)
            msg = msg_prev;
    }

    return msg;
}

static te_errno
channel_add(ta_job_manager_t *manager, channel_t *channel)
{
    unsigned int created_channel_id;

    if (manager->channel_last_id == MAX_CHANNELS_AND_FILTERS_OVERALL)
    {
        ERROR("Maximum channel id has been reached");
        return TE_EFAIL;
    }

    created_channel_id = manager->channel_last_id;

    channel->id = created_channel_id;

    LIST_INSERT_HEAD(&manager->all_channels, channel, next);
    manager->channel_last_id++;

    return 0;
}

static void
channel_remove(channel_t *channel)
{
    LIST_REMOVE(channel, next);
}

static te_errno
filter_add(ta_job_manager_t *manager, filter_t *filter)
{
    unsigned int created_filter_id;

    if (manager->channel_last_id == MAX_CHANNELS_AND_FILTERS_OVERALL)
    {
        ERROR("Maximum filter id has been reached");
        return TE_EFAIL;
    }

    created_filter_id = manager->channel_last_id;

    filter->id = created_filter_id;

    LIST_INSERT_HEAD(&manager->all_filters, filter, next);
    manager->channel_last_id++;

    return 0;
}

static void
channel_add_filter(channel_t *channel, filter_t *filter)
{
    channel->filters[channel->n_filters] = filter;
    channel->n_filters++;
    filter->ref_count++;
}

static te_errno
channel_alloc(ta_job_t *job, channel_t **channel, te_bool is_input_channel)
{
    channel_t *result = calloc(1, sizeof(*result));

    if (result == NULL)
    {
        ERROR("Channel allocation failed");
        return TE_ENOMEM;
    }

    result->n_filters = 0;
    result->job = job;
    result->fd = -1;
    result->closed = FALSE;
    result->is_input_channel = is_input_channel;
    result->input_ready = FALSE;
    result->signal_on_data = FALSE;

    *channel = result;

    return 0;
}

static te_errno
filter_alloc(const char *filter_name, te_bool readable, te_log_level log_level,
             filter_t **filter)
{
    filter_t *result = calloc(1, sizeof(*result));
    te_errno rc;

    if (result == NULL)
    {
        ERROR("Filter allocation failed");
        return TE_ENOMEM;
    }

    if ((rc = init_message_queue(&result->queue)) != 0)
    {
        ERROR("Message queue init failure");
        free(result);
        return rc;
    }

    if (filter_name != NULL)
    {
        result->name = strdup(filter_name);
        if (result->name == NULL)
        {
            free(result);
            ERROR("Filter name duplication failed");
        }
    }
    result->saved_string = (te_string)TE_STRING_INIT;
    result->start_offset = 0;
    result->line_begin = TRUE;
    result->ref_count = 0;
    result->signal_on_data = FALSE;
    result->readable = readable;
    result->log_level = log_level;
    result->regexp_data = NULL;

    *filter = result;

    return 0;
}

static void
queue_destroy(message_queue_t *queue)
{
    while (queue_drop_oldest(queue))
        ; /* Do nothing */
}

static void
filter_destroy(filter_t *filter)
{
    filter->ref_count--;
    if (filter->ref_count > 0)
        return;

    LIST_REMOVE(filter, next);

    te_string_free(&filter->saved_string);
    queue_destroy(&filter->queue);
    regexp_data_destroy(filter->regexp_data);

    free(filter);
}

static void
channel_destroy(channel_t *channel)
{
    unsigned int i;

    channel_remove(channel);

    for (i = 0; i < channel->n_filters; i++)
        filter_destroy(channel->filters[i]);

    free(channel);
}

static te_errno
ta_job_add(ta_job_manager_t *manager, ta_job_t *job)
{
    static unsigned int job_last_id = 0;
    unsigned int created_job_id;

    if (job_last_id == MAX_JOBS)
    {
        ERROR("Maximum job id has been reached");
        return TE_EFAIL;
    }

    created_job_id = job_last_id;

    job->id = created_job_id;

    LIST_INSERT_HEAD(&manager->all_jobs, job, next);
    job_last_id++;

    return 0;
}

/* Note: argv and env MUST remain valid after a successful call */
static te_errno
ta_job_alloc(const char *spawner, const char *tool, char **argv,
          char **env, ta_job_t **job)
{
    ta_job_t *result = calloc(1, sizeof(*result));

    if (result == NULL)
    {
        ERROR("Job allocation failed");
        return TE_ENOMEM;
    }

    if (spawner != NULL && spawner[0] != '\0')
    {
        WARN("Job's spawner is ignored");
        result->spawner = strdup(spawner);
        if (result->spawner == NULL)
        {
            ERROR("Job spawner allocaion failed");
            free(result);
            return TE_ENOMEM;
        }
    }

    if (tool == NULL)
    {
        ERROR("Failed to allocate a job: path to a tool is not specified");
        free(result->spawner);
        free(result);
        return TE_ENOENT;
    }

    result->tool = strdup(tool);
    if (result->tool == NULL)
    {
        ERROR("Job tool allocaion failed");
        free(result->spawner);
        free(result);
        return TE_ENOMEM;
    }

    result->has_started = FALSE;
    result->pid = (pid_t)-1;
    result->argv = argv;
    result->env = env;
    result->n_out_channels = 0;
    result->n_in_channels = 0;
    LIST_INIT(&result->wrappers);
    result->sched_params = NULL;

    *job = result;

    return 0;
}

static void
ta_job_dealloc(ta_job_t *job)
{
    te_str_free_array(job->argv);
    te_str_free_array(job->env);

    free(job->spawner);
    free(job->tool);
    if (job->sched_params != NULL)
    {
        te_sched_param *item;

        for (item = job->sched_params; item->type != TE_SCHED_END; item++)
            free(item->data);
        free(job->sched_params);
    }
    free(job);
}

/* See description in ta_job.h */
te_errno
ta_job_create(ta_job_manager_t *manager, const char *spawner, const char *tool,
              char **argv, char **env, unsigned int *job_id)
{
    te_errno rc;
    ta_job_t *job;

    if ((rc = ta_job_alloc(spawner, tool, argv, env, &job)) != 0)
        return rc;

    if ((rc = ta_job_add(manager, job)) != 0)
    {
        /* Do not own argv and env on failure */
        job->argv = NULL;
        job->env = NULL;
        ta_job_dealloc(job);

        return rc;
    }

    *job_id = job->id;

    return 0;
}

static te_errno
proc_wait(pid_t pid, int timeout_ms, ta_job_status_t *status)
{
    struct timeval tv_start;
    struct timeval tv_now;
    int wstatus;
    int pid_rc;

    if (timeout_ms >= 0)
    {
        gettimeofday(&tv_start, NULL);
        while ((pid_rc = ta_waitpid(pid, &wstatus, WNOHANG)) == 0)
        {
            gettimeofday(&tv_now, NULL);
            if (TE_US2MS(TIMEVAL_SUB(tv_now, tv_start)) > timeout_ms)
                return TE_EINPROGRESS;

            usleep(PROC_WAIT_US);
        }
    }
    else
    {
        pid_rc = ta_waitpid(pid, &wstatus, 0);
    }

    if (pid_rc != pid)
    {
        te_errno rc = te_rc_os2te(errno);

        ERROR("waitpid() call returned unexpected value %d, errno %s", pid_rc,
              te_rc_err2str(rc));
        return rc;
    }

    if (status == NULL)
        return 0;

    if (WIFEXITED(wstatus))
    {
        status->type = TA_JOB_STATUS_EXITED;
        status->value = WEXITSTATUS(wstatus);
    }
    else if (WIFSIGNALED(wstatus))
    {
        status->type = TA_JOB_STATUS_SIGNALED;
        status->value = WTERMSIG(wstatus);
    }
    else
    {
        WARN("Child process with PID %d exited due to unknown reason", pid);
        status->type = TA_JOB_STATUS_UNKNOWN;
    }

    return 0;
}

static te_errno
proc_kill(pid_t pid, int signo, int term_timeout_ms)
{
    te_errno rc;
    int timeout = term_timeout_ms >= 0 ? term_timeout_ms : KILL_TIMEOUT_MS;

    /* Don't consider kill() failure if the call failed with ESRCH */
    if (kill(pid, signo) < 0 && errno != ESRCH)
    {
        ERROR("Process kill(%d, %s) failed: %s", pid,
              signum_rpc2str(signum_h2rpc(signo)), strerror(errno));

        return te_rc_os2te(errno);
    }

    /*
     * Wait for process for requested amount of time (or default)
     * before killing the process with SIGKILL
     */
    if ((rc = proc_wait(pid, timeout, NULL)) != 0 && signo != SIGKILL)
    {
        WARN("Failed to wait for killed process");

        if (kill(pid, SIGKILL) < 0 && errno != ESRCH)
        {
            ERROR("Process kill(%d, SIGKILL) failed: %s", pid, strerror(errno));
            return te_rc_os2te(errno);
        }

        /*
         * Since SIGKILL cannot be intercepted, wait for process for
         * default amout of time
         */
        if (proc_wait(pid, KILL_TIMEOUT_MS, NULL) != 0)
        {
            ERROR("Failed to wait for killed process");
            return te_rc_os2te(errno);
        }
    }

    return rc;
}

static te_errno
match_callback(ta_job_manager_t *manager, filter_t *filter,
               unsigned int channel_id, pid_t pid, size_t size,
               char *buf, te_bool eos)
{
    int log_size = size > (size_t)INT_MAX ? INT_MAX : (int)size;
    te_errno rc;

    if (!eos)
    {
        char log_user[MAX_LOG_USER_SIZE];

        rc = snprintf(log_user, sizeof(log_user), "%ld.%s", (long)pid,
                      filter->name == NULL ? "Unnamed" : filter->name);
        if (rc < 0 || (size_t)rc >= sizeof(log_user))
        {
            ERROR("Failed to create log user for logging a job's message");
            return TE_EINVAL;
        }

        /*
         * TODO: ta_log_message() does not support "%.*s" now (Bug 44).
         * After it is supported, the following code block should be used
         * instead of the next one.
         */
#if 0
        LGR_MESSAGE(filter->log_level, log_user, "%.*s", log_size, buf);
#else
        te_string message = TE_STRING_INIT;

        rc = te_string_append(&message, "%.*s", log_size, buf);
        if (rc != 0)
            return rc;

        LGR_MESSAGE(filter->log_level, log_user, "%s", message.ptr);

        te_string_free(&message);
#endif
    }

    if (filter->readable)
    {
        rc = queue_put(buf, size, eos, channel_id, filter->id, &filter->queue);
        if (rc != 0)
            return rc;
    }

    if (filter->signal_on_data)
        pthread_cond_signal(&manager->data_cond);

    return 0;
}

static te_errno
filter_regexp_exec(ta_job_manager_t *manager, filter_t *filter,
                   unsigned int channel_id, pid_t pid, int segment_length,
                   char *segment, te_bool eos,
                   te_errno (*fun_ptr)(ta_job_manager_t *, filter_t *,
                                       unsigned int, pid_t,
                                       size_t, char *, te_bool))
{
    int start_offset;
    te_bool first_exec;
    int ovector[OVECCOUNT];
    int rc;
    te_errno te_rc;
    regexp_data_t *regexp = filter->regexp_data;
    char *subject;
    int subject_length;
    int cut;
    /*
     * Position in the current subject from which to start matching
     * in the next iteration
     */
    int future_start_offset;

    te_rc = te_string_append(&filter->saved_string, "%.*s",
                             segment_length, segment);
    if (te_rc != 0)
    {
        ERROR("Failed to append segment to filter's saved string");
        goto out;
    }
    subject = filter->saved_string.ptr;
    subject_length = filter->saved_string.len;
    future_start_offset = subject_length;

    for (start_offset = filter->start_offset, first_exec = TRUE; ;
         start_offset = ovector[1], first_exec = FALSE)
    {
        char *substring_start;
        size_t substring_length;
        int options = eos ? 0 : PCRE_PARTIAL_HARD; /* Enable partial matching */

        if (!filter->line_begin)
            options |= PCRE_NOTBOL;

        if (!first_exec && ovector[0] == ovector[1])
        {
            if (ovector[0] == subject_length)
                break;
            options |= PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
        }

        rc = pcre_exec(regexp->re, regexp->sd, subject, subject_length,
                       start_offset, options, ovector, TE_ARRAY_LEN(ovector));
        if (rc < 0)
        {
            switch (rc)
            {
            case PCRE_ERROR_NOMATCH:

                if (first_exec)
                {
                    /* No match at all */
                    te_rc = 0;
                    goto out;
                }
                else
                {
                    /*
                     * All matches found. Options are checked to prevent
                     * infinite zero-length matches
                     */
                    if ((options & PCRE_NOTEMPTY_ATSTART) == 0)
                    {
                        te_rc = 0;
                        goto out;
                    }

                    /* Advance one byte */
                    ovector[1] = start_offset + 1;

                    /* If CRLF is newline & we are at CRLF, */
                    if (regexp->crlf_is_newline &&
                        start_offset < subject_length - 1 &&
                        subject[start_offset] == '\r' &&
                        subject[start_offset + 1] == '\n')
                    {
                        /* Advance by one more. */
                        ovector[1] += 1;
                    }
                    else if (regexp->utf8)
                    {
                        /* Advance whole UTF-8 character */
                        while (ovector[1] < subject_length)
                        {
                            /*
                             * Break if byte is the first byte
                             * of UTF-8 character
                             */
                            if ((subject[ovector[1]] & 0xc0) != 0x80)
                                break;

                            ovector[1] += 1;
                        }
                    }
                    continue;
                }

            case PCRE_ERROR_PARTIAL:
                future_start_offset = ovector[2];
                te_rc = 0;
                goto out;

            default:
                ERROR("Matching error %d", rc);
                te_rc = TE_EFAULT;
                goto out;
            }
        }

        /* The match succeeded, but the output vector wasn't big enough. */
        if (rc == 0)
        {
            rc = TE_ARRAY_LEN(ovector) / 3;
            WARN("ovector only has room for %d matches", rc);
        }

        if (regexp->extract >= (unsigned int)rc)
        {
            ERROR("There is no match with number %u", regexp->extract);
            te_rc = TE_EPERM;
            goto out;
        }

        substring_start = subject + ovector[2 * regexp->extract];
        substring_length = ovector[(2 * regexp->extract) + 1] -
                               ovector[2 * regexp->extract];

        if ((te_rc = fun_ptr(manager, filter, channel_id, pid, substring_length,
                             substring_start, FALSE)) != 0)
        {
            goto out;
        }
    }

out:
    if (te_rc != 0 || eos)
    {
        te_string_free(&filter->saved_string);
        filter->start_offset = 0;
        filter->line_begin = TRUE;

        return te_rc;
    }

    cut = MAX(future_start_offset - regexp->max_lookbehind, 0);
    filter->start_offset = future_start_offset - cut;
    if (cut != 0)
    {
        filter->line_begin = (subject[cut - 1] == '\n');
        te_string_cut_beginning(&filter->saved_string, cut);
    }

    return 0;
}

static te_errno
filter_exec(ta_job_manager_t *manager, filter_t *filter,
            unsigned int channel_id, pid_t pid, size_t size, char *buf)
{
    te_bool eos = (size == 0);
    te_errno rc;

    if (filter->regexp_data != NULL)
    {
        rc = filter_regexp_exec(manager, filter, channel_id, pid, size,
                                buf, eos, match_callback);
        /*
         * Call match_callback() separately for eos-message since
         * filter_regexp_exec() does not do that.
         */
        if (rc != 0 || !eos)
            return rc;
    }

    return match_callback(manager, filter, channel_id, pid, size, buf, eos);
}

static te_errno
channel_read(ta_job_manager_t *manager, channel_t *channel)
{
    char buf[MAX_MESSAGE_DATA_SIZE] = {0};
    ssize_t read_c;
    size_t i;
    te_errno rc;

    read_c = read(channel->fd, buf, sizeof(buf));
    if (read_c < 0)
        return te_rc_os2te(errno);

    for (i = 0; i < channel->n_filters; i++)
    {
        if ((rc = filter_exec(manager, channel->filters[i], channel->id,
                              channel->job->pid, read_c, buf)) != 0)
            return rc;
    }

    if (read_c == 0)
        channel->closed = TRUE;

    return 0;
}

static void
thread_destroy_unused_channels(channel_list *channels)
{
    channel_t *channel;
    channel_t *channel_tmp;

    LIST_FOREACH_SAFE(channel, channels, next, channel_tmp)
    {
        if (channel->job == NULL)
        {
            if (channel->fd > -1)
                close(channel->fd);
            channel_destroy(channel);
        }
    }
}

static void
thread_read_selected(ta_job_manager_t *manager, fd_set *rfds,
                     channel_list *channels)
{
    channel_t *active_channel = NULL;
    channel_t *channel;
    te_errno rc;

    LIST_FOREACH(channel, channels, next)
    {
        if (FD_ISSET(channel->fd, rfds))
        {
            active_channel = channel;
            break;
        }
    }

    if (active_channel == NULL)
    {
        INFO("Drop data on destroyed channel\n");
        return;
    }

    if ((rc = channel_read(manager, active_channel)) != 0)
    {
        WARN("Channel read failure '%r', continuing", rc);
    }
}

static void
thread_mark_selected_ready(ta_job_manager_t *manager, fd_set *wfds,
                           channel_list *channels)
{
    channel_t *channel;

    LIST_FOREACH(channel, channels, next)
    {
        if (FD_ISSET(channel->fd, wfds))
        {
            channel->input_ready = TRUE;
            if (channel->signal_on_data)
                pthread_cond_signal(&manager->data_cond);
        }
    }
}

static void *
thread_work_loop(void *arg)
{
    ta_job_manager_t *manager = arg;

    fd_set rfds;
    fd_set wfds;
    int select_rc;

    UNUSED(arg);

    logfork_register_user("JOB CONTROL");
    logfork_set_id_logging(FALSE);

    while (1)
    {
        int ctrl_fd = ctrl_pipe_get_read_fd(manager);
        int max_fd = ctrl_fd;
        channel_t *channel;

        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(ctrl_fd, &rfds);

        pthread_mutex_lock(&manager->channels_lock);

        LIST_FOREACH(channel, &manager->all_channels, next)
        {
            if (!channel->closed && channel->fd > -1)
            {
                if (channel->is_input_channel)
                {
                    if (!channel->input_ready)
                        FD_SET(channel->fd, &wfds);
                }
                else
                {
                    FD_SET(channel->fd, &rfds);
                }

                if (channel->fd > max_fd)
                    max_fd = channel->fd;
            }
        }

        pthread_mutex_unlock(&manager->channels_lock);

        select_rc = select(max_fd + 1, &rfds, &wfds, NULL, NULL);

        if (select_rc < 0)
        {
            switch (errno)
            {
                case EINTR:
                    /* A failure due to signal is ignored */
                    break;
                default:
                    ERROR("select() failed, %s", strerror(errno));
                    break;
            }
        }
        else if (select_rc)
        {
            te_bool channel_process_needed = TRUE;

            pthread_mutex_lock(&manager->channels_lock);

            if (FD_ISSET(ctrl_fd, &rfds))
            {
                char buf[sizeof(CTRL_MESSAGE)] = {0};
                ssize_t read_c = read(ctrl_fd, buf, sizeof(buf));

                if (read_c <= 0)
                    WARN("Control pipe read failed, continuing");

                channel_process_needed = FALSE;
            }

            thread_destroy_unused_channels(&manager->all_channels);
            abandoned_descriptors_close(manager);

            if (channel_process_needed)
            {
                thread_read_selected(manager, &rfds, &manager->all_channels);
                thread_mark_selected_ready(manager, &wfds,
                                           &manager->all_channels);
            }

            pthread_mutex_unlock(&manager->channels_lock);
        }
    }

    return NULL;
}

static te_errno
thread_start(ta_job_manager_t *manager)
{
    te_errno rc;

    if ((rc = ctrl_pipe_create(manager)) != 0)
        return rc;

    /* Check read end of the pipe for descriptor limit */
    if (ctrl_pipe_get_read_fd(manager) >= FD_SETSIZE)
    {
        ERROR("Failed to create control pipe, file descriptor limit exceeded");
        ctrl_pipe_destroy(manager);
        return TE_EMFILE;
    }

    if (pthread_create(&manager->service_thread, NULL,
                       thread_work_loop, manager) != 0)
    {
        ERROR("Thread create failure");
        ctrl_pipe_destroy(manager);
        return te_rc_os2te(errno);
    }

    return 0;
}

static void
close_valid(int fd)
{
    if (fd >= 0)
        close(fd);
}

static te_errno
ta_job_build_tool_and_args(char **tool, te_vec *args, ta_job_t *job)
{
    char *end_arg = NULL;
    te_errno rc;
    size_t j;
    wrapper_t *wrap;
    ta_job_wrapper_priority_t i;

    if (!LIST_EMPTY(&job->wrappers))
    {
        for (i = TA_JOB_WRAPPER_PRIORITY_MAX - 1;
             i > TA_JOB_WRAPPER_PRIORITY_MIN; i--)
        {
            LIST_FOREACH(wrap, &job->wrappers, next)
            {
                if (wrap->priority != i)
                    continue;
                if (*tool == NULL)
                {
                    *tool = wrap->tool;
                }

                rc = te_vec_append_str_fmt(args, "%s", wrap->tool);
                if (rc != 0)
                    return rc;

                for (j = 1; wrap->argv != NULL && wrap->argv[j] != NULL; j++)
                {
                    rc = te_vec_append_str_fmt(args, "%s", wrap->argv[j]);
                    if (rc != 0)
                        return rc;
                }
            }
        }
    }
    else
    {
        *tool = job->tool;
    }

    rc = te_vec_append_str_fmt(args, "%s", job->tool);
    if (rc != 0)
        return rc;

    for (j = 1; job->argv != NULL && job->argv[j] != NULL; j++)
    {
        rc = te_vec_append_str_fmt(args, "%s", job->argv[j]);
        if (rc != 0)
            return rc;
    }

    return TE_VEC_APPEND_RVALUE(args, char *, end_arg);
}

/* See description in ta_job.h */
te_errno
ta_job_start(ta_job_manager_t *manager, unsigned int id)
{
    te_errno rc;
    pid_t pid;
    ta_job_t *job;
    int stdout_fd = -1;
    int stderr_fd = -1;
    int stdin_fd = -1;
    int *stdout_fd_p = &stdout_fd;
    int *stderr_fd_p = &stderr_fd;
    int *stdin_fd_p = &stdin_fd;
    int descrs_to_abandon[MAX_OUTPUT_CHANNELS_PER_JOB +
                          MAX_INPUT_CHANNELS_PER_JOB];
    size_t n_descrs_to_abandon = 0;
    size_t i;
    char *tool = NULL;
    te_vec args = TE_VEC_INIT(char *);

    job = get_job(manager, id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != (pid_t)-1)
    {
        ERROR("Job is already started");
        return TE_EPERM;
    }

    if (!manager->thread_is_running)
    {
        if ((rc = thread_start(manager)) != 0)
            return rc;

        manager->thread_is_running = TRUE;
    }

    if (job->n_out_channels < 2)
        stderr_fd_p = TE_EXEC_CHILD_DEV_NULL_FD;
    if (job->n_out_channels < 1)
        stdout_fd_p = TE_EXEC_CHILD_DEV_NULL_FD;
    if (job->n_in_channels < 1)
        stdin_fd_p = TE_EXEC_CHILD_DEV_NULL_FD;

    rc = ta_job_build_tool_and_args(&tool, &args, job);
    if (rc != 0)
    {
        ERROR("Failed to build command line, rc = %r", rc);
        te_vec_deep_free(&args);
        return rc;
    }

    // TODO: use spawner method
    pid = te_exec_child(tool, (char **)args.data.ptr, job->env,
                        -1, stdin_fd_p, stdout_fd_p, stderr_fd_p,
                        job->sched_params);
    te_vec_deep_free(&args);
    if (pid < 0)
    {
        ERROR("Exec child failure\n");
        return te_rc_os2te(errno);
    }

    if (stdout_fd >= FD_SETSIZE || stderr_fd >= FD_SETSIZE ||
        stdin_fd >= FD_SETSIZE)
    {
        ERROR("Failed to start a job, file descriptor limit exceeded");
        close_valid(stdout_fd);
        close_valid(stderr_fd);
        close_valid(stdin_fd);
        proc_kill(pid, SIGTERM, -1);
        return TE_EMFILE;
    }

    pthread_mutex_lock(&manager->channels_lock);

    if ((rc = ctrl_pipe_send(manager)) != 0)
    {
        pthread_mutex_unlock(&manager->channels_lock);
        close_valid(stdout_fd);
        close_valid(stderr_fd);
        close_valid(stdin_fd);
        proc_kill(pid, SIGTERM, -1);
        return rc;
    }

    for (i = 0; i < job->n_out_channels; i++)
    {
        if (job->out_channels[i]->fd > -1)
            descrs_to_abandon[n_descrs_to_abandon++] = job->out_channels[i]->fd;
    }
    for (i = 0; i < job->n_in_channels; i++)
    {
        if (job->in_channels[i]->fd > -1)
            descrs_to_abandon[n_descrs_to_abandon++] = job->in_channels[i]->fd;
    }

    if (n_descrs_to_abandon > 0)
    {
        rc = abandoned_descriptors_add(manager, n_descrs_to_abandon,
                                       descrs_to_abandon);
        if (rc != 0)
        {
            pthread_mutex_unlock(&manager->channels_lock);
            close_valid(stdout_fd);
            close_valid(stderr_fd);
            close_valid(stdin_fd);
            proc_kill(pid, SIGTERM, -1);
            return rc;
        }
    }

    if (job->n_out_channels > 0)
    {
        job->out_channels[0]->fd = stdout_fd;
        job->out_channels[0]->closed = FALSE;
    }
    if (job->n_out_channels > 1)
    {
        job->out_channels[1]->fd = stderr_fd;
        job->out_channels[1]->closed = FALSE;
    }
    if (job->n_in_channels > 0)
    {
        job->in_channels[0]->fd = stdin_fd;
        job->in_channels[0]->closed = FALSE;
    }

    pthread_mutex_unlock(&manager->channels_lock);

    job->has_started = TRUE;
    job->pid = pid;

    return 0;
}

static te_errno
ta_job_add_allocated_channels(ta_job_manager_t *manager, ta_job_t *job,
                              channel_t **allocated_channels,
                              unsigned int n_channels, te_bool input_channels)
{
    te_errno rc;
    unsigned int i;
    unsigned int j;

    pthread_mutex_lock(&manager->channels_lock);

    for (i = 0; i < n_channels; i++)
    {
        if ((rc = channel_add(manager, allocated_channels[i])) != 0)
        {
            pthread_mutex_unlock(&manager->channels_lock);

            for (j = 0; j < i; j++)
                channel_remove(allocated_channels[j]);

            return rc;
        }
    }

    pthread_mutex_unlock(&manager->channels_lock);

    for (i = 0; i < n_channels; i++)
    {
        if (input_channels)
            job->in_channels[i] = allocated_channels[i];
        else
            job->out_channels[i] = allocated_channels[i];
    }

    if (input_channels)
        job->n_in_channels = n_channels;
    else
        job->n_out_channels = n_channels;

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_allocate_channels(ta_job_manager_t *manager, unsigned int job_id,
                         te_bool input_channels, unsigned int n_channels,
                         unsigned int *channels)
{
    channel_t **allocated_channels = NULL;
    te_errno rc = 0;
    unsigned int i;
    ta_job_t *job;

    if ((job = get_job(manager, job_id)) == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    if ((input_channels && n_channels > MAX_INPUT_CHANNELS_PER_JOB) ||
        (!input_channels && n_channels > MAX_OUTPUT_CHANNELS_PER_JOB))
    {
        ERROR("Failed to allocate %u channels for a job, limit exceeded",
              n_channels);
        rc = TE_EINVAL;
        goto out;
    }

    if ((input_channels && job->n_in_channels > 0) ||
        (!input_channels && job->n_out_channels > 0))
    {
        ERROR("Failed to allocate channels: already allocated");
        rc = TE_EPERM;
        goto out;
    }

    allocated_channels = calloc(n_channels, sizeof(*allocated_channels));
    if (allocated_channels == NULL)
    {
        ERROR("Failed to allocate channels array for a job");
        rc = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < n_channels; i++)
    {
        rc = channel_alloc(job, &allocated_channels[i], input_channels);
        if (rc != 0)
            goto out;
    }

    if ((rc = ta_job_add_allocated_channels(manager, job, allocated_channels,
                                            n_channels, input_channels)) != 0)
    {
        goto out;
    }

    if (channels != NULL)
    {
        for (i = 0; i < n_channels; i++)
            channels[i] = allocated_channels[i]->id;
    }

out:
    if (allocated_channels != NULL)
    {
        if (rc != 0)
        {
            for (i = 0; i < n_channels; i++)
                free(allocated_channels[i]);
        }
        free(allocated_channels);
    }

    return rc;
}

/** Checks if some filter may be attached to the channel */
static te_errno
channel_accepts_filters(channel_t *channel)
{
    if (channel == NULL)
        return TE_EINVAL;

    if (channel->is_input_channel)
    {
        ERROR("Failed to attach filter to input channel");
        return TE_EPERM;
    }

    if (channel->n_filters == MAX_FILTERS_PER_CHANNEL)
    {
        ERROR("Failed to attach filter to a channel, limit exceeded");
        return TE_ENOBUFS;
    }

    return 0;
}

static te_errno
ta_job_attach_filter_unsafe(ta_job_manager_t *manager, const char *filter_name,
                            unsigned int n_channels, unsigned int *channels,
                            te_bool readable, te_log_level log_level,
                            unsigned int *filter_id)
{
    filter_t *filter;
    unsigned int i;
    te_errno rc;

    for (i = 0; i < n_channels; i++)
    {
        rc = channel_accepts_filters(get_channel(manager, channels[i]));
        if (rc != 0)
            return rc;
    }

    if ((rc = filter_alloc(filter_name, readable, log_level, &filter)) != 0)
        return rc;

    if ((rc = filter_add(manager, filter)) != 0)
    {
        free(filter);
        return rc;
    }

    for (i = 0; i < n_channels; i++)
        channel_add_filter(get_channel(manager, channels[i]), filter);

    if (filter_id != NULL)
        *filter_id = filter->id;

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_attach_filter(ta_job_manager_t *manager, const char *filter_name,
                     unsigned int n_channels, unsigned int *channels,
                     te_bool readable, te_log_level log_level,
                     unsigned int *filter_id)
{
    te_errno rc;

    pthread_mutex_lock(&manager->channels_lock);

    rc = ta_job_attach_filter_unsafe(manager, filter_name, n_channels, channels,
                                     readable, log_level, filter_id);

    pthread_mutex_unlock(&manager->channels_lock);

    return rc;
}

static te_errno
ta_job_filter_add_regexp_unsafe(ta_job_manager_t *manager,
                                unsigned int filter_id, char *re,
                                unsigned int extract)
{
    filter_t *filter;
    te_errno rc;

    if ((filter = get_filter(manager, filter_id)) == NULL)
        return TE_EINVAL;

    if (filter->regexp_data != NULL)
    {
        ERROR("Filter already has a regexp");
        return TE_EPERM;
    }

    if ((rc = regexp_data_create(re, extract, &filter->regexp_data)) != 0)
        return rc;

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_filter_add_regexp(ta_job_manager_t *manager, unsigned int filter_id,
                         char *re, unsigned int extract)
{
    te_errno rc;

    pthread_mutex_lock(&manager->channels_lock);

    rc = ta_job_filter_add_regexp_unsafe(manager, filter_id, re, extract);

    pthread_mutex_unlock(&manager->channels_lock);

    return rc;
}

static te_errno
ta_job_filter_add_channels_unsafe(ta_job_manager_t *manager,
                                  unsigned int filter_id,
                                  unsigned int n_channels,
                                  unsigned int *channels)
{
    te_errno rc;
    filter_t *filter;
    unsigned int i, j;

    if ((filter = get_filter(manager, filter_id)) == NULL)
        return TE_EINVAL;

    for (i = 0; i < n_channels; i++)
    {
        channel_t *channel = get_channel(manager, channels[i]);

        rc = channel_accepts_filters(channel);
        if (rc != 0)
            return rc;

        /*
         * MAX_FILTERS_PER_CHANNEL is only 32 so this check will not take
         * too long
         */
        for (j = 0; j < channel->n_filters; j++)
        {
            if (channel->filters[j] == filter)
                return TE_EALREADY;
        }
    }

    for (i = 0; i < n_channels; i++)
        channel_add_filter(get_channel(manager, channels[i]), filter);

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_filter_add_channels(ta_job_manager_t *manager, unsigned int filter_id,
                           unsigned int n_channels, unsigned int *channels)
{
    te_errno rc;

    pthread_mutex_lock(&manager->channels_lock);

    rc = ta_job_filter_add_channels_unsafe(manager, filter_id, n_channels,
                                           channels);

    pthread_mutex_unlock(&manager->channels_lock);

    return rc;
}

static te_errno
move_or_copy_message_to_buffer(const message_t *msg, ta_job_buffer_t *buffer,
                               te_bool move)
{
    buffer->channel_id = msg->channel_id;
    buffer->filter_id = msg->filter_id;

    if (move)
    {
        buffer->data = msg->data;
    }
    else
    {
        if ((buffer->data = malloc(msg->size)) == NULL)
        {
            ERROR("Failed to copy message to buffer");
            return TE_ENOMEM;
        }
        memcpy(buffer->data, msg->data, msg->size);
    }

    buffer->size = msg->size;
    buffer->dropped = msg->dropped;
    buffer->eos = msg->eos;

    return 0;
}

static te_errno
filter_receive_common(ta_job_manager_t *manager, unsigned int filter_id,
                      ta_job_buffer_t *buffer, queue_action_t action)
{
    message_t *msg;
    filter_t *filter;
    te_errno rc = 0;

    pthread_mutex_lock(&manager->channels_lock);

    if ((filter = get_filter(manager, filter_id)) == NULL)
    {
        pthread_mutex_unlock(&manager->channels_lock);
        ERROR("Invalid filter id passed to filter receive");
        return TE_EINVAL;
    }

    switch (action)
    {
        case EXTRACT_FIRST:
            msg = queue_extract_first_msg(&filter->queue);
            break;
        case GET_LAST:
            msg = queue_get_last_msg(&filter->queue);
            break;
        default:
            pthread_mutex_unlock(&manager->channels_lock);
            assert(0);
    }

    if (msg != NULL)
    {
        rc = move_or_copy_message_to_buffer(msg, buffer,
                                            action == EXTRACT_FIRST);
        if (rc != 0)
        {
            pthread_mutex_unlock(&manager->channels_lock);
            return rc;
        }

        if (action == EXTRACT_FIRST)
            free(msg);
    }
    else
    {
        rc = TE_ENODATA;
    }

    pthread_mutex_unlock(&manager->channels_lock);

    return rc;
}

static void
switch_signal_on_data(ta_job_manager_t *manager, unsigned int channel_id,
                      te_bool on)
{

    channel_t *channel = NULL;
    filter_t *filter = NULL;

    get_channel_or_filter(manager, channel_id, &channel, &filter);
    if (channel != NULL)
        channel->signal_on_data = on;
    else if (filter != NULL)
        filter->signal_on_data = on;
}

static te_errno
channel_or_filter_ready(ta_job_manager_t *manager, unsigned int id,
                        te_bool filter_only, te_bool *ready)
{
    channel_t *channel = NULL;
    filter_t *filter = NULL;

    get_channel_or_filter(manager, id, &channel, &filter);
    if (channel != NULL)
    {
        if (filter_only)
        {
            ERROR("Invalid id, expected only filter id");
            return TE_EPERM;
        }
        if (!channel->is_input_channel)
        {
            ERROR("Unable to check data on out channel, in channel expected");
            return TE_EPERM;
        }
        if (channel->fd < 0)
        {
            ERROR("Unable to check data on input channel without binded fd");
            return TE_EBADF;
        }
        *ready = channel->input_ready;
    }
    else if (filter != NULL)
    {
        if (!filter->readable)
        {
            ERROR("Failed to check data on unreadable filter");
            return TE_EPERM;
        }

        *ready = queue_has_data(&filter->queue);
    }
    else
    {
        return TE_EINVAL;
    }

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_poll(ta_job_manager_t *manager, unsigned int n_channels,
            unsigned int *channel_ids, int timeout_ms, te_bool filter_only)
{
    unsigned int i;
    te_errno rc = 0;
    int wait_rc;
    struct timeval now;
    struct timespec timeout;
    uint64_t nanoseconds;
    te_bool ready = FALSE;

    pthread_mutex_lock(&manager->channels_lock);

    /* After this loop all channel ids are checked to be valid */
    for (i = 0; i < n_channels; i++)
    {
        te_bool ready_tmp = FALSE;

        if ((rc = channel_or_filter_ready(manager, channel_ids[i], filter_only,
                                          &ready_tmp)) != 0)
        {
            pthread_mutex_unlock(&manager->channels_lock);
            ERROR("Job poll failed, %r", rc);
            return rc;
        }
        if (ready_tmp)
            ready = TRUE;
    }

    if (ready)
    {
        pthread_mutex_unlock(&manager->channels_lock);
        return 0;
    }

    for (i = 0; i < n_channels; i++)
        switch_signal_on_data(manager, channel_ids[i], TRUE);

    if (timeout_ms >= 0)
    {
        gettimeofday(&now, NULL);

        nanoseconds = TE_SEC2NS(now.tv_sec);
        nanoseconds += TE_US2NS(now.tv_usec);
        nanoseconds += TE_MS2NS(timeout_ms);

        TE_NS2TS(nanoseconds, &timeout);

        wait_rc = pthread_cond_timedwait(&manager->data_cond,
                                         &manager->channels_lock, &timeout);
    }
    else
    {
        wait_rc = pthread_cond_wait(&manager->data_cond,
                                    &manager->channels_lock);
    }

    if (wait_rc != 0)
    {
        if (wait_rc != ETIMEDOUT)
            WARN("pthread_cond_(timed)wait failed");
        rc = te_rc_os2te(wait_rc);
    }

    for (i = 0; i < n_channels; i++)
        switch_signal_on_data(manager, channel_ids[i], FALSE);

    pthread_mutex_unlock(&manager->channels_lock);

    return rc;
}

static te_errno
ta_job_receive_common(ta_job_manager_t *manager, unsigned int n_filters,
                      unsigned int *filters, te_bool do_poll, int timeout_ms,
                      ta_job_buffer_t *buffer, queue_action_t action)
{
    unsigned int i;
    te_errno rc;

    if (do_poll)
    {
        rc = ta_job_poll(manager, n_filters, filters, timeout_ms, TRUE);
        if (rc != 0)
            return rc;
    }

    for (i = 0; i < n_filters; i++)
    {
        rc = filter_receive_common(manager, filters[i], buffer, action);
        switch (rc)
        {
            case 0:
                return 0;
            case TE_ENODATA:
                continue;
            default:
                ERROR("Job receive failed, %r", rc);
                return rc;
        }
    }

    if (do_poll)
    {
        ERROR("Critical job receive error");
        return TE_EIO;
    }
    else
    {
        return TE_ENODATA;
    }
}

/* See description in ta_job.h */
te_errno
ta_job_receive(ta_job_manager_t *manager, unsigned int n_filters,
               unsigned int *filters, int timeout_ms, ta_job_buffer_t *buffer)
{
    return ta_job_receive_common(manager, n_filters, filters, TRUE, timeout_ms,
                                 buffer, EXTRACT_FIRST);
}

/* See description in ta_job.h */
te_errno
ta_job_receive_last(ta_job_manager_t *manager, unsigned int n_filters,
                    unsigned int *filters, int timeout_ms,
                    ta_job_buffer_t *buffer)
{
    return ta_job_receive_common(manager, n_filters, filters, TRUE, timeout_ms,
                                 buffer, GET_LAST);
}

/* See description in ta_job.h */
te_errno
ta_job_receive_many(ta_job_manager_t *manager, unsigned int n_filters,
                    unsigned int *filters, int timeout_ms,
                    ta_job_buffer_t **buffers, unsigned int *count)
{
    te_vec bufs = TE_VEC_INIT(ta_job_buffer_t);
    ta_job_buffer_t buf;
    te_errno rc;
    int i;

    if ((rc = ta_job_poll(manager, n_filters, filters, timeout_ms, TRUE)) != 0)
    {
        *count = 0;
        return rc;
    }

    for (i = 0; i < *count || *count == 0; i++)
    {
        memset(&buf, 0, sizeof(buf));

        rc = ta_job_receive_common(manager, n_filters, filters, FALSE, 0, &buf,
                                   EXTRACT_FIRST);
        if (rc != 0)
        {
            if (rc == TE_ENODATA)
                rc = 0;

            break;
        }

        rc = TE_VEC_APPEND(&bufs, buf);
        if (rc != 0)
            break;
    }

    *buffers = (ta_job_buffer_t *)(bufs.data.ptr);
    *count = i;

    return rc;
}

/* See description in ta_job.h */
te_errno
ta_job_clear(ta_job_manager_t *manager, unsigned int n_filters,
             unsigned int *filters)
{
    unsigned int i;

    pthread_mutex_lock(&manager->channels_lock);

    for (i = 0; i < n_filters; i++)
    {
        if (get_filter(manager, filters[i]) == NULL)
        {
            pthread_mutex_unlock(&manager->channels_lock);
            ERROR("Invalid filter id passed to job clear");
            return TE_EINVAL;
        }
    }

    for (i = 0; i < n_filters; i++)
        queue_destroy(&get_filter(manager, filters[i])->queue);

    pthread_mutex_unlock(&manager->channels_lock);

    return 0;
}

static te_errno
ta_job_send_unsafe(ta_job_manager_t *manager, unsigned int channel_id,
                   size_t count, uint8_t *buf)
{
    channel_t *channel = get_channel(manager, channel_id);
    ssize_t write_rc;
    te_errno rc;

    if (channel == NULL)
        return TE_EINVAL;

    if (!channel->is_input_channel)
    {
        ERROR("Failed to send data to process' output channel");
        return TE_EPERM;
    }

    if (channel->fd < 0)
    {
        ERROR("Channel's file descriptor is not binded to process");
        return TE_EBADFD;
    }

    if (!channel->input_ready)
    {
        ERROR("Channel is not ready to accept input");
        return TE_EAGAIN;
    }

    if ((write_rc = write(channel->fd, buf, count)) < 0)
    {
        rc = te_rc_os2te(errno);
        if (rc == TE_EPIPE)
        {
            WARN("Attempt to write to closed descriptor");
            channel->closed = TRUE;
        }
        else
        {
            ERROR("write() failed");
        }
        return te_rc_os2te(errno);
    }

    channel->input_ready = FALSE;

    if ((size_t)write_rc != count)
    {
        ERROR("Incomplete write");
        return TE_EIO;
    }

    /* Wake up the service thread to continue monitoring the descriptor */
    if ((rc = ctrl_pipe_send(manager)) != 0)
        return rc;

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_send(ta_job_manager_t *manager, unsigned int channel_id,
            size_t count, uint8_t *buf)
{
    te_errno rc;
    pthread_mutex_lock(&manager->channels_lock);

    rc = ta_job_send_unsafe(manager, channel_id, count, buf);

    pthread_mutex_unlock(&manager->channels_lock);

    return rc;
}

/* See description in ta_job.h */
te_errno
ta_job_kill(ta_job_manager_t *manager, unsigned int job_id, int signo)
{
    ta_job_t *job;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid < 0)
        return TE_ESRCH;

    if (kill(job->pid, signo) < 0)
    {
        ERROR("kill() failed");
        return te_rc_os2te(errno);
    }

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_killpg(ta_job_manager_t *manager, unsigned int job_id, int signo)
{
    ta_job_t *job;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid < 0)
        return TE_ESRCH;

    if (killpg(getpgid(job->pid), signo) < 0)
    {
        ERROR("killpg() failed");
        return te_rc_os2te(errno);
    }

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_wait(ta_job_manager_t *manager, unsigned int job_id, int timeout_ms,
            ta_job_status_t *status)
{
    ta_job_t *job;
    te_errno rc;
    ta_job_status_t job_status;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid < 0)
    {
        if (!job->has_started)
            return TE_ECHILD;

        if (status != NULL)
            *status = job->last_status;

        return 0;
    }

    if ((rc = proc_wait(job->pid, timeout_ms, &job_status)) == 0)
    {
        job->pid = -1;
        job->last_status = job_status;
        if (status != NULL)
            *status = job_status;
    }

    return rc;
}

/* See description in ta_job.h */
te_errno
ta_job_stop(ta_job_manager_t *manager, unsigned int job_id, int signo,
            int term_timeout_ms)
{
    te_errno rc = 0;
    ta_job_t *job;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != -1)
    {
        rc = proc_kill(job->pid, signo, term_timeout_ms);
        if (rc == 0)
            job->pid = -1;
    }

    return rc;
}

static void
wrapper_dealloc(wrapper_t *wrap)
{
    unsigned int i;

    for (i = 0; wrap->argv != NULL && wrap->argv[i] != NULL; i++)
        free(wrap->argv[i]);
    free(wrap->argv);

    free(wrap->tool);
    free(wrap);
}

/* See description in ta_job.h */
te_errno
ta_job_destroy(ta_job_manager_t *manager, unsigned int job_id,
               int term_timeout_ms)
{
    unsigned int i;
    ta_job_t *job;
    wrapper_t *wrapper;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    /*
     * Try to kill the job before destroying,
     * destroy it regardless of the result
     */
    if (job->pid != (pid_t)-1)
        proc_kill(job->pid, SIGTERM, term_timeout_ms);

    LIST_REMOVE(job, next);

    pthread_mutex_lock(&manager->channels_lock);

    for (i = 0; i < job->n_out_channels; i++)
        job->out_channels[i]->job = NULL;
    for (i = 0; i < job->n_in_channels; i++)
        job->in_channels[i]->job = NULL;

    pthread_mutex_unlock(&manager->channels_lock);

    while ((wrapper = LIST_FIRST(&job->wrappers)) != NULL)
    {
        LIST_REMOVE(wrapper, next);
        wrapper_dealloc(wrapper);
    }

    ta_job_dealloc(job);

    return 0;
}

static te_errno
wrapper_alloc(const char *tool, char **argv,
              ta_job_wrapper_priority_t priority, wrapper_t **wrap)
{
    wrapper_t *wrapper;

    if (tool == NULL)
    {
        ERROR("Failed to allocate a wrapper: path to a tool is not specified");
        return TE_ENOENT;
    }

    wrapper = calloc(1, sizeof(*wrapper));

    if (wrapper == NULL)
    {
        ERROR("Wrapper allocation failed");
        return TE_ENOMEM;
    }

    wrapper->tool = strdup(tool);
    if (wrapper->tool == NULL)
    {
        ERROR("Wrapper tool allocation failed");
        free(wrapper);
        return TE_ENOMEM;
    }

    wrapper->argv = argv;
    wrapper->priority = priority;

    *wrap = wrapper;

    return 0;
}

static void
wrapper_append(wrapper_t *wrap, ta_job_t *job)
{
    unsigned int created_wrap_id;

    if (LIST_EMPTY(&job->wrappers))
        created_wrap_id = 0;
    else
        created_wrap_id = LIST_FIRST(&job->wrappers)->id + 1;

    wrap->id = created_wrap_id;

    LIST_INSERT_HEAD(&job->wrappers, wrap, next);
}

/* See description in ta_job.h */
te_errno
ta_job_wrapper_add(ta_job_manager_t *manager, const char *tool, char **argv,
                   unsigned int job_id, ta_job_wrapper_priority_t priority,
                   unsigned int *wrapper_id)
{
    te_errno rc;
    wrapper_t *wrap;
    ta_job_t *job;

    rc = te_file_check_executable(tool);
    if (rc != 0)
        return rc;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != (pid_t)-1)
    {
        ERROR("Failed to allocate a wrapper: Job has been started.");
        return TE_EPERM;
    }

    rc = wrapper_alloc(tool, argv, priority, &wrap);
    if (rc != 0)
        return rc;

    wrapper_append(wrap, job);

    if (wrapper_id != NULL)
        *wrapper_id = wrap->id;

    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_wrapper_delete(ta_job_manager_t *manager, unsigned int job_id,
                      unsigned int wrapper_id)
{
    wrapper_t *wrapper;
    ta_job_t *job;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    for (wrapper = LIST_FIRST(&job->wrappers);
         wrapper != NULL; wrapper = LIST_NEXT(wrapper, next))
    {
        if (wrapper->id == wrapper_id)
        {
            LIST_REMOVE(wrapper, next);
            wrapper_dealloc(wrapper);
            break;
        }
    }
    return 0;
}

/* See description in ta_job.h */
te_errno
ta_job_add_sched_param(ta_job_manager_t *manager, unsigned int job_id,
                       te_sched_param *sched_params)
{
    ta_job_t *job;

    job = get_job(manager, job_id);
    if (job == NULL)
        return TE_EINVAL;

    job->sched_params = sched_params;
    return 0;
}
