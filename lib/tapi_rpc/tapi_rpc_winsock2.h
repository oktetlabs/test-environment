/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of WinSOCK2-specific routines.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_WINSOCK2_H__
#define __TE_TAPI_RPC_WINSOCK2_H__

#include "tapi_rpc_unistd.h"


/* Windows Event Objects */
typedef void *rpc_wsaevent;

/* Windows WSAOVERLAPPED structure */
typedef void *rpc_overlapped;

/* WSASocket() */
extern int rpc_wsa_socket(rcf_rpc_server *rpcs,
                          rpc_socket_domain domain, rpc_socket_type type,
                          rpc_socket_proto protocol,
                          uint8_t *info, int info_len, te_bool overlapped);

/**
 * WSADuplicateSocket(). Protocol info is copied to the Test Engine
 * and then back  to the TA (in rpc_wsa_socket() function) as is.
 *
 * @param rpcs          RPC server
 * @param s             old socket
 * @param pid           destination process PID
 * @param info          buffer for protocol info or NULL
 * @param info_len      buffer length location (IN)/protocol info length
 *                      location (OUT)
 *
 * @return value returned by WSADuplicateSocket() function
 */
extern int rpc_wsa_duplicate_socket(rcf_rpc_server *rpcs,
                                    int s, int pid,
                                    uint8_t *info, int *info_len);

extern int rpc_connect_ex(rcf_rpc_server *rpcs,
                          int s, const struct sockaddr *addr,
                          socklen_t addrlen,
                          void *buf, ssize_t len_buf,
                          ssize_t *bytes_sent,
                          rpc_overlapped overlapped);

extern int rpc_disconnect_ex(rcf_rpc_server *rpcs, int s,
                             rpc_overlapped overlapped, int flags);

typedef enum accept_verdict {
    CF_REJECT,
    CF_ACCEPT,
    CF_DEFER
} accept_verdict;

typedef struct accept_cond {
    unsigned short port;
    accept_verdict verdict;
} accept_cond;

#define RCF_RPC_MAX_ACCEPT_CONDS    4

/**
 * WSAAccept() with condition function support. List of conditions
 * describes the condition function behaivour.
 */
extern int rpc_wsa_accept(rcf_rpc_server *rpcs,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen, socklen_t raddrlen,
                          accept_cond *cond, int cond_num);
/**
 * Client implementation of AcceptEx()-GetAcceptExSockAddr() call.
 *
 * @param rpcs              RPC server
 * @param s                 Descriptor of socket that has already been
 *                          called with the listen function
 * @param s_a               Descriptor of a socket on wich to accept
 *                          an incomming connection
 * @param len               Length of the buffer to receive data (should not
 *                          include the size of local and remote addresses)
 * @param overlapped        WSAOVERLAPPED structure
 * @param bytes_received    number of received data bytes
 *
 * @return value returned by AcceptEx() function.
 */
extern int rpc_accept_ex(rcf_rpc_server *rpcs, int s, int s_a,
                         size_t len, rpc_overlapped overlapped,
                         size_t *bytes_received);

/**
 * Client implementation of GetAcceptExSockAddr() call.
 *
 * @param rpcs          RPC server
 * @param s             descriptor of socket that was passed
 *                      to rpc_accept_ex() fuinction as 3d parameter
 * @param buf           pointer to a buffer passed to
 *                      rpc_get_overlapped_result()
 * @param buflen        size of the buffer passed to
 *                      rpc_get_overlapped_result()
 * @param len           buffer size wich was passed to rpc_accept_ex()
 * @param laddr         local address returned by GetAcceptExSockAddr()
 * @param raddr         remote address returned by GetAcceptExSockAddr()
 *
 * @return N/A
 */
extern void rpc_get_accept_addr(rcf_rpc_server *rpcs,
                                int s, void *buf, size_t buflen, size_t len,
                                struct sockaddr *laddr,
                                struct sockaddr *raddr);

extern int rpc_transmit_file(rcf_rpc_server *rpcs, int s, char *file,
                             ssize_t len, ssize_t len_per_send,
                             rpc_overlapped overlapped,
                             void *head, ssize_t head_len,
                             void *tail, ssize_t tail_len, ssize_t flags);

/* WSARecvEx() */
extern ssize_t rpc_wsa_recv_ex(rcf_rpc_server *rpcs,
                               int s, void *buf, size_t len,
                               rpc_send_recv_flags *flags,
                               size_t rbuflen);

/* WSACreateEvent() */
extern rpc_wsaevent rpc_create_event(rcf_rpc_server *rpcs);

/* WSACloseEvent() */
extern int rpc_close_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);

/* WSAResetEvent() */
extern int rpc_reset_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);

/* Create WSAOVERLAPPED */
extern rpc_overlapped rpc_create_overlapped(rcf_rpc_server *rpcs,
                                            rpc_wsaevent hevent,
                                            unsigned int offset,
                                            unsigned int offset_high);

/* Delete WSAOVERLAPPED */
extern void rpc_delete_overlapped(rcf_rpc_server *rpcs,
                                  rpc_overlapped overlapped);

/* WSASend() */
extern int rpc_wsa_send(rcf_rpc_server *rpcs,
                       int s, const struct rpc_iovec *iov,
                       size_t iovcnt, rpc_send_recv_flags flags,
                       int *bytes_sent, rpc_overlapped overlapped,
                       te_bool callback);

/* WSARecv() */
extern int rpc_wsa_recv(rcf_rpc_server *rpcs,
                        int s, const struct rpc_iovec *iov,
                        size_t iovcnt, size_t riovcnt,
                        rpc_send_recv_flags *flags,
                        int *bytes_received, rpc_overlapped overlapped,
                        te_bool callback);

/* WSASendTo */
extern int rpc_wsa_send_to(rcf_rpc_server *handle, int s,
                           const struct rpc_iovec *iov,
                           size_t iovcnt, rpc_send_recv_flags flags,
                           int *bytes_sent, const struct sockaddr *to,
                           socklen_t tolen, rpc_overlapped overlapped,
                           te_bool callback);

/* WSARecvFrom */
extern int rpc_wsa_recv_from(rcf_rpc_server *handle, int s,
                             const struct rpc_iovec *iov, size_t iovcnt,
                             size_t riovcnt, rpc_send_recv_flags *flags,
                             int *bytes_received, struct sockaddr *from,
                             socklen_t *fromlen, rpc_overlapped overlapped,
                             te_bool callback);

/* WSASendDisconnect */
extern int rpc_wsa_send_disconnect(rcf_rpc_server *handle,
                                   int s, const struct rpc_iovec *iov);

/* WSARecvDisconnect */
extern int rpc_wsa_recv_disconnect(rcf_rpc_server *handle,
                                   int s, const struct rpc_iovec *iov);

/* WSAGetOverlappedResult() */
extern int rpc_get_overlapped_result(rcf_rpc_server *rpcs,
                                     int s, rpc_overlapped overlapped,
                                     int *bytes, te_bool wait,
                                     rpc_send_recv_flags *flags,
                                     char *buf, int buflen);
/**
 * Get result of completion callback (if called).
 *
 * @param rpcs          RPC server
 * @param called        number of callback calls since last call
 *                      of this function
 * @param bytes         number of tramsmitted bytes reported to last
 *                      callback
 * @param error         overlapped operation error reported to the last
 *                      callback
 * @param overlapped    overlapped object reported to the last callback
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_completion_callback(rcf_rpc_server *rpcs,
                                   int *called, int *error, int *bytes,
                                   rpc_overlapped *overlapped);

/* WSAEventSelect() */
extern int rpc_wsa_event_select(rcf_rpc_server *rpcs,
                                int s, rpc_wsaevent event_object,
                                rpc_network_event event);

/**
 * Client implementation of WSAEnumNetworkEvents().
 *
 * @param rpcs              RPC server
 * @param s                 socket descriptor
 * @param event_object      event object to be reset
 * @param event             network events that occurred
 *
 * @return value returned by WSAEnumNetworkEvents() function
 */
extern int rpc_enum_network_events(rcf_rpc_server *rpcs,
                                  int s, rpc_wsaevent event_object,
                                  rpc_network_event *event);

/** Return codes for rpc_wait_multiple_events */
enum {
    WSA_WAIT_FAILED,
    WAIT_IO_COMPLETION,
    WSA_WAIT_TIMEOUT,
    WSA_WAIT_EVENT_0
};

/** Convert WSAWaitForMultipleEvents() return code to the string */
static inline const char *
wsa_wait_rpc2str(int code)
{
    static char buf[32];

    switch (code)
    {
        case WSA_WAIT_FAILED:    return "WSA_WAIT_FAILED";
        case WAIT_IO_COMPLETION: return "WSA_WAIT_COMPLETION";
        case WSA_WAIT_TIMEOUT:   return "WSA_WAIT_TIMOUT";
        default:
            if (code < WSA_WAIT_EVENT_0)
                return "WSA_UNKNOWN";
            sprintf(buf, "WSA_WAIT_EVENT_%d", code - WSA_WAIT_EVENT_0);
            return buf;
    }
}

/** WSAWaitForMultipleEvents(), return -1 in the case of RPC error */
extern int rpc_wait_multiple_events(rcf_rpc_server *rpcs,
                                    int count, rpc_wsaevent *events,
                                    te_bool wait_all, uint32_t timeout,
                                    te_bool alertable, int rcount);

/* Window objects */

typedef void *rpc_hwnd;

/** CreateWIndow() */
extern rpc_hwnd rpc_create_window(rcf_rpc_server *rpcs);

/** DestroyWindow() */
extern void rpc_destroy_window(rcf_rpc_server *rpcs, rpc_hwnd hwnd);

/** WSAAsyncSelect() */
extern int rpc_wsa_async_select(rcf_rpc_server *rpcs,
                                int s, rpc_hwnd hwnd,
                                rpc_network_event event);

/** PeekMessage() */
extern int rpc_peek_message(rcf_rpc_server *rpcs,
                            rpc_hwnd hwnd, int *s,
                            rpc_network_event *event);

/**
 * Check, if RPC server is located on TA with winsock2.
 *
 * @param rpcs  RPC server handle
 *
 * @return TRUE, if it is definitely known that winsock2 is used and FALSE
 *         otherwise
 */
static inline te_bool
rpc_is_winsock2(rcf_rpc_server *rpcs)
{
    rpc_wsaevent hevent = rpc_create_event(rpcs);
    
    if (hevent != NULL)
    {
        rpc_close_event(rpcs, hevent);
        return TRUE;
    }
    return FALSE;
}

#endif /* !__TE_TAPI_RPC_WINSOCK2_H__ */
