/** @file
 * @brief Unix Test Agent
 *
 * Definitions necessary for RPC implementation
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TARPC_SERVER_H__
#define __TARPC_SERVER_H__

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif


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

#define TE_ERRNO_LOG_UNKNOWN_OS_ERRNO

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
#if __x86_64__ || __ppc64__
#define SYS_recvmmsg 299
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
#if __x86_64__ || __ppc64__
#define SYS_sendmmsg 307
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

/** @} */

/**
 * @name RPC call reference name depending on RPC function prototype
 * The third argument of TARPC_FUNC() macro defines context in which
 * function call is made. That call can be done via @p func variable
 * defined in the macro context, but the type of @p func variable is
 * api_func, which may not match with RPC function prototype.
 * In order to avoid compilation warning associated with incorrect
 * function type you can use the following names instead of @p func -
 * they are just casted versions of @p func variable.
 */

/**
 * RPC function call reference name to use when RPC call matches
 * @ref api_func_ptr prototype
 */
#define func_ptr                ((api_func_ptr)func)
/**
 * RPC function call reference name to use when RPC call matches
 * @ref api_func_void prototype
 */
#define func_void               ((api_func_void)func)
/**
 * RPC function call reference name to use when RPC call matches
 * @ref api_func_ret_ptr prototype
 */
#define func_ret_ptr            ((api_func_ret_ptr)func)
/**
 * RPC function call reference name to use when RPC call matches
 * @ref api_func_ptr_ret_ptr prototype
 */
#define func_ptr_ret_ptr        ((api_func_ptr_ret_ptr)func)
/**
 * RPC function call reference name to use when RPC call matches
 * @ref api_func_void_ret_ptr prototype
 */
#define func_void_ret_ptr       ((api_func_void_ret_ptr)func)

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
    struct checked_arg *next; /**< Next checked argument in the list */

    uint8_t    *real_arg;     /**< Pointer to real buffer */
    void       *control;      /**< Pointer to control buffer */
    int         len;          /**< Whole length of the buffer */
    int         len_visible;  /**< Length passed to the function
                                   under test */
    char       *name;         /**< Argument name to be displayed
                                   in error message */
} checked_arg;

/** Initialise the checked argument and add it into the list */
static inline void
init_checked_arg_gen(checked_arg **list, uint8_t *real_arg,
                     int len, int len_visible, char *name)
{
    checked_arg *arg;

    if (real_arg == NULL || len <= len_visible)
        return;

    if ((arg = calloc(1, sizeof(*arg))) == NULL)
    {
        ERROR("Out of memory");
        return;
    }

    if ((arg->control = malloc(len - len_visible)) == NULL)
    {
        ERROR("Out of memory");
        free(arg);
        return;
    }
    memcpy(arg->control, real_arg + len_visible, len - len_visible);
    arg->real_arg = real_arg;
    arg->len = len;
    arg->len_visible = len_visible;
    arg->name = name;
    arg->next = *list;
    *list = arg;
}

#define INIT_CHECKED_ARG(_real_arg, _len, _len_visible) \
    init_checked_arg_gen(list_ptr, (uint8_t *)(_real_arg), _len, \
                         _len_visible,  #_real_arg)

static inline void
init_checked_arg(checked_arg **list, uint8_t *real_arg,
                 int len, int len_visible)
{
    return init_checked_arg_gen(list, real_arg, len, len_visible,
                                "unnamed");
}

/** Verify that arguments are not corrupted */
static inline int
check_args(checked_arg *list)
{
    int rc = 0;

    checked_arg *cur, *next;

    for (cur = list; cur != NULL; cur = next)
    {
        next = cur->next;
        if (memcmp(cur->real_arg + cur->len_visible, cur->control,
                   cur->len - cur->len_visible) != 0)
        {
            ERROR("Argument %s:\nVisible length is %u.\nControl is:%Tm"
                  "Current is:%Tm + %Tm",
                  cur->name,
                  cur->len_visible, cur->control,
                  cur->len - cur->len_visible,
                  cur->real_arg, cur->len_visible,
                  cur->real_arg + cur->len_visible,
                  cur->len - cur->len_visible);
            rc = TE_RC(TE_TA_UNIX, TE_ECORRUPTED);
        }
        free(cur->control);
        free(cur);
    }

    return rc;
}

/**
 * Convert address and register it in the list of checked arguments.
 *
 * @param _name         Where to place converted value handle
 * @param _value        Value to be converted
 * @param _wlen         Visible len (all beyond this len should remain
 *                      unchanged after function call).
 * @param _is_local     Whether local variable or dynamically allocated
 *                      memory should be used to store converted value
 * @param _do_register  If @c TRUE, register argument in the list
 *                      to be checked after function call.
 */
#define PREPARE_ADDR_GEN(_name, _value, _wlen, _is_local, \
                         _do_register) \
    te_errno                _name ## _rc;                           \
    struct sockaddr_storage _name ## _st;   /* Storage */           \
    socklen_t               _name ## len;                           \
    struct sockaddr        *_name;                                  \
    struct sockaddr        *_name ## _dup = NULL;                   \
                                                                    \
    if (!(_value.flags & TARPC_SA_RAW &&                            \
          _value.raw.raw_len > sizeof(struct sockaddr_storage)))    \
    {                                                               \
        _name ## _rc = sockaddr_rpc2h(&(_value), SA(&_name ## _st), \
                                      sizeof(_name ## _st),         \
                                      &_name, &_name ## len);       \
        if (_name ## _rc != 0)                                      \
        {                                                           \
             out->common._errno = _name ## _rc;                     \
        }                                                           \
        else                                                        \
        {                                                           \
            if (!_is_local)                                         \
            {                                                       \
                if (_name != NULL)                                  \
                {                                                   \
                    _name ## _dup = calloc(1, _name ## len);        \
                    if (_name ## _dup == NULL)                      \
                        out->common._errno = TE_ENOMEM;             \
                    else                                            \
                        memcpy(_name ## _dup, _name, _name ## len); \
                }                                                   \
                                                                    \
                if (_do_register)                                   \
                INIT_CHECKED_ARG((char *)_name ## _dup,             \
                                 _name ## len,                      \
                                 _wlen);                            \
            }                                                       \
            else if (_do_register)                                  \
                INIT_CHECKED_ARG((char *)_name, _name ## len,       \
                                 _wlen);                            \
        }                                                           \
    }

/**
 * Convert address and register it in the list of checked arguments.
 *
 * @param _name     Where to place converted value handle
 * @param _value    Value to be converted
 * @param _wlen     Visible len (all beyond this len should remain
 *                  unchanged after function call.
 */
#define PREPARE_ADDR(_name, _value, _wlen) \
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

/**
 * Find function by its name with check.
 * out variable is assumed in the context.
 */
#define FIND_FUNC(_lib, _name, _func) \
    do {                                                 \
        int rc = tarpc_find_func(_lib, _name, &(_func)); \
                                                         \
        if (rc != 0)                                     \
        {                                                \
             out->common._errno = rc;                    \
             return TRUE;                                \
        }                                                \
    } while (0)

/** Wait time specified in the input argument. */
#define WAIT_START(msec_start)                                  \
    do {                                                        \
        struct timeval t;                                       \
                                                                \
        uint64_t msec_now;                                      \
                                                                \
        gettimeofday(&t, NULL);                                 \
        msec_now = ((uint64_t)(t.tv_sec)) * 1000ULL +           \
                   (t.tv_usec) / 1000;                          \
                                                                \
        if (msec_start > msec_now)                              \
        {                                                       \
            RING("Sleep %u microseconds before call",           \
                 (unsigned)TE_MS2US(msec_start - msec_now));    \
            usleep(TE_MS2US(msec_start - msec_now));            \
        }                                                       \
        else if (msec_start != 0)                               \
            WARN("Start time is gone");                         \
    } while (0)

/**
 * Declare and initialise time variables; execute the code and store
 * duration and errno in the output argument.
 */
#define MAKE_CALL(x)                                             \
    do {                                                         \
        struct timeval t_start;                                  \
        struct timeval t_finish;                                 \
        int            _rc;                                      \
        int            _errno_save = errno;                      \
                                                                 \
        WAIT_START(in->common.start);                            \
        VERB("Calling: %s" , #x);                                \
        gettimeofday(&t_start, NULL);                            \
        x;                                                       \
        out->common.errno_changed = (_errno_save != errno);      \
        out->common._errno = RPC_ERRNO;                          \
        gettimeofday(&t_finish, NULL);                           \
        out->common.duration =                                   \
            TE_SEC2US(t_finish.tv_sec - t_start.tv_sec) +        \
            t_finish.tv_usec - t_start.tv_usec;                  \
        _rc = check_args(*list_ptr);                             \
        if (out->common._errno == 0 && _rc != 0)                 \
            out->common._errno = _rc;                            \
    } while (0)

/**
 * Macro to define RPC function content.
 *
 * @param _func       RPC function name
 * @param _copy_args  block of code that can be used for
 *                    copying input argument values into output
 *                    (this is usually done for IN/OUT arguments)
 * @param _actions    RPC function body
 */
#define TARPC_FUNC(_func, _copy_args, _actions)                     \
                                                                    \
typedef struct _func##_arg {                                        \
    api_func            func;                                       \
    tarpc_##_func##_in  in;                                         \
    tarpc_##_func##_out out;                                        \
    sigset_t            mask;                                       \
    te_bool             done;                                       \
} _func##_arg;                                                      \
                                                                    \
static void *                                                       \
_func##_proc(void *arg)                                             \
{                                                                   \
    _func##_arg            *data = (_func##_arg *)arg;              \
    api_func                func = data->func;                      \
                                                                    \
    tarpc_##_func##_in     *in = &(data->in);                       \
    tarpc_##_func##_out    *out = &(data->out);                     \
                                                                    \
    checked_arg            *list = NULL;                            \
    checked_arg           **list_ptr = &list;                       \
                                                                    \
    UNUSED(list_ptr); /* Possibly unused */                         \
    UNUSED(func); /* Possibly unused */                             \
                                                                    \
    logfork_register_user(#_func);                                  \
                                                                    \
    VERB("Entry thread %s", #_func);                                \
                                                                    \
    sigprocmask(SIG_SETMASK, &(data->mask), NULL);                  \
                                                                    \
    { _actions }                                                    \
                                                                    \
    data->done = TRUE;                                              \
                                                                    \
    return arg;                                                     \
}                                                                   \
                                                                    \
bool_t                                                              \
_##_func##_1_svc(tarpc_##_func##_in *in, tarpc_##_func##_out *out,  \
                 struct svc_req *rqstp)                             \
{                                                                   \
    api_func        func;                                           \
    _func##_arg    *arg;                                            \
                                                                    \
    UNUSED(rqstp);                                                  \
                                                                    \
    memset(out, 0, sizeof(*out));                                   \
                                                                    \
    VERB("PID=%d TID=%d: Entry %s",                                 \
         (int)getpid(), (int)pthread_self(), #_func);               \
                                                                    \
    FIND_FUNC(in->common.use_libc, #_func, func);                   \
                                                                    \
    { _copy_args }                                                  \
                                                                    \
    switch (in->common.op)                                          \
    {                                                               \
        case RCF_RPC_CALL_WAIT:                                     \
        {                                                           \
            checked_arg    *list = NULL;                            \
            checked_arg   **list_ptr = &list;                       \
                                                                    \
            UNUSED(list_ptr); /* Possibly unused */                 \
                                                                    \
            VERB("%s(): CALL-WAIT", #_func);                        \
                                                                    \
            { _actions  }                                           \
                                                                    \
            break;                                                  \
        }                                                           \
                                                                    \
        case RCF_RPC_CALL:                                          \
        {                                                           \
            pthread_t _tid;                                         \
                                                                    \
            VERB("%s(): CALL", #_func);                             \
                                                                    \
            if ((arg = calloc(1, sizeof(*arg))) == NULL)            \
            {                                                       \
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);  \
                break;                                              \
            }                                                       \
                                                                    \
            arg->in   = *in;                                        \
            arg->out  = *out;                                       \
            arg->func = func;                                       \
            sigprocmask(SIG_SETMASK, NULL, &(arg->mask));           \
            arg->done = FALSE;                                      \
                                                                    \
            if (pthread_create(&_tid, NULL, _func##_proc, arg) != 0)\
            {                                                       \
                free(arg);                                          \
                out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);   \
                break;                                              \
            }                                                       \
                                                                    \
            /*                                                      \
             * Preset 'in' and 'out' with zeros to avoid any        \
             * resource deallocations by the caller.                \
             * 'out' is preset with zeros above, but may be         \
             * modified in '_copy_args'.                            \
             */                                                     \
            memset(in,  0, sizeof(*in));                            \
            memset(out, 0, sizeof(*out));                           \
                                                                    \
            out->common.tid = (tarpc_pthread_t)_tid;                \
            out->common.done = rcf_pch_mem_alloc(&arg->done);       \
                                                                    \
            break;                                                  \
        }                                                           \
                                                                    \
        case RCF_RPC_WAIT:                                          \
        {                                                           \
            pthread_t   _tid =                                      \
                (pthread_t) in->common.tid;                         \
            enum xdr_op op;                                         \
                                                                    \
            VERB("%s(): WAIT", #_func);                             \
                                                                    \
            /*                                                      \
             * If WAIT is called, all resources are deallocated     \
             * in any case.                                         \
             */                                                     \
            rcf_pch_mem_free(in->common.done);                      \
                                                                    \
            if (_tid == (pthread_t)(long)NULL)                      \
            {                                                       \
                ERROR("No thread with ID %llu to wait",             \
                      (unsigned long long int)in->common.tid);      \
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOENT);  \
                break;                                              \
            }                                                       \
            if (pthread_join(_tid, (void **)&(arg)) != 0)           \
            {                                                       \
                ERROR("pthread_join() failed");                     \
                out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);   \
                break;                                              \
            }                                                       \
            if (arg == NULL)                                        \
            {                                                       \
                ERROR("pthread_join() returned invalid thread "     \
                      "return value");                              \
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);  \
                break;                                              \
            }                                                       \
                                                                    \
            /* Free locations copied in 'out' by _copy_args' */     \
            op = XDR_FREE;                                          \
            if (!xdr_tarpc_##_func##_out((XDR *)&op, out))          \
                ERROR("xdr_tarpc_" #_func "_out() failed");         \
                                                                    \
            /* Copy output prepared in the thread */                \
            *out = arg->out;                                        \
            free(arg);                                              \
                                                                    \
            break;                                                  \
        }                                                           \
                                                                    \
        default:                                                    \
            ERROR("Unknown RPC operation");                         \
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);      \
            break;                                                  \
    }                                                               \
    return TRUE;                                                    \
}

typedef void (*sighandler_t)(int);

#endif /* __TARPC_SERVER_H__ */
