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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_RPC_H__
#define __TE_RCF_RPC_H__

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "rcf_common.h"
#include "rcf_rpc_defs.h"
#include "conf_api.h"

#include "tarpc.h"


/**
 * @defgroup te_lib_rcfrpc API: RCF RPC
 * @ingroup te_lib_rpc
 * @{
 */

/** Unspecified RPC timeout in milliseconds */
#define RCF_RPC_UNSPEC_TIMEOUT      0


/** Return RPC server context errno */
#define RPC_ERRNO(_rpcs) \
    (((_rpcs) != NULL) ? ((_rpcs)->_errno) : TE_RC(TE_RCF_API, TE_EINVAL))

/** Check, if RPC is called successfully */
#define RPC_IS_CALL_OK(_rpcs) \
    (((_rpcs) != NULL) && RPC_IS_ERRNO_RPC((_rpcs)->_errno))

/** Do not jump from the TAPI RPC library call in the case of IUT error */
#define RPC_AWAIT_IUT_ERROR(_rpcs) \
    (_rpcs)->iut_err_jump = FALSE

/** Roll back RPC_AWAIT_IUT_ERROR effect */
#define RPC_DONT_AWAIT_IUT_ERROR(_rpcs) \
    (_rpcs)->iut_err_jump = TRUE

/** Are we awaiting an error? */
#define RPC_AWAITING_ERROR(_rpcs)       (!(_rpcs)->iut_err_jump)

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


/** RPC server context */
typedef struct rcf_rpc_server {
    /* Configuration parameters */
    rcf_rpc_op  op;             /**< Instruction for RPC call */
    rcf_rpc_op  last_op;        /**< op value in the last call */
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

    te_bool errno_change_check; /**< Check errno changes in the case
                                     of success */

    char       *nv_lib;         /**< Library name set for the server */

    te_bool     use_libc;       /**< Use libc library instead of set one */
    te_bool     use_libc_once;  /**< Same as use_libc, but one call only */
    te_bool     last_use_libc;  /**< Last value of use_libc_once */

    /* Read-only fields filled by API internals when server is created */
    char        ta[RCF_MAX_NAME];   /**< Test Agent name */
    char        name[RCF_MAX_NAME]; /**< RPC server name */
    int         sid;                /**< RCF session identifier */

    /* Returned read-only fields with status of the last operation */
    uint64_t        duration;   /**< Call Duration in microseconds */
    int             _errno;     /**< error number */
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_t lock;       /**< lock mutex */
#endif    
    tarpc_pthread_t tid0;       /**< Identifier of thread performing
                                     non-blocking operations */
    char            proc[RCF_MAX_NAME];
                                /**< Last called function */
    uint32_t        is_done_ptr;
                                /**< Pointer to the variable in
                                     RPC server context to check
                                     state of non-blocking RPC */
} rcf_rpc_server;


/** Default RPC timeout in milliseconds */
#define RCF_RPC_DEFAULT_TIMEOUT     rcp_rpc_default_timeout()

/**
 * Function to get default RPC timeout in milliseconds from Environment.
 *
 * @return Timeout in milliseconds.
 */
static inline unsigned int
rcp_rpc_default_timeout(void)
{
    const char         *var_name = "TE_RCFRPC_TIMEO_MS";
    const unsigned int  def_val = 10000;
    const char         *value;
    const char         *end;
    unsigned long       timeo;

    value = getenv(var_name);
    if (value == NULL || *value == '\0')
        return def_val; 

    timeo = strtoul(value, (char **)&end, 10);
    if (*end != '\0' || timeo >= UINT_MAX)
    {
        ERROR("Invalid value '%s' in Environment variable '%s'",
              value, var_name);
        return def_val;
    }

    return timeo;
}

/**
 * Obtain server handle. RPC server is created/restarted, if necessary.
 *
 * @param ta            Test Agent name
 * @param name          name of the new server (should not start from
 *                      fork_, forkexec_ or thread_) or existing server
 * @param father        father name or NULL (should be NULL if
 *                      RCF_RPC_SERVER_GET_REUSE or
 *                      RCF_RPC_SERVER_GET_EXISTING flag is set).
 * @param flags         RCF_RPC_SERVER_GET_* flags
 * @param p_handle      location for new RPC server handle or NULL
 *
 * @return Status code
 */
extern te_errno rcf_rpc_server_get(const char *ta, const char *name,
                                   const char *father, int flags,
                                   rcf_rpc_server **p_handle);

/**
 * Create RPC server.
 *
 * @param ta            Test Agent name
 * @param name          name of the new server
 * @param p_handle      location for new RPC server handle
 *
 * @return Status code
 */
static inline te_errno 
rcf_rpc_server_create(const char *ta, const char *name, 
                      rcf_rpc_server **p_handle)
{
    return rcf_rpc_server_get(ta, name, NULL, 0, p_handle);
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
static inline te_errno 
rcf_rpc_server_thread_create(rcf_rpc_server *rpcs, const char *name,
                             rcf_rpc_server **p_new)
{
    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    return rcf_rpc_server_get(rpcs->ta, name, rpcs->name, 
                              RCF_RPC_SERVER_GET_THREAD, p_new);
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
static inline te_errno 
rcf_rpc_server_fork(rcf_rpc_server *rpcs, const char *name,
                    rcf_rpc_server **p_new)
{
    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    return rcf_rpc_server_get(rpcs->ta, name, rpcs->name, 0, p_new);
}          

/**
 * Fork-and-exec RPC server.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return Status code
 */
static inline te_errno 
rcf_rpc_server_fork_exec(rcf_rpc_server *rpcs, const char *name,
                         rcf_rpc_server **p_new)
{
    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    return rcf_rpc_server_get(rpcs->ta, name, rpcs->name,
                              RCF_RPC_SERVER_GET_EXEC, p_new);
}          

/**
 * Fork RPC server with non-default conditions.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param flags         RCF_RPC_SERVER_GET_* flags to use
 *                      on process creation
 * @param p_new         location for new RPC server handle
 *
 * @return Status code
 */
extern te_errno rcf_rpc_server_create_process(rcf_rpc_server *rpcs, 
                                              const char *name,
                                              int flags,
                                              rcf_rpc_server **p_new);

/**
 * Perform execve() on the RPC server. Filename of the running process
 * if used as the first argument.
 *
 * @param rpcs          RPC server handle
 *
 * @return Status code
 */
extern te_errno rcf_rpc_server_exec(rcf_rpc_server *rpcs);

/**
 * Set dynamic library name to be used for additional name resolution.
 *
 * @param rpcs          existing RPC server handle
 * @param libname       name of the dynamic library or NULL
 *
 * @return Status code
 */
extern te_errno rcf_rpc_setlibname(rcf_rpc_server *rpcs, 
                                   const char *libname);

/**
 * Restart RPC server.
 *
 * @param rpcs          RPC server handle
 *
 * @return Status code
 */
static inline te_errno 
rcf_rpc_server_restart(rcf_rpc_server *rpcs)
{
    rcf_rpc_server *new_rpcs;
    int             rc;
    
    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    rc = rcf_rpc_server_get(rpcs->ta, rpcs->name, NULL, 0, &new_rpcs);
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
 * Restart all RPC servers.
 *
 * @attention This function has a lot of limitations. It does not
 *            work when fork child and thread RPC servers exist.
 *            It does not preserve name of the dynamic library to
 *            be used for functions name to pointer resolution.
 *
 * @return Status code.
 */
extern te_errno rcf_rpc_servers_restart_all(void);

/**
 * Destroy RPC server. The caller should assume the RPC server non-existent 
 * even if the function returned non-zero.
 *
 * @param rpcs          RPC server handle
 *
 * @return status code
 */
extern te_errno rcf_rpc_server_destroy(rcf_rpc_server *rpcs);

/**
 * Mark RPC server as dead to force its killing by rcf_rpc_server_destroy().
 *
 * @param rpcs          RPC server handle
 *
 * @return Status code
 */
static inline te_errno
rcf_rpc_server_dead(rcf_rpc_server *rpcs)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 1), 
                                "/agent:%s/rpcserver:%s/dead:", 
                                rpcs->ta, rpcs->name);
} 

/**
 * Mark RPC server as finished (i.e. it was terminated and
 * waitpid() or pthread_join() was already called - so only
 * associated data structures remained to be freed on TA).
 *
 * @param rpcs          RPC server handle
 *
 * @return Status code
 */
static inline te_errno
rcf_rpc_server_finished(rcf_rpc_server *rpcs)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 1), 
                                "/agent:%s/rpcserver:%s/finished:", 
                                rpcs->ta, rpcs->name);
}

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
extern te_errno rcf_rpc_server_is_op_done(rcf_rpc_server *rpcs,
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
 * @param op      RCF RPC operation
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

/**
 * Check is the RPC server has thread children.
 *
 * @param rpcs          RPC server
 *
 * @return TRUE if RPC server has children
 */
extern te_bool rcf_rpc_server_has_children(rcf_rpc_server *rpcs);


/**
 * Get RPC server handle, restart it if necessary.
 *
 * @param _ta     name of Test Agent
 * @param _name   name of RCF RPC server
 * @param _rpcs   variable for RPC server handle
 */
#define TEST_GET_RPCS(_ta, _name, _rpcs) \
    do {                                                               \
        te_errno    rc = 0;                                            \
        int         is_dead = 0;                                       \
                                                                       \
        if ((rc = rcf_rpc_server_get(_ta, _name, NULL,                 \
                                     RCF_RPC_SERVER_GET_REUSE,         \
                                     &(_rpcs))) != 0)                  \
            TEST_FAIL("Failed to get RPC server handle: %r", rc);      \
                                                                       \
        /* Restart RPC server if it is dead */                         \
        if ((rc = cfg_get_instance_fmt(NULL, &is_dead,                 \
                                       "/agent:%s/rpcserver:%s/dead:", \
                                       _ta, _name)) != 0)              \
            TEST_FAIL("Failed to get 'dead' status of RPC "            \
                      "server %s: %r", _name, rc);                     \
                                                                       \
        if (is_dead == 1)                                              \
        {                                                              \
            WARN("RPC server %s is dead, try to restart", _name);      \
                                                                       \
            if ((rc = rcf_rpc_server_restart(_rpcs)) != 0)             \
                TEST_FAIL("Failed to restart dead RPC server %s: %r",  \
                          _name, rc);                                  \
        }                                                              \
    } while (0)

/**@} <!-- END te_lib_rcfrpc --> */

#endif /* !__TE_RCF_RPC_H__ */
