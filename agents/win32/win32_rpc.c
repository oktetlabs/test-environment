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
 * $Id: win32_rpc.c 3921 2004-07-22 18:39:49Z arybchik $
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

#include "win32_dummy.h"

#include "te_defs.h"
#include "te_errno.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "rcf_rpc_defs.h"
#include "tarpc.h"
#include "win32_rpc.h"

#define LGR_USER    "RCF RPC"
#define WIN32_RPC_INSIDE
#include "win32_rpc_log.h"

/** Maximum length of the pipe address */
#define PIPENAME_LEN    sizeof(((struct sockaddr_un *)0)->sun_path)

/** Structure corresponding to one RPC server */
typedef struct srv {
    struct srv *next;   /**< Next server */
    char        name[RCF_RPC_NAME_LEN]; /**< Name of the server */
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

extern int ta_pid;

#define TARPC_SERVER_SYNC_TIMEOUT       1000000
/** PID of the TA process */
struct sockaddr_in ta_log_addr; 
int                ta_log_sock = -1;
int                ta_rpc_sync_socks[2] = {-1, -1};

/** Socket used by the last started RPC server */
int rpcserver_sock = -1;

/** Name of the last started RPC server */
char *rpcserver_name = "";

static srv *srv_list = NULL;
static char buf[RCF_RPC_MAX_BUF]; 

static int supervise_started = 0;

static sigset_t rpcs_received_signals;


#define TARPC_SERVER_NAME_LEN   256
#define TARPC_SERVER_MAP_SIZE   256

typedef struct srv_tcp_mapping {
    char            name[TARPC_SERVER_NAME_LEN];
    unsigned short  port;
} srv_tcp_mapping;

/* Mapping table between AF_UNIX and AF_INET sockets */
static srv_tcp_mapping  srv_tcp_map[TARPC_SERVER_MAP_SIZE];

static int              srv_tcp_map_initialized = 0;


/* Add mapping record between AF_UNIX and AF_INET sockets */
static int
tarpc_server_mapping_add(char *name, unsigned short port)
{
    int map_no;
    if (!srv_tcp_map_initialized) {
        for (map_no = 0; map_no < TARPC_SERVER_MAP_SIZE; map_no++)
        {
            *(srv_tcp_map[map_no].name) = '\0';
            srv_tcp_map[map_no].port = 0;
        }
        srv_tcp_map_initialized++;
    }

    fprintf(stdout, "  try to add mapping: %s -> port %d\n", name, port);
    
    for (map_no = 0; map_no < TARPC_SERVER_MAP_SIZE; map_no++)
        if (srv_tcp_map[map_no].port == 0)
            break;

    if (map_no >= TARPC_SERVER_MAP_SIZE)
        return TE_RC(TE_TA_WIN32, ETOOMANY);

    strncpy(srv_tcp_map[map_no].name, name, TARPC_SERVER_NAME_LEN);
    srv_tcp_map[map_no].port = port;
    
    return 0;
}

/* Delete mapping record between AF_UNIX and AF_INET sockets */
static int
tarpc_server_mapping_del(char *name)
{
    int map_no;
    if (!srv_tcp_map_initialized) {
        fprintf(stdout, "  %s(): port map is not initialized\n", __FUNCTION__);
        return TE_RC(TE_TA_WIN32, EINVAL);
    }

    fprintf(stdout, "  try to remove mapping: %s\n", name);
    
    for (map_no = 0; map_no < TARPC_SERVER_MAP_SIZE; map_no++)
        if ((srv_tcp_map[map_no].port != 0) &&
            (strncmp(name, srv_tcp_map[map_no].name, TARPC_SERVER_NAME_LEN) == 0))
            break;

    if (map_no >= TARPC_SERVER_MAP_SIZE)
        return TE_RC(TE_TA_WIN32, EINVAL);

    srv_tcp_map[map_no].port = 0;
    *(srv_tcp_map[map_no].name) = '\0';
    
    return 0;
}

/* Lookup for mapping between AF_UNIX and AF_INET sockets */
static unsigned short
tarpc_server_mapping_lookup(char *name)
{
    int map_no;
    if (!srv_tcp_map_initialized) {
        fprintf(stdout, "  %s(): port map is not initialized\n", __FUNCTION__);
        return 0;
    }
    
    for (map_no = 0; map_no < TARPC_SERVER_MAP_SIZE; map_no++)
        if ((srv_tcp_map[map_no].port != 0) &&
            (strncmp(name, srv_tcp_map[map_no].name, TARPC_SERVER_NAME_LEN) == 0))
            break;

    if (map_no >= TARPC_SERVER_MAP_SIZE)
    {
        fprintf(stdout, "  unresolved mapping: %s\n", name);
        return 0;
    }
    fprintf(stdout, "  mapping: %s -> port %d\n", name, srv_tcp_map[map_no].port);

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
                WARN("RPC Server with PID %d exited due unknown reason", pid); 
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
        ERROR("Cannot create a socket for gathering the log, errno %d", errno);
        return -1;
    }

    fprintf(stdout, "Trying to bind socket\n");
    
    memset(&ta_log_addr, 0, sizeof(ta_log_addr));
    ta_log_addr.sin_family = AF_INET;
    ta_log_addr.sin_port = 0;
    if (bind(s, (struct sockaddr *)&ta_log_addr, len) < 0)
    {
        ERROR("Cannot bind a socket for gathering the log, errno %d", errno);
        close(s);
        return -1;
    }

    fprintf(stdout, "Trying to getsockname() on binded socket\n");
    
    if (getsockname(s, (struct sockaddr *)&ta_log_addr, &len) < 0)
    {
        ERROR("Cannot getsockname for a socket for gathering the log, errno %d",
              errno);
        close(s);
        return -1;
    }

    fprintf(stdout, "Log socket is binded to port %d\n",
            ntohs(ta_log_addr.sin_port));
    
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
        fprintf(stdout, "Socket for gathering the log was not created\n");
        ERROR("Socket for gathering the log was not created");
        return NULL;
    }

    fprintf(stdout, "Startting gather_log() routine\n");
    
    UNUSED(arg);
    while (1)
    {
        char log_pkt[RPC_LOG_PKT_MAX] = {0, };
      
        if (recv(s, log_pkt, sizeof(log_pkt), 0) < (int)RPC_LOG_OVERHEAD)
        {
            WARN("Failed to receive the logging message from RPC server");
            continue;
        }
        log_message(*(uint16_t *)log_pkt, "", LGR_USER,
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
 *
 * @return status code
 */
int 
tarpc_add_server(char *name)
{
    struct sockaddr_in  addr;
    int len = sizeof(struct sockaddr_in);
    
    srv *tmp;
    int  tries = MAX_CONNECT_TRIES;

    fprintf(stdout, "%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);

    if ((tmp = calloc(1, sizeof(srv))) == NULL)
    {
        ERROR("calloc() failed");
        return TE_RC(TE_TA_WIN32, ENOMEM);
    }
    strcpy(tmp->name, name);

    sprintf(tmp->pipename, "/tmp/%s_%u", name, ta_pid);

    if ((tmp->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERROR("socket(AF_INET, SOCK_STREAM, 0) failed: %d", errno);
        free(tmp);
        return TE_RC(TE_TA_WIN32, errno);
    }

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    if (bind(tmp->sock, &addr, sizeof(addr))) {
        fprintf(stdout, "cannot bind socket %d to family %d, addr: 0x%08lx:%d, errno=%d\n", tmp->sock,
            addr.sin_family, ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port), errno);
    }

    len = sizeof(struct sockaddr_in);
    if (getsockname(tmp->sock, (struct sockaddr *)&addr, &len) < 0)
    {
        fprintf(stdout, "cannot getsockname for socket %d, errno=%d\n", tmp->sock, errno);
    }

    fprintf(stdout, "socket %d binded to family %d, addr: 0x%08lx:%d\n", tmp->sock,
            addr.sin_family, ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(tarpc_server_mapping_lookup(tmp->pipename));

    if (addr.sin_port == 0)
        return TE_RC(TE_TA_WIN32, EINVAL);

    fprintf(stdout, "try to connect to family %d, addr: 0x%08lx:%d\n",
            addr.sin_family, ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));

    while (tries > 0 && connect(tmp->sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stdout, "connection to '%s' failed, errno=%d, retries=%d\n",
                name, errno, tries);
        usleep(10000);
        tries--;
    }
           
    if (tries == 0)
    {
        fprintf(stdout, "Cannot connect to RPC Server '%s', errno=%d\n", name, errno);
        ERROR("Cannot connect to RPC Server '%s'", name);
        free(tmp);
        return TE_RC(TE_TA_WIN32, errno);
    }

    fprintf(stdout, "connected successfuly to localhost:%d\n", ntohs(addr.sin_port));
    
    tmp->next = srv_list;
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
        return TE_RC(TE_TA_WIN32, ENOENT);
    }
        
    if (prev)
        prev->next = cur->next;
    else
        srv_list = cur->next;
        
    VERB("RPC Server '%s' is deleted from the list", cur->name);

    RELEASE_SRV(cur);

    return 0;
}

static void
sigint_handler(int s)
{
    UNUSED(s);
    exit(1);
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
    char     pipename[PIPENAME_LEN] = { 0, };
    int      sock;

    struct sockaddr_in addr;
    int len = sizeof(struct sockaddr_in);

    tarpc_in_arg  arg1;
    tarpc_in_arg *in = &arg1;

    fprintf(stdout, "%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);
    
    memset(&arg1, 0, sizeof(arg1));
    strcpy(arg1.name, (char *)arg);
    rpcserver_name = (char *)arg;

    RPC_LGR_MESSAGE(RING_LVL, "Started %s (PID %d, TID %u)", (char *)arg, 
                    (int)getpid(), (unsigned int)pthread_self());

    sigemptyset(&rpcs_received_signals);
    signal(SIGINT, sigint_handler);
    
    pmap_unset(tarpc, ver0);
    sprintf(pipename, "/tmp/%s_%u", (char *)arg, ta_pid);

    fprintf(stdout, "try to call pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS)\n");
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    fprintf(stdout, "try to create TCP socket\n");
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        close(ta_rpc_sync_socks[1]);
        RPC_LGR_MESSAGE(ERROR_LVL, "socket() failed");
        return ((SVCXPRT *)NULL);
    }

    fprintf(stdout, "try to call svctcp_create()\n");
    transp = svctcp_create(sock, 1024, 1024);
    if (transp == NULL)
    {
        close(sock);
        close(ta_rpc_sync_socks[1]);
        RPC_LGR_MESSAGE(ERROR_LVL, "svcunix_create() returned NULL");
        return NULL;
    }
    
    fprintf(stdout, "try to call getsockname()\n");
    if (getsockname(sock, (struct sockaddr *)&addr, &len) != 0)
    {
        close(sock);
        close(ta_rpc_sync_socks[1]);
        RPC_LGR_MESSAGE(ERROR_LVL, "getsockname() failed");
        return NULL;
    }
    
    fprintf(stdout, "RPC server binded to family %d, addr: 0x%08lx:%d\n",
            addr.sin_family, ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
    
    fprintf(stdout, "try to send RPC server port number to parent process\n");
    if (send(ta_rpc_sync_socks[1], &(addr.sin_port), sizeof(addr.sin_port), 0) < 0)
    {
        close(sock);
        close(ta_rpc_sync_socks[1]);
        return NULL;
    }

    close(ta_rpc_sync_socks[1]);

    fprintf(stdout, "try to call svc_register()\n");
    if (!svc_register(transp, tarpc, ver0, tarpc_1, 0))
    {
        close(sock);
        tarpc_server_mapping_del(pipename);
        RPC_LGR_MESSAGE(ERROR_LVL, "svc_register() failed");
        return NULL;
    }
    
    fprintf(stdout, "try to call svc_run()\n");
    svc_run();

    RPC_LGR_MESSAGE(ERROR_LVL, "Unreachable!");

    return NULL;
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
    
    fprintf(stdout, "%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);

    VERB("tarpc_server_create %s", name);
    
    if (!supervise_started)
    {
        pthread_t tid;

        ta_log_sock = create_log_socket();
        if (ta_log_sock < 0)
        {
            fprintf(stdout, "Cannot create RPC log gathering socket: %d\n", errno);
            ERROR("Cannot create RPC log gathering socket: %d", errno);
            return -1;
        }

        if (pthread_create(&tid, NULL, supervise_children, NULL) != 0)
        {
            fprintf(stdout, "Cannot create RPC servers supervising thread: %d\n", errno);
            ERROR("Cannot create RPC servers supervising thread: %d", errno);
            return -1;
        }

        if (pthread_create(&tid, NULL, gather_log, NULL) != 0)
        {
            fprintf(stdout, "Cannot create RPC log gathering thread: %d\n", errno);
            ERROR("Cannot create RPC log gathering thread: %d", errno);
            return -1;
        }

        supervise_started = 1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ta_rpc_sync_socks) < 0)
    {
        fprintf(stdout, "pipe() failed: %d\n", errno);
        ERROR("pipe() failed: %d", errno);
        return -1;
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stdout, "fork() failed: %d\n", errno);
        ERROR("fork() failed: %d", errno);
        return pid;
    }

    fprintf(stdout, "after fork(): %d\n", pid);

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
        char pipename[PIPENAME_LEN] = { 0, };

        FD_ZERO(&sync_fds);
        FD_SET(ta_rpc_sync_socks[0], &sync_fds);

        tv.tv_sec = TARPC_SERVER_SYNC_TIMEOUT / 1000000;
        tv.tv_usec = TARPC_SERVER_SYNC_TIMEOUT % 1000000;

        if (select(ta_rpc_sync_socks[0] + 1, &sync_fds, NULL, NULL, &tv) <= 0)
        {
            fprintf(stdout, "Synchronization with created RPC server is lost\n");
            close(ta_rpc_sync_socks[0]);
            return -1;
        }
        if (recv(ta_rpc_sync_socks[0], &port, sizeof(port), 0) < 0)
        {
            fprintf(stdout, "Could not retrieve RPC server port\n");
            close(ta_rpc_sync_socks[0]);
            return -1;
        }
        close(ta_rpc_sync_socks[0]);
        close(ta_rpc_sync_socks[1]);
        
        sprintf(pipename, "/tmp/%s_%u", name, ta_pid);
        fprintf(stdout, "try to call tarpc_server_mapping_add(%s, %d)\n",
                pipename, ntohs(port));
        if (tarpc_server_mapping_add(pipename, ntohs(port)) != 0) {
            fprintf(stdout, "too many tarpc servers created");
            return -1;
        }
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
        if (kill(cur->pid, SIGINT) != 0)
            ERROR("Failed to send SIGINT to PID %d", cur->pid);
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

/** Check, if some signals were received by the RPC server (as a process)
 * and return the mask of received signals. 
 */
                                                                   
bool_t                                                             
_sigreceived_1_svc(tarpc_sigreceived_in *in, tarpc_sigreceived_out *out, 
                   struct svc_req *rqstp)                            
{
    UNUSED(in);                                                                  
    UNUSED(rqstp);                                                 
    memset(out, 0, sizeof(*out));
    out->set = (tarpc_sigset_t)&rpcs_received_signals;
                                                                   
    return TRUE;                                                   
}

