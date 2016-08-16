/** @file
 * @brief Agent library
 *
 * Process management routines
 *
 *
 * Copyright (C) 2004-2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Alexandra N. Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER      "Agent library"

#include "agentlib.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#include "te_queue.h"
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_shell_cmd.h"
#include "te_sleep.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logger_ta_lock.h"
#include "logfork.h"

/** Status of exited child. */
typedef struct ta_children_dead {
    SLIST_ENTRY(ta_children_dead) links;    /**< dead children linek list */

    pid_t                       pid;        /**< PID of the child */
    int                         status;     /**< status of the child */
    struct timeval              timestamp;  /**< when child finished */
    te_bool                     valid;      /**< is this entry valid? */
} ta_children_dead;

/** Length of pre-allocated list for dead children records. */
#define TA_CHILDREN_DEAD_MAX 128

/** Head of the list with children statuses. */
static ta_children_dead ta_children_dead_heap[TA_CHILDREN_DEAD_MAX];

/** Is the heap initialized? */
static te_bool ta_children_dead_heap_inited = FALSE;

/** Head of empty entries list */
SLIST_HEAD(, ta_children_dead) ta_children_dead_pool;

/** Head of dead children list */
SLIST_HEAD(, ta_children_dead) ta_children_dead_list;

static sem_t sigchld_sem;

/** Initialize ta_children_dead heap. */
static void
ta_children_dead_heap_init(void)
{
    int i;

    SLIST_INIT(&ta_children_dead_pool);
    SLIST_INIT(&ta_children_dead_list);

    for (i = 0; i < TA_CHILDREN_DEAD_MAX; i++)
    {
        ta_children_dead *dead = &ta_children_dead_heap[i];
        memset(dead, 0, sizeof(ta_children_dead));
        SLIST_INSERT_HEAD(&ta_children_dead_pool, dead, links);
        dead->valid = FALSE;
    }
    ta_children_dead_heap_inited = TRUE;
}

/** Is logger available in signal handler? */
static inline te_bool
is_logger_available(void)
{
    ta_log_lock_key key;

    if (ta_log_trylock(&key) != 0)
        return FALSE;

    (void)ta_log_unlock(&key);
    return TRUE;
}

/**
 * Logs death of a child. This function SHOULD be called after waitpid() to
 * log exist status.
 *
 * @param pid       pid of the dead child
 * @param status    status of the death child
 */
static void
log_child_death(int pid, int status)
{
    if (WIFEXITED(status))
    {
        INFO("Child process with PID %d exited with value %d",
             pid, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        if (WTERMSIG(status) == SIGTERM)
            RING("Child process with PID %d was terminated", pid);
        else
        {
            WARN("Child process with PID %d is killed "
                 "by the signal %d", pid, WTERMSIG(status));
        }
    }
#ifdef WCOREDUMP
    else if (WCOREDUMP(status))
        ERROR("Child process with PID %d core dumped", pid);
#endif
    else
    {
        WARN("Child process with PID %d exited due to unknown reason",
             pid);
    }
}

/**
 * Wait for a child and log its exit status information.
 *
 * @note It is declared as non-static to be visible in TA symbol table.
 */
/* static, see above */ void
ta_sigchld_handler(void)
{
    int     status;
    int     pid;
    int     get = 0;
    te_bool logger = is_logger_available();
    int     saved_errno = errno;

    /*
     * we can't wait for the semaphore in signal handler,
     * so we exit and the moment the sema is released the handler
     * should be called by responsible context
     */
    if (sem_trywait(&sigchld_sem) < 0)
    {
        errno = saved_errno;
        return;
    }

    if (!ta_children_dead_heap_inited)
        ta_children_dead_heap_init();

    /*
     * Some system may loose SIGCHLD, so we should catch all uncatched
     * children. From other side, if a system does not loose SIGCHLD,
     * it may be that all children were catched by previous call of this
     * handler.
     */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        ta_children_dead *dead = NULL;
        ta_children_dead *oldest = NULL;

        errno = saved_errno;
        get++;
        if (get > 1 && logger)
            WARN("Get %d children from on SIGCHLD handler call", get);

        for (dead = SLIST_FIRST(&ta_children_dead_list);
             dead != NULL; dead = SLIST_NEXT(dead, links))
        {
            /*
             * if we have a valid dead child with same pid, it means it's
             * dead for ages and must be replaced by his younger dead
             * brother
             */
            oldest = dead; /* Oldest entry is always the last */
            if ((pid == dead->pid) && dead->valid)
            {
                WARN("Removing obsoleted entry with the same pid = %d, "
                     "status = 0x%x from the list of dead children.",
                     dead->pid, dead->status);
                SLIST_REMOVE(&ta_children_dead_list, dead,
                             ta_children_dead, links);
                break;
            }
        }

        if (dead == NULL)
        {
            /*
             * Entry with specified pid is not found.
             * Allocate new entry from pool
             */
            dead = SLIST_FIRST(&ta_children_dead_pool);
            if (dead != NULL)
            {
                SLIST_REMOVE(&ta_children_dead_pool, dead,
                             ta_children_dead, links);
            }
        }

        if (dead == NULL)
        {
            /*
             * Pool is already empty. Free the oldest entry in the list.
             */
            dead = oldest;
            INFO("Removing oldest entry with pid = %d, status = 0x%x "
                 "from the list of dead children.",
                 dead->pid, dead->status);
            SLIST_REMOVE(&ta_children_dead_list, dead,
                         ta_children_dead, links);
        }

        dead->pid = pid;
        dead->status = status;
        dead->valid = TRUE;
        gettimeofday(&dead->timestamp, NULL);
        SLIST_INSERT_HEAD(&ta_children_dead_list, dead, links);

        /* Now try to log status of the child */
        if (logger)
            log_child_death(pid, status);
    }

    if (logger && get == 0)
    {
        /*
         * Linux behaviour:
         * - if the process has children, but none of them is zombie,
         *   we will get 0.
         * - if there is no children at all, we will get -1 with ECHILD.
         */
        if (pid == 0 || errno == ECHILD)
        {
            INFO("No child was available in SIGCHILD handler");
            errno = saved_errno;
        }
        else
        {
            ERROR("waitpid() failed with errno %d", errno);
        }
    }
    else
        errno = saved_errno;

    sem_post(&sigchld_sem);
}

/**
 * Find an entry about dead child and remove it from the list.
 * This function is to be called from waitpid.
 * It should be called with lock held.
 *
 * @param pid    pid of process to find or -1 to find any pid
 * @param status status of the process to return
 *
 * @return TRUE is a child was found, FALSE overwise.
 */
static te_bool
find_dead_child(pid_t pid, int *status)
{
    ta_children_dead *dead = NULL;

    if (!ta_children_dead_heap_inited)
        ta_children_dead_heap_init();

    sem_wait(&sigchld_sem);
    for (dead = SLIST_FIRST(&ta_children_dead_list);
         dead != NULL; dead = SLIST_NEXT(dead, links))
    {
        if (dead->pid == pid || pid == -1)
        {
            SLIST_REMOVE(&ta_children_dead_list, dead,
                         ta_children_dead, links);
            SLIST_INSERT_HEAD(&ta_children_dead_pool, dead, links);

            *status = dead->status;
            dead->valid = FALSE;
            break;
        }

        /* Note, we should not ever get here */
        if (!dead->valid)
        {
            WARN("%s: invalid pid in the list", __FUNCTION__);
            dead = NULL;
            break;
        }
    }

    sem_post(&sigchld_sem);
    /* call handler to find out if we have any unhandled signals
     * when sem is locked */
    ta_sigchld_handler();

    return dead != NULL;
}

/* See description in unix_internal.h */
/* FIXME: Possible use after free in the function */
pid_t
ta_waitpid(pid_t pid, int *p_status, int options)
{
    int     rc;
    int     status;
    int     saved_errno = errno;

    if (pid < -1 || pid == 0)
    {
        ERROR("%s: process groups are not supported.", __FUNCTION__);
        errno = EINVAL;
        return -1;
    }

    if (options & ~WNOHANG)
    {
        ERROR("%s: only WNOHANG option is supported.", __FUNCTION__);
        errno = EINVAL;
        return -1;
    }

    /* Start race: who'll get the status, our waitpid() or SIGCHILD?
     * We are ready to handle both cases! */
    status = 0;
#ifndef SA_RESTART
    /*
     * If SA_RESTART is not defined, waitpid() will not be resumed and
     * it will fail with 'EINTR' errno. This will happen any time we
     * receive a signal about child termination (any we registered for
     * handling of this signal).
     */
    do {
#endif
        rc = waitpid(pid, &status, options);
#ifndef SA_RESTART
    } while ((rc < 0) && (errno == EINTR));
#endif

    if (rc > 0)
    {
        /* We've got the real status */
        int old_status;
        log_child_death(pid, status);

        /* If we already have a status from the same pid, remove it. */
        find_dead_child(pid, &old_status);

        if (p_status != NULL)
            *p_status = status;
        return rc;
    }
    else if (rc < 0)
    {
        if (errno == EINTR)
            return rc;

        assert(errno == ECHILD);
        errno = saved_errno;
        /* The child is probably dead, get the status from the list */

        if (find_dead_child(pid, &status))
        {
            if (p_status != NULL)
                *p_status = status;
            return pid;
        }
        /* No such child */
        errno = ECHILD;
        return -1;
    }

    /* rc == 0 */
    assert(options & WNOHANG);
    return 0;

}

/* See description in unix_internal.h */
int
ta_system(const char *cmd)
{
    pid_t   pid = te_shell_cmd(cmd, -1, NULL, NULL, NULL);
    int     status = -1;

    if (pid <= 0)
        return -1;

    ta_waitpid(pid, &status, 0);

    return status;
}

te_errno
ta_popen_r(const char *cmd, pid_t *cmd_pid, FILE **f)
{
    int   out_fd = -1;
    int   status;
    int   rc = 0;

    *cmd_pid = te_shell_cmd(cmd, -1, NULL, &out_fd, NULL);
    if (*cmd_pid < 0)
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    if ((*f = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("Failed to obtain file pointer for shell command output");
        rc = TE_OS_RC(TE_TA_UNIX, te_rc_os2te(errno));
        close(out_fd);

        ta_waitpid(*cmd_pid, &status, 0);
        if (!WIFEXITED(status))
        {
            ERROR("%s(): '%s' was not terminated normally: %d",
                  __FUNCTION__, cmd, status);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
    }

    return rc;
}

te_errno
ta_pclose_r(pid_t cmd_pid, FILE *f)
{
    int rc = 0;
    int status;

    if (fclose(f) < 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);

    ta_waitpid(cmd_pid, &status, 0);
    if (!WIFEXITED(status))
    {
        ERROR("%s(): proccess with pid %d was not terminated normally: %d",
              __FUNCTION__, cmd_pid, status);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    return rc;
}

/* See description in unix_internal.h */
int
ta_kill_death(pid_t pid)
{
    int rc;
    int saved_errno = errno;

    if (ta_waitpid(pid, NULL, WNOHANG) == pid)
        return 0;
    rc = kill(-pid, SIGTERM);
    if (rc != 0 && errno != ESRCH)
        return -1;
    errno = saved_errno;

    /* Wait for termination. */
    te_msleep(500);

    /* Check if the process exited. If kill failed, waitpid can't fail */
    if (ta_waitpid(pid, NULL, WNOHANG) == pid)
        return 0;
    else if (rc != 0)
        return -1;

    /* Wait for termination. */
    te_msleep(500);
    kill(-pid, SIGKILL);
    ta_waitpid(pid, NULL, 0);
    return 0;
}

te_errno
ta_process_mgmt_init(void)
{
    te_errno rc = 0;
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(sigact));
#ifdef SA_RESTART
    sigact.sa_flags = SA_RESTART;
#endif
    sigemptyset(&sigact.sa_mask);

    if (sem_init(&sigchld_sem, 0, 1) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Can't initialize sigchld sem: %r");
    }
    /* FIXME: Is it used by RPC */
    sigact.sa_handler = (void *)ta_sigchld_handler;
    if (sigaction(SIGCHLD, &sigact, NULL) != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Cannot set SIGCHLD action: %r");
    }
#ifdef HAVE_PTHREAD_H
    pthread_atfork(NULL, NULL, ta_children_dead_heap_init);
#endif

    return rc;
}

typedef struct vfork_hook {
    SLIST_ENTRY(vfork_hook) next;    /**< next hook */
    void (*hook[VFORK_HOOK_N_PHASES])(void);
} vfork_hook;

static SLIST_HEAD(, vfork_hook) vfork_hook_list;

/* See description in agentlib.h */
te_errno
register_vfork_hook(void (*prepare)(void), void (*child)(void),
                    void (*parent)(void))
{
    vfork_hook *new_hook = TE_ALLOC(sizeof(*new_hook));

    if (new_hook == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    new_hook->hook[VFORK_HOOK_PHASE_PREPARE] = prepare;
    new_hook->hook[VFORK_HOOK_PHASE_CHILD]   = child;
    new_hook->hook[VFORK_HOOK_PHASE_PARENT]  = parent;
    
    SLIST_INSERT_HEAD(&vfork_hook_list, new_hook, next);

    return 0;
}

void
run_vfork_hooks(enum vfork_hook_phase phase)
{
    const vfork_hook *hook;

    assert(phase >= VFORK_HOOK_PHASE_PREPARE && phase < VFORK_HOOK_N_PHASES);

    SLIST_FOREACH(hook, &vfork_hook_list, next)
    {
        if (hook->hook[phase] != NULL)
            hook->hook[phase]();
    }
}

