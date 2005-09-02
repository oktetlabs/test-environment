/** @file
 * @brief Unix Test Agent
 *
 * Unix Test Agent implementation.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 *
 * $Id$
 */

#define TE_LGR_USER      "Main"

#include "te_config.h"
#include "config.h"

#include <sys/time.h>
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/un.h>

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_shell_cmd.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_ta.h"

#include "unix_internal.h"


/** Send answer to the TEN */
#define SEND_ANSWER(_fmt...) \
    do {                                                                \
        int _rc;                                                        \
                                                                        \
        if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,  \
                             _fmt) >= (buflen - answer_plen))           \
        {                                                               \
            VERB("answer is truncated\n");                              \
        }                                                               \
        rcf_ch_lock();                                                  \
        _rc = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);     \
        rcf_ch_unlock();                                                \
        return _rc;                                                     \
    } while (FALSE)


/** Status of exited child. */
typedef struct ta_children_dead {
    pid_t               pid;        /**< PID of the child */
    int                 status;     /**< status of the child */
    struct timeval      timestamp;  /**< when child finished */
    te_bool             valid;      /**< is this entry valid? */
    int                 next;       /**< next id in the heap array */
    int                 prev;       /**< prev id in the heap array */
} ta_children_dead;

/** Reclaim to get status of child. */
typedef struct ta_children_wait {
    pid_t                    pid;    /**< PID to wait for */
    sem_t                    sem;    /**< semaphore to wake reclaimer */
    struct ta_children_wait *prev;   /**< pointer to the prev entry */
    struct ta_children_wait *next;   /**< pointer to the next entry */
} ta_children_wait;


extern void *rcf_ch_symbol_addr_auto(const char *name, te_bool is_func);
extern char *rcf_ch_symbol_name_auto(const void *addr);

/** Function to start RPC server after execve */
extern void tarpc_init(int argc, char **argv);


/** Logger entity name */
DEFINE_LGR_ENTITY("(unix)");

/** Test Agent executable name */
const char *ta_execname = NULL;
/** Test Agent name */
const char *ta_name = "(unix)";

/** Tasks to be killed during TA shutdown */
static unsigned int     tasks_len = 0;
static unsigned int     tasks_index = 0;
static pid_t           *tasks = NULL; 

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;


/** Length of pre-allocated list for dead children records. */
#define TA_CHILDREN_DEAD_MAX 1024

/** Head of the list with children statuses. */
static ta_children_dead ta_children_dead_heap[TA_CHILDREN_DEAD_MAX];

/** Is the heap initialized? */
static te_bool ta_children_dead_heap_inited = FALSE;

/** Best candidate for heap entry to be used */
static int ta_children_dead_heap_next = 0;

/** Head of dead children list */
static volatile int ta_children_dead_list = -1;

/** Head of ta_children_wait list */
static ta_children_wait *volatile ta_children_wait_list;

/** Read-write lock for both children lists. */
static pthread_mutex_t children_lock = PTHREAD_MUTEX_INITIALIZER;


/** Initialize ta_children_dead heap. */
static void
ta_children_dead_heap_init(void)
{
    int i;

    for (i = 0; i < TA_CHILDREN_DEAD_MAX; i++)
        ta_children_dead_heap[i].valid = FALSE;
    ta_children_dead_heap_inited = TRUE;
}

/** Add the talk pid into the list */
static void
store_pid(pid_t pid)
{
    unsigned int i;

    for (i = 0; i < tasks_index && tasks[i] != 0; i++);

    if (i < tasks_index)
    {
        tasks[i] = pid;
        return;
    }
    if (i == tasks_len)
    {
        tasks_len += 32;
        tasks = realloc(tasks, tasks_len * sizeof(pid_t));
        if (tasks == NULL)
        {
            ERROR("%s(): realloc() failed", __FUNCTION__);
            tasks_len = tasks_index = 0;
            return;
        }
    }
    tasks[tasks_index++] = pid;
}

/** Kill all tasks started using rcf_ch_start_task() */
static void
kill_tasks(void)
{
    unsigned int i;
    int          rc;

    for (i = 0; i < tasks_index; i++)
    {
        if (tasks[i] != 0)
        {
            rc = kill(-tasks[i], SIGTERM);
            RING("Sent SIGTERM to PID=%d - rc=%d, errno=%d",
                 -tasks[i], rc, (rc == 0) ? 0 : errno);
        }
    }
    sleep(1);
    for (i = 0; i < tasks_index; i++)
    {
        if (tasks[i] != 0)
        {
            rc = kill(-tasks[i], SIGKILL);
            RING("Sent SIGTERM to PID=%d - rc=%d, errno=%d",
                 -tasks[i], rc, (rc == 0) ? 0 : errno);
        }
    }
    free(tasks);
}

/* See description in rcf_ch_api.h */
int
rcf_ch_init()
{
    return 0;
}


/* See description in rcf_ch_api.h */
void
rcf_ch_lock()
{
    int rc = pthread_mutex_lock(&ta_lock);

    if (rc != 0)
        ERROR("%s(): pthread_mutex_lock() failed - rc=%d, errno=%d",
              __FUNCTION__, rc, errno);
}

/* See description in rcf_ch_api.h */
void
rcf_ch_unlock()
{
    int rc = pthread_mutex_unlock(&ta_lock);

    if (rc != 0)
        ERROR("%s(): pthread_mutex_unlock() failed - rc=%d, errno=%d",
              __FUNCTION__, rc, errno);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_shutdown(struct rcf_comm_connection *handle,
                char *cbuf, size_t buflen, size_t answer_plen)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    /* Standard handler is OK */
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_reboot(struct rcf_comm_connection *handle,
              char *cbuf, size_t buflen, size_t answer_plen,
              const uint8_t *ba, size_t cmdlen, const char *params)
{
    size_t len = answer_plen;
    
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(params);
    
    len += snprintf(cbuf + answer_plen,             
                     buflen - answer_plen, "0") + 1;
    rcf_ch_lock();                                  
    rcf_comm_agent_reply(handle, cbuf, len);         
    rcf_ch_unlock();                                
    
    ta_system("/sbin/reboot");
    return 0;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_configure(struct rcf_comm_connection *handle,
                 char *cbuf, size_t buflen, size_t answer_plen,
                 const uint8_t *ba, size_t cmdlen,
                 rcf_ch_cfg_op_t op, const char *oid, const char *val)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    /* Standard handler is OK */
    VERB("Configure: op %d OID <%s> val <%s>\n", op,
         oid == NULL ? "" : oid, val == NULL ? "" : val);
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_vread(struct rcf_comm_connection *handle,
             char *cbuf, size_t buflen, size_t answer_plen,
             rcf_var_type_t type, const char *var)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(type);
    UNUSED(var);

    /* Standard handler is OK */
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_vwrite(struct rcf_comm_connection *handle,
              char *cbuf, size_t buflen, size_t answer_plen,
              rcf_var_type_t type, const char *var, ...)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(type);
    UNUSED(var);

    /* Standard handler is OK */
    return -1;
}

/* See description in rcf_ch_api.h */
void *
rcf_ch_symbol_addr(const char *name, te_bool is_func)
{
    return rcf_ch_symbol_addr_auto(name, is_func);
}

/* See description in rcf_ch_api.h */
char *
rcf_ch_symbol_name(const void *addr)
{
    return rcf_ch_symbol_name_auto(addr);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_file(struct rcf_comm_connection *handle,
            char *cbuf, size_t buflen, size_t answer_plen,
            const uint8_t *ba, size_t cmdlen,
            rcf_op_t op, const char *filename)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(op);
    UNUSED(filename);

    /* Standard handler is OK */
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_call(struct rcf_comm_connection *handle,
            char *cbuf, size_t buflen, size_t answer_plen,
            const char *rtn, te_bool is_argv, int argc, void **params)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(rtn);
    UNUSED(is_argv);
    UNUSED(argc);
    UNUSED(params);

    /* Standard handler is OK */
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_start_task(struct rcf_comm_connection *handle,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  int priority, const char *rtn, te_bool is_argv,
                  int argc, void **params)
{
    void *addr = rcf_ch_symbol_addr(rtn, TRUE);
    int   pid;

    VERB("Start task handler is executed");

    if (addr != NULL)
    {
        VERB("fork process with entry point '%s'", rtn);

        if ((pid = fork()) == 0)
        {
            rcf_pch_detach();
            /* Set the process group to allow killing all children */
            setpgid(getpid(), getpid());
            logfork_register_user(rtn);
            if (is_argv)
                ((rcf_argv_rtn)(addr))(argc, (char **)params);
            else
                ((rcf_rtn)(addr))(params[0], params[1], params[2],
                                  params[3], params[4], params[5],
                                  params[6], params[7], params[8],
                                  params[9]);
            exit(0);
        }
        else if (pid > 0)
        {
            store_pid(pid);
            SEND_ANSWER("%d %d", 0, pid);
        }
        else
        {
            int save_errno = errno;

            ERROR("%s(): fork() failed", __FUNCTION__);
            SEND_ANSWER("%d", save_errno);
        }
        /* Unreachable */
        assert(0);
    }

    /* Try shell process */
    if (is_argv)
    {
        char check_cmd[RCF_MAX_PATH];

        sprintf(check_cmd,
                "TMP=`which %s 2>/dev/null` ; test -n \"$TMP\" ;", rtn);
        if (ta_system(check_cmd) != 0)
            SEND_ANSWER("%d", TE_ENOENT);

        if ((pid = fork()) == 0)
        {
            rcf_pch_detach();
            /* Set the process group to allow killing all children */
            setpgid(getpid(), getpid());
            logfork_register_user(rtn);
            execlp(rtn, rtn, params[0], params[1], params[2], params[3],
                             params[4], params[5], params[6], params[7],
                             params[8], params[9]);
            exit(0);
        }
        else if (pid < 0)
        {
            int save_errno = errno;

            ERROR("%s(): fork() failed", __FUNCTION__);
            SEND_ANSWER("%d", save_errno);
            /* Unreachable */
            assert(0);
        }
#ifdef HAVE_SYS_RESOURCE_H
        if (setpriority(PRIO_PROCESS, pid, priority) != 0)
        {
            ERROR("setpriority() failed - continue", errno);
            /* Continue with default priority */
        }
#else
        UNUSED(priority);
        ERROR("Unable to set task priority, ignore it.");
#endif
        store_pid(pid);
        SEND_ANSWER("%d %d", 0, pid);
        /* Unreachable */
        assert(0);
    }

    SEND_ANSWER("%d", TE_ENOENT);
}

struct rcf_thread_parameter {
    te_bool    active;
    pthread_t  id;
    void      *addr;
    te_bool is_argv;
    int        argc;
    void     **params;
    int        rc;
    te_bool    sem_created;
    sem_t params_processed;
};

#define TA_MAX_THREADS 16
static struct rcf_thread_parameter thread_pool[TA_MAX_THREADS];
static pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
rcf_ch_thread_wrapper(void *arg)
{
    struct rcf_thread_parameter *parm = arg;

    if (parm->is_argv)
        parm->rc = ((rcf_argv_thr_rtn)(parm->addr))(
                        &parm->params_processed, parm->argc, 
                        (char **)(parm->params));
    else
    {
        parm->rc = ((rcf_thr_rtn)(parm->addr))(&parm->params_processed,
                                               parm->params[0], 
                                               parm->params[1], 
                                               parm->params[2], 
                                               parm->params[3], 
                                               parm->params[4], 
                                               parm->params[5],
                                               parm->params[6], 
                                               parm->params[7], 
                                               parm->params[8], 
                                               parm->params[9]);
    }
    VERB("thread is terminating");
    pthread_mutex_lock(&thread_pool_mutex);
    parm->active = FALSE;
    pthread_mutex_unlock(&thread_pool_mutex);
    return NULL;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_start_task_thr(struct rcf_comm_connection *handle,
                      char *cbuf, size_t buflen, size_t answer_plen,
                      int priority, const char *rtn, te_bool is_argv,
                      int argc, void **params)
{
    void *addr = rcf_ch_symbol_addr(rtn, TRUE);
    
    struct rcf_thread_parameter *iter;


    UNUSED(priority);
    if (addr != NULL)
    {
        VERB("start thread with entry point '%s'", rtn);

        pthread_mutex_lock(&thread_pool_mutex);
        for (iter = thread_pool; 
             iter < thread_pool + TA_MAX_THREADS;
             iter++)
        {
            if (!iter->active)
            {
                int rc;

                iter->addr = addr;
                iter->argc = argc;
                iter->is_argv = is_argv;
                iter->params = params;
                iter->rc = 0;
                iter->id = 0;
                if (!iter->sem_created)
                {
                    sem_init(&iter->params_processed, FALSE, 0);
                    iter->sem_created = TRUE;
                }
                if ((rc = pthread_create(&iter->id, NULL, 
                                         rcf_ch_thread_wrapper, iter)) != 0)
                {
                    pthread_mutex_unlock(&thread_pool_mutex);
                    SEND_ANSWER("%d", rc);                    
                }
                VERB("started thread %d", iter - thread_pool);
                iter->active = TRUE;
                sem_wait(&iter->params_processed);
                pthread_mutex_unlock(&thread_pool_mutex);
                SEND_ANSWER("0 %d", (int)(iter - thread_pool));
            }
        }
        pthread_mutex_unlock(&thread_pool_mutex);
        SEND_ANSWER("%d", TE_ETOOMANY);
    }

    SEND_ANSWER("%d", TE_ENOENT);
}

/* Kill all the threads started by rcf_ch_start_task_thr */
static void 
kill_threads(void)
{
    struct rcf_thread_parameter *iter;

    for (iter = thread_pool; 
         iter < thread_pool + TA_MAX_THREADS;
         iter++)
    {
        if (iter->active)
        {
            pthread_cancel(iter->id);
            pthread_join(iter->id, NULL);
        }
    }
}

/* See description in rcf_ch_api.h */
int
rcf_ch_kill_task(struct rcf_comm_connection *handle,
                 char *cbuf, size_t buflen, size_t answer_plen,
                 unsigned int pid)
{
    int             kill_errno = 0;
    unsigned int    i;
    int             p = pid;

    for (i = 0; i < tasks_index; i++)
    {
        if (tasks[i] == (pid_t)pid)
        {
            tasks[i] = 0;
            p = -pid;
            break;
        }
    }

    if (kill(p, SIGTERM) != 0)
    {
        kill_errno = errno;
        ERROR("Failed to send SIGTERM to process with PID=%u: %d",
              pid, kill_errno);

        if (kill(p, SIGKILL) != 0)
        {
            ERROR("Failed to send SIGKILL to process with PID=%u: %d",
                  pid, errno);
        }
        else
        {
            RING("Sent SIGKILL to PID=%u", pid);
        }
    }
    else
    {
        RING("Sent SIGTERM to PID=%u", pid);
    }

    SEND_ANSWER("%d", kill_errno);
}



/**
 * Routine to be executed remotely to run any program from shell.
 *
 * @param argc      number of arguments in array
 * @param argv      array with pointer to string arguments
 *
 * @return Status code.
 *
 * @todo Use system-dependent maximum command line length.
 */
int
shell(int argc, char * const argv[])
{
    char            cmdbuf[2048];
    unsigned int    cmdlen = sizeof(cmdbuf);
    int             i;
    unsigned int    used = 0;
    int             rc;
    
    for (i = 0; (i < argc) && (used < cmdlen); ++i)
    {
        used += snprintf(cmdbuf + used, cmdlen - used, "%s ", argv[i]);
    }
    if (used >= cmdlen)
        return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);

    VERB("SHELL: run %s, errno before the run is %d\n", cmdbuf, errno);
    rc = ta_system(cmdbuf);
    
    if (rc == -1)
    {
        int err = errno;
        
        VERB("The command fails with errno %d\n", err);
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    VERB("Successfully completes");

#ifdef WCOREDUMP
    if (WCOREDUMP(rc))
        ERROR("Command executed in shell dumped core");
#endif
    if (!WIFEXITED(rc))
        ERROR("Abnormal termination of command executed in shell");

    /* FIXME */
    return TE_RC(TE_TA_UNIX, WEXITSTATUS(rc));
}


/**
 * Restart system service.
 *
 * @param service       name of the service (e.g. dhcpd)
 *
 * @return 0 (success) or system error
 */
int
restart_service(char *service)
{
    char cmd[RCF_MAX_PATH];
    int  rc;

    sprintf(cmd, "/etc/rc.d/init.d/%s restart", service);
    rc = ta_system(cmd);
    if (rc < 0)
        rc = TE_EPERM;

    /* FIXME */
    return TE_RC(TE_TA_UNIX, rc);
}

/**
 * Create a file with specified size filled by the specified pattern.
 *
 * @return 0 (success) or system error
 */
int
create_data_file(char *pathname, char c, int len)
{
    char  buf[1024];
    FILE *f;
    
    if ((f = fopen(pathname, "w")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);
    
    memset(buf, c, sizeof(buf));
    
    while (len > 0)
    {
        int copy_len = ((unsigned int)len  > sizeof(buf)) ?
                           (int)sizeof(buf) : len;

        if ((copy_len = fwrite((void *)buf, sizeof(char), copy_len, f)) < 0)
        {
            fclose(f);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        len -= copy_len;
    }
    if (fclose(f) < 0)
    {
        ERROR("fclose() failed errno=%d", errno);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/* Declarations of routines to be linked with the agent */
int
ta_rtn_unlink(char *arg)
{
    int rc = unlink(arg);

    VERB("%s(): arg=%s rc=%d errno=%d", __FUNCTION__,
         (arg == NULL) ? "(null)" : arg, rc, errno);
    return (rc == 0) ? 0 : errno;
}


/**
 * Signal handler to be registered for SIGINT signal.
 * 
 * @param sig   Signal number
 */
static void
ta_sigint_handler(int sig)
{
    /*
     * We can't log here using TE logging facilities, but we need
     * to make a mark, that TA was killed.
     */
    fprintf(stderr, "Test Agent killed by %d signal\n", sig);
    exit(EXIT_FAILURE);
}

/**
 * Signal handler to be registered for SIGPIPE signal.
 * 
 * @param sig   Signal number
 */
static void
ta_sigpipe_handler(int sig)
{
    fprintf(stderr, "SIGPIPE is received\n");
    UNUSED(sig);
}

/** TRUE if first timeval is less than second */
#define TV_LESS(_le, _ge) \
    ((_le).tv_sec < (_ge).tv_sec ||                               \
     ((_le).tv_sec == (_ge).tv_sec && (_le).tv_usec < (_ge).tv_usec))

/** Is logger available in signal handler? */
static inline te_bool
is_logger_available(void)
{
    ta_lgr_lock_key key;

    if (ta_lgr_trylock(key) != 0)
    {
        fprintf(stderr, "Logger is locked, drop the message\n");
        return FALSE;
    }
    (void)ta_lgr_unlock(key);
    return TRUE;
}

/**
 * Wait for a child and log its exit status information.
 */
static void
ta_sigchld_handler(int sig)
{
    int     status;
    int     pid;
    int     get = 0;
    te_bool logger = is_logger_available();

    UNUSED(sig);
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
        int               dead = ta_children_dead_heap_next;
        ta_children_wait *wake;

        get++;
        if (get > 1 && logger)
            WARN("Get %d children from on SIGCHLD handler call", get);
        while (ta_children_dead_heap[dead].valid)
        {
            int next;

            next = (dead + 1) % TA_CHILDREN_DEAD_MAX;
            if (next == ta_children_dead_heap_next)
            {
                struct timeval tv;
                int i;

                tv = ta_children_dead_heap[dead = 0].timestamp;
                for (i = 1; i < TA_CHILDREN_DEAD_MAX; i++)
                {
                    if (TV_LESS(ta_children_dead_heap[i].timestamp, tv))
                        tv = ta_children_dead_heap[dead = i].timestamp;
                }
                if (ta_children_dead_heap[dead].prev == -1 && logger)
                {
                    ERROR("List of dead child statuses "
                          "is in inconsistent state: "
                          "oldest entry has no prev, but heap is full.");
                }
                else 
                {
                    ta_children_dead_heap[
                        ta_children_dead_heap[dead].prev].next = -1;
                    if (ta_children_dead_heap[dead].next != -1 && logger)
                    {
                        ERROR("List of dead child statuses "
                              "is in inconsistent state: "
                              "oldest entry is not the last.");
                    }
                }
                if (logger)
                {
                    WARN("Removing oldest entry with pid = %d, "
                         "status = 0x%x from the list of dead children.", 
                         ta_children_dead_heap[dead].pid,
                         ta_children_dead_heap[dead].status);
                }
                break;
            }
            dead = next;
        }
        ta_children_dead_heap_next = (dead + 1) % TA_CHILDREN_DEAD_MAX;

        ta_children_dead_heap[dead].pid = pid;
        ta_children_dead_heap[dead].status = status;
        gettimeofday(&ta_children_dead_heap[dead].timestamp, NULL);
        ta_children_dead_heap[dead].valid = TRUE;
        ta_children_dead_heap[dead].prev = -1;
        if (ta_children_dead_list != -1)
            ta_children_dead_heap[ta_children_dead_list].prev = dead;
        ta_children_dead_heap[dead].next = ta_children_dead_list;
        ta_children_dead_list = dead;

        for (wake = ta_children_wait_list; wake != NULL; wake = wake->next)
        {
            if (wake->pid == pid)
                sem_post(&wake->sem);
        }

        /* Now try to log status of the child */
        if (!logger)
            continue;
        if (WIFEXITED(status))
        {
            RING("Child process with PID %d exited with value %d", 
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
            RING("No child was available in SIGCHILD handler");
        }
        else
        {
            ERROR("waitpid() failed with errno %d", errno);
        }
    }
}

sigset_t rpcs_received_signals;


/* See description in unix_internal.h */
void
signal_registrar(int signum)
{
    sigaddset(&rpcs_received_signals, signum);
}


/*
 * TCE support
 */

int (*tce_stop_function)(void);
int (*tce_notify_function)(void);
int (*tce_get_peer_function)(void);
const char *(*tce_get_conn_function)(void);

static void 
init_tce_subsystem(void)
{
    tce_stop_function =
        rcf_ch_symbol_addr("tce_stop_collector", TRUE);
    tce_notify_function =
        rcf_ch_symbol_addr("tce_notify_collector", TRUE);
    tce_get_peer_function = 
        rcf_ch_symbol_addr("tce_obtain_principal_peer_id", TRUE);
    tce_get_conn_function = 
        rcf_ch_symbol_addr("tce_obtain_principal_connect", TRUE);
}

/**
 * Cleans up dead children list.  May be called at startup/after fork only,
 * because it does not lock the list.
 */
static void
ta_children_cleanup()
{
    ta_children_dead_list = -1;
    ta_children_dead_heap_init();
    while (ta_children_wait_list != NULL)
    {
        ta_children_wait *wake;
        wake = ta_children_wait_list;
        ta_children_wait_list = wake->next;
        free(wake);
    }
    pthread_mutex_init(&children_lock, NULL);
}

/**
 * Find an entry about dead child and remove it from the list.
 * This function is to be called from waitpid.
 * It should be called with lock held.
 *
 * @param pid pid of process to find or -1 to find any pid
 *
 * @return entry number with required pid or -1
 */
static inline int
find_dead_child(pid_t pid)
{
    int     dead;

    for (dead = ta_children_dead_list;
         dead != -1;
         dead = ta_children_dead_heap[dead].next)
    {
        if (ta_children_dead_heap[dead].pid == pid || pid == -1)
        {
            if (ta_children_dead_heap[dead].prev != -1)
            {
                ta_children_dead_heap[
                    ta_children_dead_heap[dead].prev].next = 
                    ta_children_dead_heap[dead].next;
            }
            else
            {
                ta_children_dead_list = ta_children_dead_heap[dead].next;
            }
            break;
        }
    }

    return dead;
}

/* See description in unix_internal.h */
pid_t
ta_waitpid(pid_t pid, int *status, int options)
{
    int         dead;
    int         rc;

    ta_children_wait  *wake;

    if (!ta_children_dead_heap_inited)
        ta_children_dead_heap_init();
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
    if ((wake = malloc(sizeof(ta_children_wait))) == NULL)
    {
        ERROR("%s: Out of memory", __FUNCTION__);
    }

/**
 * Log that a function returned impossible value.
 * @todo there thould be ERROR, not PRINT, but I'd like to see this log
 * from RPC.
 */
#define IMPOSSIBLE_LOG_AND_RET(_func) \
    do {                                                    \
        PRINT("%s: Impossible! " #_func " retured %d: %s",  \
              __FUNCTION__, rc, strerror(errno));           \
        free(wake);                                         \
        return -1;                                          \
    } while(0)
#define LOCK \
    do {                                                \
        rc = pthread_mutex_lock(&children_lock);        \
        if (rc != 0)                                    \
            IMPOSSIBLE_LOG_AND_RET(pthread_mutex_lock); \
    } while(0)
#define UNLOCK \
    do {                                                    \
        rc = pthread_mutex_unlock(&children_lock);          \
        if (rc != 0)                                        \
            IMPOSSIBLE_LOG_AND_RET(pthread_mutex_unlock);   \
    } while(0)

    /* Lock both volatile lists. */
    rc = pthread_mutex_lock(&children_lock);
    if (rc != 0)
    {
        if (errno != EINVAL)
            IMPOSSIBLE_LOG_AND_RET(pthread_mutex_lock);
        ERROR("%s: dead_child_lock mutex is not initialized in %d", 
              __FUNCTION__, getpid());
        rc = pthread_mutex_init(&children_lock, NULL);
        if (rc != 0)
            IMPOSSIBLE_LOG_AND_RET(pthread_mutex_init);
        LOCK;
    }

    /* Add an entry to ta_children_wait, so signal handler can wake us. */
    if (!(options & WNOHANG))
    {
        wake->pid = pid;
        rc = sem_init(&wake->sem, 0, 0);
        if (rc != 0)
        {
            pthread_mutex_unlock(&children_lock);
            IMPOSSIBLE_LOG_AND_RET(sem_init);
        }
        wake->prev = NULL;
        wake->next = ta_children_wait_list;
        ta_children_wait_list = wake;
        if (wake->next != NULL)
            wake->next->prev = wake;
    }
    UNLOCK; LOCK; /* @todo is it really necessary? */

    /* Find dead child entry */
    dead = find_dead_child(pid);
    UNLOCK;

    /* Sleep */
    if (!(options & WNOHANG))
    {
        while (dead == -1 && (rc = sem_wait(&wake->sem)) != 0)
        {
            if (errno != EINTR)
                IMPOSSIBLE_LOG_AND_RET(sem_wait);
            /* Really, if it is "our" signal (SIGCHLD), we can try to 
             * call find_dead_child, but we should not free(wake) before
             * signal handler will call sem_post(), so let's sleep until
             * ta_sigchld_handler will wake up us explicitly. */
        }

        /* Clean up wait queue and release the lock */
        LOCK;
        if (wake->prev != NULL)
            wake->prev->next = wake->next;
        else
            ta_children_wait_list = wake->next;
        free(wake);
        if (dead == -1)
            dead = find_dead_child(pid);
        UNLOCK;
    }

    /* Get the results. */
    if (dead != -1)
    {
        if (status != NULL)
            *status = ta_children_dead_heap[dead].status;
        ta_children_dead_heap[dead].valid = FALSE;
    }
    return dead == -1 ? -1 : pid;
}
#undef LOCK
#undef UNLOCK
#undef IMPOSSIBLE_LOG_AND_RET

/* See description in unix_internal.h */
int 
ta_system(const char *cmd)
{
    pid_t   pid = te_shell_cmd(cmd, -1, NULL, NULL);
    int     status;
        
    if (pid < 0)
        return -1;
    ta_waitpid(pid, &status, 0);
    return status;
}

/* See description in unix_internal.h */
int
ta_kill_death(pid_t pid)
{
    int rc;

    if (ta_waitpid(pid, NULL, WNOHANG) == pid)
        return 0;
    rc = kill(pid, SIGTERM);
    if (rc != 0 && errno != ESRCH)
        return -1;

    /* Check if the process exited. If kill failed, waitpid can't fail */
    if (ta_waitpid(pid, NULL, WNOHANG) == pid)
        return 0;
    else if (rc != 0)
        return -1;

    /* Wait for termination. */
    MSLEEP(500);
    kill(pid, SIGKILL);
    ta_waitpid(pid, NULL, 0);
    return 0;
}

/** Print environment to the console */
int
env(void)
{
    return ta_system("env");
}



/**
 * Method for generating stream of data.
 * Prototype is according with callback type 'tad_stream_callback'
 *
 * @param offset        offset in stream of first byte in buffer
 * @param length        length of buffer
 * @param buffer        location for generated data (OUT)
 *
 * @return status code
 */
int
arithm_progr(uint64_t offset, uint32_t length, uint8_t *buffer)
{
    unsigned i;

    if (buffer == NULL)
        return TE_EWRONGPTR;

    for (i = 0; i < length; i++)
        buffer[i] = offset + i;

    return 0;
}


/**
 * Entry point of the Unix Test Agent.
 *
 * Usage:
 *     taunix <ta_name> <communication library configuration string>
 */
int
main(int argc, char **argv)
{
    int rc, retval = 0;
    
    pthread_t tid;
    
    char buf[16];
    
    chdir("/tmp");
    
    if (argc < 3)
    {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    ta_execname = argv[0];

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);    

    (void)signal(SIGINT, ta_sigint_handler);
    (void)signal(SIGPIPE, ta_sigpipe_handler);

    sigemptyset(&rpcs_received_signals);
    
#ifdef RCF_RPC
    /* After execve */
    if (strcmp(argv[1], "rpcserver") == 0)
    {
        tarpc_init(argc, argv);
        return 0;
    }
#endif

    if ((rc = log_init()) != 0)
    {
        fprintf(stderr, "log_init() failed: error=%d\n", rc);
        return rc;
    }
    
    te_lgr_entity = ta_name = argv[1];

    (void)signal(SIGCHLD, ta_sigchld_handler);
    pthread_atfork(NULL, NULL, ta_children_cleanup);

    RING("Starting");

    sprintf(buf, "PID %u", getpid());

    pthread_create(&tid, NULL, (void *)logfork_entry, NULL);

    init_tce_subsystem();
    
    rc = rcf_pch_run(argv[2], buf);
    if (rc != 0)
    {
        fprintf(stderr, "rcf_pch_run() failed: error=%d\n", rc);
        if (retval == 0)
            retval = rc;
    }

    unlink(argv[0]);

    if (tce_stop_function != NULL)
        tce_stop_function();

    kill_tasks();
    kill_threads();

    rc = log_shutdown();
    if (rc != 0)
    {
        fprintf(stderr, "log_shutdown() failed: error=%d\n", rc);
        if (retval == 0)
            retval = rc;
    }
    
    return retval;
}

