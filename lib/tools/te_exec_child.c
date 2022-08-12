/** @file
 * @brief Routines to execute a program in a child process
 *
 * Auxiluary routines to execute a program in a child process
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TE Exec Child"

#include "te_config.h"

#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "logger_api.h"

#include "te_exec_child.h"
#include "te_string.h"
#include "te_errno.h"

#define VALID_FD_PTR(ptr) ((ptr) != NULL && (ptr) != TE_EXEC_CHILD_DEV_NULL_FD)

static void
maybe_open_dev_null(const int *fd_arg, te_bool input, int *dev_null_fd)
{
    int flags = input ? O_RDONLY : O_WRONLY;

    if (fd_arg == TE_EXEC_CHILD_DEV_NULL_FD)
    {
        if ((*dev_null_fd = open("/dev/null", flags)) < 0)
            ERROR("Failed to open /dev/null");
    }
}

/**
 * Create a pipe, set close-on-exec flag on a descriptor that will
 * be used (meaning will remain opened after the call) by parent process
 * (write end of input pipe or read end of output pipe).
 */
static int
pipe_cloexec_parent_end(int pipe_fd[2], te_bool input)
{
    int rc;
    int result[2];
    int parent_end;

    if ((rc = pipe(result)) != 0)
        return rc;

    parent_end = input ? result[1] : result[0];

    if ((rc = fcntl(parent_end, F_SETFD, FD_CLOEXEC)) < 0)
    {
        close(result[0]);
        close(result[1]);
    }

    pipe_fd[0] = result[0];
    pipe_fd[1] = result[1];

    return 0;
}

static te_errno
printf_affinity(pid_t pid)
{
    cpu_set_t mask;
    te_string cpu_list = TE_STRING_INIT;
    long nproc;
    te_errno te_rc = 0;
    int i;

    if (sched_getaffinity(pid, sizeof(mask), &mask) < 0)
    {
        WARN("Failed to get pid %d's affinity: %s", getpid(), strerror(errno));
        te_rc = TE_EFAIL;
        goto out;
    }

    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    for (i = 0; i < nproc; i++)
    {
        if(CPU_ISSET(i, &mask) != 0)
        {
            if (te_string_append(&cpu_list, "%d ", i) != 0)
            {
                WARN("Failed to generate cpu list");
                te_rc = TE_EFAIL;
                goto out;
            }
        }
    }

    RING("pid %d's new affinity mask: %s", pid, cpu_list.ptr);

out:
    te_string_free(&cpu_list);

    return te_rc;
}

static te_errno
set_affinity(pid_t pid, const te_sched_affinity_param *affinity)
{
    int rc;
    cpu_set_t mask;
    size_t i;

    CPU_ZERO(&mask);

    for (i = 0; i < affinity->cpu_ids_len; i++)
        CPU_SET(affinity->cpu_ids[i], &mask);

    rc = sched_setaffinity(pid, sizeof(mask), &mask);
    if (rc < 0)
    {
        int tmp_err = errno;
        WARN("Failed to set pid %d's affinity: %s", getpid(),
             strerror(tmp_err));
        return te_rc_os2te(tmp_err);
    }

    printf_affinity(pid);
    return 0;
}

static te_errno
set_priority(const te_sched_priority_param *prio)
{
    int rc;
    errno = 0;

    rc = nice(prio->priority);
    if (rc == -1 && errno != 0)
    {
        int tmp_err = errno;
        WARN("Cannot set niceness to %d: %s", prio->priority,
             strerror(tmp_err));
        return te_rc_os2te(tmp_err);
    }
    else
    {
        RING("pid %d's new niceness: %d", getpid(), rc);
    }
    return 0;
}

static te_errno
add_sched_param(pid_t pid, const te_sched_param *sched_param)
{
    te_errno rc_nice = 0;
    te_errno rc_affinity = 0;
    const te_sched_param *iter;

    for (iter = sched_param; iter->type != TE_SCHED_END; iter++)
    {
        switch (iter->type)
        {
            case TE_SCHED_AFFINITY:
            {
#if defined(HAVE_SCHED_SETAFFINITY) && defined(HAVE_SCHED_GETAFFINITY)
                const te_sched_affinity_param *affinity = iter->data;

                rc_affinity = set_affinity(pid, affinity);
#else
                WARN("It's impossible to set CPU affinity for this platform");
                rc_affinity = TE_ENOSYS;
#endif
                break;
            }

            case TE_SCHED_PRIORITY:
            {
                const te_sched_priority_param *prio = iter->data;

                rc_nice = set_priority(prio);
                break;
            }

            default:
                ERROR("Unsupported scheduling parameter. type = %d", iter->type);
                return TE_EINVAL;
        }
    }

    if (rc_affinity != 0)
        return rc_affinity;
    return rc_nice;
}

pid_t
te_exec_child(const char *file, char *const argv[],
              char *const envp[], uid_t uid, int *in_fd,
              int *out_fd, int *err_fd,
              const te_sched_param *sched_param)
{
    int   pid;
    int   in_pipe[2], out_pipe[2], err_pipe[2];

    if (file == NULL || argv == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    if (VALID_FD_PTR(in_fd) && pipe_cloexec_parent_end(in_pipe, TRUE) != 0)
        return -1;
    if (VALID_FD_PTR(out_fd) && pipe_cloexec_parent_end(out_pipe, FALSE) != 0)
    {
        if (VALID_FD_PTR(in_fd))
        {
            close(in_pipe[0]);
            close(in_pipe[1]);
        }
        return -1;
    }
    if (VALID_FD_PTR(err_fd) && pipe_cloexec_parent_end(err_pipe, FALSE) != 0)
    {
        if (VALID_FD_PTR(in_fd))
        {
            close(in_pipe[0]);
            close(in_pipe[1]);
        }
        if (VALID_FD_PTR(out_fd))
        {
            close(out_pipe[0]);
            close(out_pipe[1]);
        }
        return -1;
    }

    pid = fork();
    if (pid == 0)
    {
        int pipe_fd[3] = { 0, 1, 2};
        int busy_fd[6] = { -1, -1, -1, -1, -1, -1};
        int dev_null_fd[3] = { -1, -1, -1};
        int i;
        int rc;

        /* Set us to be the process leader */
        setpgid(getpid(), getpid());
        if (uid != (uid_t)(-1) && setuid(uid) != 0)
        {
            ERROR("Failed to set user %d before runing program \"%s\"",
                  uid, file);
        }

        maybe_open_dev_null(in_fd, TRUE, &dev_null_fd[0]);
        maybe_open_dev_null(out_fd, FALSE, &dev_null_fd[1]);
        maybe_open_dev_null(err_fd, FALSE, &dev_null_fd[2]);

        /* Find file descriptors */
        if (VALID_FD_PTR(in_fd))
        {
            close(in_pipe[1]);
            pipe_fd[0] = in_pipe[0];
        }
        else if (in_fd == TE_EXEC_CHILD_DEV_NULL_FD && dev_null_fd[0] >= 0)
        {
            dup2(dev_null_fd[0], STDIN_FILENO);
        }

        if (VALID_FD_PTR(out_fd))
        {
            close(out_pipe[0]);
            pipe_fd[1] = out_pipe[1];
        }
        else if (out_fd == TE_EXEC_CHILD_DEV_NULL_FD && dev_null_fd[1] >= 0)
        {
            dup2(dev_null_fd[1], STDOUT_FILENO);
        }

        if (VALID_FD_PTR(err_fd))
        {
            close(err_pipe[0]);
            pipe_fd[2] = err_pipe[1];
        }
        else if (err_fd == TE_EXEC_CHILD_DEV_NULL_FD && dev_null_fd[2] >= 0)
        {
            dup2(dev_null_fd[2], STDERR_FILENO);
        }

        for (i = 0; i < 3; i++)
        {
            if (dev_null_fd[i] >= 0)
                close(dev_null_fd[i]);
        }

        for (i = 0; i < 3; i++)
        {
            if (pipe_fd[i] >= 0 && pipe_fd[i] < 6)
                busy_fd[pipe_fd[i]] = i;
        }

        /* Dup file descriptors where necessary */
        for (i = 0; i < 3; i++)
        {
            if (busy_fd[i] == -1)
            {
                dup2(pipe_fd[i], i);
                close(pipe_fd[i]);
                pipe_fd[i] = i;
            }
        }

        /* Most likely, all pipe fds are duped correctly at this point */
        if (pipe_fd[0] != 0 || pipe_fd[1] != 1 || pipe_fd[2] != 2)
        {
            WARN("This part of code in %s is not debugged properly. "
                 "Tell sasha@oktetlabs.ru if it is really triggered",
                 __FUNCTION__);
            for (i = 0; i < 3; i++)
            {
                if (pipe_fd[i] != i && pipe_fd[i] < 3)
                {
                    /*
                     * Free 0,1,2 fds by duping them into 4,5,6
                     * busy_fd is now correct in 3-5 interval only
                     */
                    int j;

                    for (j = 3; j < 6; j++)
                    {
                        if (busy_fd[j] == -1)
                        {
                            dup2(pipe_fd[i], j);
                            close(pipe_fd[i]);
                            busy_fd[j] = i;
                            pipe_fd[i] = j;
                            break;
                        }
                    }
                }
            }
            for (i = 0; i < 3; i++)
            {
                if (pipe_fd[i] != i)
                {
                    dup2(pipe_fd[i], i);
                    close(pipe_fd[i]);
                }
            }
        }

        if (sched_param != NULL)
        {
            rc = add_sched_param(0, sched_param);
            if (rc != 0)
                WARN("Failed to set process settings");
        }

        if (envp == NULL)
            execvp(file, argv);
        else
            execvpe(file, argv, envp);

        /* Terminate child process in case of exec failure */
        ERROR("%s: execvp[e](%s) failed: %s", __func__, file, strerror(errno));
        _exit(EXIT_FAILURE);
    }

    if (VALID_FD_PTR(in_fd))
        close(in_pipe[0]);
    if (VALID_FD_PTR(out_fd))
        close(out_pipe[1]);
    if (VALID_FD_PTR(err_fd))
        close(err_pipe[1]);
    if (pid < 0)
    {
        if (VALID_FD_PTR(in_fd))
        {
            close(in_pipe[1]);
            *in_fd = -1;
        }
        if (VALID_FD_PTR(out_fd))
        {
            close(out_pipe[0]);
            *out_fd = -1;
        }
        if (VALID_FD_PTR(err_fd))
        {
            close(err_pipe[0]);
            *err_fd = -1;
        }
    }
    else
    {
        if (VALID_FD_PTR(in_fd))
            *in_fd = in_pipe[1];
        if (VALID_FD_PTR(out_fd))
            *out_fd = out_pipe[0];
        if (VALID_FD_PTR(err_fd))
            *err_fd = err_pipe[0];
    }
    return pid;
}
