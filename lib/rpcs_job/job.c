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

#include "te_shell_cmd.h"
#include "te_sleep.h"
#include "te_string.h"

#include "rpc_server.h"
#include "te_errno.h"

#include "agentlib.h"

#define PROC_WAIT_US 1000
#define KILL_TIMEOUT_MS 10

#define MAX_OUTPUT_CHANNELS_PER_JOB 32
#define MAX_INPUT_CHANNELS_PER_JOB 32
#define MAX_FILTERS_PER_CHANNEL 32

#define MAX_MESSAGE_DATA_SIZE 8192

typedef LIST_HEAD(filter_list, filter_t) filter_list;

typedef struct filter_t {
    LIST_ENTRY(filter_t) next;

    unsigned int id;

    int ref_count;
    te_log_level log_level;
    char *name;
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
    };
} channel_t;

typedef LIST_HEAD(job_list, job_t) job_list;

typedef struct job_t {
    LIST_ENTRY(job_t) next;

    unsigned int id;

    channel_t *out_channels[MAX_OUTPUT_CHANNELS_PER_JOB];
    unsigned int n_out_channels;
    channel_t *in_channels[MAX_INPUT_CHANNELS_PER_JOB];
    unsigned int n_in_channels;

    pid_t pid;
    char *spawner;
    char *tool;
    char **argv;
    char **env;
    char *cmd;
} job_t;

#define CTRL_PIPE_INITIALIZER {-1, -1}

/* Static data */

static job_list all_jobs = LIST_HEAD_INITIALIZER(&all_jobs);
static channel_list all_channels = LIST_HEAD_INITIALIZER(&all_channels);
static filter_list all_filters = LIST_HEAD_INITIALIZER(&all_filters);

static pthread_mutex_t channels_lock = PTHREAD_MUTEX_INITIALIZER;
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

#undef DEF_GETTER

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
abandoned_descriptors_close()
{
    size_t i;

    for (i = 0; i < n_abandoned_descriptors; i++)
        close(abandoned_descriptors[i]);

    n_abandoned_descriptors = 0;
}

#define CTRL_MESSAGE ("c\n")

static te_errno
ctrl_pipe_create()
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
ctrl_pipe_destroy()
{
    if (ctrl_pipe[0] > -1)
        close(ctrl_pipe[0]);
    if (ctrl_pipe[1] > -1)
        close(ctrl_pipe[1]);

    ctrl_pipe[0] = -1;
    ctrl_pipe[1] = -1;
}

static int
ctrl_pipe_get_read_fd()
{
    return ctrl_pipe[0];
}

static te_errno
ctrl_pipe_send()
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

    *channel = result;

    return 0;
}

static te_errno
filter_alloc(const char *filter_name, te_log_level log_level, filter_t **filter)
{
    filter_t *result = calloc(1, sizeof(*result));

    if (result == NULL)
    {
        ERROR("Filter allocation failed");
        return TE_ENOMEM;
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
    result->ref_count = 0;
    result->log_level = log_level;

    *filter = result;

    return 0;
}

static void
filter_destroy(filter_t *filter)
{
    filter->ref_count--;
    if (filter->ref_count > 0)
        return;

    LIST_REMOVE(filter, next);

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

static te_errno
build_command(const char *tool, char **argv, char **command)
{
    te_string result = TE_STRING_INIT;
    unsigned int i;
    te_errno rc;

    if ((rc = te_string_append(&result, "%s ", tool)) != 0)
    {
        te_string_free(&result);
        return rc;
    }

    for (i = 0; argv[i] != NULL; i++)
    {
        if ((rc = te_string_append(&result, "%s ", argv[i])) != 0)
        {
            te_string_free(&result);
            return rc;
        }
    }

    *command = result.ptr;

    return 0;
}

/* Note: argv and env MUST remain valid after a successful call */
static te_errno
job_alloc(const char *spawner, const char *tool, char **argv,
          char **env, job_t **job)
{
    job_t *result = calloc(1, sizeof(*result));
    te_errno rc;

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

    if ((rc = build_command(tool, argv, &result->cmd)) != 0)
    {
        ERROR("Failed to build command-line string");
        free(result->tool);
        free(result->spawner);
        free(result);
        return rc;
    }
    result->pid = (pid_t)-1;
    result->argv = argv;
    result->env = env;
    result->n_out_channels = 0;
    result->n_in_channels = 0;

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
    free(job->cmd);
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

te_errno
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

static void
proc_kill(pid_t pid)
{
    tarpc_job_status dummy;

    /* Don't consider kill() failure if the call failed with ESRCH */
    if (kill(pid, SIGTERM) < 0 && errno != ESRCH)
        WARN("Process kill(%d, SIGTERM) failed: %s", pid, strerror(errno));

    if (proc_wait(pid, KILL_TIMEOUT_MS, &dummy) != 0)
        ERROR("Failed to wait for killed process");
}

static te_errno
match_callback(filter_t *filter, unsigned int channel_id, size_t size,
               char *buf, te_bool eos)
{
    int log_size = size > (size_t)INT_MAX ? INT_MAX : (int)size;

    if (!eos)
    {
        LOG_MSG(filter->log_level, "Filter '%s':\n%.*s",
                filter->name == NULL ? "Unnamed" : filter->name, log_size, buf);
    }

    return 0;
}

static te_errno
filter_exec(filter_t *filter, unsigned int channel_id, size_t size, char *buf)
{
    te_bool eos = (size == 0);

    return match_callback(filter, channel_id, size, buf, eos);
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
                              read_c, buf)) != 0)
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

static void *
thread_work_loop(void *arg)
{
    fd_set rfds;
    int select_rc;

    UNUSED(arg);

    while (1)
    {
        int ctrl_fd = ctrl_pipe_get_read_fd();
        int max_fd = ctrl_fd;
        channel_t *channel;

        FD_ZERO(&rfds);
        FD_SET(ctrl_fd, &rfds);

        pthread_mutex_lock(&channels_lock);

        LIST_FOREACH(channel, &all_channels, next)
        {
            if (!channel->closed && channel->fd > -1)
            {
                if (!channel->is_input_channel)
                {
                    if (channel->fd > max_fd)
                        max_fd = channel->fd;

                    FD_SET(channel->fd, &rfds);
                }
            }
        }

        pthread_mutex_unlock(&channels_lock);

        select_rc = select(max_fd + 1, &rfds, NULL, NULL, NULL);

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
            }

            pthread_mutex_unlock(&channels_lock);
        }
    }

    return NULL;
}

static void
job_stop(job_t *job, te_bool kill_process)
{
    if (kill_process)
        proc_kill(job->pid);

    job->pid = (pid_t)-1;
}

static te_errno
thread_start()
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
        stderr_fd_p = NULL;
    if (job->n_out_channels < 1)
        stdout_fd_p = NULL;
    if (job->n_in_channels < 1)
        stdin_fd_p = NULL;

    // TODO: use spawner method
    if ((pid = te_shell_cmd(job->cmd, -1, stdin_fd_p, stdout_fd_p,
                            stderr_fd_p)) < 0)
    {
        ERROR("Shell cmd failure\n");
        return te_rc_os2te(errno);
    }

    if (stdout_fd >= FD_SETSIZE || stderr_fd >= FD_SETSIZE ||
        stdin_fd >= FD_SETSIZE)
    {
        ERROR("Failed to start a job, file descriptor limit exceeded");
        close_valid(stdout_fd);
        close_valid(stderr_fd);
        close_valid(stdin_fd);
        job_stop(job, TRUE);
        return TE_EMFILE;
    }

    pthread_mutex_lock(&channels_lock);

    if ((rc = ctrl_pipe_send()) != 0)
    {
        pthread_mutex_unlock(&channels_lock);
        close_valid(stdout_fd);
        close_valid(stderr_fd);
        close_valid(stdin_fd);
        job_stop(job, TRUE);
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
            job_stop(job, TRUE);
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
                         unsigned int *channels, te_log_level log_level,
                         unsigned int *filter_id)
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
            return TE_EPERM;
        }
    }

    if ((rc = filter_alloc(filter_name, log_level, &filter)) != 0)
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
                  unsigned int *channels, te_log_level log_level,
                  unsigned int *filter_id)
{
    te_errno rc;

    pthread_mutex_lock(&channels_lock);

    rc = job_attach_filter_unsafe(filter_name, n_channels, channels,
                                  log_level, filter_id);

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
    {
        ERROR("Job is not running");
        return TE_EPERM;
    }

    if (kill(job->pid, signo) < 0)
    {
        ERROR("kill() failed");
        return te_rc_os2te(errno);
    }

    return 0;
}

static te_errno
job_wait(unsigned int job_id, int timeout_ms, tarpc_job_status *status)
{
    job_t *job;
    te_errno rc;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid < 0)
    {
        ERROR("Job is not running");
        return TE_EPERM;
    }

    if ((rc = proc_wait(job->pid, timeout_ms, status)) == 0)
        job_stop(job, FALSE);

    return rc;
}

static te_errno
job_destroy(unsigned int job_id)
{
    unsigned int i;
    job_t *job;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != (pid_t)-1)
        job_stop(job, TRUE);

    LIST_REMOVE(job, next);

    pthread_mutex_lock(&channels_lock);

    for (i = 0; i < job->n_out_channels; i++)
        job->out_channels[i]->job = NULL;
    for (i = 0; i < job->n_in_channels; i++)
        job->in_channels[i]->job = NULL;

    pthread_mutex_unlock(&channels_lock);

    job_dealloc(job);

    return 0;
}

TARPC_FUNC_STATIC(job_create, {},
{
    char **argv = NULL;
    char **env = NULL;
    unsigned int i;

    argv = calloc(in->argv.argv_len + 1, sizeof(*argv));
    if (argv == NULL)
        goto err;

    for (i = 0; i < in->argv.argv_len; ++i)
    {
        argv[i] = strdup(in->argv.argv_val[i].str);
        if (argv[i] == NULL)
            goto err;
    }

    env = calloc(in->env.env_len + 1, sizeof(*env));
    if (env == NULL)
        goto err;

    for (i = 0; i < in->env.env_len; ++i)
    {
        env[i] = strdup(in->env.env_val[i].str);
        if (env[i] == NULL)
            goto err;
    }

    MAKE_CALL(out->retval = func(in->spawner, in->tool, argv, env,
                                 &out->job_id));
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
})

TARPC_FUNC_STATIC(job_attach_filter, {},
{
    MAKE_CALL(out->retval = func(in->filter_name, in->channels.channels_len,
                                 in->channels.channels_val,
                                 in->log_level, &out->filter));
})

TARPC_FUNC_STATIC(job_start, {},
{
    MAKE_CALL(out->retval = func(in->job_id));
})

TARPC_FUNC_STATIC(job_kill, {},
{
    MAKE_CALL(out->retval = func(in->job_id, signum_rpc2h(in->signo)));
})

TARPC_FUNC_STATIC(job_wait, {},
{
    MAKE_CALL(out->retval = func(in->job_id, in->timeout_ms, &out->status));
})

TARPC_FUNC_STATIC(job_destroy, {},
{
    MAKE_CALL(out->retval = func(in->job_id));
})
