/** @file
 * @brief Linux Test Agent
 *
 * Linux RCF RPC support
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
 *
 * $Id$
 */

#include "te_config.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#include <pthread.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_rpc_defs.h"
#include "tarpc.h"
#include "linux_internal.h"
#include "linux_rpc.h"
#include "logger_ta.h"


#undef TE_LGR_USER
#define TE_LGR_USER     "RCF RPC"

#include "logfork.h"


/** Whether supervise children using SIGCHLD or using thread with wait(). */
#define SUPERVISE_CHILDREN_BY_SIGNAL

/** Maximum length of the pipe address */
#define PIPENAME_LEN    sizeof(((struct sockaddr_un *)0)->sun_path)


/** Structure corresponding to one RPC server */
typedef struct srv {
    struct srv *next;   /**< Next server */
    char        name[RCF_RPC_NAME_LEN]; 
                        /**< Name of the server */
    int         pid;    /**< Process identifier */
    char        pipename[PIPENAME_LEN]; 
                        /**< Name of the pipe to interact with the server */
    int         sock;   /**< Pipe itself */
} srv;

/** Release resource allocated for RPC server entry */
#define RELEASE_SRV(_s) \
    do {                        \
        close((_s)->sock);      \
        unlink((_s)->pipename); \
        free(_s);               \
    } while (0)

extern void tarpc_1(struct svc_req *rqstp, register SVCXPRT *transp);

/** PID of the TA process */
struct sockaddr_un ta_log_addr; 
struct sockaddr   *ta_log_addr_s;
int                ta_log_addr_len = sizeof(struct sockaddr_un);

static srv *srv_list = NULL;
static char buf[RCF_RPC_MAX_BUF]; 

static int supervise_started = 0;

sigset_t rpcs_received_signals;


/**
 * Wait for a child and log its exit status information.
 */
static void
wait_child_and_log(void)
{
    int status;
    int pid;

#define CHECK_LGR_LOCK \
    do {                                                              \
        ta_lgr_lock_key key;                                          \
                                                                      \
        if (ta_lgr_trylock(key) != 0)                                 \
        {                                                             \
            fprintf(stderr, "Logger is locked, drop the message\n");  \
            return;                                                   \
        }                                                             \
        (void)ta_lgr_unlock(key);                                     \
    } while (0)
    
    if ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        CHECK_LGR_LOCK;
        if (WIFEXITED(status))
            VERB("RPC Server process with PID %d is deleted", pid);
        else if (WIFSIGNALED(status))
        {
            if (WTERMSIG(status) == SIGTERM)
                VERB("RPC Server process with PID %d is deleted", pid);
            else
                WARN("RPC Server process with PID %d is killed "
                     "by the signal %d", pid, WTERMSIG(status));
        }
#ifdef WCOREDUMP
        else if (WCOREDUMP(status))
            ERROR("RPC Server with PID %d core dumped", pid);
#endif
        else
            WARN("RPC Server with PID %d exited due unknown reason", pid); 
    }
    else if (pid == 0)
    {
        CHECK_LGR_LOCK;
        WARN("No child was available");
    }
    else
    {
        CHECK_LGR_LOCK;
        if (errno != EINTR)
            ERROR("waitpid() failed with errno %d", errno);
    }
#undef CHECK_LGR_LOCK
}


#ifdef SUPERVISE_CHILDREN_BY_SIGNAL

/**
 * SIGCHLD handler.
 *
 * @param s     received signal
 */
static void
sigchld_handler(int s)
{
    UNUSED(s);

    wait_child_and_log();
}

#else /* !SUPERVISE_CHILDREN_BY_SIGNAL */

/**
 * Wait for finishing of the children and report about it.
 */
static void *
supervise_children(void *arg)
{
    UNUSED(arg);
    
    while (1)
    {
        wait_child_and_log();
    }
}

#endif /* !SUPERVISE_CHILDREN_BY_SIGNAL */


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


/** Routine to free the result of thread-safe RPC call */
int
tarpc_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    UNUSED(transp);
    xdr_free(xdr_result, result);
    return 1;
}

#define MAX_CONNECT_TRIES       512

/**
 * Create an entry for new server in the list and establish a connection
 * with it.
 *
 * @param name  name of the server
 * @param pid   pid of the server
 *
 * @return status code
 */
int 
tarpc_add_server(char *name, int pid)
{
    struct sockaddr_un addr;
    
    srv *tmp;
    int  len;
    int  tries = MAX_CONNECT_TRIES;

    if ((tmp = calloc(1, sizeof(srv))) == NULL)
    {
        ERROR("calloc() failed");
        return TE_RC(TE_TA_LINUX, ENOMEM);
    }
    strcpy(tmp->name, name);

    sprintf(tmp->pipename, "/tmp/terpcs_%s_%u", name, ta_pid);
    len = strlen(tmp->pipename) + 1;

    if ((tmp->sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        ERROR("socket(AF_UNIX, SOCK_STREAM, 0) failed: %d", errno);
        free(tmp);
        return TE_RC(TE_TA_LINUX, errno);
    }

    memset(&addr, '\0', sizeof (addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, tmp->pipename, len);
    len = sizeof(struct sockaddr_un) - PIPENAME_LEN + len;
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    addr.sun_len = len;
#endif
    
    while (tries > 0)
    {
        if (connect(tmp->sock, (struct sockaddr *)&addr, len) == 0)
        {
            break;
        }
        else if ((errno != ENOENT) && (errno != ECONNREFUSED))
        {
            free(tmp);
            ERROR("Connect to RPC Server '%s' failed: %s",
                  name, strerror(errno));
            return TE_RC(TE_TA_LINUX, errno);
        }
        usleep(10000);
        tries--;
    }
    if (tries == 0)
    {
        free(tmp);
        ERROR("Cannot connect to RPC Server '%s'", name);
        return TE_RC(TE_TA_LINUX, ECONNREFUSED);
    }
    
    tmp->next = srv_list;
    tmp->pid = pid;
    srv_list = tmp;

    VERB("RPC Server '%s' successfully added to the list", name);
    
    return 0;
}

/**
 * Delete an entry for the server from the list and close the connection
 * with it.
 *
 * @param name  name of the server to be deleted
 *
 * @return status code
 */
int 
tarpc_del_server(char *name)
{
    srv *cur;
    srv *prev;
    
    VERB("tarpc_del_server '%s'", name);
    
    for (prev = NULL, cur = srv_list; 
         cur != NULL && strcmp(cur->name, name) != 0;
         prev = cur, cur = cur->next)
    {
        VERB("skip %s", cur->name);
    }
         
    if (cur == NULL)
    {
        ERROR("Failed to find RPC Server '%s' to delete", name);
        return TE_RC(TE_TA_LINUX, ENOENT);
    }
        
    if (prev)
        prev->next = cur->next;
    else
        srv_list = cur->next;
        
    VERB("RPC Server '%s' is deleted from the list", cur->name);

    RELEASE_SRV(cur);

    return 0;
}

/**
 * Set correct PID of exec'ed server.
 *
 * @param name  name of the server to be deleted
 * @param pid   new PID
 *
 * @return status code
 */
int 
tarpc_set_server_pid(char *name, int pid)
{
    srv *cur;
    
    VERB("tarpc_set_server_pid '%s' = %d", name, pid);
    
    for (cur = srv_list;
         cur != NULL && strcmp(cur->name, name) != 0;
         cur = cur->next)
    {
        VERB("skip %s", cur->name);
    }
         
    if (cur == NULL)
    {
        ERROR("Failed to find RPC Server '%s' to set PID", name);
        return TE_RC(TE_TA_LINUX, ENOENT);
    }
    cur->pid = pid;

    return 0;
}

static void
sig_handler(int s)
{
    UNUSED(s);
    exit(1);
}

/**
 * Create an RPC server as a new process.
 *
 * @param name          name of the new server
 *
 * @return pid in the case of success or -1 in the case of failure
 */
int
tarpc_server_create(const char *name)
{
    int  pid;
    
    VERB("tarpc_server_create %s", name);
    
    if (!supervise_started)
    {
        pthread_t tid;
        int       len;

        memset(&ta_log_addr, 0, sizeof(ta_log_addr));
        ta_log_addr.sun_family = AF_UNIX;
        len = snprintf(ta_log_addr.sun_path, PIPENAME_LEN,
                       "/tmp/te_rpc_log_%d", ta_pid);
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
        ta_log_addr.sun_len = sizeof(struct sockaddr_un) -
                                  PIPENAME_LEN + len + 1 /* \0 */;
#endif
        ta_log_addr_s = (struct sockaddr *)&ta_log_addr;
#ifdef SUPERVISE_CHILDREN_BY_SIGNAL
        (void)signal(SIGCHLD, sigchld_handler);
#else
        if (pthread_create(&tid, NULL, supervise_children, NULL) != 0)
        {
            ERROR("Cannot create RPC servers supervising thread: %d",
                  errno);
            return -1;
        }
#endif
        supervise_started = 1;
    }

    pid = fork();
    if (pid < 0)
    {
        ERROR("fork() failed: %d", errno);
        return pid;
    }

    if (pid == 0)
    {
        /* Child */
        rcf_pch_detach();
        tarpc_server(name);
        exit(EXIT_FAILURE);
    }
    
    VERB("RPC Server '%s' is created", name);

    return pid;
}

/**
 * Destroy all RPC server processes and release the list of RPC servers.
 */
void
tarpc_destroy_all()
{
    srv *cur;
    
    for (cur = srv_list; cur != NULL; cur = srv_list)
    {
        srv_list = cur->next;
        if (kill(cur->pid, SIGTERM) != 0)
            ERROR("Failed to send SIGTERM to PID %d", cur->pid);
        RELEASE_SRV(cur);
    }

    unlink(ta_log_addr.sun_path); 
}

/**
 * Forward RPC call to proper RPC server.
 *
 * @param timeout timeout (in milliseconds)
 * @param name    RPC server name
 * @param file    pathname of the file with RPC call
 *
 * @return status code
 */
int 
tarpc_call(int timeout, const char *name, const char *file)
{
    FILE *f;
    int   len;
    srv *cur;
    
    struct timeval tv;
    fd_set         set;

    VERB("tarpc_call entry");

    for (cur = srv_list; 
         cur != NULL && strcmp(cur->name, name) != 0;
         cur = cur->next)
    {
        VERB("%s(): skip '%s'", __FUNCTION__, cur->name);
    }
         
    if (cur == NULL)
    {
        ERROR("RPC Server '%s' does not exist", name);
        return TE_RC(TE_TA_LINUX, ENOENT);
    }
    
    if ((f = fopen(file, "r")) == NULL)
    {
        ERROR("Failed to open file '%s' with RPC data for reading", file);
        return TE_RC(TE_TA_LINUX, errno);
    }
    
    if ((len = fread(buf, 1, sizeof(buf), f)) <= 0)
    {
        ERROR("Failed to read RPC data from the file");
        fclose(f);
        return TE_RC(TE_TA_LINUX, errno);
    }
    fclose(f);
    
    if (write(cur->sock, buf, len) < len)
    {
        ERROR("Failed to write data to the RPC pipe: %s", strerror(errno));
        return TE_RC(TE_TA_LINUX, errno);
    }
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    FD_ZERO(&set);
    FD_SET(cur->sock, &set);
    VERB("Server %s timeout %d", cur->name, timeout);
    if (select(cur->sock + 1, &set, NULL, NULL, &tv) <= 0)
    {
        ERROR("Timeout ocurred during reading from RPC pipe");
        return TE_RC(TE_TA_LINUX, ETERPCTIMEOUT);
    }
    
    if ((len = read(cur->sock, buf, RCF_RPC_MAX_BUF)) < 0)
    {
        ERROR("Failed to read data from the RPC pipe; errno %d", errno);
        return TE_RC(TE_TA_LINUX, errno);
    }
    if (len == 0)
    {
        ERROR("RPC client connection closed, it's likely that RPC "
              "server '%s' is dead", name);
        return TE_RC(TE_TA_LINUX, EIO);
    }
    /* Read the rest of data, if exist */
    while (1)
    {
        int n;
        
        tv.tv_sec = RCF_RPC_EOR_TIMEOUT / 1000000;
        tv.tv_usec = RCF_RPC_EOR_TIMEOUT % 1000000;
        FD_ZERO(&set);
        FD_SET(cur->sock, &set);
        if (select(cur->sock + 1, &set, NULL, NULL, &tv) <= 0)
        {
            if (len == 0)
            {
                ERROR("Cannot read data from RPC client");
                return TE_RC(TE_TA_LINUX, EIO);
            }
            else
                break;
        }
        if (len == RCF_RPC_MAX_BUF)
        {
            ERROR("RPC data are too long - increase RCF_RPC_MAX_BUF");
            return TE_RC(TE_TA_LINUX, ENOMEM);
        }
        
        if ((n = read(cur->sock, buf + len, RCF_RPC_MAX_BUF - len)) < 0)
        {
            ERROR("Cannot read data from RPC client");
            return TE_RC(TE_TA_LINUX, errno);
        }
        if (n == 0)
        {
            ERROR("RPC client connection closed after got of some data, "
                  "it's likely that RPC server '%s' is dead", name);
            return TE_RC(TE_TA_LINUX, EIO);
        }
        len += n;
    }
    if ((f = fopen(file, "w")) == NULL)
    {
        ERROR("Failed to open file with RPC data for writing");
        return TE_RC(TE_TA_LINUX, errno);
    }
    if (fwrite(buf, 1, len, f) < (size_t)len)
    {
        ERROR("Failed to write data to the RPC file");
        fclose(f);
        return TE_RC(TE_TA_LINUX, errno);
    }
    
    fclose(f);
    
    return 0;
}


#ifndef LOGFORK_LOG
#define LOGFORK_LOG  1
#endif

#include "logfork.h"

/**
 * Entry function for RPC server (never returns). Creates the transport
 * and runs main RPC loop (see SUN RPC documentation).
 *
 * @param arg   -1 if the new server is a process or 0 if it's a thread
 */
void *
tarpc_server(const void *arg)
{
    const char         *name = (const char *)arg;
    SVCXPRT            *transp;
    struct sockaddr_un  addr;

    tarpc_in_arg  arg1;
    tarpc_in_arg *in = &arg1;
    
    memset(&arg1, 0, sizeof(arg1));
    arg1.name.name_len = strlen(name) + 1;
    arg1.name.name_val = strdup(name);

    if (logfork_register_user(name) != 0)
    {
        RING("logfork_register_user() fail to register %s server", name);
        
        /* Try again */
        logfork_register_user(name);
    }
    
    RING("Started %s (PID %d, TID %u)", name, 
                    (int)getpid(), (unsigned int)pthread_self());
    
    sigemptyset(&rpcs_received_signals);
    signal(SIGTERM, sig_handler);
    
    pmap_unset(tarpc, ver0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path),
             "/tmp/terpcs_%s_%u", name, ta_pid);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

#ifdef HAVE_SVCUNIX_CREATE
    VERB("%s(): call svcunix_create()", __FUNCTION__);
    transp = svcunix_create(RPC_ANYSOCK, 1024, 1024, addr.sun_path);
    if (transp == NULL)
    {
        ERROR("svcunix_create() returned NULL");
        return NULL;
    }
#else
    if (1) {
        int sock = socket(PF_UNIX, SOCK_STREAM, 0);

        if (sock < 0)
        {
            ERROR("socket(PF_UNIX, SOCK_STREAM, 0) failed");
            return NULL;
        }
        if (bind(sock, SA(&addr), sizeof(addr)) != 0)
        {
            ERROR("bind() to RPC server address failed");
            close(sock);
            return NULL;
        }
        if (listen(sock, 2) != 0)
        {
            ERROR("listen() on RPC server socket failed");
            close(sock);
            return NULL;
        }

        transp = svc_vc_create(sock, 1024, 1024);
        if (transp == NULL)
        {
            ERROR("svc_vc_create() returned NULL");
            return NULL;
        }
    }
#endif

    VERB("%s(): call svc_register()", __FUNCTION__);
    if (!svc_register(transp, tarpc, ver0, tarpc_1, 0))
    {
        ERROR("svc_register() failed");
        return NULL;
    }
    
    VERB("%s(): call svc_run()", __FUNCTION__);
    svc_run();

    ERROR("Unreachable!");

    return NULL;
}
