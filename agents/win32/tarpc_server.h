/** @file
 * @brief Windows Test Agent
 *
 * RPC server common definitions
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TARPC_SERVER_H__
#define __TARPC_SERVER_H__

#include <winsock2.h>
#include <winerror.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#define _SYS_SOCKET_H
#define _NETINET_IN_H
#include "tarpc.h"
#include "ta_common.h"

#if defined(_WINSOCK_H) || defined(__CYGWIN__)
struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};
#endif

#define TE_ERRNO_LOG_UNKNOWN_OS_ERRNO

#undef ERROR
#include "te_errno.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "rcf_rpc_defs.h"
#include "te_rpc_types.h"

/** Unspecified error code */
#define ERROR_UNSPEC    0xFFFFFF

#define PRINT(msg...) \
    do {                                                \
       printf(msg); printf("\n"); fflush(stdout);       \
    } while (0)

/** Check return code and update the errno */
#define TARPC_CHECK_RC(expr_) \
    do {                                            \
        int rc_ = (expr_);                          \
                                                    \
        if (rc_ != 0 && out->common._errno == 0)    \
            out->common._errno = rc_;               \
    } while (FALSE)

#ifdef WSP_PRIVATE_PROTO

#define WSP_IPPROTO_TCP 200
#define WSP_IPPROTO_UDP 201
static inline int 
wsp_proto_rpc2h(int socktype, int proto)
{
    int proto_h = proto_rpc2h(proto);

    switch (proto_h)
    {
        case IPPROTO_TCP:
            proto_h = WSP_IPPROTO_TCP;
            break;
         
        case IPPROTO_UDP:
            proto_h = WSP_IPPROTO_UDP;
            break;
        
        case 0:
            switch (socktype_rpc2h(socktype))
            {
                case SOCK_STREAM:
                    proto_h = WSP_IPPROTO_TCP;
                    break;
                  
                case SOCK_DGRAM:
                    proto_h = WSP_IPPROTO_UDP;
                    break;
            }
            break;
    }

    return proto_h;
}

#else

static inline int 
wsp_proto_rpc2h(int socktype, int proto)
{
    UNUSED(socktype);
    return proto_rpc2h(proto);
}

#endif

/* Microsoft extended defines */
#define WSAID_ACCEPTEX \
    {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_CONNECTEX \
    {0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
#define WSAID_DISCONNECTEX \
    {0x7fda2e11,0x8630,0x436f, \
     {0xa0, 0x31, 0xf5, 0x36, 0xa6, 0xee, 0xc1, 0x57}}
#define WSAID_GETACCEPTEXSOCKADDRS \
    {0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_TRANSMITFILE \
    {0xb5367df0,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_TRANSMITPACKETS \
    {0xd9689da0,0x1f90,0x11d3,{0x99,0x71,0x00,0xc0,0x4f,0x68,0xc8,0x76}}
#define WSAID_WSARECVMSG \
    {0xf689d7c8,0x6f1f,0x436b,{0x8a,0x53,0xe5,0x4f,0xe3,0x51,0xc3,0x22}}

/** Function prototypes for Microsoft extended functions
 *  from Windows Sockets 2 */
/* ConnectEx() */
typedef BOOL (__stdcall  *LPFN_CONNECTEX)(SOCKET s,
                                          const struct sockaddr* name,
                                          int namelen,
                                          PVOID lpSendBuffer,
                                          DWORD dwSendDataLength,
                                          LPDWORD lpdwBytesSent,
                                          LPOVERLAPPED lpOverlapped);
/* DisconnectEx() */
typedef BOOL (__stdcall  *LPFN_DISCONNECTEX)(SOCKET hSocket,
                                             LPOVERLAPPED lpOverlapped,
                                             DWORD dwFlags,
                                             DWORD reserved);
/* AcceptEx() */
typedef BOOL (__stdcall *LPFN_ACCEPTEX)(SOCKET sListenSocket,
                                        SOCKET sAcceptSocket,
                                        PVOID lpOutputBuffer,
                                        DWORD dwReceiveDataLength,
                                        DWORD dwLocalAddressLength,
                                        DWORD dwRemoteAddressLength,
                                        LPDWORD lpdwBytesReceived,
                                        LPOVERLAPPED lpOverlapped);
/* GetAcceptExSockAddrs() */
typedef void (__stdcall *LPFN_GETACCEPTEXSOCKADDRS)(PVOID lpOutputBuffer,
                                          DWORD dwReceiveDataLength,
                                          DWORD dwLocalAddressLength,
                                          DWORD dwRemoteAddressLength,
                                          LPSOCKADDR *LocalSockaddr,
                                          LPINT LocalSockaddrLength,
                                          LPSOCKADDR *RemoteSockaddr,
                                          LPINT RemoteSockaddrLength);
/* TransmitFile() */
typedef BOOL (__stdcall *LPFN_TRANSMITFILE)(SOCKET hSocket,
                                  HANDLE hFile,
                                  DWORD nNumberOfBytesToWrite,
                                  DWORD nNumberOfBytesPerSend,
                                  LPOVERLAPPED lpOverlapped,
                                  TRANSMIT_FILE_BUFFERS *lpTransmitBuffers,
                                  DWORD dwFlags);
/* WSARecvMsg() */
typedef int (__stdcall *LPFN_WSARECVMSG)(SOCKET s,
                               LPWSAMSG lpMsg,
                               LPDWORD lpdwNumberOfBytesRecvd,
                               LPWSAOVERLAPPED lpOverlapped,
                               LPWSAOVERLAPPED_COMPLETION_ROUTINE
                               lpCompletionRoutine);

extern LPFN_CONNECTEX            pf_connect_ex;
extern LPFN_DISCONNECTEX         pf_disconnect_ex;
extern LPFN_ACCEPTEX             pf_accept_ex;
extern LPFN_GETACCEPTEXSOCKADDRS pf_get_accept_ex_sockaddrs;
extern LPFN_TRANSMITFILE         pf_transmit_file;
extern LPFN_WSARECVMSG           pf_wsa_recvmsg;

#define IN_HWND         ((HWND)(rcf_pch_mem_get(in->hwnd)))
#define IN_HEVENT       ((WSAEVENT)(rcf_pch_mem_get(in->hevent)))
#define IN_OVERLAPPED   ((rpc_overlapped *)rcf_pch_mem_get(in->overlapped))
#define IN_FDSET        ((fd_set *)(rcf_pch_mem_get(in->set)))
#define IN_CALLBACK     ((LPWSAOVERLAPPED_COMPLETION_ROUTINE) \
                             rcf_ch_symbol_addr(in->callback, 1))

/**
 * Converts the windows error to RPC one.
 */ 
static inline te_errno 
win_rpc_errno(int err)
{
    switch (err)
    {
        case 0: return 0;
        case WSAEACCES: return RPC_EACCES;
        case WSAEFAULT: return RPC_EFAULT;
        case WSAEINVAL: return RPC_EINVAL;
        case WSAEMFILE: return RPC_EMFILE;
        case WSAEWOULDBLOCK: return RPC_EAGAIN;
        case WSAEINPROGRESS: return RPC_EINPROGRESS;
        case WSAEALREADY: return RPC_EALREADY;
        case WSAENOTSOCK: return RPC_ENOTSOCK;
        case WSAEDESTADDRREQ: return RPC_EDESTADDRREQ;
        case WSAEMSGSIZE: return RPC_EMSGSIZE;
        case WSAEPROTOTYPE: return RPC_EPROTOTYPE;
        case WSAENOPROTOOPT: return RPC_ENOPROTOOPT;
        case WSAEPROTONOSUPPORT: return RPC_EPROTONOSUPPORT;
        case WSAESOCKTNOSUPPORT: return RPC_ESOCKTNOSUPPORT;
        case WSAEOPNOTSUPP: return RPC_EOPNOTSUPP;
        case WSAEPFNOSUPPORT: return RPC_EPFNOSUPPORT;
        case WSAEAFNOSUPPORT: return RPC_EAFNOSUPPORT;
        case WSAEADDRINUSE: return RPC_EADDRINUSE;
        case WSAEADDRNOTAVAIL: return RPC_EADDRNOTAVAIL;
        case WSAENETDOWN: return RPC_ENETDOWN;
        case WSAENETUNREACH: return RPC_ENETUNREACH;
        case WSAENETRESET: return RPC_ENETRESET;
        case WSAECONNABORTED: return RPC_ECONNABORTED;
        case WSAECONNRESET: return RPC_ECONNRESET;
        case WSAENOBUFS: return RPC_ENOBUFS;
        case WSAEISCONN: return RPC_EISCONN;
        case WSAENOTCONN: return RPC_ENOTCONN;
        case WSAESHUTDOWN: return RPC_ESHUTDOWN;
        case WSAETIMEDOUT: return RPC_ETIMEDOUT;
        case WSAECONNREFUSED: return RPC_ECONNREFUSED;
        case WSAEHOSTDOWN: return RPC_EHOSTDOWN;
        case WSAEHOSTUNREACH: return RPC_EHOSTUNREACH;
        case WSA_INVALID_HANDLE: return RPC_EBADF;
        
        case ERROR_UNEXP_NET_ERR: return RPC_E_UNEXP_NET_ERR;
        case ERROR_IO_INCOMPLETE: return RPC_E_IO_INCOMPLETE;
        case ERROR_IO_PENDING: return RPC_E_IO_PENDING;
        case ERROR_NOACCESS: return RPC_EFAULT; /* FIXME? */
        case ERROR_MORE_DATA: return RPC_EMSGSIZE; /* FIXME? */
        case ERROR_INVALID_PARAMETER: return RPC_EINVAL; /* FIXME? */
        case WAIT_TIMEOUT: return RPC_E_WAIT_TIMEOUT;
        
        case ERROR_UNSPEC: return RPC_EUNSPEC;
        
        default: 
            ERROR("Unknown windows error code %d", err);
            return RPC_EUNKNOWN; 
    }
}

/**
 * Create a thread.
 *
 * @param func          entry point
 * @param arg           argument
 * @param tid           location for thread identifier
 *
 * @return Status code
 */
static inline te_errno
thread_create(void *func, void *arg, uint32_t *tid)
{
    DWORD  tmp;
    HANDLE hp = CreateThread(NULL, 0, func, arg, 0, &tmp);
    
    if (hp == NULL)
        return win_rpc_errno(GetLastError());
    
    *tid = rcf_pch_mem_alloc(hp);
    
    SetLastError(0);

    return 0;
}

/** 
 * Cancel the thread.
 *
 * @param tid   thread identifier
 *
 * @return Status code
 */
static inline te_errno
thread_cancel(uint32_t tid)
{
    HANDLE hp = rcf_pch_mem_get(tid);

    if (!TerminateThread(hp, 0))
        return win_rpc_errno(GetLastError());
        
    rcf_pch_mem_free(tid);

    return 0;
}

/** 
 * Exit from the thread returning DWORD exit code.
 *
 * @param argument to return
 */
static inline void
thread_exit(void *ret)
{
    DWORD rc = rcf_pch_mem_alloc(ret);
    
    if (rc == STILL_ACTIVE)
    {
        DWORD new_rc = rcf_pch_mem_alloc(ret);
        
        rcf_pch_mem_free(rc);
        
        ExitThread(new_rc);
        
        return;
    }

    ExitThread(rc);
}

/** 
 * Join the thread.
 *
 * @param tid   thread identifier
 * @param arg   location for thread result
 *
 * @return Status code
 */
static inline te_errno
thread_join(uint32_t tid, void **arg)
{
    HANDLE hp = rcf_pch_mem_get(tid);
    DWORD  rc;

    while (TRUE)
    {
        if (!GetExitCodeThread(hp, &rc))
            return win_rpc_errno(GetLastError());
            
        if (rc != STILL_ACTIVE)
            break;
            
        SleepEx(10, TRUE);
    }
    
    if (arg != NULL)
        *arg = rcf_pch_mem_get(rc);
    
    rcf_pch_mem_free(rc);
    rcf_pch_mem_free(tid);
            
    return 0;
}

/** 
 * Overlapped object with additional parameters - memory associated
 * with overlapped information  
 */
typedef struct rpc_overlapped {
    WSAOVERLAPPED  overlapped; /**< WSAOVERLAPPED object itself */
    LPWSABUF       buffers;    /**< List of allocated buffers   */
    int            bufnum;     /**< Number of allocated buffers */
    
    /** Test-specific cookies */
    uint32_t       cookie1;
    uint32_t       cookie2;
} rpc_overlapped;

/**
 * Free memory allocated for overlapped routine; it is assumed
 * that test does not call two routines with single overlapped object
 * simultaneously.
 */
static inline void
rpc_overlapped_free_memory(rpc_overlapped *overlapped)
{
    int i;

    if (overlapped == NULL || overlapped->buffers == NULL)
        return;

    for (i = 0; i < overlapped->bufnum; i++)
        free(overlapped->buffers[i].buf);

    free(overlapped->buffers);
    overlapped->buffers = NULL;
    overlapped->bufnum = 0;
}

/**
 * Allocate memory for the overlapped object and copy vector content to it.
 */
static inline int
iovec2overlapped(rpc_overlapped *overlapped, int vector_len,
                 struct tarpc_iovec *vector)
{
    rpc_overlapped_free_memory(overlapped);
    
    if (vector_len == 0)
    {
        overlapped->buffers = NULL;
        overlapped->bufnum = 0;
        return 0;
    }

    if ((overlapped->buffers =
            (WSABUF *)calloc(vector_len, sizeof(WSABUF))) == NULL)
    {
        return TE_ENOMEM;
    }

    for (; overlapped->bufnum < vector_len; overlapped->bufnum++)
    {
        if (vector[overlapped->bufnum].iov_len != 0 &&
            (overlapped->buffers[overlapped->bufnum].buf =
             calloc(vector[overlapped->bufnum].iov_len, 1)) == NULL)
        {
            rpc_overlapped_free_memory(overlapped);
            return TE_ENOMEM;
        }
        overlapped->buffers[overlapped->bufnum].len =
            vector[overlapped->bufnum].iov_len;
        if (vector[overlapped->bufnum].iov_base.iov_base_val != NULL)
        {
            memcpy(overlapped->buffers[overlapped->bufnum].buf,
                   vector[overlapped->bufnum].iov_base.iov_base_val,
                   vector[overlapped->bufnum].iov_len);
        }
    }

    return 0;
}

/** Copy memory from the overlapped object to vector and free it */
static inline int
overlapped2iovec(rpc_overlapped *overlapped, int *vector_len,
                 struct tarpc_iovec **vector_val)
{
    int i;
    
    if (overlapped->bufnum == 0)
    {
        *vector_val = NULL;
        *vector_len = 0;
        return 0;
    }

    *vector_val = (struct tarpc_iovec *)
        malloc(sizeof(struct tarpc_iovec) * overlapped->bufnum);
    if (*vector_val == NULL)
    {
        rpc_overlapped_free_memory(overlapped);
        return TE_ENOMEM;
    }
    *vector_len = overlapped->bufnum;
    for (i = 0; i < overlapped->bufnum; i++)
    {
        (*vector_val)[i].iov_base.iov_base_val = overlapped->buffers[i].buf;
        (*vector_val)[i].iov_len = (*vector_val)[i].iov_base.iov_base_len =
           overlapped->buffers[i].len;
        overlapped->buffers[i].buf = NULL;
    }
    free(overlapped->buffers);
    overlapped->buffers = NULL;
    overlapped->bufnum = 0;
    return 0;
}

/**
 * Allocate memory for the overlapped object and copy buffer content to it.
 */
static inline int
buf2overlapped(rpc_overlapped *overlapped, int buflen, char *buf)
{
    struct tarpc_iovec vector;

    vector.iov_base.iov_base_len = vector.iov_len = buflen;
    vector.iov_base.iov_base_val = buf;

    return iovec2overlapped(overlapped, 1, &vector);
}

#if FOR_FUTURE
/** Copy memory from the overlapped object to buffer and free it */
static inline void
overlapped2buf(rpc_overlapped *overlapped, int *buf_len, char **buf_val)
{
    if (overlapped->buffers == NULL)
    {
        *buf_val = NULL;
        *buf_len = 0;
        return;
    }
    *buf_val = overlapped->buffers[0].buf;
    *buf_len = overlapped->buffers[0].len;
    overlapped->buffers[0].buf = NULL;
    free(overlapped->buffers);
    overlapped->buffers = NULL;
    overlapped->bufnum = 0;
}
#endif

/**
 * Convert shutdown parameter from RPC to native representation.
 */
static inline int
shut_how_rpc2h(rpc_shut_how how)
{
    switch (how)
    {
        case RPC_SHUT_WR:   return SD_SEND;
        case RPC_SHUT_RD:   return SD_RECEIVE;
        case RPC_SHUT_RDWR: return SD_BOTH;
        default: return (SD_SEND + SD_RECEIVE + SD_BOTH + 1);
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
        WARN("Strange tarpc_sa length %d is received",
             rpc_addr->sa_data.sa_data_len);
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

/** Structure for checking of variable-length arguments safity */
typedef struct checked_arg {
    struct checked_arg *next; /**< Next checked argument in the list */

    char *real_arg;     /**< Pointer to real buffer */
    char *control;      /**< Pointer to control buffer */
    int   len;          /**< Whole length of the buffer */
    int   len_visible;  /**< Length passed to the function under test */
} checked_arg;

/** Initialise the checked argument and add it into the list */
static inline void
init_checked_arg(checked_arg **list, char *real_arg, int len, 
                 int len_visible)
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
    init_checked_arg(&list, _real_arg, _len, _len_visible)

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
            rc = TE_RC(TE_TA_WIN32, TE_ECORRUPTED);
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

/** Wait time specified in the input argument. */
#define WAIT_START(msec_start)                                  \
    do {                                                        \
        struct timeval t;                                       \
                                                                \
        uint64_t msec_now;                                      \
                                                                \
        gettimeofday(&t, NULL);                                 \
        msec_now = (uint64_t)((uint32_t)(t.tv_sec) * 1000 +     \
                              (uint32_t)(t.tv_usec) / 1000);    \
                                                                \
        if (msec_start > msec_now)                              \
        {                                                       \
            RING("Sleep %u microseconds before call",           \
                 (msec_start - msec_now) * 1000);               \
            Sleep(msec_start - msec_now);                       \
        }                                                       \
        else if (msec_start != 0)                               \
            WARN("Start time is gone");                         \
    } while (0)


/**
 * Declare and initialise time variables; execute the code and store
 * duration and errno in the output argument.
 */
#define MAKE_CALL(x)                                                \
    do {                                                            \
        struct timeval t_start;                                     \
        struct timeval t_finish;                                    \
        int           _rc;                                          \
                                                                    \
        WAIT_START(in->common.start);                               \
        gettimeofday(&t_start, NULL);                               \
        SetLastError(ERROR_UNSPEC);                                 \
        x;                                                          \
        out->common._errno = win_rpc_errno(GetLastError());         \
        gettimeofday(&t_finish, NULL);                              \
        out->common.duration =                                      \
            (t_finish.tv_sec - t_start.tv_sec) * 1000000 +          \
            t_finish.tv_usec - t_start.tv_usec;                     \
        _rc = check_args(list);                                     \
        if (out->common._errno == 0 && _rc != 0)                    \
            out->common._errno = _rc;                               \
    } while (0)

#define TARPC_FUNC(_func, _copy_args, _actions)                         \
                                                                        \
typedef struct _func##_arg {                                            \
    tarpc_##_func##_in  in;                                             \
    tarpc_##_func##_out out;                                            \
    te_bool             done;                                           \
} _func##_arg;                                                          \
                                                                        \
static void *                                                           \
_func##_proc(void *arg)                                                 \
{                                                                       \
    tarpc_##_func##_in  *in = &(((_func##_arg *)arg)->in);              \
    tarpc_##_func##_out *out = &(((_func##_arg *)arg)->out);            \
    checked_arg         *list = NULL;                                   \
                                                                        \
    logfork_register_user(#_func);                                      \
                                                                        \
    VERB("Entry thread %s", #_func);                                    \
                                                                        \
    { _actions }                                                        \
                                                                        \
    ((_func##_arg *)arg)->done = TRUE;                                  \
                                                                        \
    thread_exit(arg);                                                   \
                                                                        \
    return NULL;                                                        \
}                                                                       \
                                                                        \
bool_t                                                                  \
_##_func##_1_svc(tarpc_##_func##_in *in, tarpc_##_func##_out *out,      \
                 struct svc_req *rqstp)                                 \
{                                                                       \
    checked_arg  *list = NULL;                                          \
    _func##_arg  *arg;                                                  \
    enum xdr_op  op = XDR_FREE;                                         \
                                                                        \
    UNUSED(rqstp);                                                      \
    memset(out, 0, sizeof(*out));                                       \
    VERB("PID=%d TID=%d: Entry %s",                                     \
         (int)getpid(), (int)thread_self(), #_func);                    \
                                                                        \
    _copy_args                                                          \
                                                                        \
    if (in->common.op == RCF_RPC_CALL_WAIT)                             \
    {                                                                   \
        _actions                                                        \
        return TRUE;                                                    \
    }                                                                   \
                                                                        \
    if (in->common.op == RCF_RPC_CALL)                                  \
    {                                                                   \
        uint32_t _tid;                                                  \
                                                                        \
        if ((arg = malloc(sizeof(*arg))) == NULL)                       \
        {                                                               \
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);         \
            return TRUE;                                                \
        }                                                               \
                                                                        \
        arg->in = *in;                                                  \
        arg->out = *out;                                                \
        arg->done = FALSE;                                              \
                                                                        \
        if (thread_create(_func##_proc,                                 \
                          (void *)arg, &_tid) != 0)                     \
        {                                                               \
            free(arg);                                                  \
            out->common._errno = TE_OS_RC(TE_TA_WIN32, errno);          \
        }                                                               \
                                                                        \
        memset(in, 0, sizeof(*in));                                     \
        memset(out, 0, sizeof(*out));                                   \
        out->common.tid = _tid;                                         \
        out->common.done = rcf_pch_mem_alloc(&arg->done);               \
                                                                        \
        return TRUE;                                                    \
    }                                                                   \
                                                                        \
    if ((out->common._errno =                                           \
             thread_join(in->common.tid, (void *)(&arg))) != 0)         \
    {                                                                   \
        return TRUE;                                                    \
    }                                                                   \
    if (arg == NULL)                                                    \
    {                                                                   \
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);             \
        return TRUE;                                                    \
    }                                                                   \
    xdr_tarpc_##_func##_out((XDR *)&op, out);                           \
    *out = arg->out;                                                    \
    rcf_pch_mem_free(in->common.done);                                  \
    free(arg);                                                          \
    return TRUE;                                                        \
}

#endif /* __TARPC_SERVER_H__ */
