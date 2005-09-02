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
#include <pwd.h>

#ifdef _WINSOCK_H
struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};
#endif

#undef ERROR
#include "te_defs.h"
#include "te_errno.h"
#include "rcf_pch.h"
#include "logger_ta.h"
#include "rcf_ch_api.h"
#include "rcf_rpc_defs.h"
#include "te_rpc_types.h"

#define PRINT(msg...) \
    do {                                                \
       printf(msg); printf("\n"); fflush(stdout);       \
    } while (0)

/** Obtain RCF RPC errno code */
#define RPC_ERRNO errno_h2rpc(errno)

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
static int 
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

static int 
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

extern sigset_t rpcs_received_signals;
extern HINSTANCE ta_hinstance;

static LPFN_CONNECTEX            pf_connect_ex = NULL;
static LPFN_DISCONNECTEX         pf_disconnect_ex = NULL;
static LPFN_ACCEPTEX             pf_accept_ex = NULL;
static LPFN_GETACCEPTEXSOCKADDRS pf_get_accept_ex_sockaddrs = NULL;
static LPFN_TRANSMITFILE         pf_transmit_file = NULL;
static LPFN_WSARECVMSG           pf_wsa_recvmsg = NULL;

static te_bool init = FALSE;

#define IN_HWND         ((HWND)(rcf_pch_mem_get(in->hwnd)))
#define IN_HEVENT       ((WSAEVENT)(rcf_pch_mem_get(in->hevent)))
#define IN_OVERLAPPED   ((rpc_overlapped *)rcf_pch_mem_get(in->overlapped))
#define IN_SIGSET       ((sigset_t *)(rcf_pch_mem_get(in->set)))
#define IN_FDSET        ((fd_set *)(rcf_pch_mem_get(in->set)))

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

static void 
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
        return TE_ENOMEM;
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
    if (!init)                                                          \
    {                                                                   \
        wsa_func_handles_discover();                                    \
        init = TRUE;                                                    \
    }                                                                   \
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
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);            \
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
            out->common._errno = TE_OS_RC(TE_TA_WIN32, errno);          \
        }                                                               \
                                                                        \
        memset(in, 0, sizeof(*in));                                     \
        memset(out, 0, sizeof(*out));                                   \
        out->common.tid = rcf_pch_mem_alloc(_tid);                      \
        out->common.done = rcf_pch_mem_alloc(&arg->done);               \
                                                                        \
        return TRUE;                                                    \
    }                                                                   \
                                                                        \
    if (pthread_join((pthread_t)rcf_pch_mem_get(in->common.tid),        \
                     (void *)(&arg)) != 0)                              \
    {                                                                   \
        out->common._errno = TE_OS_RC(TE_TA_WIN32, errno);              \
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
    rcf_pch_mem_free(in->common.tid);                                   \
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
        rcf_pch_detach();
        rcf_pch_rpc_server(in->name.name_val);
        exit(EXIT_FAILURE);
    }
}
)

/*-------------- pthread_create() -----------------------------*/
TARPC_FUNC(pthread_create, {},
{
    MAKE_CALL(out->retval = pthread_create((pthread_t *)&(out->tid), NULL,
                                           (void *)rcf_pch_rpc_server,
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
    static rcf_pch_mem_id id = 0;
    
    UNUSED(in);
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    
    if (id == 0)
        id = rcf_pch_mem_alloc(&rpcs_received_signals);
    out->set = id;

    return TRUE;
}

/*-------------- signal() --------------------------------*/

typedef void (*sighandler_t)(int);

TARPC_FUNC(signal,
{
    if (in->signum == RPC_SIGINT)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EPERM);
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
            out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
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
                    out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
                }
                else
                    out->handler.handler_len = strlen(name) + 1;
            }
            else
            {
                if ((name = calloc(1, 16)) == NULL)
                {
                    signal(signum_rpc2h(in->signum), old_handler);
                    out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
                                  wsp_proto_rpc2h(in->type, in->proto),
                                  (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                  0, in->flags ? WSA_FLAG_OVERLAPPED : 0));
}
)

/*-------------- close() ------------------------------*/

TARPC_FUNC(close, {}, 
{ 
    MAKE_CALL(out->retval = closesocket(in->fd)); 
    if (out->retval == -1 && out->common._errno == RPC_ENOTSOCK)
        out->retval = close(in->fd); /* For files */
})

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
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    
    PREPARE_ADDR(in->addr, 0);
    if (overlapped != NULL)
    {
        if (buf2overlapped(overlapped, in->buf.buf_len,
                           in->buf.buf_val) != 0)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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

    if (IN_OVERLAPPED != NULL)
        overlapped = &(IN_OVERLAPPED->overlapped);
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
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    buflen = in->buflen + 2 * (sizeof(struct sockaddr_storage) + 16);
    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (in->buflen > 0)
    {
        if (buf2overlapped(overlapped, buflen, NULL) != 0)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
    TRANSMIT_FILE_BUFFERS  transmit_buffers;
    rpc_overlapped        *overlapped = IN_OVERLAPPED;

    memset(&transmit_buffers, 0, sizeof(transmit_buffers));
    if (in->head.head_len != 0)
         transmit_buffers.Head = in->head.head_val;
    transmit_buffers.HeadLength = in->head.head_len;
    if (in->tail.tail_len != 0)
         transmit_buffers.Tail = in->tail.tail_val;
    transmit_buffers.TailLength = in->tail.tail_len;

    if (overlapped != NULL)
    {
        rpc_overlapped_free_memory(overlapped);
        if ((overlapped->buffers = calloc(2, sizeof(WSABUF))) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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

    MAKE_CALL( out->retval = (*pf_transmit_file)(in->fd,
                              (HANDLE)rcf_pch_mem_get(in->file),
                              in->len, in->len_per_send,
                              (LPWSAOVERLAPPED)overlapped,
                              &transmit_buffers,
                              transmit_file_flags_rpc2h(in->flags)); );
    finish:
    ;
}
)

/*----------- TransmitFile(), 2nd version ------------------*/

TARPC_FUNC(transmitfile_tabufs, {},
{
    TRANSMIT_FILE_BUFFERS  transmit_buffers;
    rpc_overlapped        *overlapped = IN_OVERLAPPED;

    memset(&transmit_buffers, 0, sizeof(transmit_buffers));
    transmit_buffers.Head = rcf_pch_mem_get(in->head);
    transmit_buffers.HeadLength = in->head_len;
    transmit_buffers.Tail = rcf_pch_mem_get(in->tail);
    transmit_buffers.TailLength = in->tail_len;

    if (overlapped != NULL)
    {
        rpc_overlapped_free_memory(overlapped);
        if ((overlapped->buffers = calloc(2, sizeof(WSABUF))) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
            goto finish;
        }
        overlapped->buffers[0].buf = rcf_pch_mem_get(in->head);
        overlapped->buffers[0].len = in->head_len;
        overlapped->buffers[1].buf = rcf_pch_mem_get(in->tail);
        overlapped->buffers[1].len = in->tail_len;
        overlapped->bufnum = 2;
    }

    MAKE_CALL( out->retval = (*pf_transmit_file)(in->s,
                              (HANDLE)rcf_pch_mem_get(in->file),
                              in->len, in->bytes_per_send,
                              (LPWSAOVERLAPPED)overlapped,
                              &transmit_buffers,
                              transmit_file_flags_rpc2h(in->flags)); );
    finish:
    ;
}
)

/*------------------------- CreateFile() -------------------------
 *------ Attention: not all flags currently are supported. ------*/
static inline DWORD cf_access_right_rpc2h(unsigned int ar)
{
    return (!!(ar & RPC_CF_GENERIC_EXECUTE) * GENERIC_EXECUTE) |
           (!!(ar & RPC_CF_GENERIC_READ) * GENERIC_READ) |
           (!!(ar & RPC_CF_GENERIC_WRITE) * GENERIC_WRITE);
}

static inline DWORD cf_share_mode_rpc2h(unsigned int sm)
{
    return (!!(sm & RPC_CF_FILE_SHARE_DELETE) * FILE_SHARE_DELETE) |
           (!!(sm & RPC_CF_FILE_SHARE_READ) * FILE_SHARE_READ) |
           (!!(sm & RPC_CF_FILE_SHARE_WRITE) * FILE_SHARE_WRITE);
}

static inline DWORD cf_creation_disposition_rpc2h(unsigned int cd)
{
    return (!!(cd & RPC_CF_CREATE_ALWAYS) * CREATE_ALWAYS) |
           (!!(cd & RPC_CF_CREATE_NEW) * CREATE_NEW) |
           (!!(cd & RPC_CF_OPEN_ALWAYS) * OPEN_ALWAYS) |
           (!!(cd & RPC_CF_OPEN_EXISTING) * OPEN_EXISTING) |
           (!!(cd & RPC_CF_TRUNCATE_EXISTING) * TRUNCATE_EXISTING);
}

static inline DWORD cf_flags_attributes_rpc2h(unsigned int fa)
{
    return (!!(fa & RPC_CF_FILE_ATTRIBUTE_NORMAL) * FILE_ATTRIBUTE_NORMAL);
}

TARPC_FUNC(create_file, {},
{
    HANDLE handle = NULL;

    MAKE_CALL(handle = CreateFile(in->name.name_val,
        cf_access_right_rpc2h(in->desired_access),
        cf_share_mode_rpc2h(in->share_mode),
        (LPSECURITY_ATTRIBUTES)rcf_pch_mem_get(in->security_attributes),
        cf_creation_disposition_rpc2h(in->creation_disposition),
        cf_flags_attributes_rpc2h(in->flags_attributes),
        (HANDLE)rcf_pch_mem_get(in->template_file))
    );
    
    if (handle != NULL)
        out->handle = (tarpc_handle)rcf_pch_mem_alloc((void *)handle);
    else
        out->handle = (tarpc_handle)NULL;
}
)

/*-------------- CloseHandle() --------------*/
TARPC_FUNC(close_handle, {},
{
    UNUSED(list);
    out->retval = CloseHandle((HANDLE)rcf_pch_mem_get(in->handle));
    rcf_pch_mem_free(in->handle);
}
)

/*-------------- HasOverlappedIoCompleted() --------------*/
TARPC_FUNC(has_overlapped_io_completed, {},
{
    UNUSED(list);
    MAKE_CALL(out->retval =
            HasOverlappedIoCompleted((LPWSAOVERLAPPED)IN_OVERLAPPED));
}
)

/*-------------- GetCurrentProcessId() -------------------*/

TARPC_FUNC(get_current_process_id, {},
{
    UNUSED(in);
    UNUSED(list);
    UNUSED(in);
    out->retval = GetCurrentProcessId();
}
)

/* Get various system information */
TARPC_FUNC(get_sys_info, {},
{
    MEMORYSTATUS ms;
    SYSTEM_INFO si;

    UNUSED(in);
    UNUSED(list);

    memset(&ms, 0, sizeof(ms));
    GlobalMemoryStatus(&ms);
    out->ram_size = ms.dwTotalPhys;

    memset(&si, 0, sizeof(si));
    GetSystemInfo(&si);
    out->page_size = si.dwPageSize;
    out->number_of_processors = si.dwNumberOfProcessors;
}
)

/*-------------------- VM trasher --------------------*/

static pthread_mutex_t vm_trasher_lock = PTHREAD_MUTEX_INITIALIZER;
/* Is set to TRUE when the VM trasher thread must exit */
static volatile te_bool vm_trasher_stop = FALSE;
/* VM trasher thread identifier */
static pthread_t vm_trasher_thread_id = (pthread_t)-1;

void *vm_trasher_thread(void *arg)
{
    MEMORYSTATUS    ms;
    SIZE_T          len;
    SIZE_T          pos;
    double          dpos;
    char            *buf;
    struct timeval  tv;

    UNUSED(arg);

    memset(&ms, 0, sizeof(ms));
    GlobalMemoryStatus(&ms);

    len = ms.dwTotalPhys / 2 * 3; /* 1.5 RAM */
    buf = malloc(len);
    if (buf == NULL)
    {
        INFO("vm_trasher_thread() could not allocate "
             "%u bytes, errno = %d", len, errno);
        return (void *)-1;
    }

    /* Make dirty each page of buffer */
    for (pos = 0; pos < len; pos += 4096)
        buf[pos] = 0x5A;

    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    
    /* Perform VM trashing to keep memory pressure */
    while (vm_trasher_stop == FALSE)
    {
        /* Choose a random page */
        dpos = (double)rand() / (double)RAND_MAX * (double)(len / 4096 - 1);
        /* Read and write a byte of the chosen page */
        buf[(SIZE_T)dpos * 4096] |= 0x5A;    
    }
    
    free(buf);
    return (void *)0;
}

TARPC_FUNC(vm_trasher, {},
{
    UNUSED(list);
    UNUSED(out);

    pthread_mutex_lock(&vm_trasher_lock);

    if (in->start)
    {
        /* If the VM trasher thread is not started yet */
        if (vm_trasher_thread_id == (pthread_t)-1)
        {
            /* Start the VM trasher thread */
            pthread_create(&vm_trasher_thread_id, NULL,
                              vm_trasher_thread, NULL);
        }
    }
    else
    {
        /* If the VM trasher thread is already started */
        if (vm_trasher_thread_id != (pthread_t)-1)
        {
            /* Stop the VM trasher thread */
            vm_trasher_stop = TRUE;
            /* Wait for VM trasher thread exit */
            if (pthread_join(vm_trasher_thread_id, NULL) != 0)
            {
                INFO("vm_trasher: pthread_join() returned "
                              "non-zero, errno = ", errno);
            }
            /* Allow another one VM trasher thread to start later */
            vm_trasher_stop = FALSE;
            vm_trasher_thread_id = (pthread_t)-1;
        }
    }

    pthread_mutex_unlock(&vm_trasher_lock);
}
)

/*-------------- rpc_write_at_offset() -------------------*/

TARPC_FUNC(write_at_offset, {},
{
    off_t offset = in->offset;
    
    MAKE_CALL(
        out->offset = lseek(in->fd, offset, SEEK_SET);
        if (out->offset != (off_t)-1)
            out->written = write(in->fd, in->buf.buf_val, in->buf.buf_len);
    );
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }
    else
    {
        out->common._errno = RPC_ERRNO;
        out->retval = rcf_pch_mem_alloc(set);
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
    free(IN_FDSET);
    rcf_pch_mem_free(in->set);
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

    FD_ZERO(IN_FDSET);
    return TRUE;
}

/*-------------- FD_SET --------------------------------*/

bool_t
_do_fd_set_1_svc(tarpc_do_fd_set_in *in, tarpc_do_fd_set_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_SET((unsigned int)in->fd, IN_FDSET);
    return TRUE;
}

/*-------------- FD_CLR --------------------------------*/

bool_t
_do_fd_clr_1_svc(tarpc_do_fd_clr_in *in, tarpc_do_fd_clr_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_SET((unsigned int)in->fd, IN_FDSET);
    return TRUE;
}

/*-------------- FD_ISSET --------------------------------*/

bool_t
_do_fd_isset_1_svc(tarpc_do_fd_isset_in *in, tarpc_do_fd_isset_out *out,
                   struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    out->retval = FD_ISSET(in->fd, IN_FDSET);
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

    MAKE_CALL(out->retval = select(in->n, 
                                   (fd_set *)rcf_pch_mem_get(in->readfds),
                                   (fd_set *)rcf_pch_mem_get(in->writefds),
                                   (fd_set *)rcf_pch_mem_get(in->exceptfds),
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
                out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
                out->retval = -1;
                goto finish;
                break;
        }
        call_setsockopt:
        INIT_CHECKED_ARG(opt, optlen, 0);
        if (in->optlen == RPC_OPTLEN_AUTO)
            in->optlen = optlen;
        MAKE_CALL(out->retval = setsockopt(in->s,
                                           socklevel_rpc2h(in->level),
                                           sockopt_rpc2h(in->optname),
                                           opt, in->optlen));
    }
    finish:
    ;
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
        
        if (out->optlen.optlen_val != NULL &&
            *(out->optlen.optlen_val) == RPC_OPTLEN_AUTO)
        {
            switch (out->optval.optval_val[0].opttype)
            {
                case OPT_INT:
                    *(out->optlen.optlen_val) = sizeof(int);
                    break;
                    
                case OPT_LINGER:
                    *(out->optlen.optlen_val) = sizeof(struct linger);
                    break;

                case OPT_IPADDR:
                    *(out->optlen.optlen_val) = sizeof(struct in_addr);
                    break;

                case OPT_TIMEVAL:
                    *(out->optlen.optlen_val) = sizeof(struct timeval);
                    break;

                default:
                    ERROR("incorrect option type %d is received",
                          out->optval.optval_val[0].opttype);
                    break;
            }
        }

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
    char *req = NULL;
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
                out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
                out->retval = -1;
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
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
            out->res.res_len = 1;
    }
}
)

/*-------------- open() --------------------------------*/
TARPC_FUNC(open, {},
{
    MAKE_CALL(out->fd = open((in->path.path_len == 0) ? NULL :
                                 in->path.path_val,
                             fcntl_flags_rpc2h(in->flags),
                             file_mode_flags_rpc2h(in->mode)));
}
)

/*-------------- fopen() --------------------------------*/
TARPC_FUNC(fopen, {},
{
    MAKE_CALL(out->mem_ptr = (int)fopen(in->path.path_val,
                                        in->mode.mode_val));
}
)

/*-------------- fclose() -------------------------------*/
TARPC_FUNC(fclose, {},
{
    MAKE_CALL(out->retval = fclose((FILE *)in->mem_ptr));
}
)

/*-------------- fileno() --------------------------------*/
TARPC_FUNC(fileno, {},
{
    MAKE_CALL(out->fd = fileno((FILE *)in->mem_ptr));
}
)

/*-------------- getpwnam() --------------------------------*/
#define PUT_STR(_field) \
        do {                                                            \
            out->passwd._field._field##_val = strdup(pw->pw_##_field);  \
            if (out->passwd._field._field##_val == NULL)                \
            {                                                           \
                out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);        \
                goto finish;                                            \
            }                                                           \
            out->passwd._field._field##_len =                           \
                strlen(out->passwd._field._field##_val) + 1;            \
        } while (0)
TARPC_FUNC(getpwnam, {}, 
{ 
    struct passwd *pw;
    
    MAKE_CALL(pw = getpwnam(in->name.name_val));
    
    if (pw != NULL)
    {
            
        PUT_STR(name);
        PUT_STR(passwd);
        out->passwd.uid = pw->pw_uid;
        out->passwd.gid = pw->pw_gid;
        PUT_STR(gecos);
        PUT_STR(dir);
        PUT_STR(shell);

    } 
    finish:
    if (out->common._errno != 0)
    {
        free(out->passwd.name.name_val);
        free(out->passwd.passwd.passwd_val);
        free(out->passwd.gecos.gecos_val);
        free(out->passwd.dir.dir_val);
        free(out->passwd.shell.shell_val);
        memset(&(out->passwd), 0, sizeof(out->passwd));
    }
    ;
}
)
#undef PUT_STR

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

    memset(buf, 0xAB, sizeof(buf));

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
            int err = 0;
            if (in->ignore_err)
                continue;
            err = WSAGetLastError();
            ERROR("send() failed in simple_sender(): errno %d", err);
            return -1;
        }

        if (len < size)
        {
            if (in->ignore_err)
                continue;
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
        RING(buf);
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
            int err = 0;
            
            err = WSAGetLastError();
            ERROR("select() failed in simple_receiver(): errno %d", err);
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
            int err = 0;
            
            err = WSAGetLastError();
            ERROR("recv() failed in simple_receiver(): errno %d", err);
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
                ERROR("%s(): ioctl(FIONBIO) failed: %d",
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
                ERROR("%s(): gettimeofday(timestamp) failed): %d",
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
        ERROR("%s(): gettimeofday(timeout) failed: %d",
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
            ERROR("%s(): select() failed: %d", __FUNCTION__, errno);
            return -1;
        }

        /* Receive data from sockets that are ready */
        for (i = 0; i < socknum; i++)
        {
            if (FD_ISSET(sockets[i], &rfds))
            {
                received = recv(sockets[i], buf, sizeof(buf), 0);
                if (received < 0)
                {
                    ERROR("%s(): read() failed: %d", __FUNCTION__, errno);
                    return -1;
                }
                if (rx_stat != NULL)
                    rx_stat[i] += received;
                session_rx = TRUE;

                sent = send(sockets[i], buf, received, 0);
                if (sent < 0)
                {
                    ERROR("%s(): write() failed: %d",
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
                ERROR("%s(): gettimeofday(timestamp) failed: %d",
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
            ERROR("%s(): ioctl(FIONBIO) failed: %d", __FUNCTION__, errno);
            rc = -1;
            goto local_exit;
        }
    }

    file_d = open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (file_d < 0)
    {
        ERROR("%s(): open(%s, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO) "
              "failed: %d", __FUNCTION__, path, errno);
        rc = -1;
        goto local_exit;
    }
    INFO("socket_to_file(): file %s opened with descriptor=%d",
         path, file_d);

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
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
            ERROR("%s(): select() failed: %d", __FUNCTION__, errno);
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
                              "(received=%d, written=%d): %d",
                              __FUNCTION__, received, written, errno);
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
                ERROR("%s(): gettimeofday(timestamp) failed): %d",
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
            ERROR("%s(): ioctl(FIONBIO, off) failed: %d",
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
    out->retval = rcf_pch_mem_alloc(WSACreateEvent());
}
)

/*-------------- WSACloseEvent ----------------------------*/

TARPC_FUNC(close_event, {},
{
    UNUSED(list);
    out->retval = WSACloseEvent(IN_HEVENT);
    rcf_pch_mem_free(in->hevent);
}
)

/*-------------- WSAResetEvent ----------------------------*/

TARPC_FUNC(reset_event, {},
{
    UNUSED(list);
    out->retval = WSAResetEvent(IN_HEVENT);
}
)

/*-------------- WSASetEvent ----------------------------*/
TARPC_FUNC(set_event, {},
{
    UNUSED(list);
    out->retval = WSASetEvent(IN_HEVENT);
}
)

/*-------------- WSAEventSelect ----------------------------*/

TARPC_FUNC(event_select, {},
{
    UNUSED(list);
    out->retval = WSAEventSelect(in->fd, IN_HEVENT,
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
    out->retval = WSAEnumNetworkEvents(in->fd, IN_HEVENT,
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

    out->hwnd = rcf_pch_mem_alloc(
                    CreateWindow("MainWClass", "tawin32",
                                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                 0, CW_USEDEFAULT, 0, NULL, NULL,
                                 ta_hinstance, NULL));
    return TRUE;
}

/*-------------- DestroyWindow ----------------------------*/

TARPC_FUNC(destroy_window, {},
{
    UNUSED(out);
    UNUSED(list);
    DestroyWindow(IN_HWND);
    rcf_pch_mem_free(in->hwnd);
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
    out->retval = WSAAsyncSelect(in->sock, IN_HWND, WM_USER + 1,
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

    while ((out->retval = PeekMessage(&msg, IN_HWND,
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }
    else
    {
        tmp->overlapped.hEvent = IN_HEVENT;
        tmp->overlapped.Offset = in->offset;
        tmp->overlapped.OffsetHigh = in->offset_high;
        out->retval = rcf_pch_mem_alloc(tmp);
    }
}
)

/*-------------- Delete WSAOVERLAPPED ----------------------------*/

TARPC_FUNC(delete_overlapped, {},
{
    UNUSED(list);
    UNUSED(out);
    
    rpc_overlapped_free_memory(IN_OVERLAPPED);
    free(IN_OVERLAPPED);
    rcf_pch_mem_free(in->overlapped);
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
    completion_overlapped = 
        (tarpc_overlapped)rcf_pch_mem_get_id(overlapped);
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
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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

     
    if ((out->retval >= 0) || (out->common.win_error == RPC_WSAEMSGSIZE))
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
    rpc_overlapped *overlapped = IN_OVERLAPPED;

    UNUSED(list);
    MAKE_CALL(out->retval =
                  WSAGetOverlappedResult(in->s,
                                         (LPWSAOVERLAPPED)overlapped,
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

        if (in->get_data)
        {
            overlapped2iovec(overlapped, &out->vector.vector_len,
                             &out->vector.vector_val);
        }
        else
        {
            out->vector.vector_val = NULL;
            out->vector.vector_len = 0;
        }
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
#define MULTIPLE_EVENTS_MAX     128
TARPC_FUNC(wait_multiple_events, 
{
    if (in->events.events_len > MULTIPLE_EVENTS_MAX)
    {
        ERROR("Too many events are awaited");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        return TRUE;
    }
},
{
    WSAEVENT events[MULTIPLE_EVENTS_MAX];
    uint32_t i;
    
    for (i = 0; i < in->events.events_len; i++)
        events[i] = rcf_pch_mem_get(in->events.events_val[i]);
    
    INIT_CHECKED_ARG((char *)events, sizeof(events), 0);

    MAKE_CALL(out->retval = WSAWaitForMultipleEvents(in->events.events_len,
                                                     events,
                                                     in->wait_all, 
                                                     in->timeout, 
                                                     in->alertable));
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
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
    
    if ((out->retval >= 0) || (out->common.win_error == RPC_WSAEMSGSIZE))
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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

/*--------------- WSARecvMsg() -----------------------------*/
TARPC_FUNC(wsa_recv_msg,
{
    if (in->msg.msg_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(msg);
    COPY_ARG(bytes_received);
},
{
    WSAMSG                msg;
    rpc_overlapped       *overlapped = IN_OVERLAPPED;
    rpc_overlapped        tmp;
    struct tarpc_msghdr  *rpc_msg;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;    
    }

    rpc_msg = out->msg.msg_val;

    if (rpc_msg == NULL)
    {
        MAKE_CALL(out->retval =
            (*pf_wsa_recvmsg)(in->s, NULL,
                out->bytes_received.bytes_received_len == 0 ? NULL :
                    (LPDWORD)(out->bytes_received.bytes_received_val),
                in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
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
            if (iovec2overlapped(overlapped, rpc_msg->msg_iov.msg_iov_len,
                                 rpc_msg->msg_iov.msg_iov_val) != 0)
            {
                out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
                goto finish;
            }
            msg.lpBuffers = overlapped->buffers;
        }

        if (rpc_msg->msg_control.msg_control_len > 0)
        {
            ERROR("Non-zero Control is not supported");
            out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
            goto finish;
        }

        msg.dwFlags = send_recv_flags_rpc2h(rpc_msg->msg_flags);

        /*
         * msg_name, msg_iov, msg_iovlen and msg_control MUST NOT be
         * changed.
         * msg_namelen, msg_controllen and msg_flags MAY be changed.
         */
        INIT_CHECKED_ARG((char *)&msg.name, sizeof(msg.name), 0);
        INIT_CHECKED_ARG((char *)&msg.lpBuffers, sizeof(msg.lpBuffers), 0);
        INIT_CHECKED_ARG((char *)&msg.dwBufferCount,
                         sizeof(msg.dwBufferCount), 0);
        INIT_CHECKED_ARG((char *)&msg.Control, sizeof(msg.Control), 0);
            
        MAKE_CALL(out->retval =
            (*pf_wsa_recvmsg)(in->s, &msg,
                out->bytes_received.bytes_received_len == 0 ? NULL :
                    (LPDWORD)(out->bytes_received.bytes_received_val),
                in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
                in->callback ? (LPWSAOVERLAPPED_COMPLETION_ROUTINE)
                    completion_callback : NULL));

        if (out->retval >= 0)
        {
            unsigned int i;

            /* Free the current buffers of vector */
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                free(rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val);
                rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val =
                                                                       NULL;
            }

            /* Make the buffers of overlapped structure
             * to become the buffers of vector */
            overlapped2iovec(overlapped, &(rpc_msg->msg_iov.msg_iov_len),
                             &(rpc_msg->msg_iov.msg_iov_val));

            sockaddr_h2rpc(a, &(rpc_msg->msg_name));
            rpc_msg->msg_namelen = msg.namelen;
            rpc_msg->msg_flags = send_recv_flags_h2rpc(msg.dwFlags);
        }
        else if (in->overlapped == 0 ||
                 out->common.win_error != RPC_WSA_IO_PENDING)
        {
            rpc_overlapped_free_memory(overlapped);
        }
    }

    finish:
    ;
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }
    else
    {
        out->common._errno = RPC_ERRNO;
        out->set = rcf_pch_mem_alloc(set);
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
    free(rcf_pch_mem_get(in->set));
    rcf_pch_mem_free(in->set);
    out->common._errno = RPC_ERRNO;

    return TRUE;
}

/*-------------- sigemptyset() ------------------------------*/

TARPC_FUNC(sigemptyset, {},
{
    MAKE_CALL(out->retval = sigemptyset(IN_SIGSET));
}
)

/*-------------- sigpendingt() ------------------------------*/

TARPC_FUNC(sigpending, {},
{
    MAKE_CALL(out->retval = sigpending(IN_SIGSET));
}
)

/*-------------- sigsuspend() ------------------------------*/

TARPC_FUNC(sigsuspend, {},
{
    MAKE_CALL(out->retval = sigsuspend(IN_SIGSET));
}
)

/*-------------- sigfillset() ------------------------------*/

TARPC_FUNC(sigfillset, {},
{
    MAKE_CALL(out->retval = sigfillset(IN_SIGSET));
}
)

/*-------------- sigaddset() -------------------------------*/

TARPC_FUNC(sigaddset, {},
{
    MAKE_CALL(out->retval = sigaddset(IN_SIGSET, signum_rpc2h(in->signum)));
}
)

/*-------------- sigdelset() -------------------------------*/

TARPC_FUNC(sigdelset, {},
{
    MAKE_CALL(out->retval = sigdelset(IN_SIGSET, signum_rpc2h(in->signum)));
}
)

/*-------------- sigismember() ------------------------------*/

TARPC_FUNC(sigismember, {},
{
    INIT_CHECKED_ARG((char *)(IN_SIGSET), sizeof(sigset_t), 0);
    MAKE_CALL(out->retval = sigismember(IN_SIGSET, 
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        rc = -1;
        goto overfill_buffers_exit;
    }

    memset(buf, 0xAB, sizeof(max_len));
    
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
                  (HANDLE)rcf_pch_mem_get(in->async_task_handle)));
    rcf_pch_mem_free(in->async_task_handle);
}
)

/**
 * Allocate a single buffer of specified size and return a pointer to it.
 */
TARPC_FUNC(malloc, {},
{
    void *buf;
    
    UNUSED(list);
    
    buf = malloc(in->size);

    if (buf == NULL)
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    else
        out->retval = rcf_pch_mem_alloc(buf);
}
)

/**
 * Free a previously allocated buffer.
 */
TARPC_FUNC(free, {},
{
    UNUSED(list);
    UNUSED(out);
    free(rcf_pch_mem_get(in->buf));
    rcf_pch_mem_free(in->buf);
}
)

/**
 * Fill in the buffer.
 */
TARPC_FUNC(set_buf, {},
{
    void *dst_buf;
    
    UNUSED(list);
    UNUSED(out);
    
    dst_buf = rcf_pch_mem_get(in->dst_buf);
    if (dst_buf != NULL && in->src_buf.src_buf_len != 0)
    {
        memcpy(dst_buf + (unsigned int)in->offset,
               in->src_buf.src_buf_val, in->src_buf.src_buf_len);
    }
}
)

TARPC_FUNC(get_buf, {},
{
    void *src_buf; 
    
    UNUSED(list);

    src_buf = rcf_pch_mem_get(in->src_buf);
    if (src_buf != NULL && in->len != 0)
    {
        char *buf;

        buf = malloc(in->len);
        if (buf == NULL)
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
        {
            memcpy(buf, src_buf + (unsigned int)in->offset, in->len);
            out->dst_buf.dst_buf_val = buf;
            out->dst_buf.dst_buf_len = in->len;
        }
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
    if ((wsabuf != NULL) && (in->len != 0))
        buf = calloc(1, in->len);
    else
        buf = NULL;

    if ((wsabuf == NULL) || ((buf == NULL) && (in->len != 0)))
    {
        if (wsabuf != NULL)
            free(wsabuf);
        if (buf != NULL)
            free(buf);
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        out->wsabuf = 0;
        out->wsabuf_buf = 0;
        out->retval = -1;
    }
    else
    {
        wsabuf->buf = buf;
        wsabuf->len = in->len;
        out->wsabuf = rcf_pch_mem_alloc(wsabuf);
        out->wsabuf_buf = rcf_pch_mem_alloc(buf);
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
    wsabuf = (WSABUF *)rcf_pch_mem_get(in->wsabuf);
    if (wsabuf != NULL)
    {
        rcf_pch_mem_free_mem(wsabuf->buf);
        free(wsabuf->buf);
        free(wsabuf);
    }
    rcf_pch_mem_free(in->wsabuf);
}
)

/*
 * Copy the data from tarpc_flowspec to FLOWSPEC structure.
 */
static void flowspec_rpc2h(FLOWSPEC *fs, tarpc_flowspec *tfs)
{
    fs->TokenRate = tfs->TokenRate;
    fs->TokenBucketSize = tfs->TokenBucketSize;
    fs->PeakBandwidth = tfs->PeakBandwidth;
    fs->Latency = tfs->Latency;
    fs->DelayVariation = tfs->DelayVariation;
    fs->ServiceType =
        (SERVICETYPE)servicetype_flags_rpc2h(tfs->ServiceType);
    fs->MaxSduSize = tfs->MaxSduSize;
    fs->MinimumPolicedSize = tfs->MinimumPolicedSize;
}

/*
 * Copy the data from FLOWSPEC to tarpc_flowspec structure.
 */
static void flowspec_h2rpc(FLOWSPEC *fs, tarpc_flowspec *tfs)
{
    tfs->TokenRate = fs->TokenRate;
    tfs->TokenBucketSize = fs->TokenBucketSize;
    tfs->PeakBandwidth = fs->PeakBandwidth;
    tfs->Latency = fs->Latency;
    tfs->DelayVariation = fs->DelayVariation;
    tfs->ServiceType =
        servicetype_flags_h2rpc((unsigned int)fs->ServiceType);
    tfs->MaxSduSize = fs->MaxSduSize;
    tfs->MinimumPolicedSize = fs->MinimumPolicedSize;
}

/*-------------- WSAConnect -----------------------------*/
TARPC_FUNC(wsa_connect, {},
{
    QOS sqos;
    QOS *psqos;
    PREPARE_ADDR(in->addr, 0);

    if (in->sqos_is_null == TRUE)
        psqos = NULL;
    else
    {
        psqos = &sqos;
        memset(&sqos, 0, sizeof(sqos));
        flowspec_rpc2h(&sqos.SendingFlowspec, &in->sqos.sending);
        flowspec_rpc2h(&sqos.ReceivingFlowspec, &in->sqos.receiving);
        sqos.ProviderSpecific.buf =
            (char*)in->sqos.provider_specific_buf.provider_specific_buf_val;
        sqos.ProviderSpecific.len =
            in->sqos.provider_specific_buf.provider_specific_buf_len;
    }

    MAKE_CALL(out->retval = WSAConnect(in->s, a, in->addrlen,
                                       (LPWSABUF)
                                       rcf_pch_mem_get(in->caller_wsabuf),
                                       (LPWSABUF)
                                       rcf_pch_mem_get(in->callee_wsabuf),
                                       psqos, NULL));
}
)

/**
 * Convert the TA-dependent result (output) of the WSAIoctl() call into
 * the wsa_ioctl_request structure representation.
 */
static void convert_wsa_ioctl_result(DWORD code, char *buf,
                                     wsa_ioctl_request *res)
{
    if (buf == NULL)
        return;

    switch (code)
    {
        case RPC_WSA_FIONREAD: /* unsigned int */
        case RPC_WSA_SIOCATMARK: /* BOOL */
        case RPC_WSA_SIO_CHK_QOS: /* DWORD */
        case RPC_WSA_SIO_UDP_CONNRESET: /* BOOL */
        case RPC_WSA_SIO_TRANSLATE_HANDLE: /* HANDLE??? */
            res->wsa_ioctl_request_u.req_int = *(int*)buf;
            break;

        case RPC_WSA_SIO_ADDRESS_LIST_QUERY:
        {
            SOCKET_ADDRESS_LIST *sal;
            tarpc_sa            *tsa;
            int                 i;
        
            sal = (SOCKET_ADDRESS_LIST *)buf;
            tsa = (tarpc_sa *)calloc(sal->iAddressCount, sizeof(tarpc_sa));
        
            for (i = 0; i < sal->iAddressCount; i++)
            {
                sockaddr_h2rpc(sal->Address[i].lpSockaddr, &tsa[i]);
            }
            res->wsa_ioctl_request_u.req_saa.req_saa_val = tsa;
            res->wsa_ioctl_request_u.req_saa.req_saa_len = i;
            
            break;
        }

        case RPC_WSA_SIO_GET_BROADCAST_ADDRESS:
        case RPC_WSA_SIO_ROUTING_INTERFACE_QUERY:
            sockaddr_h2rpc((struct sockaddr *)buf,
                &res->wsa_ioctl_request_u.req_sa);
            break;

        case RPC_WSA_SIO_GET_EXTENSION_FUNCTION_POINTER:
            res->wsa_ioctl_request_u.req_ptr = 
                rcf_pch_mem_alloc(*(void **)buf);
            break;

        case RPC_WSA_SIO_GET_GROUP_QOS:
        case RPC_WSA_SIO_GET_QOS:
        {
            QOS       *qos;
            tarpc_qos *tqos;
            
            qos = (QOS*)buf;
            tqos = &res->wsa_ioctl_request_u.req_qos;

            flowspec_h2rpc(&qos->SendingFlowspec, &tqos->sending);
            flowspec_h2rpc(&qos->ReceivingFlowspec, &tqos->receiving);
                                             
            if (qos->ProviderSpecific.len != 0)
            {
               tqos->provider_specific_buf.provider_specific_buf_val =
                   malloc(qos->ProviderSpecific.len);
               memcpy(tqos->provider_specific_buf.provider_specific_buf_val,
                   qos->ProviderSpecific.buf, qos->ProviderSpecific.len);
               tqos->provider_specific_buf.provider_specific_buf_len =
                   qos->ProviderSpecific.len;
            }
            else
            {
               tqos->provider_specific_buf.provider_specific_buf_val = NULL;
               tqos->provider_specific_buf.provider_specific_buf_len = 0;
            }
        }
    }
}

/*-------------- WSAIoctl -------------------------------*/
TARPC_FUNC(wsa_ioctl, {},
{
    rpc_overlapped           *overlapped = NULL;
    void                     *inbuf = NULL;
    void                     *outbuf = NULL;
    DWORD                    inbuf_len = 0;
    DWORD                    outbuf_len = 0;
    struct sockaddr_storage  addr;
    QOS                      qos;
    struct tcp_keepalive     tka;
    GUID                     guid;

    switch (in->req.type)
    {
        case WSA_IOCTL_VOID:
        case WSA_IOCTL_SAA:
            break;

        case WSA_IOCTL_INT:
            inbuf = &in->req.wsa_ioctl_request_u.req_int;
            inbuf_len = sizeof(tarpc_int);
            break;

        case WSA_IOCTL_SA:
            inbuf = sockaddr_rpc2h(
                    &in->req.wsa_ioctl_request_u.req_sa, &addr);
            INIT_CHECKED_ARG((char *)inbuf,
                in->req.wsa_ioctl_request_u.req_sa.sa_data.sa_data_len
                 + SA_COMMON_LEN, 0);
            inbuf_len = sizeof(struct sockaddr);
            break;

        case WSA_IOCTL_GUID:
            guid.Data1 = in->req.wsa_ioctl_request_u.req_guid.data1;
            guid.Data2 = in->req.wsa_ioctl_request_u.req_guid.data2;
            guid.Data3 = in->req.wsa_ioctl_request_u.req_guid.data3;
            memcpy(guid.Data4,
                   in->req.wsa_ioctl_request_u.req_guid.data4.data4_val, 8);
            inbuf = &guid;
            inbuf_len = sizeof(GUID);
            break;

        case WSA_IOCTL_TCP_KEEPALIVE:
        {
            tarpc_tcp_keepalive *intka;

            intka = &in->req.wsa_ioctl_request_u.req_tka;
            tka.onoff = intka->onoff;
            tka.keepalivetime = intka->keepalivetime;
            tka.keepaliveinterval = intka->keepaliveinterval;
            inbuf = &tka;
            inbuf_len = sizeof(tka);
            break;
        }

        case WSA_IOCTL_QOS:
        {
            tarpc_qos *inqos;

            inqos = &in->req.wsa_ioctl_request_u.req_qos;
            flowspec_rpc2h(&qos.SendingFlowspec, &inqos->sending);
            flowspec_rpc2h(&qos.ReceivingFlowspec, &inqos->receiving);
            qos.ProviderSpecific.buf =
                inqos->provider_specific_buf.provider_specific_buf_val;
            qos.ProviderSpecific.len =
                inqos->provider_specific_buf.provider_specific_buf_len;
            inbuf = &qos;
            inbuf_len = sizeof(QOS);
            break;
        }

        case WSA_IOCTL_PTR:
            inbuf = rcf_pch_mem_get(in->req.wsa_ioctl_request_u.req_ptr);
            inbuf_len = in->inbuf_len;
            break;
    }

    outbuf_len = in->outbuf_len;

    if (outbuf_len != 0)
    {
        outbuf = calloc(1, outbuf_len);
        if (outbuf == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
            goto finish;
        }
    }

    if (in->overlapped != 0)
    {
        overlapped = IN_OVERLAPPED;
        rpc_overlapped_free_memory(overlapped);

        if (outbuf_len != 0)
        {
            overlapped->buffers = malloc(sizeof(WSABUF));
            if (overlapped->buffers == NULL)
            {
                out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
                goto finish;
            }
            overlapped->buffers[0].buf = outbuf;
            overlapped->buffers[0].len = outbuf_len;
        }
    }

    MAKE_CALL(out->retval = WSAIoctl(in->s, wsa_ioctl_rpc2h(in->code),
                                inbuf,
                                inbuf_len,
                                outbuf,
                                outbuf_len,
                                &out->bytes_returned,
                                in->overlapped == 0 ? NULL
                                  : (LPWSAOVERLAPPED)overlapped,
                                in->callback ?
                                  (LPWSAOVERLAPPED_COMPLETION_ROUTINE)
                                  completion_callback : NULL));

    if (out->retval == 0)
    {
        if ((outbuf != NULL) && (out->bytes_returned != 0))
        {
            convert_wsa_ioctl_result(in->code, outbuf, &out->result);
            if ( ! ((overlapped != NULL) &&
                    (overlapped->buffers != NULL) &&
                    (overlapped->buffers[0].buf == outbuf)))
            {
                free(outbuf);
            }
        }
    }
    else if ((overlapped != NULL) &&
            (out->common.win_error != RPC_WSA_IO_PENDING))
    {
        rpc_overlapped_free_memory(overlapped);
    }

    finish:
    ;
}
)

TARPC_FUNC(get_wsa_ioctl_overlapped_result,
{
    COPY_ARG(bytes);
    COPY_ARG(flags);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;

    UNUSED(list);
    MAKE_CALL(out->retval = WSAGetOverlappedResult(in->s,
                                (LPWSAOVERLAPPED)overlapped,
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

        convert_wsa_ioctl_result(in->code,
                                 overlapped->buffers[0].buf, &out->result);

        rpc_overlapped_free_memory(overlapped);
    }
}
)

/*-------------- WSAAsyncGetHostByAddr ------------------*/
TARPC_FUNC(wsa_async_get_host_by_addr, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetHostByAddr(
                                IN_HWND, in->wmsg, in->addr.addr_val,
                                in->addrlen, addr_family_rpc2h(in->type),
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetHostByName ------------------*/
TARPC_FUNC(wsa_async_get_host_by_name, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetHostByName(
                                IN_HWND, in->wmsg, in->name.name_val,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetProtoByName -----------------*/
TARPC_FUNC(wsa_async_get_proto_by_name, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetProtoByName(
                                IN_HWND, in->wmsg, in->name.name_val,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetProtoByNumber ---------------*/
TARPC_FUNC(wsa_async_get_proto_by_number, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetProtoByNumber(
                                IN_HWND, in->wmsg, in->number,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetServByName ---------------*/
TARPC_FUNC(wsa_async_get_serv_by_name, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetServByName(
                                IN_HWND, in->wmsg,
                                in->name.name_val, in->proto.proto_val,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetServByPort ---------------*/
TARPC_FUNC(wsa_async_get_serv_by_port, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetServByPort(
                                IN_HWND, in->wmsg, in->port,
                                in->proto.proto_val, 
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- rpc_is_op_done() -----------------------------*/

bool_t
_rpc_is_op_done_1_svc(tarpc_rpc_is_op_done_in  *in,
                      tarpc_rpc_is_op_done_out *out,
                      struct svc_req           *rqstp)
{
    te_bool *is_done = (te_bool *)rcf_pch_mem_get(in->common.done);

    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    if ((is_done != NULL) && (in->common.op == RCF_RPC_IS_DONE))
    {
        out->common._errno = 0;
        out->common.done = (*is_done) ? in->common.done : 0;
    }
    else
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    return TRUE;
}

/*------------ CreateIoCompletionPort() -------------------*/

TARPC_FUNC(create_io_completion_port,
{},
{
    HANDLE file_handle = NULL;
    HANDLE iocp = NULL;
    HANDLE existing_iocp = NULL;

    UNUSED(list);

    /* ATTENTION! The code below supposes that in->file_handle can
     * be only a socket descriptor: currently the socket descriptors
     * are not passed through rcf_pch_mem_alloc() (when returned by
     * socket() RPC), but other handles are. I.e. other handles would
     * have to be obtained here by rcf_pch_mem_get(in->file_handle),
     * not just directly in->file_handle. */
    file_handle = in->file_handle == (tarpc_handle)-1 ?
                  INVALID_HANDLE_VALUE : (HANDLE)in->file_handle;

    existing_iocp = (HANDLE)rcf_pch_mem_get(in->existing_completion_port);

    MAKE_CALL(iocp = CreateIoCompletionPort(file_handle,
                         existing_iocp, (ULONG_PTR)in->completion_key,
                         (DWORD)in->number_of_concurrent_threads)
    );

    if (iocp != NULL)
    {
        if (iocp == existing_iocp)
            out->retval = in->existing_completion_port;
        else
            out->retval = (tarpc_handle)rcf_pch_mem_alloc(iocp);
    }
    else
        out->retval = (tarpc_handle)0;
}
)

/*------------ GetQueuedCompletionStatus() -------------------*/

TARPC_FUNC(get_queued_completion_status,
{},
{
    OVERLAPPED *overlapped = NULL;

    UNUSED(list);

    MAKE_CALL(out->retval = GetQueuedCompletionStatus(
        (HANDLE)rcf_pch_mem_get(in->completion_port),
        (DWORD *)&out->number_of_bytes,
        (ULONG_PTR *)&out->completion_key,
        &overlapped, (DWORD)in->milliseconds)
    );

    if (overlapped != NULL)
        out->overlapped = (tarpc_overlapped)rcf_pch_mem_get_id(overlapped);
    else
        out->overlapped = (tarpc_overlapped)0;
}
)

/*------------ PostQueuedCompletionStatus() -------------------*/

TARPC_FUNC(post_queued_completion_status,
{},
{
    UNUSED(list);

    MAKE_CALL(out->retval = PostQueuedCompletionStatus(
        (HANDLE)rcf_pch_mem_get(in->completion_port),
        (DWORD)in->number_of_bytes,
        (ULONG_PTR)in->completion_key,
        in->overlapped == 0 ? NULL :
            (HANDLE)rcf_pch_mem_get(in->overlapped))
    );
}
)

/*-------------- gettimeofday() --------------------------------*/
TARPC_FUNC(gettimeofday,
{
    COPY_ARG(tv);
},
{
    struct timeval  tv;
    struct timezone tz;

    if (out->tv.tv_len != 0)
        TARPC_CHECK_RC(timeval_rpc2h(out->tv.tv_val, &tv));

    if (out->common._errno != 0)
    {
        out->retval = -1;
    }
    else
    {
        MAKE_CALL(out->retval = 
                      gettimeofday(out->tv.tv_len == 0 ? NULL : &tv,
                                   NULL));
        if (out->tv.tv_len != 0)
            TARPC_CHECK_RC(timeval_h2rpc(&tv, out->tv.tv_val));
        if (TE_RC_GET_ERROR(out->common._errno) == TE_EH2RPC)
            out->retval = -1;
    }
}
)
