/** @file
 * @brief Test API - Socket API RPC
 *
 * TAPI for remote socket calls
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

#ifndef __TE_TAPI_RPCSOCK_H__
#define __TE_TAPI_RPCSOCK_H__

#include "tapi_rpcsock_defs.h"

/* Windows Event Objects */
typedef void *rpc_wsaevent;

/* Windows WSAOVERLAPPED structure */
typedef void *rpc_overlapped;

/**
 * Set dynamic library name to be used for additional name resolution.
 *
 * @param rpcs          Existing RPC server handle
 * @param libname       Name of the dynamic library or NULL
 *
 * @return Status code
 */
extern int rpc_setlibname(rcf_rpc_server *rpcs, const char *libname);


/*
 * Most functions have the same prototype and semantics of parameters
 * and return code as Linux implementation of Berkeley Socket API
 * functions.
 *
 * The only exception is the first parameter - RPC server handle created
 * using RCF RPC library calls.
 *
 * If NULL pointer is provided to API call, no data are read/write
 * from/to.  The pointer is just passed to RPC interface to call
 * remote function with NULL argument.
 *
 * *_gen functions provide more generic interface for test purposes.
 * Additional arguments specify real length of the provided buffers.
 * Such functionality is required to verify that called function
 * writes nothing across provided buffer boundaries.
 */

extern int rpc_socket(rcf_rpc_server *handle,
                      rpc_socket_domain domain, rpc_socket_type type,
                      rpc_socket_proto protocol);

/* WSASocket() */
extern int rpc_wsa_socket(rcf_rpc_server *handle,
                          rpc_socket_domain domain, rpc_socket_type type,
                          rpc_socket_proto protocol,
                          uint8_t *info, int info_len, te_bool overlapped);

/**
 * WSADuplicateSocket(). Protocol info is copied to the Test Engine
 * and then back  to the TA (in rpc_wsa_socket() function) as is.
 *
 * @param handle        RPC Server handle
 * @param s             old socket
 * @param pid           destination process PID
 * @param info          buffer for protocol info or NULL
 * @param info_len      buffer length location (IN)/protocol info length
 *                      location (OUT)
 *
 * @return value returned by WSADuplicateSocket() function
 */
extern int rpc_wsa_duplicate_socket(rcf_rpc_server *handle,
                                    int s, int pid,
                                    uint8_t *info, int *info_len);

extern int rpc_close(rcf_rpc_server *handle,
                     int fd);

extern int rpc_dup(rcf_rpc_server *handle,
                   int oldfd);
extern int rpc_dup2(rcf_rpc_server *handle,
                    int oldfd, int newfd);

extern int rpc_shutdown(rcf_rpc_server *handle,
                        int s, rpc_shut_how how);


extern int rpc_write(rcf_rpc_server *handle,
                     int fd, const void *buf, size_t count);

extern int rpc_read_gen(rcf_rpc_server *handle,
                        int fd, void *buf, size_t count,
                        size_t rbuflen);

static inline int
rpc_read(rcf_rpc_server *handle,
         int fd, void *buf, size_t count)
{
    return rpc_read_gen(handle, fd, buf, count, count);
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

extern int rpc_writev(rcf_rpc_server *handle,
                      int fd, const struct rpc_iovec *iov,
                      size_t iovcnt);

extern int rpc_readv_gen(rcf_rpc_server *handle,
                         int fd, const struct rpc_iovec *iov,
                         size_t iovcnt, size_t riovcnt);

static inline int
rpc_readv(rcf_rpc_server *handle,
          int fd, const struct rpc_iovec *iov,
          size_t iovcnt)
{
    return rpc_readv_gen(handle, fd, iov, iovcnt, iovcnt);
}

extern ssize_t rpc_send(rcf_rpc_server *handle,
                        int s, const void *buf, size_t len,
                        int flags);

extern ssize_t rpc_sendto(rcf_rpc_server *handle,
                          int s, const void *buf, size_t len,
                          int flags,
                          const struct sockaddr *to, socklen_t tolen);


extern ssize_t rpc_recv_gen(rcf_rpc_server *handle,
                            int s, void *buf, size_t len,
                            rpc_send_recv_flags flags,
                            size_t rbuflen);

/* WSARecvEx() */
extern ssize_t rpc_wsa_recv_ex(rcf_rpc_server *handle,
                               int s, void *buf, size_t len,
                               rpc_send_recv_flags *flags,
                               size_t rbuflen);

static inline ssize_t
rpc_recv(rcf_rpc_server *handle,
         int s, void *buf, size_t len, rpc_send_recv_flags flags)
{
    return rpc_recv_gen(handle, s, buf, len, flags, len);
}

extern ssize_t rpc_recvfrom_gen(rcf_rpc_server *handle,
                                int s, void *buf, size_t len,
                                rpc_send_recv_flags flags,
                                struct sockaddr *from, socklen_t *fromlen,
                                size_t rbuflen, socklen_t rfrombuflen);

static inline ssize_t
rpc_recvfrom(rcf_rpc_server *handle,
             int s, void *buf, size_t len,
             rpc_send_recv_flags flags,
             struct sockaddr *from, socklen_t *fromlen)
{
    return rpc_recvfrom_gen(handle, s, buf, len, flags, from, fromlen,
                            len, (fromlen == NULL) ? 0 : *fromlen);
}


typedef struct rpc_msghdr {
    void             *msg_name;         /**< protocol address */
    socklen_t         msg_namelen;      /**< size of protocol address */
    struct rpc_iovec *msg_iov;          /**< scatter/gather array */
    size_t            msg_iovlen;       /**< # elements in msg_iov */
    void             *msg_control;      /**< ancillary data; must be
                                             aligned for a cmsghdr
                                             stucture */
    socklen_t            msg_controllen; /**< length of ancillary data */
    rpc_send_recv_flags  msg_flags;      /**< flags returned by recvmsg() */

    /* Non-standard fields for test purposes */
    socklen_t         msg_rnamelen;     /**< real size of protocol
                                             address buffer to be copied
                                             by RPC */
    size_t            msg_riovlen;      /**< real number of elements
                                             in msg_iov */
    socklen_t         msg_rcontrollen;  /**< real length of the
                                             ancillary data buffer bo be
                                             copied by RPC */
} rpc_msghdr;

extern ssize_t rpc_sendmsg(rcf_rpc_server *handle,
                           int s, const struct rpc_msghdr *msg,
                           rpc_send_recv_flags flags);

extern ssize_t rpc_recvmsg(rcf_rpc_server *handle,
                           int s, struct rpc_msghdr *msg,
                           rpc_send_recv_flags flags);


extern int rpc_bind(rcf_rpc_server *handle,
                    int s, const struct sockaddr *my_addr,
                    socklen_t addrlen);

extern int rpc_connect(rcf_rpc_server *handle,
                       int s, const struct sockaddr *addr,
                       socklen_t addrlen);

extern int rpc_connect_ex(rcf_rpc_server *handle,
                          int s, const struct sockaddr *addr,
                          socklen_t addrlen,
                          void *buf, ssize_t len_buf,
                          ssize_t *bytes_sent,
                          rpc_overlapped overlapped);

extern int rpc_disconnect_ex(rcf_rpc_server *handle, int s,
                             rpc_overlapped overlapped, int flags);

extern int rpc_listen(rcf_rpc_server *handle,
                      int s, int backlog);


extern int rpc_accept_gen(rcf_rpc_server *handle,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen,
                          socklen_t raddrlen);

static inline int
rpc_accept(rcf_rpc_server *handle,
           int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return rpc_accept_gen(handle, s, addr, addrlen,
                          (addrlen == NULL) ? 0 : *addrlen);
}

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
extern int rpc_wsa_accept(rcf_rpc_server *handle,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen, socklen_t raddrlen,
                          accept_cond *cond, int cond_num);
/**
 * Client implementation of AcceptEx()-GetAcceptExSockAddr() call.
 *
 * @param handle           RPC server handle
 * @param s                Descriptor of socket that has already been
 *                         called with the listen function
 * @param s_a              Descriptor of a socket on wich to accept
 *                         an incomming connection
 * @param len              Length of the buffer to receive data (should not
 *                         include the size of local and remote addresses)
 * @param overlapped       WSAOVERLAPPED structure
 * @param bytes_received   number of received data bytes
 *
 * @return value returned by AcceptEx() function.
 */
extern int
rpc_accept_ex(rcf_rpc_server *handle, int s, int s_a,
              size_t len, rpc_overlapped overlapped,
              size_t *bytes_received);
/**
 * Client implementation of GetAcceptExSockAddr() call.
 *
 * @param handle           RPC server handle
 * @param s                descriptor of socket that was passed
 *                         to rpc_accept_ex() fuinction as 3d parameter
 * @param buf              pointer to a buffer passed to
 *                         rpc_get_overlapped_result()
 * @param buflen           size of the buffer passed to
 *                         rpc_get_overlapped_result()
 * @param len              buffer size wich was passed to rpc_accept_ex()
 * @param laddr            local address returned by GetAcceptExSockAddr()
 * @param raddr            remote address returned by GetAcceptExSockAddr()
 *
 * @return N/A
 */
extern void
rpc_get_accept_addr(rcf_rpc_server *handle,
                    int s, void *buf, size_t buflen, size_t len,
                    struct sockaddr *laddr,
                    struct sockaddr *raddr);

extern int rpc_transmit_file(rcf_rpc_server *handle, int s, char *file,
                             ssize_t len, ssize_t len_per_send,
                             rpc_overlapped overlapped,
                             void *head, ssize_t head_len,
                             void *tail, ssize_t tail_len, ssize_t flags);

extern rpc_fd_set *rpc_fd_set_new(rcf_rpc_server *handle);
extern void rpc_fd_set_delete(rcf_rpc_server *handle,
                              rpc_fd_set *set);

extern void rpc_do_fd_zero(rcf_rpc_server *handle,
                           rpc_fd_set *set);
extern void rpc_do_fd_set(rcf_rpc_server *handle,
                          int fd, rpc_fd_set *set);
extern void rpc_do_fd_clr(rcf_rpc_server *handle,
                          int fd, rpc_fd_set *set);
extern int  rpc_do_fd_isset(rcf_rpc_server *handle,
                            int fd, rpc_fd_set *set);

extern int rpc_select(rcf_rpc_server *handle,
                      int n, rpc_fd_set *readfds, rpc_fd_set *writefds,
                      rpc_fd_set *exceptfds, struct timeval *timeout);

extern int rpc_pselect(rcf_rpc_server *handle,
                       int n, rpc_fd_set *readfds, rpc_fd_set *writefds,
                       rpc_fd_set *exceptfds, struct timespec *timeout,
                       const rpc_sigset_t *sigmask);


struct rpc_pollfd {
    int            fd;         /**< a file descriptor */
    short events;     /**< requested events FIXME */
    short revents;    /**< returned events FIXME */
};

extern int rpc_poll_gen(rcf_rpc_server *handle,
                        struct rpc_pollfd *ufds, unsigned int nfds,
                        int timeout, unsigned int rnfds);

static inline int
rpc_poll(rcf_rpc_server *handle,
         struct rpc_pollfd *ufds, unsigned int nfds,
         int timeout)
{
    return rpc_poll_gen(handle, ufds, nfds, timeout, nfds);
}


extern int rpc_ioctl(rcf_rpc_server *handle,
                     int fd, rpc_ioctl_code request, ...);

extern int rpc_fcntl(rcf_rpc_server *handle, int fd,
                     int cmd, int arg);


extern int rpc_getsockopt_gen(rcf_rpc_server *handle,
                              int s, rpc_socklevel level,
                              rpc_sockopt optname,
                              void *optval, socklen_t *optlen,
                              socklen_t roptlen);

static inline int
rpc_getsockopt(rcf_rpc_server *handle,
               int s, rpc_socklevel level, rpc_sockopt optname,
               void *optval, socklen_t *optlen)
{
    return rpc_getsockopt_gen(handle, s, level, optname, optval, optlen,
                              (optlen != NULL) ? *optlen : 0);
}


extern int rpc_setsockopt(rcf_rpc_server *handle,
                          int s, rpc_socklevel level, rpc_sockopt optname,
                          const void *optval, socklen_t optlen);


extern int rpc_getsockname_gen(rcf_rpc_server *handle,
                               int s, struct sockaddr *name,
                               socklen_t *namelen,
                               socklen_t rnamelen);

static inline int
rpc_getsockname(rcf_rpc_server *handle,
                int s, struct sockaddr *name, socklen_t *namelen)
{
    return rpc_getsockname_gen(handle, s, name, namelen,
                               (namelen == NULL) ? 0 : *namelen);
}

extern int rpc_getpeername_gen(rcf_rpc_server *handle,
                               int s, struct sockaddr *name,
                               socklen_t *namelen,
                               socklen_t rnamelen);

static inline int
rpc_getpeername(rcf_rpc_server *handle,
                int s, struct sockaddr *name, socklen_t *namelen)
{
    return rpc_getpeername_gen(handle, s, name, namelen,
                               (namelen == NULL) ? 0 : *namelen);
}


/*
 * Interface name/index
 */

extern unsigned int rpc_if_nametoindex(rcf_rpc_server *handle,
                                       const char *ifname);

extern char *rpc_if_indextoname(rcf_rpc_server *handle,
                                unsigned int ifindex, char *ifname);

extern struct if_nameindex * rpc_if_nameindex(rcf_rpc_server *handle);

extern void rpc_if_freenameindex(rcf_rpc_server *handle,
                                 struct if_nameindex *ptr);

/* WSACreateEvent() */
extern rpc_wsaevent rpc_create_event(rcf_rpc_server *handle);

/* WSACloseEvent() */
extern int rpc_close_event(rcf_rpc_server *handle, rpc_wsaevent hevent);

/* WSAResetEvent() */
extern int rpc_reset_event(rcf_rpc_server *handle, rpc_wsaevent hevent);

/* Create WSAOVERLAPPED */
extern rpc_overlapped rpc_create_overlapped(rcf_rpc_server *handle,
                                            rpc_wsaevent hevent,
                                            unsigned int offset,
                                            unsigned int offset_high);

/* Delete WSAOVERLAPPED */
extern void rpc_delete_overlapped(rcf_rpc_server *handle,
                                  rpc_overlapped overlapped);

/* WSASend() */
extern int rpc_wsa_send(rcf_rpc_server *handle,
                       int s, const struct rpc_iovec *iov,
                       size_t iovcnt, rpc_send_recv_flags flags,
                       int *bytes_sent, rpc_overlapped overlapped,
                       te_bool callback);

/* WSARecv() */
extern int rpc_wsa_recv(rcf_rpc_server *handle,
                        int s, const struct rpc_iovec *iov,
                        size_t iovcnt, size_t riovcnt,
                        rpc_send_recv_flags *flags,
                        int *bytes_received, rpc_overlapped overlapped,
                        te_bool callback);

/* WSAGetOverlappedResult() */
extern int rpc_get_overlapped_result(rcf_rpc_server *handle,
                                     int s, rpc_overlapped overlapped,
                                     int *bytes, te_bool wait,
                                     rpc_send_recv_flags *flags,
                                     char *buf, int buflen);
/**
 * Get result of completion callback (if called).
 *
 * @param handle        handle of RPC server
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
extern int rpc_completion_callback(rcf_rpc_server *handle,
                                   int *called, int *error, int *bytes,
                                   rpc_overlapped *overlapped);

/* WSAEventSelect() */
extern int
rpc_wsa_event_select(rcf_rpc_server *handle,
                     int s, rpc_wsaevent event_object,
                     rpc_network_event event);

/**
 * Client implementation of WSAEnumNetworkEvents().
 *
 * @param handle           RPC server handle
 * @param s                socket descriptor
 * @param event_object     event object to be reset
 * @param event            network events that occurred
 *
 * @return value returned by WSAEnumNetworkEvents() function
 */
extern int rpc_enum_network_events(rcf_rpc_server *handle,
                                  int s, rpc_wsaevent event_object,
                                  rpc_network_event *event);

/** Return codes for rpc_wait_multiple_events */
enum {
    WSA_WAIT_FAILED,
    WAIT_IO_COMPLETION,
    WSA_WAIT_TIMEOUT,
    WSA_WAIT_EVENT_0
};

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

/** WSAWaitForMultipleEvents(), returns -1 if timeout expires */
extern int rpc_wait_multiple_events(rcf_rpc_server *handle,
                                    int count, rpc_wsaevent *events,
                                    te_bool wait_all, uint32_t timeout,
                                    te_bool alertable, int rcount);

/* Window objects */

typedef void *rpc_hwnd;

/** CreateWIndow() */
extern rpc_hwnd rpc_create_window(rcf_rpc_server *handle);

/** DestroyWindow() */
extern void rpc_destroy_window(rcf_rpc_server *handle, rpc_hwnd hwnd);

/** WSAAsyncSelect() */
extern int rpc_wsa_async_select(rcf_rpc_server *handle,
                                int s, rpc_hwnd hwnd,
                                rpc_network_event event);

/** PeekMessage() */
extern int rpc_peek_message(rcf_rpc_server *handle,
                            rpc_hwnd hwnd, int *s,
                            rpc_network_event *event);

/*
 * Signals
 */

extern char *rpc_signal(rcf_rpc_server *handle,
                        rpc_signum signum, const char *handler);

extern int rpc_kill(rcf_rpc_server *handle,
                    pid_t pid, rpc_signum signum);


#define RCF_RPC_MAX_FUNC_NAME    64

typedef struct rpc_struct_sigaction {
    char          mm_handler[RCF_RPC_MAX_FUNC_NAME];
    rpc_sigset_t *mm_mask;
    rpc_sa_flags  mm_flags;
    char          mm_restorer[RCF_RPC_MAX_FUNC_NAME];
} rpc_struct_sigaction;

extern int rpc_sigaction(rcf_rpc_server *handle,
                         rpc_signum signum,
                         const struct rpc_struct_sigaction *act,
                         struct rpc_struct_sigaction *oldact);


/*
 * Signal sets
 */

/**
 * Allocate new signal set on RPC server side.
 *
 * @param handle    RPC server handle
 *
 * @return Handle of allocated signal set or NULL.
 */
extern rpc_sigset_t *rpc_sigset_new(rcf_rpc_server *handle);

/**
 * Free allocated using rpc_sigset_new() signal set.
 *
 * @param handle    RPC server handle
 * @param set       signal set handler
 */
extern void rpc_sigset_delete(rcf_rpc_server *handle, rpc_sigset_t *set);

/**
 * Get handle of set of signals received by special signal handler
 * 'signal_registrar'.
 *
 * @param handle    RPC server handle
 *
 * @return Handle of the signal set (unique for RPC server,
 *         i.e. for each thread).
 */
extern rpc_sigset_t *rpc_sigreceived(rcf_rpc_server *handle);


extern int rpc_sigprocmask(rcf_rpc_server *handle,
                           rpc_sighow how, const rpc_sigset_t *set,
                           rpc_sigset_t *oldset);


extern int rpc_sigemptyset(rcf_rpc_server *handle,
                           rpc_sigset_t *set);

extern int rpc_sigfillset(rcf_rpc_server *handle,
                          rpc_sigset_t *set);

extern int rpc_sigaddset(rcf_rpc_server *handle,
                         rpc_sigset_t *set, rpc_signum signum);

extern int rpc_sigdelset(rcf_rpc_server *handle,
                         rpc_sigset_t *set, rpc_signum signum);

extern int rpc_sigismember(rcf_rpc_server *handle,
                           const rpc_sigset_t *set, rpc_signum signum);

extern int rpc_sigpending(rcf_rpc_server *handle, rpc_sigset_t *set);

extern int rpc_sigsuspend(rcf_rpc_server *handle, const rpc_sigset_t *set);


/*
 * Name/address resolution
 */

/*
 * Memory is allocated by these functions using malloc().
 * They are thread-safe.
 */
extern struct hostent *rpc_gethostbyname(rcf_rpc_server *handle,
                                         const char *name);

extern struct hostent *rpc_gethostbyaddr(rcf_rpc_server *handle,
                                         const char *addr,
                                         int len,
                                         rpc_socket_addr_family type);

extern int rpc_getaddrinfo(rcf_rpc_server *handle,
                           const char *node, const char *service,
                           const struct addrinfo *hints,
                           struct addrinfo **res);

extern void rpc_freeaddrinfo(rcf_rpc_server *handle,
                             struct addrinfo *res);

extern int rpc_pipe(rcf_rpc_server *handle,
                    int filedes[2]);

extern int rpc_socketpair(rcf_rpc_server *handle,
                          rpc_socket_domain domain, rpc_socket_type type,
                          rpc_socket_proto protocol, int sv[2]);

extern FILE *rpc_fopen(rcf_rpc_server *handle,
                       const char *path, const char *mode);

extern FILE *rpc_popen(rcf_rpc_server *handle,
                       const char *cmd, const char *mode);

/** Maximum resulting command length for rpc_shell() */
#define RPC_SHELL_CMDLINE_MAX   256
/**
 * Execute shell command on the IPC server and read the output.
 *
 * @param handle        RPC server handle
 * @param buf           output buffer
 * @param buflen        output buffer length
 * @param cmd           format of the command to be executed
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_shell(rcf_rpc_server *handle,
                     char *buf, int buflen, const char *cmd,...);

/**
 * Execute shell command on the IPC server and read the output.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 *
 * @param handle        RPC server handle
 * @param buf           location for the command output buffer 
 * @param cmd           format of the command to be executed
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_shell_get_all(rcf_rpc_server *handle,
                             char **pbuf, const char *cmd,...);

/**
 * Execute shell command on the IPC server and return file descriptor
 * for it's standard input or output.
 *
 * @param handle        RPC server handle
 * @param mode          "r" or "w"
 * @param cmd           format of the command to be executed
 *
 * @return File descriptor or -1 in the case of failure
 */
extern int rpc_cmd_spawn(rcf_rpc_server *handle, const char *mode,
                         const char *cmd,...);

extern int rpc_fileno(rcf_rpc_server *handle,
                      FILE *f);

extern uid_t rpc_getuid(rcf_rpc_server *handle);

extern int rpc_setuid(rcf_rpc_server *handle,
                      uid_t uid);

extern uid_t rpc_geteuid(rcf_rpc_server *handle);

extern int rpc_seteuid(rcf_rpc_server *handle,
                       uid_t uid);

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
 * @param received          location for number of received bytes
 *
 * @return number of received bytes or -1 in the case of failure
 */
extern int rpc_simple_receiver(rcf_rpc_server *handle,
                               int s, uint64_t *received);

/**
 * Send traffic. Send UDP datagrams from different sockets
 * toward different addresses.
 *
 * @param handle        RPC server
 * @param num           Number of UDP datagrams to being sent
 * @param s             List of sockets (num)
 * @param buf           Buffer to being sent
 * @param len           buffer size
 * @param flags         flags passed to sendto()
 * @param to            List of sockaddr-s (num)
 * @param tolen         Address size
 * 
 * @return 0 in case of success, -1 in case of failure
 */ 
int
rpc_send_traffic(rcf_rpc_server *handle, int num,
                 int *s, const void *buf, size_t len,
                 int flags,
                 struct sockaddr *to, socklen_t tolen);

/**
 * Asynchronous read test procedure.
 *
 * @param handle            RPC server
 * @param s                 a socket to be user for receiving
 * @param signum            signal to be used as notification event
 * @param timeout           timeout for blocking on select() in seconds
 * @param buf               buffer for received data
 * @param buflen            buffer length to be passed to the read
 * @param rlen              real buffer length
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of received bytes or -1 in the case of failure
 */
extern int rpc_aio_read_test(rcf_rpc_server *handle,
                             int s, rpc_signum signum, int timeout,
                             char *buf, int buflen, int rlen,
                             char *diag, int diag_len);

/**
 * Asynchronous error processing test procedure.
 *
 * @param handle            RPC server
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of received bytes or -1 in the case of failure
 */
extern int rpc_aio_error_test(rcf_rpc_server *handle,
                              char *diag, int diag_len);

/*
 * Asynchronous write test procedure.
 *
 * @param handle            RPC server
 * @param s                 a socket to be user for receiving
 * @param signum            signal to be used as notification event
 * @param buf               buffer for data to be sent
 * @param buflen            data length
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of sent bytes or -1 in the case of failure
 */
extern int rpc_aio_write_test(rcf_rpc_server *handle,
                              int s, rpc_signum signum,
                              char *buf, int buflen,
                              char *diag, int diag_len);
/**
 * Susending on asynchronous events test procedure.
 *
 * @param handle            RPC server
 * @param s                 a socket to be user for receiving
 * @param s_aux             dummy socket
 * @param signum            signal to be used as notification event
 * @param timeout           timeout for blocking on ai_suspend() in seconds
 * @param buf               buffer for received data
 * @param buflen            buffer length to be passed to the read
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of received bytes or -1 in the case of failure
 */
extern int rpc_aio_suspend_test(rcf_rpc_server *handle,
                                int s, int s_aux, rpc_signum signum,
                                int timeout, char *buf, int buflen,
                                char *diag, int diag_len);


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
                             int bulkszs, int time2run, int iomux,
                             te_bool rx_nonblock,
                             uint64_t *tx_stat,
                             uint64_t *rx_stat);

/**
 * Routine which receives data from specified set of
 * sockets using I/O multiplexing and sends them back
 * to the socket.
 *
 * @param handle        RPC server
 * @param sockets       Set of sockets to be processed
 * @param socknum       Number of sockets to be processed
 * @param time2run      How long send data (in seconds)
 * @param iomux         Type of I/O Multiplexing function
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
 * @param sock          Pointer on a socket
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
 * @param sock          Socket for sending
 * @param nops          The number of send() operation should be executed
 *                      (the length of len_array)
 * @param vector        Array of lenghts for appropriate send() operation
 *
 * @return file descriptor, which may be used for reading/writing data
 */
extern int rpc_many_send(rcf_rpc_server *handle, int sock,
                         const int *vector, int nops, uint64_t *sent);

#endif /* __TE_TAPI_RPCSOCK_H__ */
