/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for auxilury remote calls
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
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_MISC_H__
#define __TE_TAPI_RPC_MISC_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_rpc.h"


/**
 * Set dynamic library name to be used for additional name resolution.
 *
 * @param rpcs          existing RPC server handle
 * @param libname       name of the dynamic library or NULL
 *
 * @return Status code
 */
extern int rpc_setlibname(rcf_rpc_server *rpcs, const char *libname);


/**
 * Convert 'struct timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv    - pointer to 'struct timeval'
 *
 * @return null-terminated string
 */
extern const char *timeval2str(const struct timeval *tv);

/**
 * Convert 'struct timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv    - pointer to 'struct timespec'
 *
 * @return null-terminated string
 */
extern const char *timespec2str(const struct timespec *tv);


/**
 * Simple sender.
 *
 * @param handle            RPC server
 * @param s                 a socket to be user for sending
 * @param size_min          minimum size of the message in bytes
 * @param size_max          maximum size of the message in bytes
 * @param size_rnd_once     if true, random size should be calculated
 *                          only once and used for all messages;
 *                          if false, random size is calculated for
 *                          each message
 * @param delay_min         minimum delay between messages in microseconds
 * @param delay_max         maximum delay between messages in microseconds
 * @param delay_rnd_once    if true, random delay should be calculated
 *                          only once and used for all messages;
 *                          if false, random delay is calculated for
 *                          each message
 * @param time2run          how long run (in seconds)
 * @param sent              location for number of sent bytes
 * @param ignore_err        Ignore errors while run
 *
 * @return number of sent bytes or -1 in the case of failure if ignore_err
 *         set to FALSE or number of sent bytes or 0 if ignore_err set to
 *         TRUE
 */
extern int rpc_simple_sender(rcf_rpc_server *handle,
                             int s,
                             int size_min, int size_max,
                             int size_rnd_once,
                             int delay_min, int delay_max,
                             int delay_rnd_once,
                             int time2run, uint64_t *sent,
                             int ignore_err);

/**
 * Simple receiver.
 *
 * @param handle            RPC server
 * @param s                 a socket to be user for receiving
 * @param time2run          how long run (in seconds)
 * @param received          location for number of received bytes
 *
 * @return number of received bytes or -1 in the case of failure
 */
extern int rpc_simple_receiver(rcf_rpc_server *handle,
                               int s, uint32_t time2run,
                               uint64_t *received);

/**
 * Send traffic. Send UDP datagrams from different sockets
 * toward different addresses.
 *
 * @param handle        RPC server
 * @param num           number of UDP datagrams to being sent
 * @param s             list of sockets (num)
 * @param buf           buffer to being sent
 * @param len           buffer size
 * @param flags         flags passed to sendto()
 * @param to            list of sockaddr-s (num)
 * @param tolen         address size
 * 
 * @return 0 in case of success, -1 in case of failure
 */ 
extern int rpc_send_traffic(rcf_rpc_server *handle, int num, int *s,
                            const void *buf, size_t len, int flags,
                            struct sockaddr *to, socklen_t tolen);
                            
/**
 * For each address from addresses list routine sends UDP datagram,
 * receives it back and check that it happens within determined
 * period of time. sendmsg() and recvmsg() are used.
 *
 * @param rpcs          RPC server
 * @param s             DGRAM socket to send/receive UDP datagram
 * @param size          Size of UDP datagram
 * @param vector_len    iovec_len in msghdr structure
 * @param timeout       routine waiting for UDP datagram coming back
 *                      within timeout given (passed to select)
 * @param time2wait     UDP datagram must be sent and received within
 *                      time2wait period
 * @param flags         flags passed to sendmsg(recvmsg)
 * @param num           number of addresses in addresses list
 * @param to            addresses list
 * @param tolen         address length
 *
 * @return 
 *     0 - success
 *     ROUND_TRIP_ERROR_SEND - sendmsg() failed
 *     ROUND_TRIP_ERROR_RECV - recvmsg() failed
 *     ROUND_TRIP_ERROR_TIMEOUT - select() returned because
 *                                timeout expired
 *     ROUND_TRIP_ERROR_TIME_EXPIRED - time2wait expired
 *     ROUND_TRIP_ERROR_OTHER - some other error occured 
 *                             (memory allocation etc.)
 */ 
extern int rpc_timely_round_trip(rcf_rpc_server *rpcs, int s,
                                 size_t size, size_t vector_len,
                                 uint32_t timeout, uint32_t time2wait,
                                 int flags, int num, struct sockaddr *to,
                                 socklen_t tolen);
  
/**
 * For each DGRAM socket in socket list routine determines 
 * if the socket is readable, if it so, routine called recvmsg() 
 * to receive UDP datagram, and sends it back using recvmsg().
 *
 * @param rpcs           RPC server
 * @param num            number of sockets in sockets list
 * @param s              DGRAM sockets list
 * @param size           Size of UDP datagram
 * @param vector_len     iovec_len in msghdr structure
 * @param timeout        routine waiting for UDP daragram coming
 *                       withing timeout given (passed to select)
 * @param flags          flags passed to recvmsg(sendmsg)
 *
 * @return
 *     0 - success
 *     ROUND_TRIP_ERROR_SEND - sendmsg() failed
 *     ROUND_TRIP_ERROR_RECV - recvmsg() failed
 *     ROUND_TRIP_ERROR_TIMEOUT - select() returned because
 *                                timeout expired
 *     ROUND_TRIP_ERROR_OTHER - some other error occured
 *                              (memory allocation etc.)
 */ 
extern int rpc_round_trip_echoer(rcf_rpc_server *rpcs, int num, int *s,
                                 size_t size, size_t vector_len,
                                 uint32_t timeout, int flags);

/**
 * Routine which receives data from specified set of sockets and sends
 * data to specified set of sockets with maximum speed using I/O
 * multiplexing.
 *
 * @param handle        RPC server
 * @param rcvrs         set of receiver sockets
 * @param rcvnum        number of receiver sockets
 * @param sndrs         set of sender sockets
 * @param sndnum        number of sender sockets
 * @param bulkszs       sizes of data bulks to send for each sender
 *                      (in bytes, 1024 bytes maximum)
 * @param time2run      how long send data (in seconds)
 * @param time2wait     how long wait data (in seconds)
 * @param iomux         type of I/O Multiplexing function
 *                      (@b select(), @b pselect(), @b poll())
 * @param rx_nonblock   push all Rx sockets in nonblocking mode
 * @param tx_stat       Tx statistics for set of sender socket to be
 *                      updated (IN/OUT)
 * @param rx_stat       Rx statistics for set of receiver socket to be
 *                      updated (IN/OUT)
 *
 * @attention Non-blocking mode is required if the same socket is used
 *            in many contexts (in order to avoid deadlocks).
 *
 * @return number of sent bytes or -1 in the case of failure
 */
extern int rpc_iomux_flooder(rcf_rpc_server *handle,
                             int *sndrs, int sndnum, int *rcvrs, int rcvnum,
                             int bulkszs, int time2run, int time2wait,
                             int iomux, te_bool rx_nonblock,
                             uint64_t *tx_stat,
                             uint64_t *rx_stat);

/**
 * Routine which receives data from specified set of
 * sockets using I/O multiplexing and sends them back
 * to the socket.
 *
 * @param handle        RPC server
 * @param sockets       Set of sockets to be processed
 * @param socknum       number of sockets to be processed
 * @param time2run      how long send data (in seconds)
 * @param iomux         type of I/O Multiplexing function
 *                      (@b select(), @b pselect(), @b poll())
 * @param tx_stat       Tx statistics for set of sender socket to be
 *                      updated (IN/OUT)
 * @param rx_stat       Rx statistics for set of receiver socket to be
 *                      updated (IN/OUT)
 *
 * @return 0 on success or -1 in the case of failure
 */
extern int rpc_iomux_echoer(rcf_rpc_server *handle,
                            int *sockets, int socknum,
                            int time2run, int iomux,
                            uint64_t *tx_stat, uint64_t *rx_stat);

/**
 * Routine which receives data from opened socket descriptor and write its
 * to the file. Processing is finished when timeout expired.
 *
 * @param handle        RPC server
 * @param sock          socket descriptor for data receiving
 * @param path_name     path name where write received data
 * @param timeout       timeout to finish processing
 *
 * @return    number of processed bytes
 *            or -1 in the case of failure and appropriate errno
 */
extern ssize_t rpc_socket_to_file(rcf_rpc_server *handle,
                                  int sock, const char *path_name,
                                  long timeout);

/**
 * Open the connection for reading/writing the file.
 *
 * @param handle        RPC server
 * @param uri           FTP uri: ftp://user:password@server/directory/file
 * @param rdonly        if TRUE, get file
 * @param passive       if TRUE, passive mode
 * @param offset        file offset
 * @param sock          pointer to a socket descriptor
 *
 * @return file descriptor, which may be used for reading/writing data
 */
extern int rpc_ftp_open(rcf_rpc_server *handle,
                        char *uri, te_bool rdonly, te_bool passive,
                        int offset, int *sock);

/**
 * Execute a number of send() operation each after other with no delay.
 *
 *
 * @param handle        RPC server
 * @param sock          socket for sending
 * @param nops          The number of send() operation should be executed
 *                      (the length of len_array)
 * @param vector        array of lenghts for appropriate send() operation
 * @param sent          total bytes are sent on exit
 *
 * @return   -1 in the case of failure or 0 on success
 */
extern int rpc_many_send(rcf_rpc_server *handle, int sock,
                         const int *vector, int nops, uint64_t *sent);

/**
 * Overfill the buffers on receive and send sides of TCP connection.
 *
 * @param rpcs          RPC server
 * @param sock          socket for sending
 * @param sent          total bytes written to sending socket
 *                      while both sending and receiving side buffers
 *                      are overfilled.
 *
 * @return    -1 in the case of failure or 0 on success
 */
extern int rpc_overfill_buffers(rcf_rpc_server *rpcs, int sock,
                                uint64_t *sent);

/**
 * VM trasher to keep memory pressure on the
 * host where the specified RPC server runs.
 *
 * @param rpcs          RPC server
 * @param start         TRUE if want to start the VM trasher,
 *                      FALSE if want to stop the VM trasher.
 */
extern void rpc_vm_trasher(rcf_rpc_server *rpcs, te_bool start);

#endif /* !__TE_TAPI_RPC_MISC_H__ */
