/** @file
 * @brief Windows Test Agent
 *
 * RPC server implementation for Berkeley Socket API RPCs
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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

#include <winsock2.h>
#include <winerror.h>
#include <mswsock.h>
#define _SYS_SOCKET_H
#define _NETINET_IN_H
#include "tarpc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#undef ERROR
#include "te_defs.h"
#include "te_errno.h"
#include "rcf_ch_api.h"
#include "rcf_rpc_defs.h"

#include "tapi_rpcsock_defs.h"
#include "win32_rpc.h"
#include "ta_logfork.h"

#define PRINT(msg...) \
    do {                                                \
       printf(msg); printf("\n"); fflush(stdout);       \
    } while (0)

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

extern int ta_pid;
extern sigset_t rpcs_received_signals;
extern HINSTANCE ta_hinstance;

static LPFN_CONNECTEX            pf_connect_ex = NULL;
static LPFN_DISCONNECTEX         pf_disconnect_ex = NULL;
static LPFN_ACCEPTEX             pf_accept_ex = NULL;
static LPFN_GETACCEPTEXSOCKADDRS pf_get_accept_ex_sockaddrs = NULL;
static LPFN_TRANSMITFILE         pf_transmit_file = NULL;
static LPFN_WSARECVMSG           pf_wsa_recvmsg = NULL;

/**
 * Translates WSAError to errno.
 */ 
static int 
wsaerr2errno(int wsaerr)
{
    int err = 0;
    
    switch (wsaerr)
    {
        case RPC_WSAEACCES:
        {
            err = RPC_EACCES;
            break;
        }    
        
        case RPC_WSAEFAULT:
        {
            err = RPC_EFAULT;
            break;
        }
        
        case RPC_WSAEINVAL:
        {
            err = RPC_EINVAL;
            break;
        }    
        
        case RPC_WSAEMFILE:
        {
            err = RPC_EMFILE;
            break;
        }    
        
        case RPC_WSAEWOULDBLOCK:
        {
            err = RPC_EAGAIN;
            break;
        }    
        
        case RPC_WSAEINPROGRESS:
        {
            err = RPC_EINPROGRESS;
            break;
        }    
        
        case RPC_WSAEALREADY:
        {
            err = RPC_EALREADY;
            break;
        }
        
        case RPC_WSAENOTSOCK:
        {
            err = RPC_ENOTSOCK;
            break;
        }    
        
        case RPC_WSAEDESTADDRREQ:
        {
            err = RPC_EDESTADDRREQ;
            break;
        }    
        
        case RPC_WSAEMSGSIZE:
        {
            err = RPC_EMSGSIZE;
            break;
        }
        
        case RPC_WSAEPROTOTYPE:
        {
            err = RPC_EPROTOTYPE;
            break;
        }    
        
        case RPC_WSAENOPROTOOPT:
        {
            err = RPC_ENOPROTOOPT;
            break;
        }    
        
        case RPC_WSAEPROTONOSUPPORT:
        {
            err = RPC_EPROTONOSUPPORT;
            break;
        }    
        
        case RPC_WSAESOCKTNOSUPPORT:
        {
            err = RPC_ESOCKTNOSUPPORT;
            break;
        }    
        
        case RPC_WSAEOPNOTSUPP:
        {
            err = RPC_EOPNOTSUPP;
            break;
        } 
        
        case RPC_WSAEPFNOSUPPORT:
        {
            err = RPC_EPFNOSUPPORT;
            break;
        } 
        
        case RPC_WSAEAFNOSUPPORT:
        {
            err = RPC_EAFNOSUPPORT;
            break;
        } 
        
        case RPC_WSAEADDRINUSE:
        {
            err = RPC_EADDRINUSE;
            break;
        } 
        
        case RPC_WSAEADDRNOTAVAIL:
        {
            err = RPC_EADDRNOTAVAIL;
            break;
        } 
        
        case RPC_WSAENETDOWN:
        {
            err = RPC_ENETDOWN;
            break;
        } 
        
        case RPC_WSAENETUNREACH:
        {
            err = RPC_ENETUNREACH;
            break;
        } 
        
        case RPC_WSAENETRESET:
        {
            err = RPC_ENETRESET;
            break;
        } 
        
        case RPC_WSAECONNABORTED:
        {
            err = RPC_ECONNABORTED;
            break;
        } 
        
        case RPC_WSAECONNRESET:
        {
            err = RPC_ECONNRESET;
            break;
        }
        
        case RPC_WSAENOBUFS:
        {
            err = RPC_ENOBUFS;
            break;
        } 

        case RPC_WSAEISCONN:
        {
            err = RPC_EISCONN;
            break;
        } 
        
        case RPC_WSAENOTCONN:
        {
            err = RPC_ENOTCONN;
            break;
        } 
        
        case RPC_WSAESHUTDOWN:
        {
            err = RPC_ESHUTDOWN;
            break;
        } 
        
        case RPC_WSAETIMEDOUT:
        {
            err = RPC_ETIMEDOUT;
            break;
        } 
        
        case RPC_WSAECONNREFUSED:
        {
            err = RPC_ECONNREFUSED;
            break;
        } 
        
        case RPC_WSAEHOSTDOWN:
        {
            err = RPC_EHOSTDOWN;
            break;
        } 
        
        case RPC_WSAEHOSTUNREACH:
        {
            err = RPC_EHOSTUNREACH;
            break;
        } 
 
        default:
        {
            err = RPC_EINVAL; 
        }
    }    

    
    return err;    
}

void 
wsa_func_handles_discover()
{
    GUID  guid_connect_ex = WSAID_CONNECTEX;
    GUID  guid_disconnect_ex = WSAID_DISCONNECTEX;
    GUID  guid_accept_ex = WSAID_ACCEPTEX;
    GUID  guid_get_accept_ex_sockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    GUID  guid_transmit_file = WSAID_TRANSMITFILE;
    GUID  guid_wsa_recvmsg = WSAID_WSARECVMSG;
    DWORD bytes_returned;
    int   s = socket(AF_INET, SOCK_DGRAM, 0);
    
#define DISCOVER_FUNC(_func, _func_cup) \
    do {                                                                \
        if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,             \
                     (LPVOID)&guid_##_func, sizeof(GUID),               \
                     (LPVOID)&pf_##_func, sizeof(LPFN_##_func_cup),     \
                      &bytes_returned, NULL, NULL) == SOCKET_ERROR)     \
        {                                                               \
            int _errno = WSAGetLastError();                             \
            PRINT("Cannot retrieve %s pointer via WSAIoctl();"          \
                 " errno %d", #_func, _errno);                          \
        }                                                               \
    } while (0)

    DISCOVER_FUNC(connect_ex, CONNECTEX);
    DISCOVER_FUNC(disconnect_ex, DISCONNECTEX);
    DISCOVER_FUNC(accept_ex, ACCEPTEX);
    DISCOVER_FUNC(get_accept_ex_sockaddrs, GETACCEPTEXSOCKADDRS);
    DISCOVER_FUNC(transmit_file, TRANSMITFILE);
    DISCOVER_FUNC(wsa_recvmsg, WSARECVMSG);
    
#undef DISCOVER_FUNC    
    
    closesocket(s);
}

static int wsaerr2errno(int wsaerr);

/** Overlapped object with additional parameters - memory associated
    with overlapped information  */
typedef struct rpc_overlapped {
    WSAOVERLAPPED  overlapped; /**< WSAOVERLAPPED object itself */
    LPWSABUF       buffers;     /**< List of allocated buffers   */
    int            bufnum;     /**< Number of allocated buffers */
} rpc_overlapped;

/**
 * Free memory allocated for overlapped routine; it is assumed
 * that test does not call two routines with single overlapped object
 * simultaneously.
 */
static void
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
static int
iovec2overlapped(rpc_overlapped *overlapped, int vector_len,
                 struct tarpc_iovec *vector)
{
    rpc_overlapped_free_memory(overlapped);

    if ((overlapped->buffers =
            (WSABUF *)calloc(vector_len, sizeof(WSABUF))) == NULL)
    {
        return ENOMEM;
    }

    for (; overlapped->bufnum < vector_len; overlapped->bufnum++)
    {
        if (vector[overlapped->bufnum].iov_len != 0 &&
        (overlapped->buffers[overlapped->bufnum].buf =
             calloc(vector[overlapped->bufnum].iov_len, 1)) == NULL)
    {
        rpc_overlapped_free_memory(overlapped);
        return ENOMEM;
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
static int
overlapped2iovec(rpc_overlapped *overlapped, int *vector_len,
                 struct tarpc_iovec **vector_val)
{
    int i;

    *vector_val = (struct tarpc_iovec *)
        malloc(sizeof(struct tarpc_iovec) * overlapped->bufnum);
    if (*vector_val == NULL)
    {
        rpc_overlapped_free_memory(overlapped);
        return ENOMEM;
    }
    *vector_len = overlapped->bufnum;
    for (i = 0; i < overlapped->bufnum; i++)
    {
        (*vector_val)[i].iov_base.iov_base_val = overlapped->buffers[i].buf;
        (*vector_val)[i].iov_len = (*vector_val)[i].iov_base.iov_base_len =
           overlapped->buffers[i].len;
    }
    free(overlapped->buffers);
    overlapped->buffers = NULL;
    overlapped->bufnum = 0;
    return 0;
}

/**
 * Allocate memory for the overlapped object and copy buffer content to it.
 */
static int
buf2overlapped(rpc_overlapped *overlapped, int buflen, char *buf)
{
    struct tarpc_iovec vector;

    vector.iov_base.iov_base_len = vector.iov_len = buflen;
    vector.iov_base.iov_base_val = buf;

    return iovec2overlapped(overlapped, 1, &vector);
}

#if FOR_FUTURE
/** Copy memory from the overlapped object to buffer and free it */
static void
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
static void
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
            rc = TE_RC(TE_TA_WIN32, ETECORRUPTED);
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
#define MAKE_CALL(x)                                                \
    do {                                                            \
        struct timeval t_start;                                     \
        struct timeval t_finish;                                    \
        int           _rc;                                          \
                                                                    \
        WAIT_START(in->common.start);                               \
        gettimeofday(&t_start, NULL);                               \
        errno = 0;                                                  \
        WSASetLastError(0);                                         \
        x;                                                          \
        out->common.win_error = win_error_h2rpc(WSAGetLastError()); \
        out->common._errno = RPC_ERRNO;                             \
        if (out->common.win_error != 0)                             \
            out->common._errno =                                    \
                wsaerr2errno(out->common.win_error);                \
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
    sigset_t            mask;                                           \
} _func##_arg;                                                          \
                                                                        \
static void *                                                           \
_func##_proc(void *arg)                                                 \
{                                                                       \
    tarpc_##_func##_in  *in = &(((_func##_arg *)arg)->in);              \
    tarpc_##_func##_out *out = &(((_func##_arg *)arg)->out);            \
    checked_arg         *list = NULL;                                   \
                                                                        \
    VERB("Entry thread %s", #_func);                                    \
                                                                        \
    { _actions }                                                        \
                                                                        \
    pthread_exit(arg);                                                  \
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
         (int)getpid(), (int)pthread_self(), #_func);                   \
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
        pthread_t _tid;                                                 \
                                                                        \
        if ((arg = malloc(sizeof(*arg))) == NULL)                       \
        {                                                               \
            out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);            \
            return TRUE;                                                \
        }                                                               \
                                                                        \
        arg->in = *in;                                                  \
        arg->out = *out;                                                \
                                                                        \
        if (pthread_create(&_tid, NULL, _func##_proc,                   \
                           (void *)arg) != 0)                           \
        {                                                               \
            free(arg);                                                  \
            out->common._errno = TE_RC(TE_TA_WIN32, errno);             \
        }                                                               \
                                                                        \
        memset(in, 0, sizeof(*in));                                     \
        memset(out, 0, sizeof(*out));                                   \
        out->common.tid = (int)_tid;                                    \
                                                                        \
        return TRUE;                                                    \
    }                                                                   \
                                                                        \
    if (pthread_join((pthread_t)in->common.tid, (void *)(&arg)) != 0)   \
    {                                                                   \
        out->common._errno = TE_RC(TE_TA_WIN32, errno);                 \
        return TRUE;                                                    \
    }                                                                   \
    if (arg == NULL)                                                    \
    {                                                                   \
        out->common._errno = TE_RC(TE_TA_WIN32, EINVAL);                \
        return TRUE;                                                    \
    }                                                                   \
    xdr_tarpc_##_func##_out((XDR *)&op, out);                           \
    *out = arg->out;                                                    \
    free(arg);                                                          \
    return TRUE;                                                        \
}

bool_t
_setlibname_1_svc(tarpc_setlibname_in *in, tarpc_setlibname_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);
    UNUSED(in);
    memset(out, 0, sizeof(*out));
    return TRUE;
}

/*-------------- fork() ---------------------------------*/
TARPC_FUNC(fork, {},
{
    MAKE_CALL(out->pid = fork());

    if (out->pid == 0)
    {
#ifdef HAVE_SVC_EXIT
        svc_exit();
#endif
        tarpc_server(in->name.name_val);
        exit(EXIT_FAILURE);
    }
}
)

/*-------------- pthread_create() -----------------------------*/
TARPC_FUNC(pthread_create, {},
{
    MAKE_CALL(out->retval = pthread_create((pthread_t *)&(out->tid), NULL,
                                           tarpc_server,
                                           strdup(in->name.name_val)));
}
)

/*-------------- pthread_cancel() -----------------------------*/
TARPC_FUNC(pthread_cancel, {},
{
    MAKE_CALL(out->retval = pthread_cancel((pthread_t)in->tid));
}
)

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
            out->common._errno = TE_RC(TE_TA_WIN32, EINVAL);
    }
    if (out->common._errno == 0)
    {
        MAKE_CALL(old_handler = (sighandler_t)signal(
                      signum_rpc2h(in->signum), handler));
        if (old_handler != SIG_ERR)
        {
            char *name = rcf_ch_symbol_name(old_handler);

            if (name != NULL)
            {
                if ((out->handler.handler_val = strdup(name)) == NULL)
                {
                    signal(signum_rpc2h(in->signum), old_handler);
                    out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
                }
                else
                    out->handler.handler_len = strlen(name) + 1;
            }
            else
            {
                if ((name = calloc(1, 16)) == NULL)
                {
                    signal(signum_rpc2h(in->signum), old_handler);
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

/*-------------- socket() ------------------------------*/

TARPC_FUNC(socket, {},
{
    MAKE_CALL(out->fd = WSASocket(domain_rpc2h(in->domain),
                                  socktype_rpc2h(in->type),
                                  proto_rpc2h(in->proto),
                                  (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                  0, in->flags ? WSA_FLAG_OVERLAPPED : 0));
}
)

/*-------------- close() ------------------------------*/

TARPC_FUNC(close, {}, { MAKE_CALL(out->retval = closesocket(in->fd)); })

/*-------------- bind() ------------------------------*/

TARPC_FUNC(bind, {},
{
    PREPARE_ADDR(in->addr, 0);
    MAKE_CALL(out->retval = bind(in->fd, a, in->len));
}
)

/*-------------- connect() ------------------------------*/

TARPC_FUNC(connect, {},
{
    PREPARE_ADDR(in->addr, 0);

    MAKE_CALL(out->retval = connect(in->fd, a, in->len));
}
)

/*-------------- ConnectEx() ------------------------------*/
TARPC_FUNC(connect_ex,
{
    COPY_ARG(len_sent);
},
{
    PREPARE_ADDR(in->addr, 0);
    if (in->overlapped != 0)
    {
        rpc_overlapped *overlapped = (rpc_overlapped *)(in->overlapped);

        if (buf2overlapped(overlapped, in->buf.buf_len,
                           in->buf.buf_val) != 0)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        }
        else
        {
            MAKE_CALL(out->retval =
                          (*pf_connect_ex)(in->fd, a, in->len,
                               overlapped->buffers[0].buf,
                               in->len_buf,
                               out->len_sent.len_sent_len == 0 ? NULL :
                                   (LPDWORD)(out->len_sent.len_sent_val),
                               (LPWSAOVERLAPPED)overlapped));
        }
    }
    else
    {
        MAKE_CALL(out->retval =
                      (*pf_connect_ex)(in->fd, a, in->len,
                                       in->buf.buf_len == 0 ? NULL :
                                           in->buf.buf_val,
                                       in->len_buf,
                                       out->len_sent.len_sent_len == 0 ?
                                           NULL :
                                           (LPDWORD)(out->len_sent.
                                                          len_sent_val),
                                       NULL));
    }
}
)

/*-------------- DisconnectEx() ------------------------------*/

TARPC_FUNC(disconnect_ex, {},
{
    LPWSAOVERLAPPED overlapped = NULL;

    if (in->overlapped != 0)
        overlapped = &(((rpc_overlapped *)(in->overlapped))->overlapped);
    MAKE_CALL(out->retval = (*pf_disconnect_ex)(in->fd, overlapped,
                                transmit_file_flags_rpc2h(in->flags), 0));
}
)

/*-------------- listen() ------------------------------*/

TARPC_FUNC(listen, {},
{
    MAKE_CALL(out->retval = listen(in->fd, in->backlog));
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

    MAKE_CALL(out->retval = accept(in->fd, a,
                                   out->len.len_len == 0 ? NULL :
                                   (int *)(out->len.len_val)));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- WSAAccept() ------------------------------*/

typedef struct accept_cond {
     unsigned short port;
     int            verdict;
} accept_cond;

static int CALLBACK
accept_callback(LPWSABUF caller_id, LPWSABUF caller_data, LPQOS sqos,
                LPQOS gqos, LPWSABUF callee_id, LPWSABUF callee_data,
                GROUP *g, DWORD_PTR user_data)
{
    accept_cond *cond = (accept_cond *)user_data;

    struct sockaddr_in *addr;

    UNUSED(caller_data);
    UNUSED(sqos);
    UNUSED(gqos);
    UNUSED(callee_id);
    UNUSED(callee_data);
    UNUSED(g);

    if (cond == NULL)
        return CF_ACCEPT;

    if (caller_id == NULL || caller_id->len == 0)
        return CF_REJECT;

    addr = (struct sockaddr_in *)(caller_id->buf);

    for (; cond->port != 0; cond++)
        if (cond->port == addr->sin_port)
               return cond->verdict;

    return CF_REJECT;
}

TARPC_FUNC(wsa_accept,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    accept_cond *cond = NULL;
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);


    if (in->cond.cond_len != 0)
    {
        unsigned int i;

        /* FIXME: memory allocated here is lost */
        if ((cond = calloc(in->cond.cond_len + 1,
                           sizeof(accept_cond))) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
            goto finish;
        }
        for (i = 0; i < in->cond.cond_len; i++)
        {
            cond[i].port = in->cond.cond_val[i].port;
            cond[i].verdict =
                in->cond.cond_val[i].verdict == TARPC_CF_ACCEPT ?
                    CF_ACCEPT :
                in->cond.cond_val[i].verdict == TARPC_CF_REJECT ?
                    CF_REJECT : CF_DEFER;
        }
    }

    MAKE_CALL(out->retval = WSAAccept(in->fd, a,
                                      out->len.len_len == 0 ? NULL :
                                      (int *)(out->len.len_val),
                                      (LPCONDITIONPROC)accept_callback,
                                      (DWORD)cond));
    sockaddr_h2rpc(a, &(out->addr));
    finish:
    ;
}
)

/*-------------- AcceptEx() ------------------------------*/

TARPC_FUNC(accept_ex,
{
    COPY_ARG(count);
},
{
    int             buflen;
    rpc_overlapped *overlapped;
    rpc_overlapped  tmp;

    buflen = in->buflen + 2 * (sizeof(struct sockaddr_storage) + 16);
    if (in->overlapped != 0)
    {
        overlapped = (rpc_overlapped *)(in->overlapped);
    }
    else
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (in->buflen > 0)
    {
        if (buf2overlapped(overlapped, buflen, NULL) != 0)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
            goto finish;
        }
    }
    MAKE_CALL(out->retval =
              (*pf_accept_ex)(in->fd, in->fd_a,
                              in->buflen == 0 ? NULL : 
                              overlapped->buffers[0].buf, 
                              in->buflen,
                              sizeof(struct sockaddr_storage) + 16,
                              sizeof(struct sockaddr_storage) + 16,
                              out->count.count_len == 0 ? NULL :
                              (LPDWORD)out->count.count_val, 
                              in->overlapped == 0 ? NULL :
                              (LPWSAOVERLAPPED)overlapped)); 
    if (overlapped == &tmp)
        rpc_overlapped_free_memory(overlapped);

    finish:
    ;
}
)

/*-------------- GetAcceptExSockAddr() ---------------------------*/

TARPC_FUNC(get_accept_addr,
{
    COPY_ARG_ADDR(laddr);
    COPY_ARG_ADDR(raddr);
},
{
    struct sockaddr *l_a = 0;
    struct sockaddr *r_a = 0 ;
    int               addrlen1 = 0;
    int               addrlen2 = 0;

    UNUSED(list);

    (*pf_get_accept_ex_sockaddrs)(in->buf.buf_val,
                                  in->buflen,
                                  sizeof(struct sockaddr_storage) + 16,
                                  sizeof(struct sockaddr_storage) + 16,
                                  &l_a, &addrlen1, &r_a, &addrlen2);
    sockaddr_h2rpc(l_a, &(out->laddr));
    sockaddr_h2rpc(r_a, &(out->raddr));
}
)


/*-------------- TransmitFile() ----------------------------*/

TARPC_FUNC(transmit_file, {},
{
    TRANSMIT_FILE_BUFFERS transmit_buffers;
    DWORD                 flags;
    HANDLE                file = NULL;

    rpc_overlapped *overlapped = NULL;

    memset(&transmit_buffers, 0, sizeof(transmit_buffers));
    if (in->head.head_len != 0)
         transmit_buffers.Head = in->head.head_val;
    transmit_buffers.HeadLength = in->head.head_len;
    if (in->tail.tail_len != 0)
         transmit_buffers.Tail = in->tail.tail_val;
    transmit_buffers.TailLength = in->tail.tail_len;

    if (in->overlapped != 0)
    {
        overlapped = (rpc_overlapped *)in->overlapped;
        rpc_overlapped_free_memory(overlapped);
        if ((overlapped->buffers = calloc(2, sizeof(WSABUF))) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
            goto finish;
        }
        overlapped->buffers[0].buf = in->head.head_val;
        in->head.head_val = NULL;
        overlapped->buffers[0].len = in->head.head_len;
        in->head.head_len = 0;
        overlapped->buffers[1].buf = in->tail.tail_val;
        in->tail.tail_val = NULL;
        overlapped->buffers[1].len = in->tail.tail_len;
        in->tail.tail_len = 0;
        overlapped->bufnum = 2;
    }

    flags = transmit_file_flags_rpc2h(in->flags);
    if (in->file.file_len != 0)
        file = CreateFile(in->file.file_val, FILE_READ_DATA,
                          FILE_SHARE_DELETE, NULL, OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    MAKE_CALL(out->retval =
        (*pf_transmit_file)(in->fd, file,
                            in->len, in->len_per_send,
                            (LPWSAOVERLAPPED)overlapped,
                            &transmit_buffers, flags));
    finish:
    ;
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

    MAKE_CALL(out->retval = recvfrom(in->fd, out->buf.buf_val, in->len,
                                     send_recv_flags_rpc2h(in->flags), a,
                                     out->fromlen.fromlen_len == 0 ? NULL :
                                     (int *)(out->fromlen.fromlen_val)));
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

    MAKE_CALL(out->retval = recv(in->fd, out->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
}
)

/*-------------- WSARecvEx() ------------------------------*/

TARPC_FUNC(wsa_recv_ex,
{
    COPY_ARG(buf);
    COPY_ARG(flags);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);
    if (out->flags.flags_len > 0)
        out->flags.flags_val[0] =
            send_recv_flags_rpc2h(out->flags.flags_val[0]);

    MAKE_CALL(out->retval = WSARecvEx(in->fd, in->len == 0 ? NULL :
                                      out->buf.buf_val,
                                      in->len,
                                      out->flags.flags_len == 0 ? NULL :
                                      (int *)(out->flags.flags_val)));

    if (out->flags.flags_len > 0)
        out->flags.flags_val[0] =
            send_recv_flags_h2rpc(out->flags.flags_val[0]);
}
)

/*-------------- shutdown() ------------------------------*/

TARPC_FUNC(shutdown, {},
{
    MAKE_CALL(out->retval = shutdown(in->fd, shut_how_rpc2h(in->how)));
}
)

/*-------------- sendto() ------------------------------*/

TARPC_FUNC(sendto, {},
{
    PREPARE_ADDR(in->to, 0);

    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = sendto(in->fd, in->buf.buf_val, in->len,
                                   send_recv_flags_rpc2h(in->flags),
                                   a, in->tolen));
}
)

/*-------------- send() ------------------------------*/

TARPC_FUNC(send, {},
{
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = send(in->fd, in->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
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

    MAKE_CALL(out->retval = getsockname(in->fd, a,
                                        out->len.len_len == 0 ? NULL :
                                        (int *)(out->len.len_val)));
                                        
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

    MAKE_CALL(out->retval = getpeername(in->fd, a,
                                        out->len.len_len == 0 ? NULL :
                                        (int *)(out->len.len_val)));
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
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
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

    FD_SET((unsigned int)in->fd, (fd_set *)(in->set));
    return TRUE;
}

/*-------------- FD_CLR --------------------------------*/

bool_t
_do_fd_clr_1_svc(tarpc_do_fd_clr_in *in, tarpc_do_fd_clr_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_SET((unsigned int)in->fd, (fd_set *)(in->set));
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

    MAKE_CALL(out->retval = select(in->n, (fd_set *)(in->readfds),
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

/*-------------- setsockopt() ------------------------------*/

TARPC_FUNC(setsockopt, {},
{
    if (in->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = setsockopt(in->s,
                                           socklevel_rpc2h(in->level),
                                           sockopt_rpc2h(in->optname),
                                           NULL, in->optlen));
    }
    else
    {
        char *opt;
        int   optlen;

        struct linger   linger;
        struct in_addr  addr;
        struct timeval  tv;
        
        if (in->optname == RPC_SO_SNDTIMEO || 
            in->optname == RPC_SO_RCVTIMEO)
        {
            static int optval;
            
            optval = 
                in->optval.optval_val[0].option_value_u.
                opt_timeval.tv_sec * 1000 + 
                in->optval.optval_val[0].option_value_u.
                opt_timeval.tv_usec / 1000;
            
            opt = (char *)&optval;
            optlen = sizeof(int);
            goto call_setsockopt;
        }

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

            case OPT_IPADDR:
            {
                opt = (char *)&addr;

                memcpy(&addr,
                       &(in->optval.optval_val[0].
                         option_value_u.opt_ipaddr),
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
        call_setsockopt:
        INIT_CHECKED_ARG(opt, optlen, 0);
        MAKE_CALL(out->retval = setsockopt(in->s,
                                           socklevel_rpc2h(in->level),
                                           sockopt_rpc2h(in->optname),
                                           opt, in->optlen));
    }
}
)

/*-------------- getsockopt() ------------------------------*/

TARPC_FUNC(getsockopt,
{
    COPY_ARG(optval);
    COPY_ARG(optlen);
},
{
    if (out->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = 
                      getsockopt(in->s, socklevel_rpc2h(in->level),
                      sockopt_rpc2h(in->optname),
                      NULL, (int *)(out->optlen.optlen_val)));
    }
    else
    {
        char opt[sizeof(struct linger)];

        memset(opt, 0, sizeof(opt));
        INIT_CHECKED_ARG(opt, sizeof(opt),
                         (out->optlen.optlen_val == NULL) ?
                            0 : *out->optlen.optlen_val);

        MAKE_CALL(out->retval = 
                      getsockopt(in->s, socklevel_rpc2h(in->level),
                                 sockopt_rpc2h(in->optname),
                                 opt, (int *)(out->optlen.optlen_val)));

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

            case OPT_IPADDR:
            {
                struct in_addr *addr = (struct in_addr *)opt;

                memcpy(&(out->optval.optval_val[0].
                         option_value_u.opt_ipaddr),
                       addr, sizeof(*addr));
                break;
            }

            case OPT_TIMEVAL:
            {
                if (in->optname == RPC_SO_SNDTIMEO || 
                    in->optname == RPC_SO_RCVTIMEO)
                {
                    int msec = *(int *)opt;
                    
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_sec = msec / 1000;
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_usec = (msec % 1000) * 1000;
                }
                else
                {
                    struct timeval *tv = (struct timeval *)opt;
                
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_sec = tv->tv_sec;
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_usec = tv->tv_usec;
                }
                break;
            }

            case OPT_STRING:
            {
                char *str = (char *)opt;

                memcpy(out->optval.optval_val[0].option_value_u.opt_string.
                       opt_string_val, str,
                       out->optval.optval_val[0].option_value_u.opt_string.
                       opt_string_len);
                break;
            }

            default:
                ERROR("incorrect option type %d is received",
                      out->optval.optval_val[0].opttype);
                break;
        }
    }
}
)

TARPC_FUNC(ioctl,
{
    COPY_ARG(req);
},
{
    char *req;
    int   reqlen = 0;

    static struct timeval req_timeval;
    static int            req_int;

    DWORD ret_num;

    if (out->req.req_val != NULL)
    {
        switch(out->req.req_val[0].type)
        {
            case IOCTL_TIMEVAL:
            {
                req = (char *)&req_timeval;
                reqlen = sizeof(struct timeval);
                req_timeval.tv_sec =
                    out->req.req_val[0].ioctl_request_u.req_timeval.tv_sec;
                req_timeval.tv_usec =
                    out->req.req_val[0].ioctl_request_u.req_timeval.tv_usec;

                break;
            }

            case IOCTL_INT:
            {
                req = (char *)&req_int;
                req_int = out->req.req_val[0].ioctl_request_u.req_int;
                reqlen = sizeof(int);
                break;
            }

            default:
                ERROR("incorrect request type %d is received",
                      out->req.req_val[0].type);
                out->common._errno = TE_RC(TE_TA_WIN32, EINVAL);
                goto finish;
                break;
        }
    }

    if (in->access == IOCTL_WR)
        INIT_CHECKED_ARG(req, reqlen, 0);
    MAKE_CALL(out->retval = WSAIoctl(in->s, ioctl_rpc2h(in->code), req,
                                     reqlen, req, reqlen, &ret_num,
                                     NULL, NULL));
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

           default: /* Should not occur */
               break;
        }
    }
    finish:
    ;
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

    MAKE_CALL(he = (struct hostent *)gethostbyname(in->name.name_val));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
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

    MAKE_CALL(he = (struct hostent *)gethostbyaddr(in->addr.val.val_val,
                                         in->addr.val.val_len,
                                          addr_family_rpc2h(in->type)));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        else
            out->res.res_len = 1;
    }
}
)

/*-------------- fopen() --------------------------------*/
TARPC_FUNC(fopen, {},
{
    MAKE_CALL(out->mem_ptr = (int)fopen(in->path.path_val,
                                        in->mode.mode_val));
}
)

/*-------------- fileno() --------------------------------*/
TARPC_FUNC(fileno, {},
{
    MAKE_CALL(out->fd = fileno((FILE *)in->mem_ptr));
}
)

/*-------------- getuid() --------------------------------*/
TARPC_FUNC(getuid, {}, { MAKE_CALL(out->uid = getuid()); })

/*-------------- geteuid() --------------------------------*/
TARPC_FUNC(geteuid, {}, { MAKE_CALL(out->uid = geteuid()); })

/*-------------- setuid() --------------------------------*/
TARPC_FUNC(setuid, {}, { MAKE_CALL(out->retval = setuid(in->uid)); })

/*-------------- seteuid() --------------------------------*/
TARPC_FUNC(seteuid, {}, { MAKE_CALL(out->retval = seteuid(in->uid)); })

/*-------------- simple_sender() -----------------------------*/
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
    char buf[1024];

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

    memset(buf, 0xDEADBEEF, sizeof(buf));

    for (start = now = time(NULL);
         now - start <= (time_t)in->time2run;
         now = time(NULL))
    {
        int len;

        if (!in->size_rnd_once)
            size = rand_range(in->size_min, in->size_max);

        if (!in->delay_rnd_once)
            delay = rand_range(in->delay_min, in->delay_max);

        if (delay / 1000000 > (time_t)in->time2run - (now - start) + 1)
            break;

        usleep(delay);

        len = send(in->s, buf, size, 0);

        if (len < 0)
        {
            ERROR("send() failed in simple_sender(): errno %x", errno);
            return -1;
        }

        if (len < size)
        {
            ERROR("send() returned %d instead %d in simple_sender()",
                  len, size);
            return -1;
        }

        sent += len;
#ifdef TA_DEBUG
        control += len;
        if (control > 0x20000)
        {
            char buf[128];

            sprintf(buf,
                    "echo \"Intermediate %llu time %d %d %d\" >> "
                    "/tmp/sender",
                    sent, now, start, in->time2run);
            system(buf);
            control = 0;
        }
#endif
    }

    {
        char buf[64] = {0,};
        sprintf(buf, "Sent %llu", sent);
        ERROR(buf);
    }

    out->bytes = sent;

    return 0;
}

TARPC_FUNC(simple_sender, {},
{
    MAKE_CALL(out->retval = simple_sender(in, out));
}
)

/*-------------- simple_receiver() --------------------------*/

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
    char buf[1024];

    uint64_t received = 0;
#ifdef TA_DEBUG
    uint64_t control = 0;
    int start = time(NULL);
#endif

    while (TRUE)
    {
        struct timeval tv = { 1, 0 };
        fd_set         set;
        int            len;

        FD_ZERO(&set);
        FD_SET((unsigned int)in->s, &set);
        if (select(in->s + 1, &set, NULL, NULL, &tv) < 0)
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

        len = recv(in->s, buf, sizeof(buf), 0);

        if (len < 0)
        {
            ERROR("recv() failed in simple_receiver(): errno %x", errno);
            return -1;
        }

        if (len == 0)
        {
            ERROR("recv() returned 0 in simple_receiver()");
            return -1;
        }

        received += len;
#ifdef TA_DEBUG
        control += len;
        if (control > 0x20000)
        {
            char buf[128];
            sprintf(buf,
                    "echo \"Intermediate %llu time %d\" >> /tmp/receiver",
                    received, time(NULL) - start);
            system(buf);
            control = 0;
        }
#endif
    }

    {
        char buf[64] = {0,};
        sprintf(buf, "Received %llu", received);
        ERROR(buf);
    }

    out->bytes = received;

    return 0;
}

TARPC_FUNC(simple_receiver, {},
{
    MAKE_CALL(out->retval = simple_receiver(in, out));
}
)

#define FLOODER_ECHOER_WAIT_FOR_RX_EMPTY        1
#define FLOODER_BUF                             4096

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
    int      *rcvrs = (int *)(in->rcvrs.rcvrs_val);
    int       rcvnum = in->rcvrs.rcvrs_len;
    int      *sndrs = (int *)(in->sndrs.sndrs_val);
    int       sndnum = in->sndrs.sndrs_len;
    int       bulkszs = in->bulkszs;
    int       time2run = in->time2run;
    te_bool   rx_nb = in->rx_nonblock;

    uint32_t *tx_stat = (uint32_t *)(in->tx_stat.tx_stat_val);
    uint32_t *rx_stat = (uint32_t *)(in->rx_stat.rx_stat_val);

    int      i;
    int      rc, err;
    int      sent;
    int      received;
    char     rcv_buf[FLOODER_BUF];
    char     snd_buf[FLOODER_BUF];

    fd_set          rfds0, wfds0;
    fd_set          rfds, wfds;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx = TRUE;

    memset(rcv_buf, 0x0, FLOODER_BUF);
    memset(snd_buf, 0xA, FLOODER_BUF);

    if (rx_nb)
    {
        u_long on = 1;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctlsocket(rcvrs[i], FIONBIO, &on)) != 0)
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
        FD_SET((unsigned int)rcvrs[i], &rfds0);
    }
    for (i = 0; i < sndnum; ++i)
    {
        if (sndrs[i] > max_descr)
            max_descr = sndrs[i];
        FD_SET((unsigned int)sndrs[i], &wfds0);
    }

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
              __FUNCTION__, errno);
        return -1;
    }
    timeout.tv_sec += time2run;

    call_timeout.tv_sec = time2run;
    call_timeout.tv_usec = 0;

    INFO("%s(): time2run=%d, timeout=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        if (time2run_not_expired)
        {
            wfds = wfds0;
        }
        else
        {
            FD_ZERO(&wfds);
            session_rx = FALSE;
        }
        rfds = rfds0;

        if ((rc = select(max_descr + 1, &rfds,
                         time2run_not_expired ? &wfds : NULL,
                         NULL, &call_timeout)) < 0)
        {
            ERROR("%s(): (p)select() failed: %d", __FUNCTION__, 
                  WSAGetLastError());
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
                    sent = send(sndrs[i], snd_buf, bulkszs, 0);
                    err = WSAGetLastError();
                    if (sent < 0 && err != WSAEWOULDBLOCK)
                    {
                        ERROR("%s(): write() failed: %d",
                              __FUNCTION__, err);
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
                received = recv(rcvrs[i], rcv_buf, sizeof(rcv_buf), 0);
                err = WSAGetLastError();
                if (received < 0 && err != WSAEWOULDBLOCK)
                {
                    ERROR("%s(): read() failed: %d", __FUNCTION__, err);
                    return -1;
                }
                if (received > 0)
                {
                    if (rx_stat != NULL)
                        rx_stat[i] += received;
                    if (!time2run_not_expired)
                        VERB("FD=%d Rx=%d", rcvrs[i], received);
                    session_rx = TRUE;
                }
            }
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
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            call_timeout.tv_usec = 0;
        }

    } while (time2run_not_expired || session_rx);

    if (rx_nb)
    {
        u_long off = 0;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctlsocket((unsigned int)rcvrs[i], FIONBIO, &off)) != 0)
            {
                ERROR("%s(): ioctl(FIONBIO) failed: %d",
                      __FUNCTION__, WSAGetLastError());
                return -1;
            }
        }
    }

    INFO("%s(): OK", __FUNCTION__);

    /* Clean up errno */
    errno = 0;

    return 0;
}

/*-------------- flooder() --------------------------*/
TARPC_FUNC(flooder, {},
{
    if (in->iomux != FUNC_SELECT)
    {
       ERROR("Unsipported iomux type for flooder");
       out->retval = TE_RC(TE_TA_WIN32, ENOTSUP);
       return 0;
    }
    MAKE_CALL(out->retval = flooder(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

/*-------------- echoer() --------------------------*/

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
    int *sockets = (int *)(in->sockets.sockets_val);
    int  socknum = in->sockets.sockets_len;
    int  time2run = in->time2run;

    uint32_t *tx_stat = (uint32_t *)(in->tx_stat.tx_stat_val);
    uint32_t *rx_stat = (uint32_t *)(in->rx_stat.rx_stat_val);

    int      i;
    int      rc;
    int      sent;
    int      received;
    char     buf[FLOODER_BUF];

    fd_set          rfds;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;

    memset(buf, 0x0, FLOODER_BUF);


    /* Calculate max descriptor */
    for (i = 0; i < socknum; i++)
    {
        if (sockets[i] > max_descr)
            max_descr = sockets[i];
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

        /* Prepare sets of file descriptors */
        FD_ZERO(&rfds);
        for (i = 0; i < socknum; i++)
            FD_SET((unsigned int)sockets[i], &rfds);

        if (select(max_descr + 1, &rfds, NULL, NULL, &call_timeout) < 0)
        {
            ERROR("%s(): select() failed: %X", __FUNCTION__, errno);
            return -1;
        }

        /* Receive data from sockets that are ready */
        for (i = 0; rc > 0 && i < socknum; i++)
        {
            if (FD_ISSET(sockets[i], &rfds))
            {
                received = recv(sockets[i], buf, sizeof(buf), 0);
                if (received < 0)
                {
                    ERROR("%s(): read() failed: %X", __FUNCTION__, errno);
                    return -1;
                }
                if (rx_stat != NULL)
                    rx_stat[i] += received;
                session_rx = TRUE;

                sent = send(sockets[i], buf, received, 0);
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

TARPC_FUNC(echoer, {},
{
    if (in->iomux != FUNC_SELECT)
    {
       ERROR("Unsipported iomux type for echoer");
       out->retval = TE_RC(TE_TA_WIN32, ENOTSUP);
       return 0;
    }
    MAKE_CALL(out->retval = echoer(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

/*-------------- socket_to_file() ----------------------------*/
#define SOCK2FILE_BUF_LEN  4096

/**
 * Routine which receives data from socket and write data
 * to specified path.
 *
 * @return -1 in the case of failure or some positive value in other cases
 */
int
socket_to_file(tarpc_socket_to_file_in *in)
{
    int      sock = in->sock;
    char    *path = in->path.path_val;
    long     time2run = in->timeout;

    int      rc = 0, err;
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
    te_bool         next_read;

    path[in->path.path_len] = '\0';
    INFO("socket_to_file() called with: sock=%d, path=%s, timeout=%ld",
         sock, path, time2run);
    {
        u_long on = 1;

        if ((ioctlsocket(sock, FIONBIO, &on)) != 0)
        {
            ERROR("%s(): ioctl(FIONBIO) failed: %X", __FUNCTION__, errno);
            rc = -1;
            goto local_exit;
        }
    }

    file_d = open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (file_d < 0)
    {
        ERROR("%s(): open(%s, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO) "
              "failed: %X", __FUNCTION__, path, errno);
        rc = -1;
        goto local_exit;
    }
    INFO("socket_to_file(): file %s opened with descriptor=%d",
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

    INFO("%s(): time2run=%ld, timeout=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        /* Prepare sets of file descriptors */
        FD_ZERO(&rfds);
        FD_SET((unsigned int)sock, &rfds);

        rc = select(sock + 1, &rfds, NULL, NULL, &call_timeout);
        if (rc < 0)
        {
            ERROR("%s(): select() failed: %X", __FUNCTION__, errno);
            rc = -1;
            goto local_exit;
        }
        INFO("socket_to_file(): select finishes for waiting of events");

        /* Receive data from socket that are ready */
        if (FD_ISSET(sock, &rfds))
        {
            next_read = TRUE;
            do {
                INFO("socket_to_file(): select observes data for "
                     "reading on the socket=%d", sock);
                received = recv(sock, buffer, sizeof(buffer), 0);
                err = WSAGetLastError();
                if (received < 0 && err != WSAEWOULDBLOCK)
                {
                    ERROR("%s(): read() failed: %d", __FUNCTION__, err);
                    rc = -1;
                    goto local_exit;
                }
                INFO("socket_to_file(): read() retrieve %d bytes",
                     received);
                if (received < 0)
                {
                    next_read = FALSE;
                }
                else
                {
                    total += received;
                    INFO("socket_to_file(): write retrieved data to file");
                    written = write(file_d, buffer, received);
                    INFO("socket_to_file(): %d bytes are written to file",
                         written);
                    if (written < 0)
                    {
                        ERROR("%s(): write() failed: %d", 
                              __FUNCTION__, err);
                        rc = -1;
                        goto local_exit;
                    }
                    if (written != received)
                    {
                        ERROR("%s(): write() cannot write all received in "
                              "the buffer data to the file "
                              "(received=%d, written=%d): %X",
                              __FUNCTION__, errno, received, written);
                        rc = -1;
                        goto local_exit;
                    }
                }
            } while (next_read);
        }

        if (time2run_not_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed): %X",
                      __FUNCTION__, errno);
                rc = -1;
                goto local_exit;
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
    } while (time2run_not_expired);

local_exit:
    INFO("%s(): %s", __FUNCTION__, (rc == 0)? "OK" : "FAILED");

    if (file_d != -1)
        close(file_d);

    {
        u_long off = 1;

        if ((ioctlsocket(sock, FIONBIO, &off)) != 0)
        {
            ERROR("%s(): ioctl(FIONBIO, off) failed: %X",
                  __FUNCTION__, errno);
            rc = -1;
        }
    }
    /* Clean up errno */
    if (!rc)
    {
        errno = 0;
        rc = total;
    }
    return rc;
}

TARPC_FUNC(socket_to_file, {},
{
   MAKE_CALL(out->retval = socket_to_file(in));
}
)

/*-------------- WSACreateEvent ----------------------------*/

TARPC_FUNC(create_event, {},
{
    UNUSED(list);
    UNUSED(in);
    out->retval = (tarpc_wsaevent)WSACreateEvent();
}
)

/*-------------- WSACloseEvent ----------------------------*/

TARPC_FUNC(close_event, {},
{
    UNUSED(list);
    out->retval = WSACloseEvent((HANDLE)(in->hevent));
}
)

/*-------------- WSAResetEvent ----------------------------*/

TARPC_FUNC(reset_event, {},
{
    UNUSED(list);
    out->retval = WSAResetEvent((HANDLE)(in->hevent));
}
)

/*-------------- WSASetEvent ----------------------------*/
TARPC_FUNC(set_event, {},
{
    UNUSED(list);
    out->retval = WSASetEvent((HANDLE)(in->hevent));
}
)

/*-------------- WSAEventSelect ----------------------------*/

TARPC_FUNC(event_select, {},
{
    UNUSED(list);
    out->retval = WSAEventSelect(in->fd, (WSAEVENT)in->event_object,
                                 network_event_rpc2h(in->event));
}
)

/*-------------- WSAEnumNetworkEvents ----------------------------*/

TARPC_FUNC(enum_network_events,
{
    COPY_ARG(event);
},
{
    WSANETWORKEVENTS  events_occured;

    UNUSED(list);
    out->retval = WSAEnumNetworkEvents(in->fd, (WSAEVENT)in->event_object,
                                       out->event.event_len == 0 ? NULL :
                                       &events_occured);
    if (out->event.event_len != 0)
        out->event.event_val[0] =
            network_event_h2rpc(events_occured.lNetworkEvents);
}
)
/*-------------- CreateWindow ----------------------------*/

LRESULT CALLBACK
message_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg > WM_USER)
        PRINT("Unexpected message %d is received", uMsg - WM_USER);

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool_t
_create_window_1_svc(tarpc_create_window_in *in,
                     tarpc_create_window_out *out,
                     struct svc_req *rqstp)
{
    static te_bool init = FALSE;

    UNUSED(rqstp);
    UNUSED(in);

    memset(out, 0, sizeof(*out));

    /* Should not be called in thread to prevent double initialization */

    if (!init)
    {
        WNDCLASSEX wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = (WNDPROC)message_callback;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = ta_hinstance;
        wcex.hIcon = NULL;
        wcex.hCursor = NULL;
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = "MainWClass";
        wcex.hIconSm = NULL;

        if (!RegisterClassEx(&wcex))
        {
            ERROR("Failed to register class\n");
            out->hwnd = 0;
            return TRUE;
        }
        init = TRUE;
    }

    out->hwnd = (tarpc_hwnd)CreateWindow("MainWClass", "tawin32",
                                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                         0, CW_USEDEFAULT, 0, NULL, NULL,
                                         ta_hinstance, NULL);
    return TRUE;
}

/*-------------- DestroyWindow ----------------------------*/

TARPC_FUNC(destroy_window, {},
{
    UNUSED(out);
    UNUSED(list);
    DestroyWindow((HWND)(in->hwnd));
}
)

/*-------------- WSAAsyncSelect ---------------------------*/
bool_t
_wsa_async_select_1_svc(tarpc_wsa_async_select_in *in,
                        tarpc_wsa_async_select_out *out,
                        struct svc_req *rqstp)
{
    /*
     * Should not be called in thread to avoid troubles with threads
     * when message is retrieved.
     */

    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    out->retval = WSAAsyncSelect(in->sock, (HWND)(in->hwnd), WM_USER + 1,
                                 network_event_rpc2h(in->event));
    return TRUE;
}

/*-------------- PeekMessage ---------------------------------*/
bool_t
_peek_message_1_svc(tarpc_peek_message_in *in,
                   tarpc_peek_message_out *out,
                   struct svc_req *rqstp)
{
    MSG msg;

    /*
     * Should not be called in thread to avoid troubles with threads
     * when message is retrieved.
     */

    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    while ((out->retval = PeekMessage(&msg, (HWND)(in->hwnd),
                                      0, 0, PM_REMOVE)) != 0 &&
           msg.message != WM_USER + 1);

    if (out->retval != 0)
    {
        out->sock = msg.wParam;
        out->event = network_event_h2rpc(msg.lParam);
    }

    return TRUE;
}

/*-------------- Create WSAOVERLAPPED --------------------------*/

TARPC_FUNC(create_overlapped, {},
{
    rpc_overlapped *tmp;

    UNUSED(list);
    if ((tmp = (rpc_overlapped *)calloc(1, sizeof(rpc_overlapped))) == 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
    }
    else
    {
        tmp->overlapped.hEvent = (WSAEVENT)(in->hevent);
        tmp->overlapped.Offset = in->offset;
        tmp->overlapped.OffsetHigh = in->offset_high;
        out->retval = (tarpc_overlapped)tmp;
    }
}
)

/*-------------- Delete WSAOVERLAPPED ----------------------------*/

TARPC_FUNC(delete_overlapped, {},
{
    UNUSED(list);
    UNUSED(out);
    rpc_overlapped_free_memory((rpc_overlapped *)(in->overlapped));
    free((void *)(in->overlapped));
}
)

/*-------------- Completion callback-related staff ---------------------*/
static int completion_called = 0;
static int completion_error = 0;
static int completion_bytes = 0;
static tarpc_overlapped completion_overlapped = 0;
static pthread_mutex_t completion_lock = PTHREAD_MUTEX_INITIALIZER;

static void CALLBACK
completion_callback(DWORD error, DWORD bytes, LPWSAOVERLAPPED overlapped,
                    DWORD flags)
{
    UNUSED(flags);
    pthread_mutex_lock(&completion_lock);
    completion_called++;
    completion_error = win_error_h2rpc(error);
    completion_bytes = bytes;
    completion_overlapped = (tarpc_overlapped)overlapped;
    pthread_mutex_unlock(&completion_lock);
}

TARPC_FUNC(completion_callback, {},
{
    UNUSED(list);
    UNUSED(in);
    pthread_mutex_lock(&completion_lock);
    out->called = completion_called;
    completion_called = 0;
    out->bytes = completion_bytes;
    completion_bytes = 0;
    out->error = completion_error;
    out->overlapped = completion_overlapped;
    pthread_mutex_unlock(&completion_lock);
}
)

/*-------------- WSASend() ------------------------------*/
TARPC_FUNC(wsa_send,
{
    COPY_ARG(bytes_sent);
},
{
    rpc_overlapped *overlapped = NULL;
    rpc_overlapped  tmp;

    if (in->overlapped != 0)
    {
        overlapped = (rpc_overlapped *)(in->overlapped);
    }
    else
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        goto finish;
    }

    MAKE_CALL(out->retval =
                  WSASend(in->s, overlapped->buffers, in->count,
                          out->bytes_sent.bytes_sent_len == 0 ? NULL :
                          (LPDWORD)(out->bytes_sent.bytes_sent_val),
                          send_recv_flags_rpc2h(in->flags),
                          in->overlapped == 0 ? NULL
                                  : (LPWSAOVERLAPPED)overlapped,
                          in->callback ?
                          (LPWSAOVERLAPPED_COMPLETION_ROUTINE)
                              completion_callback : NULL));

    if (in->overlapped == 0 || out->retval >= 0 ||
        out->common.win_error != RPC_WSA_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }
    finish:
    ;
}
)

/*-------------- WSARecv() ------------------------------*/
TARPC_FUNC(wsa_recv,
{
    COPY_ARG(bytes_received);
    COPY_ARG(flags);
},
{
    rpc_overlapped *overlapped;
    rpc_overlapped  tmp;

    if (in->overlapped != 0)
    {
        overlapped = (rpc_overlapped *)(in->overlapped);
    }
    else
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        goto finish;
    }
    if (out->flags.flags_len > 0)
        out->flags.flags_val[0] =
            send_recv_flags_rpc2h(out->flags.flags_val[0]);
    MAKE_CALL(out->retval =
                  WSARecv(in->s, overlapped->buffers, in->count,
                      out->bytes_received.bytes_received_len == 0 ?
                        NULL :
                        (LPDWORD)(out->bytes_received.bytes_received_val),
                      out->flags.flags_len > 0 ?
                        (LPDWORD)(out->flags.flags_val) : NULL,
                      in->overlapped == 0 ? NULL :
                        (LPWSAOVERLAPPED)overlapped,
                      in->callback ?
                        (LPWSAOVERLAPPED_COMPLETION_ROUTINE)
                          completion_callback : NULL));
    if (out->retval >= 0)
    {
        overlapped2iovec(overlapped, &(out->vector.vector_len),
                     &(out->vector.vector_val));
        if (out->flags.flags_len > 0)
            out->flags.flags_val[0] =
                send_recv_flags_h2rpc(out->flags.flags_val[0]);
    }
    else if (in->overlapped == 0 ||
             out->common.win_error != RPC_WSA_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }
    finish:
    ;
}
)

TARPC_FUNC(get_overlapped_result,
{
    COPY_ARG(bytes);
    COPY_ARG(flags);
},
{
    rpc_overlapped *overlapped = (rpc_overlapped *)(in->overlapped);

    UNUSED(list);
    MAKE_CALL(out->retval =
                  WSAGetOverlappedResult(in->s,
                                         in->overlapped == 0 ?
                                         NULL : (LPWSAOVERLAPPED)overlapped,
                                         out->bytes.bytes_len == 0 ? NULL :
                                         (LPDWORD)(out->bytes.bytes_val),
                                         in->wait,
                                         out->flags.flags_len > 0 ?
                                         (LPDWORD)(out->flags.flags_val) :
                                         NULL));
    if (out->retval)
    {
        if (out->flags.flags_len > 0)
            out->flags.flags_val[0] =
                send_recv_flags_h2rpc(out->flags.flags_val[0]);
        overlapped2iovec(overlapped, &out->vector.vector_len,
                         &out->vector.vector_val);
    }
}
)

/*-------------- getpid() --------------------------------*/
TARPC_FUNC(getpid, {}, { MAKE_CALL(out->retval = getpid()); })

/*-------------- WSADuplicateSocket() ---------------------------*/
TARPC_FUNC(duplicate_socket,
{
    if (in->info.info_len != 0 &&
        in->info.info_len < sizeof(WSAPROTOCOL_INFO))
    {
        ERROR("Too short buffer for protocol info is provided");
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        return TRUE;
    }
    COPY_ARG(info);
},
{
    MAKE_CALL(out->retval =
                  WSADuplicateSocket(in->s, in->pid,
                      out->info.info_len == 0 ? NULL :
                      (LPWSAPROTOCOL_INFO)(out->info.info_val)));
    out->info.info_len = sizeof(WSAPROTOCOL_INFO);
}
)

/*-------------- WSAWaitForMultipleEvents() -------------------------*/
TARPC_FUNC(wait_multiple_events, {},
{
    INIT_CHECKED_ARG((char *)(in->events.events_val),
                     in->events.events_len * sizeof(tarpc_wsaevent),
                     in->count * sizeof(tarpc_wsaevent));

    MAKE_CALL(out->retval =
                  WSAWaitForMultipleEvents(in->count,
                      (WSAEVENT *)(in->events.events_val),
                      in->wait_all, in->timeout, in->alertable));
    switch (out->retval)
    {
        case WSA_WAIT_FAILED:
            out->retval = TARPC_WSA_WAIT_FAILED;
            break;

        case WAIT_IO_COMPLETION:
            out->retval = TARPC_WAIT_IO_COMPLETION;
            break;

        case WSA_WAIT_TIMEOUT:
            out->retval = TARPC_WSA_WAIT_TIMEOUT;
            break;

        default:
            out->retval = TARPC_WSA_WAIT_EVENT_0 +
                         (out->retval - WSA_WAIT_EVENT_0);
    }
}
)


/*----------------- WSASendTo() -------------------------*/
TARPC_FUNC(wsa_send_to,
{
    COPY_ARG(bytes_sent);
},
{
    rpc_overlapped *overlapped = NULL;
    rpc_overlapped  tmp;

    if (in->overlapped != 0)
    {
        overlapped = (rpc_overlapped *)(in->overlapped);
    }
    else
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        goto finish;
    }

    PREPARE_ADDR(in->to, 0);

    MAKE_CALL(out->retval =
        WSASendTo(in->s,
            overlapped->buffers,
            in->count,
            out->bytes_sent.bytes_sent_len == 0 ? NULL :
                (LPDWORD)(out->bytes_sent.bytes_sent_val),
            send_recv_flags_rpc2h(in->flags),
            a,
            in->tolen,
            in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
            in->callback ?
                (LPWSAOVERLAPPED_COMPLETION_ROUTINE)completion_callback :
                    NULL));


    if (in->overlapped == 0 || out->retval >= 0 ||
        out->common.win_error != RPC_WSA_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }

    finish:
    ;
}
)

/*----------------- WSARecvFrom() -------------------------*/
TARPC_FUNC(wsa_recv_from,
{
    COPY_ARG(bytes_received);
    COPY_ARG(flags);
    COPY_ARG(fromlen);
    COPY_ARG_ADDR(from);
},
{
    rpc_overlapped *overlapped;
    rpc_overlapped  tmp;

    if (in->overlapped != 0)
    {
        overlapped = (rpc_overlapped *)(in->overlapped);
    }
    else
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        goto finish;
    }
    if (out->flags.flags_len > 0)
        out->flags.flags_val[0] =
            send_recv_flags_rpc2h(out->flags.flags_val[0]);

    PREPARE_ADDR(out->from, out->fromlen.fromlen_len == 0 ? 0 :
                                    *out->fromlen.fromlen_val);

    MAKE_CALL(out->retval =
        WSARecvFrom(in->s,
            overlapped->buffers,
            in->count,
            out->bytes_received.bytes_received_len == 0 ?
                NULL : (LPDWORD)(out->bytes_received.bytes_received_val),
            out->flags.flags_len > 0 ?
                (LPDWORD)(out->flags.flags_val) : NULL,
            a,
            out->fromlen.fromlen_len == 0 ? NULL :
                (LPINT)out->fromlen.fromlen_val,
            in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
            in->callback ? (LPWSAOVERLAPPED_COMPLETION_ROUTINE)
                completion_callback : NULL));

    if (out->retval >= 0)
    {
        overlapped2iovec(overlapped, &(out->vector.vector_len),
                         &(out->vector.vector_val));
        if (out->flags.flags_len > 0)
            out->flags.flags_val[0] =
                send_recv_flags_h2rpc(out->flags.flags_val[0]);

        sockaddr_h2rpc(a, &(out->from));    
    }
    else if (in->overlapped == 0 ||
             out->common.win_error != RPC_WSA_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }

    finish:
    ;
}
)

/*----------------- WSASendDisconnect() -------------------------*/
TARPC_FUNC(wsa_send_disconnect, {},
{
    rpc_overlapped  tmp;
    rpc_overlapped *overlapped = &tmp;

    memset(&tmp, 0, sizeof(tmp));

    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        goto finish;
    }
    
    MAKE_CALL(out->retval = WSASendDisconnect(in->s, overlapped->buffers));

    if (out->retval >= 0)
        rpc_overlapped_free_memory(overlapped);

    finish:
    ;
}
)

/*----------------- WSARecvDisconnect() -------------------------*/
TARPC_FUNC(wsa_recv_disconnect, {},
{
    rpc_overlapped  tmp;
    rpc_overlapped *overlapped = &tmp;

    memset(&tmp, 0, sizeof(tmp));

    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        goto finish;
    }

    MAKE_CALL(out->retval = WSARecvDisconnect(in->s, overlapped->buffers));

    if (out->retval >= 0)
    {
        overlapped2iovec(overlapped, &(out->vector.vector_len),
                         &(out->vector.vector_val));
    }
    else
        rpc_overlapped_free_memory(overlapped);

    finish:
    ;
}
)

/* WSARecvMsg */
/*--------------- WSARecvMsg -----------------------------*/
TARPC_FUNC(wsa_recv_msg,
{
    if (in->msg.msg_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        return TRUE;
    }
    COPY_ARG(msg);
    COPY_ARG(bytes_received);
},
{
    WSABUF               buf_arr[RCF_RPC_MAX_IOVEC];
    WSAMSG               msg;
    WSAOVERLAPPED        *overlapped;
    struct tarpc_msghdr  *rpc_msg;
    unsigned int         i;

    memset(buf_arr, 0, sizeof(buf_arr));

    if (in->overlapped != 0)
    {
        overlapped = (WSAOVERLAPPED *)(in->overlapped);
    }
    else
    {
        overlapped = NULL;
    }

    rpc_msg = out->msg.msg_val;

    if (rpc_msg == NULL)
    {
        MAKE_CALL(out->retval =
                  (*pf_wsa_recvmsg)(in->s, NULL,
                      out->bytes_received.bytes_received_len == 0 ?
                        NULL :
                        (LPDWORD)(out->bytes_received.bytes_received_val),
                      overlapped,
                      in->callback ? (LPWSAOVERLAPPED_COMPLETION_ROUTINE)
                        completion_callback : NULL));
    }
    else
    {
        memset(&msg, 0, sizeof(msg));

        PREPARE_ADDR(rpc_msg->msg_name, rpc_msg->msg_namelen);
        msg.namelen = rpc_msg->msg_namelen;
        msg.name = a;

        msg.dwBufferCount = rpc_msg->msg_iovlen;
        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                INIT_CHECKED_ARG(
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_len,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len);
                buf_arr[i].buf =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val;
                buf_arr[i].len =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len;
            }
            msg.lpBuffers = buf_arr;

            INIT_CHECKED_ARG((char *)buf_arr, sizeof(buf_arr), 0);
        }

        INIT_CHECKED_ARG(rpc_msg->msg_control.msg_control_val,
                         rpc_msg->msg_control.msg_control_len,
                         rpc_msg->msg_controllen);
        msg.Control.buf = rpc_msg->msg_control.msg_control_val;
        msg.Control.len = rpc_msg->msg_controllen;

        msg.dwFlags = send_recv_flags_rpc2h(rpc_msg->msg_flags);

        /*
         * msg_name, msg_iov, msg_iovlen and msg_control MUST NOT be
         * changed.
         *
         * msg_namelen, msg_controllen and msg_flags MAY be changed.
         */
        INIT_CHECKED_ARG((char *)&msg.name, sizeof(msg.name), 0);
        INIT_CHECKED_ARG((char *)&msg.lpBuffers, sizeof(msg.lpBuffers), 0);
        INIT_CHECKED_ARG((char *)&msg.dwBufferCount,
                         sizeof(msg.dwBufferCount), 0);
        INIT_CHECKED_ARG((char *)&msg.Control, sizeof(msg.Control), 0);
            
        MAKE_CALL(out->retval =
                  (*pf_wsa_recvmsg)(in->s, &msg,
                      out->bytes_received.bytes_received_len == 0 ?
                        NULL :
                        (LPDWORD)(out->bytes_received.bytes_received_val),
                      overlapped,
                      in->callback ? (LPWSAOVERLAPPED_COMPLETION_ROUTINE)
                        completion_callback : NULL));

        sockaddr_h2rpc(a, &(rpc_msg->msg_name));
        rpc_msg->msg_namelen = msg.namelen;
        rpc_msg->msg_controllen = msg.Control.len;
        rpc_msg->msg_flags = send_recv_flags_h2rpc(msg.dwFlags);
    }
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
    MAKE_CALL(out->retval = sigemptyset((sigset_t *)(in->set)));
}
)

/*-------------- sigpendingt() ------------------------------*/

TARPC_FUNC(sigpending, {},
{
    MAKE_CALL(out->retval = sigpending((sigset_t *)(in->set)));
}
)

/*-------------- sigsuspend() ------------------------------*/

TARPC_FUNC(sigsuspend, {},
{
    MAKE_CALL(out->retval = sigsuspend((sigset_t *)(in->set)));
}
)

/*-------------- sigfillset() ------------------------------*/

TARPC_FUNC(sigfillset, {},
{
    MAKE_CALL(out->retval = sigfillset((sigset_t *)(in->set)));
}
)

/*-------------- sigaddset() -------------------------------*/

TARPC_FUNC(sigaddset, {},
{
    MAKE_CALL(out->retval = sigaddset((sigset_t *)(in->set), 
                                      signum_rpc2h(in->signum)));
}
)

/*-------------- sigdelset() -------------------------------*/

TARPC_FUNC(sigdelset, {},
{
    MAKE_CALL(out->retval = sigdelset((sigset_t *)(in->set), 
                                      signum_rpc2h(in->signum)));
}
)

/*-------------- sigismember() ------------------------------*/

TARPC_FUNC(sigismember, {},
{
    INIT_CHECKED_ARG((char *)(in->set), sizeof(sigset_t), 0);
    MAKE_CALL(out->retval = sigismember((sigset_t *)(in->set), 
                                        signum_rpc2h(in->signum)));
}
)

/*-------------- kill() --------------------------------*/

TARPC_FUNC(kill, {},
{
    MAKE_CALL(out->retval = kill(in->pid, signum_rpc2h(in->signum)));
}
)

/*-------------- overfill_buffers() -----------------------------*/
int
overfill_buffers(tarpc_overfill_buffers_in *in,
                 tarpc_overfill_buffers_out *out)
{
    ssize_t    rc = 0;
    int        err = 0;
    size_t     max_len = 1;
    uint8_t   *buf = NULL;
    uint64_t   total = 0;
    int        unchanged = 0;
    u_long     val = 1;

    out->bytes = 0;
    
    buf = calloc(1, max_len);
    if (buf == NULL)
    {
        ERROR("%s(): No enough memory", __FUNCTION__);
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        rc = -1;
        goto overfill_buffers_exit;
    }

    memset(buf, 0xDEADBEEF, sizeof(max_len));
    
    /* ATTENTION! socket is assumed in blocking state */
    
    if (ioctlsocket(in->sock, FIONBIO, &val) < 0)
    {
        err = WSAGetLastError();
        rc = -1;
        ERROR("%s(): Failed to move socket to non-blocking state", 
              __FUNCTION__);
        goto overfill_buffers_exit;
    }

    do {
        do {
            rc = send(in->sock, buf, max_len, 0);
            err = WSAGetLastError();
            
            if (rc == -1 && err != WSAEWOULDBLOCK)
            {
                ERROR("%s(): send() failed", __FUNCTION__);
                goto overfill_buffers_exit;
            }
            if (rc != -1)
                out->bytes += rc;
            else
                Sleep(100);
        } while (err != WSAEWOULDBLOCK);

        if (total != out->bytes)
        {
            total = out->bytes;
            unchanged = 0;
        }
        else
        {
            unchanged++;
        }
        rc = 0;
        err = 0;
    } while (unchanged != 3);
    
overfill_buffers_exit:
    val = 0;
    if (ioctlsocket(in->sock, FIONBIO, &val) < 0)
    {
        err = WSAGetLastError();
        rc = -1;
        ERROR("%s(): Failed to move socket back to blocking state", 
              __FUNCTION__);
    }
    out->common.win_error = win_error_h2rpc(err);
    out->common._errno = wsaerr2errno(out->common.win_error); 

    free(buf);
    return rc;
}

TARPC_FUNC(overfill_buffers,{},
{
    MAKE_CALL(out->retval = overfill_buffers(in, out));
}
)

/*-------------- WSAAddressToString ---------------------*/
TARPC_FUNC(wsa_address_to_string,
{
    COPY_ARG(addrstr);
    COPY_ARG(addrstr_len);
},
{
    PREPARE_ADDR(in->addr, 0);

    MAKE_CALL(out->retval = WSAAddressToString(a, in->addrlen,
                                (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                out->addrstr.addrstr_val,
                                out->addrstr_len.addrstr_len_val));
}
)

/*-------------- WSAStringToAddress ---------------------*/
TARPC_FUNC(wsa_string_to_address,
{
    COPY_ARG(addrlen);
},
{
    PREPARE_ADDR(out->addr, out->addrlen.addrlen_len == 0 ? 0 :
                                    *out->addrlen.addrlen_val);

    MAKE_CALL(out->retval = WSAStringToAddress(in->addrstr.addrstr_val,
                                domain_rpc2h(in->address_family),
                                (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                a,
                                (LPINT)out->addrlen.addrlen_val));

    if (out->retval == 0)
        sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- WSACancelAsyncRequest ------------------*/
TARPC_FUNC(wsa_cancel_async_request, {},
{
    MAKE_CALL(out->retval = WSACancelAsyncRequest(
                                (HANDLE)in->async_task_handle));
}
)

/**
 * Allocate a single buffer of specified size and return a pointer to it.
 */
TARPC_FUNC(alloc_buf, {},
{
    void *buf;
    
    UNUSED(list);
    buf = calloc(1, in->size);

    if (buf == NULL)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        out->retval = (tarpc_ptr)NULL;
    }
    else
        out->retval = (tarpc_ptr)buf;
}
)

/**
 * Free a previously allocated buffer.
 */
TARPC_FUNC(free_buf, {},
{
    UNUSED(list);
    UNUSED(out);
    free((void *)in->buf);
}
)

/**
 * Fill in the buffer.
 */
TARPC_FUNC(set_buf, {},
{
    UNUSED(list);
    UNUSED(out);
    if ((in->src_buf.src_buf_len != 0) && ((void*)in->dst_buf != NULL))
    {
        memcpy((void*)in->dst_buf, in->src_buf.src_buf_val,
                                  in->src_buf.src_buf_len);
    }
}
)

TARPC_FUNC(get_buf, {},
{
    UNUSED(list);
    if (((char*)in->src_buf != NULL) && (in->len != 0))
    {
        out->dst_buf.dst_buf_val = (char*)in->src_buf;
        out->dst_buf.dst_buf_len = in->len;
    }
    else
    {
        out->dst_buf.dst_buf_val = NULL;
        out->dst_buf.dst_buf_len = 0;
    }
}
)

/**
 * Allocate a single WSABUF structure and a buffer of specified size;
 * set the fields of the allocated structure according to the allocated
 * buffer pointer and length. Return two pointers: to the structure and
 * to the buffer.
 */
TARPC_FUNC(alloc_wsabuf, {},
{
    WSABUF *wsabuf;
    void *buf;

    UNUSED(list);
    wsabuf = malloc(sizeof(WSABUF));
    if (in->len != 0)
        buf = calloc(1, in->len);
    else
        buf = NULL;

    if ((wsabuf == NULL) || ((buf == NULL) && (in->len != 0)))
    {
        if (wsabuf != NULL)
            free(wsabuf);
        if (buf != NULL)
            free(buf);
        out->common._errno = TE_RC(TE_TA_WIN32, ENOMEM);
        out->wsabuf = (tarpc_ptr)NULL;
        out->wsabuf_buf = (tarpc_ptr)NULL;
        out->retval = -1;
    }
    else
    {
        wsabuf->buf = buf;
        wsabuf->len = in->len;
        out->wsabuf = (tarpc_ptr)wsabuf;
        out->wsabuf_buf = (tarpc_ptr)buf;
        out->retval = 0;
    }
}
)

/**
 * Free a previously allocated WSABUF structure and its buffer.
 */
TARPC_FUNC(free_wsabuf, {},
{
    WSABUF *wsabuf;

    UNUSED(list);
    UNUSED(out);
    wsabuf = (WSABUF*)in->wsabuf;
    if ((void*)wsabuf->buf != NULL)
        free((void*)wsabuf->buf);
    free((void*)wsabuf);
}
)

/*-------------- WSAConnect -----------------------------*/
TARPC_FUNC(wsa_connect, {},
{
    QOS sqos;
    QOS *psqos = &sqos;
    tarpc_flowspec *fs;

    PREPARE_ADDR(in->addr, 0);

    memset(&sqos, 0, sizeof(sqos));
    
    if ((in->sending.sending_len != 0) && (in->sending.sending_val != NULL))
    {
        fs = in->sending.sending_val;
        sqos.SendingFlowspec.TokenRate = fs->TokenRate;
        sqos.SendingFlowspec.TokenBucketSize = fs->TokenBucketSize;
        sqos.SendingFlowspec.PeakBandwidth = fs->PeakBandwidth;
        sqos.SendingFlowspec.Latency = fs->Latency;
        sqos.SendingFlowspec.DelayVariation = fs->DelayVariation;
        sqos.SendingFlowspec.ServiceType =
            servicetype_flags_rpc2h(fs->ServiceType);
        sqos.SendingFlowspec.MaxSduSize = fs->MaxSduSize;
        sqos.SendingFlowspec.MinimumPolicedSize = fs->MinimumPolicedSize;
    }

    if ((in->receiving.receiving_len != 0)
        && (in->receiving.receiving_val != NULL))
    {
        fs = in->receiving.receiving_val;
        sqos.ReceivingFlowspec.TokenRate = fs->TokenRate;
        sqos.ReceivingFlowspec.TokenBucketSize = fs->TokenBucketSize;
        sqos.ReceivingFlowspec.PeakBandwidth = fs->PeakBandwidth;
        sqos.ReceivingFlowspec.Latency = fs->Latency;
        sqos.ReceivingFlowspec.DelayVariation = fs->DelayVariation;
        sqos.ReceivingFlowspec.ServiceType =
            servicetype_flags_rpc2h(fs->ServiceType);
        sqos.ReceivingFlowspec.MaxSduSize = fs->MaxSduSize;
        sqos.ReceivingFlowspec.MinimumPolicedSize = fs->MinimumPolicedSize;
    }
    
    sqos.ProviderSpecific.buf = (char*)in->provider_specific_buf;
    sqos.ProviderSpecific.len = in->provider_specific_buf_len;

    if ((in->sending.sending_val == NULL)
        && (in->receiving.receiving_val == NULL)
        && ((char*)in->provider_specific_buf == NULL)
        && (in->provider_specific_buf_len == 0))
    {
        psqos = NULL;
    }

    MAKE_CALL(out->retval = WSAConnect(in->s, a, in->addrlen,
                                (LPWSABUF)in->caller_wsabuf,
                                (LPWSABUF)in->callee_wsabuf,
                                psqos, NULL));
}
)

/*-------------- WSAAsyncGetHostByAddr ------------------*/
TARPC_FUNC(wsa_async_get_host_by_addr, {},
{
    MAKE_CALL(out->retval = (tarpc_handle)WSAAsyncGetHostByAddr(
                                (HWND)in->hwnd, in->wmsg, in->addr.addr_val,
                                in->addrlen, addr_family_rpc2h(in->type),
                                (char*)in->buf, in->buflen));
}
)

/*-------------- WSAAsyncGetHostByName ------------------*/
TARPC_FUNC(wsa_async_get_host_by_name, {},
{
    MAKE_CALL(out->retval = (tarpc_handle)WSAAsyncGetHostByName(
                                (HWND)in->hwnd, in->wmsg, in->name.name_val,
                                (char*)in->buf, in->buflen));
}
)

/*-------------- WSAAsyncGetProtoByName -----------------*/
TARPC_FUNC(wsa_async_get_proto_by_name, {},
{
    MAKE_CALL(out->retval = (tarpc_handle)WSAAsyncGetProtoByName(
                                (HWND)in->hwnd, in->wmsg, in->name.name_val,
                                (char*)in->buf, in->buflen));
}
)

/*-------------- WSAAsyncGetProtoByNumber ---------------*/
TARPC_FUNC(wsa_async_get_proto_by_number, {},
{
    MAKE_CALL(out->retval = (tarpc_handle)WSAAsyncGetProtoByNumber(
                                (HWND)in->hwnd, in->wmsg, in->number,
                                (char*)in->buf, in->buflen));
}
)

/*-------------- WSAAsyncGetServByName ---------------*/
TARPC_FUNC(wsa_async_get_serv_by_name, {},
{
    MAKE_CALL(out->retval = (tarpc_handle)WSAAsyncGetServByName(
                                (HWND)in->hwnd, in->wmsg,
                                in->name.name_val, in->proto.proto_val,
                                (char*)in->buf, in->buflen));
}
)

/*-------------- WSAAsyncGetServByPort ---------------*/
TARPC_FUNC(wsa_async_get_serv_by_port, {},
{
    MAKE_CALL(out->retval = (tarpc_handle)WSAAsyncGetServByPort(
                                (HWND)in->hwnd, in->wmsg, in->port,
                                in->proto.proto_val, (char*)in->buf,
                                in->buflen));
}
)
