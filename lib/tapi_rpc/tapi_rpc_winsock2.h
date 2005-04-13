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
#include "tapi_rpc_socket.h"


/** Windows Event Objects */
typedef void *rpc_wsaevent;

/** Windows WSAOVERLAPPED structure */
typedef void *rpc_overlapped;

/** Windows HWND */
typedef void *rpc_hwnd;

/** Windows HANDLE  :o\  */
typedef void *rpc_handle;

/* Windows FLOWSPEC structure */
typedef struct _rpc_flowspec {
    uint32_t TokenRate;
    uint32_t TokenBucketSize;
    uint32_t PeakBandwidth;
    uint32_t Latency;
    uint32_t DelayVariation;
    uint32_t ServiceType;
    uint32_t MaxSduSize;
    uint32_t MinimumPolicedSize;
} rpc_flowspec;

/* Windows QOS structure */
typedef struct _rpc_qos {
    rpc_flowspec  sending;
    rpc_flowspec  receiving;
    char          *provider_specific_buf;
    size_t        provider_specific_buf_len;
} rpc_qos;

/* Windows GUID */
typedef struct _rpc_guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} rpc_guid;


/** 
 * See @b WSASocket() 
 *
 * @param rpcs      RPC server handle
 * @param domain    communication domain
 * @param type      type specification of the new socket
 * @param protocol  specifies the protocol to be used
 * @param info      pointer to an object than defines the characteristics
 *                  of the created socket
 * @param info_len  length of @b info
 * @param overlapped support of overlapped I/O
 *
 * @return socket descriptor upon successful completion, otherwise -1
 *         is returned.
 */
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

/**
 *  Establish a connection to a specified socket, and optionally send data
 *  once connection is established.
 *
 *  @param rpcs     RPC server handle
 *  @param s        socket descriptor
 *  @param addr     pointer to a @b sockaddr structure containing the 
 *                  address to connect to.
 *  @param addrlen  length of sockaddr structure
 *  @param buf      pointer to buffer containing connect data
 *  @param len_buf  length of the buffer @b buf
 *  @param bytes_sent pointer a the number of bytes sent 
 *  @param overlapped pointer to @b overlapped object
 *
 *  @return value returned by @b ConnectEx()
 */
extern int rpc_connect_ex(rcf_rpc_server *rpcs,
                          int s, const struct sockaddr *addr,
                          socklen_t addrlen,
                          void *buf, ssize_t len_buf,
                          ssize_t *bytes_sent,
                          rpc_overlapped overlapped);

/**
 * Close connection to a socket and allow the socket handle to be reused.
 *
 * @param rpcs        RPC server handle
 * @param s           conencted socket descriptor
 * @param overlapped  pointer to a rpc_overlapped structure
 * @param flags       customize behavior of operation
 *
 * @return value returned by @b DisconnectEx()
 */
extern int rpc_disconnect_ex(rcf_rpc_server *rpcs, int s,
                             rpc_overlapped overlapped, int flags);

/** Accept decision making */
typedef enum accept_verdict {
    CF_REJECT,
    CF_ACCEPT,
    CF_DEFER
} accept_verdict;

/** Accept Condition */
typedef struct accept_cond {
    unsigned short port;      /**< port*/
    accept_verdict verdict;   /**< accept decision maker */
} accept_cond;

/** Maximal number of accept conditions */
#define RCF_RPC_MAX_ACCEPT_CONDS    4

/**
 * WSAAccept() with condition function support. List of conditions
 * describes the condition function behavior.
 *
 * @param rpcs     RPC server handle (IN)
 * @param s        listening socket descriptor (IN)
 * @param addr     pointer to a sockaddr structure containing the address of
 *                 the peer entity (OUT)
 * @param addrlen  pointer to the length of the address @b addr
 * @param raddrlen real length of the address @b addr
 * @param cond     specified condition that makes decision based on the
 *                 caller information passed in as parameters.
 * @param cond_num number of conditions
 *
 * @return new connected socket upon successful completion, otherwise
 *         -1 is returned.
 *                
 */
extern int rpc_wsa_accept(rcf_rpc_server *rpcs,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen, socklen_t raddrlen,
                          accept_cond *cond, int cond_num);
/**
 * Client implementation of AcceptEx()-GetAcceptExSockAddr() call.
 *
 * @param rpcs              RPC server
 * @param s                 descriptor of socket that has already been
 *                          called with the listen function
 * @param s_a               descriptor idenfifying a socket on which to 
 *                          accept an incomming connection
 * @param len               length of the buffer to receive data (should not
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
/**
 * Transmit file data over a connected socket. This function uses the 
 * operating system cache manager to retrive the file data, and perform 
 * high-performance file data transfert over sockets.
 *
 * @param rpcs         RPC server handle
 * @param s            connected socket descriptor
 * @param file         name of the file that contain data to be transmited.
 * @param len          length of data to transmit
 * @param len_per_send size of each block of data in each send operation.
 * @param overlapped   pointer to an rpc_overlapped structure
 * @param head         pointer to a buffer to be transmitted before file 
 *                     data is transmitted
 * @param head_len     size of buffer @b head
 * @param tail         pointer to a buffer to be transmitted after  
 *                     transmission of file data.
 * @param tail_len     size of buffer @b tail
 * @param flags      call flags (See @b Transmit file for more information)
 *
 * @return value returned by @b TransmitFile upon successful completion,
 *         otherwise -1 is returned.
 */
extern int rpc_transmit_file(rcf_rpc_server *rpcs, int s, char *file,
                             ssize_t len, ssize_t len_per_send,
                             rpc_overlapped overlapped,
                             void *head, ssize_t head_len,
                             void *tail, ssize_t tail_len, ssize_t flags);

/** 
 * See @b WSARecvEx() 
 *
 * @param rpcs    RPC server handle
 * @param s       socket descriptor
 * @param buf     pointer to a buffer containing the incoming data
 * @param len     Maximum length of expected data
 * @param flags   specify whether data is fully or partially received
 * @param rbuflen real size of buffer @b buf
 *
 * @return number of bytes received upon successful completion. If the 
 *         connection has been close it returned zero.
 *         Otherwise -1 is returned.
 */
extern ssize_t rpc_wsa_recv_ex(rcf_rpc_server *rpcs,
                               int s, void *buf, size_t len,
                               rpc_send_recv_flags *flags,
                               size_t rbuflen);

/**
 * Create a new event object
 *
 * @param rpcs    RPC server handle
 *
 * @return Upon successful completion this function returns an
 *         @b rpc_wsaevent structure, otherwise -1 is returned
 */
extern rpc_wsaevent rpc_create_event(rcf_rpc_server *rpcs);

/**
 * Close an open event object handle
 *
 * @param rpcs    RPC server handle
 * @param hevent  handle of the event to be close
 *
 * @return value returned by @b WSACloseEvent upon successful completion,
 *         otherwise -1 is returned
 */
extern int rpc_close_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);

/** 
 * Reset the state of the specified event object to nonsignaled.
 *
 * @param rpcs   RPC server handle
 * @param hevent event object handle
 *
 * @return value returned by @b WSAResetEvent upon successful completion.
 *         Otherwise -1 is returned.
 */
extern int rpc_reset_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);

/**
 * Set the state of the specified event object to signaled.
 *
 * @param rpcs    RPC server handle
 * @param  hevent  event object handle
 *
 * @return  value returned by @b WSASetEvent upon successful completion.
 *         Otherwise -1 is returned.
 */
extern int rpc_set_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);


/**
 * Convert a @b sockaddr structure to its string representation.
 *
 * @param rpcs     RPC server handle
 * @param addr     pointer to the @b sockaddr structure to be converted
 * @param addrlen  length of the address @b addr
 * @param info     pointer to an object which is associated with
 *                 the provider to be used or @b NULL
 * @param info_len length of object @b info (if not NULL)
 * @param addrstr  buffer that contain the string representation of the
 *                 specified address (OUT)
 * @param addrstr_len length of buffer @b addrstr
 *
 * @retval  0 on success
 * @retval -1 on error
 */
extern int rpc_wsa_address_to_string(rcf_rpc_server *rpcs,
                                     struct sockaddr *addr,
                                     socklen_t addrlen, uint8_t *info,
                                     int info_len, char *addrstr,
                                     ssize_t *addrstr_len);
     
/* WSAStringToAddress */
/**
 * Convert a numeric string to a @b sockaddr structure
 *
 * @param rpcs       RPC server handle
 * @param addrstr    numeric address string
 * @param address_family address family to which the string belongs
 * @param info       pointer to an object which is associated with 
 *                   the provider to be used or NULL
 * @param info_len   length of object @b info (if not null)
 * @param addr       pointer to a @b sockaddr structure that receive the 
 *                   converted address.
 * @param addrlen    points to the length of @b addr
 *
 * @retval  0 on success
 * @retval -1 on error
 */
extern int rpc_wsa_string_to_address(rcf_rpc_server *rpcs, char *addrstr,
                                     rpc_socket_domain address_family,
                                     uint8_t *info, int info_len,
                                     struct sockaddr *addr,
                                     socklen_t *addrlen);


/**
 * Cancel an incomplete asynchronous task
 *
 * @param rpcs   RPC server handle
 * @param async_task_handle task to be canceled
 *
 * @retval  0 upon successful completion
 * @retval -1 on failure 
 */
extern int rpc_wsa_cancel_async_request(rcf_rpc_server *rpcs,
                                        rpc_handle async_task_handle);

/**
 * Allocate a buffer of specified size in the TA address space
 *
 * @param rpcs    RPC server handle
 * @param size    size of the buffer to be allocated
 *
 * @return   pointer to the allocated buffer upon successful completion
 *           otherwise NULL is retruned.
 */
extern rpc_ptr rpc_alloc_buf(rcf_rpc_server *rpcs, size_t size);

/**
 * Free the specified buffer in TA address space
 *
 * @param rpcs   RPC server handle
 * @param buf    pointer to the buffer to be freed
 */
extern void rpc_free_buf(rcf_rpc_server *rpcs, rpc_ptr buf);

/**
 * Copy the @b src_buf buffer in @b dst_buf located in TA address space
 *
 * @param rpcs    RPC server handle
 * @param src_buf pointer to the source buffer
 * @param len     length of data to be copied
 * @param dst_buf pointer to the destination buffer
 */
extern void rpc_set_buf(rcf_rpc_server *rpcs, char *src_buf,
                        size_t len, rpc_ptr dst_buf);

/**
 * Copy the @b src_buf buffer located in TA address space to the 
 * @b dst_buf buffer
 *
 * @param rpcs     RPC server handle
 * @param src_buf  source buffer
 * @param len      length of data to be copied
 * @param dst_buf  destination buffer
 */
extern void rpc_get_buf(rcf_rpc_server *rpcs, rpc_ptr src_buf,
                        size_t len, char *dst_buf);

/**
 * Allocate a WSABUF structure, a buffer of specified length and fill in
 * the fields of allocated structure according to the allocated buffer
 * pointer and length in the TA address space.
 *
 * @param rpcs       RPC server handle
 * @param len        length of WSABUF buffer field
 * @param wsabuf     pointer to a WSABUF structure
 * @param wsabuf_buf pointer to the buffer whose length is @b len
 *
 * @retval  0 upon successful completion
 * @retval -1 on failure
 */
extern int rpc_alloc_wsabuf(rcf_rpc_server *rpcs, size_t len,
                            rpc_ptr *wsabuf, rpc_ptr *wsabuf_buf);

/**
 * Free a previously allocated by @b rpc_alloc_wsabuf buffer
 *
 * @param rpcs     RPC server handle
 * @param wsabuf   pointer to the buffer to be freed
 */
extern void rpc_free_wsabuf(rcf_rpc_server *rpcs, rpc_ptr wsabuf);

/* WSAConnect */
extern int rpc_wsa_connect(rcf_rpc_server *rpcs, int s,
                           struct sockaddr *addr, socklen_t addrlen,
                           rpc_ptr caller_wsabuf, rpc_ptr callee_wsabuf,
                           rpc_qos *sqos);

/* WSAIoctl */
extern int rpc_wsa_ioctl(rcf_rpc_server *rpcs, int s,
                         rpc_wsa_ioctl_code control_code,
                         char *inbuf, unsigned int inbuf_len,
                         char *outbuf, unsigned int outbuf_len,
                         unsigned int *bytes_returned,
                         rpc_overlapped overlapped, te_bool callback);

/* rpc_get_wsa_ioctl_overlapped_result() */
extern int rpc_get_wsa_ioctl_overlapped_result(rcf_rpc_server *rpcs,
                                    int s, rpc_overlapped overlapped,
                                    int *bytes, te_bool wait,
                                    rpc_send_recv_flags *flags,
                                    char *buf, int buflen,
                                    rpc_wsa_ioctl_code control_code);

/**
 * Asynchronously retrieve host information by given address.
 *  See @b WSAAsyncGetHostByAddr 
 *
 *  @param rpcs    RPC server handle 
 *  @param hwnd    handle to a window that receive a message when the
 *                 asynchronous request completes
 *  @param wmsg    message to be received when asynchronous request
 *                 completes
 *  @param addr    pointer to the network address of the host
 *  @param addrlen length of address @b addr
 *  @param type    type of the address
 *  @param buf     valid buffer pointer in the TA address space. 
 *                 Contain the host entry data
 *  @param buflen  size of the buffer @b buf, in bytes 
 *
 *  @return  value returned by  @b WSAAsyncGetHostByAddr 
 */
extern rpc_handle
rpc_wsa_async_get_host_by_addr(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, char *addr,
                               ssize_t addrlen, rpc_socket_type type,
                               rpc_ptr buf, ssize_t buflen);

/**
 * Asynchronously retrieve host information by given name.
 * See @b WSAAsyncGetHostByName 
 *
 * @param rpcs    RPC server handle 
 * @param hwnd    handle to a window that receive a message when the
 *                asynchronous request completes
 * @param wmsg    message to be received when asynchronous request
 *                completes
 * @param name    pointer to the host name
 * @param buf     valid buffer pointer in the TA address space. 
 *                Contain the host entry data
 * @param buflen  size of the buffer @b buf, in bytes 
 *
 * @return  value returned by  @b WSAAsyncGetHostByName 
 */
extern rpc_handle
rpc_wsa_async_get_host_by_name(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, char *name,
                               rpc_ptr buf, ssize_t buflen);
/** 
 * Asynchronously retrieve protocol information by given name.
 * See @b WSAAsyncGetProtoByName 
 *
 * @param rpcs    RPC server handle 
 * @param hwnd    handle to a window that receive a message when the
 *                 asynchronous request completes
 * @param wmsg    message to be received when asynchronous request
 *                 completes
 * @param name    pointer to a null-terminated protocol name
 * @param buf     valid buffer pointer in the TA address space. 
 *                 Contain the protocol entry data
 * @param buflen  size of the buffer @b buf, in bytes 
 *
 * @return  value returned by  @b WSAAsyncGetProtoByName 
 */
extern rpc_handle
rpc_wsa_async_get_proto_by_name(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                                unsigned int wmsg, char *name,
                                rpc_ptr buf, ssize_t buflen);

/** 
 * Asynchronously retrieve protocol information by given number.
 * See @b WSAAsyncGetProtoByNumber 
 *
 * @param rpcs    RPC server handle 
 * @param hwnd    handle to a window that receive a message when the
 *                 asynchronous request completes
 * @param wmsg    message to be received when asynchronous request
 *                 completes
 * @param number  protocol number in host byte order
 * @param buf     valid buffer pointer in the TA address space. 
 *                 Contain the protocol entry data
 * @param buflen  size of the buffer @b buf, in bytes 
 *
 * @return  value returned by  @b WSAAsyncGetProtoByNumber 
 */
extern rpc_handle
rpc_wsa_async_get_proto_by_number(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                                  unsigned int wmsg, int number,
                                  rpc_ptr buf, ssize_t buflen);

/** 
 * Asynchronously retrieve service information that corresponds to a 
 * service name.
 * 
 * @param rpcs    RPC server handle 
 * @param hwnd    handle to a window that receive a message when the
 *                 asynchronous request completes
 * @param wmsg    message to be received when asynchronous request
 *                 completes
 * @param name    pointer to a null-terminated service name
 * @param proto   pointer to a protocol name
 * @param buf     valid buffer pointer in the TA address space. 
 *                 Contain the service entry data
 * @param buflen  size of the buffer @b buf, in bytes 
 *
 * @return  value returned by  @b WSAAsyncGetServByName 
 */
extern rpc_handle
rpc_wsa_async_get_serv_by_name(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, char *name, char *proto,
                               rpc_ptr buf, ssize_t buflen);

/** 
 * Asynchronously retrieve service information that corresponds to a 
 * port and protocol.
 * 
 * @param rpcs    RPC server handle 
 * @param hwnd    handle to a window that receive a message when the
 *                 asynchronous request completes
 * @param wmsg    message to be received when asynchronous request
 *                 completes
 * @param port    port for the service
 * @param proto   pointer to a protocol name
 * @param buf     valid buffer pointer in the TA address space. 
 *                 Contain the service entry data
 * @param buflen  size of the buffer @b buf, in bytes 
 *
 * @return  value returned by  @b WSAAsyncGetServByName 
 */
extern rpc_handle
rpc_wsa_async_get_serv_by_port(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, int port, char *proto,
                               rpc_ptr buf, ssize_t buflen);

/** 
 * Create WSAOVERLAPPED structure on TA side 
 *
 * @param rpcs        RPC server handle
 * @param hevent      handle to an event object
 * @param offset      position at which to start the transfert
 * @param offset_high high-order word of the position at which to start
 *                    the transfert
 * 
 * @return WSAOVERLAPPED structure upon successful completion, otherwise
 *         error code is returned (ENOMEM)
 */
extern rpc_overlapped rpc_create_overlapped(rcf_rpc_server *rpcs,
                                            rpc_wsaevent hevent,
                                            unsigned int offset,
                                            unsigned int offset_high);

/** 
 * Delete specified WSAOVERLAPPED structure 
 *
 * @param rpcs       RPC server handle
 * @param overlapped  WSAOVERLAPPED structure 
 */
extern void rpc_delete_overlapped(rcf_rpc_server *rpcs,
                                  rpc_overlapped overlapped);

/** 
 * Send data on a connected socket
 *
 * @param rpcs        RPC server handle
 * @param s           connected socket descriptor
 * @param iov         pointer to a vector of buffers containing the data 
 *                    to be sent
 * @param iovcnt      number of buffers in the vector
 * @param flags       modifies the behavior of the call.      
 * @param bytes_sent  pointer to the number of bytes sent
 * @param overlapped  overlapped structure
 * @param callback    support of completion routine. If true a completion
 *                    routine is call when the send operation has been 
 *                    completed
 * @retval  0 on success
 * @retval -1 on failure
 */ 
extern int rpc_wsa_send(rcf_rpc_server *rpcs,
                       int s, const struct rpc_iovec *iov,
                       size_t iovcnt, rpc_send_recv_flags flags,
                       int *bytes_sent, rpc_overlapped overlapped,
                       te_bool callback);

/**
 * Receive data from a connected socket
 *
 * @param rpcs        RPC server handle
 * @param s           connected socket descriptor
 * @param iov         pointer to a vector of buffers where received data
 *                    have to be stored
 * @param iovcnt      number of buffers in the vector @b iov
 * @param riovcnt     number of buffers to receive
 * @param flags       modifies the behavior of the call.
 *                    (See @b rpc_send_recv_flags)
 * @param bytes_received pointer to the number of bytes received
 * @param overlapped  overlapped structure
 * @param callback    support of completion routine. If true a completion
 *                    routine is call when the send operation has been 
 *                    completed
 * @retval  0 on success
 * @retval -1 on failure
 */
extern int rpc_wsa_recv(rcf_rpc_server *rpcs,
                        int s, const struct rpc_iovec *iov,
                        size_t iovcnt, size_t riovcnt,
                        rpc_send_recv_flags *flags,
                        int *bytes_received, rpc_overlapped overlapped,
                        te_bool callback);

/**
 * Send data to a specified destination
 *
 * @param rpcs         RPC server handle
 * @param s            descriptor identifying a possibly connected socket
 * @param iov          pointer to a vector of buffers containing the data 
 *                     to be sent 
 * @param iovcnt       number of buffer in the vector @b iov
 * @param flags        modifies the behavior of the call.
 *                     (See @b rpc_send_recv_flags)
 * @param bytes_sent   pointer to the number of bytes sent
 * @param to           pointer to the address of the target socket
 * @param tolen        size of the address @b to
 * @param overlapped   overlapped structure
 * @param callback     support of completion routine. If true a completion
 *                     routine is call when the send operation has been 
 *                     completed
 * @retval  0 on success
 * @retval -1 on failure
 */
extern int rpc_wsa_send_to(rcf_rpc_server *rpcs, int s,
                           const struct rpc_iovec *iov,
                           size_t iovcnt, rpc_send_recv_flags flags,
                           int *bytes_sent, const struct sockaddr *to,
                           socklen_t tolen, rpc_overlapped overlapped,
                           te_bool callback);

/**
 * Receive datagram from socket
 *
 * @param rpcs           RPC server handle
 * @param s              descriptor identifying a socket
 * @param iov            pointer to a vector of buffers where received data
 *                       have to be stored 
 * @param iovcnt         number of buffers in the vector @b iov
 * @param riovcnt        number of buffers to receive
 * @param flags          modifies the behavior of the call.
 *                       (See @b rpc_send_recv_flags)
 * @param bytes_received pointer to the number of bytes received
 * @param from           pointer to a buffer that hold the source address 
 *                       upon the completion of overlapped operation
 * @param fromlen        size of the address @b from
 * @param overlapped     overlapped structure
 * @param callback       support of completion routine. If true a completion
 *                       routine is call when the send operation has been 
 *                       completed
 * @retval  0 on success
 * @retval -1 on failure
 */
extern int rpc_wsa_recv_from(rcf_rpc_server *rpcs, int s,
                             const struct rpc_iovec *iov, size_t iovcnt,
                             size_t riovcnt, rpc_send_recv_flags *flags,
                             int *bytes_received, struct sockaddr *from,
                             socklen_t *fromlen, rpc_overlapped overlapped,
                             te_bool callback);

/**
 * Initiate termination of the connection for the socket
 * and send disconnect data
 *
 * @param rpcs    RPC server handle
 * @param s       descriptor identifying a socket
 * @param iov     vector of buffers containing the disconnect data
 *
 * @retval  0 on success
 * @retval -1 on failure
 */
extern int rpc_wsa_send_disconnect(rcf_rpc_server *rpcs,
                                   int s, const struct rpc_iovec *iov);

/**
 * Terminate reception on a socket, and retrieve disconnect data
 * in case of connection oriented socket
 * 
 * @param rpcs    RPC server handle
 * @param s       descriptor identifying a socket
 * @param iov      vector of buffers containing the disconnect data
 *
 * @retval  0 on success
 * @retval -1 on failure
 */
extern int rpc_wsa_recv_disconnect(rcf_rpc_server *rpcs,
                                   int s, const struct rpc_iovec *iov);

/**
 * Retrieve data and control information from connected or unconnected
 * sockets
 *
 * @param rpcs           RPC server handle
 * @param s              descriptor identifying a socket
 * @param msg            pointer to a @b rpc_msghdr structure containing
 *                       received data
 * @param bytes_received pointer to the number of bytes received
 * @param overlapped     overlapped structure  
 * @param callback       support of completion routine. If true a completion
 *                       routine is call when the send operation has been 
 *                       completed
 * @retval  0 on success
 * @retval -1 on failure
 */
extern int rpc_wsa_recv_msg(rcf_rpc_server *rpcs, int s,
                            struct rpc_msghdr *msg, int *bytes_received,
                            rpc_overlapped overlapped, te_bool callback);

/**
 * Retrieve the result of an overlapped operation on a specified socket
 *
 * @param rpcs           RPC server handle
 * @param s              desccriptor identifying a socket
 * @param overlapped     overlapped structure
 * @param bytes          pointer to the number of bytes that were 
 *                       transfered by a send or receive operation
 * @param wait           specifies whether the function should wait for
 *                       the overlapped operation to complete
 *                       (should wait, if TRUE)
 * @param flags          pointer to a variable that will receive one or
 *                       more flags that supplement the completion status
 * @param buf            pointer to a buffer containing result data
 * @param buflen         size of buffer @b buf
 */
extern int rpc_get_overlapped_result(rcf_rpc_server *rpcs,
                                     int s, rpc_overlapped overlapped,
                                     int *bytes, te_bool wait,
                                     rpc_send_recv_flags *flags,
                                     char *buf, int buflen);
/**
 * Get result of completion callback (if called).
 *
 * @param rpcs          RPC server handle
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

/**
 * Specify an event object to be associated with the specified set of
 * network events
 *
 * @param rpcs         RPC server handle
 * @param s            socket descriptor
 * @param event_object event object
 * @param event        bitmask the specifies the combination of network 
 *                     events
 *
 * @retval  0 on success
 * @retval -1 on failure
 */
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

/** 
 * Convert WSAWaitForMultipleEvents() return code to a string 
 *
 * @param code code to be converted
 *
 * @return string representation of the event
 */
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

/** 
 * WSAWaitForMultipleEvents(), 
 * 
 * @param rpcs     RPC server handle
 * @param count    number of events in @b events
 * @param events   pointer to an array of events
 * @param wait_all wait type, when @b TRUE the function return when the 
 *                 state of all events is signaled
 * @param timeout  time-out interval, milliseconds. If the interval expires,
 *                 the function return. If @b timeout is zero the function 
 *                 tests the state of specified event objects and returns 
 *                 immediately. If @b timeout is WSA_INFINITE, the time-out
 *                 interval never expires.
 * @param alertable specify whether the completion routine has to be 
 *                  executed before the function returns.
 * @param rcount    maximum number of events
 *                   
 * @return the event object that make the function to return.
 *         -1 is returned in the case of RPC error 
 */
extern int rpc_wait_multiple_events(rcf_rpc_server *rpcs,
                                    int count, rpc_wsaevent *events,
                                    te_bool wait_all, uint32_t timeout,
                                    te_bool alertable, int rcount);


/** 
 * Create a window for receiving event notifications
 *
 * @param rpcs RPC server handle
 *
 * @return handle of the created window, upon successful completion,
 *         otherwise NULL is returned
 */
extern rpc_hwnd rpc_create_window(rcf_rpc_server *rpcs);

/** 
 * Destroy the specified window 
 *
 * @param rpcs RPC server handle
 * @param hwnd handle of window to be destroyed
 */
extern void rpc_destroy_window(rcf_rpc_server *rpcs, rpc_hwnd hwnd);

/** 
 * Request window-based notification of network events for a socket
 *
 * @param rpcs   RPC server handle
 * @param s      socket descriptor
 * @param hwnd   handle of the window which receives message when a network
 *               event occurs
 * @param event  bitmask that specifies a combination of network events
 *
 * @retval  0 on success
 * @retval -1 on failure
 */
extern int rpc_wsa_async_select(rcf_rpc_server *rpcs,
                                int s, rpc_hwnd hwnd,
                                rpc_network_event event);

/** PeekMessage() */
/**
 * Check the thread message queue for a posted message and retrieve
 * the message, if any exist
 *
 * @param rpcs    RPC server handle
 * @param hwnd    handle of the windows whose messages are to be examinied
 * @param s       socket descriptor whose event causes message notification
 * @param event   pointer to the bitmask that specifies a combination
 *                of network events
 */
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
extern te_bool rpc_is_winsock2(rcf_rpc_server *rpcs);

#endif /* !__TE_TAPI_RPC_WINSOCK2_H__ */
