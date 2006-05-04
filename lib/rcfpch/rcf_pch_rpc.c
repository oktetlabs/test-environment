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
    uint32_t  tid;         /**< Thread identifier or 0 */
    
    uint32_t  timeout;     /**< Timeout for the last sent request */
    int       last_sid;    /**< SID received with the last command */
    te_bool   dead;        /**< RPC server does not respond */
    te_bool   local;       /**< RPC server is TA thread */
    time_t    sent;        /**< Time of the last request sending */
} rpcserver;

static rpcserver *list;        /**< List of all RPC servers */
static uint8_t   *rpc_buf;     /**< Buffer for receiving of RPC answers;
                                    may be used in dispatch thread
                                    context only */

static te_errno rpcserver_get(unsigned int, const char *, char *,
                              const char *);
static te_errno rpcserver_add(unsigned int, const char *, const char *,
                              const char *);
static te_errno rpcserver_del(unsigned int, const char *,
                              const char *);
static te_errno rpcserver_list(unsigned int, const char *, char **);

static rcf_pch_cfg_object node_rpcserver =
    { "rpcserver", 0, NULL, NULL,
      (rcf_ch_cfg_get)rpcserver_get, NULL,
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
    char    c = '\0';

    ((tarpc_in_arg *)in)->lib = &c;
    
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
 * Create child RPC server.
 *
 * @param rpcs    RPC server handle
 *
 * @return Status code
 */
static te_errno
fork_child(rpcserver *rpcs)
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
    in.inherit = TRUE;
    in.net_init = TRUE;
    
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
    rcf_ch_lock();
    rcf_comm_agent_reply(conn_saved, error_buf, n);
    rcf_ch_unlock();
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
        
        rpc_transport_read_set_init();
        
        pthread_mutex_lock(&lock);
        for (rpcs = list; rpcs != NULL && !rpcs->dead; rpcs = rpcs->next)
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
               
            if (rpcs->sent > 0 && 
                ((uint32_t)(now - rpcs->sent) > rpcs->timeout ||
                 (rpcs->timeout == 0xFFFFFFFF && now - rpcs->sent > 5)))
            {
                ERROR("Timeout on server %s", rpcs->name);
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
                rcf_ch_lock();
                rcf_comm_agent_reply(conn_saved, s0, strlen(s0) + 1);
                rcf_ch_unlock();
            }
            else
            {
                /* Send as binary attachment */
                char s[64];
                
                snprintf(s, sizeof(s), "SID %d 0 attach %u",
                         rpcs->last_sid, (unsigned)len);
                rcf_ch_lock();
                rcf_comm_agent_reply(conn_saved, s, strlen(s) + 1);
                rcf_comm_agent_reply(conn_saved, rpc_buf, len);
                rcf_ch_unlock();
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
        pthread_mutex_unlock(&lock);
    }
}
 

/** 
 * Initialize RCF RPC server structures and link RPC configuration
 * nodes to the root.
 */
void 
rcf_pch_rpc_init()
{
    pthread_t tid = 0;

    if (rpc_transport_init() != 0)
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
    te_bool     existing = FALSE, local = FALSE, thread = FALSE;
    
    UNUSED(gid);
    UNUSED(oid);
    
#ifdef __CYGWIN__
    {
        extern uint32_t ta_processes_num;
        ta_processes_num++;
    }
#endif    
    
    if (strcmp("thread_local", value) == 0)
    {
        local = TRUE;
    }
    else if (strcmp_start("thread_existing_", value) == 0)
    {
        existing = TRUE;
        father_name = value + strlen("thread_existing_");
    }
    else if (strcmp_start("thread_", value) == 0)
    {
        father_name = value + strlen("thread_");
        thread = TRUE;
    }
    else if (strcmp("fork_existing", value) == 0)
    {
        existing = TRUE;
    }
    else if (strcmp_start("fork_", value) == 0)
    {
        father_name = value + strlen("fork_");
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
    
    if ((rpcs = (rpcserver *)calloc(1, sizeof(*rpcs))) == NULL)
    {
        pthread_mutex_unlock(&lock);
        ERROR("%s(): calloc(1, %u) failed", __FUNCTION__,
              (unsigned)sizeof(*rpcs));
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }
    
    strcpy(rpcs->name, new_name);
    strcpy(rpcs->value, value);
    rpcs->father = father;
    rpcs->local = local;
    
    if (existing)
        goto connect;
         
    if (father == NULL && !local)
    {
        char *argv[1];
        
        argv[0] = rpcs->name;
        
        if ((rc = rcf_ch_start_process((pid_t *)&rpcs->pid, 0, 
                                       "rcf_pch_rpc_server_argv",
                                       TRUE, 1, (void **)argv)) != 0)
        {
            pthread_mutex_unlock(&lock);
            free(rpcs);
            ERROR("Failed to spawn RPC server process: error=%r", rc);
            return rc;
        }
        goto connect;
    }
    
    if (local)
    {
        char *argv[1];
        
        argv[0] = rpcs->name;
        
        if ((rc = rcf_ch_start_thread((int *)&rpcs->tid, 0, 
                                      "rcf_ch_rpc_server_thread",
                                      TRUE, 1, (void **)argv)) != 0)
        {
            pthread_mutex_unlock(&lock);
            free(rpcs);
            ERROR("Failed to spawn RPC server thread: error=%r", rc);
            return rc;
        }

        goto connect;
    }
    
    if (thread)
        rc = create_thread_child(rpcs);
    else
        rc = fork_child(rpcs);
            
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
            if (rpcs->local)
                rcf_ch_kill_thread(rpcs->tid);
            else
                delete_thread_child(rpcs);
        }
        else if (!existing)
        {
            rcf_ch_kill_process(rpcs->pid);
        }
        pthread_mutex_unlock(&lock);
        free(rpcs);
        return rc;
    }
    
    if (rpcs->tid > 0)
        rpcs->father->ref++;
    
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
    
    uint8_t buf[64];
    size_t  len = sizeof(buf);
    
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
    
    if (rpcs->ref > 0)
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

    /* Try soft shutdown first */
    if (rpcs->sent > 0 || 
        rpc_transport_send(rpcs->handle, (uint8_t *)"FIN", 
                           sizeof("FIN")) != 0 ||
        rpc_transport_recv(rpcs->handle, buf, &len, 5) != 0 ||
        strcmp((char *)buf, "OK") != 0)
    {
        WARN("Soft shutdown of RPC server '%s' failed", rpcs->name);
        if (rpcs->tid > 0)
        {
            if (rpcs->local)
                rcf_ch_kill_thread(rpcs->tid);
            else
                delete_thread_child(rpcs);
        }
        else
            rcf_ch_kill_process(rpcs->pid);
    }
    rpc_transport_close(rpcs->handle);

    pthread_mutex_unlock(&lock);
    
    free(rpcs);
    
    return 0;
}

static te_errno
rpcserver_list(unsigned int gid, const char *oid, char **value)
{
    rpcserver *rpcs;
    char       buf[1024];
    char      *s = buf;
    
    UNUSED(gid);
    UNUSED(oid);
    
    pthread_mutex_lock(&lock);
    *buf = 0;
    
    for (rpcs = list; rpcs != NULL; rpcs = rpcs->next)
        s += sprintf(s, "%s ", rpcs->name);
        
    if ((*value = strdup(buf)) == NULL)
    {
        pthread_mutex_unlock(&lock);
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, buf);
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }
    pthread_mutex_unlock(&lock);
    
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
        rcf_ch_lock();                                          \
        rc = rcf_comm_agent_reply(conn, buf, n);                \
        rcf_ch_unlock();                                        \
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
                                                                
    rcf_ch_lock();                                     
    rc = rcf_comm_agent_reply(conn, buf, n);
    rcf_ch_unlock(); 
    
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
