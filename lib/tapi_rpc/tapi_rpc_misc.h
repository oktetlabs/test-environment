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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_rpc.h"
#include "te_rpc_sys_resource.h"
#include "te_rpc_sys_systeminfo.h"
#include "tapi_rpc_signal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get host value of sizeof(type_name).
 *
 * @param handle      RPC server
 * @param type_name   Name of the type
 *
 * @return          Size of the type or 
 *                  -1 if such a type does not exist.
 */
extern tarpc_ssize_t rpc_get_sizeof(rcf_rpc_server *rpcs,
                                    const char *type_name);

/**
 * Compare protocol information (WSAPROTOCOL_INFO)
 *
 * @param rpcs               RPC server
 * @param buf1               buffer containing protocol info to compare
 * @param buf2               buffer containing protocol info to compare
 * @param is_wide1           boolean defining if fields of first 
 *                           protocol info  are wide-character 
 * @param is_wide2           boolean defining if fields of second 
 *                           protocol info are wide-character       
 *
 * @return                   TRUE if information in @p buf1 is equal to 
 *                           information in buf2
 */
extern tarpc_bool rpc_protocol_info_cmp(rcf_rpc_server *rpcs, 
                                        const uint8_t *buf1,
                                        const uint8_t *buf2,
                                        tarpc_bool is_wide1,
                                        tarpc_bool is_wide2);



/**
 * Get address of the variable known on RPC server.
 *
 * @param handle RPC server
 * @param name   variable name
 *
 * @return RPC address pointer or NULL is variable is not found
 */
extern rpc_ptr rpc_get_addrof(rcf_rpc_server *rpcs, const char *name);

/**
 * Get value of the integer variable.
 *
 * @param handle  RPC server
 * @param name    variable name
 * @param size    variable size (1, 2, 4, 8)
 *
 * @return variable value
 *
 * @note Routine jumps to failure is the variable is not found or incorrect
 *       parameters are provided
 */
extern uint64_t rpc_get_var(rcf_rpc_server *rpcs, 
                            const char *name, tarpc_size_t size);

/**
 * Change value of the integer variable.
 *
 * @param handle  RPC server
 * @param name    variable name
 * @param size    variable size (1, 2, 4, 8)
 * @param val     variable value
 *
 * @note Routine jumps to failure is the variable is not found or incorrect
 *       parameters are provided
 */
extern void rpc_set_var(rcf_rpc_server *rpcs, 
                        const char *name, tarpc_size_t size, uint64_t val);

/**
 * Convert 'struct timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv     pointer to 'struct timeval'
 *
 * @return null-terminated string
 */
extern const char *tarpc_timeval2str(const struct tarpc_timeval *tv);

/**
 * Convert 'struct timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv     pointer to 'struct timespec'
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
 * Wait for readable socket.
 *
 * @param rpcs            RPC server
 * @param s               a socket to be user for select
 * @param timeout         Receive timeout (in milliseconds)
 *
 * @return result of select() call
 */
extern int rpc_wait_readable(rcf_rpc_server *rpcs, int s, uint32_t timeout);

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
 * On Windows, this call excepts socket to be in blocking mode. To use
 * it on non-blocking sockets refer to rpc_overfill_buffers_ex.
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
 * Copy the @b src_buf buffer in @b dst_buf located in TA address space
 *
 * @param rpcs     RPC server handle
 * @param src_buf  pointer to the source buffer
 * @param len      length of data to be copied
 * @param dst_buf  pointer to the destination buffer
 * @param dst_off   displacement in the destination buffer
 */
extern void rpc_set_buf_gen(rcf_rpc_server *rpcs, const uint8_t *src_buf,
                            size_t len, rpc_ptr dst_buf, size_t dst_off);
static inline void
rpc_set_buf(rcf_rpc_server *rpcs, const uint8_t *src_buf,
            size_t len, rpc_ptr dst_buf)
{
    rpc_set_buf_gen(rpcs, src_buf, len, dst_buf, 0);
}
static inline void
rpc_set_buf_off(rcf_rpc_server *rpcs, const uint8_t *src_buf,
                size_t len, rpc_ptr_off *dst_buf)
{
    rpc_set_buf_gen(rpcs, src_buf, len, dst_buf->base, dst_buf->offset);
}

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
extern void rpc_set_buf_pattern_gen(rcf_rpc_server *rpcs, int pattern,
                                    size_t len, 
                                    rpc_ptr dst_buf, size_t dst_off);
static inline void
rpc_set_buf_pattern(rcf_rpc_server *rpcs, int pattern,
            size_t len, rpc_ptr dst_buf)
{
    rpc_set_buf_pattern_gen(rpcs, pattern, len, dst_buf, 0);
}
static inline void
rpc_set_buf_pattern_off(rcf_rpc_server *rpcs, int pattern,
                        size_t len, rpc_ptr_off *dst_buf)
{
    rpc_set_buf_pattern_gen(rpcs, pattern, len, 
                            dst_buf->base, dst_buf->offset);
}

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
extern void rpc_get_buf_gen(rcf_rpc_server *rpcs, 
                            rpc_ptr src_buf, size_t src_off,
                            size_t len, uint8_t *dst_buf);
static inline void
rpc_get_buf(rcf_rpc_server *rpcs, rpc_ptr src_buf,
            size_t len, uint8_t *dst_buf)
{
    rpc_get_buf_gen(rpcs, src_buf, 0, len, dst_buf);
}
static inline void
rpc_get_buf_off(rcf_rpc_server *rpcs, rpc_ptr_off *src_buf,
                size_t len, uint8_t *dst_buf)
{
    rpc_get_buf_gen(rpcs, src_buf->base, src_buf->offset, len, dst_buf);
}


/**
 * Create a child process (with a duplicated socket in case of winsock2).
 *
 * @param method         "inherit", "DuplicateSocket" or "DuplicateHandle"
 * @param pco_father     RPC server handle
 * @param father_s       socket on @b pco_father
 * @param domain         domain, used in test
 * @param sock_type      type of socket, used in test
 * @param pco_child      new process
 * @param child_s        duplicated socket on @b pco_child
 */
extern void rpc_create_child_process_socket(const char *method,
                                            rcf_rpc_server *pco_father, 
                                            int father_s, 
                                            rpc_socket_domain domain,
                                            rpc_socket_type sock_type,
                                            rcf_rpc_server **pco_child, 
                                            int *child_s);

/**
 * Wrapper for rpc_sigaction() function to install 'sa_sigaction'
 * with blocking of all signals and @c SA_RESTART flag.
 *
 * @param rpcs      RPC server handle
 * @param signum    Signal number
 * @param handler   Signal handler 'sa_sigaction'
 * @param oldact    Pointer to previously associated with the signal
 *                  action or NULL
 *
 * @return Status code.
 */
extern te_errno tapi_sigaction_simple(rcf_rpc_server *rpcs,
                                      rpc_signum signum,
                                      const char *handler,
                                      struct rpc_struct_sigaction *oldact);

/**
 * Join a multicasting group on specified interface.
 * 
 * @param  rpcs       RPC server handle
 * @param  s          socket descriptor
 * @param  mcast_addr multicast address (IPv4 or IPv6).
 * @param  if_index   interface index
 * @param  how        joining method:
 * 
 *    @value TARPC_MCAST_OPTIONS   IP_ADD/DROP_MEMBERSHIP options
 *    @value TARPC_MCAST_WSA       WSAJoinLeaf() function
 * 
 * @return 0 on success, -1 on failure
 */

extern int rpc_mcast_join(rcf_rpc_server *rpcs, int s,
                          const struct sockaddr *mcast_addr, int if_index,
                          tarpc_joining_method how);

/**
 * Leave a multicasting group.
 *
 * Parameters are same as above, except how.
 */
extern int rpc_mcast_leave(rcf_rpc_server *rpcs, int s,
                           const struct sockaddr *mcast_addr, int if_index);

#if HAVE_LINUX_ETHTOOL_H
/**
 * Perform ethtool ioctl
 *
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param ifname    interface name to work with
 * @param edata     ethtool data structure
 *
 * @return ioctl return code
 */
extern int rpc_ioctl_ethtool(rcf_rpc_server *rpcs, int fd, 
                             const char *ifname, void *edata);
#endif /* HAVE_LINUX_ETHTOOL_H */


/**
 * Compare memory areas
 *
 * @param rpcs  RPC server handle
 * @param s1    RPC pointer to the first memory area
 * @param s2    RPC pointer to the second memory area
 * @param n     Size of memory areas to be compared
 *
 * @return      An integer less than, equal to, or greater than zero if the
 *              first n bytes of s1 is found, respectively, to be less 
 *              than, to match, or be greater than the first n bytes of s2.
 * @note See memcmp(3)
 */
extern int rpc_memcmp(rcf_rpc_server *rpcs, 
                      rpc_ptr_off *s1, rpc_ptr_off *s2, size_t n);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_MISC_H__ */
