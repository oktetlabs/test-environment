/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for auxilury remote calls
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * $Id: tapi_rpc_misc.h,v 94b3e14a5ce4 2015/09/24 09:58:33 Andrey $
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
#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_rpc.h"
#include "te_rpc_sys_resource.h"
#include "te_rpc_sys_systeminfo.h"
#include "tapi_rpc_signal.h"
#include "te_dbuf.h"
#include "tq_string.h"
#include "te_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_misc TAPI for miscellaneous remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/** Default read buffer size. */
#define TAPI_READ_BUF_SIZE 4096

/**
 * Try to search a given symbol in the current library used by
 * a given PCO with help of @b tarpc_find_func().
 *
 * @param rpcs        RPC server
 * @param func_name   Symbol to be searched
 *
 * @return            Value returned by @b tarpc_find_func()
 */
extern te_bool rpc_find_func(rcf_rpc_server *rpcs,
                             const char *func_name);

/**
 * Return parent network interface name of vlan interface.
 *
 * @param handle        RPC server handle
 * @param vlan_ifname   VLAN interface name
 * @param parent_ifname Pointer to the parent network interface name
 *                      with at least @c IF_NAMESIZE bytes size.
 *
 * @return 0 on success or -1 in the case of failure
 *
 */
extern int rpc_vlan_get_parent(rcf_rpc_server *rpcs,
                               const char *vlan_ifname,
                               char *parent_ifname);

/**
 * Return slaves network interfaces names of a bond interface.
 *
 * @param rpcs          RPC server handle
 * @param bond_ifname   Bond interface name
 * @param slaves        Where to save list of slaves names
 * @param slaves_num    Where to save number of interfaces
 *                      in @p slaves (may be NULL)
 *
 * @return 0 on success or -1 in case of failure
 */
extern int rpc_bond_get_slaves(rcf_rpc_server *rpcs,
                               const char *bond_ifname,
                               tqh_strings *slaves, int *slaves_num);

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
extern te_bool rpc_protocol_info_cmp(rcf_rpc_server *rpcs,
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
 * Convert 'struct tarpc_timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv     pointer to 'struct tarpc_timespec'
 *
 * @return null-terminated string
 */
extern const char *tarpc_timespec2str(const struct tarpc_timespec *tv);

/**
 * Convert 'struct tarpc_hwtstamp_config' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param hw_cfg     pointer to 'struct tarpc_hwtstamp_config'
 *
 * @return null-terminated string
 */
extern const char *tarpc_hwtstamp_config2str(
                        const tarpc_hwtstamp_config *hw_cfg);

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
 * Patterned data sender.
 *
 * @param rpcs              RPC server
 * @param s                 a socket to be user for sending
 * @param fname             a function used to generate a pattern
 * @param iomux             IO multiplexing function
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
 * @param send_failed       This will be set to @c TRUE if it was send()
 *                          who failed
 *
 * @return Number of sent bytes or -1 in the case of failure
 */
extern int rpc_pattern_sender(rcf_rpc_server *rpcs,
                              int s, char *fname, int iomux, int size_min,
                              int size_max, int size_rnd_once,
                              int delay_min, int delay_max,
                              int delay_rnd_once, int time2run,
                              uint64_t *sent, int ignore_err,
                              te_bool *send_failed);

/**
 * Patterned data receiver.
 *
 * @param rpcs            RPC server
 * @param s               a socket to be user for receiving
 * @param fname           a function used to generate a pattern
 * @param iomux           IO multiplexing function
 * @param received        location for number of received bytes
 * @param recv_failed     This will be set to @c TRUE if it was recv()
 *                        who failed
 *
 * @return number of received bytes, -2 if data doesn't match the pattern,
 *         or -1 in the case of another failure
 */
extern int rpc_pattern_receiver(rcf_rpc_server *rpcs, int s,
                                char *fname, int iomux,
                                uint32_t time2run, uint64_t *received,
                                te_bool *recv_failed);

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
 * @param tx_stat       Tx statistics for set of sender socket to be
 *                      updated (IN/OUT)
 * @param rx_stat       Rx statistics for set of receiver socket to be
 *                      updated (IN/OUT)
 *
 * @return number of sent bytes or -1 in the case of failure
 */
extern int rpc_iomux_flooder(rcf_rpc_server *handle,
                             int *sndrs, int sndnum, int *rcvrs, int rcvnum,
                             int bulkszs, int time2run, int time2wait,
                             int iomux,
                             uint64_t *tx_stat, uint64_t *rx_stat);

/**
 * Send packets during a period of time, call an iomux to check OUT
 * event if send operation is failed.
 *
 * @param rpcs          RPC server handle.
 * @param sock          Socket.
 * @param iomux         Multiplexer function.
 * @param send_func     Transmitting function.
 * @param msg_dontwait  Use flag @b MSG_DONTWAIT.
 * @param packet_size   Payload size to be sent by a single call, bytes.
 * @param duration      How long transmit datagrams, milliseconds.
 * @param packets       Sent packets (datagrams) number.
 * @param errors        @c EAGAIN errors counter.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int rpc_send_flooder_iomux(rcf_rpc_server *rpcs, int sock,
                                  iomux_func iomux,
                                  tarpc_send_function send_func,
                                  te_bool msg_dontwait, int packet_size,
                                  int duration, uint64_t *packets,
                                  uint32_t *errors);

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
 * Routine which calls I/O multiplexing function and then calls splice.
 *
 * @param handle        RPC server
 * @param iomux         type of I/O Multiplexing function
 *                      (@b select(), @b pselect(), @b poll())
 * @param fd_in         file descriptor opened for writing
 * @param fd_out        file descriptor opened for reading
 * @param len           'len' parameter for @b splice() call
 * @param flags         flags for @b splice() call
 * @param time2run      Duration of the call
 *
 * @return number of sent bytes or -1 in the case of failure
 */
extern int rpc_iomux_splice(rcf_rpc_server *rpcs, int iomux, int fd_in,
                            int fd_out, size_t len, int flags,
                            int time2run);

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
 * Copy data between one file descriptor and another. If @p in_fd is a
 * file which supports mmap(2)-like operations then it is recommended to
 * use @p rpc_sendfile cause it is more efficient.
 *
 * @param rpcs          RPC server handle.
 * @param out_fd        File descriptor opened for writing. Can be a socket.
 * @param in_fd         File descriptor opened for reading. Can be a socket.
 * @param timeout       Number of milliseconds that function should block
 *                      waiting for @b in_fd to become ready to read the
 *                      next portion of data while all requested data will
 *                      not be read.
 * @param count         Number of bytes to copy between the file
 *                      descriptors. If @c 0 then all available data should
 *                      be copied, i.e. while @c EOF will not be gotten.
 *
 * @return If the transfer was successful, the number of copied bytes is
 *         returned. On error, @c -1 is returned, and errno is set
 *         appropriately.
 */
extern int64_t rpc_copy_fd2fd(rcf_rpc_server *rpcs,
                              int             out_fd,
                              int             in_fd,
                              int             timeout,
                              uint64_t        count);

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
extern int rpc_overfill_buffers_gen(rcf_rpc_server *rpcs, int sock,
                                    uint64_t *sent, iomux_func iomux);
static inline int
rpc_overfill_buffers(rcf_rpc_server *rpcs, int sock, uint64_t *sent)
{
    return rpc_overfill_buffers_gen(rpcs, sock, sent, FUNC_DEFAULT_IOMUX);
}

/**
 * Drain all data on a fd.
 *
 * @param rpcs      RPC server handle.
 * @param fd        File descriptor or socket.
 * @param size      Read buffer size, bytes.
 * @param time2wait How long wait for data after read failure, milliseconds.
 *                  If a negative value is set, then blocking @c recv() will
 *                  be used to read data.
 * @param read      Pointer for read data amount or @c NULL.
 *
 * @return The last return code of @c recv() function: @c -1 in the case of
 * failure or @c 0 on success. In a common case @c -1 with @c RPC_EAGAIN
 * should be considered as the correct behavior if the connection was not
 * closed from the peer side.
 */
extern int rpc_drain_fd(rcf_rpc_server *rpcs, int fd, size_t size,
                        int time2wait, uint64_t *read);

/**
 * Simplified call of @c rpc_drain_fd(). It executes the call with default
 * @b size and @b time2wait parameters what is useful in a common case. Also
 * it checks return code and errno.
 *
 * @param rpcs      RPC server handle.
 * @param fd        File descriptor or socket.
 * @param read      Pointer for read data amount or @c NULL.
 *
 * @return The last return code of @c recv() function. Actually it can be
 * @c -1 with @c RPC_EAGAIN or @c 0, otherwise the functions reports a
 * verdict and jumps to cleanup. Zero return code indicates that the
 * connection was closed from the peer side.
 */
extern int rpc_drain_fd_simple(rcf_rpc_server *rpcs, int fd,
                               uint64_t *read);

/**
 * Overfill the buffers of the pipe.
 *
 * @param rpcs          RPC server
 * @param write_end     Write end of the pipe
 * @param sent          total bytes written to the pipe
 *
 * @return -1 in the case of failure or 0 on success
 */
extern int rpc_overfill_fd(rcf_rpc_server *rpcs, int write_end,
                           uint64_t *sent);

/**
 * Read all data on an fd and append it to @p dbuf.
 *
 * @param rpcs      RPC server handle.
 * @param fd        File descriptor or socket.
 * @param time2wait Time to wait for data, milliseconds. Negative value means
                    an infinite timeout.
 * @param amount    Number of bytes to read, if @c 0 then only @p time2wait
 *                  limits it.
 * @param dbuf      Buffer to append read data to.
 *
 * @return @c -1 in the case of failure or @c 0 on success (timeout is expired,
 * @p amount bytes is read, EOF is got).
 */
extern int rpc_read_fd2te_dbuf_append(rcf_rpc_server *rpcs, int fd,
                                int time2wait, size_t amount, te_dbuf *dbuf);

/**
 * Read all data from fd to @p dbuf.
 *
 * @note The function resets @p dbuf.
 *
 * @param rpcs      RPC server handle.
 * @param fd        File descriptor or socket.
 * @param time2wait Time to wait for data, milliseconds. Negative value means
                    an infinite timeout.
 * @param amount    Number of bytes to read, if @c 0 then only @p time2wait
 *                  limits it.
 * @param dbuf      Buffer to put read data to.
 *
 * @return @c -1 in the case of failure or @c 0 on success (timeout is expired,
 * @p amount bytes is read, EOF is got).
 */
extern int rpc_read_fd2te_dbuf(rcf_rpc_server *rpcs, int fd, int time2wait,
                               size_t amount, te_dbuf *dbuf);

/**
 * Read all data on an fd.
 *
 * @note @p buf should be freed with free(3) when it is no longer needed,
 * independently on result.
 *
 * @param rpcs      RPC server handle.
 * @param fd        File descriptor or socket.
 * @param time2wait Time to wait for data, milliseconds. Negative value means
                    an infinite timeout.
 * @param amount    Number of bytes to read, if @c 0 then only @p time2wait
 *                  limits it.
 * @param buf       Pointer to buffer (it is allocated by the function).
 * @param read      Number of read bytes.
 *
 * @return @c -1 in the case of failure or @c 0 on success (timeout is expired,
 * @p amount bytes is read, EOF is got).
 */
extern int rpc_read_fd(rcf_rpc_server *rpcs, int fd, int time2wait,
                       size_t amount, void **buf, size_t *read);

/**
 * Read all string data on an fd and append it to @p testr.
 *
 * @param rpcs      RPC server handle.
 * @param fd        File descriptor or socket.
 * @param time2wait Time to wait for data, milliseconds. Negative value means
                    an infinite timeout.
 * @param amount    Number of bytes to read, if @c 0 then only @p time2wait
 *                  limits it.
 * @param testr     Buffer to append read data to.
 *
 * @return @c -1 in the case of failure or @c 0 on success (timeout is expired,
 * @p amount bytes is read, EOF is got).
 */
extern int rpc_read_fd2te_string_append(rcf_rpc_server *rpcs, int fd,
                            int time2wait, size_t amount, te_string *testr);

/**
 * Read all string data from fd to @p testr.
 *
 * @note The function resets @p testr.
 *
 * @param rpcs      RPC server handle.
 * @param fd        File descriptor or socket.
 * @param time2wait Time to wait for data, milliseconds. Negative value means
                    an infinite timeout.
 * @param amount    Number of bytes to read, if @c 0 then only @p time2wait
 *                  limits it.
 * @param testr     Buffer to put read data to.
 *
 * @return @c -1 in the case of failure or @c 0 on success (timeout is expired,
 * @p amount bytes is read, EOF is got).
 */
extern int rpc_read_fd2te_string(rcf_rpc_server *rpcs, int fd, int time2wait,
                                 size_t amount, te_string *testr);

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
 *    @value TARPC_MCAST_ADD_DROP     sockopt IP_ADD/DROP_MEMBERSHIP
 *    @value TARPC_MCAST_JOIN_LEAVE   sockopt MCAST_JOIN/LEAVE_GROUP
 *    @value TARPC_MCAST_WSA          WSAJoinLeaf(), no leave
 * 
 * @return 0 on success, -1 on failure
 */

extern int rpc_mcast_join(rcf_rpc_server *rpcs, int s,
                          const struct sockaddr *mcast_addr, int if_index,
                          tarpc_joining_method how);

/**
 * Leave a multicasting group.
 *
 * Parameters are same as above.
 */
extern int rpc_mcast_leave(rcf_rpc_server *rpcs, int s,
                           const struct sockaddr *mcast_addr, int if_index,
                           tarpc_joining_method how);

/**
 * Join a multicasting group with source on specified interface.
 * 
 * @param  rpcs        RPC server handle
 * @param  s           socket descriptor
 * @param  mcast_addr  multicast address
 * @param  source_addr source address
 * @param  if_index    interface index
 * @param  how         joining method:
 *
 * @value TARPC_MCAST_SOURCE_ADD_DROP   
 *          sockopt IP_ADD/DROP_SOURCE_MEMBERSHIP
 * @value TARPC_MCAST_SOURCE_JOIN_LEAVE 
 *          sockopt MCAST_JOIN/LEAVE_SOURCE_GROUP
 * 
 * @return 0 on success, -1 on failure
 */
extern int rpc_mcast_source_join(rcf_rpc_server *rpcs, int s,
                                 const struct sockaddr *mcast_addr,
                                 const struct sockaddr *source_addr,
                                 int if_index,
                                 tarpc_joining_method how);

/**
 * Leave a multicasting group with source.
 *
 * Parameters are same as above.
 */
extern int rpc_mcast_source_leave(rcf_rpc_server *rpcs, int s,
                                  const struct sockaddr *mcast_addr,
                                  const struct sockaddr *source_addr,
                                  int if_index,
                                  tarpc_joining_method how);

extern int rpc_common_mcast_join(rcf_rpc_server *rpcs, int s,
                                 const struct sockaddr *mcast_addr,
                                 const struct sockaddr *source_addr,
                                 int if_index, tarpc_joining_method how);

extern int rpc_common_mcast_leave(rcf_rpc_server *rpcs, int s,
                                  const struct sockaddr *mcast_addr,
                                  const struct sockaddr *source_addr,
                                  int if_index, tarpc_joining_method how);

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

/**
 * Initialize iomux_state with zero value
 *
 * @param rpcs      RPC server handle
 * @param iomux     Multiplexer function type
 * @param iomux_st  The multiplexer context pointer
 *
 * @note Caller must free memory allocated for iomux_st using rpc_iomux_close()
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int rpc_iomux_create_state(rcf_rpc_server *rpcs, iomux_func iomux,
                                  tarpc_iomux_state *iomux_st);

/**
 * Call I/O multiplexing wait function multiple times
 *
 * @param rpcs      RPC server handle
 * @param fd        File descriptor
 * @param iomux     Iomux to be called
 * @param iomux_st  The multiplexer context pointer
 * @param events    @b poll() events to be checked for
 * @param count     How many times to call a function or @c -1 for unlimited
 * @param duration  Call iomux during a specified time in milliseconds
 *                  or @c -1
 * @param exp_rc    Expected return value
 * @param number    If not @c NULL, will be set to the number
 *                  of iomux calls before timeout occured or
 *                  an error was returned.
 * @param last_rc   If not @c NULL, will be set to the last
 *                  return value of an iomux function.
 * @param zero_rc   If not @c NULL, number of zero code returned by iomux
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int rpc_multiple_iomux_wait(rcf_rpc_server *rpcs, int fd,
                                   iomux_func iomux,
                                   tarpc_iomux_state iomux_st,int events,
                                   int count, int duration, int exp_rc,
                                   int *number, int *last_rc, int *zero_rc);

/**
 * Close iomux state when necessary
 *
 * @param rpcs      RPC server handle
 * @param iomux     Multiplexer function type
 * @param iomux_st  The multiplexer context pointer
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
extern int rpc_iomux_close_state(rcf_rpc_server *rpcs, iomux_func iomux,
                                 tarpc_iomux_state iomux_st);

/**
 * Call I/O multiplexing function multiple times.
 *
 * @param rpcs      RPC server handle
 * @param fd        File descriptor
 * @param iomux     Iomux to be called
 * @param events    @b poll() events to be checked for
 * @param count     How many times to call a function or @c -1 for unlimited
 * @param duration  Call iomux during a specified time in milliseconds
 *                  or @c -1
 * @param exp_rc    Expected return value
 * @param number    If not @c NULL, will be set to the number
 *                  of iomux calls before timeout occured or
 *                  an error was returned.
 * @param last_rc   If not @c NULL, will be set to the last
 *                  return value of an iomux function.
 * @param zero_rc   If not @c NULL, number of zero code returned by iomux
 */
extern int rpc_multiple_iomux(rcf_rpc_server *rpcs, int fd,
                              iomux_func iomux, int events, int count,
                              int duration, int exp_rc, int *number,
                              int *last_rc, int *zero_rc);

/**
 * Convert raw data to integer (this is useful when raw data was
 * obtained from a test machine with different endianness).
 *
 * @param rpcs  RPC server
 * @param data  Data to be converted
 * @param len   Length of data to be converted
 *
 * @return @c 0 on success or @c -1
 */
extern int rpc_raw2integer(rcf_rpc_server *rpcs, uint8_t *data,
                           size_t len);

/**
 * Convert integer value to raw representation (this is useful
 * when raw data is to be processed by a test machine with different
 * endianness).
 *
 * @param rpcs      RPC server
 * @param number    Number to be converted
 * @param data      Buffer where to place converted value
 * @param len       Integer type size
 *
 * @return @c 0 on success or @c -1
 */
extern int rpc_integer2raw(rcf_rpc_server *rpcs, uint64_t number,
                           uint8_t *data, size_t len);

/**
 * Specioal function for sockapi-ts/basic/vfork_check_hang test.
 *
 * @param rpcs      RPC server
 * @param use_exec  Use @b execve() or @b _exit() function
 *
 * @return @c 0 on success or @c -1
 */
extern int rpc_vfork_pipe_exec(rcf_rpc_server *rpcs, te_bool use_exec);

/**
 * Determine if the interface is grabbed by the testing.
 *
 * @param ta         Test agent name
 * @param interface  Interface name
 *
 * @return @c TRUE if the interface is grabbed.
 */
static inline te_bool
tapi_interface_is_mine(const char *ta, const char *interface)
{
    cfg_val_type  val_type = CVT_STRING;
    char         *val;

    if (cfg_get_instance_fmt(&val_type, &val, "/agent:%s/rsrc:%s", ta,
                            interface) == 0)
    {
        free(val);
        return TRUE;
    }

    return FALSE;
}

/**
 * Set new MTU value for a given interface (increasing MTU for
 * the interfaces it is based on if necessary).
 *
 * @note This function does not save previous values of MTU for affected
 *       "ancestor" interfaces; you should rely on Configurator to restore
 *       them. Still it makes sense to restore MTU for the interface
 *       passed to this function, as otherwise Configurator may
 *       be not able to restore MTU values for its "descendant"
 *       interfaces, which may change as a side effect if we descrease
 *       MTU.
 *
 * @param ta         Test agent name
 * @param interface  Network interface
 * @param mtu        MTU value
 * @param old_mtu    If not @c NULL, previous value of MTU
 *                   for the interface will be saved.
 *
 * @return Status code.
 */
extern te_errno tapi_set_if_mtu_smart(const char *ta,
                                      const struct if_nameindex *interface,
                                      int mtu, int *old_mtu);

/** Structure for storing MTU values. */
typedef struct te_saved_mtu {
    LIST_ENTRY(te_saved_mtu)    links;              /**< List links. */
    char                        ta[RCF_MAX_NAME];   /**< Test agent name. */
    char                        if_name[IFNAMSIZ];  /**< Interface name. */
    int                         mtu;                /**< MTU value. */
} te_saved_mtu;

/** Type of list of MTU values. */
typedef LIST_HEAD(te_saved_mtus, te_saved_mtu) te_saved_mtus;

/**
 * Free memory allocated for items of a list of MTU values.
 *
 * @param mtus    Pointer to the list.
 */
extern void tapi_saved_mtus_free(te_saved_mtus *mtus);

/**
 * Convert list of saved MTU values to string.
 *
 * @param mtus        List of saved MTU values.
 * @param str         Where to save pointer to the string
 *                    (memory is allocated by this function).
 *
 * @return Status code.
 */
extern te_errno tapi_saved_mtus2str(te_saved_mtus *mtus, char **str);

/**
 * Convert string to list of saved MTU values.
 *
 * @param str       String to convert.
 * @param mtus      Pointer to list head.
 *
 * @return Status code.
 */
extern te_errno tapi_str2saved_mtus(const char *str, te_saved_mtus *mtus);

/**
 * Save MTU values to a temporary local file.
 *
 * @note List of MTU values will be empty after calling this function.
 *       Register "saved_mtus" node under "/local" in Configuration tree
 *       to use this function.
 *
 * @param ta        Test Agent name.
 * @param name      Unique name.
 * @param mtus      List of MTU values.
 *
 * @return Status code.
 */
extern te_errno tapi_store_saved_mtus(const char *ta,
                                      const char *name,
                                      te_saved_mtus *mtus);

/**
 * Check whether MTU values are already stored under a given name.
 *
 * @param ta      Test Agent name.
 * @param name    Name used to identify MTU values list.
 *
 * @return @c TRUE if MTU values are stored under the @p name,
 *         @c FALSE otherwise.
 */
extern te_bool tapi_stored_mtus_exist(const char *ta,
                                      const char *name);

/**
 * Retrieve saved MTU values from a temporary local file.
 *
 * @note This function may be called only once for a given name,
 *       as the file fill be removed as a result.
 *
 * @param ta        Test Agent name.
 * @param name      Unique name previously passed to tapi_store_save_mtus().
 * @param mtus      MTU values.
 *
 * @return Status code.
 */
extern te_errno tapi_retrieve_saved_mtus(const char *ta,
                                         const char *name,
                                         te_saved_mtus *mtus);

/**
 * Set new MTU value for a given interface (increasing MTU for
 * the interfaces it is based on if necessary).
 *
 * @note The same backup argument may be passed to several
 *       calls of this function; in that case all changes made
 *       by them can be reverted with a single call of
 *       tapi_set_if_mtu_smart2_rollback().
 *
 * @param ta         Test agent name
 * @param if_name    Network interface name
 * @param mtu        MTU value
 * @param backup     If not @c NULL, original values of MTU for all
 *                   affected interfaces will be saved here.
 *
 * @return Status code.
 */
extern te_errno tapi_set_if_mtu_smart2(const char *ta,
                                       const char *if_name,
                                       int mtu,
                                       te_saved_mtus *backup);

/**
 * Revert changes made by tapi_set_if_mtu_smart2().
 *
 * @param backup      Where the original MTU values are saved.
 *
 * @return Status code.
 */
extern te_errno tapi_set_if_mtu_smart2_rollback(te_saved_mtus *backup);

/**
 * Check if the interface is VLAN interface.
 * 
 * @param rpcs       RPC server handler
 * @param interface  Interface name
 * 
 * @return @c TRUE if the interface is grabbed.
 */
extern te_bool tapi_interface_is_vlan(rcf_rpc_server *rpcs,
                                      const struct if_nameindex *interface);

/**
 * Compute number of VLAN interfaces on which the interface is based
 * (including the interface itself). This should be the number of VLAN
 * tags in Ethernet frames going via this interface.
 *
 * @param ta        Test agent name
 * @param if_name   Interface name
 * @param num       Number of VLAN "ancestor" interfaces, including this
 *                  interface itself if it is VLAN
 *
 * @return Status code.
 */
extern te_errno tapi_interface_vlan_count(const char *ta, const char *if_name,
                                          size_t *num);

/**
 * Release the RPC pointer with specified namespace without any system call
 *
 * @param rpcs          RPC server handle
 * @param ptr           RPC pointer
 * @param ns_string     Namespace as string for @p ptr
 */
extern void rpc_release_rpc_ptr(
        rcf_rpc_server *rpcs, rpc_ptr ptr, char *ns_string);

/**@} <!-- END te_lib_rpc_misc --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_MISC_H__ */
