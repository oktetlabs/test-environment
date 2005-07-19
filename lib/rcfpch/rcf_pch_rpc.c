/** @file 
 * @brief RCF Portable Command Handler
 *
 * RCF RPC support.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

//#define TCP_TRANSPORT

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef TCP_TRANSPORT

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#else

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

/** Maximum length of the pipe address */
#define PIPENAME_LEN    sizeof(((struct sockaddr_un *)0)->sun_path)

#endif /* TCP_TRANSPORT */

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_common.h"
#include "rcf_internal.h"
#include "comm_agent.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"

#include "rcf_pch_internal.h"

#include "rpc_xdr.h"
#include "rcf_rpc_defs.h"
#include "tarpc.h"

/** Data corresponding to one RPC server */
typedef struct rpcserver {
    struct rpcserver *next;   /**< Next server in the list */
    struct rpcserver *father; /**< Father pointer */

    char name[RCF_MAX_ID]; /**< RPC server name */
    int  sock;             /**< Socket for interaction with the server */
    int  ref;              /**< Number of thread children */
    
    int       pid;         /**< Process identifier */
    uint32_t  tid;         /**< Thread identifier or 0 */
    time_t    sent;        /**< Time of the last request sending */
    uint32_t  timeout;     /**< Timeout for the last sent request */
    int       last_sid;    /**< SID received with the last command */
    te_bool   dead;        /**< RPC server does not respond */
} rpcserver;

static rpcserver *list;       /**< List of all RPC servers */
static char      *rpc_buf;    /**< Buffer for sending/receiving of RPCs */
static int        lsock = -1; /**< Listening socket */

static int rpcserver_get(unsigned int, const char *, char *,
                         const char *);
static int rpcserver_add(unsigned int, const char *, const char *,
                         const char *);
static int rpcserver_del(unsigned int, const char *,
                         const char *);
static int rpcserver_list(unsigned int, const char *, char **);

static rcf_pch_cfg_object node_rpcserver =
    { "rpcserver", 0, NULL, NULL,
      (rcf_ch_cfg_get)rpcserver_get, NULL,
      (rcf_ch_cfg_add)rpcserver_add, (rcf_ch_cfg_del)rpcserver_del,
      (rcf_ch_cfg_list)rpcserver_list, NULL, NULL};

static struct rcf_comm_connection *conn_saved;

/** 
 * Receive data with specified timeout.
 *
 * @param s     socket
 * @param buf   buffer for reading
 * @param len   buffer length
 * @param t     timeout in seconds
 *
 * @return recv() return code or -2 if timeout expired
 */
static int
recv_timeout(int s, void *buf, int len, int t)
{
    struct timeval tv = { t, 0 };
    fd_set         set;
    int            rc = -1;
    
    while (rc < 0)
    {
        FD_ZERO(&set);
        FD_SET(s, &set);
    
        if ((rc = select(s + 1, &set, NULL, NULL, &tv)) == 0)
            return -2;
    }
        
    return recv(s, buf, len, 0);
}

/**
 * Receive encoded data from the RPC server.
 *
 * @param rpcs          RPC server handle
 * @param p_len         location for length of received data
 *
 * @return Status code
 */
static int
recv_result(rpcserver *rpcs, uint32_t *p_len)
{
    uint32_t len = RCF_RPC_HUGE_BUF_LEN, offset = 0;
    
    int rc = recv_timeout(rpcs->sock, &len, sizeof(len), 5);
    
    if (rc == -2)      /* AF_UNIX bug work-around */
        return EAGAIN;
    
    if (rc != sizeof(len))
    {
        ERROR("Failed to receive RPC data from the server %s", rpcs->name);
        return TE_RC(TE_RCF_PCH, ETESUNRPC);
    }
    
    if (len > RCF_RPC_HUGE_BUF_LEN)
    {
        ERROR("Too long RPC data bulk");
        return TE_RC(TE_RCF_PCH, ETESUNRPC);
    }
    
    while (offset < len)
    {
        int n = recv_timeout(rpcs->sock, rpc_buf + offset, 
                             RCF_RPC_HUGE_BUF_LEN - offset, 5);
        
        if (n <= 0)
        {
            ERROR("Too long RPC data bulk");
            return TE_RC(TE_RCF_PCH, ETESUNRPC);
        }

        offset += n;
    }

    *p_len = len;
    
    return 0;
}

/**
 * Call RPC on the specified RPC server.
 *
 * @param rpcs  RPC server structure
 * @param name  RPC name
 * @param in    input parameter C structure
 * @param out   output parameter C structure
 *
 * @return Status code
 */
static int
call(rpcserver *rpcs, char *name, void *in, void *out)
{
    size_t   buflen = RCF_RPC_HUGE_BUF_LEN;
    uint32_t len;
    int      rc;
    char     c = '\0';

    ((tarpc_in_arg *)in)->lib = &c;
    
    if (rpcs->sent > 0)
    {
        ERROR("RPC server %s is busy", rpcs->name);
        return TE_RC(TE_RCF_PCH, EBUSY);
    }
    
    if ((rc = rpc_xdr_encode_call(name, rpc_buf, &buflen, in)) != 0) 
    {
        if (TE_RC_GET_ERROR(rc) == ENOENT)
            ERROR("Unknown RPC %s is called from TA", name);
        else
            ERROR("Encoding of RPC %s input parameters failed", name);
        return rc;
    }
    
    len = buflen;
    if (send(rpcs->sock, &len, sizeof(len), 0) != (ssize_t)sizeof(len) ||
        send(rpcs->sock, rpc_buf, len, 0) != (ssize_t)len)
    {
        ERROR("Failed to send RPC data to the server %s", rpcs->name);
        return TE_RC(TE_RCF_PCH, ETESUNRPC);
    }
    
    if (recv_result(rpcs, &len) != 0)
        return TE_RC(TE_RCF_PCH, ETESUNRPC);

    if ((rc = rpc_xdr_decode_result(name, rpc_buf, len, out)) != 0)
    {
        ERROR("Decoding of RPC %s input parameters failed", name);
        return rc;
    }
        
    return 0;
}

/**
 * Create child thread RPC server.
 *
 * @param rpcs    RPC server handle
 *
 * @return Status code
 */
static int
create_thread_child(rpcserver *rpcs)
{
    int rc;

    tarpc_pthread_create_in  in;
    tarpc_pthread_create_out out;

    RING("Create thread RPC server '%s' from '%s'",
         rpcs->name, rpcs->father->name);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.name.name_len = strlen(rpcs->name) + 1;
    in.name.name_val = rpcs->name;
    
    if ((rc = call(rpcs->father, "pthread_create", &in, &out)) != 0)
        return rc;
        
    if (out.common._errno != 0 || out.retval != 0)
    {
        ERROR("RPC pthread_create() failed on the server %s with errno 0x%X", 
              rpcs->father->name, out.common._errno);
        return out.common._errno;
    }
    
    rpcs->tid = out.tid;
    
    return 0;
}

/**
 * Delete thread child RPC server.
 *
 * @param rpcs    RPC server handle
 */
static void
delete_thread_child(rpcserver *rpcs)
{
    tarpc_pthread_cancel_in  in;
    tarpc_pthread_cancel_out out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.tid = rpcs->tid;
    
    if (call(rpcs->father, "pthread_cancel", &in, &out) != 0)
        return;
        
    if (out.common._errno != 0 || out.retval != 0)
    {
        WARN("RPC pthread_cancel() failed on the server %s with errno 0x%X",
              rpcs->father->name, out.common._errno);
    }
}

/**
 * Create child RPC server.
 *
 * @param rpcs    RPC server handle
 *
 * @return Status code
 */
static int
fork_child(rpcserver *rpcs)
{
    int rc;

    tarpc_fork_in  in;
    tarpc_fork_out out;

    RING("Fork RPC server '%s' from '%s'",
         rpcs->name, rpcs->father->name);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.name.name_len = strlen(rpcs->name) + 1;
    in.name.name_val = rpcs->name;
    
    if ((rc = call(rpcs->father, "fork", &in, &out)) != 0)
        return rc;
        
    if (out.common._errno != 0 || out.pid < 0)
    {
        ERROR("RPC fork() failed on the server %s with errno 0x%X", 
              rpcs->father->name, out.common._errno);
        return out.common._errno;
    }
    
    rpcs->pid = out.pid;
    
    return 0;
}


/**
 * Kill RPC server.
 *
 * @param rpcs    RPC server handle
 */
static void
kill_rpcserver(rpcserver *rpcs)
{
    int rc;

    rc = kill(rpcs->pid, SIGTERM);
    RING("Sent SIGTERM signal to RPC server %s:%d - rc=%d, errno=%d",
         rpcs->name, rpcs->pid, rc, (rc == 0) ? 0 : errno);
    sleep(1);
    rc = kill(rpcs->pid, SIGKILL);
    RING("Sent SIGKILL signal to RPC server %s:%d - rc=%d, errno=%d",
         rpcs->name, rpcs->pid, rc, (rc == 0) ? 0 : errno);
}

/**
 * Accept the connection from newly created or execve()-ed RPC server
 * and call RPC getpid() on it.
 *
 * @param rpcs  RPC server structure
 *
 * @return Status code
 */
static int
connect_getpid(rpcserver *rpcs)
{
    struct timeval  tv = { 50, 0 };
    fd_set          set;
    struct sockaddr addr;
    int             len = sizeof(addr);
    int             rc;
    
    tarpc_getpid_in  in;
    tarpc_getpid_out out;
    
    FD_ZERO(&set);
    FD_SET(lsock, &set);
    
    /* Accept the connection */
    while ((rc = select(lsock + 1, &set, NULL, NULL, &tv)) <= 0)
    {
        if (rc == 0)
        {
            ERROR("RPC server %s does not try to connect", rpcs->name);
            return TE_RC(TE_RCF_PCH, ETIMEDOUT);
        }
        else if (errno != EINTR)
        {
            int err = errno;
        
            ERROR("select() failed with unexpected errno=%d", err);
            return TE_RC(TE_RCF_PCH, err);
        }
    }
    if ((rpcs->sock = accept(lsock, &addr, &len)) < 0)
    {
        int err = errno;
        
        ERROR("Failed to accept connection from RPC server %s; errno 0x%X",
              rpcs->name, err);
        return TE_RC(TE_RCF_PCH, err);
    }
    
    /* Call getpid() RPC to verify that the server is usable */
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    if ((rc = call(rpcs, "getpid", &in, &out)) != 0)
        return rc;
        
    if (out.common._errno != 0)
    {
        ERROR("RPC getpid() failed on the server %s with errno 0x%X", 
              rpcs->name, out.common._errno);
        return out.common._errno;
    }
    
    rpcs->pid = out.retval;

    return 0;
}

/**
 * Send error to RCF if RPC server is dead.
 *
 * @param rpcs  RPC server handle
 * @param rc    error code
 */
static void
rpc_error(rpcserver *rpcs, int rc)
{
    rc = TE_RC(TE_RCF_PCH, rc); 

    sprintf(rpc_buf, "SID %d %d", rpcs->last_sid, rc); 
    rcf_comm_agent_reply(conn_saved, rpc_buf, strlen(rpc_buf) + 1);
} 

/**
 * Entry point for the thread forwarding answers from RPC servers 
 * to RCF.
 */
static void *
dispatch(void *arg)
{
    fd_set set0, set1;
    
    UNUSED(arg);
    
    FD_ZERO(&set0);
    FD_ZERO(&set1);
    
    while (TRUE)
    {
        struct timeval tv = { 1, 0 };
        rpcserver     *rpcs;
        time_t         now;
        uint32_t       len;
        int            rc;
        
        set0 = set1;

        if ((rc = select(FD_SETSIZE, &set0, NULL, NULL, &tv)) < 0)
        {
            int err = errno;
            
            WARN("select() failed; errno %d", err);
        }
        
        FD_ZERO(&set1);
        rcf_ch_lock();
        now = time(NULL);
        for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
        {
            if (rpcs->dead)
                continue;
               
            if (rpcs->sent > 0 && 
                ((uint32_t)(now - rpcs->sent) > rpcs->timeout ||
                 (rpcs->timeout == 0xFFFFFFFF && now - rpcs->sent > 2)))
            {
                ERROR("Timeout on server %s", rpcs->name);
                rpcs->dead = TRUE;
                rpc_error(rpcs, ETERPCTIMEOUT);
                continue;
            } 
            FD_SET(rpcs->sock, &set1);

            if (!FD_ISSET(rpcs->sock, &set0))
                continue;
                
            if (rpcs->last_sid == 0) /* AF_UNIX bug work-around */
                continue;
                
            if ((rc = recv_result(rpcs, &len)) != 0)
            {
                if (rc != EAGAIN) /* AF_UNIX bug work-around */
                {
                    rpcs->dead = TRUE;
                    rpc_error(rpcs, ETERPCDEAD);
                }
                continue;
            }
            
            /* Send response */
            if (len < RCF_MAX_VAL && strcmp_start("<?xml", rpc_buf) == 0)
            {
                /* Send as string */
                char *s0 = rpc_buf + RCF_MAX_VAL;
                char *s = s;
                
                s += sprintf(s, "SID %d 0 ", rpcs->last_sid);
                write_str_in_quotes(s, rpc_buf, len);
                rcf_comm_agent_reply(conn_saved, s0, strlen(s0) + 1);
            }
            else
            {
                /* Send as binary attachment */
                char s[64];
                
                snprintf(s, sizeof(s), "SID %d 0 attach %u",
                         rpcs->last_sid, (unsigned)len);
                rcf_comm_agent_reply(conn_saved, s, strlen(s) + 1);
                rcf_comm_agent_reply(conn_saved, rpc_buf, len);
            }
            
            if (rpcs->timeout == 0xFFFFFFFF) /* execve() */
            {
                rpcs->sent = 0;
                if (connect_getpid(rpcs) != 0)
                {
                    rpcs->dead = TRUE;
                    continue;
                }
            }
            
            rpcs->timeout = rpcs->sent = rpcs->last_sid = 0;
        }
        rcf_ch_unlock();
    }
}
 

/** 
 * Initialize RCF RPC server structures and link RPC configuration
 * nodes to the root.
 */
void 
rcf_pch_rpc_init()
{
    rcf_pch_cfg_object *root = rcf_ch_conf_root();
#ifdef TCP_TRANSPORT    
    struct sockaddr_in  addr;
#else    
    struct sockaddr_un  addr;
#endif    
    int                 len;
    char                port[16];
    pthread_t           tid = 0;
    
#define RETERR(msg...) \
    do {                  \
        ERROR(msg);       \
        if (lsock >= 0)   \
            close(lsock); \
        free(rpc_buf);    \
        rpc_buf = NULL;   \
        return;           \
    } while (0)

    if ((rpc_buf = malloc(RCF_RPC_HUGE_BUF_LEN)) == NULL)
        RETERR("Cannot allocate memory for RPC buffer on the TA");

#ifdef TCP_TRANSPORT    
    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        RETERR("Failed to open listening socket for RPC servers");
#else
    if ((lsock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        RETERR("Failed to open listening socket for RPC servers");
#endif        

    memset(&addr, 0, sizeof(addr));
#ifdef TCP_TRANSPORT    
    addr.sin_family = AF_INET;
    len = sizeof(struct sockaddr_in);
#else
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), 
             "/tmp/terpc_%d", getpid());
    len = sizeof(struct sockaddr_un) - PIPENAME_LEN +  
          strlen(addr.sun_path);
#endif    

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ((struct sockaddr *)&addr)->sa_len = len;
#endif

    if (bind(lsock, (struct sockaddr *)&addr, len) < 0)
    {
        int err = errno;
        
        RETERR("Failed to bind listening socket for RPC servers; "
               "errno %d", err);
    }
        
    if (listen(lsock, 1) < 0)
        RETERR("listen() failed for listening socket for RPC servers");

#ifdef TCP_TRANSPORT    
    if (getsockname(lsock, (struct sockaddr *)&addr, &len) < 0)
        RETERR("getsockname() failed for listening socket for RPC servers");

    sprintf(port, "%d", addr.sin_port);
#else    
    strcpy(port, addr.sun_path);
#endif
        
    if (setenv("TE_RPC_PORT", port, 1) < 0)
    {
        int err = errno;
        
        RETERR("Failed to set TE_RPC_PORT environment variable;"
               " errno 0x%X", err);
    }

    if (pthread_create(&tid, NULL, dispatch, NULL) != 0)
        RETERR("Failed to create the thread for RPC servers dispatching");
    
    node_rpcserver.brother = root->son;
    root->son = &node_rpcserver;
    
#undef RETERR    
}

/** 
 * Cleanup RCF RPC server structures.
 */
void 
rcf_pch_rpc_shutdown()
{
    rpcserver *rpcs, *next;
#ifndef TCP_TRANSPORT
    char *pipename = getenv("TE_RPC_PORT");
#endif        
    
    if (lsock >= 0)
        close(lsock);
        
#ifndef TCP_TRANSPORT
    if (pipename != NULL)
        unlink(pipename);
#endif        
        
    rcf_ch_lock();
    for (rpcs = list; rpcs != NULL; rpcs = next)
    {
        next = rpcs->next;
        close(rpcs->sock);
        usleep(100);
        kill_rpcserver(rpcs);
        free(rpcs);
    }
    rcf_ch_unlock();

    free(rpc_buf);
    rpc_buf = NULL;
}


/**
 * Get RPC server value (father name).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param name          RPC server name
 *
 * @return Status code
 */
static int 
rpcserver_get(unsigned int gid, const char *oid, char *value,
              const char *name)
{
    rpcserver *rpcs;
    
    UNUSED(gid);
    UNUSED(oid);
    
    rcf_ch_lock();
    
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, name) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        rcf_ch_unlock();
        return TE_RC(TE_RCF_PCH, ETENOSUCHNAME);
    }
        
    if (rpcs->father == NULL)
        *value = 0;
    else if (rpcs->tid > 0)
        sprintf(value, "thread_%s", rpcs->father->name);
    else
        sprintf(value, "fork_%s", rpcs->father->name);
        
    rcf_ch_unlock();        
        
    return 0;        
}

              
/**
 * Create RPC server.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         fork_<father_name> or thread_<father_name> or
 *                      empty string
 * @param new_name      New RPC server name
 *
 * @return Status code
 */
static int 
rpcserver_add(unsigned int gid, const char *oid, const char *value,
              const char *new_name)
{
    rpcserver  *rpcs;
    rpcserver  *father = NULL;
    const char *father_name;
    int         rc;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if (strcmp_start("thread_", value) == 0)
        father_name = value + strlen("thread_");
    else if (strcmp_start("fork_", value) == 0)
        father_name = value + strlen("fork_");
    else if (value[0] == '\0')
        father_name = NULL;
    else
    {
        ERROR("Incorrect RPC server '%s' father '%s'", new_name, value);
        return TE_RC(TE_RCF_PCH, EINVAL);
    }
    
    rcf_ch_lock();
    for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
    {
        if (strcmp(rpcs->name, new_name) == 0)
        {
            rcf_ch_unlock();
            return TE_RC(TE_RCF_PCH, EEXIST);
        }
            
        if (father_name != NULL && strcmp(rpcs->name, father_name) == 0)
            father = rpcs;
    }
    
    if (father_name != NULL && father == NULL)
    {
        rcf_ch_unlock();
        ERROR("Cannot find father '%s' for RPC server '%s' (%s)",
              father_name, new_name, value);
        return TE_RC(TE_RCF_PCH, EEXIST);
    }
    
    if ((rpcs = (rpcserver *)calloc(1, sizeof(*rpcs))) == NULL)
    {
        rcf_ch_unlock();
        ERROR("%s(): calloc(1, %u) failed", __FUNCTION__,
              (unsigned)sizeof(*rpcs));
        return TE_RC(TE_RCF_PCH, ENOMEM);
    }
    
    strcpy(rpcs->name, new_name);
    rpcs->father = father;
    if (father == NULL)
    {
        if ((rpcs->pid = fork()) == 0)
        {
            rcf_ch_unlock();
            rcf_pch_detach();
            rcf_pch_rpc_server(rpcs->name);
#ifdef __CYGWIN__
            _exit(0);
#else
            exit(0);
#endif
        }
        
        if (rpcs->pid < 0)
        {
            int err = errno;
            
            rcf_ch_unlock();
            free(rpcs);
            ERROR("Failed to spawn RPC server process; errno 0x%X", err);
            fprintf(stderr, "Failed to spawn RPC server process\n");
            return TE_RC(TE_RCF_PCH, err);
        }
    }
    else 
    {
        if (*value == 't')
            rc = create_thread_child(rpcs);
        else
            rc = fork_child(rpcs);
            
        if (rc != 0)
        {
            rcf_ch_unlock();
            free(rpcs);
            return TE_RC(TE_RCF_PCH, rc);
        }
    }
    
    if ((rc = connect_getpid(rpcs)) != 0)
    {
        if (rpcs->tid > 0)
            delete_thread_child(rpcs);
        else
            kill_rpcserver(rpcs);
        rcf_ch_unlock();
        free(rpcs);
        return TE_RC(TE_RCF_PCH, rc);
    }
    
    if (rpcs->tid > 0)
        rpcs->father->ref++;
    
    rpcs->next = list;
    list = rpcs;
    
    rcf_ch_unlock();
    
    return 0;
}

/**
 * Delete RPC server.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param name          RPC server name
 *
 * @return Status code
 */
static int 
rpcserver_del(unsigned int gid, const char *oid, const char *name)
{
    rpcserver *rpcs, *prev;
    
    UNUSED(gid);
    UNUSED(oid);
    
    rcf_ch_lock();
    for (rpcs = list, prev = NULL; 
         rpcs != NULL; 
         prev = rpcs, rpcs = rpcs->next)
    {
        if (strcmp(rpcs->name, name) == 0)
            break;
    }
    if (rpcs == NULL)
    {
        rcf_ch_unlock();
        ERROR("RPC server '%s' to be deleted not found", name);
        return TE_RC(TE_RCF_PCH, ENOENT);
    }
    
    if (rpcs->ref > 0)
    {
        rcf_ch_unlock();
        ERROR("Cannot delete RPC server '%s' with threads", name);
        return TE_RC(TE_RCF_PCH, EPERM);
    }
    
    if (prev != NULL)
        prev->next = rpcs->next;
    else
        list = rpcs->next;
        
    if (rpcs->tid > 0)
        rpcs->father->ref--;

    /* Try soft shutdown first */
    if (rpcs->sent > 0 || 
        send(rpcs->sock, "FIN", 4, 0) != 4 ||
        recv_timeout(rpcs->sock, rpc_buf, sizeof(rpc_buf), 5) != 3 || 
        strcmp(rpc_buf, "OK") != 0)
    {
        WARN("Soft shutdown of RPC server '%s' failed", rpcs->name);
        if (rpcs->tid > 0)
            delete_thread_child(rpcs);
        else
            kill_rpcserver(rpcs);
    }

    rcf_ch_unlock();
    
    close(rpcs->sock);
    free(rpcs);
    
    return 0;
}

static int 
rpcserver_list(unsigned int gid, const char *oid, char **value)
{
    rpcserver *rpcs;
    char      *s = rpc_buf;
    
    UNUSED(gid);
    UNUSED(oid);
    
    rcf_ch_lock();
    *rpc_buf = 0;
    
    for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
        s += sprintf(s, "%s ", rpcs->name);
        
    if ((*value = strdup(rpc_buf)) == NULL)
    {
        rcf_ch_unlock();
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, rpc_buf);
        return TE_RC(TE_RCF_PCH, ENOMEM);
    }
    rcf_ch_unlock();
    
    return 0;
}

/**
 * RPC handler.
 *
 * @param conn          connection handle
 * @param sid           session identifier
 * @param data          pointer to data in the command buffer
 * @param len           length of encoded data 
 * @param server        RPC server name
 * @param timeout       timeout in seconds or 0 for unlimited
 *
 * @return 0 or error returned by communication library
 */
int 
rcf_pch_rpc(struct rcf_comm_connection *conn, int sid, 
            const char *data, size_t len,
            const char *server, uint32_t timeout)
{
    rpcserver *rpcs;
    uint32_t   rpc_data_len = len;
    
    conn_saved = conn;
    
#define RETERR(rc) \
    do {                                                                \
        int _rc = TE_RC(TE_RCF_PCH, rc);                                \
                                                                        \
        sprintf(rpc_buf, "SID %d %d", sid, _rc);                        \
        _rc = rcf_comm_agent_reply(conn, rpc_buf, strlen(rpc_buf) + 1); \
        rcf_ch_unlock();                                                \
        return _rc;                                                     \
    } while (0)
    
    rcf_ch_lock();
    
    /* Look for the RPC server */
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, server) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        ERROR("Failed to find RPC server %s", server);
        RETERR(ENOENT);
    }
    
    if (rpcs->dead)
    {
        ERROR("Request to dead RPC server %s", server);
        RETERR(ETERPCDEAD);
    }
    
    /* Mark RPC server as busy */
    if (rpcs->sent != 0)
    {
        ERROR("RPC server %s is busy", server);
        RETERR(EBUSY);
    }
    rpcs->sent = time(NULL);
    rpcs->last_sid = sid;
    rpcs->timeout = timeout == 0xFFFFFFFF ? timeout : timeout / 1000;
    
    /* Send encoded data to server */
    if (send(rpcs->sock, &rpc_data_len, sizeof(rpc_data_len), 0) != 
        sizeof(rpc_data_len) ||
        send(rpcs->sock, data, rpc_data_len, 0) != (ssize_t)rpc_data_len)
    {
        ERROR("Failed to send RPC data to the server %s", rpcs->name);
        RETERR(ETESUNRPC);
    }
    
    /* The answer will be sent by the thread */

    rcf_ch_unlock();
    
    return 0;
    
#undef RETERR    
}            


#include "ta_logfork.h"

#ifdef HAVE_SIGNAL_H

static void
sig_handler(int s)
{
    UNUSED(s);
    exit(1);
}

#endif

/**
 * Entry function for RPC server. 
 *
 * @param name   RPC server name
 *
 * @return NULL
 */
void *
rcf_pch_rpc_server(const char *name)
{
#ifdef TCP_TRANSPORT    
    struct sockaddr_in  addr;
#else    
    struct sockaddr_un  addr;
#endif    

    char *buf = NULL;
    int   s = -1;
    int   sock_len;

#define STOP(msg...)    \
    do {                \
        ERROR(msg);     \
        goto cleanup;   \
    } while (0)

#ifdef HAVE_SIGNAL_H
    signal(SIGTERM, sig_handler);
#endif

    if (logfork_register_user(name) != 0)
    {
        fprintf(stderr,
                "logfork_register_user() failed to register %s server\n",
                name);
    }

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    if ((buf = malloc(RCF_RPC_HUGE_BUF_LEN)) == NULL)
        STOP("Failed to allocate the buffer for RPC data");
    
    memset(&addr, 0, sizeof(addr));
#ifdef TCP_TRANSPORT    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = atoi(getenv("TE_RPC_PORT"));
    sock_len = sizeof(struct sockaddr_in);
#else
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, getenv("TE_RPC_PORT"));
    sock_len = sizeof(struct sockaddr_un) - PIPENAME_LEN +  
               strlen(addr.sun_path);
#endif

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ((struct sockaddr *)&addr)->sa_len = sock_len;
#endif

#ifdef TCP_TRANSPORT    
    s = socket(AF_INET, SOCK_STREAM, 0);
#else
    s = socket(AF_UNIX, SOCK_STREAM, 0);
#endif    
    if (s < 0)
    {
        ERROR("Failed to open socket");
        goto cleanup;
    }

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        int err = errno;
        
        STOP("Failed to connect to TA; errno = %d", err);
    }
    
    RING("RPC server '%s' (re-)started (PID %d, TID %u)",
         name, (int)getpid(), (unsigned int)pthread_self());

    while (TRUE)
    {
        char      rpc_name[RCF_RPC_MAX_NAME];        
                                        /* RPC name, i.e. "bind" */
        void     *in = NULL;            /* Input parameter C structure */
        void     *out = NULL;           /* Output parameter C structure */
        rpc_info *info = NULL;          /* RPC information */
        te_bool   result = FALSE;       /* "rc" attribute */

        size_t    buflen;
        uint32_t  len, offset = 0;
        
        strcpy(rpc_name, "Unknown");
        
        /* 
         * We cannot call recv() here, but should call select() because
         * otherwise in the case of winsock2 the thread is not in
         * alertable state and async I/O callbacks are not called.
         */
        
        if (recv_timeout(s, &len, sizeof(len), 0xFFFF) < 
            (int)sizeof(len))
        {
            STOP("recv() failed");
        }
        
        if (strcmp((char *)&len, "FIN") == 0)
        {
            send(s, "OK", sizeof("OK"), 0);
            RING("RPC server '%s' finished", name);
            sleep(1);
            goto cleanup;
        }
        
        if (len > RCF_RPC_HUGE_BUF_LEN)
            STOP("Too long RPC data bulk");
        
        while (offset < len)
        {
            int n = recv(s, buf + offset, RCF_RPC_HUGE_BUF_LEN - offset, 0);
            
            if (n <= 0)
                STOP("recv() failed");

            offset += n;
        }
        if (rpc_xdr_decode_call(buf, len, rpc_name, &in) != 0)
        {
            ERROR("Decoding of RPC %s call failed", rpc_name);
            goto result;
        }
        
        info = rpc_find_info(rpc_name);
        assert(info != NULL);
        
        if ((out = calloc(1, info->out_len)) == NULL)
        {
            ERROR("Memory allocation failure");
            goto result;
        }
        
        result = (info->rpc)(in, out, NULL);
        
        result: /* Send an answer */
        
        if (in != NULL && info != NULL)
            rpc_xdr_free(info->in, in);
        free(in);
            
        buflen = RCF_RPC_HUGE_BUF_LEN;
        if (rpc_xdr_encode_result(rpc_name, result, buf, &buflen, out) != 0)
            STOP("Fatal error: encoding of RPC %s output "
                 "parameters failed", rpc_name);
            
        if (info != NULL)
            rpc_xdr_free(info->out, out);
        free(out);
        
        len = buflen;
        if (send(s, &len, sizeof(len), 0) < (ssize_t)sizeof(len) ||
            send(s, buf, len, 0) < (ssize_t)len)
        {
            STOP("send() failed");
        }
    }

cleanup:    
    free(buf);
    if (s >= 0)
        close(s);
    
#undef STOP    

    return NULL;
}
