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

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS   64

#include "te_config.h"
#include "config.h"

#include <sys/time.h>
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/un.h>
#if defined (__QNX__)
#include <spawn.h>
#endif
#ifdef _SYS_QUEUE_H_
#include <sys/queue.h>
#else
#include "te_queue.h"
#endif

#ifdef __linux__
#include <elf.h>
#endif

#if defined(HAVE_AIO_H)
#include <aio.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_shell_cmd.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logger_ta_lock.h"
#include "logfork.h"
#include "ta_common.h"
#include "te_sleep.h"

#include "unix_internal.h"
#include "rpc_server.h"
#include "tarpc.h"
#include "te_rpc_errno.h"
#include "te_rpc_signal.h"

#include "te_kernel_log.h"

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
        RCF_CH_LOCK;                                                    \
        _rc = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);     \
        RCF_CH_UNLOCK;                                                  \
        return _rc;                                                     \
    } while (FALSE)

/** User environment */
extern char **environ;

/** Logger entity name */
DEFINE_LGR_ENTITY("(unix)");

/** Test Agent executable name */
/**
 * This is done to avoid possible problems
 * with build conficts in test suites due to
 * changed definition of this variable.
 */
char ta_execname_storage[RCF_MAX_PATH];
const char *ta_execname = ta_execname_storage;

/** Test Agent name */
const char *ta_name = "(unix)";
/** Test Agent data and binaries location */ 
char ta_dir[RCF_MAX_PATH];

#if __linux__
const char *ta_tmp_path = "/tmp/";
#else
const char *ta_tmp_path = "/usr/tmp/";
#endif

#if __linux__
/** Test Agent vsyscall page entrance */
const void *vsyscall_enter = NULL;
#endif

#if defined(ENABLE_GENERATED_SYMTBL)
extern rcf_symbol_entry generated_table[];
#endif

static rcf_symbol_entry essential_symbols[] = {
   {.name = "socket",       .addr = (void *)socket,       .is_func = TRUE},
   {.name = "bind",         .addr = (void *)bind,         .is_func = TRUE},
   {.name = "select",       .addr = (void *)select,       .is_func = TRUE},
   {.name = "connect",      .addr = (void *)connect,      .is_func = TRUE},
   {.name = "listen",       .addr = (void *)listen,       .is_func = TRUE},
   {.name = "accept",       .addr = (void *)accept,       .is_func = TRUE},
   {.name = "send",         .addr = (void *)send,         .is_func = TRUE},
   {.name = "sendto",       .addr = (void *)sendto,       .is_func = TRUE},
   {.name = "recv",         .addr = (void *)recv,         .is_func = TRUE},
   {.name = "read",         .addr = (void *)read,         .is_func = TRUE},
   {.name = "write",        .addr = (void *)write,        .is_func = TRUE},
   {.name = "close",        .addr = (void *)close,        .is_func = TRUE},
   {.name = "waitpid",      .addr = (void *)waitpid,      .is_func = TRUE},
   {.name = "te_shell_cmd", .addr = (void *)te_shell_cmd, .is_func = TRUE},
   {.name = "getsockname",  .addr = (void *)getsockname,  .is_func = TRUE},
   {.name = "poll",         .addr = (void *)poll,         .is_func = TRUE},
#if defined (HAVE_AIO_H)
   {.name = "aio_read",     .addr = (void *)aio_read,     .is_func = TRUE},
   {.name = "aio_error",    .addr = (void *)aio_error,    .is_func = TRUE},
   {.name = "aio_return",   .addr = (void *)aio_return,   .is_func = TRUE},
   {.name = "aio_write",    .addr = (void *)aio_write,    .is_func = TRUE},
   {.name = "aio_suspend",  .addr = (void *)aio_suspend,  .is_func = TRUE},
   {.name = "aio_cancel",   .addr = (void *)aio_cancel,   .is_func = TRUE},
   {.name = "lio_listio",   .addr = (void *)lio_listio,   .is_func = TRUE},
#endif
   {.name = NULL, .addr = NULL}
};

/** Tasks to be killed during TA shutdown */
static unsigned int     tasks_len = 0;
static unsigned int     tasks_index = 0;
static pid_t           *tasks = NULL; 

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;

/** Default SIGINT action */
static struct sigaction sigaction_int;
/** Default SIGPIPE action */
static struct sigaction sigaction_pipe;

/** Add the task pid into the list */
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
    
    if (tasks_index == 0)
        return;

    for (i = 0; i < tasks_index; i++)
    {
        if (tasks[i] != 0)
        {
            rc = kill(-tasks[i], SIGTERM);
            if (!(rc == -1 && errno == ESRCH))
            {
                LOG_PRINT("Sent SIGTERM to PID=%ld - rc=%d, errno=%d",
                          (long)-tasks[i], rc, (rc == 0) ? 0 : errno);
            }
            else 
                tasks[i] = 0;
        }
    }
    usleep(100000);
    for (i = 0; i < tasks_index; i++)
    {
        if (tasks[i] != 0)
        {
            rc = kill(-tasks[i], SIGKILL);
            LOG_PRINT("Sent SIGKILL to PID=%ld - rc=%d, errno=%d",
                      (long)-tasks[i], rc, (rc == 0) ? 0 : errno);
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
    {
        LOG_PRINT("%s(): pthread_mutex_lock() failed - rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }
}

/* See description in rcf_ch_api.h */
void
rcf_ch_unlock()
{
    int rc;

    rc = pthread_mutex_trylock(&ta_lock);
    if (rc == 0)
    {
        WARN("rcf_ch_unlock() without rcf_ch_lock()!\n"
             "It may happen in the case of asynchronous cancellation.");
    }
    else if (rc != EBUSY)
    {
        LOG_PRINT("%s(): pthread_mutex_trylock() failed - rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }

    rc = pthread_mutex_unlock(&ta_lock);
    if (rc != 0)
    {
        LOG_PRINT("%s(): pthread_mutex_unlock() failed - rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }
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
    RCF_CH_LOCK;
    rcf_comm_agent_reply(handle, cbuf, len);         
    RCF_CH_UNLOCK;
    
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

    if ((type == RCF_STRING) &&
        (strcmp(var, "rcf_consistency_checks") == 0))
    {
        /*
         * Value @b rcf_consistency_checks is generated by the script
         * @b te_rcf_consistency_checks
         */
        extern const char * const  rcf_consistency_checks;
        SEND_ANSWER("0 %s", rcf_consistency_checks);
        return 0;
    }

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
int
rcf_ch_file(struct rcf_comm_connection *handle,
            char *cbuf, size_t buflen, size_t answer_plen,
            const uint8_t *ba, size_t cmdlen,
            rcf_op_t op, const char *filename)
{

    UNUSED(ba);

#ifdef AUX_BUFFER_LEN
#undef AUX_BUFFER_LEN
#endif
#define AUX_BUFFER_LEN       65536

/** Prefix for file operations in /proc directory */
#ifdef RCF_FILE_PROC_PREFIX
#undef RCF_FILE_PROC_PREFIX
#endif
#define RCF_FILE_PROC_PREFIX  "/proc/"

    size_t     reply_buflen = buflen - answer_plen;
    int        rc;
    int        fd = -1;
    int8_t    *auxbuf = NULL;
    int8_t    *auxbuf_p = NULL;
    size_t     procfile_len = 0;


    if (strncmp(RCF_FILE_PROC_PREFIX, filename,
                strlen(RCF_FILE_PROC_PREFIX)) == 0)
    {
        if ((auxbuf = malloc(AUX_BUFFER_LEN)) == NULL)
        {
            ERROR("Impossible allocate buffer");
            rc = TE_RC(TE_RCF_PCH, TE_ENOMEM);
            goto reject;
        }

        VERB("file operation in '/proc/'");
        if (op != RCFOP_FGET)
        {
            ERROR("Unsupported file operation in '/proc/': %d", op);
            rc = TE_RC(TE_RCF_PCH, TE_EPERM);
            goto reject;
        }

        fd = open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
        if (fd < 0)
        {
            ERROR("Failed to open file '%s'", filename);
            rc = TE_RC(TE_RCF_PCH, TE_ENOENT);
            goto reject;
        }

        /*
         * We should read /proc/filename before replying to requester
         * because we can not know the length of /proc/filename
         * in advice. If the number of bytes returned by read()
         * equal to AUX_BUFFER_LEN then possibly there was more bytes
         * to read.
         */
        rc = read(fd, auxbuf, AUX_BUFFER_LEN);
        if (rc < 0)
        {
            rc = TE_OS_RC(TE_RCF_PCH, errno);
            ERROR("rcfpch", "read(/proc/...) failed %r", rc);
            goto reject;
        }
        else if (rc == AUX_BUFFER_LEN)
        {
            WARN("Because of insufficient buffer length only part of "
                 "data retrieved from %s", filename);
        }
        procfile_len = (size_t)rc;

        if ((size_t)snprintf(cbuf + answer_plen, reply_buflen,
                    "0 attach %u", (unsigned int)procfile_len)
                >= reply_buflen)
        {
            ERROR("Command buffer too small for reply");
            rc = TE_RC(TE_RCF_PCH, TE_E2BIG);
            goto reject;
        }

        RCF_CH_LOCK;
        rc = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);

        auxbuf_p = auxbuf;
        while ((rc == 0) && (procfile_len > 0))
        {
            int len = (procfile_len > buflen) ? buflen : procfile_len;

            procfile_len -= len;
            memcpy(cbuf, auxbuf_p, len);
            auxbuf_p += len;
            rc = rcf_comm_agent_reply(handle, cbuf, len);
        }
        RCF_CH_UNLOCK;
        close(fd);

        free(auxbuf);

        EXIT("%r", rc);
        return rc;
reject:
        free(auxbuf);
        if (fd != -1)
            close(fd);
        if (cmdlen > buflen)
        {
            int     error;
            size_t  rest;

            do {
                rest = reply_buflen;
                error = rcf_comm_agent_wait(handle, cbuf + answer_plen,
                        &rest, NULL);
                if (error != 0 && TE_RC_GET_ERROR(error) != TE_EPENDING)
                {
                    return TE_RC(TE_RCF_PCH, error);
                }
            } while (error != 0);
        }
        EXIT("%r", rc);
        SEND_ANSWER("%d", rc);
    } else
    {
        /* Standard handler is OK */
        return -1;
    }
#undef AUX_BUFFER_LEN
#undef RCF_FILE_PROC_PREFIX
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
rcf_ch_start_process(pid_t *pid, 
                    int priority, const char *rtn, te_bool do_exec,
                    int argc, void **params)
{
    void *addr = rcf_ch_symbol_addr(rtn, TRUE);

    VERB("Start task handler is executed");

    if (addr != NULL)
    {
        VERB("fork process with entry point '%s'", rtn);

#if !defined (__QNX__)
        if ((*pid = fork()) == 0)
        {
            /* Set the process group to allow killing all children */
            setpgid(getpid(), getpid());
            logfork_register_user(rtn);
            if (do_exec)
            {
                const char *argv[30];

                /* work around for fork/pthread problem: do exec */
                if (argc > 25)
                {
                    ERROR("Too many arguments for %s, "
                          "increase constant in %s %d", 
                          __FILE__, __LINE__);
                    return TE_RC(TE_TA_UNIX, TE_E2BIG);
                }
                
                logfork_delete_user(getpid(), thread_self());
                memset(argv, 0, sizeof(argv));
                argv[0] = ta_execname;
                argv[1] = "exec";
                argv[2] = rtn;
                memcpy(argv + 3, params, argc * sizeof(void *));

                execve(ta_execname, (char * const *)argv, 
                       (char * const *)environ);

                assert(errno != 0);
                return TE_OS_RC(TE_TA_UNIX, errno);
            }
            else
            {
                ((rcf_rtn)(addr))(params[0], params[1], params[2],
                                  params[3], params[4], params[5],
                                  params[6], params[7], params[8],
                                  params[9]);
                exit(0);
            }
        }
        else 
#else
        if (do_exec)
        {
            /*
             * In QNX we can't do 'fork' from a thread
             * (we have a separate logger thread right now), which
             * is why we do 'spawn' call here.
             */
            struct inheritance  inh;
            const char         *argv[30];

            memset(&inh, 0, sizeof(inh));
            memset(argv, 0, sizeof(argv));
            argv[0] = ta_execname;
            argv[1] = "exec";
            argv[2] = rtn;
            memcpy(argv + 3, params, argc * sizeof(void *));

            *pid = spawn(ta_execname,
                         0, /* int fd_count */
                         NULL, /* const int fd_map[ ] */
                         &inh,
                         argv, NULL);
        }
        else
        {
            /*
             * Actually it is easy to support and*/
            ERROR("NOT do_exec (direct function call "
                  "in a separate process is not supported)");
            *pid = 0;
            return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        }
#endif /* __QNX__ */
        if (*pid > 0)
        {
            store_pid(*pid);
            return 0;
        }
        else
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("%s(): fork() failed: %r", __FUNCTION__, rc);

            return rc;
        }
    }

    /* Try shell process */
    if (do_exec || argc == 0)
    {
        char check_cmd[RCF_MAX_PATH];

        sprintf(check_cmd,
                "TMP=`which %s 2>/dev/null` ; test -n \"$TMP\" ;", rtn);
        if (ta_system(check_cmd) != 0)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        if ((*pid = fork()) == 0)
        {
            /* Set the process group to allow killing all children */
            setpgid(getpid(), getpid());
            logfork_register_user(rtn);
            /* FIXME */
            if (argc == 10)
                execlp(rtn, rtn, 
                   params[0], params[1], params[2], params[3], params[4], 
                   params[5], params[6], params[7], params[8], params[9],
                   (const char *)NULL);
            exit(0);
        }
        else if (*pid < 0)
        {
            int rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("%s(): fork() failed", __FUNCTION__);
            
            return rc;
        }
#ifdef HAVE_SYS_RESOURCE_H
        if (setpriority(PRIO_PROCESS, *pid, priority) != 0)
        {
            ERROR("setpriority() failed - continue", errno);
            /* Continue with default priority */
        }
#else
        UNUSED(priority);
        WARN("Unable to set task priority, ignore it.");
#endif
        store_pid(*pid);
        
        return 0;
    }
    
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

struct rcf_thread_parameter {
    te_bool    active;
    pthread_t  id;
    void      *addr;
    te_bool is_argv;
    int        argc;
    void     **params;
    te_errno   rc;
    te_bool    sem_created;
    sem_t      params_processed;
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
rcf_ch_start_thread(int *tid,
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
                    return TE_OS_RC(TE_TA_UNIX, rc);
                }
                VERB("started thread %d", iter - thread_pool);
                iter->active = TRUE;
                sem_wait(&iter->params_processed);
                pthread_mutex_unlock(&thread_pool_mutex);
                *tid = (int)(iter - thread_pool);
                return 0;
            }
        }
        pthread_mutex_unlock(&thread_pool_mutex);
        return TE_RC(TE_TA_UNIX, TE_ETOOMANY);
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
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
rcf_ch_kill_process(unsigned int pid)
{
    int            rc = 0;
    int            rc1 = 0;
    unsigned int   i;
    int            p = pid;
    int            tries = 0;

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
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Failed to send SIGTERM to process with PID=%u: %r",
              pid, rc);
    }
    else
    {
        RING("Sent SIGTERM to PID=%u", pid);
    }
    if (rc == 0)
        while ((rc1 = kill(p, 0)) == 0 && tries < 10)
        {
            usleep(10000);
            tries++;
        }
    if (rc != 0 || rc1 == 0 || (rc1 == -1 && errno != ESRCH))
    {
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

    return rc;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_free_proc_data(unsigned int pid)
{
    unsigned int   i;

    for (i = 0; i < tasks_index; i++)
    {
        if (tasks[i] == (pid_t)pid)
        {
            tasks[i] = 0;
            break;
        }
    }

    return 0;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_kill_thread(unsigned int tid)
{
    if (pthread_cancel((pthread_t)tid) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);
    else
    {
        if (pthread_join((pthread_t)tid, NULL) != 0)
            return TE_OS_RC(TE_TA_UNIX, errno);
        return 0;
    }
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

#ifndef TA_USE_SLOW_LSEEK
#define TA_USE_SLOW_LSEEK   1
#endif

#ifndef TA_LSEEK_STEP_SIZE
#undef TA_LSEEK_STEP_SIZE
#endif
#define TA_LSEEK_STEP_SIZE  0x10000000

/**
 * Work around 32-bit offset limits
 * (llseek() sometimes does not work properly)
 *
 * @return 0 (success) or system error
 */
static int64_t
long_lseek_set(int fd, int64_t offset)
{
#if TA_USE_SLOW_LSEEK
    off_t off;
    
    if (lseek(fd, 0, SEEK_SET) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    while (offset > 0)
    {
        off = (offset < TA_LSEEK_STEP_SIZE) ? offset : TA_LSEEK_STEP_SIZE;

        if (lseek(fd, off, SEEK_CUR) < 0)
            return TE_OS_RC(TE_TA_UNIX, errno);

        offset -= off;
    }
    return offset;
    
#else
    return llseek(fd, offset, SEEK_SET);
#endif
}

#ifdef TA_SPARSE_BUF_SIZE
#undef TA_SPARSE_BUF_SIZE
#endif
#define TA_SPARSE_BUF_SIZE  1024

/**
 * Create a sparse file (with holes), where the real data of specified
 * length filled with specified pattern starts from specified offset only.
 *
 * @return 0 (success) or system error
 */
int
create_sparse_file(char *path_name, int64_t offset,
                   int64_t payload_length, char ptrn)
{
    char  buf[TA_SPARSE_BUF_SIZE];
    int   fd;
    int   flags = O_CREAT | O_RDWR;
    int   mode = S_IRWXU | S_IRWXG | S_IRWXO;
#if 0
    int   mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
#endif

#if !__linux__ && defined(O_LARGEFILE)
    flags |= O_LARGEFILE;
#endif

    if ((fd = open(path_name, flags, mode)) < 0)
    {
        ERROR("Failed to create sparse file \"%s\"", path_name);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    memset(buf, ptrn, TA_SPARSE_BUF_SIZE);

    if (long_lseek_set(fd, offset) < 0)
    {
        ERROR("Failed to lseek() to %lld offset", offset);
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    
    while (payload_length > 0)
    {
        int copy_len = (payload_length > TA_SPARSE_BUF_SIZE) ?
                       TA_SPARSE_BUF_SIZE : payload_length;

        if ((copy_len = write(fd, (void *)buf, copy_len)) < 0)
        {
            ERROR("Failed to write() to file \"%s\"", path_name);
            close(fd);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        payload_length -= copy_len;
    }

    close(fd);

    return 0;
}

#ifdef TA_CMP_BUF_SIZE
#undef TA_CMP_BUF_SIZE
#endif
#define TA_CMP_BUF_SIZE  1024

int
compare_files(char *path_name1, int64_t offset1,
              char *path_name2, int64_t offset2,
              int64_t cmp_length)
{
    int fd1 = -1;
    int fd2 = -1;
    
    int size1;
    int size2;
    
    char buf1[TA_CMP_BUF_SIZE];
    char buf2[TA_CMP_BUF_SIZE];

    int len;
    
    if ((fd1 = open(path_name1, O_RDONLY)) < 0)
    {
        ERROR("Failed to create sparse file \"%s\"", path_name1);
        goto error;
    }

    if (long_lseek_set(fd1, offset1) < 0)
    {
        ERROR("Failed to lseek() on file \"%s\" to %lld offset",
              path_name1, offset1);
        goto error;
    }

    if ((fd2 = open(path_name2, O_RDONLY)) < 0)
    {
        ERROR("Failed to create sparse file \"%s\"", path_name2);
        goto error;
    }

    if (long_lseek_set(fd2, offset2) < 0)
    {
        ERROR("Failed to lseek() on file \"%s\" to %lld offset",
              path_name2, offset2);
        goto error;
    }

    while (cmp_length > 0)
    {
        len = (cmp_length < TA_CMP_BUF_SIZE) ? cmp_length : TA_CMP_BUF_SIZE;

        if ((size1 = read(fd1, buf1, len)) < 0)
        {
            ERROR("Failed to read() from file \"%s\"", path_name1);
            goto error;
        }

        if ((size2 = read(fd2, buf2, len)) < 0)
        {
            ERROR("Failed to read() from file \"%s\"", path_name2);
            goto error;
        }

        if (size1 != size2)
        {
            break;
        }
        if (size1 == 0)
        {
            cmp_length = 0;
            break;
        }
        if (memcmp(buf1, buf2, size1) != 0)
        {
            break;
        }
        
        cmp_length -= len;
    }

    close(fd1);
    close(fd2);
    
    return (cmp_length > 0) ? -1 : 0;

error:
    if (fd1 >= 0)
        close(fd1);
    if (fd2 >= 0)
        close(fd2);
    return TE_OS_RC(TE_TA_UNIX, errno);
}



/* Declarations of routines to be linked with the agent */
int
ta_rtn_unlink(char *arg)
{
    int rc = unlink(arg);

    VERB("%s(): arg=%s rc=%d errno=%d", __FUNCTION__,
         (arg == NULL) ? "(null)" : arg, rc, errno);
    return (rc == 0) ? 0 : TE_OS_RC(TE_TA_UNIX, errno);
}


/**
 * Signal handler to be registered for SIGINT signal.
 * 
 * @note It is declared as non-static to be visible in TA symbol table.
 */
/* static, see above */ void
ta_sigint_handler(void)
{
    /*
     * We can't log here using TE logging facilities, but we need
     * to make a mark, that TA was killed.
     */
    fprintf(stderr, "Test Agent killed by SIGINT\n");
    exit(EXIT_FAILURE);
}

/**
 * Signal handler to be registered for SIGPIPE signal.
 * 
 * @note It is declared as non-static to be visible in TA symbol table.
 */
/* static, see above */ void
ta_sigpipe_handler(void)
{
    static te_bool here = FALSE;

    if (!here)
    {
        here = TRUE;
        fprintf(stderr, "SIGPIPE is received\n");
        here = FALSE;
    }
}

/** TRUE if first timeval is less than second */
#define TV_LESS(_le, _ge) \
    ((_le).tv_sec < (_ge).tv_sec ||                               \
     ((_le).tv_sec == (_ge).tv_sec && (_le).tv_usec < (_ge).tv_usec))

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

int
rcf_ch_shutdown(struct rcf_comm_connection *handle,
                char *cbuf, size_t buflen, size_t answer_plen)
{
    int rc;
    
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    (void)sigaction(SIGINT, &sigaction_int, NULL);
    (void)sigaction(SIGPIPE, &sigaction_pipe, NULL);

    rc = ta_log_shutdown();
    if (rc != 0)
        fprintf(stderr, "ta_log_shutdown() failed: error=0x%X\n", rc);

    if (tce_stop_function != NULL)
        tce_stop_function();

    kill_tasks();
    kill_threads();

    return -1; /* Call default callback as well */
}

/** 
 * Get identifier of the current thread. 
 *
 * @return Thread identifier
 */
uint32_t
thread_self()
{
    return (unsigned int)pthread_self();
}

/** 
 * Create a mutex.
 *
 * @return Mutex handle
 */
void *
thread_mutex_create(void)
{
    static pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_t *mutex = (pthread_mutex_t *)calloc(1, sizeof(*mutex));
    
    if (mutex != NULL)
        *mutex = init;
        
    return (void *)mutex;
}

/** 
 * Destroy a mutex.
 *
 * @param mutex     mutex handle
 */
void 
thread_mutex_destroy(void *mutex)
{
    free(mutex);
}

/** 
 * Lock the mutex.
 *
 * @param mutex     mutex handle
 *
 */
void 
thread_mutex_lock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to lock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_lock(mutex);
}

/** 
 * Unlock the mutex.
 *
 * @param mutex     mutex handle
 */
void 
thread_mutex_unlock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to unlock NULL mutex", __FUNCTION__);
    else        
        pthread_mutex_unlock(mutex);
}

/* See description in ta_common.h */
int
rcf_rpc_server_init(void)
{
    return 0;
}

/* See description in ta_common.h */
int
rcf_rpc_server_finalize(void)
{
    return 0;
}

#ifdef RCF_RPC
/**
 * Entry point for RPC server started as TA thread.
 *
 * @param ready         semaphore to be posted after params processing
 * @param argc          number of arguments in argv array
 * @param argv          arguments (RPC server name first)     
 *
 * @return Status code.
 */
te_errno 
rcf_ch_rpc_server_thread(void *ready, int argc, char *argv[])
{
    char *name;
    
    if (argc < 1)
    {
        ERROR("Too few parameters for rcf_ch_rpcserver_thread");
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if ((name = strdup(argv[0])) == NULL)
    {
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    sem_post(ready);
    rcf_pch_rpc_server(name);

    return 0;
}
#endif

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
    
    pthread_t   logfork_tid;

    struct sigaction    sigact;
    
    char  buf[16];
    char *tmp;

#ifdef __linux__
    const void **av = (const void **)&argv[argc + 1];

    /* Set size for core dump */
    const struct rlimit rlim= { 500000000, 500000000 };
    if (setrlimit(RLIMIT_CORE, &rlim) != 0)
    {
        fprintf(stderr, "Failed to set RLIMIT_CORE\n");
        /* Continue */
    }

#if defined (AT_SYSINFO)
    /* Skip the environment */
    for (; *av != NULL; av++);

    /* Look for SYSINFO block in the auxiliary vector */
    for (av++; *av != NULL; av += 2)
    {
        if (((const unsigned int *)av)[0] == AT_SYSINFO)
        {
            vsyscall_enter = av[1];
            break;
        }
    }
#endif /* AT_SYSINFO */
#endif

    te_kernel_log_set_system_func(&ta_system);

    /*
     * Set locale to POSIX in order to get all messages in standard
     * format, language, etc.
     */
    if (setenv("LC_ALL", "POSIX", !0) != 0)
    {
        fprintf(stderr, "Failed to set LC_ALL to POSIX\n");
        /* Continue */
    }

    /* Forget user's home and initial working directories */
    unsetenv("PWD");
    unsetenv("HOME");

#if defined (__QNX__)
    /* 'unsetenv' may set errno to ESRCH even when successful */
    errno = 0;
#endif

    /* 
     * Change working directory to /tmp in order to create all
     * temporary files there.
     */
    if (chdir("/tmp") != 0)
    {
        fprintf(stderr, "Failed to change current directory to /tmp\n");
        /* Continue */
    }
    
    if (argc < 3)
    {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    strcpy((char *)ta_execname, argv[0]);
    strcpy(ta_dir, argv[0]);
    tmp = strrchr(ta_dir, '/');
    if (tmp == NULL)
        ta_dir[0] = 0;
    else
        *(tmp + 1) = 0;

    rcf_ch_register_symbol_table(essential_symbols);
#if defined(ENABLE_GENERATED_SYMTBL)
    rcf_ch_register_symbol_table(generated_table);
#endif

    memset(&sigact, 0, sizeof(sigact));
#ifdef SA_RESTART
    sigact.sa_flags = SA_RESTART;
#endif
    sigemptyset(&sigact.sa_mask);

    /* FIXME: Is it used by RPC */
    if (getenv("TE_LEAVE_SIGINT_HANDLER") == NULL)
    { /* some libinit tests need SIGINT handler untouched */
        sigact.sa_handler = (void *)ta_sigint_handler;
        if (sigaction(SIGINT, &sigact, &sigaction_int) != 0)
        {
            rc = te_rc_os2te(errno);
            ERROR("Cannot set SIGINT action: %r");
        }
    }
#if defined (__QNX__)
    /* 'getenv' may set errno to ESRCH even when successful */
    errno = 0;
#endif

    /* FIXME: Is it used by RPC */
    sigact.sa_handler = (void *)ta_sigpipe_handler;
    if (sigaction(SIGPIPE, &sigact, &sigaction_pipe) != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Cannot set SIGPIPE action: %r");
    }

    rc = ta_process_mgmt_init();
    if (rc != 0)
    {
        LOG_PRINT("Cannot initialize process management: %d", rc);
        return rc;
    }

    /* FIXME */
    sigemptyset(&rpcs_received_signals);
    
    if (strcmp(argv[1], "exec") == 0)
    {
        void (* func)(int, char **) = rcf_ch_symbol_addr(argv[2], 1);
        
        if (func == NULL)
        {
            LOG_PRINT("Cannot resolve address of the function %s", argv[2]);
            return 1;
        }
        func(argc - 3, argv + 3);
        return 0;
    }

    if ((rc = ta_log_init()) != 0)
    {
        fprintf(stderr, "ta_log_init() failed: error=%d\n", rc);
        return rc;
    }
    
    /* Is it required for logfork to used by exec'ed */
    te_lgr_entity = ta_name = argv[1];

    RING("Starting");

    sprintf(buf, "PID %lu", (unsigned long)getpid());

    rc = pthread_create(&logfork_tid, NULL, (void *)logfork_entry, NULL);
    if (rc != 0)
    {
        fprintf(stderr, "pthread_create(logfork_entry) failed: rc=%d\n",
                rc);
        /* Continue */
    }

    /* FIXME Is it OK position? */
    init_tce_subsystem();
    
    rc = rcf_pch_run(argv[2], buf);
    if (rc != 0)
    {
        fprintf(stderr, "rcf_pch_run() failed: error=0x%X\n", rc);
        if (retval == 0)
            retval = rc;
    }

    /* Cancel and join logfork thread */
    rc = pthread_cancel(logfork_tid);
    if (rc != 0)
    {
        fprintf(stderr, "pthread_cancel(logfork_tid) failed: rc=%d\n", rc);
    }
    else
    {
        rc = pthread_join(logfork_tid, NULL);
        if (rc != 0)
        {
            fprintf(stderr, "pthread_join(logfork_tid) failed: rc=%d\n",
                    rc);
        }
    }

    /* FIXME Correct retval to return */
    return retval;
}

