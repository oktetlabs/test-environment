/** @file
 * @brief Windows Test Agent
 *
 * Windows RCF RPC support
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

#ifndef __INSIDE_CYGWIN__
#define __INSIDE_CYGWIN__
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#include <windows.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#undef ERROR

#include "te_defs.h"
#include "te_errno.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "rcf_rpc_defs.h"
#include "tarpc.h"
#include "win32_rpc.h"

#define TE_LGR_USER     "RCF RPC"
#define TA_RPC_INSIDE
#include "ta_rpc_log.h"

/** Structure corresponding to one RPC server */
typedef struct srv {
    struct srv *next;   /**< Next server */
    char        name[RCF_RPC_NAME_LEN]; /**< Name of the server */
    int         pid;    /**< Process identifier */
    int         sock;   /**< Socket to interact with the server */
} srv;


/** Release resource allocated for RPC server entry */
#define RELEASE_SRV(_s) \
    do {                        \
        close((_s)->sock);      \
        free(_s);               \
    } while (0)

extern void tarpc_1(struct svc_req *rqstp, register SVCXPRT *transp);

extern int ta_pid;

#define TARPC_SERVER_SYNC_TIMEOUT       1000000
/** PID of the TA process */
struct sockaddr_in ta_log_addr; 
int                ta_log_addr_len = sizeof(struct sockaddr_in);
struct sockaddr   *ta_log_addr_s;
int                ta_log_sock = -1;
int                ta_rpc_sync_socks[2] = {-1, -1};

/** Socket used by the last started RPC server */
int rpcserver_sock = -1;

/** Name of the last started RPC server */
char *rpcserver_name = "";

static srv *srv_list = NULL;
static char buf[RCF_RPC_MAX_BUF]; 

static int supervise_started = 0;

sigset_t rpcs_received_signals;


#define TARPC_SERVER_NAME_LEN   256
#define TARPC_SERVER_MAP_SIZE   256

typedef struct srv_tcp_mapping {
    int            pid;
    unsigned short port;
} srv_tcp_mapping;

/* Mapping table between AF_UNIX and AF_INET sockets */
static srv_tcp_mapping  srv_tcp_map[TARPC_SERVER_MAP_SIZE];

/* Add mapping record between pid and RPC server port */
static int
tarpc_server_mapping_add(int pid, unsigned short port)
{
    int map_no;
    
    for (map_no = 0; map_no < TARPC_SERVER_MAP_SIZE; map_no++)
        if (srv_tcp_map[map_no].pid == 0)
            break;

    if (map_no >= TARPC_SERVER_MAP_SIZE)
        return TE_RC(TE_TA_WIN32, ETOOMANY);

    srv_tcp_map[map_no].pid = pid;
    srv_tcp_map[map_no].port = port;
    
    return 0;
}

/* Lookup for mapping between AF_UNIX and AF_INET sockets */
static unsigned short
tarpc_server_mapping_lookup(int pid)
{
    int map_no;
    
    for (map_no = 0; map_no < TARPC_SERVER_MAP_SIZE; map_no++)
        if (srv_tcp_map[map_no].pid == pid)
            break;

    if (map_no >= TARPC_SERVER_MAP_SIZE)
        return 0;

    srv_tcp_map[map_no].pid = 0;

    return srv_tcp_map[map_no].port;
}


/**
 * Wait for finishing of the children and report about it.
 */
static void *
supervise_children(void *arg)
{
    int status;
    int pid;
    
    UNUSED(arg);
    
    while (1)
    {    
        if ((pid = wait(&status)) > 0)
        {
            if (WIFEXITED(status))
                VERB("RPC Server process with PID %d is deleted", pid);
            else if (WIFSIGNALED(status))
            {
                if (WTERMSIG(status) == SIGINT)
                    VERB("RPC Server process with PID %d is deleted", pid);
                else
                    WARN("RPC Server process with PID %d is killed "
                         "by the signal", pid, WTERMSIG(status));
            }
#ifdef WCOREDUMP
            else if (WCOREDUMP(status))
                ERROR("RPC Server with PID %d core dumped", pid);
#endif
            else
                WARN("RPC Server with PID %d exited due unknown reason",
                     pid); 
        }
    }
}

static int
create_log_socket()
{
    int len = sizeof(ta_log_addr);
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (s < 0)
    {
        ERROR("Cannot create a socket for gathering the log, errno %d",
              errno);
        return -1;
    }

    memset(&ta_log_addr, 0, sizeof(ta_log_addr));
    ta_log_addr.sin_family = AF_INET;
    ta_log_addr.sin_port = 0;
    ta_log_addr_s = (struct sockaddr *)&ta_log_addr;
    if (bind(s, (struct sockaddr *)&ta_log_addr, len) < 0)
    {
        ERROR("Cannot bind a socket for gathering the log, errno %d",
              errno);
        close(s);
        return -1;
    }

    if (getsockname(s, (struct sockaddr *)&ta_log_addr, &len) < 0)
    {
        ERROR("Cannot getsockname for a socket for gathering the log, "
              "errno %d", errno);
        close(s);
        return -1;
    }

    return s;
}

/**
 * Wait for a log from RPC servers.
 */
static void *
gather_log(void *arg)
{
    int s = ta_log_sock;

    if (s < 0)
    {
        ERROR("Socket for gathering the log was not created");
        return NULL;
    }

    UNUSED(arg);
    while (1)
    {
        char log_pkt[RPC_LOG_PKT_MAX] = {0, };
      
        if (recv(s, log_pkt, sizeof(log_pkt), 0) < (int)RPC_LOG_OVERHEAD)
        {
            WARN("Failed to receive the logging message from RPC server");
            continue;
        }
        log_message(*(uint16_t *)log_pkt, "", TE_LGR_USER,
                    "RPC server %s: %s", log_pkt + sizeof(uint16_t), 
                    log_pkt + RPC_LOG_OVERHEAD);
    }
}

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
    struct sockaddr_in  addr;
    int len = sizeof(struct sockaddr_in);
    
    srv *tmp;
    int  tries = MAX_CONNECT_TRIES;
    
    if ((tmp = calloc(1, sizeof(srv))) == NULL)
    {
        ERROR("calloc() failed");
        return TE_RC(TE_TA_WIN32, ENOMEM);
    }
    strcpy(tmp->name, name);

    if ((tmp->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERROR("socket(AF_INET, SOCK_STREAM, 0) failed: %d", errno);
        free(tmp);
        return TE_RC(TE_TA_WIN32, errno);
    }

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    if (bind(tmp->sock, (struct sockaddr *)&addr, sizeof(addr))) {
        ERROR("bind failed: %d", errno);
        free(tmp);
        return TE_RC(TE_TA_WIN32, errno);
    }

    len = sizeof(struct sockaddr_in);
    if (getsockname(tmp->sock, (struct sockaddr *)&addr, &len) < 0)
    {
        ERROR("getsockname failed: %d", errno);
        free(tmp);
        return TE_RC(TE_TA_WIN32, errno);
    }

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(tarpc_server_mapping_lookup(pid));

    if (addr.sin_port == 0)
        return TE_RC(TE_TA_WIN32, EINVAL);

    while (tries > 0 && connect(tmp->sock, SA(&addr), sizeof(addr)) < 0)
    {
        usleep(10000);
        tries--;
    }
           
    if (tries == 0)
    {
        ERROR("Cannot connect to RPC Server '%s'", name);
        free(tmp);
        return TE_RC(TE_TA_WIN32, errno);
    }

    tmp->next = srv_list;
    tmp->pid = pid;
    srv_list = tmp;

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
        return TE_RC(TE_TA_WIN32, ENOENT);
    }
    
    close(cur->sock);
        
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
sigint_handler(int s)
{
    UNUSED(s);
    exit(0);
}

/**
 * Entry function for RPC server (never returns). Creates the transport
 * and runs main RPC loop (see SUN RPC documentation).
 *
 * @param arg   -1 if the new server is a process or 0 if it's a thread
 */
void *
tarpc_server(void *arg)
{
    SVCXPRT *transp;
    int      sock;

    struct sockaddr_in addr;
    int len = sizeof(struct sockaddr_in);

    tarpc_in_arg  arg1;
    tarpc_in_arg *in = &arg1;
    
    signal(SIGINT, sigint_handler);
    
    memset(&arg1, 0, sizeof(arg1));
    strcpy(arg1.name, (char *)arg);
    rpcserver_name = (char *)arg;

    RPC_LGR_MESSAGE(TE_LL_RING, "Started %s (PID %d, TID %u)", (char *)arg, 
                    (int)getpid(), (unsigned int)pthread_self());

    sigemptyset(&rpcs_received_signals);
    
    pmap_unset(tarpc, ver0);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        close(ta_rpc_sync_socks[1]);
        RPC_LGR_MESSAGE(TE_LL_ERROR, "socket() failed");
        return ((SVCXPRT *)NULL);
    }

    transp = svctcp_create(sock, 1024, 1024);
    if (transp == NULL)
    {
        close(sock);
        close(ta_rpc_sync_socks[1]);
        RPC_LGR_MESSAGE(TE_LL_ERROR, "svcunix_create() returned NULL");
        return NULL;
    }
    
    if (getsockname(sock, (struct sockaddr *)&addr, &len) != 0)
    {
        close(sock);
        close(ta_rpc_sync_socks[1]);
        RPC_LGR_MESSAGE(TE_LL_ERROR, "getsockname() failed");
        return NULL;
    }
    
    if (send(ta_rpc_sync_socks[1], &(addr.sin_port), 
             sizeof(addr.sin_port), 0) < 0)
    {
        close(sock);
        close(ta_rpc_sync_socks[1]);
        return NULL;
    }
    usleep(10000);
    close(ta_rpc_sync_socks[1]);

    if (!svc_register(transp, tarpc, ver0, tarpc_1, 0))
    {
        close(sock);
        RPC_LGR_MESSAGE(TE_LL_ERROR, "svc_register() failed");
        return NULL;
    }

    svc_run();
    
    exit(0);
}

/**
 * Create an RPC server as a new process.
 *
 * @param name          name of the new server
 *
 * @return pid in the case of success or -1 in the case of failure
 */
int
tarpc_server_create(char *name)
{
    int  pid;
    
    VERB("tarpc_server_create %s", name);
    
    if (!supervise_started)
    {
        pthread_t tid;

        ta_log_sock = create_log_socket();
        if (ta_log_sock < 0)
        {
            ERROR("Cannot create RPC log gathering socket: %d", errno);
            return -1;
        }

        if (pthread_create(&tid, NULL, supervise_children, NULL) != 0)
        {
            ERROR("Cannot create RPC servers supervising thread: %d",
                  errno);
            return -1;
        }

        if (pthread_create(&tid, NULL, gather_log, NULL) != 0)
        {
            ERROR("Cannot create RPC log gathering thread: %d", errno);
            return -1;
        }

        supervise_started = 1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ta_rpc_sync_socks) < 0)
    {
        ERROR("pipe() failed: %d", errno);
        return -1;
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
        close(ta_rpc_sync_socks[0]);
        tarpc_server(name);
        exit(EXIT_FAILURE);
    }
    else
    {
        struct timeval tv;
        fd_set sync_fds;
        unsigned short port;

        FD_ZERO(&sync_fds);
        FD_SET(ta_rpc_sync_socks[0], &sync_fds);

        tv.tv_sec = TARPC_SERVER_SYNC_TIMEOUT / 1000000;
        tv.tv_usec = TARPC_SERVER_SYNC_TIMEOUT % 1000000;

        if (select(ta_rpc_sync_socks[0] + 1,
                   &sync_fds, NULL, NULL, &tv) <= 0)
        {
            close(ta_rpc_sync_socks[0]);
            return -1;
        }
        if (recv(ta_rpc_sync_socks[0], &port, sizeof(port), 0) < 0)
        {
            close(ta_rpc_sync_socks[0]);
            return -1;
        }
        close(ta_rpc_sync_socks[0]);
        close(ta_rpc_sync_socks[1]);
        
        if (tarpc_server_mapping_add(pid, ntohs(port)) != 0)
            return -1;
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
        close(cur->sock);
        RELEASE_SRV(cur);
    }
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
tarpc_call(int timeout, char *name, const char *file)
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
        return TE_RC(TE_TA_WIN32, ENOENT);
    }
    
    if ((f = fopen(file, "r")) == NULL)
    {
        ERROR("Failed to open file '%s' with RPC data for reading", file);
        return TE_RC(TE_TA_WIN32, errno);
    }
    
    if ((len = fread(buf, 1, sizeof(buf), f)) <= 0)
    {
        ERROR("Failed to read RPC data from the file");
        fclose(f);
        return TE_RC(TE_TA_WIN32, errno);
    }
    fclose(f);
    
    if (write(cur->sock, buf, len) < len)
    {
        ERROR("Failed to write data to the RPC pipe");
        return TE_RC(TE_TA_WIN32, errno);
    }
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    FD_ZERO(&set);
    FD_SET(cur->sock, &set);
    if (select(cur->sock + 1, &set, NULL, NULL, &tv) <= 0)
    {
        ERROR("Timeout ocurred during reading from RPC pipe");
        return TE_RC(TE_TA_WIN32, ETERPCTIMEOUT);
    }
    
    if ((len = read(cur->sock, buf, RCF_RPC_MAX_BUF)) <= 0)
    {
        ERROR("Failed to read data from the RPC pipe; errno %d", errno);
        return TE_RC(TE_TA_WIN32, errno);
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
                VERB("Cannot read data from RPC client");
                return TE_RC(TE_TA_WIN32, ENODATA);
            }
            else
                break;
        }
        if (len == RCF_RPC_MAX_BUF)
        {
            VERB("RPC data are too long - increase RCF_RPC_MAX_BUF");
            return TE_RC(TE_TA_WIN32, ENOMEM);
        }
        
        if ((n = read(cur->sock, buf + len, RCF_RPC_MAX_BUF - len)) < 0)
        {
            VERB("Cannot read data from RPC client");
            return TE_RC(TE_TA_WIN32, errno);
        }
        len += n;
    }
    if ((f = fopen(file, "w")) == NULL)
    {
        ERROR("Failed to open file with RPC data for writing");
        return TE_RC(TE_TA_WIN32, errno);
    }
    if (fwrite(buf, 1, len, f) < (size_t)len)
    {
        ERROR("Failed to write data to the RPC file");
        fclose(f);
        return TE_RC(TE_TA_WIN32, errno);
    }
    
    fclose(f);
    
    return 0;
}

