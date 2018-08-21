/** @file
 * @brief Unix Test Agent
 *
 * Definitions necessary for RPC implementation
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TARPC_SERVER_H__
#define __TARPC_SERVER_H__

#include "te_config.h"
#include "rpc_server_config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#ifdef HAVE_STRUCT_EPOLL_EVENT
#include <sys/epoll.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_AIO_H
#include <aio.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#define TE_ERRNO_LOG_UNKNOWN_OS_ERRNO

#include <stddef.h>
#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "logger_api.h"
#include "logfork.h"
#include "rcf_common.h"
#if defined(ENABLE_RPC_MEM)
#include "rcf_pch_mem.h"
#endif
#include "rcf_rpc_defs.h"
#include "te_rpc_types.h"
#include "rpc_xdr.h"
#include "tarpc.h"

/** System call number of recvmmsg function */
#ifndef SYS_recvmmsg
#ifdef __NR_recvmmsg
#define SYS_recvmmsg __NR_recvmmsg
#else
#if __GNUC__
#if __x86_64__ || __powerpc64__
#define SYS_recvmmsg 299
#elif __powerpc__
#define SYS_recvmmsg 343
#else
#define SYS_recvmmsg 337
#endif
#else
#define SYS_recvmmsg -1
#endif
#endif
#endif

/** System call number of sendmmsg function */
#ifndef SYS_sendmmsg
#ifdef __NR_sendmmsg
#define SYS_sendmmsg __NR_sendmmsg
#else
#if __GNUC__
#if __x86_64__ || __powerpc64__
#define SYS_sendmmsg 307
#elif __powerpc__
#define SYS_sendmmsg 349
#else
#define SYS_sendmmsg 345
#endif
#else
#define SYS_sendmmsg -1
#endif
#endif
#endif

/** Extract sigset from in argument */
#define IN_SIGSET       ((sigset_t *)(rcf_pch_mem_get(in->set)))

/** Extract fdset from in argument */
#define IN_FDSET        ((fd_set *)(rcf_pch_mem_get(in->set)))

/**
 * Extract fdset with namespace from @b in argument
 *
 * @param _ns       Namespace id
 */
#define IN_FDSET_NS(_ns)                                        \
    ((fd_set *)(RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->set, (_ns))))

/** Extract AIO control block from in argument */
#define IN_AIOCB        ((struct aiocb *)(rcf_pch_mem_get(in->cb)))

/** Obtain RCF RPC errno code */
#define RPC_ERRNO errno_h2rpc(errno)

#define TARPC_CHECK_RC(expr_) \
    do {                                            \
        int rc_ = (expr_);                          \
                                                    \
        if (rc_ != 0 && out->common._errno == 0)    \
            out->common._errno = rc_;               \
    } while (FALSE)


/**
 * @name Some generic prototypes of RPC calls
 * RPC framework provides a set of generic macros to
 * define this or that RPC call, but in order to avoid
 * warnings with function type casts we define a number of
 * prototypes that can be used in casting generic function
 * pointer to your function type.
 *
 * @deprecated Use type-safe RPC instead whenever possible
 * @sa TARPC_FUNC_DYNAMIC_SAFE(), TARPC_FUNC_STATIC_SAFE()
 */

/**@{*/
/**
 * Type of RPC call function where
 * - the first argument is of integer type;
 * - return value is an integer.
 * .
 */
typedef int (*api_func)(int param,...);
/**
 * Type of RPC call function where
 * - the first argument is of pointer type;
 * - return value is an integer.
 * .
 */
typedef int (*api_func_ptr)(void *param,...);
/**
 * Type of RPC call function where
 * - there is no arguments;
 * - return value is an integer.
 * .
 */
typedef int (*api_func_void)(void);
/**
 * Type of RPC call function where
 * - the first argument is of integer type;
 * - return value is a pointer.
 * .
 */
typedef void *(*api_func_ret_ptr)(int param,...);
/**
 * Type of RPC call function where
 * - the first argument is of pointer type;
 * - return value is a pointer.
 * .
 */
typedef void *(*api_func_ptr_ret_ptr)(void *param,...);
/**
 * Type of RPC call function where
 * - there is no arguments;
 * - return value is a pointer.
 * .
 */
typedef void *(*api_func_void_ret_ptr)(void);

/**
 * Type of RPC call function where
 * - the first argument is of integer type;
 * - return value is an integer64.
 * .
 */
typedef int64_t (*api_func_ret_int64)(int param,...);

/** @} */

/**
 * Convert shutdown parameter from RPC to native representation.
 *
 * FIXME: Move it to lib/rpc_types.
 */
static inline int
shut_how_rpc2h(rpc_shut_how how)
{
    switch (how)
    {
        case RPC_SHUT_RD:   return SHUT_RD;
        case RPC_SHUT_WR:   return SHUT_WR;
        case RPC_SHUT_RDWR: return SHUT_RDWR;
        default: return (SHUT_RD + SHUT_WR + SHUT_RDWR + 1);
    }
}


/**
 * Get the loading status of dynamic library.
 *
 * @return @c TRUE if dynamic library loaded
 */
extern te_bool tarpc_dynamic_library_loaded(void);

/**
 * Sleep the pending timeout and return error in case
 * dynamic library is not loaded.
 *
 * @param _timeout  Timeout of pending (in milliseconds)
 *
 * @return @b TE_RC(@c TE_TA_UNIX, @c TE_EPENDING) if dynamic library
 *         is not loaded
 */
#define RPCSERVER_PLUGIN_AWAIT_DYNAMIC_LIBRARY(_timeout)    \
    do {                                                    \
        if (!tarpc_dynamic_library_loaded())                \
        {                                                   \
            usleep((_timeout) * 1000);                      \
            return TE_RC(TE_TA_UNIX, TE_EPENDING);          \
        }                                                   \
    } while (0)

/**
 * Find the function by its name.
 *
 * @param use_libc  use the preset library or libc?
 * @param name      function name
 * @param func      location for function address
 *
 * @return status code
 */
extern int tarpc_find_func(te_bool use_libc, const char *name,
                           api_func *func);

/** Structure for checking of variable-length arguments safety */
typedef struct checked_arg {
    STAILQ_ENTRY(checked_arg) next; /**< Next checked argument in the list */

    uint8_t    *real_arg;     /**< Pointer to real buffer */
    uint8_t    *pristine;     /**< Pointer to pristine buffer */
    size_t      len;          /**< Whole length of the buffer */
    size_t      len_visible;  /**< Length passed to the function
                                   under test */
    const char *name;         /**< Argument name to be displayed
                                   in error message */
} checked_arg;

/** List of checked argguments */
typedef STAILQ_HEAD(checked_arg_list, checked_arg) checked_arg_list;

/** Initialise the checked argument and add it into the list */
extern void tarpc_init_checked_arg(checked_arg_list *list, uint8_t *real_arg,
                                   size_t len, size_t len_visible,
                                   const char *name);


#define INIT_CHECKED_ARG_GEN(_list, _real_arg, _len, _len_visible)  \
    tarpc_init_checked_arg(_list, (uint8_t *)(_real_arg), _len,     \
                           _len_visible,  #_real_arg)

#define INIT_CHECKED_ARG(_real_arg, _len, _len_visible)             \
    INIT_CHECKED_ARG_GEN(arglist, _real_arg, _len, _len_visible)


/** Verify that arguments are not corrupted */
extern te_errno tarpc_check_args(checked_arg_list *list);

/**
 * Convert address and register it in the list of checked arguments.
 *
 * @param _name         Where to place converted value handle
 * @param _value        Value to be converted
 * @param _wlen         Visible len (all beyond this len should remain
 *                      unchanged after function call).
 * @param _is_local     Whether local variable or dynamically allocated
 *                      memory should be used to store converted value.
 *                      This *must* be a literal TRUE or FALSE
 * @param _do_register  If @c TRUE, register argument in the list
 *                      to be checked after function call.
 *                      This *must* be a literal TRUE or FALSE
 */
#define PREPARE_ADDR_GEN(_name, _value, _wlen, _is_local,           \
                         _do_register)                              \
    struct sockaddr_storage _name ## _st;   /* Storage */           \
    socklen_t               _name ## len = 0;                       \
    struct sockaddr        *_name = NULL;                           \
    PREPARE_ADDR_GEN_IS_LOCAL_DEFS_##_is_local(_name)               \
                                                                    \
    if (!(_value.flags & TARPC_SA_RAW &&                            \
          _value.raw.raw_len > sizeof(struct sockaddr_storage)))    \
    {                                                               \
        te_errno __rc = sockaddr_rpc2h(&(_value), SA(&_name ## _st),\
                                       sizeof(_name ## _st),        \
                                       &_name, &_name ## len);      \
        if (__rc != 0)                                              \
        {                                                           \
            out->common._errno = __rc;                              \
        }                                                           \
        else                                                        \
        {                                                           \
            PREPARE_ADDR_GEN_IS_LOCAL_COPY_##_is_local(_name);      \
                                                                    \
            PREPARE_ADDR_GEN_DO_REGISTER_##_do_register(            \
                PREPARE_ADDR_GEN_IS_LOCAL_VAR_##_is_local(_name),   \
                _name ## len, _wlen);                               \
        }                                                           \
    }

#define PREPARE_ADDR_GEN_IS_LOCAL_DEFS_TRUE(_name)
#define PREPARE_ADDR_GEN_IS_LOCAL_DEFS_FALSE(_name)     \
    struct sockaddr        *_name ## _dup = NULL;       \

#define PREPARE_ADDR_GEN_IS_LOCAL_VAR_TRUE(_name) _name
#define PREPARE_ADDR_GEN_IS_LOCAL_VAR_FALSE(_name) _name ## _dup

#define PREPARE_ADDR_GEN_IS_LOCAL_COPY_FALSE(_name)                 \
    if (_name != NULL)                                              \
    {                                                               \
        _name ## _dup = calloc(1, _name ## len);                    \
        if (_name ## _dup == NULL)                                  \
            out->common._errno = TE_ENOMEM;                         \
        else                                                        \
            memcpy(_name ## _dup, _name, _name ## len);             \
    }
#define PREPARE_ADDR_GEN_IS_LOCAL_COPY_TRUE(_name)

#define PREPARE_ADDR_GEN_DO_REGISTER_TRUE(_name, _len, _wlen)   \
    INIT_CHECKED_ARG(_name, _len, _wlen);

#define PREPARE_ADDR_GEN_DO_REGISTER_FALSE(_name, _len, _wlen)


/**
 * Convert address and register it in the list of checked arguments.
 *
 * @param _name     Where to place converted value handle
 * @param _value    Value to be converted
 * @param _wlen     Visible len (all beyond this len should remain
 *                  unchanged after function call.
 */
#define PREPARE_ADDR(_name, _value, _wlen)              \
    PREPARE_ADDR_GEN(_name, _value, _wlen, TRUE, TRUE)

/**
 * Copy in variable argument to out variable argument and zero in argument.
 * out and in are assumed in the context.
 */
#define COPY_ARG(_a)                        \
    do {                                    \
        out->_a._a##_len = in->_a._a##_len; \
        out->_a._a##_val = in->_a._a##_val; \
        in->_a._a##_len = 0;                \
        in->_a._a##_val = NULL;             \
    } while (0)

#define COPY_ARG_ADDR(_a) \
    do {                                    \
        out->_a = in->_a;                   \
        memset(&in->_a, 0, sizeof(in->_a)); \
    } while (0)

#define COPY_ARG_NOTNULL(_a)                                    \
    do {                                                        \
        if (in->_a._a##_len == 0)                               \
        {                                                       \
            ERROR("Argument %s cannot be NULL", #_a);           \
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);  \
            return TRUE;                                        \
        }                                                       \
        COPY_ARG(_a);                                           \
    } while (0)

struct rpc_call_data;

/** Structure for storing data about error occurred during RPC call. */
typedef struct te_rpc_error_data {
    te_errno    err;                     /**< Error number. */
    char        str[RPC_ERROR_MAX_LEN];  /**< String describing error. */
} te_rpc_error_data;

/**
 * Save information about error occurred during RPC call which will
 * be reported to caller.
 *
 * @note If this function is not used (or @p err is set to @c 0),
 *       errno value will be reported to caller.
 *
 * @param err    Error number.
 * @param msg    Format string for error message.
 * @param ...    Arguments for format string.
 */
extern void te_rpc_error_set(te_errno err, const char *msg, ...);

/**
 * Get error number set with te_rpc_error_set() the last time.
 *
 * @return Error number.
 */
extern te_errno te_rpc_error_get_num(void);

/**
 * Do some preparations before passing an call to a real function:
 * - probably wait for a specific deadline
 * - record a starting timestamp
 * - save errno
 * - do logging
 *
 * @note This function is normally only called from inside MAKE_CALL()
 * @sa tarpc_after_call()
 */
extern void tarpc_before_call(struct rpc_call_data *call, const char *id);

/**
 * Do some postprocessing after the real RPC work is done:
 * - record errno status
 * - record call duration
 * - check the registered checked args validity
 *
 * @note This function is normally only called from inside MAKE_CALL()
 * @sa tarpc_before_call()
 */
extern void tarpc_after_call(struct rpc_call_data *call);

/**
 * Execute code wrapped into tarpc_before_call()/tarpc_after_call()
 */
#define MAKE_CALL(_code)                                            \
    do {                                                            \
        tarpc_before_call(_call, #_code);                           \
        _code;                                                      \
        tarpc_after_call(_call);                                    \
    } while (0)

/**
 * Type of functions implementing an RPC wrapper around real code
 * @note This is a function type, not a function pointer
 */
typedef void rpc_wrapper_func(struct rpc_call_data *call);

/**
 * Type of functions doing input-to-output copying for RPC calls.
 * @note This is a function type, not a function pointer
 */
typedef te_bool rpc_copy_func(void *in, void *out);

/**
 * Generic XDR resource-freeing routine pointer
 */
typedef bool_t (*rpc_generic_xdr_out)(XDR *, void *out);

/** Description of a RPC routine implementation */
typedef struct rpc_func_info {
    const char *funcname;        /**< Name of the function */
    rpc_wrapper_func *wrapper;   /**< RPC wrapper */
    rpc_copy_func *copy;         /**< Argument copying routing */
    rpc_generic_xdr_out xdr_out; /**< XDR resource deallocator */
    size_t in_size;              /**< Size of input arguments structure */
    size_t out_size;             /**< Size of output arguments structure */
    size_t in_common_offset;     /**< Offset of #tarpc_in_arg within the
                                  *   input structure (usually 0)
                                  */
    size_t out_common_offset;    /**< Offset of #tarpc_in_arg within the
                                  *   input structure (usually 0)
                                  */
} rpc_func_info;

/** RPC call activation details */
typedef struct rpc_call_data {
    const rpc_func_info *info; /**< RPC routine description */
    api_func func;             /**< Actual function to call */
    void *in;                  /**< Input data */
    void *out;                 /**< Output data */
    checked_arg_list checked_args; /**< List of checked arguments */
    te_bool done;              /**< Completion status
                                * @note Only used for async calls
                                */
    struct timeval call_start; /**< Timestamp when the actual code is
                                * called (used by MAKE_CALL())
                                */
    int saved_errno;           /**< Saved errno (used by MAKE_CALL()) */
} rpc_call_data;


extern void tarpc_call_unsupported(const char *name, void *out,
                                   size_t outsize, size_t common_offset);


/**
 * Opaque structure to hold the list of asynchronous RPC calls
 */
typedef struct deferred_call_list deferred_call_list;

extern te_errno tarpc_defer_call(deferred_call_list *list,
                                 uintptr_t jobid, rpc_call_data *call);

extern te_bool tarpc_has_deferred_calls(const deferred_call_list *list);

/**
 * Generic RPC handler.
 * It does all preparations, most imporant
 * - copy arguments
 * - set up an asynchronous call context if needed
 * and then calls the real code
 */
extern void tarpc_generic_service(deferred_call_list *list,
                                  rpc_call_data *call);

/*
 * FIXME: here TE_FUNC_CAST() macro is used because we by default
 * represent function as api_func (take integer argument and may be
 * other arguments as well, return integer value), and not every
 * system function matches such prototype. Perhaps we need to
 * use void (*)(void) as default type which at least can be casted
 * to any other function pointer type without warning, so that
 * there will be no need in TE_FUNC_CAST() here.
 */

#define TARPC_FUNC_DECL_UNSAFE(_func)                                   \
    const api_func func = _call->func;                                  \
    const api_func_ptr func_ptr = TE_FUNC_CAST(api_func_ptr, func);     \
    const api_func_void func_void =  TE_FUNC_CAST(api_func_void, func); \
    const api_func_ret_ptr func_ret_ptr =                               \
        TE_FUNC_CAST(api_func_ret_ptr, func);                           \
    const api_func_ptr_ret_ptr func_ptr_ret_ptr =                       \
        TE_FUNC_CAST(api_func_ptr_ret_ptr, func);                       \
                                                                        \
    const api_func_void_ret_ptr func_void_ret_ptr =                     \
        TE_FUNC_CAST(api_func_void_ret_ptr, func);                      \
    const api_func_ret_int64 func_ret_int64 =                           \
        TE_FUNC_CAST(api_func_ret_int64, func);

#define TARPC_FUNC_DECL_SAFE(_func)                                     \
    __typeof(_func) * const func =                                      \
                  TE_FUNC_CAST(__typeof(_func) *, _call->func);         \
    __typeof(_func) * const func_ptr = func;                            \
    __typeof(_func) * const func_void =  func;                          \
    __typeof(_func) * const func_ret_ptr = func;                        \
    __typeof(_func) * const func_ptr_ret_ptr = func;                    \
    __typeof(_func) * const func_void_ret_ptr = func;                   \
    __typeof(_func) * const func_ret_int64 = func;

#define TARPC_FUNC_DECL_STANDALONE(_func)

#define TARPC_FUNC_INIT_DYNAMIC(_func) NULL
#define TARPC_FUNC_INIT_STATIC(_func) (TE_FUNC_CAST(api_func, _func))
#define TARPC_FUNC_INIT_STANDALONE(_func) (TE_FUNC_CAST(api_func, abort))

#define TARPC_FUNC_UNUSED_UNSAFE                                        \
    UNUSED(func_ptr);                                                   \
    UNUSED(func_void);                                                  \
    UNUSED(func_ret_ptr);                                               \
    UNUSED(func_ptr_ret_ptr);                                           \
    UNUSED(func_void_ret_ptr);                                          \
    UNUSED(func_ret_int64);                                             \

#define TARPC_FUNC_UNUSED_SAFE TARPC_FUNC_UNUSED_UNSAFE
#define TARPC_FUNC_UNUSED_STANDALONE

/** @name RPC wrapper definitions.
 *
 * There is now a family of macros which vary in how the target symbol
 * lookup is done and whether the call is type-checked against the symbol
 * prototype:
 * - TARPC_FUNC_DYNAMIC_UNSAFE(): retains the behaviour of earlier
 * TARPC_FUNC(). Symbol lookup is done purely at runtime, and the type of
 * the function is not checked
 * - TARPC_FUNC_DYNAMIC_SAFE(): Symbol lookup is done dynamically, but the
 * type of the function is checked by the compiler according to the
 * prototype.
 * - TARPC_FUNC_STATIC_SAFE(): Symbol lookup is done statically by the
 * compiler and the function type is checked
 * - TARPC_FUNC_STANDALONE(): There is no targer symbol, so there's no no
 * symbol lookup and nothing to check.
 *
 * The use of `_SAFE` macros is only possible if it's enabled at
 * compile-time and the compiler supports `__typeof` (it is enabled by
 * default).
 *
 * The implicit macros are also provided: TARPC_FUNC() and
 * TARPC_FUNC_STATIC() which will use either typesafe or type-generic
 * interface, depending on whether typesafe RPCs are enabled.
 *
 * The flowchart to select the right macro is as follows:
 * - If any other concers do not apply, always use TARPC_FUNC().
 * - If the target symbol is an inline function, use TARPC_FUNC_STATIC().
 * - If the target symbol is a static function, use TARPC_FUNC_STATIC().
 * - If the prototype of the function cannot be made known at compile time,
 *   or if the prototype may vary, use TARPC_FUNC_DYNAMIC_UNSAFE(). In
 *   general, such situtation should be avoided.
 * - If the target symbol is actually a function-like macro,
 *   use TARPC_FUNC_STANDALONE(), because even TARPC_FUNC_STATIC() would
 *   attempt to take the address of the function. However, if the target
 *   symbol is a constant-like macro which aliases another function, then
 *   TARPC_FUNC_STATIC() would work (but *not* dynamic TARPC_FUNC())
 * - If the RPC wrapper is short and simple enough to do all the work by
 *   itself, TARPC_FUNC_STANDALONE() should be used. But it is recommended
 *   to keep RPC wrappers really short, and abstract all non-trivial code
 *   into separate functions
 * - If the RPC wrapper does something non-trivial, use
 *   TARPC_FUNC_STANDALONE(). A common example is when the target symbol has
 *   different name from the name of the RPC function. TARPC_FUNC() cannot
 *   handle such cases. However, such usage should in general be
 *   discouraged.
 *
 * @note If an RPC implementation code defined by any of those macros may
 * safely call `return` with no argument if it wishes to terminate
 * processing early (i.e. in case of an error).
 */

/**@{*/

/**
 * Internal support macro to define RPC function content.
 *
 * @param _safe       Whether to use type-safe or type-generic definitions
 * @param _static     Whether to use compile-time or run-time symbol
 *                    resolution
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 * @private
 */
#define TARPC_FUNC_COMMON(_safe, _static, _func, _copy_args, _actions)  \
                                                                        \
    static rpc_wrapper_func _func##_wrapper;                            \
                                                                        \
    static void                                                         \
    _func##_wrapper(rpc_call_data *_call)                               \
    {                                                                   \
        tarpc_##_func##_in  * const in  = _call->in;                    \
        tarpc_##_func##_out * const out = _call->out;                   \
        TARPC_FUNC_DECL_##_safe(_func)                                  \
                                                                        \
        checked_arg_list *arglist = &(_call->checked_args);             \
                                                                        \
        UNUSED(in);                                                     \
        UNUSED(out);                                                    \
        TARPC_FUNC_UNUSED_##_safe                                       \
        UNUSED(arglist);                                                \
                                                                        \
        { _actions }                                                    \
    }                                                                   \
                                                                        \
    static rpc_copy_func _func##_docopy;                                \
                                                                        \
    static te_bool                                                      \
    _func##_docopy(void *_in, void *_out)                               \
    {                                                                   \
        tarpc_##_func##_in  * const in  = _in;                          \
        tarpc_##_func##_out * const out = _out;                         \
                                                                        \
        UNUSED(in);                                                     \
        UNUSED(out);                                                    \
                                                                        \
        { _copy_args }                                                  \
                                                                        \
        return FALSE;                                                   \
    }                                                                   \
                                                                        \
    bool_t                                                              \
    _##_func##_1_svc(tarpc_##_func##_in *_in,                           \
                     tarpc_##_func##_out *_out,                         \
                     struct svc_req *_rqstp)                            \
    {                                                                   \
        static const rpc_func_info _info = {                            \
            .funcname = #_func,                                         \
            .wrapper = _func##_wrapper,                                 \
            .copy = _func##_docopy,                                     \
            .xdr_out = (rpc_generic_xdr_out)xdr_tarpc_##_func##_out,    \
            .in_size = sizeof(*_in),                                    \
            .in_common_offset = offsetof(tarpc_##_func##_in, common),   \
            .out_size = sizeof(*_out),                                  \
            .out_common_offset = offsetof(tarpc_##_func##_out, common)  \
        };                                                              \
                                                                        \
        rpc_call_data _call = {                                         \
            .info = &_info,                                             \
            .in = _in,                                                  \
            .out = _out,                                                \
            .func = TARPC_FUNC_INIT_##_static(_func),                   \
            .checked_args = STAILQ_HEAD_INITIALIZER(_call.checked_args) \
        };                                                              \
                                                                        \
        tarpc_generic_service((void *)_rqstp->rq_xprt->xp_p1, &_call);  \
        return TRUE;                                                    \
    }

/**
 * Macro to define RPC wrapper.
 *
 * This macro uses dynamic run-time symbol lookup and provides
 * type-generic thunks to the underlying function:
 * - `func`              (#api_func)
 * - `func_ptr`          (#api_func_ptr)
 * - `func_void`         (#api_func_void)
 * - `func_ret_ptr`      (#api_func_ret_ptr)
 * - `func_ptr_ret_ptr`  (#api_func_ptr_ret_ptr)
 * - `func_void_ret_ptr` (#api_func_void_ret_ptr)
 * - `func_ret_int64`    (#api_func_ret_int64)
 * .
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 *
 * @note Use this macro only if the prototype of the underlying function
 * is not known at compile-time, in other cases use plain TARPC_FUNC()
 */
#define TARPC_FUNC_DYNAMIC_UNSAFE(_func, _copy_args, _actions) \
    TARPC_FUNC_COMMON(UNSAFE, DYNAMIC, _func,                  \
                      _copy_args, _actions)

/**
 * Macro to define RPC wrapper.
 *
 * It is analogous to TARPC_FUNC_STATIC_UNSAFE(), but it uses
 * static (i.e. compile-time) symbol lookup, so the symbol must be
 * declared at the point of use of this macro.
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 *
 * @deprecated *Never ever* use this macro. It is provided only to
 * support static symbol resolution for compilers that lack `__typeof`
 * keyword support.
 */

#define TARPC_FUNC_STATIC_UNSAFE(_func, _copy_args, _actions)  \
    TARPC_FUNC_COMMON(UNSAFE, STATIC, _func,                   \
                      _copy_args, _actions)

#ifdef ENABLE_TYPESAFE_RPC

/**
 * Macro to define RPC wrapper.
 * This macro is analogous to TARPC_FUNC_DYNAMIC_UNSAFE(), but it
 * provides type-safe thunks (e.g. `func`) to the underlying function.
 * The function implementation is looked up at run-time, but its prototype
 * must be declared at the point where the macro used.
 * For compatibility, the whole list of thunks is provided
 * (see TARPC_FUNC_DYNAMIC_UNSAFE()), but they all have the same type as `func`.
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 */
#define TARPC_FUNC_DYNAMIC_SAFE(_func, _copy_args, _actions)   \
    TARPC_FUNC_COMMON(SAFE, DYNAMIC, _func,                    \
                      _copy_args, _actions)

/**
 * Macro to define RPC wrapper.
 * This macro is analogous to TARPC_FUNC_DYNAMIC_SAFE(), but it
 * uses compile-type symbol resolution, so both the function prototype
 * must be known and the function itself must be linked to the agent.
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 */

#define TARPC_FUNC_STATIC_SAFE(_func, _copy_args, _actions)    \
    TARPC_FUNC_COMMON(SAFE, STATIC, _func,                     \
                      _copy_args, _actions)

/**
 * Macro to define RPC wrapper.
 * This macro is equivalent to either TARPC_FUNC_STATIC_SAFE() or
 * TARPC_FUNC_STATIC_UNSAFE() depending on whether type-safe RPC support
 * is enabled (which is the default)
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 */
#define TARPC_FUNC_STATIC(_func, _copy_args, _actions)  \
    TARPC_FUNC_STATIC_SAFE(_func, _copy_args, _actions)

/**
 * Macro to define RPC wrapper.
 * This macro is equivalent to either TARPC_FUNC_DYNAMIC_SAFE() or
 * TARPC_FUNC_DYNAMIC_UNSAFE() depending on whether type-safe RPC support
 * is enabled (which is the default)
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 * @note Always use this macro if there is no special considerations
 */
#ifdef DEFAULT_STATIC_RPC_LOOKUP
#define TARPC_FUNC(_func, _copy_args, _actions)             \
    TARPC_FUNC_STATIC_SAFE(_func, _copy_args, _actions)
#else
#define TARPC_FUNC(_func, _copy_args, _actions)             \
    TARPC_FUNC_DYNAMIC_SAFE(_func, _copy_args, _actions)
#endif

#else
#define TARPC_FUNC_STATIC(_func, _copy_args, _actions)      \
    TARPC_FUNC_STATIC_UNSAFE(_func, _copy_args, _actions)

#ifdef DEFAULT_STATIC_RPC_LOOKUP
#define TARPC_FUNC(_func, _copy_args, _actions)             \
    TARPC_FUNC_STATIC_UNSAFE(_func, _copy_args, _actions)
#else
#define TARPC_FUNC(_func, _copy_args, _actions)             \
    TARPC_FUNC_DYNAMIC_UNSAFE(_func, _copy_args, _actions)
#endif

#endif /* ENABLE_TYPESAFE_RPC */

/**
 * Macro to define RPC wrapper.
 * This is a special version for wrappers that do all the work themselves,
 * having no underlying ordinary function.
 * Therefore, `func` and its keen are *not* available in this macro.
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 * @note It is only recommended to use this macro for short custom
 * functions, where making a separate C functions would be too verbose
 * (cf. `inline` C functions)
 */

#define TARPC_FUNC_STANDALONE(_func, _copy_args, _actions) \
    TARPC_FUNC_COMMON(STANDALONE, STANDALONE, _func,       \
                      _copy_args, _actions)

/**@}*/

/**
 * Check that a RPC input parameter @p _inname is not NULL.
 * Intended to be used before MAKE_CALL() for system functions that
 * are declared `nonnull`.
 * @note This macro should be called from inside TARPC_FUNC() or similiar
 * macro
 */
#define TARPC_ENSURE_NOT_NULL(_inname)                          \
    do {                                                        \
        if (in->_inname._inname##_len == 0)                     \
        {                                                       \
            ERROR("Argument %s cannot be NULL", #_inname);      \
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);  \
            return;                                             \
        }                                                       \
    } while(0)

typedef void (*sighandler_t)(int);

/**
 * Sleep the pending timeout and return error in case
 * non-blocking call is executed.
 *
 * @param _list     List of asynchronous RPCs
 * @param _timeout  Timeout of pending (in milliseconds)
 *
 * @return @b TE_RC(@c TE_TA_UNIX, @c TE_EPENDING) if non-blocking call
 *         is executed
 */
#define RPCSERVER_PLUGIN_AWAIT_RPC_CALL(_list, _timeout)    \
    do {                                                    \
        if (tarpc_has_deferred_calls(_list))                \
        {                                                   \
            usleep((_timeout) * 1000);                      \
            return TE_RC(TE_TA_UNIX, TE_EPENDING);          \
        }                                                   \
    } while (0)

/**
 * Entry function for RPC server.
 *
 * @param name    RPC server name
 *
 * @return NULL
 */
extern void *rcf_pch_rpc_server(const char *name);

/**
 * Wrapper to call rcf_pch_rpc_server via "ta exec func" mechanism.
 *
 * @param argc    should be 1
 * @param argv    should contain pointer to RPC server name
 */
extern void rcf_pch_rpc_server_argv(int argc, char **argv);

/**
 * Find all callbacks and enable the RPC server plugin.
 *
 * @param install   Function name for install plugin or empty string or
 *                  @c NULL
 * @param action    Function name for plugin action or empty string or
 *                  @c NULL
 * @param uninstall Function name for uninstall plugin or empty string or
 *                  @c NULL
 *
 * @return Status code
 */
extern te_errno rpcserver_plugin_enable(
        const char *install, const char *action, const char *uninstall);

/**
 * Disable the RPC server plugin.
 *
 * @return Status code
 */
extern te_errno rpcserver_plugin_disable(void);

/**
 * Special signal handler which registers signals.
 *
 * @param signum    received signal
 */
extern void signal_registrar(int signum);

/**
 * Special signal handler which registers signals and also
 * saves signal information.
 *
 * @param signum    received signal
 * @param siginfo   pointer to siginfo_t structure
 * @param context   pointer to user context
 */
extern void signal_registrar_siginfo(int signum, siginfo_t *siginfo,
                                     void *context);

extern sigset_t rpcs_received_signals;
extern tarpc_siginfo_t last_siginfo;


#endif /* __TARPC_SERVER_H__ */
