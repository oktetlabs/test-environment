/** @file
 * @brief Test API - Socket API RPC
 *
 * Macros for remote socket calls
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPCSOCK_MACROS_H__
#define __TE_TAPI_RPCSOCK_MACROS_H__

#ifndef MACRO_ERROR_EXIT
#define MACRO_ERROR_EXIT        goto cleanup
#endif

#ifndef MACRO_TEST_ERROR
#define MACRO_TEST_ERROR        result = EXIT_FAILURE
#endif

/**
 * All the macros defined in the file expect user having in the calling
 * context the following:
 *     - @p result variable of type @c int;
 *     - @c cleanup label, which is responsible for freeing 
 *       resources allocated or occuped in the test.
 *     .
 */


/**
 * The macro should not be used anywhere but this file
 */
#define LOG_ERRNO(rpcs_, rc_, func_, fmt_func_args_, func_args_value_...) \
    do {                                                                  \
        int err_ = RPC_ERRNO(rpcs_);                                      \
                                                                          \
        if (IS_IUT_ERRNO(err_))                                           \
        {                                                                 \
            if (!!((#func_args_value_)[0]))                               \
            {                                                             \
                ERROR("RPC " #func_  fmt_func_args_                       \
                      " on %s failed retval=%d, RPC_errno=%X",            \
                      func_args_value_ + 0,                               \
                      RPC_NAME(rpcs_),                                    \
                      (rc_),                                              \
                      TE_RC_GET_ERROR(err_));                             \
            }                                                             \
            else                                                          \
            {                                                             \
                ERROR("RPC " #func_  fmt_func_args_                       \
                      " on %s failed retval=%d, RPC_errno=%X",            \
                      RPC_NAME(rpcs_),                                    \
                      (rc_),                                              \
                      TE_RC_GET_ERROR(err_));                             \
            }                                                             \
        }                                                                 \
        MACRO_TEST_ERROR;                                                 \
    } while (0)


/**
 * Macro to check function 'func_' on returning non negative value
 * 
 * @param rpcs_    RPS server
 * @param retval_  Return value (OUT)
 * @param func_    RPC function name to call (without rpc_ prefix)
 * @param args_    A set of arguments pased to the function
 */
#define RPC_FUNC_WITH_RETVAL(rpcs_, retval_, func_, args_...) \
    do {                                                        \
        int err2_ = RPC_ERRNO(rpcs_);                           \
                                                                \
        (retval_) =  rpc_ ## func_(rpcs_, args_);               \
        VERB("rpc_" #func_ " returns: %d, %X",                  \
             retval_, TE_RC_GET_ERROR(err2_));                  \
        if ((retval_) < 0)                                      \
        {                                                       \
            LOG_ERRNO((rpcs_), (retval_), func_, "()");         \
            MACRO_ERROR_EXIT;                                   \
        }                                                       \
    } while (0)

/**
 * Macro to check function 'func_' on returning non negative value
 * 
 * @param rpcs_    RPS server
 * @param retval_  Return value (OUT)
 * @param func_    RPC function name to call (without rpc_ prefix)
 */
#define RPC_FUNC_WITH_RETVAL0(rpcs_, retval_, func_)            \
    do {                                                        \
        int err2_ = RPC_ERRNO(rpcs_);                           \
                                                                \
        (retval_) =  rpc_ ## func_(rpcs_);                      \
        VERB("rpc_" #func_ " returns: %d, %X",                  \
             retval_, TE_RC_GET_ERROR(err2_));                  \
        if ((retval_) < 0)                                      \
        {                                                       \
            LOG_ERRNO((rpcs_), (retval_), func_, "()");         \
            MACRO_ERROR_EXIT;                                   \
        }                                                       \
    } while (0)

/**
 * Macro to check function 'func_' on returning exactly specified value.
 * 
 * @param rpcs_     RPS server
 * @param retval_   Return value (OUT)
 * @param expect_   Expected return value
 * @param func_     RPC function name to call (without rpc_ prefix)
 * @param args_     A set of arguments pased to the function
 */
#define RPC_FUNC_WITH_EXACT_RETVAL(rpcs_, retval_, expect_, \
                                   func_, args_...)             \
    do {                                                        \
        int err2_ = RPC_ERRNO(rpcs_);                           \
                                                                \
        (retval_) =  rpc_ ## func_(rpcs_, args_);               \
        VERB("rpc_" #func_ " returns: %d, %X",                  \
             retval_, TE_RC_GET_ERROR(err2_));                  \
        if ((int)(retval_) != (int)(expect_))                   \
        {                                                       \
            if (RPC_ERRNO(rpcs_) == 0)                          \
            {                                                   \
                ERROR(#func_ "() returned unexpected value %d " \
                      "instead of %d", (retval_), (expect_));   \
            }                                                   \
            else                                                \
            {                                                   \
                LOG_ERRNO((rpcs_), (retval_), func_, "()");     \
            }                                                   \
            MACRO_ERROR_EXIT;                                   \
        }                                                       \
    } while (0)

/**
 * Macro to check function 'func_' on returning non NULL value
 * 
 * @param rpcs_    RPS server
 * @param retval_  Return value (OUT)
 * @param func_    RPC function name to call (without rpc_ prefix)
 * @param args_    A set of arguments pased to the function
 */
#define RPC_FUNC_WITH_PTR_RETVAL(rpcs_, retval_, func_, args_...) \
    do {                                                      \
        (retval_) =  rpc_ ## func_(rpcs_, args_);             \
        if ((retval_) == NULL)                                \
        {                                                     \
            LOG_ERRNO((rpcs_), (retval_), func_, "()");       \
            MACRO_ERROR_EXIT;                                 \
        }                                                     \
    } while (0)


/**
 * Macro to check function 'func_' on returning non NULL value
 * 
 * @param rpcs_    RPS server
 * @param retval_  Return value (OUT)
 * @param func_    RPC function name to call (without rpc_ prefix)
 */
#define RPC_FUNC_WITH_PTR_RETVAL0(rpcs_, retval_, func_) \
    do {                                                 \
        (retval_) =  rpc_ ## func_(rpcs_);               \
        if ((retval_) == NULL)                           \
        {                                                \
            LOG_ERRNO((rpcs_), (retval_), func_, "()");  \
            MACRO_ERROR_EXIT;                            \
        }                                                \
    } while (0)

/**
 * Macro to check function 'func_' on returning zero value
 *
 * @param rpcs_    RPS server
 * @param func_    RPC function name to call (without rpc_ prefix)
 * @param args_    A set of arguments pased to the function
 */
#define RPC_FUNC_ZERO_RETVAL(rpcs_, func_, args_...) \
    do {                                             \
        int rc_;                                     \
                                                     \
        (rc_) =  rpc_ ## func_(rpcs_, args_);        \
        if ((rc_) != 0)                              \
        {                                            \
            LOG_ERRNO((rpcs_), (rc_), func_, "()");  \
            MACRO_ERROR_EXIT;                        \
        }                                            \
    } while (0)
    

/**
 * Create a new socket on specified RPC server
 *
 * @param sockd_    Variable that is updated with the descriptor
 *                  of created socket (OUT)
 * @param rpcs_     RPC server handle
 * @param domain_   Socket domain
 * @param type_     Socket type
 * @param proto_    Protocol value
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SOCKET(sockd_, rpcs_, domain_, type_, proto_) \
            RPC_FUNC_WITH_RETVAL(rpcs_, sockd_, socket,         \
                                 (domain_), (type_), (proto_))

/**
 * Duplicate a file descriptor.
 *
 * @param sockd_    Variable that is updated with the descriptor of 
 *                  created socket (OUT)
 * @param rpcs_     RPC server handle
 * @param oldfd_    FD to be duplicated
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_DUP(sockd_, rpcs_, oldfd_) \
    RPC_FUNC_WITH_RETVAL(rpcs_, sockd_, dup, oldfd_)

/**
 * Duplicate a file descriptor to specified file descriptor.
 *
 * @param sockd_    Variable that is updated with the descriptor of 
 *                  created socket (OUT)
 * @param rpcs_     RPC server handle
 * @param oldfd_    FD to be duplicated
 * @param newfd_    FD to duplicate to
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_DUP2(sockd_, rpcs_, oldfd_, newfd_) \
    RPC_FUNC_WITH_RETVAL(rpcs_, sockd_, dup2, oldfd_, newfd_)


/**
 * Close a socket on a particular RPC server, non-changing variable 
 * with socket descriptor.
 *
 * @param rpcs_   RPC server handle
 * @param sockd_  Socket descriptor
 */
#define RPC_CLOSE_UNSAFE(rpcs_, sockd_) \
    do {                                             \
        int rc_;                                     \
                                                     \
        rc_ = rpc_close((rpcs_), (sockd_));          \
        if ((rc_) < 0)                               \
        {                                            \
            LOG_ERRNO(rpcs_, (rc_), close, "()");    \
            MACRO_ERROR_EXIT;                        \
        }                                            \
    } while (0)


/**
 * Close a socket on a particular RPC server
 *
 * @param rpcs_   RPC server handle
 * @param sockd_  Socket descriptor
 *
 * @note @p sockd_ parameter is updated to -1 after successfull 
 * completion of the macro.
 */
#define RPC_CLOSE(rpcs_, sockd_) \
    do {                                             \
        int rc_;                                     \
                                                     \
        rc_ = rpc_close((rpcs_), (sockd_));          \
        if ((rc_) < 0)                               \
        {                                            \
            LOG_ERRNO(rpcs_, (rc_), close, "()");    \
            (sockd_) = -1;                           \
            MACRO_ERROR_EXIT;                        \
        }                                            \
        (sockd_) = -1;                               \
    } while (0)


/**
 * Bind a socket that resides on specified RPC server
 *
 * @param rpcs_     - RPC server handle
 * @param sockd_    - Socket descriptor
 * @param addr_     - Address the socket to be bound to
 * @param addrlen_  - Address length value
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_BIND(rpcs_, sockd_, addr_, addrlen_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, bind, (sockd_), (addr_), (addrlen_))

/**
 * Connect a socket that resides on specified RPC server
 *
 * @param rpcs_     - RPC server handle
 * @param sockd_    - Socket descriptor
 * @param addr_     - Address the socket to be connected to
 * @param addrlen_  - Address length value
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_CONNECT(rpcs_, sockd_, addr_, addrlen_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, connect, (sockd_), (addr_), (addrlen_))

/**
 * Make the socket as a server
 *
 * @param rpcs_     - RPC server handle
 * @param sockd_    - Socket descriptor
 * @param backlog_  - Backlog value
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_LISTEN(rpcs_, sockd_, backlog_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, listen, (sockd_), (backlog_))

/**
 * Accept a new socket from the list of pending sockets
 *
 * @param new_sockd_ - Variable that is updated with
 *                     the descriptor of a new socket (OUT)
 * @param rpcs_      - RPC server handle
 * @param sockd_     - Socket descriptor
 * @param addr_      - Location for the peer address
 * @param addrlen_   - Location for the peer address length (IN/OUT)
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_ACCEPT(new_sockd_, rpcs_, sockd_, addr_, addrlen_) \
    do {                                                                   \
        if (((addr_) == NULL && (addrlen_) != NULL) ||                     \
            ((addr_) != NULL && (addrlen_) == NULL))                       \
        {                                                                  \
            ERROR("RPC_ACCEPT(): Address and address length parameters "   \
                  "should be both not NULL or both NULL");                 \
            MACRO_TEST_ERROR;                                              \
            MACRO_ERROR_EXIT;                                              \
        }                                                                  \
                                                                           \
        RPC_FUNC_WITH_RETVAL(rpcs_, new_sockd_, accept,                    \
                             (sockd_), (addr_), (addrlen_));               \
    } while (0)

/**
 * Call send() function on RPC server and check return value.
 *
 * @param sent_     Number of sent bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param buf_      Buffer pointer
 * @param len_      Length of the data to be sent
 * @param flags_    Operation flags
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SEND(sent_, rpcs_, sockd_, buf_, len_, flags_) \
    RPC_FUNC_WITH_EXACT_RETVAL(rpcs_, sent_, len_, send, \
                               sockd_, buf_, len_, flags_)

/**
 * Call sendto() function on RPC server and check return value.
 *
 * @param sent_     Number of sent bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param buf_      Buffer pointer
 * @param len_      Length of the data to be sent
 * @param flags_    Operation flags
 * @param addr_     Destination address
 * @param addrlen_  Destination address length
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SENDTO(sent_, rpcs_, sockd_, buf_, len_, flags_, \
                   addr_, addrlen_)                                 \
    do {                                                            \
        if ((((addr_) == NULL) && ((addrlen_) != 0)) ||             \
            (((addr_) != NULL) && ((addrlen_) == 0)))               \
        {                                                           \
            ERROR("RPC_SENDTO(): Address and address length "       \
                  "parameters should be either NULL and zero, "     \
                  "or not NULL and not zero");                      \
            MACRO_TEST_ERROR;                                       \
            MACRO_ERROR_EXIT;                                       \
        }                                                           \
                                                                    \
        RPC_FUNC_WITH_EXACT_RETVAL(rpcs_, sent_, len_, sendto,      \
                                   sockd_, buf_, len_, flags_,      \
                                   addr_, addrlen_);                \
    } while (0)

/**
 * Call select() function on RPC server and check return value.
 *
 * @param retval_   Return value (OUT)
 * @param rpcs_     RPC server handle
 * @param maxfd_    Maximum socket descriptor plus 1 to be analysed
 * @param rd_       Read descriptors set
 * @param wr_       Write descriptors set
 * @param ex_       Exceptions descriptors set
 * @param tv_       Timeout
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SELECT(retval_, rpcs_, maxfd_, rd_, wr_, ex_, tv_) \
            RPC_FUNC_WITH_RETVAL(rpcs_, retval_, select,                \
                                 (maxfd_), (rd_), (wr_), (ex_), (tv_))

/** Call fd_set() on the specified RPC server */
#define RPC_DO_FD_SET(rpcs_, sockd_, set_) \
    do {                                                  \
        rpc_do_fd_set((rpcs_), (sockd_), (set_));         \
        CHECK_RPC_ERRNO((rpcs_), 0, "fd_set() fails to "  \
                        "add socket to the fdset");       \
    } while (0)

/**
 * Call recv() function on RPC server and check return value.
 *
 * @param received_ Number of received bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param buf_      Buffer for received data
 * @param len_      Size of the buffer
 * @param flags_    Operation flags
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_RECV(received_, rpcs_, sockd_, buf_, len_, flags_) \
            RPC_FUNC_WITH_RETVAL(rpcs_, received_, recv,             \
                                 (sockd_), (buf_), (len_), (flags_))

/**
 * Call recvfrom() function on RPC server and check return value.
 *
 * @param received_  Number of received bytes (OUT)
 * @param rpcs_      RPC server handle
 * @param sockd_     Socket descriptor
 * @param buf_       Buffer for received data
 * @param len_       Size of the buffer
 * @param flags_     Operation flags
 * @param addr_      Address of the peer from which the data is received (OUT)
 * @param addrlen_   Peer address length (IN/OUT)
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_RECVFROM(received_, rpcs_, sockd_, buf_, len_, flags_,  \
                     addr_, addrlen_)                                \
    RPC_FUNC_WITH_RETVAL(rpcs_, received_, recvfrom,         \
                         (sockd_), (buf_), (len_), (flags_), \
                         addr_, addrlen_)

/**
 * Call sendmsg() function on RPC server and check return value.
 *
 * @param sent_     Number of sent bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param msg_      Message to be sent
 * @param flags_    Operation flags
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SENDMSG(sent_, rpcs_, sockd_, msg_, flags_) \
    RPC_FUNC_WITH_RETVAL(rpcs_, sent_, sendmsg,      \
                         (sockd_), (msg_), (flags_))

/**
 * Call recvmsg() function on RPC server and check return value.
 *
 * @param received_ Number of received bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param msg_      Message for received data
 * @param flags_    Operation flags
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_RECVMSG(received_, rpcs_, sockd_, msg_, flags_) \
        RPC_FUNC_WITH_RETVAL(rpcs_, received_, recvmsg,     \
                             (sockd_), (msg_), (flags_))

/**
 * Call write() function on RPC server and check return value.
 *
 * @param sent_     Number of sent bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param buf_      Buffer pointer
 * @param len_      Length of the data to be sent
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_WRITE(sent_, rpcs_, sockd_, buf_, len_) \
    RPC_FUNC_WITH_EXACT_RETVAL(rpcs_, sent_, len_, write, \
                               sockd_, buf_, len_)

/**
 * Call read() function on RPC server and check return value.
 *
 * @param received_ Number of received bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param buf_      Buffer for received data
 * @param len_      Size of the buffer
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_READ(received_, rpcs_, sockd_, buf_, len_) \
        RPC_FUNC_WITH_RETVAL(rpcs_, received_, read,   \
                             (sockd_), (buf_), (len_))


/**
 * Call getsockopt() function on RPC server and check return value.
 *
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param level_    Option level
 * @param optname_  Option name
 * @param val_      Pointer to the option value (OUT)
 * @param len_      Pointer to the length of the option value (IN/OUT)
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_GETSOCKOPT(rpcs_, sockd_, level_, optname_, val_, len_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, getsockopt, (sockd_), (level_),     \
                         (optname_), (val_), (len_))

/**
 * Call setsockopt() function on RPC server and check return value.
 *
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor
 * @param level_    Option level
 * @param optname_  Option name
 * @param val_      Pointer to the option value
 * @param len_      Length of the option value
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SETSOCKOPT(rpcs_, sockd_, level_, optname_, val_, len_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, setsockopt, (sockd_), (level_),     \
                         (optname_), (val_), (len_))

/**
 * Get local address of the socket
 *
 * @param rpcs_      - RPC server handle
 * @param sockd_     - Socket descriptor
 * @param addr_      - Location for the local address
 * @param addrlen_   - Location for the local address length (IN/OUT)
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_GETSOCKNAME(rpcs_, sockd_, addr_, addrlen_) \
    do {                                                            \
        if ((addr_) == NULL || (addrlen_) == NULL)                  \
        {                                                           \
            ERROR("RPC_GETSOCKNAME(): Address and address length "  \
                  "parameters are not allowed to be NULL");         \
            MACRO_TEST_ERROR;                                       \
            MACRO_ERROR_EXIT;                                       \
        }                                                           \
                                                                    \
        RPC_FUNC_ZERO_RETVAL(rpcs_, getsockname,                    \
                             (sockd_), (addr_), (addrlen_));        \
    } while (0)

/**
 * Get peer address of the socket
 *
 * @param rpcs_      - RPC server handle
 * @param sockd_     - Socket descriptor
 * @param addr_      - Location for the peer address
 * @param addrlen_   - Location for the peer address length (IN/OUT)
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_GETPEERNAME(rpcs_, sockd_, addr_, addrlen_) \
    do {                                                            \
        if ((addr_) == NULL || (addrlen_) == NULL)                  \
        {                                                           \
            ERROR("RPC_GETPEERNAME(): Address and address length "  \
                  "parameters are not allowed to be NULL");         \
            MACRO_TEST_ERROR;                                       \
            MACRO_ERROR_EXIT;                                       \
        }                                                           \
                                                                    \
        RPC_FUNC_ZERO_RETVAL(rpcs_, getpeername,                    \
                             (sockd_), (addr_), (addrlen_));        \
    } while (0)

/**
 * Call ioctl request on specified RPC server with specified socket
 *
 * @param rpcs_      - RPC server handle
 * @param sockd_     - Socket descriptor
 * @param req_name_  - IOCTL request name
 * @param req_val_   - Request value
 * 
 * @todo const char *ioctl_val2str(req_name_, req_val_);
 */
#define RPC_IOCTL(rpcs_, sockd_, req_name_, req_val_) \
            RPC_FUNC_ZERO_RETVAL(rpcs_, ioctl,                       \
                                 (sockd_), (req_name_), (req_val_))

/**
 * Shutdown socket with specified mode
 *
 * @param rpcs_      - RPC server handle
 * @param sockd_     - Socket descriptor
 * @param shut_mode_ - Shutdown mode: SHUT_RD, SHUT_WR, or SHUT_RDWR
 */
#define RPC_SHUTDOWN(rpcs_, sockd_, shut_mode_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, shutdown, sockd_, shut_mode_)

/**
 * Send signal to process
 *
 * @param rpcs_         RPC server handle
 * @param pid_          PID of the target process
 * @param sig_          Signal to be sent
 */
#define RPC_KILL(rpcs_, pid_, sig_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, kill, pid_, sig_)

/**
 * Register signal handler for capturing a particular signal
 * 
 * @param rc_       Previous signal handler (OUT)
 * @param rpcs_     RPC server handle
 * @param signum_   Signal number
 * @param handler_  Signal handler (use SIGNAL_REGISTRAR to default handler)
 */
#define RPC_SIGNAL(rc_,  rpcs_, signum_, handler_) \
    do {                                                  \
        rc_ = rpc_signal((rpcs_), (signum_), (handler_)); \
        if ((rc_) == RPC_SIG_ERR)                         \
        {                                                 \
            LOG_ERRNO(rpcs_, rc_, signal, "(%s, %s)",     \
                      signum_rpc2str(signum_), handler_); \
            MACRO_ERROR_EXIT;                             \
        }                                                 \
    } while (0)

/**
 * Changes the list  of  currently  blocked signals.
 *
 * @param rpcs_          RPC server handle
 * @param how_           How change the list  of  currently  blocked
 *                       signals
 * @param sigmask_       New signal mask
 * @param sigmask_old_   Where the previous value of the signal mask is stored
 */
#define RPC_SIGPROCMASK(rpcs_, how_, sigmask_, sigmask_old_) \
    do {                                                            \
        int rc_;                                                    \
        rc_ = rpc_sigprocmask(rpcs_, how_, sigmask_, sigmask_old_); \
        if (rc_ == -1)                                              \
        {                                                           \
            LOG_ERRNO(rpcs_, rc_, sigprocmask, "(%s)",              \
                      signum_rpc2str(how_));                        \
            MACRO_ERROR_EXIT;                                       \
        }                                                           \
    } while (0)


/**
 * Get set of pending signals.
 *
 * @param rpcs_     RPC server handle
 * @param sigmask_  Sigmask handle
 */
#define RPC_SIGPENDING(rpcs_, sigmask_)\
    do {                                                 \
        int rc_;                                         \
        rc_ = rpc_sigpending(rpcs_, sigmask_);           \
        if (rc_ == -1)                                   \
        {                                                \
            LOG_ERRNO(rpcs_, rc_, sigpending, "()");     \
            MACRO_ERROR_EXIT;                            \
        }                                                \
    } while (0)


/**
 * Make the pair of file descriptors connected with a pipe on 
 * specified RPC server
 *
 * @param rpcs_     - RPC server handle
 * @param filedes_  - Array of size 2 for file descriptors
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_PIPE(rpcs_, filedes_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, pipe, (filedes_))

/**
 * Create a pair of connected sockets on specified RPC server
 *
 * @param rpcs_    RPC server handle
 * @param domain_  Socket domain
 * @param type_    Socket type
 * @param proto_   Protocol value
 * @param sv_      Array of size 2 for socket descriptors
 *
 * @note Usually supported only for PF_UNIX domain.
 * 
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SOCKETPAIR(rpcs_, domain_, type_, proto_, sv_) \
    RPC_FUNC_ZERO_RETVAL(rpcs_, socketpair, domain_, type_, proto_, sv_)


/**
 * Restore signal handler set before the test
 *
 * @param rpcs_         RPC server handle
 * @param signum_       Signal number
 * @param handler_      Signal handler to restore
 * @param old_handler_  Expected value of rpc_signal function, NULL
 *                      if we do not want to check return value
 */
#define CLEANUP_RPC_SIGNAL(rpcs_, signum_, handler_, old_handler_) \
    do {                                                                \
        if ((handler_) != NULL)                                         \
        {                                                               \
            char *ret_handler_;                                         \
                                                                        \
            (ret_handler_) = rpc_signal((rpcs_), (signum_), (handler_));\
            if ((ret_handler_) == NULL)                                 \
            {                                                           \
                LOG_ERRNO(rpcs_, ret_handler_, signal, "()");           \
                MACRO_TEST_ERROR;                                       \
            }                                                           \
            else if ((old_handler_) != NULL)                            \
            {                                                           \
                if (strcmp(ret_handler_, (old_handler_)) != 0)          \
                {                                                       \
                    ERROR("Value returned from rpc_signal() is "        \
                          "not the same as expected ");                 \
                    MACRO_TEST_ERROR;                                   \
                }                                                       \
                free(ret_handler_);                                     \
            }                                                           \
        }                                                               \
    } while (0)

/**
 * Check whether a particaular signal is a member of sigmask on 
 * a particular RPC server
 *
 * @param rc_       Returned value of type 'te_bool' (OUT)
 * @param rpcs_     RPC server handle
 * @param sigmask_  Signal mask
 * @param signum_   Signal number to check
 */
#define RPC_SIGISMEMBER(rc_, rpcs_, sigmask_, signum_) \
    RPC_FUNC_WITH_RETVAL(rpcs_, rc_, sigismember, (sigmask_), (signum_))

/**
 * Empty a signal set.
 *
 * @param rpcs_     RPC server handle
 * @param sigmask_  Signal mask
 */
#define RPC_SIGEMPTYSET(rpcs_, sigmask_) \
            RPC_FUNC_ZERO_RETVAL(rpcs_, sigemptyset, (sigmask_))

/**
 * Add a signal ts a set of signals
 *
 * @param rpcs_     RPC server handle
 * @param sigmask_  Signal mask
 * @param signum_   Signal number to add
 */
#define RPC_SIGADDSET(rpcs_, sigmask_, signum_) \
            RPC_FUNC_ZERO_RETVAL(rpcs_, sigaddset, (sigmask_), (signum_))

/**
 * Delete a signal from a set of signals
 *
 * @param rpcs_     RPC server handle
 * @param sigmask_  Signal mask
 * @param signum_   Signal number to delete
 */
#define RPC_SIGDELSET(rpcs_, sigmask_, signum_) \
            RPC_FUNC_ZERO_RETVAL(rpcs_, sigdelset, (sigmask_), (signum_))

/**
 * Get set of signals received by signal_registrar routine.
 *
 * @param set_      Set of received signals (OUT)
 * @param rpcs_     RPC server handle
 */
#define RPC_SIGRECEIVED(set_, rpcs_) \
    do {                                    \
        (set_) = rpc_sigreceived((rpcs_));  \
        if ((set_) == NULL)                 \
        {                                   \
            MACRO_TEST_ERROR;               \
            MACRO_ERROR_EXIT;               \
        }                                   \
    } while (0)

/**
 * Sets specified UID on RPC server
 *
 * @param rpcs_     RPC server handle
 * @param user_id_  A new UID
 */
#define RPC_SETUID(rpcs_, user_id_) \
            RPC_FUNC_ZERO_RETVAL((rpcs_), setuid, (user_id_))

/**
 * Get socket state and match it with specified expected value.
 *
 * @param rpcs_     RPC server handle
 * @param sockd_    Descriptor of socket to be processed
 * @param peer_     Peer RPC server handle or NULL
 * @param peer_s_   Peer socket or -1
 * @param state_    Expected socket state
 */
#define CHECK_SOCKET_STATE(rpcs_, sockd_, peer_, peer_s_, state_) \
    do {                                                                \
        int rc_;                                                        \
        sockts_socket_state_t got_state;                                \
                                                                        \
        rc_ = sockts_get_socket_state((rpcs_), (sockd_),                \
                                      (peer_), (peer_s_), &got_state);  \
        if (rc_ == 0)                                                   \
            INFO("Got state: RPC (%s,%s), socket %d, %s",               \
                    (rpcs_)->ta, (rpcs_)->name, (sockd_),               \
                    socket_state2str(got_state));                       \
        if (rc_ == 0 && got_state != (state_))                          \
        {                                                               \
            ERROR("Wrong socket state, got %s; expected %s",            \
                  socket_state2str(got_state),                          \
                  socket_state2str(state_));                            \
            rc_ = -1;                                                   \
        }                                                               \
        if (rc_ < 0)                                                    \
        {                                                               \
            MACRO_TEST_ERROR;                                           \
            MACRO_ERROR_EXIT;                                           \
        }                                                               \
    } while (0);


/**
 * Close a socket in cleanup part of the test
 *
 * @param rpcs_   - RPC server handle
 * @param sockd_  - Socket descriptor to be closed
 */
#define CLEANUP_RPC_CLOSE(rpcs_, sockd_) \
    do {                                                            \
        if ((sockd_) >= 0)                                          \
        {                                                           \
            int rc_ = rpc_close(rpcs_, sockd_);                     \
                                                                    \
            if (rc_ != 0)                                           \
            {                                                       \
                int err_ = RPC_ERRNO(rpcs_);                        \
                                                                    \
                if (IS_IUT_ERRNO(err_))                             \
                {                                                   \
                    ERROR("RPC close() on %s failed retval=%d "     \
                          "RPC_errno=%X",                           \
                          RPC_NAME(rpcs_), rc_,                     \
                          TE_RC_GET_ERROR(err_));                   \
                }                                                   \
                MACRO_TEST_ERROR;                                   \
            }                                                       \
        }                                                           \
    } while (0)

/**
 * Check current value of errno on a particular RPC server against 
 * some expected value
 *
 * @param rpcs_      - RPC server handle
 * @param exp_errno_ - Expected value of errno on the server
 * @param err_msg_   - Error message that should be used in case of failure
 * @param args_      - Arguments of err_msg_
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define CHECK_RPC_ERRNO(rpcs_, exp_errno_, err_msg_, args_...) \
    do {                                                                \
        rpc_errno err_ = RPC_ERRNO(rpcs_);                              \
                                                                        \
        if (err_ != (exp_errno_))                                       \
        {                                                               \
            if (!!((#args_)[0]))                                        \
            {                                                           \
                if (IS_IUT_ERRNO(err_))                                 \
                {                                                       \
                    ERROR(err_msg_ " sets errno to %s instead of %s",   \
                          args_ + 0, errno_rpc2str(err_),               \
                          errno_rpc2str(exp_errno_));                   \
                }                                                       \
                else                                                    \
                {                                                       \
                    ERROR(err_msg_ " sets errno to %X instead of %s",   \
                          args_ + 0, TE_RC_GET_ERROR(err_),             \
                          errno_rpc2str(exp_errno_));                   \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                if (IS_IUT_ERRNO(err_))                                 \
                {                                                       \
                    ERROR(err_msg_ " sets errno to %s instead of %s",   \
                          errno_rpc2str(err_),                          \
                          errno_rpc2str(exp_errno_));                   \
                }                                                       \
                else                                                    \
                {                                                       \
                    ERROR(err_msg_ " sets errno to %X instead of %s",   \
                          TE_RC_GET_ERROR(err_),                        \
                          errno_rpc2str(exp_errno_));                   \
                }                                                       \
            }                                                           \
            MACRO_TEST_ERROR;                                           \
            MACRO_ERROR_EXIT;                                           \
        }                                                               \
    } while (0)

/**
 * Get readability or writability of a particular socket.
 *
 * @param answer_    variable where the answer is stored
 *                   (it should be of type 'te_bool')
 * @param rpcs_      RPC server handle
 * @param sockd_     Socket to be checked
 * @param timeout_   Timeout in seconds
 * @param type_      Type of checking (read or write ability) -
 *                   you should specify READ or WRITE words
 *
 * @note In case of failure it calls TEST_FAIL macro, otherwise it
 * gets back just after the checking
 */
#define GET_RW_ABILITY(answer_, rpcs_, sockd_, timeout_, type_) \
    do {                                                                \
        struct timeval  tv_ = { timeout_ , 0 };                         \
        rpc_fd_set     *fds_ = rpc_fd_set_new(rpcs_);                   \
        int             rc_;                                            \
                                                                        \
        if (fds_ == NULL)                                               \
        {                                                               \
            TEST_FAIL("Failed to create a new rpc_fd_set entry");       \
        }                                                               \
        rpc_do_fd_zero(rpcs_, fds_);                                    \
        if (RPC_ERRNO(rpcs_))                                           \
        {                                                               \
            TEST_FAIL("rpc_do_fd_zero() fails with RPC_errno: %X",      \
                      RPC_ERRNO(rpcs_));                                \
        }                                                               \
        rpc_do_fd_set(rpcs_, sockd_, fds_);                             \
        if (RPC_ERRNO(rpcs_))                                           \
        {                                                               \
            TEST_FAIL("rpc_do_fd_set() fails with RPC_errno: %X",       \
                      RPC_ERRNO(rpcs_));                                \
        }                                                               \
                                                                        \
        if (#type_[0] == 'R')                                           \
        {                                                               \
            RPC_SELECT(rc_, rpcs_, (sockd_) + 1,                        \
                       fds_, NULL, NULL, &tv_);                         \
        }                                                               \
        else                                                            \
        {                                                               \
            RPC_SELECT(rc_, rpcs_, (sockd_) + 1,                        \
                       NULL, fds_, NULL, &tv_);                         \
        }                                                               \
        if (rc_ == 0)                                                   \
            answer_ = FALSE;                                            \
        else if ( rc_ == 1)                                             \
            answer_ = TRUE;                                             \
                                                                        \
        rpc_fd_set_delete(rpcs_, fds_);                                 \
        if (RPC_ERRNO(rpcs_))                                           \
        {                                                               \
            TEST_FAIL("rpc_fd_set_delete() fails with RPC_errno: %X",   \
                      RPC_ERRNO(rpcs_));                                \
        }                                                               \
    } while (0)

/**
 * Get readability of a particular socket.
 *
 * @param answer_    variable where stored is readable or not
 *                   (it should be of type 'te_bool')
 * @param rpcs_      RPC server handle
 * @param sockd_     Socket to be checked on readabilty
 * @param timeout_   timeout in seconds
 *
 * @note In case of failure it calls TEST_FAIL macro, otherwise it
 * gets back just after the checking
 */
#define GET_READABILITY(answer_, rpcs_, sockd_, timeout_) \
    GET_RW_ABILITY(answer_, rpcs_, sockd_, timeout_, READ)

/**
 * Get writability of a particular socket.
 *
 * @param answer_    variable where stored is readable or not
 *                   (it should be of type 'te_bool')
 * @param rpcs_      RPC server handle
 * @param sockd_     Socket to be checked on writability
 * @param timeout_   timeout in seconds
 *
 * @note In case of failure it calls TEST_FAIL macro, otherwise it
 * gets back just after the checking
 */
#define GET_WRITABILITY(answer_, rpcs_, sockd_, timeout_) \
    GET_RW_ABILITY(answer_, rpcs_, sockd_, timeout_, WRITE)

/**
 * Check readability of a particular socket.
 *
 * @param rpcs_                RPC server handle
 * @param sockd_               Socket to be checked on readabilty
 * @param should_be_readable_  Whether it should be readable or not
 *                             (it should be of type 'te_bool')
 *
 * @note In case of failure it calls TEST_FAIL macro, otherwise it
 * gets back just after the checking
 */
#define CHECK_READABILITY(rpcs_, sockd_, should_be_readable_) \
    do {                                                                \
        te_bool         answer_;                                        \
                                                                        \
        GET_READABILITY(answer_, rpcs_, sockd_, 1);                     \
        if (should_be_readable_ == TRUE && answer_ == FALSE)            \
        {                                                               \
            TEST_FAIL("Socket '" #sockd_ "' is expected to be "         \
                      "readable, but it is not");                       \
        }                                                               \
        else if (should_be_readable_ == FALSE && answer_ == TRUE)       \
        {                                                               \
            TEST_FAIL("Socket '" #sockd_ "' is not expected to be "     \
                      "readable, but it is");                           \
        }                                                               \
    } while (0)

/**
 * Check writability of a particular socket.
 *
 * @param rpcs_                 RPC server handle
 * @param sockd_                Socket to be checked on writabilty
 * @param should_be_writable_  Whether it should be writable or not
 *                              (it should be of type 'te_bool')
 *
 * @note In case of failure it calls TEST_FAIL macro, otherwise it
 * gets back just after the checking
 */
#define CHECK_WRITABILITY(rpcs_, sockd_, should_be_writable_) \
    do {                                                                \
        te_bool         answer_;                                        \
                                                                        \
        GET_WRITABILITY(answer_, rpcs_, sockd_, 1);                     \
        if (should_be_writable_ == TRUE && answer_ == FALSE)            \
        {                                                               \
            TEST_FAIL("Socket '" #sockd_ "' is expected to be "         \
                      "writable, but it is not");                       \
        }                                                               \
        else if (should_be_writable_ == FALSE && answer_ == TRUE)       \
        {                                                               \
            TEST_FAIL("Socket '" #sockd_ "' is not expected to be "     \
                      "writable, but it is");                           \
        }                                                               \
    } while (0)

/**
 * Call fileno() function on RPC server and check return value on errors.
 *
 * @param fd_       Integer file descriptor of the stream
 * @param rpcs_     RPC server handle
 * @param stream_   A stream
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_FILENO(fd_, rpcs_, stream_) \
    RPC_FUNC_WITH_RETVAL((rpcs_), (fd_), fileno, (stream_))

/**
 * Call fopen() function on RPC server and check return value on errors.
 *
 * @param f_        Stream descriptor
 * @param rpcs_     RPC server handle
 * @param fn_       File name
 * @param mode_     Open mode
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_FOPEN(f_, rpcs_, fn_, mode_) \
    RPC_FUNC_WITH_PTR_RETVAL((rpcs_), (f_), fopen, strdup(fn_), strdup(mode_))

/**
 * Call sendfile() function on RPC server and check return value
 * on errors.
 *
 * @param sent_     Number of written bytes (OUT)
 * @param rpcs_     RPC server handle
 * @param out_fd_   File descriptor opened for writing
 * @param in_fd_    File descriptor opened for reading
 * @param offset_   Pointer to a variable holding the input file pointer 
 *                  positionLength of the data to be sent
 * @param count_    Number of bytes to copy
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SENDFILE(sent_, rpcs_, out_fd_, in_fd_, offset_, count_) \
    RPC_FUNC_WITH_RETVAL(rpcs_, sent_, sendfile, (out_fd_),      \
                         (in_fd_), (offset_), (count_))

/**
 * Call socket_to_file function on RPC server and check return value
 * on errors.
 *
 * @param recv_       Number of received bytes (OUT)
 * @param rpcs_       RPC server handle
 * @param sockd_      Socket descriptor
 * @param file_name_  File name where write received data
 * @param timeout_    timeout in seconds
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SOCKET_TO_FILE(recv_, rpcs_, sockd_, file_name_, timeout_) \
    do {                                                                \
        char *position;                                                 \
        char  path_name[RCF_MAX_PATH] = "/tmp/";                        \
                                                                        \
        position = path_name + strlen(path_name);                       \
        position = strncpy(position, file_name_,                        \
                           sizeof(path_name) - strlen(path_name));      \
        (recv_) = rpc_socket_to_file((rpcs_), (sockd_),                 \
                                      path_name, (timeout_));           \
        if ((recv_) < 0)                                                \
        {                                                               \
            LOG_ERRNO((rpcs_), (recv_), socket_to_file, "()");          \
            MACRO_ERROR_EXIT;                                           \
        }                                                               \
    } while (0)

/**
 * Call simple_receiver function on RPC server and check return value
 * on errors.
 *
 * @param recv_       Number of received bytes (uint64_t) (OUT)
 * @param rpcs_       RPC server handle
 * @param sockd_      Socket descriptor
 * @param timeout_    timeout in seconds
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define RPC_SIMPLE_RECEIVER(recv_, rpcs_, sockd_, timeout_) \
    do {                                                                \
        int rc_ = rpc_simple_receiver((rpcs_), (sockd_), &(recv_));     \
        if ((rc_) != 0)                                                 \
        {                                                               \
            LOG_ERRNO((rpcs_), (recv_), simple_receiver, "()");         \
            MACRO_ERROR_EXIT;                                           \
        }                                                               \
    } while (0)

#endif /* !__TE_TAPI_RPCSOCK_MACROS_H__ */
