/** @file
 * @brief SUN RPC control interface
 *
 * Definition of functions to create/destroy RPC servers on Test Agents
 * and to set/get RPC server context parameters.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_RPC_H__
#define __TE_RCF_RPC_H__

#undef MAX
#undef MIN
#include <rpc/rpc.h>
#include <rpc/clnt.h>
#include <semaphore.h>

#include "te_defs.h"
#include "rcf_common.h"
#include "rcf_rpc_defs.h"


/** Default RPC timeout in milliseconds */
#define RCF_RPC_DEFAULT_TIMEOUT     100000


/** Return RPC server context errno */
#define RPC_ERRNO(_rpcs) \
    (((_rpcs) != NULL) ? ((_rpcs)->_errno) : TE_RC(TE_RCF_API, EINVAL))

/** Check, if RPC is called successfully */
#define RPC_IS_CALL_OK(_rpcs) \
    (((_rpcs) != NULL) && \
     (((_rpcs)->_errno == 0) || RPC_ERRNO_RPC((_rpcs)->_errno)))

/**
 * Extract name of the PCO by RCP server handle
 *
 * @param rpcs_ - RPC server handle
 *
 * @return Name of the PCO
 */
#define RPC_NAME(rpcs_) \
    ((((rpcs_) != NULL) && ((rpcs_)->name != NULL)) ? \
     (rpcs_)->name : "<UNKNOWN>")


/** Forward declarations of RPC server context */
typedef struct rcf_rpc_server {
    /* Configuration paramters */
    rcf_rpc_op  op;             /**< Instruction for RPC call */
    uint64_t    start;          /**< Time when RPC should be called on 
                                     the server (in milliseconds since
                                     Epoch; 0 if it should be called
                                     immediately) */

    uint32_t    def_timeout;    /**< Default RPC call timeout in 
                                     milliseconds */
    uint32_t    timeout;        /**< Next RPC call timeout in milliseconds
                                     (after call it's automatically reset
                                     to def_timeout) */

    /* Read-only fields filled by API internals when server is created */
    char            ta[RCF_MAX_NAME];   /**< Test Agent name */
    char            name[RCF_MAX_NAME]; /**< RPC server name */
    int             pid;                /**< RPC server process identifier
                                             (for fork()-ed server) */
    int             tid;                /**< RPC server thread identifier
                                             (for thread) */

    /* Returned read-only fields with status of the last operation */
    uint64_t        duration;   /**< Call Duration in microseconds */
    enum clnt_stat  stat;
    int             _errno;
    int             win_error;  /**< Value returned by GetLastError() */
    
    pthread_mutex_t lock;
    int             tid0;       /**< Identifier of thread performing
                                     non-blocking operations */
    uint32_t        is_done_ptr;
                                /**< Pointer to the variable in
                                     RPC server context to check
                                     state of non-blocking RPC */
    te_bool         dead;       /**< The server is dead and could not
                                     process RPCs */
    sem_t           fw_sem;     /**< Semaphore to be used to synchronize
                                     with forwarding thread */ 
    struct rcf_rpc_server *father;     
                                /**< Father of the server created using
                                     pthread_create() */
    unsigned int    children;   /**< Number of children */
} rcf_rpc_server;


/**
 * Create RPC server.
 *
 * @param ta            a test agent
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return status code
 */
extern int rcf_rpc_server_create(const char *ta, const char *name, 
                                 rcf_rpc_server **p_handle);

/**
 * Create thread in the process with RPC server.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return status code
 */
extern int rcf_rpc_server_thread_create(rcf_rpc_server *rpcs,
                                        const char *name,
                                        rcf_rpc_server **p_new);

/**
 * Fork RPC server.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return status code
 */
extern int rcf_rpc_server_fork(rcf_rpc_server *rpcs, const char *name,
                               rcf_rpc_server **p_new);

/**
 * Perform execve() on the RPC server. Filename of the running process
 * if used as the first argument.
 *
 * @param rpcs          RPC server handle
 *
 * @return status code
 */
extern int rcf_rpc_server_exec(rcf_rpc_server *rpcs);

/**
 * Destroy RPC server.
 *
 * @param rpcs          RPC server handle
 *
 * @return status code
 */
extern int rcf_rpc_server_destroy(rcf_rpc_server *rpcs);


/**
 * Call SUN RPC on the TA via RCF. The function is also used for
 * checking of status of non-blocking RPC call and waiting for
 * the finish of the non-blocking RPC call.
 *
 * @param rpcs          RPC server
 * @param proc          RPC to be called
 * @param in_arg        input argument
 * @param in_proc       function for handling of the input argument
 *                      (generated)
 * @param out_arg       output argument
 * @param out_proc      function for handling of output argument
 *                      (generated)
 *
 * @attention The status code is returned in rpcs _errno.
 *            If rpcs is NULL the function does nothing.
 */
extern void rcf_rpc_call(rcf_rpc_server *rpcs, int proc,
                         void *in_arg, xdrproc_t in_proc, 
                         void *out_arg, xdrproc_t out_proc);

/**
 * Check whether a function called using non-blocking RPC has been done.
 *
 * @param rpcs          existing RPC server handle
 * @param done          location for the result
 *
 * @return status code
 */
extern int rcf_rpc_server_is_op_done(rcf_rpc_server *rpcs,
                                     te_bool *done);

/** Free memory allocated by rcf_rpc_call */
static inline void
rcf_rpc_free_result(void *out_arg, xdrproc_t out_proc)
{
    enum xdr_op op = XDR_FREE;

    out_proc((XDR *)&op, out_arg);
}

/**
 * Convert RCF RPC operation to string.
 *
 * @param op    - RCF RPC operation
 *
 * @return null-terminated string
 */
static inline const char *
rpcop2str(rcf_rpc_op op)
{
    switch (op)
    {
        case RCF_RPC_CALL:      return " call";
        case RCF_RPC_IS_DONE:   return " is done";
        case RCF_RPC_WAIT:      return " wait";
        case RCF_RPC_CALL_WAIT: return "";
        default:                assert(FALSE);
    }
    return " (unknown)";
}

#endif /* !__TE_RCF_RPC_H__ */
