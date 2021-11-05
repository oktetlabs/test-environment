/** @file
 * @brief RPC for Agent job control
 *
 * RPC routines implementation to call Agent job control functions
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC JOB"

#include "te_config.h"

#include <pcre.h>

#include "te_exec_child.h"
#include "te_sleep.h"
#include "te_string.h"
#include "te_vector.h"
#include "te_file.h"
#include "te_alloc.h"

#include "rpc_server.h"
#include "te_errno.h"
#include "te_string.h"

#include "agentlib.h"

/* The maximum size of log user entry (in bytes) for filter logging. */
#define MAX_LOG_USER_SIZE 128

#define PROC_WAIT_US 1000
/*
 * The amount of time needed for a process to terminate after receiving
 * a signal (that is meant to terminate the process) with default handler
 */
#define KILL_TIMEOUT_MS 10

#define MAX_OUTPUT_CHANNELS_PER_JOB 32
#define MAX_INPUT_CHANNELS_PER_JOB 32
#define MAX_FILTERS_PER_CHANNEL 32

#define MAX_QUEUE_SIZE (16 * 1024 * 1024)
#define MAX_MESSAGE_DATA_SIZE 8192

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

    struct job_t *job;
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

typedef LIST_HEAD(job_list, job_t) job_list;

/**
 * note: New priority values must be added between
 * WRAPPER_PRIORITY_MIN and WRAPPER_PRIORITY_MAX.
 */
typedef enum  {
    /**
     * This is a service item. Indicates the minimum
     * value in this enum.
     */
    WRAPPER_PRIORITY_MIN,

    WRAPPER_PRIORITY_LOW,
    WRAPPER_PRIORITY_DEFAULT,
    WRAPPER_PRIORITY_HIGH,

    /**
     * This is a service item. Indicates the maxixmum
     * value in this enum.
     */
    WRAPPER_PRIORITY_MAX
} wrapper_priority;

typedef struct wrapper_t {
    LIST_ENTRY(wrapper_t) next;

    unsigned int id;

    char *tool;
    char **argv;
    wrapper_priority priority;
} wrapper_t;

typedef struct job_t {
    LIST_ENTRY(job_t) next;

    unsigned int id;

    channel_t *out_channels[MAX_OUTPUT_CHANNELS_PER_JOB];
    unsigned int n_out_channels;
    channel_t *in_channels[MAX_INPUT_CHANNELS_PER_JOB];
    unsigned int n_in_channels;

    pid_t pid;
    /* A job has started once */
    te_bool has_started;
    tarpc_job_status last_status;
    char *spawner;
    char *tool;
    char **argv;
    char **env;

    LIST_HEAD(wrapper_list, wrapper_t) wrappers;

    te_sched_param *sched_params;
} job_t;

#define CTRL_PIPE_INITIALIZER {-1, -1}

/* Static data */

static job_list all_jobs = LIST_HEAD_INITIALIZER(&all_jobs);
static channel_list all_channels = LIST_HEAD_INITIALIZER(&all_channels);
static filter_list all_filters = LIST_HEAD_INITIALIZER(&all_filters);

static pthread_mutex_t channels_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond = PTHREAD_COND_INITIALIZER;
static int ctrl_pipe[2] = CTRL_PIPE_INITIALIZER;

static int abandoned_descriptors[FD_SETSIZE];
static size_t n_abandoned_descriptors = 0;

// TODO: volatile (to stop from thread itself), atomic
static te_bool thread_is_running = false;
static pthread_t service_thread;

#define DEF_GETTER(name, type, head)                    \
static type *                                           \
get_##name(unsigned int id)                             \
{                                                       \
    type *item;                                         \
                                                        \
    LIST_FOREACH(item, head, next)                      \
    {                                                   \
        if (item->id == id)                             \
            return item;                                \
    }                                                   \
    ERROR(#name " with %d id is not found", id);        \
    return NULL;                                        \
}

DEF_GETTER(job, job_t, &all_jobs);
DEF_GETTER(channel, channel_t, &all_channels);
DEF_GETTER(filter, filter_t, &all_filters);

#undef DEF_GETTER

static void
get_channel_or_filter(unsigned int id, channel_t **channel, filter_t **filter)
{
    channel_t *c;
    filter_t *f;

    LIST_FOREACH(c, &all_channels, next)
    {
        if (c->id == id)
        {
            *channel = c;
            return;
        }
    }

    LIST_FOREACH(f, &all_filters, next)
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

    result->re = pcre_compile(pattern, PCRE_MULTILINE, &error, &erroffset, NULL);
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
abandoned_descriptors_add(size_t count, int *fds)
{
    size_t i;

    if (n_abandoned_descriptors + count > TE_ARRAY_LEN(abandoned_descriptors))
    {
        ERROR("Failed to add abandoned descriptors: not enough space");
        return TE_ENOBUFS;
    }

    for (i = 0; i < count; i++)
        abandoned_descriptors[n_abandoned_descriptors++] = fds[i];

    return 0;
}

static void
abandoned_descriptors_close(void)
{
    size_t i;

    for (i = 0; i < n_abandoned_descriptors; i++)
        close(abandoned_descriptors[i]);

    n_abandoned_descriptors = 0;
}

#define CTRL_MESSAGE ("c\n")

static te_errno
ctrl_pipe_create(void)
{
    int rc;

    if (ctrl_pipe[0] > -1 || ctrl_pipe[1] > -1)
    {
        WARN("Control pipe already created");
        return 0;
    }

    if ((rc = pipe2(ctrl_pipe, O_CLOEXEC)) != 0)
    {
        ERROR("Control pipe creation failure");
        return te_rc_os2te(rc);
    }

    return 0;
}

static void
ctrl_pipe_destroy(void)
{
    if (ctrl_pipe[0] > -1)
        close(ctrl_pipe[0]);
    if (ctrl_pipe[1] > -1)
        close(ctrl_pipe[1]);

    ctrl_pipe[0] = -1;
    ctrl_pipe[1] = -1;
}

static int
ctrl_pipe_get_read_fd(void)
{
    return ctrl_pipe[0];
}

static te_errno
ctrl_pipe_send(void)
{
    ssize_t write_rc;

    write_rc = write(ctrl_pipe[1], CTRL_MESSAGE, sizeof(CTRL_MESSAGE));
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

/* Channels and filters exist in the same handler namespace */
static unsigned int channel_last_id = 0;

static te_errno
channel_add(channel_t *channel)
{
    unsigned int created_channel_id;

    if (channel_last_id == UINT_MAX)
    {
        ERROR("Maximum channel id has been reached");
        return TE_EFAIL;
    }

    created_channel_id = channel_last_id;

    channel->id = created_channel_id;

    LIST_INSERT_HEAD(&all_channels, channel, next);
    channel_last_id++;

    return 0;
}

static void
channel_remove(channel_t *channel)
{
    LIST_REMOVE(channel, next);
}

static te_errno
filter_add(filter_t *filter)
{
    unsigned int created_filter_id;

    if (channel_last_id == UINT_MAX)
    {
        ERROR("Maximum filter id has been reached");
        return TE_EFAIL;
    }

    created_filter_id = channel_last_id;

    filter->id = created_filter_id;

    LIST_INSERT_HEAD(&all_filters, filter, next);
    channel_last_id++;

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
channel_alloc(job_t *job, channel_t **channel, te_bool is_input_channel)
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
        ; // Do nothing

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
job_add(job_t *job)
{
    static unsigned int job_last_id = 0;
    unsigned int created_job_id;

    if (job_last_id == UINT_MAX)
    {
        ERROR("Maximum job id has been reached");
        return TE_EFAIL;
    }

    created_job_id = job_last_id;

    job->id = created_job_id;

    LIST_INSERT_HEAD(&all_jobs, job, next);
    job_last_id++;

    return 0;
}

/* Note: argv and env MUST remain valid after a successful call */
static te_errno
job_alloc(const char *spawner, const char *tool, char **argv,
          char **env, job_t **job)
{
    job_t *result = calloc(1, sizeof(*result));

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
job_dealloc(job_t *job)
{
    unsigned int i;

    for (i = 0; job->argv != NULL && job->argv[i] != NULL; i++)
        free(job->argv[i]);
    free(job->argv);

    for (i = 0; job->env != NULL && job->env[i] != NULL; i++)
        free(job->env[i]);
    free(job->env);

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

/* Note: argv and env MUST remain valid after a successful call */
static te_errno
job_create(const char *spawner, const char *tool, char **argv,
           char **env, unsigned int *job_id)
{
    te_errno rc;
    job_t *job;

    if ((rc = job_alloc(spawner, tool, argv, env, &job)) != 0)
        return rc;

    if ((rc = job_add(job)) != 0)
    {
        job_dealloc(job);
        return rc;
    }

    *job_id = job->id;

    return 0;
}

static te_errno
proc_wait(pid_t pid, int timeout_ms, tarpc_job_status *status)
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

    if (WIFEXITED(wstatus))
    {
        status->type = TARPC_JOB_STATUS_EXITED;
        status->value = WEXITSTATUS(wstatus);
    }
    else if (WIFSIGNALED(wstatus))
    {
        status->type = TARPC_JOB_STATUS_SIGNALED;
        status->value = WTERMSIG(wstatus);
    }
    else
    {
        WARN("Child process with PID %d exited due to unknown reason", pid);
        status->type = TARPC_JOB_STATUS_UNKNOWN;
    }

    return 0;
}

static te_errno
proc_kill(pid_t pid, int signo, int term_timeout_ms)
{
    te_errno rc;
    tarpc_job_status dummy;
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
    if ((rc = proc_wait(pid, timeout, &dummy)) != 0 && signo != SIGKILL)
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
        if (proc_wait(pid, KILL_TIMEOUT_MS, &dummy) != 0)
        {
            ERROR("Failed to wait for killed process");
            return te_rc_os2te(errno);
        }
    }

    return rc;
}

static te_errno
match_callback(filter_t *filter, unsigned int channel_id, pid_t pid,
               size_t size, char *buf, te_bool eos)
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

        LGR_MESSAGE(filter->log_level, log_user, "%.*s", log_size, buf);
    }

    if (filter->readable)
    {
        rc = queue_put(buf, size, eos, channel_id, filter->id, &filter->queue);
        if (rc != 0)
            return rc;
    }

    if (filter->signal_on_data)
        pthread_cond_signal(&data_cond);

    return 0;
}

static te_errno
filter_regexp_exec(filter_t *filter, unsigned int channel_id, pid_t pid,
                   int segment_length, char *segment, te_bool eos,
                   te_errno (*fun_ptr)(filter_t *, unsigned int, pid_t,
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
                            /* Break if byte is the first byte of UTF-8 character */
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

        if ((te_rc = fun_ptr(filter, channel_id, pid, substring_length,
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
filter_exec(filter_t *filter, unsigned int channel_id, pid_t pid,
            size_t size, char *buf)
{
    te_bool eos = (size == 0);
    te_errno rc;

    if (filter->regexp_data != NULL)
    {
        rc = filter_regexp_exec(filter, channel_id, pid, size,
                                buf, eos, match_callback);
        /*
         * Call match_callback() separately for eos-message since
         * filter_regexp_exec() does not do that.
         */
        if (rc != 0 || !eos)
            return rc;
    }

    return match_callback(filter, channel_id, pid, size, buf, eos);
}

static te_errno
channel_read(channel_t *channel)
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
        if ((rc = filter_exec(channel->filters[i], channel->id,
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

    //LIST_FOREACH_SAFE(channel, channels, next, channel_tmp)
    for (channel = LIST_FIRST(channels); channel != NULL; channel = channel_tmp)
    {
        channel_tmp = LIST_NEXT(channel, next);

        if (channel->job == NULL)
        {
            if (channel->fd > -1)
                close(channel->fd);
            channel_destroy(channel);
        }
    }
}

static void
thread_read_selected(fd_set *rfds, channel_list *channels)
{
    channel_t *active_channel = NULL;
    channel_t *channel;
    te_errno rc;

    for (channel = LIST_FIRST(channels);
         channel != NULL; channel = LIST_NEXT(channel, next))
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

    if ((rc = channel_read(active_channel)) != 0)
    {
        WARN("Channel read failure '%r', continuing", rc);
    }
}

static void
thread_mark_selected_ready(fd_set *wfds, channel_list *channels)
{
    channel_t *channel;

    for (channel = LIST_FIRST(channels);
         channel != NULL; channel = LIST_NEXT(channel, next))
    {
        if (FD_ISSET(channel->fd, wfds))
        {
            channel->input_ready = TRUE;
            if (channel->signal_on_data)
                pthread_cond_signal(&data_cond);
        }
    }
}

static void *
thread_work_loop(void *arg)
{
    fd_set rfds;
    fd_set wfds;
    int select_rc;

    UNUSED(arg);

    logfork_register_user("JOB CONTROL");
    logfork_set_id_logging(FALSE);

    while (1)
    {
        int ctrl_fd = ctrl_pipe_get_read_fd();
        int max_fd = ctrl_fd;
        channel_t *channel;

        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(ctrl_fd, &rfds);

        pthread_mutex_lock(&channels_lock);

        LIST_FOREACH(channel, &all_channels, next)
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

        pthread_mutex_unlock(&channels_lock);

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

            pthread_mutex_lock(&channels_lock);

            if (FD_ISSET(ctrl_fd, &rfds))
            {
                char buf[sizeof(CTRL_MESSAGE)] = {0};
                ssize_t read_c = read(ctrl_fd, buf, sizeof(buf));

                if (read_c <= 0)
                    WARN("Control pipe read failed, continuing");

                channel_process_needed = FALSE;
            }

            thread_destroy_unused_channels(&all_channels);
            abandoned_descriptors_close();

            if (channel_process_needed)
            {
                thread_read_selected(&rfds, &all_channels);
                thread_mark_selected_ready(&wfds, &all_channels);
            }

            pthread_mutex_unlock(&channels_lock);
        }
    }

    return NULL;
}

static te_errno
thread_start(void)
{
    te_errno rc;

    if ((rc = ctrl_pipe_create()) != 0)
        return rc;

    /* Check read end of the pipe for descriptor limit */
    if (ctrl_pipe_get_read_fd() >= FD_SETSIZE)
    {
        ERROR("Failed to create control pipe, file descriptor limit exceeded");
        ctrl_pipe_destroy();
        return TE_EMFILE;
    }

    if (pthread_create(&service_thread, NULL, thread_work_loop, NULL) != 0)
    {
        ERROR("Thread create failure");
        ctrl_pipe_destroy();
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
job_build_tool_and_args(char **tool, te_vec *args, job_t *job)
{
    char *end_arg = NULL;
    te_errno rc;
    size_t j;
    wrapper_t *wrap;
    wrapper_priority i;

    if (!LIST_EMPTY(&job->wrappers))
    {
        for (i = WRAPPER_PRIORITY_MAX - 1; i > WRAPPER_PRIORITY_MIN; i--)
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

static te_errno
job_start(unsigned int id)
{
    te_errno rc;
    pid_t pid;
    job_t *job;
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

    job = get_job(id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != (pid_t)-1)
    {
        ERROR("Job is already started");
        return TE_EPERM;
    }

    if (!thread_is_running)
    {
        if ((rc = thread_start()) != 0)
            return rc;

        thread_is_running = TRUE;
    }

    if (job->n_out_channels < 2)
        stderr_fd_p = TE_EXEC_CHILD_DEV_NULL_FD;
    if (job->n_out_channels < 1)
        stdout_fd_p = TE_EXEC_CHILD_DEV_NULL_FD;
    if (job->n_in_channels < 1)
        stdin_fd_p = TE_EXEC_CHILD_DEV_NULL_FD;

    rc = job_build_tool_and_args(&tool, &args, job);
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

    pthread_mutex_lock(&channels_lock);

    if ((rc = ctrl_pipe_send()) != 0)
    {
        pthread_mutex_unlock(&channels_lock);
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
        rc = abandoned_descriptors_add(n_descrs_to_abandon, descrs_to_abandon);
        if (rc != 0)
        {
            pthread_mutex_unlock(&channels_lock);
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

    pthread_mutex_unlock(&channels_lock);

    job->has_started = TRUE;
    job->pid = pid;

    return 0;
}

static te_errno
job_add_allocated_channels(job_t *job, channel_t **allocated_channels,
                           unsigned int n_channels, te_bool input_channels)
{
    te_errno rc;
    unsigned int i;
    unsigned int j;

    pthread_mutex_lock(&channels_lock);

    for (i = 0; i < n_channels; i++)
    {
        if ((rc = channel_add(allocated_channels[i])) != 0)
        {
            pthread_mutex_unlock(&channels_lock);

            for (j = 0; j < i; j++)
                channel_remove(allocated_channels[j]);

            return rc;
        }
    }

    pthread_mutex_unlock(&channels_lock);

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

static te_errno
job_allocate_channels(unsigned int job_id, te_bool input_channels,
                      unsigned int n_channels, unsigned int *channels)
{
    channel_t **allocated_channels = NULL;
    te_errno rc = 0;
    unsigned int i;
    job_t *job;

    if ((job = get_job(job_id)) == NULL)
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

    if ((rc = job_add_allocated_channels(job, allocated_channels, n_channels,
                                         input_channels)) != 0)
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

static te_errno
job_attach_filter_unsafe(const char *filter_name, unsigned int n_channels,
                         unsigned int *channels, te_bool readable,
                         te_log_level log_level, unsigned int *filter_id)
{
    filter_t *filter;
    unsigned int i;
    te_errno rc;

    for (i = 0; i < n_channels; i++)
    {
        channel_t *channel = get_channel(channels[i]);
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
    }

    if ((rc = filter_alloc(filter_name, readable, log_level, &filter)) != 0)
        return rc;

    if ((rc = filter_add(filter)) != 0)
    {
        free(filter);
        return rc;
    }

    for (i = 0; i < n_channels; i++)
        channel_add_filter(get_channel(channels[i]), filter);

    if (filter_id != NULL)
        *filter_id = filter->id;

    return 0;
}

static te_errno
job_attach_filter(const char *filter_name, unsigned int n_channels,
                  unsigned int *channels, te_bool readable,
                  te_log_level log_level, unsigned int *filter_id)
{
    te_errno rc;

    pthread_mutex_lock(&channels_lock);

    rc = job_attach_filter_unsafe(filter_name, n_channels, channels,
                                  readable, log_level, filter_id);

    pthread_mutex_unlock(&channels_lock);

    return rc;
}

static te_errno
job_filter_add_regexp_unsafe(unsigned int filter_id, char *re, unsigned int extract)
{
    filter_t *filter;
    te_errno rc;

    if ((filter = get_filter(filter_id)) == NULL)
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

static te_errno
job_filter_add_regexp(unsigned int filter_id, char *re, unsigned int extract)
{
    te_errno rc;

    pthread_mutex_lock(&channels_lock);

    rc = job_filter_add_regexp_unsafe(filter_id, re, extract);

    pthread_mutex_unlock(&channels_lock);

    return rc;
}

static te_errno
move_or_copy_message_to_buffer(const message_t *msg, tarpc_job_buffer *buffer,
                               te_bool move)
{
    buffer->channel = msg->channel_id;
    buffer->filter = msg->filter_id;

    if (move)
    {
        buffer->data.data_val = msg->data;
    }
    else
    {
        if ((buffer->data.data_val = malloc(msg->size)) == NULL)
        {
            ERROR("Failed to copy message to buffer");
            return TE_ENOMEM;
        }
        memcpy(buffer->data.data_val, msg->data, msg->size);
    }

    buffer->data.data_len = msg->size;
    buffer->dropped = msg->dropped;
    buffer->eos = msg->eos;

    return 0;
}

static te_errno
filter_receive_common(unsigned int filter_id, tarpc_job_buffer *buffer,
                      queue_action_t action)
{
    message_t *msg;
    filter_t *filter;
    te_errno rc = 0;

    pthread_mutex_lock(&channels_lock);

    if ((filter = get_filter(filter_id)) == NULL)
    {
        pthread_mutex_unlock(&channels_lock);
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
            pthread_mutex_unlock(&channels_lock);
            assert(0);
    }

    if (msg != NULL)
    {
        rc = move_or_copy_message_to_buffer(msg, buffer,
                                            action == EXTRACT_FIRST);
        if (rc != 0)
        {
            pthread_mutex_unlock(&channels_lock);
            return rc;
        }

        if (action == EXTRACT_FIRST)
            free(msg);
    }
    else
    {
        rc = TE_ENODATA;
    }

    pthread_mutex_unlock(&channels_lock);

    return rc;
}

static void
switch_signal_on_data(unsigned int channel_id, te_bool on)
{

    channel_t *channel = NULL;
    filter_t *filter = NULL;

    get_channel_or_filter(channel_id, &channel, &filter);
    if (channel != NULL)
        channel->signal_on_data = on;
    else if (filter != NULL)
        filter->signal_on_data = on;
}

static te_errno
channel_or_filter_ready(unsigned int id, te_bool filter_only, te_bool *ready)
{
    channel_t *channel = NULL;
    filter_t *filter = NULL;

    get_channel_or_filter(id, &channel, &filter);
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

static te_errno
job_poll(unsigned int n_channels, unsigned int *channel_ids, int timeout_ms,
         te_bool filter_only)
{
    unsigned int i;
    te_errno rc = 0;
    int wait_rc;
    struct timeval now;
    struct timespec timeout;
    uint64_t nanoseconds;
    te_bool ready = FALSE;

    pthread_mutex_lock(&channels_lock);

    /* After this loop all channel ids are checked to be valid */
    for (i = 0; i < n_channels; i++)
    {
        te_bool ready_tmp = FALSE;

        if ((rc = channel_or_filter_ready(channel_ids[i], filter_only,
                                          &ready_tmp)) != 0)
        {
            pthread_mutex_unlock(&channels_lock);
            ERROR("Job poll failed, %r", rc);
            return rc;
        }
        if (ready_tmp)
            ready = TRUE;
    }

    if (ready)
    {
        pthread_mutex_unlock(&channels_lock);
        return 0;
    }

    for (i = 0; i < n_channels; i++)
        switch_signal_on_data(channel_ids[i], TRUE);

    if (timeout_ms >= 0)
    {
        gettimeofday(&now, NULL);

        nanoseconds = TE_SEC2NS(now.tv_sec);
        nanoseconds += TE_US2NS(now.tv_usec);
        nanoseconds += TE_MS2NS(timeout_ms);

        TE_NS2TS(nanoseconds, &timeout);

        wait_rc = pthread_cond_timedwait(&data_cond, &channels_lock, &timeout);
    }
    else
    {
        wait_rc = pthread_cond_wait(&data_cond, &channels_lock);
    }

    if (wait_rc != 0)
    {
        if (wait_rc != ETIMEDOUT)
            WARN("pthread_cond_(timed)wait failed");
        rc = te_rc_os2te(wait_rc);
    }

    for (i = 0; i < n_channels; i++)
        switch_signal_on_data(channel_ids[i], FALSE);

    pthread_mutex_unlock(&channels_lock);

    return rc;
}

static te_errno
job_receive_common(unsigned int n_filters, unsigned int *filters,
                   te_bool do_poll, int timeout_ms,
                   tarpc_job_buffer *buffer,
                   queue_action_t action)
{
    unsigned int i;
    te_errno rc;

    if (do_poll)
    {
        if ((rc = job_poll(n_filters, filters, timeout_ms, TRUE)) != 0)
            return rc;
    }

    for (i = 0; i < n_filters; i++)
    {
        rc = filter_receive_common(filters[i], buffer, action);
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

/* Receive first message from filter queue and remove it from there. */
static te_errno
job_receive(unsigned int n_filters, unsigned int *filters, int timeout_ms,
            tarpc_job_buffer *buffer)
{
    return job_receive_common(n_filters, filters, TRUE, timeout_ms, buffer,
                              EXTRACT_FIRST);
}

/*
 * Receive last (or second-to-last) message from filter queue,
 * do not remove it from there.
 */
static te_errno
job_receive_last(unsigned int n_filters, unsigned int *filters, int timeout_ms,
                 tarpc_job_buffer *buffer)
{
    return job_receive_common(n_filters, filters, TRUE, timeout_ms, buffer,
                              GET_LAST);
}

/* Receive multiple messages at once. */
static te_errno
job_receive_many(unsigned int n_filters, unsigned int *filters,
                 int timeout_ms, tarpc_job_buffer **buffers,
                 unsigned int *count)
{
    te_vec bufs = TE_VEC_INIT(tarpc_job_buffer);
    tarpc_job_buffer buf;
    te_errno rc;
    int i;

    if ((rc = job_poll(n_filters, filters, timeout_ms, TRUE)) != 0)
    {
        *count = 0;
        return rc;
    }

    for (i = 0; i < *count || *count == 0; i++)
    {
        memset(&buf, 0, sizeof(buf));

        rc = job_receive_common(n_filters, filters, FALSE, 0, &buf,
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

    *buffers = (tarpc_job_buffer *)(bufs.data.ptr);
    *count = i;

    return rc;
}

static te_errno
job_clear(unsigned int n_filters, unsigned int *filters)
{
    unsigned int i;

    pthread_mutex_lock(&channels_lock);

    for (i = 0; i < n_filters; i++)
    {
        if (get_filter(filters[i]) == NULL)
        {
            pthread_mutex_unlock(&channels_lock);
            ERROR("Invalid filter id passed to job clear");
            return TE_EINVAL;
        }
    }

    for (i = 0; i < n_filters; i++)
        queue_destroy(&get_filter(filters[i])->queue);

    pthread_mutex_unlock(&channels_lock);

    return 0;
}

static te_errno
job_send_unsafe(unsigned int channel_id, size_t count, uint8_t *buf)
{
    channel_t *channel = get_channel(channel_id);
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

    /* Wake up working thread to continue monitor the descriptor */
    if ((rc = ctrl_pipe_send()) != 0)
        return rc;

    return 0;
}

static te_errno
job_send(unsigned int channel_id, size_t count, uint8_t *buf)
{
    te_errno rc;
    pthread_mutex_lock(&channels_lock);

    rc = job_send_unsafe(channel_id, count, buf);

    pthread_mutex_unlock(&channels_lock);

    return rc;
}

static te_errno
job_kill(unsigned int job_id, int signo)
{
    job_t *job;

    job = get_job(job_id);
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

static te_errno
job_killpg(unsigned int job_id, int signo)
{
    job_t *job;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid < 0)
        return TE_ESRCH;

    if (killpg(job->pid, signo) < 0)
    {
        ERROR("killpg() failed");
        return te_rc_os2te(errno);
    }

    return 0;
}

static te_errno
job_wait(unsigned int job_id, int timeout_ms, tarpc_job_status *status)
{
    job_t *job;
    te_errno rc;
    tarpc_job_status job_status;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid < 0)
    {
        if (!job->has_started)
            return TE_ECHILD;

        *status = job->last_status;

        return 0;
    }

    if ((rc = proc_wait(job->pid, timeout_ms, &job_status)) == 0)
    {
        job->pid = -1;
        job->last_status = job_status;
        *status = job_status;
    }

    return rc;
}

static te_errno
job_stop(unsigned int job_id, int signo, int term_timeout_ms)
{
    te_errno rc = 0;
    job_t *job;

    job = get_job(job_id);
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
job_wrapper_dealloc(wrapper_t *wrap)
{
    unsigned int i;

    for (i = 0; wrap->argv != NULL && wrap->argv[i] != NULL; i++)
        free(wrap->argv[i]);
    free(wrap->argv);

    free(wrap->tool);
    free(wrap);

}

static te_errno
job_destroy(unsigned int job_id, int term_timeout_ms)
{
    unsigned int i;
    job_t *job;
    wrapper_t *wrapper;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    /*
     * Try to kill the job before destroying,
     * destroy it regardless of the result
     */
    if (job->pid != (pid_t)-1)
        proc_kill(job->pid, SIGTERM, term_timeout_ms);

    LIST_REMOVE(job, next);

    pthread_mutex_lock(&channels_lock);

    for (i = 0; i < job->n_out_channels; i++)
        job->out_channels[i]->job = NULL;
    for (i = 0; i < job->n_in_channels; i++)
        job->in_channels[i]->job = NULL;

    pthread_mutex_unlock(&channels_lock);

    while ((wrapper = LIST_FIRST(&job->wrappers)) != NULL)
    {
        LIST_REMOVE(wrapper, next);
        job_wrapper_dealloc(wrapper);
    }

    job_dealloc(job);

    return 0;
}

static te_errno
tarpc_job_wrapper_priority2wrapper_priority(
                             const tarpc_job_wrapper_priority *from,
                             wrapper_priority *to)
{
    switch(*from)
    {
        case TARPC_JOB_WRAPPER_PRIORITY_LOW:
            *to = WRAPPER_PRIORITY_LOW;
            break;

        case TARPC_JOB_WRAPPER_PRIORITY_DEFAULT:
            *to = WRAPPER_PRIORITY_DEFAULT;
            break;

        case TARPC_JOB_WRAPPER_PRIORITY_HIGH:
            *to = WRAPPER_PRIORITY_HIGH;
            break;

        default:
            ERROR("Priority value is not supported");
            return TE_EFAIL;
    }

    return 0;
}

static te_errno
job_wrapper_alloc(const char *tool, char **argv,
                  tarpc_job_wrapper_priority priority, wrapper_t **wrap)
{
    te_errno rc;
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
    rc = tarpc_job_wrapper_priority2wrapper_priority(&priority,
                                                     &wrapper->priority);
    if (rc != 0)
    {
        free(wrapper);
        return rc;
    }

    *wrap = wrapper;

    return 0;
}

static te_errno
job_wrapper_append(wrapper_t *wrap, job_t *job)
{
    unsigned int created_wrap_id;

    if (LIST_EMPTY(&job->wrappers))
        created_wrap_id = 0;
    else
        created_wrap_id = LIST_FIRST(&job->wrappers)->id + 1;

    wrap->id = created_wrap_id;

    LIST_INSERT_HEAD(&job->wrappers, wrap, next);
    return 0;
}

static te_errno
job_wrapper_add(const char *tool, char **argv, unsigned int job_id,
                tarpc_job_wrapper_priority priority,
                unsigned int *wrap_id)
{
    te_errno rc;
    wrapper_t *wrap;
    job_t *job;

    rc = te_file_check_executable(tool);
    if (rc != 0)
        return rc;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != (pid_t)-1)
    {
        ERROR("Failed to allocate a wrapper: Job has been started.");
        return TE_EPERM;
    }

    rc = job_wrapper_alloc(tool, argv, priority, &wrap);
    if (rc != 0)
        return rc;

    rc = job_wrapper_append(wrap, job);
    if (rc != 0)
    {
        job_wrapper_dealloc(wrap);
        return rc;
    }

    if (wrap_id != NULL)
        *wrap_id = wrap->id;

    return 0;
}

static te_errno
job_wrapper_delete(unsigned int job_id, unsigned int wrapper_id)
{
    wrapper_t *wrapper;
    job_t *job;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    for (wrapper = LIST_FIRST(&job->wrappers);
         wrapper != NULL; wrapper = LIST_NEXT(wrapper, next))
    {
        if (wrapper->id == wrapper_id)
        {
            LIST_REMOVE(wrapper, next);
            job_wrapper_dealloc(wrapper);
            break;
        }
    }
    return 0;
}

static te_errno
job_add_sched_param(unsigned int job_id,
                    te_sched_param *sched_params)
{
    job_t *job;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    job->sched_params = sched_params;
    return 0;
}

static te_errno
sched_affinity_param_alloc(te_sched_affinity_param **af,
                           tarpc_job_sched_param_data *data)
{
    int cpu_ids_len;
    te_sched_affinity_param *affinity;
    affinity = TE_ALLOC(sizeof(te_sched_affinity_param));
    if (affinity == NULL)
        return TE_ENOMEM;

    cpu_ids_len = data->tarpc_job_sched_param_data_u.
                    affinity.cpu_ids.cpu_ids_len;

    affinity->cpu_ids_len = cpu_ids_len;
    affinity->cpu_ids = TE_ALLOC(cpu_ids_len * sizeof(int));
    if (affinity->cpu_ids == NULL)
        return TE_ENOMEM;

    memcpy(affinity->cpu_ids,
           data->tarpc_job_sched_param_data_u.affinity.cpu_ids.cpu_ids_val,
           cpu_ids_len * sizeof(int));
    *af = affinity;
    return 0;
}

static te_errno
sched_priority_param_alloc(te_sched_priority_param **prio,
                           tarpc_job_sched_param_data *data)
{
    te_sched_priority_param *priority;
    priority = TE_ALLOC(sizeof(te_sched_priority_param));
    if (priority == NULL)
        return TE_ENOMEM;

    priority->priority = data->tarpc_job_sched_param_data_u.prio.priority;
    *prio = priority;
    return 0;
}

TARPC_FUNC_STATIC(job_create, {},
{
    char **argv = NULL;
    char **env = NULL;
    unsigned int i;

    if (in->argv.argv_len != 0)
    {
        argv = calloc(in->argv.argv_len, sizeof(*argv));
        if (argv == NULL)
            goto err;

        for (i = 0; i < in->argv.argv_len - 1; ++i)
        {
            argv[i] = strdup(in->argv.argv_val[i].str.str_val);
            if (argv[i] == NULL)
                goto err;
        }
    }

    if (in->env.env_len != 0)
    {
        env = calloc(in->env.env_len, sizeof(*env));
        if (env == NULL)
            goto err;

        for (i = 0; i < in->env.env_len - 1; ++i)
        {
            env[i] = strdup(in->env.env_val[i].str.str_val);
            if (env[i] == NULL)
                goto err;
        }
    }

    MAKE_CALL(out->retval = func(in->spawner, in->tool, argv, env,
                                 &out->job_id));
    out->common.errno_changed = FALSE;
    goto done;

err:
    out->common._errno = TE_RC(TE_RPCS, TE_ENOMEM);
    out->retval = -out->common._errno;
    for (i = 0; argv != NULL && argv[i] != NULL; i++)
        free(argv[i]);
    free(argv);
    for (i = 0; env != NULL && env[i] != NULL; i++)
        free(env[i]);
    free(env);
done:
    ;
})

TARPC_FUNC_STATIC(job_allocate_channels,
{
    COPY_ARG(channels);
},
{
    MAKE_CALL(out->retval = func(in->job_id, in->input_channels,
                                 in->n_channels, out->channels.channels_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_attach_filter, {},
{
    MAKE_CALL(out->retval = func(in->filter_name, in->channels.channels_len,
                                 in->channels.channels_val,
                                 in->readable, in->log_level, &out->filter));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_filter_add_regexp, {},
{
    MAKE_CALL(out->retval = func(in->filter, in->re, in->extract));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_start, {},
{
    MAKE_CALL(out->retval = func(in->job_id));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_receive, {},
{
    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val,
                                 in->timeout_ms, &out->buffer));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_receive_last, {},
{
    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val,
                                 in->timeout_ms, &out->buffer));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_receive_many, {},
{
    tarpc_job_buffer *bufs = NULL;
    unsigned int bufs_count = in->count;

    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val,
                                 in->timeout_ms, &bufs, &bufs_count));
    out->common.errno_changed = FALSE;

    out->buffers.buffers_val = bufs;
    out->buffers.buffers_len = bufs_count;
})

TARPC_FUNC_STATIC(job_clear, {},
{
    MAKE_CALL(out->retval = func(in->filters.filters_len,
                                 in->filters.filters_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_send, {},
{
    MAKE_CALL(out->retval = func(in->channel, in->buf.buf_len,
                                 in->buf.buf_val));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_poll, {},
{
    MAKE_CALL(out->retval = func(in->channels.channels_len,
                                 in->channels.channels_val,
                                 in->timeout_ms, FALSE));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_kill, {},
{
    MAKE_CALL(out->retval = func(in->job_id, signum_rpc2h(in->signo)));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_killpg, {},
{
    MAKE_CALL(out->retval = func(in->job_id, signum_rpc2h(in->signo)));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_wait, {},
{
    MAKE_CALL(out->retval = func(in->job_id, in->timeout_ms, &out->status));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_stop, {},
{
    MAKE_CALL(out->retval = func(in->job_id,
                                 signum_rpc2h(in->signo),
                                 in->term_timeout_ms));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_destroy, {},
{
    MAKE_CALL(out->retval = func(in->job_id, in->term_timeout_ms));
    out->common.errno_changed = FALSE;
})

TARPC_FUNC_STATIC(job_wrapper_add, {},
{
    char **argv = NULL;
    unsigned int i;

    if (in->argv.argv_len != 0)
    {
        argv = calloc(in->argv.argv_len, sizeof(*argv));
        if (argv == NULL)
            goto err;

        for (i = 0; i < in->argv.argv_len - 1; ++i)
        {
            argv[i] = strdup(in->argv.argv_val[i].str.str_val);
            if (argv[i] == NULL)
                goto err;
        }
    }

    MAKE_CALL(out->retval = func(in->tool, argv, in->job_id,
                                 in->priority, &out->wrapper_id));
    out->common.errno_changed = FALSE;
    goto done;

err:
    out->common._errno = TE_RC(TE_RPCS, TE_ENOMEM);
    out->retval = -out->common._errno;
    for (i = 0; argv != NULL && argv[i] != NULL; i++)
        free(argv[i]);
    free(argv);
done:
    ;
})

TARPC_FUNC_STATIC(job_wrapper_delete, {},
{
    MAKE_CALL(out->retval = func(in->job_id, in->wrapper_id));
})

TARPC_FUNC_STATIC(job_add_sched_param, {},
{
    te_sched_param *sched_param = NULL;
    te_sched_affinity_param *affinity = NULL;
    te_sched_priority_param *prio = NULL;
    int i;
    int len = in->param.param_len;
    te_errno rc;

    sched_param = TE_ALLOC((len + 1) * sizeof(te_sched_param));
    if (sched_param == NULL)
    {
        rc = TE_ENOMEM;
        goto err;
    }

    for (i = 0; i < len; i++)
    {
        switch (in->param.param_val[i].data.type)
        {
            case TARPC_JOB_SCHED_AFFINITY:
            {
                sched_param[i].type = TE_SCHED_AFFINITY;
                rc = sched_affinity_param_alloc(&affinity,
                                                &in->param.param_val[i].data);
                if (rc != 0)
                    goto err;

                sched_param[i].data = (void *)affinity;
                break;
            }

            case TARPC_JOB_SCHED_PRIORITY:
            {
                sched_param[i].type = TE_SCHED_PRIORITY;

                rc = sched_priority_param_alloc(&prio,
                                                &in->param.param_val[i].data);
                if (rc != 0)
                    goto err;

                sched_param[i].data = (void *)prio;
                break;
            }
            default:
                ERROR("Unsupported scheduling parameter");
                rc = TE_EINVAL;
                goto err;
        }
    }

    sched_param[len].type = TE_SCHED_END;
    sched_param[len].data = NULL;

    MAKE_CALL(out->retval = func(in->job_id, sched_param));
    out->common.errno_changed = FALSE;
    goto done;
err:
    free(sched_param);
    if (affinity != NULL)
    {
        free(affinity->cpu_ids);
        free(affinity);
    }
    free(prio);

    out->common._errno = TE_RC(TE_RPCS, rc);
    out->retval = -out->common._errno;
done:
    ;
})
