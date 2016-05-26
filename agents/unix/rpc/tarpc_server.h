/** @file
 * @brief Unix Test Agent
 *
 * Definitions necessary for RPC implementation
 *
 * Copyright (C) 2004-2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TARPC_SERVER_H__
#define __TARPC_SERVER_H__

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

#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#if HAVE_STRUCT_EPOLL_EVENT
#include <sys/epoll.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#if HAVE_SYS_FILIO_H
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

#if HAVE_AIO_H
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
#include "logger_api.h"
#include "logfork.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_rpc_defs.h"
#include "te_rpc_types.h"
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
 */

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
typedef int (*api_func_void)();
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
typedef void *(*api_func_void_ret_ptr)();

/**
 * Type of RPC call function where
 * - the first argument is of integer type;
 * - return value is an integer64.
 * .
 */
typedef int64_t (*api_func_ret_int64)(int param,...);

/** @} */

/**
 * @name RPC call reference name depending on RPC function prototype
 * The third argument of TARPC_FUNC() macro defines context in which
 * function call is made. That call can be done via @p func variable
 * defined in the macro context, but the type of @p func variable is
 * api_func, which may not match with RPC function prototype.
 * In order to avoid compilation warning associated with incorrect
 * function type you can use the following names instead of @p func -
 * they are just casted versions of @p func variable:
 * - func_ptr          (#api_func_ptr)
 * - func_void         (#api_func_void)
 * - func_ret_ptr      (#api_func_ret_ptr)
 * - func_ptr_ret_ptr  (#api_func_ptr_ret_ptr)
 * - func_void_ret_ptr (#api_func_void_ret_ptr)
 * - func_ret_int64    (#api_func_ret_int64)
 */


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

struct rpc_call_data;

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
    sigset_t mask;             /**< Saved signal mask
                                * @note Only used for async calls
                                */
    checked_arg_list checked_args; /**< List of checked arguments */
    te_bool done;              /**< Completion status
                                * @note Only used for async calls
                                */
    struct timeval call_start; /**< Timestamp when the actual code is
                                * called (used by MAKE_CALL())
                                */
    int saved_errno;           /**< Saved errno (used by MAKE_CALL()) */
} rpc_call_data;


/**
 * Generic RPC handler.
 * It does all preparations, most imporant
 * - copy arguments
 * - set up an asynchronous call context if needed
 * and then calls the real code
 */
extern void tarpc_generic_service(rpc_call_data *call);

/**
 * Macro to define RPC function content.
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 */
#define TARPC_FUNC(_func, _copy_args, _actions)                         \
                                                                        \
    static rpc_wrapper_func _func##_wrapper;                            \
                                                                        \
    static void                                                         \
    _func##_wrapper(rpc_call_data *_call)                               \
    {                                                                   \
        tarpc_##_func##_in  * const in  = _call->in;                    \
        tarpc_##_func##_out * const out = _call->out;                   \
        const api_func func = _call->func;                              \
        const api_func_ptr func_ptr = (api_func_ptr)func;               \
        const api_func_void func_void =  (api_func_void)func;           \
        const api_func_ret_ptr func_ret_ptr = (api_func_ret_ptr)func;   \
        const api_func_ptr_ret_ptr func_ptr_ret_ptr =                   \
                                   (api_func_ptr_ret_ptr)func;          \
                                                                        \
        const api_func_void_ret_ptr func_void_ret_ptr =                 \
                                   (api_func_void_ret_ptr)func;         \
        const api_func_ret_int64 func_ret_int64 = (api_func_ret_int64)func; \
                                                                        \
        checked_arg_list *arglist = &(_call->checked_args);             \
                                                                        \
        UNUSED(in);                                                     \
        UNUSED(out);                                                    \
        UNUSED(func_ptr);                                               \
        UNUSED(func_void);                                              \
        UNUSED(func_ret_ptr);                                           \
        UNUSED(func_ptr_ret_ptr);                                       \
        UNUSED(func_void_ret_ptr);                                      \
        UNUSED(func_ret_int64);                                         \
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
    _##_func##_1_svc(tarpc_##_func##_in *_in, tarpc_##_func##_out *_out, \
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
            .func = NULL,                                               \
            .checked_args = STAILQ_HEAD_INITIALIZER(_call.checked_args) \
        };                                                              \
        UNUSED(_rqstp);                                                 \
                                                                        \
        tarpc_generic_service(&_call);                                  \
        return TRUE;                                                    \
    }




typedef void (*sighandler_t)(int);

#endif /* __TARPC_SERVER_H__ */
