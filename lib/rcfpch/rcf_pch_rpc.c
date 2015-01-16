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

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "rcf_pch_internal.h"

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_common.h"
#include "rcf_internal.h"
#include "comm_agent.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "logfork.h"

#include "rpc_xdr.h"
#include "rcf_rpc_defs.h"
#include "tarpc.h"

#include "rpc_transport.h"

/** Data corresponding to one RPC server */
typedef struct rpcserver {
    struct rpcserver *next;   /**< Next server in the list */
    struct rpcserver *father; /**< Father pointer */

    char name[RCF_MAX_ID];  /**< RPC server name */
    char value[RCF_MAX_ID]; /**< RPC server father name */
    
    rpc_transport_handle handle; /**< Transport handle */
    
    int       ref;         /**< Number of thread children */
    pid_t     pid;         /**< Process identifier */

    tarpc_pthread_t  tid;  /**< Thread identifier or 0 */
    
    uint32_t  timeout;     /**< Timeout for the last sent request */
    int       last_sid;    /**< SID received with the last command */
    te_bool   dead;        /**< RPC server does not respond */
    te_bool   finished;    /**< RPC server process (or thread) was
                                terminated, waitpid() (pthread_join())
                                was already  called (if required) */
    time_t    sent;        /**< Time of the last request sending */
} rpcserver;

static rpcserver *list;        /**< List of all RPC servers */
static uint8_t   *rpc_buf;     /**< Buffer for receiving of RPC answers;
                                    may be used in dispatch thread
                                    context only */

static te_errno rpcserver_get(unsigned int, const char *, char *,
                              const char *);
static te_errno rpcserver_set(unsigned int, const char *, const char *,
                              const char *);
static te_errno rpcserver_add(unsigned int, const char *, const char *,
                              const char *);
static te_errno rpcserver_del(unsigned int, const char *,
                              const char *);
static te_errno rpcserver_list(unsigned int, const char *, char **);

static te_errno rpcserver_dead_get(unsigned int, const char *, char *,
                                   const char *);
static te_errno rpcserver_dead_set(unsigned int, const char *, char *,
                                   const char *);
static te_errno rpcserver_finished_get(unsigned int, const char *, char *,
                                       const char *);
static te_errno rpcserver_finished_set(unsigned int, const char *, char *,
                                       const char *);

static rcf_pch_cfg_object node_rpcserver_finished =
    { "finished", 0, NULL, NULL,
      (rcf_ch_cfg_get)rpcserver_finished_get, 
      (rcf_ch_cfg_set)rpcserver_finished_set,
      NULL, NULL, NULL, NULL, NULL};

static rcf_pch_cfg_object node_rpcserver_dead =
    { "dead", 0, NULL, &node_rpcserver_finished,
      (rcf_ch_cfg_get)rpcserver_dead_get, 
      (rcf_ch_cfg_set)rpcserver_dead_set,
      NULL, NULL, NULL, NULL, NULL};

static rcf_pch_cfg_object node_rpcserver =
    { "rpcserver", 0, &node_rpcserver_dead, NULL,
      (rcf_ch_cfg_get)rpcserver_get, (rcf_ch_cfg_set)rpcserver_set,
      (rcf_ch_cfg_add)rpcserver_add, (rcf_ch_cfg_del)rpcserver_del,
      (rcf_ch_cfg_list)rpcserver_list, NULL, NULL};

static struct rcf_comm_connection *conn_saved;

/** Lock for protection of RPC servers list */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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
    uint8_t buf[1024];
    size_t  len = sizeof(buf);
    int     rc;

    ((tarpc_in_arg *)in)->use_libc = 0;
    
    if (rpcs->sent > 0)
    {
        ERROR("RPC server %s is busy", rpcs->name);
        return TE_RC(TE_RCF_PCH, TE_EBUSY);
    }
    
    if ((rc = rpc_xdr_encode_call(name, buf, &len, in)) != 0) 
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            ERROR("Unknown RPC %s is called from TA", name);
        else
            ERROR("Encoding of RPC %s input parameters failed", name);
        return rc;
    }
    
    if (rpc_transport_send(rpcs->handle, buf, len) != 0)
    {
        ERROR("Failed to send RPC data to the server %s", rpcs->name);
        return TE_RC(TE_RCF_PCH, TE_ESUNRPC);
    }
    
    len = sizeof(buf);
    if (rpc_transport_recv(rpcs->handle, buf, &len, 5) != 0)
    {
        ERROR("Failed to receive RPC data from the server %s", rpcs->name);
        return TE_RC(TE_RCF_PCH, TE_ESUNRPC);
    }

    if ((rc = rpc_xdr_decode_result(name, buf, len, out)) != 0)
    {
        ERROR("Decoding of RPC %s output parameters (length %d) failed", 
              name, len);
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

    tarpc_thread_create_in  in;
    tarpc_thread_create_out out;

    RING("Create thread RPC server '%s' from '%s'",
         rpcs->name, rpcs->father->name);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.name.name_len = strlen(rpcs->name) + 1;
    in.name.name_val = rpcs->name;
    
    if ((rc = call(rpcs->father, "thread_create", &in, &out)) != 0)
        return rc;
        
    if (out.retval != 0)
    {
        ERROR("RPC thread_create() failed on the server %s with errno %r", 
              rpcs->father->name, out.common._errno);
        return (out.common._errno != 0) ?
                   out.common._errno : TE_RC(TE_RCF_PCH, TE_ECORRUPTED);
    }
    
    rpcs->tid = out.tid;
    rpcs->pid = rpcs->father->pid;
    
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
    tarpc_thread_cancel_in  in;
    tarpc_thread_cancel_out out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.tid = rpcs->tid;
    
    if (call(rpcs->father, "thread_cancel", &in, &out) != 0)
        return;
        
    if (out.retval != 0)
    {
        WARN("RPC thread_cancel() failed on the server %s with errno %r",
              rpcs->father->name, out.common._errno);
    }
}

/**
 * Join thread child RPC server.
 *
 * @param rpcs    RPC server handle
 * 
 * @return Status code
 */
static int
join_thread_child(rpcserver *rpcs)
{
    tarpc_thread_join_in  in;
    tarpc_thread_join_out out;
    int rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.tid = rpcs->tid;

    if ((rc = call(rpcs->father, "thread_join", &in, &out)) != 0)
    {
        ERROR("thread_join call failed");
        return rc;
    }

    if (out.retval != 0)
    {
        ERROR("RPC thread_join() failed on the server %s with errno %r",
              rpcs->father->name, out.common._errno);
        return TE_RC(TE_RCF_PCH, te_rc_os2te(out.retval));
    }

    return 0;
}

/**
 * Call waitpid() on terminated children RPC server.
 *
 * @param rpcs    RPC server handle
 */
static void
waitpid_child(rpcserver *rpcs)
{
    tarpc_waitpid_in  in;
    tarpc_waitpid_out out;

    if (rpcs->father != NULL && rpcs->father->dead)
        return;
    if (rpcs->father == NULL)
    {
        /*
         * waitpid() should be called in SIGCHLD signal handler
         * of Test Agent
         */
        return;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.pid = rpcs->pid;

    if (call(rpcs->father, "waitpid", &in, &out) != 0)
        return;

    if (out.pid == -1)
    {
        WARN("RPC waitpid() failed on the server %s with errno %r",
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
static te_errno
fork_child(rpcserver *rpcs, te_bool exec)
{
    int rc;

    tarpc_create_process_in  in;
    tarpc_create_process_out out;

    RING("Fork RPC server '%s' from '%s'", rpcs->name, rpcs->father->name);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    in.name.name_len = strlen(rpcs->name) + 1;
    in.name.name_val = rpcs->name;
    in.flags = RCF_RPC_SERVER_GET_INHERIT | RCF_RPC_SERVER_GET_NET_INIT;
    if (exec)
        in.flags |= RCF_RPC_SERVER_GET_EXEC;
    
    if ((rc = call(rpcs->father, "create_process", &in, &out)) != 0)
        return rc;
        
    if (out.pid < 0)
    {
        ERROR("RPC create_process() failed on the server %s with errno %r", 
              rpcs->father->name, out.common._errno);
        return (out.common._errno != 0) ?
                   out.common._errno : TE_RC(TE_RCF_PCH, TE_ECORRUPTED);
    }
    
    rpcs->pid = out.pid;
    
    return 0;
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
    tarpc_getpid_in  in;
    tarpc_getpid_out out;
    te_errno         rc;
    
    rc = rpc_transport_connect_rpcserver(rpcs->name, &rpcs->handle);
    if (rc != 0)
        return rc;
        
    /* Call getpid() RPC to verify that the server is usable */
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;
    VERB("Getting RPC server '%s' PID...", rpcs->name);
    if ((rc = call(rpcs, "getpid", &in, &out)) != 0)
        return rc;
        
    if (!RPC_IS_ERRNO_RPC(out.common._errno))
    {
        ERROR("RPC getpid() failed on the server %s with errno %r", 
              rpcs->name, out.common._errno);
        return out.common._errno;
    }
    
    rpcs->pid = out.retval;
    VERB("Connection with RPC server '%s' established", rpcs->name);

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
    char error_buf[32];
    int  n;
    
    rc = TE_RC(TE_RCF_PCH, rc); 

    n = snprintf(error_buf, sizeof(error_buf),
                 "SID %d %d", rpcs->last_sid, rc) + 1;
    RCF_CH_LOCK;
    rcf_comm_agent_reply(conn_saved, error_buf, n);
    RCF_CH_UNLOCK;
} 

/**
 * Entry point for the thread forwarding answers from RPC servers 
 * to RCF. The thread should not release memory allocated for
 * RPC server.
 */
static void *
dispatch(void *arg)
{
    UNUSED(arg);
    
    while (TRUE)
    {
        rpcserver *rpcs;
        time_t     now;
        size_t     len;
        te_errno   rc;
        uint32_t   pass_time = 0;
        
        rpc_transport_read_set_init();
        
        pthread_mutex_lock(&lock);
        for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
        {
            /* 
             * We do not require sent > 0, because RPC is sent from the 
             * other thread and sent is changed there. If we do not include 
             * RPC server handle to the set now, next time we'll have a 
             * chance only after second.
             */
            if (!rpcs->dead)
                rpc_transport_read_set_add(rpcs->handle);
        }
        pthread_mutex_unlock(&lock);
        
        rpc_transport_read_set_wait(1);
        pthread_mutex_lock(&lock);
        now = time(NULL);
        for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
        {
            if (rpcs->dead || rpcs->sent == 0)
                continue;
               
            /* Report when time goes back */
            if (now - rpcs->sent < 0)
            {
                WARN("Time goes back! Send request time = %d, "
                     "'Now' time = %d", rpcs->sent, now);
                sleep(1);
                now = time(NULL);
                continue;
            }
            else
                pass_time = (uint32_t)(now - rpcs->sent);

            if (rpcs->sent > 0 && 
                (pass_time > rpcs->timeout ||
                 (rpcs->timeout == 0xFFFFFFFF && now - rpcs->sent > 5)))
            {
                ERROR("Timeout on server %s (timeout=%ds)",
                      rpcs->name, rpcs->timeout);
                rpcs->dead = TRUE;
                rpc_error(rpcs, TE_ERPCTIMEOUT);
                continue;
            } 
            
            len = RCF_RPC_HUGE_BUF_LEN;
            rc = rpc_transport_recv(rpcs->handle, rpc_buf, &len, 0);
            if (rc != 0)
            {
                if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
                    continue;

                rpcs->dead = TRUE;
                rpc_error(rpcs, TE_ERPCDEAD);
                continue;
            }
            
            /* Send response */
            if (len < RCF_MAX_VAL && 
                strcmp_start("<?xml", (char *)rpc_buf) == 0)
            {
                /* Send as string */
                char *s0 = (char *)rpc_buf + RCF_MAX_VAL;
                char *s = s;
                
                s += sprintf(s, "SID %d 0 ", rpcs->last_sid);
                write_str_in_quotes(s, (char *)rpc_buf, len);
                RCF_CH_LOCK;
                rcf_comm_agent_reply(conn_saved, s0, strlen(s0) + 1);
                RCF_CH_UNLOCK;
            }
            else
            {
                /* Send as binary attachment */
                char s[64];
                
                snprintf(s, sizeof(s), "SID %d 0 attach %u",
                         rpcs->last_sid, (unsigned)len);
                RCF_CH_LOCK;
                rcf_comm_agent_reply(conn_saved, s, strlen(s) + 1);
                rcf_comm_agent_reply(conn_saved, rpc_buf, len);
                RCF_CH_UNLOCK;
            }
            
            if (rpcs->timeout == 0xFFFFFFFF) /* execve() */
            {
                rpc_transport_handle old_handle;

                rpcs->sent = 0;
                if (rpcs->tid > 0)
                {
                    /* execve() was called in a thread */
                    rpcs->tid = 0;
                    if (rpcs->father != NULL)
                    {
                        rpcs->father->ref--;
                        rpcs->father = rpcs->father->father;
                    }
                }

                old_handle = rpcs->handle;
                if (connect_getpid(rpcs) != 0)
                {
                    rpcs->dead = TRUE;
                    continue;
                }
                rpc_transport_close(old_handle);
            }
            
            rpcs->timeout = rpcs->sent = rpcs->last_sid = 0;
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}
 

/** 
 * Initialize RCF RPC server structures and link RPC configuration
 * nodes to the root.
 */
void 
rcf_pch_rpc_init(const char *tmp_path)
{
    pthread_t tid = 0;

    if (rpc_transport_init(tmp_path) != 0)
        return;

    if ((rpc_buf = malloc(RCF_RPC_HUGE_BUF_LEN)) == NULL)
    {
        rpc_transport_shutdown();
        ERROR("Cannot allocate memory for RPC buffer on the TA");
        return;
    }
        
    if (pthread_create(&tid, NULL, dispatch, NULL) != 0)
    {
        rpc_transport_shutdown();
        free(rpc_buf); 
        ERROR("Failed to create the thread for RPC servers dispatching");
        return;
    }
    
    rcf_pch_add_node("/agent", &node_rpcserver);
}

/** 
 * Close all RCF RPC connections.
 */
static void 
rcf_pch_rpc_close_connections(void)
{
    rpcserver *rpcs;

    for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
        rpc_transport_close(rpcs->handle);
}

/* See description in rcf_pch.h */
void 
rcf_pch_rpc_atfork(void)
{
    rpcserver *rpcs, *next;
    
    rcf_pch_rpc_close_connections();
        
    for (rpcs = list; rpcs != NULL; rpcs = next)
    {
        next = rpcs->next;
        free(rpcs);
    }
    list = NULL;

    free(rpc_buf);
    rpc_buf = NULL;
}

/** 
 * Cleanup RCF RPC server structures.
 */
void 
rcf_pch_rpc_shutdown(void)
{
    rpcserver *rpcs, *next;

    pthread_mutex_lock(&lock);
    rcf_pch_rpc_close_connections();
    rpc_transport_shutdown();
    usleep(100000);
    for (rpcs = list; rpcs != NULL; rpcs = next)
    {
        next = rpcs->next;
        if (rpcs->tid == 0)
            rcf_ch_kill_process(rpcs->pid);
        free(rpcs);
    }
    list = NULL;
    pthread_mutex_unlock(&lock);

    free(rpc_buf); 
    rpc_buf = NULL;
}

/**
 * Get RPC server state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param name          RPC server name
 *
 * @return Status code
 */
static te_errno
rpcserver_dead_get(unsigned int gid, const char *oid, char *value,
                   const char *name)
{
    rpcserver *rpcs;
    
    UNUSED(gid);
    UNUSED(oid);
    
    pthread_mutex_lock(&lock);
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, name) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        pthread_mutex_unlock(&lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }
        
    sprintf(value, "%d", rpcs->dead);

    pthread_mutex_unlock(&lock);
        
    return 0;        
}

/**
 * Change RPC server state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param name          RPC server name
 *
 * @return Status code
 */
static te_errno
rpcserver_dead_set(unsigned int gid, const char *oid, char *value,
                   const char *name)
{
    rpcserver *rpcs;
    te_bool    dead;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if (strcmp(value, "1") == 0)
        dead = 1;
    else if (strcmp(value, "0") == 0)
        dead = 0;
    else
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    
    pthread_mutex_lock(&lock);
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, name) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        pthread_mutex_unlock(&lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }
        
    if (rpcs->dead != dead)
    {
        if (!dead)
        {
            pthread_mutex_unlock(&lock);
            return TE_RC(TE_RCF_PCH, TE_EPERM);
        }
        rpcs->dead = TRUE;
        if (rpcs->sent > 0)
            rpc_error(rpcs, TE_RC(TE_RPC, TE_ERPCDEAD));
    }

    pthread_mutex_unlock(&lock);
        
    return 0;        
}

/**
 * Get RPC server termination state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param name          RPC server name
 *
 * @return Status code
 */
static te_errno
rpcserver_finished_get(unsigned int gid, const char *oid, char *value,
                   const char *name)
{
    rpcserver *rpcs;
    
    UNUSED(gid);
    UNUSED(oid);
    
    pthread_mutex_lock(&lock);
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, name) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        pthread_mutex_unlock(&lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }
        
    sprintf(value, "%d", rpcs->finished);

    pthread_mutex_unlock(&lock);
        
    return 0;        
}

/**
 * Change RPC server termination state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param name          RPC server name
 *
 * @return Status code
 */
static te_errno
rpcserver_finished_set(unsigned int gid, const char *oid, char *value,
                     const char *name)
{
    rpcserver *rpcs;
    te_bool    finished;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if (strcmp(value, "1") == 0)
        finished = 1;
    else if (strcmp(value, "0") == 0)
        finished = 0;
    else
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    
    pthread_mutex_lock(&lock);
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, name) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        pthread_mutex_unlock(&lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }
        
    if (rpcs->finished != finished)
    {
        if (!finished)
        {
            pthread_mutex_unlock(&lock);
            return TE_RC(TE_RCF_PCH, TE_EPERM);
        }
        rpcs->finished = TRUE;
        /** If it is finished, it is dead */
        rpcs->dead = TRUE;
    }

    pthread_mutex_unlock(&lock);
        
    return 0;        
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
static te_errno
rpcserver_get(unsigned int gid, const char *oid, char *value,
              const char *name)
{
    rpcserver *rpcs;
    
    UNUSED(gid);
    UNUSED(oid);
    
    pthread_mutex_lock(&lock);
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, name) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        pthread_mutex_unlock(&lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }
        
    strcpy(value, rpcs->value);

    pthread_mutex_unlock(&lock);
        
    return 0;        
}

              
/**
 * Set RPC server value (father name).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param name          RPC server name
 *
 * @return Status code
 */
static te_errno
rpcserver_set(unsigned int gid, const char *oid, const char *value,
              const char *name)
{
    rpcserver *rpcs;

    UNUSED(gid);
    UNUSED(oid);

    pthread_mutex_lock(&lock);
    for (rpcs = list;
         rpcs != NULL && strcmp(rpcs->name, name) != 0;
         rpcs = rpcs->next);

    if (rpcs == NULL)
    {
        pthread_mutex_unlock(&lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    strcpy(rpcs->value, value);

    pthread_mutex_unlock(&lock);

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
static te_errno
rpcserver_add(unsigned int gid, const char *oid, const char *value,
              const char *new_name)
{
    rpcserver  *rpcs;
    rpcserver  *father = NULL;
    const char *father_name = NULL;
    int         rc;
    te_bool     registration = FALSE;
    te_bool     thread = FALSE, exec = FALSE;
    char        new_val[RCF_RPC_NAME_LEN];
    
    UNUSED(gid);
    UNUSED(oid);
    
#ifdef __CYGWIN__
    {
        extern uint32_t ta_processes_num;
        ta_processes_num++;
    }
#endif
    
    if (strcmp_start("thread_", value) == 0)
    {
        father_name = value + strlen("thread_");
        thread = TRUE;
    }
    else if (strcmp_start("fork_register_", value) == 0)
    {
        father_name = value + strlen("fork_register_");
        snprintf(new_val, RCF_RPC_NAME_LEN, "fork_%s",
                 father_name);
        value = (const char *)new_val;
        registration = TRUE;
    }
    else if (strcmp_start("fork_", value) == 0)
        father_name = value + strlen("fork_");
    else if (strcmp_start("forkexec_register_", value) == 0)
    {
        father_name = value + strlen("forkexec_register_");
        snprintf(new_val, RCF_RPC_NAME_LEN, "forkexec_%s",
                 father_name);
        value = (const char *)new_val;
        registration = TRUE;
    }
    else if (strcmp_start("forkexec_", value) == 0)
    {
        father_name = value + strlen("forkexec_");
        exec = TRUE;
    }
    else if (value[0] != '\0')
    {
        ERROR("Incorrect RPC server '%s' father '%s'", new_name, value);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }
    
    pthread_mutex_lock(&lock);

    for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
    {
        if (strcmp(rpcs->name, new_name) == 0)
        {
            pthread_mutex_unlock(&lock);
            return TE_RC(TE_RCF_PCH, TE_EEXIST);
        }
        
        if (father_name != NULL && strcmp(rpcs->name, father_name) == 0)
            father = rpcs;
    }

    if (father_name != NULL && father == NULL)
    {
        pthread_mutex_unlock(&lock);
        ERROR("Cannot find father '%s' for RPC server '%s' (%s)",
              father_name, new_name, value);
        return TE_RC(TE_RCF_PCH, TE_EEXIST);
    }
    
    if (thread == TRUE)
    {
        if (father->tid != 0)
        {
            /* All the threads should be linked to the initial one" */
            father = father->father;
            snprintf(new_val, RCF_RPC_NAME_LEN, "thread_%s",
                     father->name);
            value = (const char *)new_val;
            father_name = value + strlen("thread_");
        }
    }

    if ((rpcs = (rpcserver *)calloc(1, sizeof(*rpcs))) == NULL)
    {
        pthread_mutex_unlock(&lock);
        ERROR("%s(): calloc(1, %u) failed", __FUNCTION__,
              (unsigned)sizeof(*rpcs));
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }
    
    memset(rpcs, 0, sizeof(*rpcs));
    strcpy(rpcs->name, new_name);
    strcpy(rpcs->value, value);
    rpcs->father = father;
    
    if (registration)
        goto connect;
         
    if (father == NULL)
    {
        void *argv[1];
        
        argv[0] = rpcs->name;
        
        if ((rc = rcf_ch_start_process((pid_t *)&rpcs->pid, 0,
                                       "rcf_pch_rpc_server_argv",
                                       TRUE, 1, argv)) != 0)
        {
            pthread_mutex_unlock(&lock);
            free(rpcs);
            ERROR("Failed to spawn RPC server process: error=%r", rc);
            return rc;
        }
        goto connect;
    }
    
    if (thread)
        rc = create_thread_child(rpcs);
    else
    {
        if (!exec && rpcs->father && rpcs->father->ref != 0)
        /* TODO  Also check if any CALL is running on father,
         * possibly by RCF_RPC_IS_DONE */
        {
            ERROR("Forking RPC server %s from %s which already has threads.  "
                  "Call only async-safe functions before exec!",
                  rpcs->name, rpcs->father->name);
        }
        rc = fork_child(rpcs, exec);
    }
     
    if (rc != 0)
    {
        pthread_mutex_unlock(&lock);
        free(rpcs);
        return rc;
    }
    
    connect:
    if ((rc = connect_getpid(rpcs)) != 0)
    {
        if (rpcs->tid > 0)
        {
            delete_thread_child(rpcs);
            join_thread_child(rpcs);
        }
        else if (!registration)
        {
            rcf_ch_kill_process(rpcs->pid);
            waitpid_child(rpcs);
        }
        pthread_mutex_unlock(&lock);
        free(rpcs);
        return rc;
    }
    
    if (rpcs->tid > 0)
        rpcs->father->ref++;
    else
        rpcs->father = NULL;
    
    rpcs->next = list;
    list = rpcs;

    pthread_mutex_unlock(&lock);
    
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
static te_errno
rpcserver_del(unsigned int gid, const char *oid, const char *name)
{
    rpcserver *rpcs, *prev;
    te_errno   rc = 0;
    uint8_t buf[64];
    size_t  len = sizeof(buf);
    te_bool soft_shutdown = FALSE;
    
    UNUSED(gid);
    UNUSED(oid);

    pthread_mutex_lock(&lock);
    for (rpcs = list, prev = NULL; 
         rpcs != NULL; 
         prev = rpcs, rpcs = rpcs->next)
    {
        if (strcmp(rpcs->name, name) == 0)
            break;
    }
    if (rpcs == NULL)
    {
        pthread_mutex_unlock(&lock);
        ERROR("RPC server '%s' to be deleted not found", name);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }
    
    if (rpcs->ref > 0 && !rpcs->finished)
    {
        pthread_mutex_unlock(&lock);
        ERROR("Cannot delete RPC server '%s' with threads", name);
        return TE_RC(TE_RCF_PCH, TE_EPERM);
    }
    
    if (prev != NULL)
        prev->next = rpcs->next;
    else
        list = rpcs->next;
        
    if (rpcs->father != NULL)
        rpcs->father->ref--;

    if (!rpcs->finished)
    {
        /* Try soft shutdown first */
        if (rpcs->sent > 0 || rpcs->dead || 
            rpc_transport_send(rpcs->handle, (uint8_t *)"FIN",
                               sizeof("FIN")) != 0 ||
            rpc_transport_recv(rpcs->handle, buf, &len, 5) != 0 ||
            strcmp((char *)buf, "OK") != 0 ||
            !(soft_shutdown = TRUE))
        {
            RING("Kill RPC server '%s'", rpcs->name);
            if (rpcs->tid > 0)
            {
                delete_thread_child(rpcs);
                rc = join_thread_child(rpcs);
            }
            else
            {
                rcf_ch_kill_process(rpcs->pid);
                waitpid_child(rpcs);
            }
        }
        else
        {
            if (rpcs->tid > 0)
                rc = join_thread_child(rpcs);
            else
            {
                rcf_ch_free_proc_data(rpcs->pid);
                waitpid_child(rpcs);
            }
        }
    }

    /*
     * Request to deleted RPC server should be answered
     * to unblock TA for new RCF requests processing.
     */
    if (rpcs->sent > 0)
        rpc_error(rpcs, TE_ERPCDEAD);

    if (!soft_shutdown)
    {
        if (rpcs->tid > 0)
            logfork_delete_user(rpcs->pid, rpcs->tid);
        else
            logfork_delete_user(rpcs->pid, 0);
    }

    rpc_transport_close(rpcs->handle);
    pthread_mutex_unlock(&lock);
    
    free(rpcs);
    
    return rc;
}

static te_errno
rpcserver_list(unsigned int gid, const char *oid, char **value)
{
    rpcserver *rpcs;
    char      *buf;
    unsigned   buflen = 1024;
    unsigned   filled_len = 0;
    
    UNUSED(gid);
    UNUSED(oid);
    
    buf = calloc(buflen, 1);
    if (buf == NULL)
    {
        ERROR("%s(): calloc failed", __FUNCTION__);
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }

    pthread_mutex_lock(&lock);
    *buf = 0;
    
    for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
    {
        if (buflen - filled_len <= strlen(rpcs->name) + 1)
        {
            char *buf1;

            buflen *= 2;
            buf1 = realloc(buf, buflen);
            if (buf1 == NULL)
            {
                pthread_mutex_unlock(&lock);
                free(buf);
                ERROR("%s(): realloc failed", __FUNCTION__);
                return TE_RC(TE_RCF_PCH, TE_ENOMEM);
            }
            buf = buf1;
        }
        filled_len += snprintf(buf + filled_len, buflen - filled_len, 
                               "%s ", rpcs->name);
    }
        
    if ((*value = strdup(buf)) == NULL)
    {
        pthread_mutex_unlock(&lock);
        free(buf);
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, buf);
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }
    pthread_mutex_unlock(&lock);
    free(buf);
    
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
    char       buf[32];
    int        n;
    te_errno   rc;
    
    conn_saved = conn;
    
#define RETERR(_rc) \
    do {                                                        \
        rc = TE_RC(TE_RCF_PCH, _rc);                            \
                                                                \
        n = snprintf(buf, sizeof(buf),                          \
                          "SID %d %d", sid, _rc) + 1;           \
                                                                \
        RCF_CH_LOCK;                                            \
        rc = rcf_comm_agent_reply(conn, buf, n);                \
        RCF_CH_UNLOCK;                                          \
        return rc;                                              \
    } while (0)
    
    /* Look for the RPC server */
    pthread_mutex_lock(&lock);
    for (rpcs = list; 
         rpcs != NULL && strcmp(rpcs->name, server) != 0;
         rpcs = rpcs->next);
         
    if (rpcs == NULL)
    {
        ERROR("Failed to find RPC server %s", server);
        pthread_mutex_unlock(&lock);
        RETERR(TE_ENOENT);
    }
    
    if (rpcs->dead)
    {
        ERROR("Request to dead RPC server %s", server);
        pthread_mutex_unlock(&lock);
        RETERR(TE_ERPCDEAD);
    }
    
    /* Mark RPC server as busy */
    if (rpcs->sent != 0)
    {
        ERROR("RPC server %s is busy", server);
        pthread_mutex_unlock(&lock);
        RETERR(TE_EBUSY);
    }
    rpcs->sent = time(NULL);
    rpcs->last_sid = sid;
    rpcs->timeout = timeout == 0xFFFFFFFF ? timeout : timeout / 1000;
    pthread_mutex_unlock(&lock);

    /* Sent ACK to RCF and pass handling to the thread */                                              
    n = snprintf(buf, sizeof(buf), "SID %d %d", sid, 
                 TE_RC(TE_RCF_PCH, TE_EACK)) + 1;
                                                                
    RCF_CH_LOCK;                                     
    rc = rcf_comm_agent_reply(conn, buf, n);
    RCF_CH_UNLOCK; 
    
    if (rc != 0)
        return rc;
    
    /* Send encoded data to server */
    if (rpc_transport_send(rpcs->handle, (uint8_t *)data, 
                           rpc_data_len) != 0)
    {
        ERROR("Failed to send RPC data to the server %s", rpcs->name);
        RETERR(TE_ESUNRPC);
    }
    
    /* The final answer will be sent by the thread */
    return 0;

#undef RETERR    
}            

#include "rcf_pch_rpc_server.c"

/**
 * Wrapper to call rcf_pch_rpc_server via "ta exec func" mechanism. 
 *
 * @param argc    should be 1
 * @param argv    should contain pointer to RPC server name
 */
void
rcf_pch_rpc_server_argv(int argc, char **argv)
{
    UNUSED(argc);

    rcf_pch_rpc_server(argv[0]);
}
