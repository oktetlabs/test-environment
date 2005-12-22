/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for auxilury remote calls
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * Get host value of sizeof(type_name).
 *
 * @param type_name Name of the type
 *
 * @return          Size of the type or 
 *                  -1 if such a type does not exist.
 */
extern tarpc_ssize_t rpc_get_sizeof(rcf_rpc_server *rpcs,
                                    const char *type_name);

/**
 * Convert 'struct timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv    - pointer to 'struct timeval'
 *
 * @return null-terminated string
 */
extern const char *tarpc_timeval2str(const struct tarpc_timeval *tv);

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
 * @return Number of sent bytes or -1 in the case of failure if ignore_err
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
 * Receive and verify all acceptable data on socket.
 * Verification made by function, which name is passed.
 * This function should generate block of data by start sequence
 * number (which is passed) and length. Then received and generated
 * buffers are compared. 
 * Socket method recv(..., MSG_DONTWAIT) used. 
 *
 * @param handle            RPC server
 * @param s                 a socket to be user for receiving
 * @param gen_data_fname    name of function to generate data
 * @param start             sequence number of first byte will be 
 *                          received. 
 *
 * @return number of received bytes, -1 if system error, -2 if data
 *         not match.
 */
extern int rpc_recv_verify(rcf_rpc_server *handle, int s,
                           const char *gen_data_fname, uint64_t start);
                            
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
 * @return Number of processed bytes or -1 in the case of failure 
 */
extern ssize_t rpc_socket_to_file(rcf_rpc_server *handle,
                                  int sock, const char *path_name,
                                  long timeout);

/**
 * Open FTP connection for reading/writing the file.
 * Control connection should be closed via ftp_close.
 *
 * @param handle        RPC server
 * @param uri           FTP uri: ftp://user:password@server/directory/file
 * @param rdonly        if TRUE, get file
 * @param passive       if TRUE, passive mode
 * @param offset        file offset
 * @param sock          pointer to a socket descriptor
 *
 * @return File descriptor, which may be used for reading/writing data
 */
extern int rpc_ftp_open(rcf_rpc_server *handle,
                        char *uri, te_bool rdonly, te_bool passive,
                        int offset, int *sock);

/**
 * Close FTP control connection.
 *
 * @param handle        RPC server
 * @param sock          control socket descriptor
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_ftp_close(rcf_rpc_server *handle, int sock);

/**
 * Overfill the buffers on receive and send sides of TCP connection.
 *
 * @param rpcs          RPC server
 * @param sock          socket for sending
 * @param sent          total bytes written to sending socket
 *                      while both sending and receiving side buffers
 *                      are overfilled.
 *
 * @return -1 in the case of failure or 0 on success
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

/**
 * Copy the @b src_buf buffer in @b dst_buf located in TA address space.
 *
 * @param rpcs     RPC server handle
 * @param src_buf  pointer to the source buffer
 * @param len      length of data to be copied
 * @param dst_buf  pointer to the destination buffer
 * @param offset   displacement in the destination buffer
 */
extern void rpc_set_buf(rcf_rpc_server *rpcs, const uint8_t *src_buf,
                        size_t len, rpc_ptr dst_buf, rpc_ptr offset);

/**
 * Fill @b dst_buf located in TA address space by specified pattern
 *
 * @param rpcs     RPC server handle
 * @param pattern  pattern to be used for buffer filling 
 *                 (TAPI_RPC_BUF_RAND if the buffer
 *                 should be filled by random data)
 * @param len      length of data to be copied
 * @param dst_buf  pointer to the destination buffer
 * @param offset   displacement in the destination buffer
 */
extern void rpc_set_buf_pattern(rcf_rpc_server *rpcs, int pattern,
                                size_t len, rpc_ptr dst_buf, 
                                rpc_ptr offset);

/**
 * Copy the @b src_buf buffer located in TA address space to the 
 * @b dst_buf buffer.
 *
 * @param rpcs     RPC server handle
 * @param src_buf  source buffer
 * @param offset   displacement in the source buffer
 * @param len      length of data to be copied
 * @param dst_buf  destination buffer
 */
extern void rpc_get_buf(rcf_rpc_server *rpcs, rpc_ptr src_buf,
                        rpc_ptr offset, size_t len, uint8_t *dst_buf);


/**
 * Create a child process (with a duplicated socket in case of winsock2).
 *
 * @param pco_father     RPC server handle
 * @param father_s       socket on @b pco_father
 * @param domain         domain, used in test
 * @param sock_type      type of socket, used in test
 * @param pco_child      new process
 * @param child_s        duplicated socket on @b pco_child
 */
extern void rpc_create_child_process_socket(rcf_rpc_server *pco_father, 
                                            int father_s, 
                                            rpc_socket_domain domain,
                                            rpc_socket_type sock_type,
                                            rcf_rpc_server **pco_child, 
                                            int *child_s);

#endif /* !__TE_TAPI_RPC_MISC_H__ */
