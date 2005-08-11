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

#include "te_defs.h"
#include "rcf_common.h"
#include "rcf_rpc_defs.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "tarpc.h"

/** Default RPC timeout in milliseconds */
#define RCF_RPC_DEFAULT_TIMEOUT     100000


/** Return RPC server context errno */
#define RPC_ERRNO(_rpcs) \
    (((_rpcs) != NULL) ? ((_rpcs)->_errno) : TE_RC(TE_RCF_API, TE_EINVAL))

/** Check, if RPC is called successfully */
#define RPC_IS_CALL_OK(_rpcs) \
    (((_rpcs) != NULL) && \
     (((_rpcs)->_errno == 0) || RPC_IS_ERRNO_RPC((_rpcs)->_errno)))

/** Do not jump from the TAPI RPC library call in the case of IUT error */
#define RPC_AWAIT_IUT_ERROR(_rpcs) \
    (_rpcs)->iut_err_jump = FALSE

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
    te_bool     iut_err_jump;   /**< Jump in the case of IUT error
                                     (true by default) */
    te_bool     err_log;        /**< Log error with ERROR log level */
    te_bool     timed_out;      /**< Timeout was received from this
                                     RPC server - it is unusable
                                     unless someone has restarted it */

    char   *nv_lib;             /**< Library name set for the server */

    char    lib[RCF_MAX_PATH];  /**< Library name to be used for the call
                                     (is set to empty line after each
                                     call) */

    /* Read-only fields filled by API internals when server is created */
    char        ta[RCF_MAX_NAME];   /**< Test Agent name */
    char        name[RCF_MAX_NAME]; /**< RPC server name */
    int         sid;                /**< RCF session identifier */

    /* Returned read-only fields with status of the last operation */
    uint64_t        duration;   /**< Call Duration in microseconds */
    int             _errno;     /**< error number */
    int             win_error;  /**< Value returned by GetLastError() */
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_t lock;       /**< lock mutex */
#endif    
    int             tid0;       /**< Identifier of thread performing
                                     non-blocking operations */
    char            proc[RCF_MAX_NAME];
                                /**< Last called function */
    uint32_t        is_done_ptr;
                                /**< Pointer to the variable in
                                     RPC server context to check
                                     state of non-blocking RPC */
} rcf_rpc_server;

/**
 * Obtain server handle. RPC server is created/restarted, if necessary.
 *
 * @param ta            a test agent
 * @param name          name of the new server (should not start from
 *                      fork_ or thread_)
 * @param father        father name or NULL (should be NULL if clear is 
 *                      FALSE or existing is TRUE)
 * @param thread        if TRUE, the thread should be created instead 
 *                      process
 * @param existing      get only existing RPC server
 * @param clear         get newly created or restarted RPC server
 * @param p_handle      location for new RPC server handle
 *
 * @return Status code
 */
extern int rcf_rpc_server_get(const char *ta, const char *name,
                              const char *father, te_bool thread, 
                              te_bool existing, te_bool clear, 
                              rcf_rpc_server **p_new);

/**
 * Create RPC server.
 *
 * @param ta            a test agent
 * @param name          name of the new server
 * @param p_handle      location for new RPC server handle
 *
 * @return Status code
 */
static inline int 
rcf_rpc_server_create(const char *ta, const char *name, 
                      rcf_rpc_server **p_handle)
{
    return rcf_rpc_server_get(ta, name, NULL, FALSE, FALSE, TRUE, p_handle);
}                      

/**
 * Create thread in the process with RPC server.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return Status code
 */
static inline int 
rcf_rpc_server_thread_create(rcf_rpc_server *rpcs, const char *name,
                             rcf_rpc_server **p_new)
{
    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    return rcf_rpc_server_get(rpcs->ta, name, rpcs->name, 
                              TRUE, FALSE, TRUE, p_new);
}                             

/**
 * Fork RPC server.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return Status code
 */
static inline int 
rcf_rpc_server_fork(rcf_rpc_server *rpcs, const char *name,
                    rcf_rpc_server **p_new)
{
    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    return rcf_rpc_server_get(rpcs->ta, name, rpcs->name, 
                              FALSE, FALSE, TRUE, p_new);
}                    

/**
 * Perform execve() on the RPC server. Filename of the running process
 * if used as the first argument.
 *
 * @param rpcs          RPC server handle
 *
 * @return Status code
 */
extern int rcf_rpc_server_exec(rcf_rpc_server *rpcs);

/**
 * Set dynamic library name to be used for additional name resolution.
 *
 * @param rpcs          existing RPC server handle
 * @param libname       name of the dynamic library or NULL
 *
 * @return Status code
 */
extern int rcf_rpc_setlibname(rcf_rpc_server *rpcs, const char *libname);

/**
 * Restart RPC server.
 *
 * @param rpcs          RPC server handle
 *
 * @return Status code
 */
static inline int 
rcf_rpc_server_restart(rcf_rpc_server *rpcs)
{
    rcf_rpc_server *new_rpcs;
    int             rc;
    
    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    rc = rcf_rpc_server_get(rpcs->ta, rpcs->name, NULL, FALSE, FALSE,
                            TRUE, &new_rpcs);
    if (rc == 0)
    {
        char *lib = rpcs->nv_lib;

        *rpcs = *new_rpcs;
        free(new_rpcs);

        if (lib != NULL)
        {
            rcf_rpc_setlibname(rpcs, lib);
            free(lib);
        }
    }
     
    return rc;       
}

/**
 * Destroy RPC server.
 *
 * @param rpcs          RPC server handle
 *
 * @return Status code
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
 * @param out_arg       output argument
 *
 * @attention The Status code is returned in rpcs _errno.
 *            If rpcs is NULL the function does nothing.
 */
extern void rcf_rpc_call(rcf_rpc_server *rpcs, const char *proc,
                         void *in_arg, void *out_arg);

/**
 * Check whether a function called using non-blocking RPC has been done.
 *
 * @param rpcs          existing RPC server handle
 * @param done          location for the result
 *
 * @return Status code
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
