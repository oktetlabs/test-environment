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


extern int rpc_close(rcf_rpc_server *rpcs,
                     int fd);

extern int rpc_dup(rcf_rpc_server *rpcs,
                   int oldfd);
extern int rpc_dup2(rcf_rpc_server *rpcs,
                    int oldfd, int newfd);

extern int rpc_write(rcf_rpc_server *rpcs,
                     int fd, const void *buf, size_t count);

extern int rpc_read_gen(rcf_rpc_server *rpcs,
                        int fd, void *buf, size_t count,
                        size_t rbuflen);

static inline int
rpc_read(rcf_rpc_server *rpcs,
         int fd, void *buf, size_t count)
{
    return rpc_read_gen(rpcs, fd, buf, count, count);
}


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
 * @param v2len     total length of the second vector
 * @param v2        the second vector
 *
 * @retval 0        vectors are equal
 * @retval -1       vectors are not equal
 */
extern int rpc_iovec_cmp(size_t v1len, const struct rpc_iovec *v1,
                         size_t v1cnt,
                         size_t v2len, const struct rpc_iovec *v2,
                         size_t v2cnt);

extern int rpc_writev(rcf_rpc_server *rpcs,
                      int fd, const struct rpc_iovec *iov,
                      size_t iovcnt);

extern int rpc_readv_gen(rcf_rpc_server *rpcs,
                         int fd, const struct rpc_iovec *iov,
                         size_t iovcnt, size_t riovcnt);

static inline int
rpc_readv(rcf_rpc_server *rpcs,
          int fd, const struct rpc_iovec *iov,
          size_t iovcnt)
{
    return rpc_readv_gen(rpcs, fd, iov, iovcnt, iovcnt);
}


extern rpc_fd_set *rpc_fd_set_new(rcf_rpc_server *rpcs);
extern void rpc_fd_set_delete(rcf_rpc_server *rpcs,
                              rpc_fd_set *set);

extern void rpc_do_fd_zero(rcf_rpc_server *rpcs,
                           rpc_fd_set *set);
extern void rpc_do_fd_set(rcf_rpc_server *rpcs,
                          int fd, rpc_fd_set *set);
extern void rpc_do_fd_clr(rcf_rpc_server *rpcs,
                          int fd, rpc_fd_set *set);
extern int  rpc_do_fd_isset(rcf_rpc_server *rpcs,
                            int fd, rpc_fd_set *set);

extern int rpc_select(rcf_rpc_server *rpcs,
                      int n, rpc_fd_set *readfds, rpc_fd_set *writefds,
                      rpc_fd_set *exceptfds, struct timeval *timeout);

extern int rpc_pselect(rcf_rpc_server *rpcs,
                       int n, rpc_fd_set *readfds, rpc_fd_set *writefds,
                       rpc_fd_set *exceptfds, struct timespec *timeout,
                       const rpc_sigset_t *sigmask);


struct rpc_pollfd {
    int            fd;         /**< a file descriptor */
    short events;     /**< requested events FIXME */
    short revents;    /**< returned events FIXME */
};

extern int rpc_poll_gen(rcf_rpc_server *rpcs,
                        struct rpc_pollfd *ufds, unsigned int nfds,
                        int timeout, unsigned int rnfds);

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


extern int rpc_ioctl(rcf_rpc_server *rpcs,
                     int fd, rpc_ioctl_code request, ...);

extern int rpc_fcntl(rcf_rpc_server *rpcs, int fd,
                     int cmd, int arg);

extern int rpc_pipe(rcf_rpc_server *rpcs,
                    int filedes[2]);

extern int rpc_socketpair(rcf_rpc_server *rpcs,
                          rpc_socket_domain domain, rpc_socket_type type,
                          rpc_socket_proto protocol, int sv[2]);

extern uid_t rpc_getuid(rcf_rpc_server *rpcs);

extern int rpc_setuid(rcf_rpc_server *rpcs,
                      uid_t uid);

extern uid_t rpc_geteuid(rcf_rpc_server *rpcs);

extern int rpc_seteuid(rcf_rpc_server *rpcs,
                       uid_t uid);


#endif /* !__TE_TAPI_RPC_UNISTD_H__ */
