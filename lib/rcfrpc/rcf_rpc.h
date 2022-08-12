/** @file
 * @brief SUN RPC control interface
 *
 * Definition of functions to create/destroy RPC servers on Test Agents
 * and to set/get RPC server context parameters.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
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
#include "te_rpc_types.h"
#include "conf_api.h"
#include "te_queue.h"

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
    do {                                \
        (_rpcs)->iut_err_jump = TRUE;   \
        /* It does not make sense to    \
         * await only non-IUT error */  \
        (_rpcs)->err_jump = TRUE;       \
    } while (0)

/** Do not jump from the TAPI RPC library call in the case of any error */
#define RPC_AWAIT_ERROR(_rpcs) \
    do {                                \
        (_rpcs)->iut_err_jump = FALSE;  \
        (_rpcs)->err_jump = FALSE;      \
    } while (0)

/** Roll back RPC_AWAIT_ERROR effect */
#define RPC_DONT_AWAIT_ERROR(_rpcs) \
    RPC_DONT_AWAIT_IUT_ERROR(_rpcs)

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

/**
 * Get RPC error message.
 *
 * @param _rpcs   Pointer to rcf_rpc_server structure.
 */
#define RPC_ERROR_MSG(_rpcs) ((_rpcs) != NULL ? (_rpcs)->err_msg : "")

/** Format string for logging RPC server error (both number and message) */
#define RPC_ERROR_FMT "%r%s%s%s"

/**
 * Arguments for RPC_ERROR_FMT.
 *
 * @param _rpcs     Pointer to rcf_rpc_server structure.
 */
#define RPC_ERROR_ARGS(_rpcs) \
    RPC_ERRNO(_rpcs),                                         \
    (RPC_ERROR_MSG(_rpcs)[0] != '\0' ?                        \
                                " (error message '" : ""),    \
    RPC_ERROR_MSG(_rpcs),                                     \
    (RPC_ERROR_MSG(_rpcs)[0] != '\0' ? "')" : "")

/** RPC server context */
typedef struct rcf_rpc_server {
    /* Configuration parameters */
    rcf_rpc_op  op;             /**< Instruction for RPC call */
    rcf_rpc_op  last_op;        /**< op value in the last call */
    uint64_t    start;          /**< Time when RPC should be called on
                                     the server (in milliseconds since
                                     Epoch; 0 if it should be called
                                     immediately) */
    uint16_t    seqno;          /**< Sequence number of the next RPC call.
                                     Only incremented by completed calls.
                                     The number should not be taken as a
                                     globally unique id, it may wrap up
                                     relatively quickly */

    uint32_t    def_timeout;    /**< Default RPC call timeout in
                                     milliseconds */
    uint32_t    timeout;        /**< Next RPC call timeout in milliseconds
                                     (after call it's automatically reset
                                     to def_timeout) */

    /*
     * TODO: err_jump and iut_err_jump should be replaced with a single
     * enum variable having three values: DONT_AWAIT_ERRORS,
     * AWAIT_IUT_ERRORS, AWAIT_ANY_ERRORS. Legacy code exists which
     * accesses iut_err_jump directly instead of using macros,
     * it should be fixed then.
     */
    te_bool     err_jump;       /**< Jump if RPC call failed (this may occur
                                     if the function called via this RPC
                                     returned error, or for other reasons
                                     such as segfault or timeout; true by
                                     default) */
    te_bool     iut_err_jump;   /**< Jump if RPC call failed because
                                     function called via this RPC returned
                                     error (true by default; overrides
                                     err_jump) */

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
    te_bool     use_syscall;    /**< Try to use syscall with library according
                                     to flag use_libc */

    /* Read-only fields filled by API internals when server is created */
    char        ta[RCF_MAX_NAME];   /**< Test Agent name */
    char        name[RCF_MAX_NAME]; /**< RPC server name */
    int         sid;                /**< RCF session identifier */

    /* Returned read-only fields with status of the last operation */
    uint64_t        duration;   /**< Call Duration in microseconds */
    int             _errno;     /**< error number */
    char            err_msg[RPC_ERROR_MAX_LEN]; /**< Optional error
                                                     message. */

#ifdef HAVE_PTHREAD_H
    pthread_mutex_t lock;       /**< lock mutex */
#endif
    uint64_t        jobid0;       /**< Identifier of a deferred operation */
    char            proc[RCF_MAX_NAME];
                                /**< Last called function */
    te_bool         silent;     /**< Perform next RPC call without
                                     logging */
    te_bool         silent_default; /**< Turn on/off RPC calls logging, can
                                         be used to change the behavior for
                                         a few calls. */
    te_bool         silent_pass;        /**< The same as @ref silent, but
                                             error log still will be logged. */
    te_bool         silent_pass_default;/**< The same as @ref silent_default,
                                             applicable for @ref silent_pass.
                                             */

    char          **namespaces;     /**< Array of namespaces for memory
                                     * pointers (rpc_ptr). */
    size_t          namespaces_len; /**< Amount of elements in @p namespaces */
} rcf_rpc_server;


/** Default RPC timeout in milliseconds */
#define RCF_RPC_DEFAULT_TIMEOUT     10000

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
 * Call vfork() in RPC server.
 *
 * @param rpcs          Existing RPC server handle
 * @param name          Name of the new server
 * @param time_to_wait  How much time to wait after
 *                      vfork() call
 * @param pid           If not NULL, will be set to
 *                      result of vfork() call
 * @param elapsed_time  Time elapsed until vfork() returned
 *
 * @return Status code
 */
extern te_errno rcf_rpc_server_vfork(rcf_rpc_server *rpcs,
                                     const char *name,
                                     uint32_t time_to_wait,
                                     pid_t *pid,
                                     uint32_t *elapsed_time);

/**
 * Input/output parameters for rcf_rpc_server_vfork() called
 * in a thread.
 */
typedef struct vfork_thread_data {
    rcf_rpc_server  *rpcs;           /**< RPC server handle */
    char            *name;           /**< Name for a new RPC server */
    uint32_t         time_to_wait;   /**< How much time to wait after
                                          vfork() call? */
    pid_t            pid;            /**< PID returned by vfork() */
    uint32_t         elapsed_time;   /**< Time elapsed before vfork()
                                          returned */
    te_errno         err;            /**< Error code returned by
                                          rcf_rpc_server_vfork() */
} vfork_thread_data;

/**
 * Call rcf_rpc_server_vfork() in a new thread.
 *
 * @param data          Data to be passed to the thread
 * @param thread_id     Where to save an ID of the created thread
 * @param p_new         Where to save an ID of the created RPC server
 *
 * @return Status code
 */
extern te_errno rcf_rpc_server_vfork_in_thread(vfork_thread_data *data,
                                               pthread_t *thread_id,
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

/**
 * Check whether RPC server is alive.
 *
 * @param rpcs          existing RPC server handle
 *
 * @return @c TRUE or @c FALSE
 */
extern te_bool rcf_rpc_server_is_alive(rcf_rpc_server *rpcs);

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

/**
 * Get RPC server handle, restart it if necessary. Don't fail.
 *
 * @param _ta     name of Test Agent
 * @param _name   name of RCF RPC server
 * @param _rpcs   variable for RPC server handle
 */
#define TEST_GET_RPCS_SAFE(_ta, _name, _rpcs) \
    do {                                                               \
        te_errno    rc = 0;                                            \
        int         is_dead = 0;                                       \
                                                                       \
        if ((rc = rcf_rpc_server_get(_ta, _name, NULL,                 \
                                     RCF_RPC_SERVER_GET_REUSE,         \
                                     &(_rpcs))) != 0)                  \
        {                                                              \
            INFO("Couldn't get RPC server %s: %r", _name, rc);         \
            _rpcs = NULL;                                              \
            break;                                                     \
        }                                                              \
                                                                       \
        /* Restart RPC server if it is dead */                         \
        if ((rc = cfg_get_instance_fmt(NULL, &is_dead,                 \
                                       "/agent:%s/rpcserver:%s/dead:", \
                                       _ta, _name)) != 0)              \
        {                                                              \
            INFO("Couldn't get RPC server dead status %s: %r",         \
                 _name, rc);                                           \
            _rpcs = NULL;                                              \
            break;                                                     \
        }                                                              \
                                                                       \
        if (is_dead == 1)                                              \
        {                                                              \
            INFO("RPC server %s is dead, try to restart", _name);      \
                                                                       \
            if ((rc = rcf_rpc_server_restart(_rpcs)) != 0)             \
            {                                                          \
                INFO("Couldn't restart RPC server %s", _name);         \
                _rpcs = NULL;                                          \
            }                                                          \
        }                                                              \
    } while (0)

/**
 * Free namespace cache
 *
 * @param [in]  rpcs    RPC server
 */
extern void rcf_rpc_namespace_free_cache(rcf_rpc_server *rpcs);

/**
 * Convert namespace id to its name.
 *
 * @param [in]  rpcs    RPC server
 * @param [in]  id      namespace id
 * @param [out] str     namespace as string
 *
 * @return              Status code
 */
extern te_errno rcf_rpc_namespace_id2str(
        rcf_rpc_server *rpcs, rpc_ptr_id_namespace id, char **str);


/* Hook's entry for rcf_rpc_server. */
typedef struct rcf_rpc_server_hook {
    SLIST_ENTRY(rcf_rpc_server_hook) next;  /**< next hook */
    void (*hook)(rcf_rpc_server *rpcs);     /**< function to execute */
} rcf_rpc_server_hook;

typedef SLIST_HEAD(, rcf_rpc_server_hook) rcf_rpc_server_hooks;

/**
 * Add new hook to rcf_rpc_server_hook_list, that will be executed after
 * rcf_rpc_server is created.
 *
 * @param hook_to_register  Function to execute
 *
 * @return                  Status code
 */
extern te_errno rcf_rpc_server_hook_register(
          void (*hook_to_register)(rcf_rpc_server *rpcs));

/**@} <!-- END te_lib_rcfrpc --> */

#endif /* !__TE_RCF_RPC_H__ */
