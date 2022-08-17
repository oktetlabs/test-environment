/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of WinSOCK2-specific routines.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_RPC_WINSOCK2_H__
#define __TE_TAPI_RPC_WINSOCK2_H__

#include "tapi_rpc_unistd.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpc_misc.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_winsock2 TAPI for remote calls of Winsock2-specific routines
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/** Windows Event Objects */
typedef rpc_ptr rpc_wsaevent;

/** HANDLE */
typedef rpc_ptr rpc_handle;

/** Windows WSAOVERLAPPED structure */
typedef rpc_ptr rpc_overlapped;

/** Windows HWND */
typedef rpc_ptr rpc_hwnd;

/** Windows QOS structure */
typedef struct _rpc_qos {
    tarpc_flowspec  sending;          /**< QOS parameters for sending */
    tarpc_flowspec  receiving;        /**< QOS parameters for receiving */
    char          *provider_specific_buf; /**< Provider specific buffer */
    size_t         provider_specific_buf_len; /**< length of buffer */
} rpc_qos;

typedef struct _rpc_sys_info {
    uint64_t      ram_size;              /** Physical RAM size */
    unsigned int  page_size;             /** Physycal memory page size */
    unsigned int  number_of_processors;  /** CPUs number on the host */
} rpc_sys_info;

/**
* @b WSAStartup() remote call.
*
* @param rpcs      RPC server handle
* @return Zero if successful.
*/
extern int rpc_wsa_startup(rcf_rpc_server *rpcs);

/**
* @b WSACleanup() remote call.
*
* @param rpcs      RPC server handle
* @return Zero if successful.
*/
extern int rpc_wsa_cleanup(rcf_rpc_server *rpcs);

/**
 * @b WSASocket() remote call.
 *
 * @param rpcs      RPC server handle
 * @param domain    communication domain
 * @param type      type specification of the new socket
 * @param protocol  specifies the protocol to be used
 * @param info      pointer to an object than defines the characteristics
 *                  of the created socket
 * @param info_len  length of @b info
 * @param flags     flags for socket creation (overlapped, multipoint, etc.)
 *
 * @return Socket descriptor upon successful completion, otherwise -1
 *         is returned.
 */
extern int rpc_wsa_socket(rcf_rpc_server *rpcs,
                          rpc_socket_domain domain, rpc_socket_type type,
                          rpc_socket_proto protocol,
                          uint8_t *info, int info_len,
                          rpc_open_sock_flags flags);

/**
 * @b WSADuplicateSocket() remote call. Protocol info is copied to the
 * Test Engine and then back  to the TA (in rpc_wsa_socket() function)
 * as is.
 *
 * @param rpcs          RPC server
 * @param s             old socket
 * @param pid           destination process PID
 * @param info          buffer for protocol info or NULL
 * @param info_len      buffer length location (IN)/protocol info length
 *                      location (OUT)
 *
 * @return Value returned by WSADuplicateSocket() function
 */
extern int rpc_wsa_duplicate_socket(rcf_rpc_server *rpcs,
                                    int s, pid_t pid,
                                    uint8_t *info, int *info_len);

/**
 * @b DuplicateHandle() remote call.
 *
 * @param rpcs          RPC server
 * @param src           source process PID
 * @param old_fd        old socket
 * @param tgt           target process PID
 * @param new_fd        new socket location
 *
 * @return Value returned by DuplicateHandle() function
 *
 * @note @a bInheritHandle is TRUE, @a dwOptions is DUPLICATE_SAME_ACCESS
 */
extern te_bool rpc_duplicate_handle(rcf_rpc_server *rpcs,
                                    pid_t src, int old_fd,
                                    pid_t tgt, int *new_fd);

/**
 * Establish a connection to a specified socket, and optionally send data
 * once connection is established.
 *
 * @param rpcs         RPC server handle
 * @param s            socket descriptor
 * @param addr         pointer to a @b sockaddr structure containing the
 *                     address to connect to.
 * @param buf          RPC pointer to buffer containing connect data
 * @param len_buf      length of the buffer @b buf
 * @param bytes_sent   pointer a the number of bytes sent
 * @param overlapped   @b overlapped object or RPC_NULL
 *
 * @return Value returned by @b ConnectEx()
 */
extern te_bool rpc_connect_ex(rcf_rpc_server *rpcs,
                              int s, const struct sockaddr *addr,
                              rpc_ptr buf, ssize_t len_buf,
                              size_t *bytes_sent,
                              rpc_overlapped overlapped);

/**
 * Close connection to a socket and allow the socket handle to be reused.
 *
 * @param rpcs        RPC server handle
 * @param s           conencted socket descriptor
 * @param overlapped  @b overlapped object or RPC_NULL
 * @param flags       customize behavior of operation
 *
 * @return Value returned by @b DisconnectEx()
 */
extern te_bool rpc_disconnect_ex(rcf_rpc_server *rpcs, int s,
                                 rpc_overlapped overlapped, int flags);

/** Accept decision making */
typedef enum accept_verdict {
    CF_REJECT,
    CF_ACCEPT,
    CF_DEFER
} accept_verdict;

/** Accept Condition */
typedef struct accept_cond {
    unsigned short port;      /**< port */
    accept_verdict verdict;   /**< accept decision maker */
    int            timeout;   /**< timeout to sleep in ms*/
} accept_cond;

/** Maximal number of accept conditions */
#define RCF_RPC_MAX_ACCEPT_CONDS    4

/**
 * @b WSAAccept() with condition function support. List of conditions
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
 * @return New connected socket upon successful completion, otherwise
 *         -1 is returned
 *
 */
extern int rpc_wsa_accept(rcf_rpc_server *rpcs,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen, socklen_t raddrlen,
                          accept_cond *cond, int cond_num);
/**
 * Client implementation of @b AcceptEx()-@b GetAcceptExSockAddr() call.
 *
 * @param rpcs              RPC server
 * @param s                 descriptor of socket that has already been
 *                          called with the listen function
 * @param s_a               descriptor idenfifying a socket on which to
 *                          accept an incomming connection
 * @param buf               RPC pointer to the buffer to receive data
 * @param len               length of the buffer to receive data (should not
 *                          include the size of local and remote addresses)
 * @param laddr_len         length of the local address
 * @param raddr_len         length of the remote address
 * @param bytes_received    number of received data bytes
 * @param overlapped        @b overlapped object or RPC_NULL
 *
 * @return Value returned by AcceptEx() function.
 */
extern te_bool rpc_accept_ex(rcf_rpc_server *rpcs, int s, int s_a,
                             rpc_ptr buf, size_t len, size_t laddr_len,
                             size_t raddr_len, size_t *bytes_received,
                             rpc_overlapped overlapped);

/**
 * Calculate length of buffer to be allocated for @b rpc_accept_ex().
 *
 * @param rpcs    RPC server
 * @param datalen space required for data
 *
 * @return Amount of space required for data and addresses.
 */
static inline size_t
rpc_accept_ex_buflen(rcf_rpc_server *rpcs, size_t datalen)
{
    return rpc_get_sizeof(rpcs, "struct sockaddr_storage") * 2 + 32 +
           datalen;
}

/**
 * @b GetAcceptExSockAddr() remote call.
 *
 * @param rpcs          RPC server
 * @param s             descriptor of socket that was passed
 *                      to rpc_accept_ex() fuinction as 3d parameter
 * @param buf           pointer to a buffer passed to
 *                      rpc_wsa_get_overlapped_result()
 * @param len           buffer size passed to AcceptEx()
 * @param laddr_len     length of the local address passed to AcceptEx()
 * @param raddr_len     length of the remote address passed to AcceptEx()
 * @param laddr         local address returned by GetAcceptExSockAddr()
 * @param l_sa_len      LocalSockaddrLen (passed transparently)
 * @param raddr         remote address returned by GetAcceptExSockAddr()
 * @param r_sa_len      RemoteSockaddrLen (passed transparently)
 */
extern void rpc_get_accept_addr_gen(rcf_rpc_server *rpcs,
                                    int s, rpc_ptr buf,
                                    size_t len,
                                    size_t laddr_len,
                                    size_t raddr_len,
                                    struct sockaddr *laddr,
                                    size_t *l_sa_len,
                                    struct sockaddr *raddr,
                                    size_t *r_sa_len);


/**
 * @b GetAcceptExSockAddr() remote call.
 *
 * @param rpcs          RPC server
 * @param s             descriptor of socket that was passed
 *                      to rpc_accept_ex() fuinction as 3d parameter
 * @param buf           pointer to a buffer passed to
 *                      rpc_wsa_get_overlapped_result()
 * @param len           buffer size wich was passed to rpc_accept_ex()
 * @param laddr         local address returned by GetAcceptExSockAddr()
 * @param raddr         remote address returned by GetAcceptExSockAddr()
 */
static inline void
rpc_get_accept_addr(rcf_rpc_server *rpcs,
                    int s, rpc_ptr buf, size_t len,
                    struct sockaddr *laddr,
                    struct sockaddr *raddr)
{
    size_t ss_len;

    ss_len = rpc_get_sizeof(rpcs, "struct sockaddr_storage");
    assert(ss_len >= sizeof(struct sockaddr_storage));

    return rpc_get_accept_addr_gen(rpcs, s, buf, len,
                                   ss_len + 16, ss_len + 16,
                                   laddr, &ss_len, raddr, &ss_len);
}

/**
 * Transmit data over a connected socket. Data can be transferred
 * from memory or from file(s).
 *
 * @param rpcs          RPC server handle
 * @param s             connected socket descriptor
 * @param data          structure describing data sources
 * @param element_count quantity of data sources
 * @param send_size     size of data sent by one pass
 * @param overlapped    @b overlapped object or RPC_NULL
 * @param flags         call flags
 *
 * @return TRUE         if data were successfully sent,
 *         FALSE        if some error occured
 *                      (and errno is set to respective value)
 */
extern te_bool
rpc_transmit_packets(rcf_rpc_server *rpcs, int s,
                     tarpc_transmit_packets_element *data,
                     ssize_t element_count, ssize_t send_size,
                     rpc_overlapped overlapped, ssize_t flags);

/**
 * Transmit file data over a connected socket. This function uses the
 * operating system cache manager to retrive the file data, and perform
 * high-performance file data transfert over sockets.
 *
 * @param rpcs         RPC server handle
 * @param s            connected socket descriptor
 * @param file         handle of the file containing the data to be
 *                     transmited; should be got by rpc_create_file().
 * @param len          length of data to transmit
 * @param len_per_send size of each block of data in each send operation.
 * @param overlapped   @b overlapped object or RPC_NULL
 * @param head         pointer to a buffer to be transmitted before file
 *                     data is transmitted
 * @param head_len     size of buffer @b head
 * @param tail         pointer to a buffer to be transmitted after
 *                     transmission of file data.
 * @param tail_len     size of buffer @b tail
 * @param flags      call flags (See @b Transmit file for more information)
 *
 * @return Value returned by @b TransmitFile()
 */
extern te_bool rpc_transmit_file(rcf_rpc_server *rpcs, int s, int file,
                                 ssize_t len, ssize_t len_per_send,
                                 rpc_overlapped overlapped,
                                 void *head, ssize_t head_len, void *tail,
                                 ssize_t tail_len, ssize_t flags);

/**
 * Transmit file data over a connected socket. This function uses the
 * operating system cache manager to retrive the file data, and perform
 * high-performance file data transfert over sockets.
 *
 * @param rpcs           RPC server handle.
 * @param s              connected socket descriptor.
 * @param file           handle of the file containing the data to be
 *                       transmited; should be got by rpc_create_file().
 * @param len            amount of file data to transmit.
 * @param bytes_per_send size of each block of data in each send operation.
 * @param overlapped     @b overlapped object or RPC_NULL
 * @param head           a pointer valid in the TA virtual address space
 *                       and pointing to a buffer to be transmitted before
 *                       the file data.
 * @param head_len       size of buffer @b head.
 * @param tail           a pointer valid in the TA virtual address space
 *                       and pointing to a buffer to be transmitted after
 *                       the file data.
 * @param tail_len       size of buffer @b tail.
 * @param flags          @b TransmitFile call flags.
 *
 * @return   @c TRUE in case of success, @c FALSE otherwise.
 *
 * ATTENTION: when using the overlapped I/O the supplied buffers @b head
 * and @b tail will be freed when you call rpc_wsa_get_overlapped_result().
 */
extern te_bool rpc_transmitfile_tabufs(rcf_rpc_server *rpcs, int s,
                                       int file, ssize_t len,
                                       ssize_t bytes_per_send,
                                       rpc_overlapped overlapped,
                                       rpc_ptr head,
                                       ssize_t head_len, rpc_ptr tail,
                                       ssize_t tail_len, ssize_t flags);

/**
 * @b CreateFile() remote call.
 *
 * @param rpcs                  RPC server handle.
 * @param name                  Null-terminated string - the name
 *                              of object to create or open.
 * @param desired_access        The access to object.
 * @param share_mode            The sharing mode of object.
 * @param security_attributes   TA-side pointer to a Windows
 *                              SECURITY_ATTRIBUTES structure.
 * @param creation_disposition  An action to take on files that
 *                              exist and do not exist.
 * @param flags_attributes      The file attributes and flags.
 * @param template_file         TA-side handle to a template file.
 *
 * @return   TA-side handle of the object, otherwise -1.
 */
extern int rpc_create_file(rcf_rpc_server *rpcs, char *name,
                           rpc_cf_access_right desired_access,
                           rpc_cf_share_mode share_mode,
                           rpc_ptr security_attributes,
                           rpc_cf_creation_disposition creation_disposition,
                           rpc_cf_flags_attributes flags_attributes,
                           int template_file);

/**
 * @b closesocket() remote call.
 *
 * @param rpcs           RPC server handle
 * @param s              Socket to close
 *
 * @return   Non-zero on success, zero otherwise.
 */
extern int rpc_closesocket(rcf_rpc_server *rpcs, int s);

/**
 * @b CancelIo() remote call.
 *
 * @param rpcs           RPC server handle
 * @param fd             File handle which requests to be cancelled
 *                       belong to
 *
 * @return  @c TRUE (success) or @c FALSE (failure)
 */
extern te_bool rpc_cancel_io(rcf_rpc_server *rpcs, int fd);

/**
 * @b HasOverlappedIoCompleted() remote call.
 *
 * @param rpcs        RPC server handle
 * @param overlapped  @b overlapped object or RPC_NULL
 *
 * @return   @c TRUE if overlapped I/O has completed, @c FALSE otherwise.
 */
extern te_bool rpc_has_overlapped_io_completed(rcf_rpc_server *rpcs,
                                               rpc_overlapped overlapped);

/**
 * @b CreateIoCompletionPort() remote call.
 *
 * @param rpcs                          RPC server handle
 * @param file_handle                   Handle of a file/socket
 *                                      opened for overlapped I/O
 * @param existing_completion_port      Handle of the existing
 *                                      completion port.
 * @param completion_key                Per-file completion key.
 * @param number_of_concurrent_threads  Maximum number of thread that the OS
 *                                      can allow to concurrently process
 *                                      completion packets for the I/O
 *                                      completion port.
 *
 * @return   Completion port handle on success, @c 0 otherwise
 */
extern int rpc_create_io_completion_port(rcf_rpc_server *rpcs,
                                         int file_handle,
                                         int existing_completion_port,
                                         uint64_t completion_key,
                                         unsigned int
                                             number_of_concurrent_threads);

/**
 * @b GetQueuedCompletionStatus() remote call.
 *
 * @param rpcs              RPC server handle
 * @param completion_port   Handle of the existing completion port
 * @param number_of_bytes   Where to store the number of bytes
 *                          transferred during the I/O operation
 * @param completion_key    Where to store the completion key value
 *                          associated with the file handle whose I/O
 *                          has completed
 * @param overlapped        @b overlapped object or RPC_NULL
 * @param milliseconds      Timeout to wait for the I/O completion.
 *
 * @return   Non-zero on success, zero otherwise
 */
extern te_bool rpc_get_queued_completion_status(rcf_rpc_server *rpcs,
                                 int completion_port,
                                 size_t *number_of_bytes,
                                 uint64_t *completion_key,
                                 rpc_overlapped *overlapped,
                                 unsigned int milliseconds);

/**
 * @b PostQueuedCompletionStatus() remote call.
 *
 * @param rpcs              RPC server handle
 * @param completion_port   Handle of the existing completion port
 * @param number_of_bytes   Number of bytes transferred
 * @param completion_key    Completion key value
 * @param overlapped        @b overlapped object or RPC_NULL
 *
 * @return   Non-zero on success, zero otherwise
 */
extern te_bool rpc_post_queued_completion_status(rcf_rpc_server *rpcs,
                                 int completion_port,
                                 unsigned int number_of_bytes,
                                 uint64_t completion_key,
                                 rpc_overlapped overlapped);

/**
 * @b GetCurrentProcessId() remote call.
 *
 * @param rpcs   RPC server handle
 *
 * @return   The specified RPC server process identifier.
 */
extern int rpc_get_current_process_id(rcf_rpc_server *rpcs);

/**
 * Get various system information of the host
 * where the specified RPC server runs.
 *
 * @param rpcs       RPC server handle
 * @param sys_info   where to store the obtained system information.
 */
extern void rpc_get_sys_info(rcf_rpc_server *rpcs, rpc_sys_info *sys_info);

/**
 * @b @b WSARecvEx() remote call.
 *
 * @param rpcs    RPC server handle
 * @param s       socket descriptor
 * @param buf     pointer to a buffer containing the incoming data
 * @param len     Maximum length of expected data
 * @param flags   specify whether data is fully or partially received
 * @param rbuflen real size of buffer @b buf
 *
 * @return Number of bytes received upon successful completion. If the
 *         connection has been close it returned zero.
 *         Otherwise -1 is returned.
 */
extern ssize_t rpc_wsa_recv_ex(rcf_rpc_server *rpcs,
                               int s, void *buf, size_t len,
                               rpc_send_recv_flags *flags,
                               size_t rbuflen);

/**
 * Create a new event object.
 *
 * @param rpcs    RPC server handle
 *
 * @return Upon successful completion this function returns an
 *         @b rpc_wsaevent structure handle, otherwise NULL is returned
 */
extern rpc_wsaevent rpc_create_event(rcf_rpc_server *rpcs);

/**
 * Create a new event object and set lower bit.
 *
 * @param rpcs    RPC server handle
 *
 * @return Upon successful completion this function returns an
 *         @b rpc_wsaevent structure handle, otherwise NULL is returned
 */
extern rpc_wsaevent rpc_create_event_with_bit(rcf_rpc_server *rpcs);

/**
 * Close an open event object handle.
 *
 * @param rpcs    RPC server handle
 * @param hevent  handle of the event to be close
 *
 * @return value returned by @b WSACloseEvent()
 */
extern te_bool rpc_close_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);

/**
 * Reset the state of the specified event object to non-signaled.
 *
 * @param rpcs   RPC server handle
 * @param hevent event object handle
 *
 * @return Value returned by @b WSAResetEvent()
 */
extern te_bool rpc_reset_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);

/**
 * Set the state of the specified event object to signaled.
 *
 * @param rpcs    RPC server handle
 * @param  hevent  event object handle
 *
 * @return Value returned by @b WSASetEvent()
 */
extern te_bool rpc_set_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent);


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
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_address_to_string(rcf_rpc_server *rpcs,
                                     struct sockaddr *addr,
                                     socklen_t addrlen, uint8_t *info,
                                     int info_len, char *addrstr,
                                     ssize_t *addrstr_len);

/**
 * Convert a numeric string to a @b sockaddr structure.
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
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_string_to_address(rcf_rpc_server *rpcs, char *addrstr,
                                     rpc_socket_domain address_family,
                                     uint8_t *info, int info_len,
                                     struct sockaddr *addr,
                                     socklen_t *addrlen);


/**
 * Cancel an incomplete asynchronous task.
 *
 * @param rpcs   RPC server handle
 * @param async_task_handle task to be canceled
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_cancel_async_request(rcf_rpc_server *rpcs,
                                        rpc_handle async_task_handle);

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
 * @return 0 on success or -1 on failure
 */
extern int rpc_alloc_wsabuf(rcf_rpc_server *rpcs, size_t len,
                            rpc_ptr *wsabuf, rpc_ptr *wsabuf_buf);

/**
 * Free a previously allocated by @b rpc_alloc_wsabuf buffer.
 *
 * @param rpcs     RPC server handle
 * @param wsabuf   pointer to the buffer to be freed
 */
extern void rpc_free_wsabuf(rcf_rpc_server *rpcs, rpc_ptr wsabuf);

/**
 * @b WSAConnect() remote call.
 *
 * @param rpcs           RPC server handle
 * @param s              Descriptor identifying an unconnected socket
 * @param addr           pointer to a @b sockaddr structure containing the
 *                       address to connect to
 * @param caller_wsabuf  TA virtual address space valid pointer to a WSABUF
 *                       structure describing the user data that is to be
 *                       transferred to the other socket during connection
 *                       establishment
 * @param callee_wsabuf  TA virtual address space valid pointer to a WSABUF
 *                       structure describing the user data that is to be
 *                       transferred back from the other socket during
 *                       connection establishment.
 * @param sqos           TA virtual address space valid pointer to a QOS
 *                       structure for socket @b s.
 *
 * @return   0 in case of success, nonzero otherwise.
 */
extern int rpc_wsa_connect(rcf_rpc_server *rpcs, int s,
                           const struct sockaddr *addr,
                           rpc_ptr caller_wsabuf, rpc_ptr callee_wsabuf,
                           rpc_qos *sqos);

/** Maximum length of expected result */
#define RPC_WSA_IOCTL_OUTBUF_MAX        4096

/**
 * @b WSAIoctl() remote call.
 *
 * If input/output argument is list of addresses, buffer should contain
 * array of sockaddr_storage structures.
 * If input/output argument is QOS, rpc_qos structure should be in the
 * buffer.
 * If input/output argument is keepalive or GUID, corresponding tarpc_*
 * structures should be in the buffer.
 *
 * @param rpcs            RPC server handle
 * @param s               Descriptor identifying a socket
 * @param control_code    Control code of operation to perform
 * @param inbuf           Pointer to the input buffer with correct
 *                        data
 * @param inbuf_len       Size of the input buffer to be passed to
 *                        WSAIoctl()
 * @param outbuf          Pointer to the output buffer big enough to
 *                        decode any result (if length of decoded result
 *                        is greater than RPC_WSA_IOCTL_OUTBUF_MAX, error
 *                        is returned)
 * @param outbuf_len      Size of the output buffer to be passed to
 *                        WSAIoctl()
 * @param bytes_returned  Pointer to the actual number of bytes of output
 *                        (filled by WSAIoctl())
 * @param overlapped      @b overlapped object or RPC_NULL
 * @param callback        completion callback name
 *
 * @return   0 in case of success, nonzero otherwise.
 */
extern int rpc_wsa_ioctl(rcf_rpc_server *rpcs, int s,
                         rpc_ioctl_code control_code,
                         char *inbuf, unsigned int inbuf_len,
                         char *outbuf, unsigned int outbuf_len,
                         size_t *bytes_returned,
                         rpc_overlapped overlapped,
                         const char *callback);

/**
 * Retrieve the result of the preceding overlapped WSAIoctl() call.
 *
 * @param rpcs           RPC server handle
 * @param s              desccriptor identifying a socket
 * @param overlapped     @b overlapped object or RPC_NULL
 * @param bytes          pointer to the variable that will accept the
 *                       number of bytes returned in @b buf
 * @param wait           specifies whether the function should wait
 *                       for the overlapped operation to complete
 *                       (should wait, if TRUE)
 * @param flags          pointer to a variable that will receive one or
 *                       more flags that supplement the completion status
 * @param buf            pointer to a buffer containing result data
 * @param control_code   the control code the preceding WSAIoctl()
 *                       call has been called with
 *
 * @return   Nonzero in case of success, otherwise 0.
 */
extern te_bool rpc_get_wsa_ioctl_overlapped_result(rcf_rpc_server *rpcs,
                                    int s, rpc_overlapped overlapped,
                                    int *bytes, te_bool wait,
                                    rpc_send_recv_flags *flags,
                                    char *buf,
                                    rpc_ioctl_code control_code);

/**
 * Asynchronously retrieve host information by given address.
 * See @b WSAAsyncGetHostByAddr().
 *
 *  @param rpcs    RPC server handle
 *  @param hwnd    handle to a window that receive a message when the
 *                 asynchronous request completes
 *  @param wmsg    message to be received when asynchronous request
 *                 completes
 *  @param addr    pointer to the network address of the host
 *  @param addrlen size of @a addr
 *  @param type    type of the address
 *  @param buf     valid buffer pointer in the TA address space.
 *                 Contain the host entry data
 *  @param buflen  size of the buffer @b buf, in bytes
 *
 *  @return  Value returned by  @b WSAAsyncGetHostByAddr()
 */
extern rpc_handle rpc_wsa_async_get_host_by_addr(rcf_rpc_server *rpcs,
                                                 rpc_hwnd hwnd,
                                                 unsigned int wmsg,
                                                 char *addr,
                                                 ssize_t addrlen,
                                                 rpc_socket_type type,
                                                 rpc_ptr buf,
                                                 ssize_t buflen);

/**
 * Asynchronously retrieve host information by given name.
 * See @b WSAAsyncGetHostByName().
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
 * @return Value returned by  @b WSAAsyncGetHostByName()
 */
extern rpc_handle rpc_wsa_async_get_host_by_name(rcf_rpc_server *rpcs,
                                                 rpc_hwnd hwnd,
                                                 unsigned int wmsg,
                                                 char *name,
                                                 rpc_ptr buf,
                                                 ssize_t buflen);
/**
 * Asynchronously retrieve protocol information by given name.
 * See @b WSAAsyncGetProtoByName().
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
 * @return  Value returned by  @b WSAAsyncGetProtoByName()
 */
extern rpc_handle rpc_wsa_async_get_proto_by_name(rcf_rpc_server *rpcs,
                                                  rpc_hwnd hwnd,
                                                  unsigned int wmsg,
                                                  char *name,
                                                  rpc_ptr buf,
                                                  ssize_t buflen);

/**
 * Asynchronously retrieve protocol information by given number.
 * See @b WSAAsyncGetProtoByNumber().
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
 * @return  Value returned by  @b WSAAsyncGetProtoByNumber()
 */
extern rpc_handle rpc_wsa_async_get_proto_by_number(rcf_rpc_server *rpcs,
                                                    rpc_hwnd hwnd,
                                                    unsigned int wmsg,
                                                    int number,
                                                    rpc_ptr buf,
                                                    ssize_t buflen);

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
 * @return  Value returned by  @b WSAAsyncGetServByName()
 */
extern rpc_handle rpc_wsa_async_get_serv_by_name(rcf_rpc_server *rpcs,
                                                 rpc_hwnd hwnd,
                                                 unsigned int wmsg,
                                                 char *name,
                                                 char *proto,
                                                 rpc_ptr buf,
                                                 ssize_t buflen);

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
 * @return  value returned by  @b WSAAsyncGetServByName()
 */
extern rpc_handle rpc_wsa_async_get_serv_by_port(rcf_rpc_server *rpcs,
                                                 rpc_hwnd hwnd,
                                                 unsigned int wmsg,
                                                 int port, char *proto,
                                                 rpc_ptr buf,
                                                 ssize_t buflen);

/**
 * Create WSAOVERLAPPED structure on TA side.
 *
 * @param rpcs        RPC server handle
 * @param hevent      handle to an event object
 * @param offset      position at which to start the transfert
 * @param offset_high high-order word of the position at which to start
 *                    the transfert
 * @param ...         cookie1 and cookie2 may be passed
 *
 * @return WSAOVERLAPPED structure upon successful completion or RPC_NULL
 */
extern rpc_overlapped rpc_create_overlapped(rcf_rpc_server *rpcs,
                                            rpc_wsaevent hevent,
                                            unsigned int offset,
                                            unsigned int offset_high, ...);

/**
 * Delete specified WSAOVERLAPPED structure.
 *
 * @param rpcs        RPC server handle
 * @param overlapped  @b overlapped object
 */
extern void rpc_delete_overlapped(rcf_rpc_server *rpcs,
                                  rpc_overlapped overlapped);

/**
 * Send data on a connected socket.
 *
 * @param rpcs        RPC server handle
 * @param s           connected socket descriptor
 * @param iov         pointer to a vector of buffers containing the data
 *                    to be sent
 * @param iovcnt      number of buffers in the vector
 * @param flags       modifies the behavior of the call.
 * @param bytes_sent  pointer to the number of bytes sent
 * @param overlapped  @b overlapped object or RPC_NULL
 * @param callback    completion callback name
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_send(rcf_rpc_server *rpcs,
                       int s, const struct rpc_iovec *iov,
                       size_t iovcnt, rpc_send_recv_flags flags,
                       ssize_t *bytes_sent, rpc_overlapped overlapped,
                       const char *callback);

/**
 * Receive data from a connected socket.
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
 * @param overlapped  @b overlapped object or RPC_NULL
 * @param callback    completion callback name
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_recv(rcf_rpc_server *rpcs,
                        int s, const struct rpc_iovec *iov,
                        size_t iovcnt, size_t riovcnt,
                        rpc_send_recv_flags *flags,
                        ssize_t *bytes_received, rpc_overlapped overlapped,
                        const char *callback);

/**
 * Send data to a specified destination.
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
 * @param overlapped   @b overlapped object or RPC_NULL
 * @param callback     completion callback name
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_send_to(rcf_rpc_server *rpcs, int s,
                           const struct rpc_iovec *iov,
                           size_t iovcnt, rpc_send_recv_flags flags,
                           ssize_t *bytes_sent, const struct sockaddr *to,
                           rpc_overlapped overlapped,
                           const char *callback);

/**
 * Receive datagram from socket.
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
 * @param overlapped     @b overlapped object or RPC_NULL
 * @param callback       completion callback name
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_recv_from(rcf_rpc_server *rpcs, int s,
                             const struct rpc_iovec *iov, size_t iovcnt,
                             size_t riovcnt, rpc_send_recv_flags *flags,
                             ssize_t *bytes_received,
                             struct sockaddr *from, socklen_t *fromlen,
                             rpc_overlapped overlapped,
                             const char *callback);

/**
 * Initiate termination of the connection for the socket
 * and send disconnect data.
 *
 * @param rpcs    RPC server handle
 * @param s       descriptor identifying a socket
 * @param iov     vector of buffers containing the disconnect data
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_send_disconnect(rcf_rpc_server *rpcs,
                                   int s, const struct rpc_iovec *iov);

/**
 * Terminate reception on a socket, and retrieve disconnect data
 * in case of connection oriented socket.
 *
 * @param rpcs    RPC server handle
 * @param s       descriptor identifying a socket
 * @param iov      vector of buffers containing the disconnect data
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_recv_disconnect(rcf_rpc_server *rpcs,
                                   int s, const struct rpc_iovec *iov);

/**
 * Retrieve data and control information from connected or unconnected
 * sockets.
 *
 * @param rpcs           RPC server handle
 * @param s              descriptor identifying a socket
 * @param msg            pointer to a @b rpc_msghdr structure containing
 *                       received data
 * @param bytes_received pointer to the number of bytes received
 * @param overlapped     @b overlapped object or RPC_NULL
 * @param callback        completion callback name
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_recv_msg(rcf_rpc_server *rpcs, int s,
                            struct rpc_msghdr *msg,
                            ssize_t *bytes_received,
                            rpc_overlapped overlapped,
                            const char *callback);

/**
 * Retrieve the result of an overlapped operation on a specified socket.
 *
 * @param rpcs           RPC server handle
 * @param s              desccriptor identifying a socket
 * @param overlapped     @b overlapped object or RPC_NULL
 * @param bytes          pointer to the number of bytes that were
 *                       transfered by a send or receive operation
 * @param wait           specifies whether the function should wait for
 *                       the overlapped operation to complete
 *                       (should wait, if TRUE)
 * @param flags          pointer to a variable that will receive one or
 *                       more flags that supplement the completion status
 * @param buf            pointer to a buffer containing result data
 * @param buflen         size of buffer @b buf
 *
 * @return TRUE on success, FALSE on failure
 */
extern te_bool rpc_wsa_get_overlapped_result(rcf_rpc_server *rpcs,
                                             int s,
                                             rpc_overlapped overlapped,
                                             size_t *bytes, te_bool wait,
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
                                   int *called, int *error, ssize_t *bytes,
                                   rpc_overlapped *overlapped);

/**
 * Cleanup completion callback statistics.
 *
 * @param rpcs          RPC server handle
 */
static inline void
rpc_cleanup_completion_callback(rcf_rpc_server *rpcs)
{
    rpc_overlapped ovl;
    int            tmp;
    ssize_t        stmp;

    rpc_completion_callback(rpcs, &tmp, &tmp, &stmp, &ovl);
}

/**
 * Specify an event object to be associated with the specified set of
 * network events.
 *
 * @param rpcs         RPC server handle
 * @param s            socket descriptor
 * @param event_object event object
 * @param event        bitmask the specifies the combination of network
 *                     events
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_event_select(rcf_rpc_server *rpcs,
                                int s, rpc_wsaevent event_object,
                                rpc_network_event event);

/**
 * @b WSAEnumNetworkEvents() remote call.
 *
 * @param rpcs              RPC server
 * @param s                 socket descriptor
 * @param event_object      event object to be reset
 * @param events            network events that occurred
 *
 * @return Value returned by WSAEnumNetworkEvents() function
 */
extern int rpc_enum_network_events(rcf_rpc_server *rpcs,
                                  int s, rpc_wsaevent event_object,
                                  struct tarpc_network_events *events);

/** Return codes for rpc_wait_for_multiple_events */
enum {
    WSA_WAIT_FAILED = 1,
    WAIT_IO_COMPLETION,
    WSA_WAIT_TIMEOUT,
    WSA_WAIT_EVENT_0
};

/**
 * Convert @b WSAWaitForMultipleEvents() return code to a string
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
        case 0:                  return "0";
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
 * @b WSAWaitForMultipleEvents() remote call.
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
 *
 * @return The event object that make the function to return.
 *         -1 is returned in the case of RPC error
 */
extern int rpc_wait_for_multiple_events(rcf_rpc_server *rpcs,
                                        int count, rpc_wsaevent *events,
                                        te_bool wait_all, uint32_t timeout,
                                        te_bool alertable);


/**
 * Create a window for receiving event notifications.
 *
 * @param rpcs RPC server handle
 *
 * @return handle of the created window, upon successful completion,
 *         otherwise NULL is returned
 */
extern rpc_hwnd rpc_create_window(rcf_rpc_server *rpcs);

/**
 * Destroy the specified window.
 *
 * @param rpcs RPC server handle
 * @param hwnd handle of window to be destroyed
 */
extern void rpc_destroy_window(rcf_rpc_server *rpcs, rpc_hwnd hwnd);

/**
 * Request window-based notification of network events for a socket.
 *
 * @param rpcs   RPC server handle
 * @param s      socket descriptor
 * @param hwnd   handle of the window which receives message when a network
 *               event occurs
 * @param event  bitmask that specifies a combination of network events
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_wsa_async_select(rcf_rpc_server *rpcs,
                                int s, rpc_hwnd hwnd,
                                rpc_network_event event);

/**
 * Check the thread message queue for a posted message and retrieve
 * the message, if any exist
 *
 * @param rpcs    RPC server handle
 * @param hwnd    handle of the windows whose messages are to be examinied
 * @param s       socket descriptor whose event causes message notification
 * @param event   pointer to the bitmask that specifies a combination
 *                of network events
 *
 * @return Value returned by @b PeekMessage()
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



/**
 * Joins a leaf node into a multipoint session.
 *
 * @param rpcs           RPC server handle
 * @param s              Descriptor identifying an unconnected socket
 * @param addr           pointer to a @b sockaddr structure containing the
 *                       address to connect to
 * @param caller_wsabuf  TA virtual address space valid pointer to a WSABUF
 *                       structure describing the user data that is to be
 *                       transferred to the other socket during connection
 *                       establishment
 * @param callee_wsabuf  TA virtual address space valid pointer to a WSABUF
 *                       structure describing the user data that is to be
 *                       transferred back from the other socket during
 *                       connection establishment.
 * @param sqos           TA virtual address space valid pointer to a QOS
 *                       structure for socket @b s.
 * @param flag           Flag to indicate that the socket is acting as a
 *                       sender (JL_SENDER_ONLY), receiver
 *                       (JL_RECEIVER_ONLY), or both (JL_BOTH).
 *
 * @return               The value of type SOCKET that is a descriptor for
 *                       the newly created multipoint socket in case of
 *                       success, a value of INVALID_SOCKET otherwise.
 */
extern int rpc_wsa_join_leaf(rcf_rpc_server *rpcs, int s,
                            const struct sockaddr *addr,
                            rpc_ptr caller_wsabuf, rpc_ptr callee_wsabuf,
                            rpc_qos *sqos, rpc_join_leaf_flags flags);

/**
 * @b ReadFile() remote call.
 *
 * @param rpcs       RPC server handle
 * @param fd         file descriptor
 * @param buf        buffer for data
 * @param count      number of bytes to be read
 * @param received   location for number of read bytes
 * @param overlapped @b overlapped object or RPC_NULL
 *
 * @return  TRUE (success) of FALSE (failure)
 */
extern te_bool rpc_read_file(rcf_rpc_server *rpcs,
                             int fd, void *buf, size_t count,
                             size_t *received, rpc_overlapped overlapped);

/**
 * @b WriteFile() remote call.
 *
 * @param rpcs       RPC server handle
 * @param fd         file descriptor
 * @param buf        buffer for data
 * @param count      number of bytes to be sent
 * @param sent       location for number of sent bytes
 * @param overlapped @b overlapped object or RPC_NULL
 *
 * @return  TRUE (success) of FALSE (failure)
 */
extern te_bool rpc_write_file(rcf_rpc_server *rpcs,
                              int fd, void *buf, size_t count,
                              size_t *sent, rpc_overlapped overlapped);

/**
 * @b ReadFileEx() remote call.
 *
 * @param rpcs       RPC server handle
 * @param fd         file descriptor
 * @param buf        buffer for data
 * @param count      number of bytes to be read
 * @param overlapped @b overlapped object or RPC_NULL
 * @param callback   completion callback name
 *
 * @return  TRUE (success) of FALSE (failure)
 */
extern te_bool rpc_read_file_ex(rcf_rpc_server *rpcs,
                                int fd, void *buf, size_t count,
                                rpc_overlapped overlapped,
                                const char *callback);

/**
 * @b WriteFile() remote call.
 *
 * @param rpcs       RPC server handle
 * @param fd         file descriptor
 * @param buf        buffer for data
 * @param count      number of bytes to be sent
 * @param overlapped @b overlapped object or RPC_NULL
 * @param callback   completion callback name
 *
 * @return  TRUE (success) of FALSE (failure)
 */
extern te_bool rpc_write_file_ex(rcf_rpc_server *rpcs,
                                 int fd, void *buf, size_t count,
                                 rpc_overlapped overlapped,
                                 const char *callback);

/**
 * Overfill the buffers on receive and send sides of TCP connection.
 * If socket is blocking, it will be set non-blocking inside and
 * changed back on return.
 *
 * @param rpcs          RPC server
 * @param sock          socket for sending
 * @param sent          total bytes written to sending socket
 *                      while both sending and receiving side buffers
 *                      are overfilled
 * @param is_nonblocking TRUE if socket is in non-blocking mode, FALSE
 *                      otherwise.
 *
 * @return -1 in the case of failure or 0 on success
 */
extern int rpc_overfill_buffers_ex(rcf_rpc_server *rpcs, int sock,
                                   uint64_t *sent, te_bool is_nonblocking);


/** Convert WSA function name to RPC name */
static inline const char *
wsa_name_convert(const char *name)
{
    if (strcmp(name, "WSARecv") == 0)
        return "wsa_recv";
    else if (strcmp(name, "WSARecvFrom") == 0)
        return "wsa_recv_from";
    else if (strcmp(name, "WSASend") == 0)
        return "wsa_send";
    else if (strcmp(name, "WSASendTo") == 0)
        return "wsa_send_to";
    else if (strcmp(name, "WSARecvEx") == 0)
        return "wsa_recv_ex";
    else if (strcmp(name, "WSARecvMsg") == 0)
        return "wsa_recv_msg";
    else if (strcmp(name, "ReadFile") == 0)
        return "read_file";
    else if (strcmp(name, "ReadFileEx") == 0)
        return "read_file_ex";
    else if (strcmp(name, "WriteFile") == 0)
        return "write_file";
    else if (strcmp(name, "WriteFileEx") == 0)
        return "write_file_ex";
    else if (strcmp(name, "WSAAccept") == 0)
        return "wsa_accept";
    else if (strcmp(name, "WSAConnect") == 0)
        return "wsa_connect";
    else if (strcmp(name, "WSADisconnect") == 0)
        return "wsa_disconnect";
    else if (strcmp(name, "AcceptEx") == 0)
        return "accept_ex";
    else if (strcmp(name, "ConnectEx") == 0)
        return "connect_ex";
    else
        return name;
}

/**@} <!-- END te_lib_rpc_winsock2 --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_WINSOCK2_H__ */
