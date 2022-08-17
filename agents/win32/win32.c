/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Windows Test Agent
 *
 * Windows Test Agent implementation.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER      "Main"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <w32api/windows.h>

#undef ERROR

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logfork.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"


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

char *ta_name = "(win32)";

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef RCF_RPC
extern te_errno create_process_rpc_server(const char *name, int32_t *pid,
                                          int flags);
extern te_errno rcf_ch_rpc_server_thread(void *ready,
                                         int argc, char *argv[]);
#endif

extern int win32_process_exec(int argc, char **argv);

/*
   Flushes local static ARP entries list
   Defined in win32conf.c
*/
extern void flush_neigh_st_list();


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
    pthread_mutex_lock(&ta_lock);
}

/* See description in rcf_ch_api.h */
void
rcf_ch_unlock()
{
    if (pthread_mutex_trylock(&ta_lock) == 0)
    {
        WARN("rcf_ch_unlock() without rcf_ch_lock()!\n"
             "It may happen in the case of asynchronous cancellation.");
    }
    pthread_mutex_unlock(&ta_lock);
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
    system("reboot -f 0");
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

uint32_t ta_processes_num = 0;

/* See description in rcf_ch_api.h */
int
rcf_ch_start_process(int *pid,
                    int priority, const char *rtn, te_bool is_argv,
                    int argc, void **params)
{
    void *addr = rcf_ch_symbol_addr(rtn, TRUE);
    int   tries = 0;

    UNUSED(priority);

    VERB("Start task handler is executed");

#ifdef RCF_RPC
    if (strcmp(rtn, "rcf_pch_rpc_server_argv") == 0)
        return create_process_rpc_server((char *)params[0], (int32_t *)pid,
                                         RCF_RPC_SERVER_GET_NET_INIT);
#endif

    while (addr != NULL)
    {
        VERB("fork process with entry point '%s'", rtn);

        if ((*pid = fork()) == 0)
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
            _exit(0);
        }
        else if (*pid < 0)
        {
            int rc = TE_OS_RC(TE_TA_WIN32, errno);

            ERROR("%s(): fork() failed", __FUNCTION__);

            if (tries++ > 10)
                return rc;

            sleep(1);
        }
        ta_processes_num++;

        return 0;
    }

    /* Try shell process */
    if (is_argv)
    {
        char check_cmd[RCF_MAX_PATH];

        sprintf(check_cmd,
                "TMP=`which %s 2>/dev/null` ; test -n \"$TMP\" ;", rtn);
        if (system(check_cmd) != 0)
            return TE_RC(TE_TA_WIN32, TE_ENOENT);

        if ((*pid = fork()) == 0)
        {
            rcf_pch_detach();
            /* Set the process group to allow killing all children */
            setpgid(getpid(), getpid());
            logfork_register_user(rtn);
            execlp(rtn, rtn,
                   params[0], params[1], params[2], params[3], params[4],
                   params[5], params[6], params[7], params[8], params[9]);
            _exit(0);
        }
        else if (*pid < 0)
        {
            int rc = TE_OS_RC(TE_TA_WIN32, errno);

            ERROR("%s(): fork() failed", __FUNCTION__);

            return rc;
        }
        ta_processes_num++;

        return 0;
    }

    return TE_RC(TE_TA_WIN32, TE_ENOENT);
}


struct rcf_thread_parameter {
    te_bool   active;
    pthread_t id;
    void     *addr;
    te_bool   is_argv;
    int       argc;
    void    **params;
    te_errno  rc;
    te_bool   sem_created;
    sem_t     params_processed;
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
                    return TE_OS_RC(TE_TA_WIN32, rc);
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
        return TE_RC(TE_TA_WIN32, TE_ETOOMANY);
    }

    return TE_RC(TE_TA_WIN32, TE_ENOENT);
}

/* See description in rcf_ch_api.h */
int
rcf_ch_kill_process(unsigned int pid)
{
    kill(pid, SIGTERM);
    kill(pid, SIGKILL);
    return 0;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_free_proc_data(unsigned int pid)
{
    UNUSED(pid);

    return 0;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_kill_thread(unsigned int tid)
{
    if (pthread_cancel((pthread_t)tid) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);
    else
        return 0;
}

/**
 * Routine to be executed remotely to run any program from shell.
 *
 * @param argc      - number of arguments in array
 * @param argv      - array with pointer to string arguments
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
        return TE_RC(TE_TA_WIN32, TE_ESMALLBUF);

    rc = system(cmdbuf);
    if (rc == -1)
        return TE_OS_RC(TE_TA_WIN32, errno);

#ifdef WCOREDUMP
    if (WCOREDUMP(rc))
        ERROR("Command executed in shell dumped core");
#endif
    if (!WIFEXITED(rc))
        ERROR("Abnormal termination of command executed in shell");

    return TE_RC(TE_TA_WIN32, WEXITSTATUS(rc));
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
    rc = system(cmd);

    if (rc < 0)
        rc = TE_EPERM;

    return TE_RC(TE_TA_WIN32, rc);
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
    int   err;

    if ((f = fopen(pathname, "w")) == NULL)
    {
        err = errno;
        ERROR("fopen(\"%s\", \"w\") failed; errno %d", pathname, err);
        return TE_RC(TE_TA_WIN32, err);
    }

    memset(buf, c, sizeof(buf));

    while (len > 0)
    {
        int copy_len = ((unsigned int)len  > sizeof(buf)) ?
                           (int)sizeof(buf) : len;

        if ((copy_len = fwrite((void *)buf, sizeof(char), copy_len, f)) < 0)
        {
            int err = errno;

            ERROR("fwrite() failed errno=%d", err);

            fclose(f);
            return TE_RC(TE_TA_WIN32, err);
        }
        len -= copy_len;
    }
    if (fclose(f) < 0)
    {
        err = errno;
        ERROR("fclose() failed errno=%d", err);
        return TE_RC(TE_TA_WIN32, err);
    }

    return 0;
}

/* Declarations of routines to be linked with the agent */
int
ta_rtn_unlink(char *arg)
{
    int rc = unlink(arg);

    VERB("%s(): arg=%s rc=%d errno=%d", __FUNCTION__,
         (arg == NULL) ? "(null)" : "" /*arg*/, rc, errno);
    return (rc == 0) ? 0 : errno;
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

    (void)signal(SIGINT, SIG_DFL);
    (void)signal(SIGPIPE, SIG_DFL);

    rc = ta_log_shutdown();
    if (rc != 0)
        fprintf(stderr, "ta_log_shutdown() failed: error=0x%X\n", rc);

    return -1; /* Call default callback as well */
}

int
die(void)
{
    _exit(0);
}

#ifdef RCF_RPC
extern void wsa_func_handles_discover();
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
#else

/** Dummy */
void
sleep_ex(int msec)
{
    usleep(msec * 1000);
}

#endif

/**
 * Entry point of the Windows Test Agent.
 *
 * Usage:
 *     tawin32 <ta_name> <communication library configuration string>
 */
int
main(int argc, char **argv)
{
    int rc, retval = 0;

    char buf[16];

    pthread_t tid;

    te_log_init("(win32)", te_log_message_file);

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (win32_process_exec(argc, argv) != 0)
        return 1;

    ta_name = argv[1];

#ifdef RCF_RPC
    if (strcmp(ta_name, "rpcserver") == 0)
    {
        wsa_func_handles_discover();
        rcf_pch_rpc_server(argv[2]);
        _exit(0);
    }
#endif

    if ((rc = ta_log_init(ta_name)) != 0)
    {
        fprintf(stderr, "ta_log_init() failed: error=%d\n", rc);
        return rc;
    }

    VERB("Started\n");

    sprintf(buf, "PID %u", getpid());

    pthread_create(&tid, NULL, (void *)logfork_entry, NULL);

    rc = rcf_pch_run(argv[2], buf);
    if (rc != 0)
    {
        fprintf(stderr, "rcf_pch_run() failed: error=0x%X\n", rc);
        if (retval == 0)
            retval = rc;
    }

    flush_neigh_st_list();

    return retval;
}

