/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of standard file operations
 * (including poll(), select(), pselect(), fcntl(), ioctl() and
 * sendfile()).
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

#ifndef __TE_TAPI_RPC_UNISTD_H__
#define __TE_TAPI_RPC_UNISTD_H__

/**
 *  Close file descriptor on RPC server side
 *
 *  @param rpcs     RPC server handle
 *  @param fd       file descriptor to close
 *
 *  @return  0 on success or -1 on failure
 */
extern int rpc_close(rcf_rpc_server *rpcs,
                     int fd);

/**
 * Write the buffer to the file starting with the specified file offset.
 *
 * @param rpcs    RPC server handle
 * @param fd      file descriptor
 * @param buf     buffer to write
 * @param buflen  size of buffer
 * @param offset  file offset to write the buffer at
 *
 * @return  Number of bytes actually written or negative value on failure.
 *
 * \retval -2  Failed to reposition the file offset.
 * \retval -1  Failed to write the data.
 * \retval -3  Other errors.
 */
extern ssize_t rpc_write_at_offset(rcf_rpc_server *rpcs, int fd, char* buf,
                                   size_t buflen, off_t offset);

/**
 * Duplicate a file descriptor on RPC server side
 *
 * @note See @b dup manual page for more information
 *
 * @param rpcs    RPC server handle
 * @param oldfd   file descriptor to be duplicated
 *
 * @return New file descriptor or -1 on error
 */
extern int rpc_dup(rcf_rpc_server *rpcs,
                   int oldfd);
/**
 * Duplicate an open file descriptor on RPC server side
 * 
 * @note See @b dup2 manual page for more information
 *
 * @param rpcs    RPC server handle
 * @param oldfd   file descriptor to be duplicated
 * @param newfd   new file descriptor
 *
 * @return  New file descriptor or -1 when an error occured.
 */
extern int rpc_dup2(rcf_rpc_server *rpcs,
                    int oldfd, int newfd);

/**
 * Write up to @b count bytes from buffer @b buf to file with descriptor 
 * @b fd
 * @note See write(2) manual page for more information
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param buf      pointer to a buffer containing data to write
 * @param count    number of bytes to be written
 *
 * @return  Number of bytes actually written, otherwise -1 on failure
 */
extern int rpc_write(rcf_rpc_server *rpcs,
                     int fd, const void *buf, size_t count);

/**
 * This generic routine attemps to read up to @b count bytes from file 
 * with descriptor @b fdinto buffer pointed by @b buf.
 * The behavior of this call depends on the following operations
 * - @b RCF_RPC_CALL do not wait the remote procedure to complete 
 *   (non-blocking call)
 * - @b RCF_RPC_WAIT wait for non-blocking call to complete
 * - @b RCF_RPC_CALL_WAIT wait for remote procedure to complete before
 *   returning (blocking call)
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param buf      pointer to buffer into where data is read
 * @param count    number of bytes to read
 * @param rbuflen  real size of the buffer to be copied by RPC
 *
 * @return  number of bytes read, otherwise -1 on error.
 */
extern int rpc_read_gen(rcf_rpc_server *rpcs,
                        int fd, void *buf, size_t count,
                        size_t rbuflen);

/**
 * Attemp to read up to @b count bytes from file with descriptor @b fd
 * into buffer pointed by @b buf.
 * The behavior of this call depends on the following operations
 * - @b RCF_RPC_CALL do not wait the remote procedure to complete 
 *   (non-blocking call)
 * - @b RCF_RPC_WAIT wait for non-blocking call to complete
 * - @b RCF_RPC_CALL_WAIT wait for remote procedure to complete before
 *   returning (blocking call)
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param buf      pointer to buffer into where data is read
 * @param count    number of bytes to read
 *
 * @return  number of bytes read, otherwise -1 on error.
 */
static inline int
rpc_read(rcf_rpc_server *rpcs,
         int fd, void *buf, size_t count)
{
    return rpc_read_gen(rpcs, fd, buf, count, count);
}


/** Sturcture to store vector of buffers */
typedef struct rpc_iovec {
    void   *iov_base;   /**< starting address of buffer */
    size_t  iov_len;    /**< size of buffer */
    size_t  iov_rlen;   /**< real size of buffer to be copied by RPC */
} rpc_iovec;

/**
 * Compare RPC alalog of 'struct iovec'.
 *
 * @param v1len     total length of the first vector
 * @param v1        the first vector
 * @param v1cnt     number of buffers in vector @b v1
 * @param v2len     total length of the second vector
 * @param v2        the second vector
 * @param v2cnt     number of buffers in vector @b v1
 *
 * @retval 0        vectors are equal
 * @retval -1       vectors are not equal
 */
extern int rpc_iovec_cmp(size_t v1len, const struct rpc_iovec *v1,
                         size_t v1cnt,
                         size_t v2len, const struct rpc_iovec *v2,
                         size_t v2cnt);

/**
 * Attempt to write data to file with descriptor @b fd from the specified
 * vector of buffer.
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param iov      vector of buffers to be written.
 * @param iovcnt   number of buffers
 *
 * @return Number of bytes really written, otherwise -1 is returned 
 *         on error
 * @note See writev manual page for more information
 */
extern int rpc_writev(rcf_rpc_server *rpcs,
                      int fd, const struct rpc_iovec *iov,
                      size_t iovcnt);

/**
 * Attempt to read data from specified file with descriptor @b fd
 * into a vector of buffer @b iov
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptorfrom which data is read
 * @param iov      vector of buffer where data is stored
 * @param iovcnt   number of buffer to read
 * @param riovcnt  real number of buffer
 *
 * @return Number of bytes read,otherwise -1 is returned when an
 *         error occured.
 * @note See @b readv manual page for more information 
 */
extern int rpc_readv_gen(rcf_rpc_server *rpcs,
                         int fd, const struct rpc_iovec *iov,
                         size_t iovcnt, size_t riovcnt);

/**
 * Attempt to read data from specified file with descriptor @b fd 
 * into a vector of buffer @b iov
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptorfrom which data is read
 * @param iov      vector of buffer where data is stored
 * @param iovcnt   number of buffer to read
 *
 * @return Number of bytes read,otherwise -1 is returned when an
 *         error occured.
 * @note See @b readv manual page for more information 
 */
static inline int
rpc_readv(rcf_rpc_server *rpcs,
          int fd, const struct rpc_iovec *iov,
          size_t iovcnt)
{
    return rpc_readv_gen(rpcs, fd, iov, iovcnt, iovcnt);
}

/**
 * Allocate a set of file descriptors on RPC server side
 *
 * @param rpcs   RPC server handle
 *
 * @return set of descriptors,otherwise @b NULL is returned on error
 */
extern rpc_fd_set_p rpc_fd_set_new(rcf_rpc_server *rpcs);

/**
 * Destroy a specified set of file descriptors allocated by 
 * @b rpc_fd_set_new
 *
 * @param rpcs     RPC server handle
 * @param set      set of descriptors to be deleted
 */
extern void rpc_fd_set_delete(rcf_rpc_server *rpcs,
                              rpc_fd_set_p set);

/**
 * Initialize the specified set of descriptors (clear the set)
 *
 * @param rpcs   RPC server handle
 * @param set    set of descriptors to initialize
 */
extern void rpc_do_fd_zero(rcf_rpc_server *rpcs,
                           rpc_fd_set_p set);

/**
 * Add a specified descriptor @b fd to a given set of descriptors @b set
 * on RPC server side
 *
 * @param rpcs  RPC server handle
 * @param fd    descriptor to add to the set
 * @param set   set to which descriptor is added
 */
extern void rpc_do_fd_set(rcf_rpc_server *rpcs,
                          int fd, rpc_fd_set_p set);

/**
 * Remove a specified descriptor from the set of descriptors
 *
 * @param rpcs   RPC server handle
 * @param fd     descriptor to remove
 * @param set    set from which descriptor is removed
 */
extern void rpc_do_fd_clr(rcf_rpc_server *rpcs,
                          int fd, rpc_fd_set_p set);

/**
 * Test existance of specified descriptor @b fd in a given set of 
 * descriptor @b set
 *
 * @param rpcs    RPC server handle
 * @param fd      descriptor to test
 * @param set     set of descriptor
 *
 * @return value other than zero, other return 0 when failed. 
 */
extern int  rpc_do_fd_isset(rcf_rpc_server *rpcs,
                            int fd, rpc_fd_set_p set);

/**
 * Examine the file descriptor sets whose addresses are passed in the 
 * @b readfds, @b writefds, and @b exceptfds parameters to see whether
 * some of there file descriptors are ready for reading, writing or have
 * and exceptional condition pending, respectively.
 * The behavior of this routine depends on  specified RPC operation:
 * @c RCF_RPC_CALL - return immediately without waiting for remote procedure
 *    to complete (non-blocking call).
 * @c RCF_RPC_WAIT - wait for non-blocking call to complete
 * @c RCF_RPC_CALL_WAIT - wait for remote procedure to complete before 
 *    returning (blocking call)

 * @param rpcs      RPC server handle
 * @param n         range of descriptors to be tested. The first @b n 
 *                  descriptors should be checked in each defined set. That
 *                  is the descriptors from 0 to @b n-1 shall be examined.
 * @param readfds   checked file descrpitors for readability
 * @param writefds  checked file descriptors for writing
 * @param exceptfds checked file descriptors for exceptional conditions
 * @param timeout   upper bound of time elapsed before remote @b select
 *                  procedure return.
 * 
 * @return Total number of file descriptors contained in the whole defined
 *         set upon successful completion, which may be zero if the 
 *         timeout expires before anything interesting happens, 
 *         otherwise -1 is returned.
 */
extern int rpc_select(rcf_rpc_server *rpcs,
                      int n, rpc_fd_set_p readfds, rpc_fd_set_p writefds,
                      rpc_fd_set_p exceptfds, struct timeval *timeout);

/**
 * Examine the file descriptor sets whose addresses are passed in the 
 * @b readfds, @b writefds, and @b exceptfds parameters to see whether
 * some of there file descriptors are ready for reading, writing or have
 * and exceptional condition pending, respectively.
 * The current signal mask is replaced by the one pointed by @b sigmask,
 * and restoted after @b select call.
 * The behavior of this routine depends on  specified RPC operation:
 * @c RCF_RPC_CALL - return immediately without waiting for remote procedure
 *    to complete (non-blocking call).
 * @c RCF_RPC_WAIT - wait for non-blocking call to complete
 * @c RCF_RPC_CALL_WAIT - wait for remote procedure to complete before 
 *    returning (blocking call)
 *
 * @param rpcs      RPC server handle
 * @param n         Range of descriptors to be tested. The first @b n 
 *                  descriptors should be checked in each defined set. That
 *                  is the descriptors from 0 to @b n-1 shall be examined.
 * @param readfds   checked file descrpitors for readability
 * @param writefds  checked file descriptors for writing possibility
 * @param exceptfds checked fie descriptors for exceptional conditions
 * @param timeout   Upper bound of time elapsed before remote @b select
 *                  procedure return.
 * @param sigmask   Pointer to a signal mask (See @b sigprocmask(2))
 * 
 * @return Total number of file descriptors contained in the whole defined
 *         set upon successful completion, which may be zero if the 
 *         timeout expires before anything interesting happens, 
 *         otherwise -1 is returned.
 *
 * @note See @b pselect manual page for more information
 */
extern int rpc_pselect(rcf_rpc_server *rpcs,
                       int n, rpc_fd_set_p readfds, rpc_fd_set_p writefds,
                       rpc_fd_set_p exceptfds, struct timespec *timeout,
                       const rpc_sigset_p sigmask);


/** Analog of pollfd structure */
struct rpc_pollfd {
    int            fd;         /**< a file descriptor */
    short events;     /**< requested events FIXME */
    short revents;    /**< returned events FIXME */
};

/**
 * Provide a generic mechanism for reporting I/O conditions associated with
 * a set of file descriptors and waiting for one or more specified 
 * conditions becomes true. Specified condition include the ability to
 * read or write data without blocking and error conditions;
 *
 * @param rpcs      RPC server handle
 * @param ufds      pointer to an array of @b rpc_pollfd structure. One for
 *                  each file descriptor of interest.
 * @param nfds      number of @b rpc_pollfd in the array of @b rpc_pollfd
 *                  structure
 * @param timeout   maximum length of time(in milliseconds) to wait before 
 *                  at least one of the specified condition occures.
 *                  A negative value means an infinite timeout.
 * @param rnfds    real size of the array of @b rpc_pollfd structures
 */
extern int rpc_poll_gen(rcf_rpc_server *rpcs,
                        struct rpc_pollfd *ufds, unsigned int nfds,
                        int timeout, unsigned int rnfds);

/**
 * Provide a mechanism for reporting I/O conditions associated with
 * a set of file descriptors and waiting for one or more specified 
 * conditions becomes true. Specified condition include the ability to
 * read or write data without blocking and error conditions;
 *
 * @param rpcs      RPC server handle
 * @param ufds      pointer to an array of @b rpc_pollfd structure. One for
 *                  each file descriptor of interest.
 * @param nfds      number of @b rpc_pollfd in the array of @b rpc_pollfd
 *                  strudture
 * @param timeout   maximum length of time(in milliseconds) to wait before 
 *                  at least one of the specified condition occures.
 *                  A negative value means an infinite timeout.
 *
 * @return  A positive number is returned upon successful completion. 
 *         That's the number of @b rpc_pollfd structure that have non-zero 
 *         revents fields. A value of zero indicates that the timed out 
 *         and no file descriptors have been selcted. -1 is returned 
 *         on error.
 *
 * @note See @b poll manual page for more information        
 */
static inline int
rpc_poll(rcf_rpc_server *rpcs,
         struct rpc_pollfd *ufds, unsigned int nfds,
         int timeout)
{
    return rpc_poll_gen(rpcs, ufds, nfds, timeout, nfds);
}

/**
 * Routine which copies data from file descriptor opened for reading
 * to the file descriptor opened for writing (processing in kernel land).
 *
 * @param handle        RPC server
 * @param out_fd        file descriptor opened for writing
 * @param in_fd         file descriptor opened for reading
 * @param offset        pointer to input file pointer position
 * @param count         number of bytes to copy between file descriptors
 *
 * @return    number of bytes written to out_fd
 *            or -1 in the case of failure and appropriate errno
 */
extern ssize_t rpc_sendfile(rcf_rpc_server *handle,
                            int out_fd, int in_fd,
                            off_t *offset, size_t count);


/**
 * Manipulate the underlying device parameters of special files. In
 * particular many operating characteristics of character special files
 * may be controlled with @b rpc_ioctl request. The argument @b fd must be
 * an opened file descriptor.
 *
 * @param rpcs     RPC server handle
 * @param fd       opened file descriptor
 * @param request  request code 
 *                 (See tapi_rpcsock_defs.h for availlable request codes)
 * @param ...      optional parameter for request specific information
 *
 * @return zero on success. Some ioctls use the returned value as an output
 *         parameter and return a non negative value on success.
 *         -1 is returned on error.
 * @note See @b ioctl manual page for more information
 */
extern int rpc_ioctl(rcf_rpc_server *rpcs,
                     int fd, rpc_ioctl_code request, ...);

/**
 * Perform various miscellaneous operation on files descriptor @b fd.
 * The operation in question is determined by @b cmd 
 *
 * @param rpcs   RPC server handle
 * @param fd     opened file descriptor
 * @param cmd    command that indicate the requested operation
 *               See @b tapi_rpcsock_defs.h for availlable command.
 * @param arg    indicate flag used according to @b cmd
 *
 * @return The returned value depends on the specified operation
 *         See @b fcntl manual page for more information.
 */
extern int rpc_fcntl(rcf_rpc_server *rpcs, int fd,
                     int cmd, int arg);

/**
 * Create a pair file descriptor on RPC server side, pointing to a pipe 
 * inode and place them in the array @b filedes
 * The file descriptor of index zero (filedes[0]) is used for reading,
 * and the other file descriptor for writing.
 *
 * @param rpcs     RPC server handle
 * @param filedes  array of created file descriptors
 *
 * @return Upon successful completion this function returns 0.
 *         On error -1 is returned.
 */
extern int rpc_pipe(rcf_rpc_server *rpcs, int filedes[2]);

/**
 * Create a unnamed pair of connected socket on RPC server side.
 * the sockets belong to specified domain @b domain, and are of specified
 * type @b type. The descriptor used in referencing the new sockets are
 * returned in sv[0] and sv[1]
 *
 * @param rpcs     RPC server handle
 * @param domain   communication domain
 * @param type     defines semantic of communication
 * @param protocol specifies the protocol to be used.
 * @param sv       array of file descriptors
 *
 * @return 0 is returned upon successful completion, otherwise
 *         -1 is returned.
 * @note See @b socketpair manual page for more information
 */
extern int rpc_socketpair(rcf_rpc_server *rpcs,
                          rpc_socket_domain domain, rpc_socket_type type,
                          rpc_socket_proto protocol, int sv[2]);

/**
 * Get RPC server process identification.
 *
 * @param rpcs  RPC server handle
 *
 * @return The effective user ID of the RPC server process
 */
extern pid_t rpc_getpid(rcf_rpc_server *rpcs);

/**
 * Query the real user ID on the RPC server.
 *
 * @param rpcs RPC server handle
 *
 * @return The real user ID
 */
extern uid_t rpc_getuid(rcf_rpc_server *rpcs);

/**
 * Set the effective user ID of the RPC server process. If the 
 * effective user ID is root, then the real and saved user ID's are also 
 * set. Under Linux @b setuid is implemented like POSIX version with 
 * _POSIX_SAVED_IDS future. This allows a setuid program to drop all of its
 * user privileges, do some un-privileged work, and then re-engage the 
 * original user ID for secure manner
 *
 * @param rpcs     RPC server handle
 * @param uid      user ID
 *
 * @return Upon successful completion returns 0, otherwise -1 is returned.
 * @note   See @b setuid manual page for more information
 */
extern int rpc_setuid(rcf_rpc_server *rpcs,
                      uid_t uid);

/**
 * Query information about the broken out fields of a line from 
 * @b /etc/passwd for the entry that matches the user name @b name on
 * RPC server side
 *
 * @param rpcs    RPC server handle
 * @param name    user name whose information is needed.
 *
 * @return Upon successful completion a pointer to a structure containing
 *         the broken out fields, otherwise a @b NULL pointer is returned.
 */
extern struct passwd *rpc_getpwnam(rcf_rpc_server *rpcs, const char *name);


/**
 * Get the effective user ID of the RPC server process. The effective user
 * ID corresponds to the set ID bit of the file currently executed.
 *
 * @param rpcs  RPC server handle
 *
 * @return The effective user ID of the RPC server process
 */
extern uid_t rpc_geteuid(rcf_rpc_server *rpcs);

/**
 * Set the effective user ID of the RPC server process. Unprivileged user
 * processes can only set the effective user ID to the real user ID, the
 * effective user ID, or the saved user ID.
 *
 * @param rpcs    RPC server handle
 * @param uid     User ID to be set
 *
 * @return Upon successful completion this function returns 0, otherwise
 * -1 is returned.
 */
extern int rpc_seteuid(rcf_rpc_server *rpcs, uid_t uid);

/**
 * Allocate a buffer of specified size in the TA address space
 *
 * @param rpcs    RPC server handle
 * @param size    size of the buffer to be allocated
 *
 * @return   pointer to the allocated buffer upon successful completion
 *           otherwise NULL is retruned.
 */
extern rpc_ptr rpc_malloc(rcf_rpc_server *rpcs, size_t size);

/**
 * Free the specified buffer in TA address space
 *
 * @param rpcs   RPC server handle
 * @param buf    pointer to the buffer to be freed
 */
extern void rpc_free(rcf_rpc_server *rpcs, rpc_ptr buf);

#endif /* !__TE_TAPI_RPC_UNISTD_H__ */
