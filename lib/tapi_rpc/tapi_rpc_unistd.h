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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_UNISTD_H__
#define __TE_TAPI_RPC_UNISTD_H__

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "rcf_rpc.h"
#include "te_rpc_fcntl.h"

#include "te_rpc_types.h"

#include "tarpc.h"

#include "tapi_jmp.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_unistd TAPI for some file operations calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Open a file or device.
 *
 * @param rpcs      RPC server handle
 * @param path      File or device name
 * @param flags     How to open
 * @param mode      The permissions to use in case a new file is created
 * 
 * @return File descriptor on success or -1 on failure
 */
extern int rpc_open(rcf_rpc_server         *rpcs,
                    const char             *path,
                    rpc_fcntl_flags         flags,
                    rpc_file_mode_flags     mode);

/**
 * Open a large file or device.
 *
 * @param rpcs      RPC server handle
 * @param path      File or device name
 * @param flags     How to open
 * @param mode      The permissions to use in case a new file is created
 * 
 * @return File descriptor on success or -1 on failure
 */
extern int rpc_open64(rcf_rpc_server         *rpcs,
                      const char             *path,
                      rpc_fcntl_flags         flags,
                      rpc_file_mode_flags     mode);

/**
 *  Close file descriptor on RPC server side.
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
 * @retval -2  Failed to reposition the file offset.
 * @retval -1  Failed to write the data.
 * @retval -3  Other errors.
 */
extern ssize_t rpc_write_at_offset(rcf_rpc_server *rpcs, int fd, char* buf,
                                   size_t buflen, off_t offset);

/**
 * Duplicate a file descriptor on RPC server side.
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
 * Duplicate an open file descriptor on RPC server side.
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
 * Duplicate an open file descriptor on RPC server side.
 * 
 * @note See @b dup3 manual page for more information
 *
 * @param rpcs    RPC server handle
 * @param oldfd   file descriptor to be duplicated
 * @param newfd   new file descriptor
 * @param flags   RPC_O_CLOEXEC flag
 *
 * @return  New file descriptor or -1 when an error occured.
 */
extern int rpc_dup3(rcf_rpc_server *rpcs,
                    int oldfd, int newfd, int flags);

/**
 * Write up to @b count bytes from buffer @b buf to file with descriptor 
 * @b fd.
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
 * Write up to @b count bytes from buffer @b buf to file with descriptor 
 * @b fd and close this file descriptor.
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param buf      pointer to a buffer containing data to write
 * @param count    number of bytes to be written
 *
 * @return  Number of bytes actually written, otherwise -1 on failure
 */
extern int rpc_write_and_close(rcf_rpc_server *rpcs,
                               int fd, const void *buf, size_t count);

/**
 * Like rpc_write(), but uses a buffer that has been allocated
 * by the user earlier.
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param buf      pointer to the data buffer in the TA address space
 * @param count    number of bytes to be written
 *
 * @return  Number of bytes actually written, otherwise -1 on failure
 */
extern tarpc_ssize_t rpc_writebuf_gen(rcf_rpc_server *rpcs, int fd,
                                      rpc_ptr buf, size_t buf_off,
                                      size_t count);
static inline tarpc_ssize_t
rpc_writebuf(rcf_rpc_server *rpcs, int fd, rpc_ptr buf, size_t count)
{
    return rpc_writebuf_gen(rpcs, fd, buf, 0, count);
}
static inline tarpc_ssize_t
rpc_writebuf_off(rcf_rpc_server *rpcs, int fd, 
                 rpc_ptr_off *buf, size_t count)
{
    return rpc_writebuf_gen(rpcs, fd, buf->base, buf->offset, count);
}

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
 * @return  Number of bytes read, otherwise -1 on error.
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


/**
 * Like rpc_read() but uses a buffer allocated by the user 
 * on the TA earlier.
 * 
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param buf      pointer to buffer into where data is read
 * @param count    number of bytes to read
 *
 * @return  number of bytes read, otherwise -1 on error.
 */
extern tarpc_ssize_t rpc_readbuf_gen(rcf_rpc_server *rpcs, int fd,
                                     rpc_ptr buf, size_t buf_off,
                                     size_t count);
static inline tarpc_ssize_t
rpc_readbuf(rcf_rpc_server *rpcs, int fd, rpc_ptr buf, size_t count)
{
    return rpc_readbuf_gen(rpcs, fd, buf, 0, count);
}
static inline tarpc_ssize_t
rpc_readbuf_off(rcf_rpc_server *rpcs, int fd, rpc_ptr_off *buf, 
                size_t count)
{
    return rpc_readbuf_gen(rpcs, fd, buf->base, buf->offset, count);
}

/**
 * RPC equivalent of 'lseek'
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param pos      position to seek
 * @param count    seek mode (SEEK_SET, SEEK_CUR, SEEK_END)
 *
 * @return Resuling file position or -1 on error
 */
extern tarpc_off_t rpc_lseek(rcf_rpc_server *rpcs,
                             int fd, tarpc_off_t pos, rpc_lseek_mode mode);

/**
 * RPC equivalent of 'fsync'
 *
 * @param rpcs    RPC server handle
 * @param fd      file descriptor
 *
 * @return 0 on success, -1 on failure
 */
extern int rpc_fsync(rcf_rpc_server *rpcs, int fd);


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
 * @return 0 on success or -1 on failure
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
 * into a vector of buffer @b iov.
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
 * into a vector of buffer @b iov.
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
 * Allocate a set of file descriptors on RPC server side.
 *
 * @param rpcs   RPC server handle
 *
 * @return Set of descriptors,otherwise @b NULL is returned on error
 */
extern rpc_fd_set_p rpc_fd_set_new(rcf_rpc_server *rpcs);

/**
 * Destroy a specified set of file descriptors allocated by 
 * @b rpc_fd_set_new.
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
 * on RPC server side.
 *
 * @param rpcs  RPC server handle
 * @param fd    descriptor to add to the set
 * @param set   set to which descriptor is added
 */
extern void rpc_do_fd_set(rcf_rpc_server *rpcs,
                          int fd, rpc_fd_set_p set);

/**
 * Remove a specified descriptor from the set of descriptors.
 *
 * @param rpcs   RPC server handle
 * @param fd     descriptor to remove
 * @param set    set from which descriptor is removed
 */
extern void rpc_do_fd_clr(rcf_rpc_server *rpcs,
                          int fd, rpc_fd_set_p set);

/**
 * Test existance of specified descriptor @b fd in a given set of 
 * descriptor @b set.
 *
 * @param rpcs    RPC server handle
 * @param fd      descriptor to test
 * @param set     set of descriptor
 *
 * @return Value other than zero, other return 0 when failed. 
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
                      rpc_fd_set_p exceptfds,
                      struct tarpc_timeval *timeout);

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
                       rpc_fd_set_p exceptfds, 
                       struct tarpc_timespec *timeout,
                       const rpc_sigset_p sigmask);


/** Analog of pollfd structure */
struct rpc_pollfd {
    int            fd;         /**< a file descriptor */
    short events;     /**< requested events FIXME */
    short revents;    /**< returned events FIXME */
};

/* TODO: Add description */
typedef union rpc_epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} rpc_epoll_data_t;


struct rpc_epoll_event {
    uint32_t         events;      /* Epoll events */
    rpc_epoll_data_t data;      /* User data variable */
};

struct rpc_f_owner_ex {
    int   type;
    pid_t pid;
};

extern int rpc_epoll_create(rcf_rpc_server *rpcs, int size);
extern int rpc_epoll_create1(rcf_rpc_server *rpcs, int flags);
extern int rpc_epoll_ctl(rcf_rpc_server *rpcs, int epfd, int oper, int fd,
                         struct rpc_epoll_event *event);
static inline int
rpc_epoll_ctl_simple(rcf_rpc_server *rpcs, int epfd, int oper, int fd,
                     uint32_t events)
{
    struct rpc_epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return rpc_epoll_ctl(rpcs, epfd, oper, fd, &event);
}

extern int rpc_epoll_wait_gen(rcf_rpc_server *rpcs, int epfd,
                              struct rpc_epoll_event *events, int rmaxev,
                              int maxevents, int timeout);
static inline int
rpc_epoll_wait(rcf_rpc_server *rpcs, int epfd,
               struct rpc_epoll_event *events, int maxevents,
               int timeout)
{
    return rpc_epoll_wait_gen(rpcs, epfd, events, maxevents, maxevents,
                              timeout);
}
extern int rpc_epoll_pwait_gen(rcf_rpc_server *rpcs, int epfd,
                               struct rpc_epoll_event *events, int rmaxev,
                               int maxevents, int timeout,
                               const rpc_sigset_p sigmask);
static inline int
rpc_epoll_pwait(rcf_rpc_server *rpcs, int epfd,
                struct rpc_epoll_event *events, int maxevents,
                int timeout, const rpc_sigset_p sigmask)
{
    return rpc_epoll_pwait_gen(rpcs, epfd, events, maxevents, maxevents,
                               timeout, sigmask);
}

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
 *
 * @return  A positive number is returned upon successful completion. 
 *         That's the number of @b rpc_pollfd structure that have non-zero 
 *         revents fields. A value of zero indicates that the timed out 
 *         and no file descriptors have been selcted. -1 is returned 
 *         on error.
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

extern int rpc_ppoll_gen(rcf_rpc_server *rpcs,
                         struct rpc_pollfd *ufds, unsigned int nfds,
                         struct tarpc_timespec *timeout,
                         const rpc_sigset_p sigmask, unsigned int rnfds);
static inline int
rpc_ppoll(rcf_rpc_server *rpcs,
          struct rpc_pollfd *ufds, unsigned int nfds,
          struct tarpc_timespec *timeout, const rpc_sigset_p sigmask)
{
    return rpc_ppoll_gen(rpcs, ufds, nfds, timeout, sigmask, nfds);
}

/**
 * Routine which copies data from file descriptor opened for reading
 * to the file descriptor opened for writing (processing in kernel land).
 *
 * @param rpcs          RPC server
 * @param out_fd        file descriptor opened for writing
 * @param in_fd         file descriptor opened for reading
 * @param offset        pointer to input file pointer position
 * @param count         number of bytes to copy between file descriptors
 * @param force64       boolean value, whether to force sendfile64 call
 *
 * @return    Number of bytes written to out_fd
 *            or -1 in the case of failure and appropriate errno
 */
extern ssize_t rpc_sendfile(rcf_rpc_server *rpcs, int out_fd, int in_fd,
                            tarpc_off_t *offset, size_t count,
                            tarpc_bool force64);


/**
 * Manipulate the underlying device parameters of special files. In
 * particular many operating characteristics of character special files
 * may be controlled with @b rpc_ioctl request. The argument @b fd must be
 * an opened file descriptor.
 *
 * @param rpcs     RPC server handle
 * @param fd       opened file descriptor
 * @param request  request code 
 *                 (See te_rpc_sys_socket.h for availlable request codes)
 * @param ...      optional parameter for request specific information
 *
 * @return Zero on success. Some ioctls use the returned value as an output
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
 *               See @b te_rpc_fcntl.h for availlable command.
 * @param arg    indicate flag used according to @b cmd
 *
 * @return The returned value depends on the specified operation
 *         See @b fcntl manual page for more information.
 */
extern int rpc_fcntl(rcf_rpc_server *rpcs,
                     int fd, int cmd, ...);

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
extern int rpc_pipe(rcf_rpc_server *rpcs,
                    int filedes[2]);

/**
 * Create a pair file descriptor on RPC server side, pointing to a pipe 
 * inode and place them in the array @b filedes
 * The file descriptor of index zero (filedes[0]) is used for reading,
 * and the other file descriptor for writing.
 *
 * @param rpcs     RPC server handle
 * @param filedes  array of created file descriptors
 * @param flags    RPC_O_NONBLOCK, RPC_O_CLOEXEC flags
 *
 * @return Upon successful completion this function returns 0.
 *         On error -1 is returned.
 */
extern int rpc_pipe2(rcf_rpc_server *rpcs,
                     int filedes[2],
                     int flags);

/**
 * Create a unnamed pair of connected socket on RPC server side.
 * the sockets belong to specified domain @b domain, and are of specified
 * type @b type. The descriptor used in referencing the new sockets are
 * returned in sv[0] and sv[1].
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
 * Terminate RPC server process via exit() call.
 *
 * @param rpcs      RPC server handle
 * @param status    Status code
 */
extern void rpc_exit(rcf_rpc_server *rpcs, int status);

/**
 * Get RPC server process identification.
 *
 * @param rpcs  RPC server handle
 *
 * @return The ID of the RPC server process
 */
extern pid_t rpc_getpid(rcf_rpc_server *rpcs);

/**
 * Get RPC server thread identification.
 *
 * @param rpcs  RPC server handle
 *
 * @return The thread ID of the RPC server thread
 */
extern tarpc_pthread_t rpc_pthread_self(rcf_rpc_server *rpcs);

/**
 * Get thread ID. This is linux-specific system call.
 *
 * @param rpcs      RPC server handle
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_gettid(rcf_rpc_server *rpcs);

/**
 * Query the real user ID on the RPC server.
 *
 * @param rpcs RPC server handle
 *
 * @return The real user ID
 */
extern tarpc_uid_t rpc_getuid(rcf_rpc_server *rpcs);

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
                      tarpc_uid_t uid);


/**
 * Checks for access permissions @p mode of @p path at @p rpcs
 *
 * @param rpcs RPC server handle
 * @param path Pathname to check
 * @param mode Access mode (see lib/rpc_types/te_rpc_sys_stat.h)
 *
 * @return 0 if @p path is available as specific @p mode, -1 otherwise
 */
extern int rpc_access(rcf_rpc_server *rpcs,
                      const char *path,
                      int mode);

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
extern struct passwd *rpc_getpwnam(rcf_rpc_server *rpcs,
                                   const char *name);


struct utsname;

/**
 * Query information about the host on RPC server site
 *
 * @param rpcs    RPC server handle
 * @param buf     Buffer to hold host info (OUT)
 *
 * @return 0 in case of success, -1 in case of error
 */
extern int rpc_uname(rcf_rpc_server *rpcs, struct utsname *buf);


/**
 * Get the effective user ID of the RPC server process. The effective user
 * ID corresponds to the set ID bit of the file currently executed.
 *
 * @param rpcs  RPC server handle
 *
 * @return The effective user ID of the RPC server process
 */
extern tarpc_uid_t rpc_geteuid(rcf_rpc_server *rpcs);

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
extern int rpc_seteuid(rcf_rpc_server *rpcs, tarpc_uid_t uid);

/**
 * Allocate a buffer of specified size in the TA address space.
 *
 * @param rpcs    RPC server handle
 * @param size    size of the buffer to be allocated
 *
 * @return  Allocated buffer identifier or RPC_NULL.
 */
extern rpc_ptr rpc_malloc(rcf_rpc_server *rpcs, size_t size);

/**
 * Free the specified buffer in TA address space.
 *
 * @param rpcs   RPC server handle
 * @param buf    identifier of the buffer to be freed
 */
extern void rpc_free(rcf_rpc_server *rpcs, rpc_ptr buf);

/**
 * Get address in the TA address space by its ID.
 *
 * @param rpcs    RPC server handle
 * @param id      Address ID
 *
 * @return  Value of address in the TA address space 
 */
extern uint64_t rpc_get_addr_by_id(rcf_rpc_server *rpcs, rpc_ptr id);

/**
 * Allocate a buffer of specified size in the TA address space.
 *
 * @param rpcs    RPC server handle
 * @param size    size of the buffer to be allocated
 *
 * @return  Allocated buffer identifier with offset. 
 *          It should be validated by RPC_PTR_OFF_IS_VALID()
 */
static inline te_bool
rpc_malloc_off(rcf_rpc_server *rpcs, size_t size, rpc_ptr_off **buf)
{
    rpc_ptr_off *ret = calloc(sizeof(rpc_ptr_off), 1);

    ret->base = rpc_malloc(rpcs, size);
    if (ret->base == RPC_NULL)
        return FALSE;
    ret->offset = 0;
    *buf = ret;
    return TRUE;
}

/**
 * Free the specified buffer in TA address space.
 *
 * @param rpcs   RPC server handle
 * @param buf    Identifier of the buffer to be freed with offset. Offset
 *               should be zero.
 */
static inline void 
rpc_free_off(rcf_rpc_server *rpcs, rpc_ptr_off *buf)
{
    if (buf->offset != 0)
    {
        ERROR("Attempt to free buffer %u with non-zero offset %u on %s", 
              buf->base, buf->offset, rpcs->ta);
        rpcs->iut_err_jump = TRUE;
        TAPI_JMP_DO(TE_EFAIL);
        return;
    }
    rpc_free(rpcs, buf->base);
    free(buf);
}

/**
 * Allocate a buffer of specified size in the TA address space
 * aligned at a specified boundary
 *
 * @param rpcs          RPC server handle
 * @param alignment     alignment of the buffer
 * @param size          size of the buffer to be allocated
 *
 * @return   Allocated buffer identifier or RPC_NULL
 */
extern rpc_ptr rpc_memalign(rcf_rpc_server *rpcs,
                            size_t alignment,
                            size_t size);


/**
 * Free memory allocated on RPC server in cleanup part of the test. 
 *
 * @param _rpcs     RPC server handle
 * @param _ptr      memory identifier
 */
#define CLEANUP_RPC_FREE(_rpcs, _ptr) \
    do {                                                \
        if ((_ptr != RPC_NULL) >= 0 && (_rpcs) != NULL) \
        {                                               \
            rpc_free(_rpcs, _ptr);                      \
            if (!RPC_IS_CALL_OK(_rpcs))                 \
                MACRO_TEST_ERROR;                       \
            _ptr = RPC_NULL;                            \
        }                                               \
    } while (0)

extern int rpc_gettimeofday(rcf_rpc_server *rpcs,
                            tarpc_timeval *tv, tarpc_timezone *tz);

/**
 * Set resource limits and usage
 *
 * @param rpcs          RPC server
 * @param resource      Resource type (see 'man 2 setrlimit').
 * @param rlim          RPC analog of rlimit structure
 *                      (see 'man 2 setrlimit').
 *
 * @return    -1 in the case of failure or 0 on success
 */
extern int rpc_setrlimit(rcf_rpc_server *rpcs,
                         int resource, const tarpc_rlimit *rlim);

/**
 * Get resource limits and usage
 *
 * @param rpcs          RPC server
 * @param resource      Resource type (see 'man 2 getrlimit').
 * @param rlim          RPC analog of rlimit structure
 *                      (see 'man 2 getrlimit').
 *
 * @return    -1 in the case of failure or 0 on success
 */
extern int rpc_getrlimit(rcf_rpc_server *rpcs,
                         int resource, tarpc_rlimit *rlim);

/**
 * RPC sysconf() call.
 *
 * @param rpcs          RPC server
 * @param name          Name of sysconf() constant
 *
 * @return    -1 in the case of failure or a value of system
 *            resource
 */
extern int64_t rpc_sysconf(rcf_rpc_server *rpcs,
                           rpc_sysconf_name name);

/**
 * Get file status.
 *
 * @param rpcs          RPC server
 * @param fd            FD to stat
 * @param buf           Buffer for file status.
 *
 * @return     -1 in the case of failure or 0 on success
 */
extern int rpc_fstat(rcf_rpc_server *rpcs,
                     int fd,
                     rpc_stat *buf);
extern int rpc_fstat64(rcf_rpc_server *rpcs,
                       int fd,
                       rpc_stat *buf);

/**
 * Get hostname
 *
 * @param rpcs      RPC server
 * @param name      Hostname
 * @param len       Length of buffer in name argument
 *
 * @return  -1 in the case of failure or 0 on success
 */
extern int rpc_gethostname(rcf_rpc_server *rpcs, char *name, size_t len);

/**
 * Change root directory.
 *
 * @param rpcs      RPC server
 * @param path      Path to a new root directory
 *
 * @return  -1 in the case of failure or 0 on success
 */
extern int rpc_chroot(rcf_rpc_server *rpcs, char *path);

/**
 * Copy dynamic libraries necessary to run TA to its
 * directory (can be used to perform exec() after chroot()).
 *
 * @param rpcs      RPC server
 * @param path      Path to a new root directory
 *
 * @return  -1 in the case of failure or 0 on success
 */
extern int rpc_copy_ta_libs(rcf_rpc_server *rpcs, char *path);

/**
 * Removie lib/ folder from TA folder (used for cleanup
 * after rpc_copy_ta_libs()).
 *
 * @param rpcs      RPC server
 * @param path      Path to a new root directory
 *
 * @return  -1 in the case of failure or 0 on success
 */
extern int rpc_rm_ta_libs(rcf_rpc_server *rpcs, char *path);

/**@} <!-- END te_lib_rpc_unistd --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_UNISTD_H__ */
