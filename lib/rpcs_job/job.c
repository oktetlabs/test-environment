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

typedef LIST_HEAD(job_list, job_t) job_list;

typedef struct job_t {
    LIST_ENTRY(job_t) next;

    unsigned int id;

    pid_t pid;
    char *spawner;
    char *tool;
    char **argv;
    char **env;
    char *cmd;
} job_t;

/* Static data */

static job_list all_jobs = LIST_HEAD_INITIALIZER(&all_jobs);

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

#undef DEF_GETTER

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

static void
job_stop(job_t *job, te_bool kill_process)
{
    if (kill_process)
        proc_kill(job->pid);

    job->pid = (pid_t)-1;
}

static te_errno
job_start(unsigned int id)
{
    pid_t pid;
    job_t *job;

    job = get_job(id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != (pid_t)-1)
    {
        ERROR("Job is already started");
        return TE_EPERM;
    }

    // TODO: use spawner method
    if ((pid = te_shell_cmd(job->cmd, -1, NULL, NULL, NULL)) < 0)
    {
        ERROR("Shell cmd failure\n");
        return te_rc_os2te(errno);
    }

    job->pid = pid;

    return 0;
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
    job_t *job;

    job = get_job(job_id);
    if (job == NULL)
        return TE_EINVAL;

    if (job->pid != (pid_t)-1)
        job_stop(job, TRUE);

    LIST_REMOVE(job, next);

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
