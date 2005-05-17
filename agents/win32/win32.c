/** @file
 * @brief Windows Test Agent
 *
 * Windows Test Agent implementation.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include <winsock2.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/un.h>
#include <semaphore.h>

#undef ERROR

#include "te_defs.h"
#include "te_errno.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#define TE_LGR_USER      "Main"
#include "logger_ta.h"
#include "logfork.h"



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


extern void *rcf_ch_symbol_addr_auto(const char *name, te_bool is_func);
extern char *rcf_ch_symbol_name_auto(const void *addr);
extern void  wsa_func_handles_discover();

DEFINE_LGR_ENTITY("(win32)");

char *ta_name = "(win32)";

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;


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
    pthread_mutex_unlock(&ta_lock);
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
            const char *rtn, te_bool is_argv, int argc, uint32_t *params)
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
                  int argc, uint32_t *params)
{
    void *addr = rcf_ch_symbol_addr(rtn, TRUE);
    int   pid;
    
    UNUSED(priority);

    VERB("Start task handler is executed");

    if (addr != NULL)
    {
        VERB("fork process with entry point '%s'", rtn);

        if ((pid = fork()) == 0)
        {
            logfork_register_user(rtn);
            if (is_argv)
                ((rcf_rtn)(addr))(argc, params);
            else
                ((rcf_rtn)(addr))(params[0], params[1], params[2],
                                  params[3], params[4], params[5],
                                  params[6], params[7], params[8],
                                  params[9]);
            exit(0);
        }

        SEND_ANSWER("%d %d", 0, pid);
    }

    /* Try shell process */
    if (is_argv)
    {
        char check_cmd[RCF_MAX_PATH];

        sprintf(check_cmd,
                "TMP=`which %s 2>/dev/null` ; test -n \"$TMP\" ;", rtn);
        if (system(check_cmd) != 0)
            SEND_ANSWER("%d", ETENOSUCHNAME);

        if ((pid = fork()) == 0)
        {
            logfork_register_user(rtn);
            execlp(rtn, rtn, params[0], params[1], params[2], params[3],
                             params[4], params[5], params[6], params[7],
                             params[8], params[9]);
            exit(0);
        }

#if 0
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
#endif

        SEND_ANSWER("%d %d", 0, pid);
    }

    SEND_ANSWER("%d", ETENOSUCHNAME);
}

struct rcf_thread_parameter {
    te_bool   active;
    pthread_t id;
    void     *addr;
    te_bool   is_argv;
    int       argc;
    uint32_t *params;
    int       rc;
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
        parm->rc = ((rcf_thr_rtn)(parm->addr))(&parm->params_processed, 
                                               parm->argc, parm->params);
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
                  int argc, uint32_t *params)
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
                SEND_ANSWER("0 %d", iter - thread_pool);
            }
        }
        pthread_mutex_unlock(&thread_pool_mutex);
        SEND_ANSWER("%d", ETOOMANY);
    }

    SEND_ANSWER("%d", ETENOSUCHNAME);
}

/* See description in rcf_ch_api.h */
int
rcf_ch_kill_task(struct rcf_comm_connection *handle,
                 char *cbuf, size_t buflen, size_t answer_plen,
                 unsigned int pid)
{
    kill(pid, SIGTERM); 
    kill(pid, SIGKILL); 
    SEND_ANSWER("0");
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
        return TE_RC(TE_TA_WIN32, ETESMALLBUF);

    rc = system(cmdbuf);
    if (rc == -1)
        return TE_RC(TE_TA_WIN32, errno);

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
        rc = EPERM;

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
    
    if ((f = fopen(pathname, "w")) == NULL)
        return TE_RC(TE_TA_WIN32, errno);
    
    memset(buf, c, sizeof(buf));
    
    while (len > 0)
    {
        int copy_len = ((unsigned int)len  > sizeof(buf)) ?
                           (int)sizeof(buf) : len;

        if ((copy_len = fwrite((void *)buf, sizeof(char), copy_len, f)) < 0)
        {
            fclose(f);
            return TE_RC(TE_TA_WIN32, errno);
        }
        len -= copy_len;
    }
    if (fclose(f) < 0)
    {
        ERROR("fclose() failed errno=%d", errno);
        return TE_RC(TE_TA_WIN32, errno);
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
    UNUSED(sig);
    WARN("Test Agent received SIGPIPE signal");
}

sigset_t rpcs_received_signals;

/**
 * Special signal handler which registers signals.
 * 
 * @param signum    received signal
 */
void
signal_registrar(int signum)
{
    sigaddset(&rpcs_received_signals, signum);
}

HINSTANCE ta_hinstance;

/**
 * Entry point of the Windows Test Agent.
 *
 * Usage:
 *     tawin32 <ta_name> <communication library configuration string>
 */
int WINAPI 
WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, 
        LPSTR lpCmdLine, int nCmdShow) 
{
    int rc, retval = 0;
    
    char  buf[16];
    char  cmd[256] = { 0, };
    char *tmp;

    pthread_t tid;

    WSADATA data;

    WSAStartup(MAKEWORD(2,2), &data);
    
    UNUSED(hPrevInstance);
    UNUSED(nCmdShow);
    
    ta_hinstance = hinstance;
    
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    strncpy(cmd, lpCmdLine, sizeof(cmd) - 1);
    ta_name = strtok(cmd, " ");
    tmp = strtok(NULL, " ");

    (void)signal(SIGINT, ta_sigint_handler);
    (void)signal(SIGPIPE, ta_sigpipe_handler);

    if ((rc = log_init()) != 0)
    {
        fprintf(stderr, "log_init() failed: error=%d\n", rc);
        return rc;
    }

    te_lgr_entity = ta_name;
    VERB("Started\n");

    sprintf(buf, "PID %u", getpid());

    pthread_create(&tid, NULL, (void *)logfork_entry, NULL);

    sigemptyset(&rpcs_received_signals);

#ifdef RCF_RPC
    wsa_func_handles_discover();
#endif        

    rc = rcf_pch_run(tmp, buf);
    if (rc != 0)
    {
        fprintf(stderr, "rcf_pch_run() failed: error=%d\n", rc);
        if (retval == 0)
            retval = rc;
    }

    rc = log_shutdown();
    if (rc != 0)
    {
        fprintf(stderr, "log_shutdown() failed: error=%d\n", rc);
        if (retval == 0)
            retval = rc;
    }
    
    return retval;
}

