/** @file
 * @brief Linux Test Agent
 *
 * RPC server implementation for Berkeley Socket API RPCs
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#if HAVE_NETINET_IN_SYSTM_H /* Required for FreeBSD build */
#include <netinet/in_systm.h>
#endif
#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <dlfcn.h>
#if HAVE_AIO_H
#include <aio.h>
#endif
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "tarpc.h"
#include "rcf_ch_api.h"
#include "rcf_rpc_defs.h"
#include "linux_rpc.h"
#include "ta_rpc_log.h"
#include "tapi_rpcsock_defs.h"

#include "linux_internal.h"

#if HAVE_AIO_H
void *dummy = aio_read;
#endif

extern sigset_t rpcs_received_signals;

typedef int (*sock_api_func)(int param,...);

/**
 * Convert shutdown parameter from RPC to native representation.
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
 * Convert RPC sockaddr to struct sockaddr.
 *
 * @param rpc_addr      RPC address location
 * @param addr          pointer to struct sockaddr_storage
 *
 * @return struct sockaddr pointer
 */
static inline struct sockaddr *
sockaddr_rpc2h(struct tarpc_sa *rpc_addr, struct sockaddr_storage *addr)
{
    uint32_t len = SA_DATA_MAX_LEN;

    if (rpc_addr->sa_data.sa_data_len == 0)
        return NULL;

    memset(addr, 0, sizeof(struct sockaddr_storage));

    addr->ss_family = addr_family_rpc2h(rpc_addr->sa_family);

    if (len < rpc_addr->sa_data.sa_data_len)
    {
#if 0
        WARN("Strange tarpc_sa length %d is received",
             rpc_addr->sa_data.sa_data_len);
#endif
    }
    else
        len = rpc_addr->sa_data.sa_data_len;

    memcpy(((struct sockaddr *)addr)->sa_data,
           rpc_addr->sa_data.sa_data_val, len);

    return (struct sockaddr *)addr;
}

/**
 * Convert RPC sockaddr to struct sockaddr. It's assumed that
 * memory allocated for RPC address has maximum possible length (i.e
 * SA_DATA_MAX_LEN).
 *
 * @param addr          pointer to struct sockaddr_storage
 * @param rpc_addr      RPC address location
 *
 * @return struct sockaddr pointer
 */
static inline void
sockaddr_h2rpc(struct sockaddr *addr, struct tarpc_sa *rpc_addr)
{
    if (addr == NULL || rpc_addr->sa_data.sa_data_val == NULL)
        return;

    rpc_addr->sa_family = addr_family_h2rpc(addr->sa_family);
    if (rpc_addr->sa_data.sa_data_val != NULL)
    {
        memcpy(rpc_addr->sa_data.sa_data_val, addr->sa_data,
               rpc_addr->sa_data.sa_data_len);
    }
}

static te_bool dynamic_library_ok = FALSE;
static char *dynamic_library_name = NULL;
static void *dynamic_library_handle = NULL;

/**
 * Find the function by its name.
 *
 * @param in    for logging
 * @param name  function name
 * @param func  location for function address
 *
 * @return status code
 */
static int
find_func(tarpc_in_arg *in, char *name, sock_api_func *func)
{
    static void *libc_handle = NULL;

    if (!dynamic_library_ok)
    {
        ERROR("Invalid dynamic library handle");
        return TE_RC(TE_TA_LINUX, EINVAL);
    }
    if (libc_handle == NULL)
    {
        if ((libc_handle = dlopen(NULL, RTLD_LAZY)) == NULL)
        {
            ERROR("dlopen() failed for myself: %s", dlerror());
            return TE_RC(TE_TA_LINUX, ENOENT);
        }
    }

    if ((dynamic_library_ok &&
         (*func = dlsym(dynamic_library_handle, name)) == NULL) &&
        (*func = dlsym(libc_handle, name)) == NULL)
    {
        VERB("Cannot resolve symbol %s in libraries: %s", name, dlerror());
        if ((*func = rcf_ch_symbol_addr(name, 1)) == NULL)
        {
            ERROR("Cannot resolve symbol %s", name);
            return TE_RC(TE_TA_LINUX, ENOENT);
        }
    }
    return 0;
}

/** Structure for checking of variable-length arguments safity */
typedef struct checked_arg {
    struct checked_arg *next; /**< Next checked argument in the list */

    char *real_arg;     /**< Pointer to real buffer */
    char *control;      /**< Pointer to control buffer */
    int   len;          /**< Whole length of the buffer */
    int   len_visible;  /**< Length passed to the function under test */
} checked_arg;

/** Initialise the checked argument and add it into the list */
static void
init_checked_arg(tarpc_in_arg *in,
                 checked_arg **list, char *real_arg,
                 int len, int len_visible)
{
    checked_arg *arg;

    if (real_arg == NULL || len <= len_visible)
        return;

    if ((arg = malloc(sizeof(*arg))) == NULL)
    {
        ERROR("No enough memory");
        return;
    }

    if ((arg->control = malloc(len - len_visible)) == NULL)
    {
        ERROR("No enough memory");
        free(arg);
        return;
    }
    memcpy(arg->control, real_arg + len_visible, len - len_visible);
    arg->real_arg = real_arg;
    arg->len = len;
    arg->len_visible = len_visible;
    arg->next = *list;
    *list = arg;
}

#define INIT_CHECKED_ARG(_real_arg, _len, _len_visible) \
    init_checked_arg((tarpc_in_arg *)in, &list, _real_arg, \
                     _len, _len_visible)

/** Verify that arguments are not corrupted */
static int
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
            rc = TE_RC(TE_TA_LINUX, ETECORRUPTED);
        }
        free(cur->control);
        free(cur);
    }

    return rc;
}

/** Convert address and register it in the list of checked arguments */
#define PREPARE_ADDR(_address, _vlen)                            \
    struct sockaddr_storage addr;                                \
    struct sockaddr        *a;                                   \
                                                                 \
    a = sockaddr_rpc2h(&(_address), &addr);                      \
    INIT_CHECKED_ARG((char *)a, (_address).sa_data.sa_data_len + \
                     SA_COMMON_LEN, _vlen);

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
    do {                                   \
        out->_a = in->_a;                  \
        in->_a.sa_data.sa_data_len = 0;    \
        in->_a.sa_data.sa_data_val = NULL; \
    } while (0)

/**
 * Find function by its name with check.
 * out variable is assumed in the context.
 */
#define FIND_FUNC(_name, _func) \
    do {                                         \
        int rc = find_func((tarpc_in_arg *)in,   \
                           _name, &(_func));     \
                                                 \
        if (rc != 0)                             \
        {                                        \
             out->common._errno = rc;            \
             return TRUE;                        \
        }                                        \
    } while (0)

/** Wait time specified in the input argument. */
#define WAIT_START(high, low)                                   \
    do {                                                        \
        struct timeval t;                                       \
                                                                \
        uint64_t msec_start = (((uint64_t)high) << 32) + (low); \
        uint64_t msec_now;                                      \
                                                                \
        gettimeofday(&t, NULL);                                 \
        msec_now = t.tv_sec * 1000 + t.tv_usec / 1000;          \
                                                                \
        if (msec_start > msec_now)                              \
            usleep((msec_start - msec_now) * 1000);             \
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
        int           _rc;                                       \
                                                                 \
        WAIT_START(in->common.start_high, in->common.start_low); \
        gettimeofday(&t_start, NULL);                            \
        errno = 0;                                               \
        x;                                                       \
        out->common._errno = RPC_ERRNO;                          \
        gettimeofday(&t_finish, NULL);                           \
        out->common.duration =                                   \
            (t_finish.tv_sec - t_start.tv_sec) * 1000000 +       \
            t_finish.tv_usec - t_start.tv_usec;                  \
        _rc = check_args(list);                                  \
        if (out->common._errno == 0 && _rc != 0)                 \
            out->common._errno = _rc;                            \
    } while (0)

#define TARPC_FUNC(_func, _copy_args, _actions)                         \
                                                                        \
typedef struct _func##_arg {                                            \
    sock_api_func       func;                                           \
    tarpc_##_func##_in  in;                                             \
    tarpc_##_func##_out out;                                            \
    sigset_t            mask;                                           \
} _func##_arg;                                                          \
                                                                        \
static void *                                                           \
_func##_proc(void *arg)                                                 \
{                                                                       \
    sock_api_func       func = ((_func##_arg *)arg)->func;              \
    tarpc_##_func##_in  *in = &(((_func##_arg *)arg)->in);              \
    tarpc_##_func##_out *out = &(((_func##_arg *)arg)->out);            \
    checked_arg         *list = NULL;                                   \
                                                                        \
    VERB("Entry thread %s", #_func);                                    \
                                                                        \
    sigprocmask(SIG_SETMASK, &(((_func##_arg *)arg)->mask), NULL);      \
                                                                        \
    { _actions }                                                        \
                                                                        \
    return arg;                                                         \
}                                                                       \
                                                                        \
bool_t                                                                  \
_##_func##_1_svc(tarpc_##_func##_in *in, tarpc_##_func##_out *out,      \
                 struct svc_req *rqstp)                                 \
{                                                                       \
    sock_api_func func;                                                 \
    checked_arg  *list = NULL;                                          \
    _func##_arg  *arg;                                                  \
    enum xdr_op  op = XDR_FREE;                                         \
                                                                        \
    UNUSED(rqstp);                                                      \
    memset(out, 0, sizeof(*out));                                       \
    VERB("PID=%d TID=%d: Entry %s",                                     \
         (int)getpid(), (int)pthread_self(), #_func);                   \
    FIND_FUNC(#_func, func);                                            \
                                                                        \
    _copy_args                                                          \
                                                                        \
    if (in->common.op == TARPC_CALL_WAIT)                               \
    {                                                                   \
        _actions                                                        \
        return TRUE;                                                    \
    }                                                                   \
                                                                        \
    if (in->common.op == TARPC_CALL)                                    \
    {                                                                   \
        pthread_t _tid;                                                 \
                                                                        \
        if ((arg = malloc(sizeof(*arg))) == NULL)                       \
        {                                                               \
            out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);            \
            return TRUE;                                                \
        }                                                               \
                                                                        \
        arg->in = *in;                                                  \
        arg->out = *out;                                                \
        arg->func = func;                                               \
        sigprocmask(SIG_SETMASK, NULL, &(arg->mask));                   \
                                                                        \
        if (pthread_create(&_tid, NULL, _func##_proc,                   \
                           (void *)arg) != 0)                           \
        {                                                               \
            free(arg);                                                  \
            out->common._errno = TE_RC(TE_TA_LINUX, errno);             \
        }                                                               \
                                                                        \
        memset(in, 0, sizeof(*in));                                     \
        memset(out, 0, sizeof(*out));                                   \
        out->common.tid = _tid;                                         \
                                                                        \
        return TRUE;                                                    \
    }                                                                   \
                                                                        \
    if (pthread_join(in->common.tid, (void **)&(arg)) != 0)             \
    {                                                                   \
        out->common._errno = TE_RC(TE_TA_LINUX, errno);                 \
        return TRUE;                                                    \
    }                                                                   \
    if (arg == NULL)                                                    \
    {                                                                   \
        out->common._errno = TE_RC(TE_TA_LINUX, EINVAL);                \
        return TRUE;                                                    \
    }                                                                   \
    xdr_tarpc_##_func##_out((XDR *)&op, out);                           \
    *out = arg->out;                                                    \
    free(arg);                                                          \
    return TRUE;                                                        \
}


/*-------------- setlibname() -----------------------------*/

/**
 * The routine called via RCF to set the name of socket library.
 */
int
setlibname(const tarpc_setlibname_in *in)
{
    const char *libname;

    if (dynamic_library_ok)
    {
        ERROR("Dynamic library has already been set to %s",
              dynamic_library_name);
        return TE_RC(TE_TA_LINUX, EEXIST);
    }
    libname = (in->libname.libname_len == 0) ?
                  NULL : in->libname.libname_val;
    if ((dynamic_library_handle = dlopen(libname, RTLD_LAZY)) == NULL)
    {
        ERROR("Cannot load shared library %s: %s", libname, dlerror());
        return TE_RC(TE_TA_LINUX, ENOENT);
    }
    dynamic_library_name = (libname != NULL) ? strdup(libname) : "(NULL)";
    if (dynamic_library_name == NULL)
    {
        ERROR("strdup(%s) failed", libname ? : "(nil)");
        return TE_RC(TE_TA_LINUX, ENOMEM);
    }
    dynamic_library_ok = TRUE;
    return 0;
}

bool_t
_setlibname_1_svc(tarpc_setlibname_in *in, tarpc_setlibname_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    VERB("PID=%d TID=%d: Entry %s",
         (int)getpid(), (int)pthread_self(), "setlibname");

    errno = 0;
    out->retval = setlibname(in);
    out->common._errno = RPC_ERRNO;
    out->common.duration = 0;
    return TRUE;
}

/*-------------- fork() ---------------------------------*/
TARPC_FUNC(fork, {},
{
    MAKE_CALL(out->pid = func(0));

    if (out->pid == 0)
    {
#ifdef HAVE_SVC_EXIT
        svc_exit();
#endif
        tarpc_server(in->name);
        exit(EXIT_FAILURE);
    }
}
)

/*-------------- pthread_create() -----------------------------*/
TARPC_FUNC(pthread_create, {},
{
    MAKE_CALL(out->retval = func((int)&(out->tid), NULL, tarpc_server,
                                 strdup(in->name)));
}
)

/*-------------- pthread_cancel() -----------------------------*/
TARPC_FUNC(pthread_cancel, {}, { MAKE_CALL(out->retval = func(in->tid)); })

/** Function to start RPC server after execve */
void
tarpc_init(int argc, char **argv)
{
    const char *name = argv[2];
    const char *pid = argv[3];
    const char *log_addr = argv[4];
    const char *libname = argv[5];

    tarpc_setlibname_in  in_local;
    tarpc_setlibname_in *in = &in_local;

    memset(&in_local, 0, sizeof(in_local));

    UNUSED(argc);
    ta_pid = atoi(pid);
    memset(&ta_log_addr, 0, sizeof(ta_log_addr));
    ta_log_addr.sun_family = AF_UNIX;
    strcpy(ta_log_addr.sun_path + 1, log_addr);
    ta_log_addr_s = (struct sockaddr *)&ta_log_addr;

    /* Emulate setlibname() call */
    if (name == NULL || (strlen(name) >= sizeof(in->common.name)))
    {
        ERROR("Invalid RPC server name");
        return;
    }
    strcpy(in->common.name, name);
    if (libname == NULL)
    {
        ERROR("Invalid dynamic library name");
        return;
    }
    if (strcmp(libname, "(NULL)") == 0)
    {
        in->libname.libname_len = 0;
        in->libname.libname_val = NULL;
    }
    else
    {
        in->libname.libname_len = strlen(libname);
        in->libname.libname_val = (char *)libname;
    }
    setlibname(in);

    tarpc_server(name);
}

/**
 * Check, if some signals were received by the RPC server (as a process)
 * and return the mask of received signals.
 */

bool_t
_sigreceived_1_svc(tarpc_sigreceived_in *in, tarpc_sigreceived_out *out,
                   struct svc_req *rqstp)
{
    UNUSED(in);
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    out->set = (tarpc_sigset_t)&rpcs_received_signals;

    return TRUE;
}


/*-------------- execve() ---------------------------------*/
TARPC_FUNC(execve, {},
{
    const char *args[7];
    static char buf[16];

    args[0] = ta_execname;
    args[1] = "rpcserver";
    args[2] = rpcserver_name;
    sprintf(buf, "%u", ta_pid);
    args[3] = buf;
    args[4] = ta_log_addr.sun_path + 1;
    args[5] = dynamic_library_name;
    args[6] = NULL;

    sleep(1);
    close(rpcserver_sock);
    MAKE_CALL(func((int)ta_execname, args, NULL));
}
)

/*-------------- getpid() --------------------------------*/
TARPC_FUNC(getpid, {}, { MAKE_CALL(out->retval = func(0)); })


/*-------------- socket() ------------------------------*/

TARPC_FUNC(socket, {},
{
    MAKE_CALL(out->fd = func(domain_rpc2h(in->domain),
                             socktype_rpc2h(in->type),
                             proto_rpc2h(in->proto)));
}
)

/*-------------- dup() --------------------------------*/

TARPC_FUNC(dup, {}, { MAKE_CALL(out->fd = func(in->oldfd)); })

/*-------------- dup2() -------------------------------*/

TARPC_FUNC(dup2, {}, { MAKE_CALL(out->fd = func(in->oldfd, in->newfd)); })

/*-------------- close() ------------------------------*/

TARPC_FUNC(close, {}, { MAKE_CALL(out->retval = func(in->fd)); })

/*-------------- bind() ------------------------------*/

TARPC_FUNC(bind, {},
{
    PREPARE_ADDR(in->addr, 0);
    MAKE_CALL(out->retval = func(in->fd, a, in->len));
}
)

/*-------------- connect() ------------------------------*/

TARPC_FUNC(connect, {},
{
    PREPARE_ADDR(in->addr, 0);

    MAKE_CALL(out->retval = func(in->fd, a, in->len));
}
)

/*-------------- listen() ------------------------------*/

TARPC_FUNC(listen, {},
{
    MAKE_CALL(out->retval = func(in->fd, in->backlog));
}
)

/*-------------- accept() ------------------------------*/

TARPC_FUNC(accept,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, a,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- recvfrom() ------------------------------*/


TARPC_FUNC(recvfrom,
{
    COPY_ARG(buf);
    COPY_ARG(fromlen);
    COPY_ARG_ADDR(from);
},
{
    PREPARE_ADDR(out->from, out->fromlen.fromlen_len == 0 ? 0 :
                            *out->fromlen.fromlen_val);


    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, out->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags), a,
                                 out->fromlen.fromlen_len == 0 ? NULL :
                                 out->fromlen.fromlen_val));
    sockaddr_h2rpc(a, &(out->from));
}
)


/*-------------- recv() ------------------------------*/

TARPC_FUNC(recv,
{
    COPY_ARG(buf);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, out->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
}
)

/*-------------- shutdown() ------------------------------*/

TARPC_FUNC(shutdown, {},
{
    MAKE_CALL(out->retval = func(in->fd, shut_how_rpc2h(in->how)));
}
)

/*-------------- sendto() ------------------------------*/

TARPC_FUNC(sendto, {},
{
    PREPARE_ADDR(in->to, 0);

    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags),
                                 a, in->tolen));
}
)

/*-------------- send() ------------------------------*/

TARPC_FUNC(send, {},
{
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
}
)

/*-------------- read() ------------------------------*/

TARPC_FUNC(read,
{
    COPY_ARG(buf);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, out->buf.buf_val, in->len));
}
)

/*-------------- write() ------------------------------*/

TARPC_FUNC(write, {},
{
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len));
}
)

/*-------------- readv() ------------------------------*/

TARPC_FUNC(readv,
{
    if (out->vector.vector_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        return TRUE;
    }
    COPY_ARG(vector);
},
{
    struct iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    unsigned int i;

    memset(iovec_arr, 0, sizeof(iovec_arr));
    for (i = 0; i < out->vector.vector_len; i++)
    {
        INIT_CHECKED_ARG(out->vector.vector_val[i].iov_base.iov_base_val,
                         out->vector.vector_val[i].iov_base.iov_base_len,
                         out->vector.vector_val[i].iov_len);
        iovec_arr[i].iov_base =
            out->vector.vector_val[i].iov_base.iov_base_val;
        iovec_arr[i].iov_len = out->vector.vector_val[i].iov_len;
    }
    INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);

    MAKE_CALL(out->retval = func(in->fd, iovec_arr, in->count));

    for (i = 0; i < out->vector.vector_len; i++)
    {
        out->vector.vector_val[i].iov_len = iovec_arr[i].iov_len;
    }
}
)

/*-------------- writev() ------------------------------*/

TARPC_FUNC(writev,
{
    if (in->vector.vector_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        return TRUE;
    }
},
{
    struct iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    unsigned int i;

    memset(iovec_arr, 0, sizeof(iovec_arr));
    for (i = 0; i < in->vector.vector_len; i++)
    {
        INIT_CHECKED_ARG(in->vector.vector_val[i].iov_base.iov_base_val,
                         in->vector.vector_val[i].iov_base.iov_base_len, 0);
        iovec_arr[i].iov_base =
            in->vector.vector_val[i].iov_base.iov_base_val;
        iovec_arr[i].iov_len = in->vector.vector_val[i].iov_len;
    }
    INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);

    MAKE_CALL(out->retval = func(in->fd, iovec_arr, in->count));
}
)

/*-------------- getsockname() ------------------------------*/
TARPC_FUNC(getsockname,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, a,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- getpeername() ------------------------------*/

TARPC_FUNC(getpeername,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, a,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- fd_set constructor ----------------------------*/

bool_t
_fd_set_new_1_svc(tarpc_fd_set_new_in *in, tarpc_fd_set_new_out *out,
                  struct svc_req *rqstp)
{
    fd_set *set;

    UNUSED(rqstp);
    UNUSED(in);

    memset(out, 0, sizeof(*out));

    errno = 0;
    if ((set = (fd_set *)malloc(sizeof(fd_set))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
    }
    else
    {
        out->common._errno = RPC_ERRNO;
        out->retval = (tarpc_fd_set)set;
    }

    return TRUE;
}

/*-------------- fd_set destructor ----------------------------*/

bool_t
_fd_set_delete_1_svc(tarpc_fd_set_delete_in *in,
                     tarpc_fd_set_delete_out *out,
                     struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    errno = 0;
    free((void *)(in->set));
    out->common._errno = RPC_ERRNO;

    return TRUE;
}

/*-------------- FD_ZERO --------------------------------*/

bool_t
_do_fd_zero_1_svc(tarpc_do_fd_zero_in *in, tarpc_do_fd_zero_out *out,
                  struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_ZERO((fd_set *)(in->set));
    return TRUE;
}

/*-------------- FD_SET --------------------------------*/

bool_t
_do_fd_set_1_svc(tarpc_do_fd_set_in *in, tarpc_do_fd_set_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_SET(in->fd, (fd_set *)(in->set));
    return TRUE;
}

/*-------------- FD_CLR --------------------------------*/

bool_t
_do_fd_clr_1_svc(tarpc_do_fd_clr_in *in, tarpc_do_fd_clr_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_SET(in->fd, (fd_set *)(in->set));
    return TRUE;
}

/*-------------- FD_ISSET --------------------------------*/

bool_t
_do_fd_isset_1_svc(tarpc_do_fd_isset_in *in, tarpc_do_fd_isset_out *out,
                   struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    out->retval = FD_ISSET(in->fd, (fd_set *)(in->set));
    return TRUE;
}

/*-------------- select() --------------------------------*/

TARPC_FUNC(select,
{
    COPY_ARG(timeout);
},
{
    struct timeval tv;

    if (out->timeout.timeout_len > 0)
    {
        tv.tv_sec = out->timeout.timeout_val[0].tv_sec;
        tv.tv_usec = out->timeout.timeout_val[0].tv_usec;
    }

    MAKE_CALL(out->retval = func(in->n, (fd_set *)(in->readfds),
                                 (fd_set *)(in->writefds),
                                 (fd_set *)(in->exceptfds),
                                 out->timeout.timeout_len == 0 ? NULL :
                                 &tv));

    if (out->timeout.timeout_len > 0)
    {
        out->timeout.timeout_val[0].tv_sec = tv.tv_sec;
        out->timeout.timeout_val[0].tv_usec = tv.tv_usec;
    }
}
)

/*-------------- if_nametoindex() --------------------------------*/

TARPC_FUNC(if_nametoindex, {},
{
    INIT_CHECKED_ARG(in->ifname.ifname_val, in->ifname.ifname_len, 0);
    MAKE_CALL(out->ifindex = func((int)in->ifname.ifname_val));
}
)

/*-------------- if_nametoindex() --------------------------------*/

TARPC_FUNC(if_indextoname,
{
    COPY_ARG(ifname);
},
{
    char *name;

    memcmp(name, out->ifname.ifname_val, out->ifname.ifname_len);

    MAKE_CALL(name = (char *)func(in->ifindex, out->ifname.ifname_val));

    if (name != NULL && name != out->ifname.ifname_val)
    {
        ERROR("if_indextoname returned incorrect pointer");
        out->common._errno = TE_RC(TE_TA_LINUX, ETECORRUPTED);
    }

    if (name == NULL &&
        memcmp(name, out->ifname.ifname_val, out->ifname.ifname_len) != 0)
    {
        out->common._errno = TE_RC(TE_TA_LINUX, ETECORRUPTED);
    }
}
)


/*-------------- if_nameindex() ------------------------------*/

TARPC_FUNC(if_nameindex, {},
{
    struct if_nameindex *ret;
    tarpc_if_nameindex  *arr = NULL;

    int i = 0;

    MAKE_CALL(out->mem_ptr = (unsigned int)func(0));

    ret = (struct if_nameindex *)(out->mem_ptr);

    if (ret != NULL)
    {
        int j;

        while (ret[i++].if_index != 0);
        arr = (tarpc_if_nameindex *)calloc(sizeof(*arr) * i, 1);
        if (arr == NULL)
        {
            out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        }
        else
        {
            for (j = 0; j < i - 1; j++)
            {
                arr[j].ifindex = ret[j].if_index;
                arr[j].ifname.ifname_val = strdup(ret[j].if_name);
                if (arr[j].ifname.ifname_val == NULL)
                {
                    for (j--; j >= 0; j--)
                        free(arr[j].ifname.ifname_val);
                    free(arr);
                    out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
                    arr = NULL;
                    i = 0;
                    break;
                }
                arr[j].ifname.ifname_len = strlen(ret[j].if_name) + 1;
            }
        }
    }
    out->ptr.ptr_val = arr;
    out->ptr.ptr_len = i;
}
)

/*-------------- if_freenameindex() ----------------------------*/

TARPC_FUNC(if_freenameindex, {},
{
    MAKE_CALL(func(in->mem_ptr));
}
)

/*-------------- sigset_t constructor ---------------------------*/

bool_t
_sigset_new_1_svc(tarpc_sigset_new_in *in, tarpc_sigset_new_out *out,
                  struct svc_req *rqstp)
{
    sigset_t *set;

    UNUSED(rqstp);
    UNUSED(in);

    memset(out, 0, sizeof(*out));

    errno = 0;
    if ((set = (sigset_t *)malloc(sizeof(sigset_t))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
    }
    else
    {
        out->common._errno = RPC_ERRNO;
        out->set = (tarpc_sigset_t)set;
    }

    return TRUE;
}

/*-------------- sigset_t destructor ----------------------------*/

bool_t
_sigset_delete_1_svc(tarpc_sigset_delete_in *in,
                     tarpc_sigset_delete_out *out,
                     struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    errno = 0;
    free((void *)(in->set));
    out->common._errno = RPC_ERRNO;

    return TRUE;
}

/*-------------- sigemptyset() ------------------------------*/

TARPC_FUNC(sigemptyset, {},
{
    MAKE_CALL(out->retval = func(in->set));
}
)

/*-------------- sigpendingt() ------------------------------*/

TARPC_FUNC(sigpending, {},
{
    MAKE_CALL(out->retval = func(in->set));
}
)

/*-------------- sigsuspend() ------------------------------*/

TARPC_FUNC(sigsuspend, {},
{
    MAKE_CALL(out->retval = func(in->set));
}
)

/*-------------- sigfillset() ------------------------------*/

TARPC_FUNC(sigfillset, {},
{
    MAKE_CALL(out->retval = func(in->set));
}
)

/*-------------- sigaddset() -------------------------------*/

TARPC_FUNC(sigaddset, {},
{
    MAKE_CALL(out->retval = func(in->set, signum_rpc2h(in->signum)));
}
)

/*-------------- sigdelset() -------------------------------*/

TARPC_FUNC(sigdelset, {},
{
    MAKE_CALL(out->retval = func(in->set, signum_rpc2h(in->signum)));
}
)

/*-------------- sigismember() ------------------------------*/

TARPC_FUNC(sigismember, {},
{
    INIT_CHECKED_ARG((char *)(in->set), sizeof(sigset_t), 0);
    MAKE_CALL(out->retval = func(in->set, signum_rpc2h(in->signum)));
}
)

/*-------------- sigprocmask() ------------------------------*/
TARPC_FUNC(sigprocmask, {},
{
    INIT_CHECKED_ARG((char *)in->set, sizeof(sigset_t), 0);
    MAKE_CALL(out->retval = func(sighow_rpc2h(in->how), in->set,
                                 in->oldset));
}
)

/*-------------- kill() --------------------------------*/

TARPC_FUNC(kill, {},
{
    MAKE_CALL(out->retval = func(in->pid, signum_rpc2h(in->signum)));
}
)

/*-------------- signal() --------------------------------*/

typedef void (*sighandler_t)(int);

TARPC_FUNC(signal,
{
    if (in->signum == RPC_SIGINT)
    {
        out->common._errno = TE_RC(TE_TA_LINUX, EPERM);
        return TRUE;
    }
},
{
    sighandler_t handler = rcf_ch_symbol_addr(in->handler.handler_val, 1);
    sighandler_t old_handler;

    if (handler == NULL && in->handler.handler_val != NULL)
    {
        char *tmp;

        handler = (sighandler_t)strtol(in->handler.handler_val, &tmp, 16);

        if (tmp == in->handler.handler_val || *tmp != 0)
            out->common._errno = TE_RC(TE_TA_LINUX, EINVAL);
    }
    if (out->common._errno == 0)
    {
        MAKE_CALL(old_handler = (sighandler_t)func(
                      signum_rpc2h(in->signum), handler));
        if (old_handler != SIG_ERR)
        {
            char *name = rcf_ch_symbol_name(old_handler);

            if (name != NULL)
            {
                if ((out->handler.handler_val = strdup(name)) == NULL)
                {
                    func(signum_rpc2h(in->signum), old_handler);
                    out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
                }
                else
                    out->handler.handler_len = strlen(name) + 1;
            }
            else
            {
                if ((name = calloc(1, 16)) == NULL)
                {
                    func(signum_rpc2h(in->signum), old_handler);
                    out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
                }
                else
                {
                    sprintf(name, "0x%x", (unsigned int)old_handler);
                    out->handler.handler_val = name;
                    out->handler.handler_len = strlen(name) + 1;
                }
            }
        }
    }
}
)

/*-------------- setsockopt() ------------------------------*/

TARPC_FUNC(setsockopt, {},
{
    if (in->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     NULL, in->optlen));
    }
    else
    {
        char *opt;
        int   optlen;

        struct linger   linger;
#ifdef __linux__
        struct ip_mreqn mreqn;
#endif
        struct in_addr  addr;
        struct timeval  tv;

        switch (in->optval.optval_val[0].opttype)
        {
            case OPT_INT:
            {
                opt = (char *)&(in->optval.optval_val[0].
                                option_value_u.opt_int);
                optlen = sizeof(int);
                break;
            }
            case OPT_LINGER:
            {
                opt = (char *)&linger;
                linger.l_onoff = in->optval.optval_val[0].
                                 option_value_u.opt_linger.l_onoff;
                linger.l_linger = in->optval.optval_val[0].
                                  option_value_u.opt_linger.l_linger;
                optlen = sizeof(linger);
                break;
            }
#ifdef __linux__
            case OPT_MREQN:
            {
                opt = (char *)&mreqn;

                memcpy((char *)&(mreqn.imr_multiaddr),
                       in->optval.optval_val[0].option_value_u.opt_mreqn.
                       imr_multiaddr, sizeof(mreqn.imr_multiaddr));
                memcpy((char *)&(mreqn.imr_address),
                       in->optval.optval_val[0].option_value_u.opt_mreqn.
                       imr_address, sizeof(mreqn.imr_address));

                mreqn.imr_ifindex = in->optval.optval_val[0].option_value_u.
                                    opt_mreqn.imr_ifindex;
                optlen = sizeof(mreqn);
                break;
            }
#endif

            case OPT_IPADDR:
            {
                opt = (char *)&addr;

                memcpy(&addr,
                       in->optval.optval_val[0].option_value_u.opt_ipaddr,
                       sizeof(struct in_addr));
                optlen = sizeof(addr);
                break;
            }

            case OPT_TIMEVAL:
            {
                opt = (char *)&tv;

                tv.tv_sec = in->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_sec;
                tv.tv_usec = in->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_usec;
                optlen = sizeof(tv);
                break;
            }

            case OPT_STRING:
            {
                opt = (char *)in->optval.optval_val[0].option_value_u.
                    opt_string.opt_string_val;
                optlen = in->optval.optval_val[0].option_value_u.
                    opt_string.opt_string_len;
                break;
            }

            default:
                ERROR("incorrect option type %d is received",
                      in->optval.optval_val[0].opttype);
                opt = NULL;
                break;
        }
        INIT_CHECKED_ARG(opt, optlen, 0);
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     opt, in->optlen));
    }
}
)

/*-------------- getsockopt() ------------------------------*/

#define COPY_TCP_INFO_FIELD(_name) \
    do {                                                               \
        out->optval.optval_val[0].option_value_u.                      \
            opt_tcp_info._name = info->_name;                          \
    } while (0)

TARPC_FUNC(getsockopt,
{
    COPY_ARG(optval);
    COPY_ARG(optlen);
},
{
    if (out->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     NULL, out->optlen.optlen_val));
    }
    else
    {
        char opt[sizeof(struct linger)
#ifdef __linux__
                 + sizeof(struct ip_mreqn) + sizeof(struct tcp_info)
#endif
                 ];

        memset(opt, 0, sizeof(opt));
        INIT_CHECKED_ARG(opt, sizeof(opt),
                         (out->optlen.optlen_val == NULL) ?
                            0 : *out->optlen.optlen_val);
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     opt, out->optlen.optlen_val));
        switch (out->optval.optval_val[0].opttype)
        {
            case OPT_INT:
            {
                /*
                 * SO_ERROR socket option keeps the value of the last
                 * pending error occurred on the socket, so that we should
                 * convert its value to host independend representation,
                 * which is RPC errno
                 */
                if (in->level == RPC_SOL_SOCKET &&
                    in->optname == RPC_SO_ERROR)
                {
                    *(int *)opt = errno_h2rpc(*(int *)opt);
                }

                /*
                 * SO_TYPE and SO_STYLE socket option keeps the value of
                 * socket type they are called for, so that we should
                 * convert its value to host independend representation,
                 * which is RPC socket type
                 */
                if (in->level == RPC_SOL_SOCKET &&
                    in->optname == RPC_SO_TYPE)
                {
                    *(int *)opt = socktype_h2rpc(*(int *)opt);
                }
                out->optval.optval_val[0].option_value_u.opt_int =
                    *(int *)opt;
                break;
            }
            case OPT_LINGER:
            {
                struct linger *linger = (struct linger *)opt;

                out->optval.optval_val[0].option_value_u.
                    opt_linger.l_onoff = linger->l_onoff;
                out->optval.optval_val[0].option_value_u.
                    opt_linger.l_linger = linger->l_linger;
                break;
            }
#ifdef __linux__
            case OPT_MREQN:
            {
                struct ip_mreqn *mreqn = (struct ip_mreqn *)opt;

                memcpy(out->optval.optval_val[0].option_value_u.opt_mreqn.
                       imr_multiaddr, (char *)&(mreqn->imr_multiaddr),
                       sizeof(mreqn->imr_multiaddr));
                memcpy(out->optval.optval_val[0].option_value_u.opt_mreqn.
                       imr_address, (char *)&(mreqn->imr_address),
                       sizeof(mreqn->imr_address));
                out->optval.optval_val[0].option_value_u.opt_mreqn.
                    imr_ifindex = mreqn->imr_ifindex;
                break;
            }
#endif

            case OPT_IPADDR:
            {
                struct in_addr *addr = (struct in_addr *)opt;

                memcpy(out->optval.optval_val[0].option_value_u.opt_ipaddr,
                       addr, sizeof(*addr));
                break;
            }

            case OPT_TIMEVAL:
            {
                struct timeval *tv = (struct timeval *)opt;

                out->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_sec = tv->tv_sec;
                out->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_usec = tv->tv_usec;
                break;
            }

            case OPT_STRING:
            {
                char *str = (char *)opt;

                memcpy(out->optval.optval_val[0].option_value_u.opt_string.
                           opt_string_val,
                       str,
                       out->optval.optval_val[0].option_value_u.opt_string.
                           opt_string_len);
                break;
            }

#ifdef __linux__
            case OPT_TCP_INFO:
            {
                struct tcp_info *info = (struct tcp_info *)opt;

                COPY_TCP_INFO_FIELD(tcpi_state);
                COPY_TCP_INFO_FIELD(tcpi_ca_state);
                COPY_TCP_INFO_FIELD(tcpi_retransmits);
                COPY_TCP_INFO_FIELD(tcpi_probes);
                COPY_TCP_INFO_FIELD(tcpi_backoff);
                COPY_TCP_INFO_FIELD(tcpi_options);
                COPY_TCP_INFO_FIELD(tcpi_snd_wscale);
                COPY_TCP_INFO_FIELD(tcpi_rcv_wscale);
                COPY_TCP_INFO_FIELD(tcpi_rto);
                COPY_TCP_INFO_FIELD(tcpi_ato);
                COPY_TCP_INFO_FIELD(tcpi_snd_mss);
                COPY_TCP_INFO_FIELD(tcpi_rcv_mss);
                COPY_TCP_INFO_FIELD(tcpi_unacked);
                COPY_TCP_INFO_FIELD(tcpi_sacked);
                COPY_TCP_INFO_FIELD(tcpi_lost);
                COPY_TCP_INFO_FIELD(tcpi_retrans);
                COPY_TCP_INFO_FIELD(tcpi_fackets);
                COPY_TCP_INFO_FIELD(tcpi_last_data_sent);
                COPY_TCP_INFO_FIELD(tcpi_last_ack_sent);
                COPY_TCP_INFO_FIELD(tcpi_last_data_recv);
                COPY_TCP_INFO_FIELD(tcpi_last_ack_recv);
                COPY_TCP_INFO_FIELD(tcpi_pmtu);
                COPY_TCP_INFO_FIELD(tcpi_rcv_ssthresh);
                COPY_TCP_INFO_FIELD(tcpi_rtt);
                COPY_TCP_INFO_FIELD(tcpi_rttvar);
                COPY_TCP_INFO_FIELD(tcpi_snd_ssthresh);
                COPY_TCP_INFO_FIELD(tcpi_snd_cwnd);
                COPY_TCP_INFO_FIELD(tcpi_advmss);
                COPY_TCP_INFO_FIELD(tcpi_reordering);
                break;
            }
#endif

            default:
                ERROR("incorrect option type %d is received",
                      out->optval.optval_val[0].opttype);
                break;
        }
    }
}
)

#undef COPY_TCP_INFO_FIELD

/*-------------- pselect() --------------------------------*/

TARPC_FUNC(pselect, {},
{
    struct timespec tv;

    if (in->timeout.timeout_len > 0)
    {
        tv.tv_sec = in->timeout.timeout_val[0].tv_sec;
        tv.tv_nsec = in->timeout.timeout_val[0].tv_nsec;
    }

    INIT_CHECKED_ARG((char *)(in->sigmask), sizeof(sigset_t), 0);
    INIT_CHECKED_ARG((char *)&tv, sizeof(tv), 0);

    MAKE_CALL(out->retval = func(in->n, (fd_set *)(in->readfds),
                                 (fd_set *)(in->writefds),
                                 (fd_set *)(in->exceptfds),
                                 in->timeout.timeout_len == 0 ? NULL :
                                 &tv, in->sigmask));
}
)

TARPC_FUNC(fcntl, {},
{
    int arg = in->arg;

    if (in->cmd == RPC_F_GETFD || in->cmd == RPC_F_GETFL ||
        in->cmd == RPC_F_SETFL)
        arg = fcntl_flag_rpc2h(arg);

    if (in->arg != 0)
        MAKE_CALL(out->retval = func(in->fd, fcntl_rpc2h(in->cmd), arg));
    else
        MAKE_CALL(out->retval = func(in->fd, fcntl_rpc2h(in->cmd)));

    if (in->cmd == RPC_F_GETFD || in->cmd == RPC_F_GETFL ||
        in->cmd == RPC_F_SETFL)
        out->retval = fcntl_flag_h2rpc(out->retval);
}
)

TARPC_FUNC(ioctl,
{
    COPY_ARG(req);
},
{
    char *req = NULL;
    int   reqlen = 0;

    static struct timeval req_timeval;
    static int            req_int;
    static struct ifreq   req_ifreq;
    static struct ifconf  req_ifconf;
    static struct arpreq  req_arpreq;

    if (out->req.req_val != NULL)
    {
        switch(out->req.req_val[0].type)
        {
            case IOCTL_TIMEVAL:
            {
                req = (char *)&req_timeval;
                reqlen = sizeof(struct timeval);
                req_timeval.tv_sec = out->req.req_val[0].
                    ioctl_request_u.req_timeval.tv_sec;
                req_timeval.tv_usec = out->req.req_val[0].
                    ioctl_request_u.req_timeval.tv_usec;
                break;
            }

            case IOCTL_INT:
            {
                req = (char *)&req_int;
                req_int = out->req.req_val[0].ioctl_request_u.req_int;
                reqlen = sizeof(int);
                break;
            }

            case IOCTL_IFREQ:
            {
                req = (char *)&req_ifreq;
                reqlen = sizeof(struct ifreq);

                memset(req, 0, reqlen);
                /* Copy the whole 'ifr_name' buffer, not just strcpy() */
                memcpy(req_ifreq.ifr_name,
                       out->req.req_val[0].ioctl_request_u.req_ifreq.
                       rpc_ifr_name,
                       sizeof(req_ifreq.ifr_name));

                INIT_CHECKED_ARG(req_ifreq.ifr_name,
                                 strlen(req_ifreq.ifr_name) + 1, 0);

                switch (in->code)
                {
                    case RPC_SIOCSIFFLAGS:
                        req_ifreq.ifr_flags =
                            if_fl_rpc2h((uint32_t)(unsigned short int)
                                out->req.req_val[0].ioctl_request_u.
                                    req_ifreq.rpc_ifr_flags);
                        break;

                    case RPC_SIOCSIFMTU:
                        req_ifreq.ifr_mtu =
                            out->req.req_val[0].ioctl_request_u.
                            req_ifreq.rpc_ifr_mtu;
                        break;

                    case RPC_SIOCSIFADDR:
                    case RPC_SIOCSIFNETMASK:
                    case RPC_SIOCSIFBRDADDR:
                    case RPC_SIOCSIFDSTADDR:
                        sockaddr_rpc2h(&(out->req.req_val[0].
                            ioctl_request_u.req_ifreq.rpc_ifr_addr),
                            (struct sockaddr_storage *)
                                (&(req_ifreq.ifr_addr)));
                       break;
                }
                break;
            }

            case IOCTL_IFCONF:
            {
                char *buf = NULL;
                int   buflen = out->req.req_val[0].ioctl_request_u.
                               req_ifconf.buflen;

                req = (char *)&req_ifconf;
                reqlen = sizeof(req_ifconf);

                if (buflen > 0 && (buf = calloc(1, buflen + 10)) == NULL)
                {
                    ERROR("No enough memory");
                    out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
                    goto finish;
                }
                req_ifconf.ifc_buf = buf;
                req_ifconf.ifc_len = buflen;
                if (buf != NULL)
                    INIT_CHECKED_ARG(buf, buflen + 10, buflen);
                break;
            }
            case IOCTL_ARPREQ:
            {
                req = (char *)&req_arpreq;
                reqlen = sizeof(req_arpreq);

                memset(req, 0, reqlen);
                /* Copy protocol address for all requests */
                sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                               req_arpreq.rpc_arp_pa),
                               (struct sockaddr_storage *)
                                   (&(req_arpreq.arp_pa)));
                if (in->code == RPC_SIOCSARP)
                {
                    /* Copy HW address */
                    sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_ha),
                                   (struct sockaddr_storage *)
                                       (&(req_arpreq.arp_ha)));
                    /* Copy ARP flags */
                    req_arpreq.arp_flags =
                        arp_fl_rpc2h(out->req.req_val[0].ioctl_request_u.
                                     req_arpreq.rpc_arp_flags);
                }
#ifdef __linux__
                if (in->code == RPC_SIOCGARP)
                {
                     /* Copy device */
                    strcpy(req_arpreq.arp_dev,
                           out->req.req_val[0].ioctl_request_u.
                           req_arpreq.rpc_arp_dev);
                }
#endif /* !__linux__ */
                break;
            }

            default:
                ERROR("incorrect request type %d is received",
                      out->req.req_val[0].type);
                out->common._errno = TE_RC(TE_TA_LINUX, EINVAL);
                goto finish;
                break;
        }
    }

    if (in->access == IOCTL_WR)
        INIT_CHECKED_ARG(req, reqlen, 0);
    MAKE_CALL(out->retval = func(in->s, ioctl_rpc2h(in->code), req));
    if (req != NULL)
    {
        switch(out->req.req_val[0].type)
        {
            case IOCTL_INT:
                out->req.req_val[0].ioctl_request_u.req_int = req_int;
                break;

            case IOCTL_TIMEVAL:
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_sec =
                    req_timeval.tv_sec;
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_usec =
                    req_timeval.tv_usec;
                break;

            case IOCTL_IFREQ:
                switch (in->code)
                {
                    case RPC_SIOCGIFFLAGS:
                    case RPC_SIOCSIFFLAGS:
                        out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_flags = if_fl_h2rpc(
                                (uint32_t)(unsigned short int)
                                    req_ifreq.ifr_flags);
                        break;

                    case RPC_SIOCGIFMTU:
                    case RPC_SIOCSIFMTU:
                        out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_mtu = req_ifreq.ifr_mtu;
                        break;

                    case RPC_SIOCGIFADDR:
                    case RPC_SIOCSIFADDR:
                    case RPC_SIOCGIFNETMASK:
                    case RPC_SIOCSIFNETMASK:
                    case RPC_SIOCGIFBRDADDR:
                    case RPC_SIOCSIFBRDADDR:
                    case RPC_SIOCGIFDSTADDR:
                    case RPC_SIOCSIFDSTADDR:
                    case RPC_SIOCGIFHWADDR:
                        sockaddr_h2rpc(&(req_ifreq.ifr_addr),
                                       &(out->req.req_val[0].
                                         ioctl_request_u.
                                         req_ifreq.rpc_ifr_addr));
                        break;

                    default:
                        ERROR("Unsupported IOCTL request %d of type IFREQ",
                              in->code);
                        out->common._errno = TE_RC(TE_TA_LINUX, EINVAL);
                        goto finish;
                }
                break;

            case IOCTL_IFCONF:
            {
                struct ifreq       *req_c;
                struct tarpc_ifreq *req_t;

                int n = 1;
                int i;

                if (req_ifconf.ifc_len >
                    out->req.req_val[0].ioctl_request_u.req_ifconf.buflen)
                {
                    n = out->req.req_val[0].ioctl_request_u.
                            req_ifconf.buflen / sizeof(struct ifreq);
                }
                else
                {
                    n = req_ifconf.ifc_len / sizeof(struct ifreq);
                }

                out->req.req_val[0].ioctl_request_u.req_ifconf.buflen =
                    req_ifconf.ifc_len;

                if (req_ifconf.ifc_req == NULL)
                    break;

                if ((req_t = calloc(n, sizeof(*req_t))) == NULL)
                {
                    free(req_ifconf.ifc_buf);
                    ERROR("No enough memory");
                    out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
                    goto finish;
                }
                out->req.req_val[0].ioctl_request_u.req_ifconf.
                    rpc_ifc_req.rpc_ifc_req_val = req_t;
                out->req.req_val[0].ioctl_request_u.req_ifconf.
                    rpc_ifc_req.rpc_ifc_req_len = n;
                req_c = ((struct ifconf *)req)->ifc_req;

                for (i = 0; i < n; i++, req_t++, req_c++)
                {
                    strcpy(req_t->rpc_ifr_name, req_c->ifr_name);
                    if ((req_t->rpc_ifr_addr.sa_data.sa_data_val =
                         calloc(1, SA_DATA_MAX_LEN)) == NULL)
                    {
                        /* 
                         * Already allocated memory will be released 
                         * by RPC
                         */
                        free(req_ifconf.ifc_buf);
                        ERROR("No enough memory");
                        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
                        goto finish;
                    }
                    req_t->rpc_ifr_addr.sa_data.sa_data_len =
                        SA_DATA_MAX_LEN;
                    sockaddr_h2rpc(&(req_c->ifr_addr),
                                   &(req_t->rpc_ifr_addr));
                }
                free(req_ifconf.ifc_buf);
                break;
            }
            case IOCTL_ARPREQ:
            {
                if (in->code == RPC_SIOCGARP)
                {
                    /* Copy protocol address */
                    sockaddr_h2rpc(&(req_arpreq.arp_pa),
                                   &(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_pa));
                    /* Copy HW address */
                    sockaddr_h2rpc(&(req_arpreq.arp_ha),
                                   &(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_ha));

                    /* Copy flags */
                    out->req.req_val[0].ioctl_request_u.req_arpreq.
                        rpc_arp_flags = arp_fl_h2rpc(req_arpreq.arp_flags);
                }
                break;
            }

        }
    }
    finish:
    ;
}
)


static const char *
msghdr2str(const struct msghdr *msg)
{
    static char buf[256];

    char   *buf_end = buf + sizeof(buf);
    char   *p = buf;
    size_t  i;

    p += snprintf(p, buf_end - p, "{name={0x%x,%u},{",
                  (unsigned int)msg->msg_name, msg->msg_namelen);
    if (p >= buf_end)
        return "(too long)";
    for (i = 0; i < msg->msg_iovlen; ++i)
    {
        p += snprintf(p, buf_end - p, "%s{0x%x,%u}",
                      (i == 0) ? "" : ",",
                      (unsigned int)msg->msg_iov[i].iov_base,
                      msg->msg_iov[i].iov_len);
        if (p >= buf_end)
            return "(too long)";
    }
    p += snprintf(p, buf_end - p, "},control={0x%x,%u},flags=0x%x}",
                  (unsigned int)msg->msg_control, msg->msg_controllen,
                  msg->msg_flags);
    if (p >= buf_end)
        return "(too long)";

    return buf;
}

/*-------------- sendmsg() ------------------------------*/

TARPC_FUNC(sendmsg,
{
    if (in->msg.msg_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        return TRUE;
    }
},
{
    struct iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    unsigned int i;

    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (in->msg.msg_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, NULL,
                                     send_recv_flags_rpc2h(in->flags)));
    }
    else
    {
        struct msghdr msg;

        struct tarpc_msghdr *rpc_msg = in->msg.msg_val;

        memset(&msg, 0, sizeof(msg));

        PREPARE_ADDR(rpc_msg->msg_name, 0);
        msg.msg_namelen = rpc_msg->msg_namelen;
        msg.msg_name = a;
        msg.msg_iovlen = rpc_msg->msg_iovlen;

        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                INIT_CHECKED_ARG(
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_len,
                    0);
                iovec_arr[i].iov_base =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val;
                iovec_arr[i].iov_len =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len;
            }
            msg.msg_iov = iovec_arr;
            INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);
        }
        INIT_CHECKED_ARG(rpc_msg->msg_control.msg_control_val,
                         rpc_msg->msg_control.msg_control_len, 0);
        msg.msg_control = rpc_msg->msg_control.msg_control_val;
        msg.msg_controllen = rpc_msg->msg_controllen;
        msg.msg_flags = send_recv_flags_rpc2h(rpc_msg->msg_flags);
        INIT_CHECKED_ARG((char *)&msg, sizeof(msg), 0);

        VERB("sendmsg(): s=%d, msg=%s, flags=0x%x", in->s,
             msghdr2str(&msg), send_recv_flags_rpc2h(in->flags));
        MAKE_CALL(out->retval = func(in->s, &msg,
                                     send_recv_flags_rpc2h(in->flags)));
    }
}
)

/*-------------- recvmsg() ------------------------------*/

TARPC_FUNC(recvmsg,
{
    if (in->msg.msg_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        return TRUE;
    }
    COPY_ARG(msg);
},
{
    struct iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    unsigned int i;

    memset(iovec_arr, 0, sizeof(iovec_arr));
    if (out->msg.msg_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, NULL,
                                     send_recv_flags_rpc2h(in->flags)));
    }
    else
    {
        struct msghdr msg;

        struct tarpc_msghdr *rpc_msg = out->msg.msg_val;

        memset(&msg, 0, sizeof(msg));

        PREPARE_ADDR(rpc_msg->msg_name, rpc_msg->msg_namelen);
        msg.msg_namelen = rpc_msg->msg_namelen;
        msg.msg_name = a;

        msg.msg_iovlen = rpc_msg->msg_iovlen;
        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                INIT_CHECKED_ARG(
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_len,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len);
                iovec_arr[i].iov_base =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val;
                iovec_arr[i].iov_len =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len;
            }
            msg.msg_iov = iovec_arr;
            INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);
        }

        INIT_CHECKED_ARG(rpc_msg->msg_control.msg_control_val,
                         rpc_msg->msg_control.msg_control_len,
                         rpc_msg->msg_controllen);
        msg.msg_control = rpc_msg->msg_control.msg_control_val;
        msg.msg_controllen = rpc_msg->msg_controllen;

        msg.msg_flags = send_recv_flags_rpc2h(rpc_msg->msg_flags);

        /*
         * msg_name, msg_iov, msg_iovlen and msg_control MUST NOT be
         * changed.
         *
         * msg_namelen, msg_controllen and msg_flags MAY be changed.
         */
        INIT_CHECKED_ARG((char *)&msg.msg_name,
                         sizeof(msg.msg_name), 0);
        INIT_CHECKED_ARG((char *)&msg.msg_iov,
                         sizeof(msg.msg_iov), 0);
        INIT_CHECKED_ARG((char *)&msg.msg_iovlen,
                         sizeof(msg.msg_iovlen), 0);
        INIT_CHECKED_ARG((char *)&msg.msg_control,
                         sizeof(msg.msg_control), 0);

        VERB("recvmsg(): in msg=%s", msghdr2str(&msg));
        MAKE_CALL(out->retval = func(in->s, &msg,
                                     send_recv_flags_rpc2h(in->flags))
                  );
        VERB("recvmsg(): out msg=%s", msghdr2str(&msg));

        rpc_msg->msg_controllen = msg.msg_controllen;
        rpc_msg->msg_flags = send_recv_flags_h2rpc(msg.msg_flags);
        sockaddr_h2rpc(a, &(rpc_msg->msg_name));
        rpc_msg->msg_namelen = msg.msg_namelen;
        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                rpc_msg->msg_iov.msg_iov_val[i].iov_len =
                    iovec_arr[i].iov_len;
            }
        }
    }
}
)

/*-------------- poll() --------------------------------*/

TARPC_FUNC(poll,
{
    if (in->ufds.ufds_len > RPC_POLL_NFDS_MAX)
    {
        ERROR("Too big nfds is passed to the poll()");
        out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        return TRUE;
    }
    COPY_ARG(ufds);
},
{
    struct pollfd ufds[RPC_POLL_NFDS_MAX];

    unsigned int i;

    VERB("poll(): IN ufds=0x%x[%u] nfds=%u timeout=%d",
         (unsigned int)out->ufds.ufds_val, out->ufds.ufds_len, in->nfds,
         in->timeout);
    for (i = 0; i < out->ufds.ufds_len; i++)
    {
        ufds[i].fd = out->ufds.ufds_val[i].fd;
        INIT_CHECKED_ARG((char *)&(ufds[i].fd), sizeof(ufds[i].fd), 0);
        ufds[i].events = poll_event_rpc2h(out->ufds.ufds_val[i].events);
        INIT_CHECKED_ARG((char *)&(ufds[i].events),
                         sizeof(ufds[i].events), 0);
        ufds[i].revents = poll_event_rpc2h(out->ufds.ufds_val[i].revents);
        VERB("poll(): IN fd=%d events=%hd revents=%hd",
             ufds[i].fd, ufds[i].events, ufds[i].revents);
    }

    VERB("poll(): call with ufds=0x%x, nfds=%u, timeout=%d",
         (unsigned int)ufds, in->nfds, in->timeout);
    MAKE_CALL(out->retval = func((int)ufds, in->nfds, in->timeout));
    VERB("poll(): retval=%d", out->retval);

    for (i = 0; i < out->ufds.ufds_len; i++)
    {
        out->ufds.ufds_val[i].revents = poll_event_h2rpc(ufds[i].revents);
        VERB("poll(): OUT host-revents=%hd rpc-revents=%hd",
             ufds[i].revents, out->ufds.ufds_val[i].revents);
    }
}
)

/**
 * Convert host representation of the hostent to the RPC one.
 * The memory is allocated by the routine.
 *
 * @param he   source structure
 *
 * @return RPC structure or NULL is memory allocation failed
 */
static tarpc_hostent *
hostent_h2rpc(struct hostent *he)
{
    tarpc_hostent *rpc_he = (tarpc_hostent *)calloc(1, sizeof(*rpc_he));

    unsigned int i;
    unsigned int k;

    if (rpc_he == NULL)
        return NULL;

    if (he->h_name != NULL)
    {
        if ((rpc_he->h_name.h_name_val = strdup(he->h_name)) == NULL)
            goto release;
        rpc_he->h_name.h_name_len = strlen(he->h_name) + 1;
    }

    if (he->h_aliases != NULL)
    {
        char **ptr;

        for (i = 1, ptr = he->h_aliases; *ptr != NULL; ptr++, i++);

        if ((rpc_he->h_aliases.h_aliases_val =
             (tarpc_h_alias *)calloc(i, sizeof(tarpc_h_alias))) == NULL)
        {
            goto release;
        }
        rpc_he->h_aliases.h_aliases_len = i;

        for (k = 0; k < i - 1; k++)
        {
            if ((rpc_he->h_aliases.h_aliases_val[k].name.name_val =
                 strdup((he->h_aliases)[k])) == NULL)
            {
                goto release;
            }
            rpc_he->h_aliases.h_aliases_val[k].name.name_len =
                strlen((he->h_aliases)[k]) + 1;
        }
    }

    rpc_he->h_addrtype = domain_h2rpc(he->h_addrtype);
    rpc_he->h_length = he->h_length;

    if (he->h_addr_list != NULL)
    {
        char **ptr;

        for (i = 1, ptr = he->h_addr_list; *ptr != NULL; ptr++, i++);

        if ((rpc_he->h_addr_list.h_addr_list_val =
             (tarpc_h_addr *)calloc(i, sizeof(tarpc_h_addr))) == NULL)
        {
            goto release;
        }
        rpc_he->h_addr_list.h_addr_list_len = i;

        for (k = 0; k < i - 1; k++)
        {
            if ((rpc_he->h_addr_list.h_addr_list_val[i].val.val_val =
                 malloc(rpc_he->h_length)) == NULL)
            {
                goto release;
            }
            rpc_he->h_addr_list.h_addr_list_val[i].val.val_len =
                rpc_he->h_length;
            memcpy(rpc_he->h_addr_list.h_addr_list_val[i].val.val_val,
                   he->h_addr_list[i], rpc_he->h_length);
        }
    }

    return rpc_he;

release:
    /* Release the memory in the case of failure */
    free(rpc_he->h_name.h_name_val);
    if (rpc_he->h_aliases.h_aliases_val != NULL)
    {
        for (i = 0; i < rpc_he->h_aliases.h_aliases_len - 1; i++)
             free(rpc_he->h_aliases.h_aliases_val[i].name.name_val);
        free(rpc_he->h_aliases.h_aliases_val);
    }
    if (rpc_he->h_addr_list.h_addr_list_val != NULL)
    {
        for (i = 0; i < rpc_he->h_addr_list.h_addr_list_len - 1; i++)
            free(rpc_he->h_addr_list.h_addr_list_val[i].val.val_val);
        free(rpc_he->h_addr_list.h_addr_list_val);
    }
    free(rpc_he);
    return NULL;
}

/*-------------- gethostbyname() -----------------------------*/

TARPC_FUNC(gethostbyname, {},
{
    struct hostent *he;

    MAKE_CALL(he = (struct hostent *)func((int)(in->name.name_val)));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        else
            out->res.res_len = 1;
    }
}
)

/*-------------- gethostbyaddr() -----------------------------*/

TARPC_FUNC(gethostbyaddr, {},
{
    struct hostent *he;

    INIT_CHECKED_ARG(in->addr.val.val_val, in->addr.val.val_len, 0);

    MAKE_CALL(he = (struct hostent *)func((int)(in->addr.val.val_val),
                                          in->addr.val.val_len,
                                          addr_family_rpc2h(in->type)));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
        else
            out->res.res_len = 1;
    }
}
)


/*-------------- getaddrinfo() -----------------------------*/

/**
 * Convert host native addrinfo to the RPC one.
 *
 * @param ai            host addrinfo structure
 * @param rpc_ai        pre-allocated RPC addrinfo structure
 *
 * @return 0 in the case of success or -1 in the case of memory allocation
 * failure
 */
static int
ai_h2rpc(struct addrinfo *ai, struct tarpc_ai *ai_rpc)
{
    ai_rpc->flags = ai_flags_h2rpc(ai->ai_flags);
    ai_rpc->family = domain_h2rpc(ai->ai_family);
    ai_rpc->socktype = socktype_h2rpc(ai->ai_socktype);
    ai_rpc->protocol = proto_h2rpc(ai->ai_protocol);
    ai_rpc->addrlen = ai->ai_addrlen - SA_COMMON_LEN;

    if (ai->ai_addr != NULL)
    {
        if ((ai_rpc->addr.sa_data.sa_data_val =
             calloc(1, ai_rpc->addrlen)) == NULL)
        {
            return -1;
        }
        ai_rpc->addr.sa_family = addr_family_h2rpc(ai->ai_addr->sa_family);
        memcpy(ai_rpc->addr.sa_data.sa_data_val, ai->ai_addr->sa_data,
               ai_rpc->addrlen);
        ai_rpc->addr.sa_data.sa_data_len = ai_rpc->addrlen;
    }

    if (ai->ai_canonname != NULL)
    {
        if ((ai_rpc->canonname.canonname_val =
             strdup(ai->ai_canonname)) == NULL)
        {
            free(ai_rpc->addr.sa_data.sa_data_val);
            return -1;
        }
        ai_rpc->canonname.canonname_len = strlen(ai->ai_canonname) + 1;
    }

    return 0;
}

TARPC_FUNC(getaddrinfo, {},
{
    struct addrinfo  hints;
    struct addrinfo *info = NULL;
    struct addrinfo *res = NULL;

    struct sockaddr_storage addr;
    struct sockaddr        *a;

    if (in->hints.hints_val != NULL)
    {
        info = &hints;
        hints.ai_flags = ai_flags_rpc2h(in->hints.hints_val[0].flags);
        hints.ai_family = domain_rpc2h(in->hints.hints_val[0].family);
        hints.ai_socktype = socktype_rpc2h(in->hints.hints_val[0].socktype);
        hints.ai_protocol = proto_rpc2h(in->hints.hints_val[0].protocol);
        hints.ai_addrlen = in->hints.hints_val[0].addrlen + SA_COMMON_LEN;
        a = sockaddr_rpc2h(&(in->hints.hints_val[0].addr), &addr);
        INIT_CHECKED_ARG((char *)a,
                         in->hints.hints_val[0].addr.sa_data.sa_data_len +
                         SA_COMMON_LEN, 0);
        hints.ai_addr = a;
        hints.ai_canonname = in->hints.hints_val[0].canonname.canonname_val;
        INIT_CHECKED_ARG(in->hints.hints_val[0].canonname.canonname_val,
                         in->hints.hints_val[0].canonname.canonname_len, 0);
        hints.ai_next = NULL;
        INIT_CHECKED_ARG((char *)info, sizeof(*info), 0);
    }
    INIT_CHECKED_ARG(in->node.node_val, in->node.node_len, 0);
    INIT_CHECKED_ARG(in->service.service_val,
                     in->service.service_len, 0);
    /* I do not understand, which function is found by usual way */
    func = (sock_api_func)getaddrinfo;
    MAKE_CALL(out->retval = func((int)(in->node.node_val),
                                 in->service.service_val, info, &res));
    if (out->retval != 0 && res != NULL)
    {
        out->common._errno = TE_RC(TE_TA_LINUX, ETECORRUPTED);
        res = NULL;
    }
    if (res != NULL)
    {
        int i;

        struct tarpc_ai *arr;

        for (i = 0, info = res; info != NULL; i++, info = info->ai_next);

        if ((arr = calloc(i, sizeof(*arr))) != NULL)
        {
            int k;

            for (k = 0, info = res; k < i; k++, info = info->ai_next)
            {
                if (ai_h2rpc(info, arr + k) < 0)
                {
                    for (k--; k >= 0; k--)
                    {
                        free(arr[k].addr.sa_data.sa_data_val);
                        free(arr[k].canonname.canonname_val);
                    }
                    free(arr);
                    arr = NULL;
                    break;
                }
            }
        }
        if (arr == NULL)
        {
            out->common._errno = TE_RC(TE_TA_LINUX, ENOMEM);
            freeaddrinfo(res);
        }
        else
        {
            out->mem_ptr = (unsigned int)res;
            out->res.res_val = arr;
            out->res.res_len = i;
        }
    }
}
)

/*-------------- freeaddrinfo() -----------------------------*/
TARPC_FUNC(freeaddrinfo, {},
{
    func = (sock_api_func)freeaddrinfo;
    MAKE_CALL(func(in->mem_ptr));
}
)

/*-------------- pipe() --------------------------------*/
TARPC_FUNC(pipe, {},
{
    MAKE_CALL(out->retval = func((int)(out->filedes)));
}
)

/*-------------- socketpair() ------------------------------*/

TARPC_FUNC(socketpair, {},
{
    MAKE_CALL(out->retval = func(domain_rpc2h(in->domain),
                                 socktype_rpc2h(in->type),
                                 proto_rpc2h(in->proto), out->sv));
}
)

/*-------------- fopen() --------------------------------*/
TARPC_FUNC(fopen, {},
{
#if 1
    func = (sock_api_func)fopen;
#endif
    MAKE_CALL(out->mem_ptr = func((int)(in->path.path_val),
                                  in->mode.mode_val));
}
)

/*-------------- popen() --------------------------------*/
TARPC_FUNC(popen, {},
{
    func = (sock_api_func)popen;
    MAKE_CALL(out->mem_ptr = func((int)(in->cmd.cmd_val),
                                  in->mode.mode_val));
}
)

/*-------------- fileno() --------------------------------*/
TARPC_FUNC(fileno, {}, { MAKE_CALL(out->fd = func(in->mem_ptr)); })

/*-------------- getuid() --------------------------------*/
TARPC_FUNC(getuid, {}, { MAKE_CALL(out->uid = func(0)); })

/*-------------- getuid() --------------------------------*/
TARPC_FUNC(geteuid, {}, { MAKE_CALL(out->uid = func(0)); })

/*-------------- setuid() --------------------------------*/
TARPC_FUNC(setuid, {}, { MAKE_CALL(out->retval = func(in->uid)); })

/*-------------- seteuid() --------------------------------*/
TARPC_FUNC(seteuid, {}, { MAKE_CALL(out->retval = func(in->uid)); })

/*-------------- simple_sender() -----------------------------*/
TARPC_FUNC(simple_sender, {},
{
    MAKE_CALL(out->retval = func((int)in, out));
}
)

/**
 * Simple sender.
 *
 * @param in                input RPC argument
 *
 * @return number of sent bytes or -1 in the case of failure
 */
int
simple_sender(tarpc_simple_sender_in *in, tarpc_simple_sender_out *out)
{
    sock_api_func send_func;
    char          buf[1024];

    uint64_t sent = 0;

    int size = rand_range(in->size_min, in->size_max);
    int delay = rand_range(in->delay_min, in->delay_max);

    time_t start;
    time_t now;

#ifdef TA_DEBUG
    uint64_t control = 0;
#endif

    if (in->size_max > (int)sizeof(buf) || in->size_min > in->size_max ||
        in->delay_min > in->delay_max)
    {
        ERROR("Incorrect size of delay parameters");
        return -1;
    }

    if (find_func((tarpc_in_arg *)in, "send", &send_func) != 0)
        return -1;

    memset(buf, 0xDEADBEEF, sizeof(buf));

    for (start = now = time(NULL);
         now - start <= in->time2run;
         now = time(NULL))
    {
        int len;

        if (!in->size_rnd_once)
            size = rand_range(in->size_min, in->size_max);

        if (!in->delay_rnd_once)
            delay = rand_range(in->delay_min, in->delay_max);

        if (delay / 1000000 > in->time2run - (now - start) + 1)
            break;

        usleep(delay);

        len = send_func(in->s, buf, size, 0);

        if (len < 0)
        {
            if (!in->ignore_err)
            {
                ERROR("send() failed in simple_sender(): errno %x", errno);
                return -1;
            }
            else
            {
                len = errno = 0;
                continue;
            }
        }
#if 0 /* it's a legal situation if len < size */
        if (len < size)
        {
            ERROR("send() returned %d instead %d in simple_sender()",
                  len, size);
            return -1;
        }
#endif
        sent += len;
    }

    out->bytes_high = sent >> 32;
    out->bytes_low = sent & 0xFFFFFFFF;

    return 0;
}

/*-------------- simple_receiver() --------------------------*/
TARPC_FUNC(simple_receiver, {},
{
    MAKE_CALL(out->retval = func((int)in, out));
}
)

/**
 * Simple receiver.
 *
 * @param in                input RPC argument
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
simple_receiver(tarpc_simple_receiver_in *in,
                tarpc_simple_receiver_out *out)
{
    sock_api_func select_func;
    sock_api_func recv_func;
    char          buf[1024];

    uint64_t received = 0;
#ifdef TA_DEBUG
    uint64_t control = 0;
    int start = time(NULL);
#endif

    if (find_func((tarpc_in_arg *)in, "select", &select_func) != 0 ||
        find_func((tarpc_in_arg *)in, "recv", &recv_func) != 0)
    {
        return -1;
    }

    while (TRUE)
    {
        struct timeval tv = { 1, 0 };
        fd_set         set;
        int            len;

        FD_ZERO(&set);
        FD_SET(in->s, &set);
        if (select_func(in->s + 1, &set, NULL, NULL, &tv) < 0)
        {
            ERROR("select() failed in simple_receiver(): errno %x", errno);
            return -1;
        }
        if (!FD_ISSET(in->s, &set))
        {
            if (received > 0)
                break;
            else
                continue;
        }

        len = recv_func(in->s, buf, sizeof(buf), 0);

        if (len < 0)
        {
            ERROR("recv() failed in simple_receiver(): errno %x", errno);
            return -1;
        }

        if (len == 0)
        {
            RING("recv() returned 0 in simple_receiver() because of peer "
                 "shutdown");
            goto simple_receiver_exit;
        }

        received += len;
    }

simple_receiver_exit:

    out->bytes_high = received >> 32;
    out->bytes_low = received & 0xFFFFFFFF;

    return 0;
}


#define FLOODER_ECHOER_WAIT_FOR_RX_EMPTY        1
#define FLOODER_BUF                             4096

/*-------------- flooder() --------------------------*/
TARPC_FUNC(flooder, {},
{
    MAKE_CALL(out->retval = func((int)in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

typedef int (*flood_api_func)(struct pollfd *ufds,
                              unsigned int nfds, int timeout);

/**
 * Routine which receives data from specified set of sockets and sends data
 * to specified set of sockets with maximum speed using I/O multiplexing.
 *
 * @param pco       - PCO to be used
 * @param rcvrs     - set of receiver sockets
 * @param rcvnum    - number of receiver sockets
 * @param sndrs     - set of sender sockets
 * @param sndnum    - number of sender sockets
 * @param bulkszs   - sizes of data bulks to send for each sender
 *                    (in bytes, 1024 bytes maximum)
 * @param time2run  - how long send data (in seconds)
 * @param iomux     - type of I/O Multiplexing function
 *                    (@b select(), @b pselect(), @b poll())
 *
 * @return 0 on success or -1 in the case of failure
 */
int
flooder(tarpc_flooder_in *in)
{
    sock_api_func select_func;
    sock_api_func pselect_func;
    sock_api_func p_func;
    sock_api_func write_func;
    sock_api_func read_func;
    sock_api_func ioctl_func;

    flood_api_func poll_func;

    int        *rcvrs = in->rcvrs.rcvrs_val;
    int         rcvnum = in->rcvrs.rcvrs_len;
    int        *sndrs = in->sndrs.sndrs_val;
    int         sndnum = in->sndrs.sndrs_len;
    int         bulkszs = in->bulkszs;
    int         time2run = in->time2run;
    iomux_func  iomux = in->iomux;
    te_bool     rx_nb = in->rx_nonblock;

    unsigned long   *tx_stat = in->tx_stat.tx_stat_val;
    unsigned long   *rx_stat = in->rx_stat.rx_stat_val;

    int      i;
    int      rc;
    int      sent;
    int      received;
    char     rcv_buf[FLOODER_BUF];
    char     snd_buf[FLOODER_BUF];

    fd_set          rfds, wfds;
    struct pollfd   ufds[RPC_POLL_NFDS_MAX] = { {0, 0, 0}, };
    int             ufds_elements = (sndnum >= rcvnum) ? sndnum: rcvnum;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    struct timespec ts;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;


    memset(rcv_buf, 0x0, FLOODER_BUF);
    memset(snd_buf, 0xA, FLOODER_BUF);

    if ((find_func((tarpc_in_arg *)in, "select", &select_func) != 0)   ||
        (find_func((tarpc_in_arg *)in, "pselect", &pselect_func) != 0) ||
        (find_func((tarpc_in_arg *)in, "poll", &p_func) != 0)          ||
        (find_func((tarpc_in_arg *)in, "read", &read_func) != 0)       ||
        (find_func((tarpc_in_arg *)in, "write", &write_func) != 0)     ||
        (find_func((tarpc_in_arg *)in, "ioctl", &ioctl_func) != 0))
    {
        return -1;
    }

    poll_func = (flood_api_func)p_func;

    if (rx_nb)
    {
        int on = 1;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctl(rcvrs[i], FIONBIO, &on)) != 0)
            {
                ERROR("%s(): ioctl(FIONBIO) failed: %X",
                      __FUNCTION__, errno);
                return -1;
            }
        }
    }

    /* Calculate max descriptor */
    for (i = 0; i < rcvnum; ++i)
    {
        if (rcvrs[i] > max_descr)
            max_descr = rcvrs[i];
    }
    for (i = 0; i < sndnum; ++i)
    {
        if (sndrs[i] > max_descr)
            max_descr = sndrs[i];
    }

    /*
     * FIXME
     * If 'b_array' does not contain all descriptors in 'l_array',
     * the last will be missing.
     */
    if (iomux == FUNC_POLL)
    {
        int  j;
        int *b_array = (sndnum >= rcvnum) ? sndrs: rcvrs;
        int  b_length = (sndnum >= rcvnum) ? sndnum: rcvnum;
        int  b_flag = (sndnum >= rcvnum) ? POLLOUT: POLLIN;
        int *l_array = (sndnum >= rcvnum) ? rcvrs: sndrs;
        int  l_length = (sndnum >= rcvnum) ? rcvnum: sndnum;
        int  l_flag = (sndnum >= rcvnum) ? POLLIN: POLLOUT;

        for (i = 0; i < b_length; i++)
        {
            ufds[i].fd = b_array[i];
            ufds[i].events = b_flag;
            for (j = 0; j < l_length; j ++)
            {
                if (ufds[i].fd == l_array[j])
                    ufds[i].events |= l_flag;
            }
        }
    }

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %X",
              __FUNCTION__, errno);
        return -1;
    }
    timeout.tv_sec += time2run;

    call_timeout.tv_sec = time2run;
    call_timeout.tv_usec = 0;

    INFO("%s(): time2run=%d, timeout=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        session_rx = FALSE;

        /* If select() or pselect() should be used as iomux function */
        if (iomux == FUNC_SELECT ||
            iomux == FUNC_PSELECT)
        {
            /* Prepare sets of file descriptors */
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            for (i = 0; time2run_not_expired && (i < sndnum); ++i)
                FD_SET(sndrs[i], &wfds);
            for (i = 0; i < rcvnum; ++i)
                FD_SET(rcvrs[i], &rfds);

            if (iomux == FUNC_SELECT)
            {
                rc = select_func(max_descr + 1, &rfds,
                                 time2run_not_expired ? &wfds : NULL,
                                 NULL, &call_timeout);
            }
            else
            {
                ts.tv_sec  = call_timeout.tv_sec;
                ts.tv_nsec = call_timeout.tv_usec * 1000;
                rc = pselect_func(max_descr + 1, &rfds,
                                  time2run_not_expired ? &wfds : NULL,
                                  NULL, &ts, NULL);
            }
            if (rc < 0)
            {
                ERROR("%s(): (p)select() failed: %X", __FUNCTION__, errno);
                return -1;
            }

            /*
             * Send data to sockets that are ready for sending
             * if time is not expired.
             */
            if (time2run_not_expired && (rc > 0))
            {
                for (i = 0; i < sndnum; i++)
                {
                    if (FD_ISSET(sndrs[i], &wfds))
                    {
                        sent = write_func(sndrs[i], snd_buf, bulkszs);
                        if ((sent < 0) &&
                            (errno != EAGAIN) && (errno != EWOULDBLOCK))
                        {
                            ERROR("%s(): write() failed: %X",
                                  __FUNCTION__, errno);
                            return -1;
                        }
                        if ((sent > 0) && (tx_stat != NULL))
                        {
                            tx_stat[i] += sent;
                        }
                    }
                }
            }

            /* Receive data from sockets that are ready */
            for (i = 0; rc > 0 && i < rcvnum; i++)
            {
                if (FD_ISSET(rcvrs[i], &rfds))
                {
                    received = read_func(rcvrs[i], rcv_buf,
                                         sizeof(rcv_buf));
                    if ((received < 0) &&
                        (errno != EAGAIN) && (errno != EWOULDBLOCK))
                    {
                        ERROR("%s(): read() failed: %X",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (received > 0)
                    {
                        session_rx = TRUE;
                        if (rx_stat != NULL)
                            rx_stat[i] += received;
                        if (!time2run_not_expired)
                            VERB("FD=%d Rx=%d", rcvrs[i], received);
                    }
                }
            }
        }
        else if (iomux == FUNC_POLL) /* poll() should be used as iomux */
        {
            rc = poll_func(ufds, ufds_elements,
                           call_timeout.tv_sec * 1000 +
                           call_timeout.tv_usec / 1000);

            if (rc < 0)
            {
                ERROR("%s(): poll() failed: %X", __FUNCTION__, errno);
                return -1;
            }

            for (i = 0; (rc > 0) && i < ufds_elements; i++)
            {
                if (time2run_not_expired && (ufds[i].revents & POLLOUT))
                {
                    sent = write_func(ufds[i].fd, snd_buf, bulkszs);
                    if ((sent < 0) &&
                        (errno != EAGAIN) && (errno != EWOULDBLOCK))
                    {
                        ERROR("%s(): write() failed: %X",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if ((sent > 0) && (tx_stat != NULL))
                    {
                        tx_stat[i] += sent;
                    }
                }
                if (ufds[i].revents & POLLIN)
                {
                    received = read_func(ufds[i].fd, rcv_buf,
                                         sizeof(rcv_buf));
                    if ((received < 0) &&
                        (errno != EAGAIN) && (errno != EWOULDBLOCK))
                    {
                        ERROR("%s(): read() failed: %X",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (received > 0)
                    {
                        session_rx = TRUE;
                        if (rx_stat != NULL)
                            rx_stat[i] += received;
                        if (!time2run_not_expired)
                            VERB("FD=%d Rx=%d", ufds[i].fd, received);
                    }
                }
#ifdef DEBUG
                if ((!time2run_not_expired) &&
                    (ufds[i].revents & ~POLLIN))
                {
                    WARN("poll() returned unexpected events: 0x%x",
                         ufds[i].revents);
                }
#endif
            }
        }
        else
        {
            ERROR("%s(): unknown iomux() function", __FUNCTION__);
            return -1;
        }


        if (time2run_not_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed): %X",
                      __FUNCTION__, errno);
                return -1;
            }
            call_timeout.tv_sec  = timeout.tv_sec  - timestamp.tv_sec;
            call_timeout.tv_usec = timeout.tv_usec - timestamp.tv_usec;
            if (call_timeout.tv_usec < 0)
            {
                --(call_timeout.tv_sec);
                call_timeout.tv_usec += 1000000;
#ifdef DEBUG
                if (call_timeout.tv_usec < 0)
                {
                    ERROR("Unexpected situation, assertation failed\n"
                          "%s:%d", __FILE__, __LINE__);
                }
#endif
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                /* Clean up POLLOUT requests for all descriptors */
                for (i = 0; i < ufds_elements; ++i)
                {
                    ufds[i].events &= ~POLLOUT;
                }
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
#ifdef DEBUG
            else if (call_timeout.tv_sec < time2run)
            {
                VERB("%s(): timeout %ld.%06ld", __FUNCTION__,
                     call_timeout.tv_sec, call_timeout.tv_usec);
                time2run >>= 1;
            }
#endif
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            call_timeout.tv_usec = 0;
            VERB("%s(): Waiting for empty Rx queue, Rx=%d",
                 __FUNCTION__, session_rx);
        }

    } while (time2run_not_expired || session_rx);

    if (rx_nb)
    {
        int off = 0;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctl(rcvrs[i], FIONBIO, &off)) != 0)
            {
                ERROR("%s(): ioctl(FIONBIO) failed: %X",
                      __FUNCTION__, errno);
                return -1;
            }
        }
    }

    INFO("%s(): OK", __FUNCTION__);

    /* Clean up errno */
    errno = 0;

    return 0;
}

/*-------------- echoer() --------------------------*/
TARPC_FUNC(echoer, {},
{
    MAKE_CALL(out->retval = func((int)in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

/**
 * Routine which receives data from specified set of
 * sockets using I/O multiplexing and sends them back
 * to the socket.
 *
 * @param pco       - PCO to be used
 * @param sockets   - set of sockets to be processed
 * @param socknum   - number of sockets to be processed
 * @param time2run  - how long send data (in seconds)
 * @param iomux     - type of I/O Multiplexing function
 *                    (@b select(), @b pselect(), @b poll())
 *
 * @return 0 on success or -1 in the case of failure
 */
int
echoer(tarpc_echoer_in *in)
{
    sock_api_func select_func;
    sock_api_func pselect_func;
    sock_api_func p_func;
    sock_api_func write_func;
    sock_api_func read_func;

    flood_api_func poll_func;

    int        *sockets = in->sockets.sockets_val;
    int         socknum = in->sockets.sockets_len;
    int         time2run = in->time2run;
    iomux_func  iomux = in->iomux;

    unsigned long   *tx_stat = in->tx_stat.tx_stat_val;
    unsigned long   *rx_stat = in->rx_stat.rx_stat_val;

    int      i;
    int      rc;
    int      sent;
    int      received;
    char     buf[FLOODER_BUF];

    fd_set          rfds;
    struct pollfd   ufds[RPC_POLL_NFDS_MAX] = { {0, 0, 0}, };
    int             ufds_elements = socknum;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    struct timespec ts;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;


    memset(buf, 0x0, FLOODER_BUF);

    if ((find_func((tarpc_in_arg *)in, "select", &select_func) != 0)   ||
        (find_func((tarpc_in_arg *)in, "pselect", &pselect_func) != 0) ||
        (find_func((tarpc_in_arg *)in, "poll", &p_func) != 0)          ||
        (find_func((tarpc_in_arg *)in, "read", &read_func) != 0)       ||
        (find_func((tarpc_in_arg *)in, "write", &write_func) != 0))
    {
        return -1;
    }

    poll_func = (flood_api_func)p_func;

    /* Calculate max descriptor */
    for (i = 0; i < socknum; i++)
    {
        if (sockets[i] > max_descr)
            max_descr = sockets[i];
    }

    if (iomux == FUNC_POLL)
    {
        for (i = 0; i < socknum; i++)
        {
            ufds[i].fd = sockets[i];
            ufds[i].events = POLLIN;
        }
    }

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %X",
              __FUNCTION__, errno);
        return -1;
    }
    timeout.tv_sec += time2run;

    call_timeout.tv_sec = time2run;
    call_timeout.tv_usec = 0;

    INFO("%s(): time2run=%d, timeout timestamp=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        session_rx = FALSE;

        /* If select() or pselect() should be used as iomux function */
        if (iomux == FUNC_SELECT ||
            iomux == FUNC_PSELECT)
        {
            /* Prepare sets of file descriptors */
            FD_ZERO(&rfds);
            for (i = 0; i < socknum; i++)
                FD_SET(sockets[i], &rfds);

            if (iomux == FUNC_SELECT)
            {
                rc = select_func(max_descr + 1, &rfds, NULL, NULL,
                                 &call_timeout);
            }
            else
            {
                ts.tv_sec  = call_timeout.tv_sec;
                ts.tv_nsec = call_timeout.tv_usec * 1000;
                rc = pselect_func(max_descr + 1, &rfds, NULL, NULL, &ts,
                                  NULL);
            }
            if (rc < 0)
            {
                ERROR("%s(): (p)select() failed: %X", __FUNCTION__, errno);
                return -1;
            }

            /* Receive data from sockets that are ready */
            for (i = 0; (rc > 0) && i < socknum; i++)
            {
                if (FD_ISSET(sockets[i], &rfds))
                {
                    received = read_func(sockets[i], buf, sizeof(buf));
                    if (received < 0)
                    {
                        ERROR("%s(): read() failed: %X",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (rx_stat != NULL)
                        rx_stat[i] += received;
                    session_rx = TRUE;

                    sent = write_func(sockets[i], buf, received);
                    if (sent < 0)
                    {
                        ERROR("%s(): write() failed: %X",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (tx_stat != NULL)
                        tx_stat[i] += sent;
                }
            }
        }
        else if (iomux == FUNC_POLL) /* poll() should be used as iomux */
        {
            rc = poll_func(ufds, ufds_elements,
                           call_timeout.tv_sec * 1000 +
                           call_timeout.tv_usec / 1000);

            if (rc < 0)
            {
                ERROR("%s(): poll() failed: %X", __FUNCTION__, errno);
                return -1;
            }

            for (i = 0; i < ufds_elements; i++)
            {
                if (ufds[i].revents & POLLIN)
                {
                    received = read_func(ufds[i].fd, buf, sizeof(buf));
                    if (received < 0)
                    {
                        ERROR("%s(): read() failed: %X",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (rx_stat != NULL)
                        rx_stat[i] += received;
                    session_rx = TRUE;

                    sent = write_func(ufds[i].fd, buf, received);
                    if (sent < 0)
                    {
                        ERROR("%s(): write() failed: %X",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (tx_stat != NULL)
                        tx_stat[i] += sent;
                }
            }
        }
        else
        {
            ERROR("%s(): unknown iomux() function", __FUNCTION__);
            return -1;
        }


        if (time2run_not_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed: %X",
                      __FUNCTION__, errno);
                return -1;
            }
            call_timeout.tv_sec  = timeout.tv_sec  - timestamp.tv_sec;
            call_timeout.tv_usec = timeout.tv_usec - timestamp.tv_usec;
            if (call_timeout.tv_usec < 0)
            {
                --(call_timeout.tv_sec);
                call_timeout.tv_usec += 1000000;
#ifdef DEBUG
                if (call_timeout.tv_usec < 0)
                {
                    ERROR("Unexpected situation, assertation failed\n"
                          "%s:%d", __FILE__, __LINE__);
                }
#endif
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
#ifdef DEBUG
            else if (call_timeout.tv_sec < time2run)
            {
                VERB("%s(): timeout %ld.%06ld", __FUNCTION__,
                     call_timeout.tv_sec, call_timeout.tv_usec);
                time2run >>= 1;
            }
#endif
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            call_timeout.tv_usec = 0;
            VERB("%s(): Waiting for empty Rx queue", __FUNCTION__);
        }

    } while (time2run_not_expired || session_rx);

    INFO("%s(): OK", __FUNCTION__);

    return 0;
}


/** Macro for producing of diagnostic messages sent to the engine */
#define DIAG(fmt...) \
    do {                                                        \
        snprintf(out->diag.diag_val, out->diag.diag_len, fmt);  \
        errno = 0;                                              \
    } while (0)

/*-------------- aio_read_test() -----------------------------*/
TARPC_FUNC(aio_read_test,
{
    COPY_ARG(buf);
    COPY_ARG(diag);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->buflen);
#ifdef HAVE_AIO_H
    MAKE_CALL(out->retval = func((int)in, out));
#else
    out->retval = -1;
    out->common._errno = EOPNOTSUPP;
#endif
}
)


#ifdef HAVE_AIO_H
int
aio_read_test(tarpc_aio_read_test_in *in, tarpc_aio_read_test_out *out)
{
    struct aiocb   cb;
    struct timeval t;
    int            rc;

    sock_api_func aio_read_func;
    sock_api_func aio_error_func;
    sock_api_func aio_return_func;

    if (find_func((tarpc_in_arg *)in, "aio_read", &aio_read_func) != 0   ||
        find_func((tarpc_in_arg *)in, "aio_error", &aio_error_func) != 0 ||
        find_func((tarpc_in_arg *)in, "aio_return", &aio_return_func) != 0)
    {
        DIAG("Failed to resolve asynchronous IO function");
        return -1;
    }

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = in->s;
    cb.aio_buf = out->buf.buf_val;
    cb.aio_nbytes = in->buflen;
    cb.aio_sigevent.sigev_signo =
        in->signum == 0 ? 0 : signum_rpc2h(in->signum);
    t.tv_sec = in->t;
    t.tv_usec = 0;

    if (aio_read_func((int)&cb) < 0)
    {
        DIAG("aio_read() returnred -1");
        return -1;
    }

    if ((rc = aio_error_func((int)&cb)) != EINPROGRESS)
    {
        DIAG("aio_error() called immediately after aio_read()"
             "returned %d instead EINPROGRESS", rc);
        return -1;
    }
    select(0, NULL, NULL, NULL, &t);
    if ((rc = aio_error_func((int)&cb)) != 0)
    {
        snprintf(out->diag.diag_val, out->diag.diag_len,
                 "aio_error() returned %d after select() unblocking", rc);
    }
    if ((rc = aio_return_func((int)&cb)) <= 0)
    {
        DIAG("aio_return() returned %d - no data are received", rc);
    }
    out->buf.buf_len = rc;

    return rc;
}
#endif /* HAVE_AIO_H */

/*-------------- aio_error_test() -----------------------------*/
TARPC_FUNC(aio_error_test,
{
    COPY_ARG(diag);
},
{
#ifdef HAVE_AIO_H
    MAKE_CALL(out->retval = func((int)in, out));
#else
    out->retval = -1;
    out->common._errno = EOPNOTSUPP;
#endif
}
)

#ifdef HAVE_AIO_H
int
aio_error_test(tarpc_aio_error_test_in *in, tarpc_aio_error_test_out *out)
{
    struct aiocb cb;
    int          rc;

    sock_api_func aio_write_func;
    sock_api_func aio_error_func;

    if (find_func((tarpc_in_arg *)in, "aio_write", &aio_write_func) != 0 ||
        find_func((tarpc_in_arg *)in, "aio_error", &aio_error_func) != 0)
    {
        DIAG("Failed to resolve asynchronous IO function");
        return -1;
    }

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = -1;
    cb.aio_buf = "dummy";
    cb.aio_nbytes = 5;
    if (aio_write_func((int)&cb) < 0)
    {
        DIAG("aio_write() failed");
        return -1;
    }
    usleep(100);
    if ((rc = aio_error_func((int)&cb)) != EBADF)
    {
        DIAG("aio_error() returned %d instead EBADF for bad request", rc);
        return -1;
    }
    errno = 0;
    return 0;
}
#endif

/*-------------- aio_write_test() -----------------------------*/
TARPC_FUNC(aio_write_test,
{
    COPY_ARG(diag);
},
{
#ifdef HAVE_AIO_H
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->buf.buf_len);
    out->retval = -1;
    MAKE_CALL(out->retval = func((int)in, out));
#else
    out->retval = -1;
    out->common._errno = EOPNOTSUPP;
#endif
}
)

#ifdef HAVE_AIO_H
int
aio_write_test(tarpc_aio_write_test_in *in, tarpc_aio_write_test_out *out)
{
    struct aiocb   cb;
    int            rc;

    sock_api_func aio_write_func;
    sock_api_func aio_error_func;
    sock_api_func aio_return_func;

    if (find_func((tarpc_in_arg *)in, "aio_write", &aio_write_func) != 0 ||
        find_func((tarpc_in_arg *)in, "aio_error", &aio_error_func) != 0 ||
        find_func((tarpc_in_arg *)in, "aio_return", &aio_return_func) != 0)
    {
        DIAG("Failed to resolve asynchronous IO function");
        return -1;
    }

    cb.aio_fildes = in->s;
    cb.aio_buf = in->buf.buf_val;
    cb.aio_nbytes = in->buf.buf_len;
    cb.aio_sigevent.sigev_signo =
        in->signum == 0 ? 0 : signum_rpc2h(in->signum);

    if (aio_write_func((int)&cb) < 0)
    {
        DIAG("aio_write() failed");
        return -1;
    }

    while (aio_error_func((int)&cb) != 0)
        usleep(100);

    if ((rc = aio_return_func((int)&cb)) <= 0)
    {
        DIAG("aio_return() returned %d - no data are sent", rc);
        return -1;
    }
    return rc;
}
#endif /* HAVE_AIO_H */

/*-------------- aio_suspend_test() ---------------------------*/
TARPC_FUNC(aio_suspend_test,
{
    COPY_ARG(buf);
    COPY_ARG(diag);
},
{
#ifdef HAVE_AIO_H
    MAKE_CALL(out->retval = func((int)in, out));
#else
    out->retval = -1;
    out->common._errno = EOPNOTSUPP;
#endif
}
)

#ifdef HAVE_AIO_H
int
aio_suspend_test(tarpc_aio_suspend_test_in *in,
                 tarpc_aio_suspend_test_out *out)
{
    struct aiocb   cb1, cb2;
    struct aiocb  *cb[3] = { &cb1 , NULL, &cb2};
    int            rc;

    struct timespec ts = { 0, 1000000 };
    struct timeval  tv1, tv2;

    sock_api_func aio_read_func;
    sock_api_func aio_return_func;
    sock_api_func aio_suspend_func;

    char aux_buf[8];

    if (find_func((tarpc_in_arg *)in, "aio_read",
                  &aio_read_func) != 0   ||
        find_func((tarpc_in_arg *)in, "aio_suspend",
                  &aio_suspend_func) != 0 ||
        find_func((tarpc_in_arg *)in, "aio_return",
                  &aio_return_func) != 0)
    {
        DIAG("Failed to resolve asynchronous IO function");
        return -1;
    }
    memset(&cb1, 0, sizeof(cb));
    cb1.aio_fildes = in->s_aux;
    cb1.aio_buf = aux_buf;
    cb1.aio_nbytes = sizeof(aux_buf);
    cb1.aio_sigevent.sigev_signo =
        in->signum == 0 ? 0 : signum_rpc2h(in->signum);
    if (aio_read_func((int)&cb1) < 0)
    {
        DIAG("aio_read() returnred -1");
        return -1;
    }
    memset(&cb2, 0, sizeof(cb));
    cb2.aio_fildes = in->s;
    cb2.aio_buf = out->buf.buf_val;
    cb2.aio_nbytes = out->buf.buf_len;
    cb2.aio_sigevent.sigev_signo =
        in->signum == 0 ? 0 : signum_rpc2h(in->signum);
    if (aio_read_func((int)&cb2) < 0)
    {
        DIAG("aio_read() returnred -1");
        return -1;
    }

    gettimeofday(&tv1, NULL);
    if (aio_suspend_func((int)cb, 3, &ts) == 0)
    {
        DIAG("aio_suspend() returned 0 whereas requests are not satisfied");
        return -1;
    }
    if (errno != EAGAIN)
    {
        DIAG("aio_suspend() set incorrect errno %d instead EAGAIN"
             " after timeout", errno);
        return -1;
    }
    gettimeofday(&tv2, NULL);
    if (tv2.tv_sec > tv1.tv_sec)
        tv2.tv_usec += 1000000;
    if (tv2.tv_usec - tv1.tv_usec < 1000)
    {
        DIAG("aio_suspend() did not block during specified timeout");
        return -1;
    }

    ts.tv_sec = in->t;
    ts.tv_nsec = 0;
    rc = aio_suspend_func((int)cb, 3, &ts);
    if (in->signum == 0 && rc < 0)
    {
        DIAG("aio_suspend() returned -1\n");
        return -1;
    }
    else if (in->signum != 0)
    {
        if (errno != EINTR)
        {
            DIAG("aio_suspend() set errno to %d instead expected EINTR\n",
                 errno);
            return -1;
        }
    }

    if ((rc = aio_return_func((int)&cb2)) <= 0)
    {
        DIAG("aio_return() returned %d - no data are received", rc);
    }
    out->buf.buf_len = rc;

    return rc;
}
#endif /* HAVE_AIO_H */


/*-------------- sendfile() ------------------------------*/

TARPC_FUNC(sendfile,
{
    COPY_ARG(offset);
},
{
    MAKE_CALL(out->retval =
        func(in->out_fd, in->in_fd,
             out->offset.offset_len == 0 ? NULL : out->offset.offset_val,
             in->count));
}
)

/*-------------- socket_to_file() ------------------------------*/
#define SOCK2FILE_BUF_LEN  4096

TARPC_FUNC(socket_to_file, {},
{
   MAKE_CALL(out->retval = func((int)in));
}
)

/**
 * Routine which receives data from socket and write data
 * to specified path.
 *
 * @return -1 in the case of failure or some positive value in other cases
 */
int
socket_to_file(tarpc_socket_to_file_in *in)
{
    sock_api_func select_func;
    sock_api_func write_func;
    sock_api_func read_func;

    int      sock = in->sock;
    char    *path = in->path.path_val;
    long     time2run = in->timeout;

    int      rc = 0;
    int      file_d = -1;
    int      written;
    int      received;
    size_t   total = 0;
    char     buffer[SOCK2FILE_BUF_LEN];

    fd_set          rfds;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;

    path[in->path.path_len] = '\0';

    INFO("%s() called with: sock=%d, path=%s, timeout=%ld",
         __FUNCTION__, sock, path, time2run);

    if ((find_func((tarpc_in_arg *)in, "select", &select_func) != 0)   ||
        (find_func((tarpc_in_arg *)in, "read", &read_func) != 0)       ||
        (find_func((tarpc_in_arg *)in, "write", &write_func) != 0))
    {
        ERROR("Failed to resolve functions addresses");
        rc = -1;
        goto local_exit;
    }

    file_d = open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (file_d < 0)
    {
        ERROR("%s(): open(%s, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO) "
              "failed: %X", __FUNCTION__, path, errno);
        rc = -1;
        goto local_exit;
    }
    INFO("%s(): file '%s' opened with descriptor=%d", __FUNCTION__,
         path, file_d);

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %X",
              __FUNCTION__, errno);
        rc = -1;
        goto local_exit;
    }
    timeout.tv_sec += time2run;

    call_timeout.tv_sec = time2run;
    call_timeout.tv_usec = 0;

    INFO("%s(): time2run=%ld, timeout timestamp=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        session_rx = FALSE;

        /* Prepare sets of file descriptors */
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        rc = select_func(sock + 1, &rfds, NULL, NULL, &call_timeout);
        if (rc < 0)
        {
            ERROR("%s(): select() failed: %X", __FUNCTION__, errno);
            break;
        }
        VERB("%s(): select finishes for waiting of events", __FUNCTION__);

        /* Receive data from socket that are ready */
        if (FD_ISSET(sock, &rfds))
        {
            VERB("%s(): select observes data for reading on the "
                 "socket=%d", __FUNCTION__, sock);
            received = read_func(sock, buffer, sizeof(buffer));
            VERB("%s(): read() retrieve %d bytes", __FUNCTION__, received);
            if (received < 0)
            {
                ERROR("%s(): read() failed: %X", __FUNCTION__, errno);
                rc = -1;
                break;
            }
            else if (received > 0)
            {
                session_rx = TRUE;

                total += received;
                VERB("%s(): write retrieved data to file", __FUNCTION__);
                written = write(file_d, buffer, received);
                VERB("%s(): %d bytes are written to file",
                     __FUNCTION__, written);
                if (written < 0)
                {
                    ERROR("%s(): write() failed: %X", __FUNCTION__, errno);
                    rc = -1;
                    break;
                }
                if (written != received)
                {
                    ERROR("%s(): write() cannot write all received in "
                          "the buffer data to the file "
                          "(received=%d, written=%d): %X",
                          __FUNCTION__, errno, received, written);
                    rc = -1;
                    break;
                }
            }
        }

        if (time2run_not_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed): %X",
                      __FUNCTION__, errno);
                rc = -1;
                break;
            }
            call_timeout.tv_sec  = timeout.tv_sec  - timestamp.tv_sec;
            call_timeout.tv_usec = timeout.tv_usec - timestamp.tv_usec;
            if (call_timeout.tv_usec < 0)
            {
                --(call_timeout.tv_sec);
                call_timeout.tv_usec += 1000000;
#ifdef DEBUG
                if (call_timeout.tv_usec < 0)
                {
                    ERROR("Unexpected situation, assertion failed\n"
                          "%s:%d", __FILE__, __LINE__);
                }
#endif
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
#ifdef DEBUG
            else if (call_timeout.tv_sec < time2run)
            {
                VERB("%s(): timeout %ld.%06ld", __FUNCTION__,
                     call_timeout.tv_sec, call_timeout.tv_usec);
                time2run >>= 1;
            }
#endif
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            call_timeout.tv_usec = 0;
            VERB("%s(): Waiting for empty Rx queue, Rx=%d",
                 __FUNCTION__, session_rx);
        }

    } while (time2run_not_expired || session_rx);

local_exit:
    RING("Stop to get data from socket %d and put to file %s, %s, "
         "received %u", sock, path,
         (!time2run_not_expired) ? "timeout expired" :
                                   "unexpected failure",
         total);
    INFO("%s(): %s", __FUNCTION__, (rc == 0) ? "OK" : "FAILED");

    if (file_d != -1)
        close(file_d);

    /* Clean up errno */
    if (rc == 0)
    {
        errno = 0;
        rc = total;
    }
    return rc;
}

/*-------------- ftp_open() ------------------------------*/

TARPC_FUNC(ftp_open, {},
{
    MAKE_CALL(out->fd = func((int)(in->uri.uri_val),
                             in->rdonly ? O_RDONLY : O_WRONLY,
                             in->passive, in->offset));
}
)
