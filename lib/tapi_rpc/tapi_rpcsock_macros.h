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

/** Cleanup on test exit */
#define MACRO_ERROR_EXIT        (TAPI_JMP_DO(EXIT_FAILURE))
#endif

#ifndef MACRO_TEST_ERROR

/** exit code on test failure */
#define MACRO_TEST_ERROR        (result = EXIT_FAILURE)
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
 * Macro to check function 'func_' on returning exactly specified value.
 *
 * @param rpcs_     RPC server
 * @param retval_   return value (OUT)
 * @param expect_   expected return value
 * @param func_     RPC function name to call (without rpc_ prefix)
 * @param args_     a set of arguments pased to the function
 */
#define RPC_FUNC_WITH_EXACT_RETVAL(rpcs_, retval_, expect_, \
                                   func_, args_...)             \
    do {                                                        \
        (retval_) =  rpc_ ## func_(rpcs_, args_);               \
        if ((int)(retval_) != (int)(expect_))                   \
        {                                                       \
            ERROR(#func_ "() returned unexpected value %d "     \
                  "instead of %d", (retval_), (expect_));       \
            MACRO_ERROR_EXIT;                                   \
        }                                                       \
    } while (0)

/**
 * REMOVE IT.
 *
 * Macro to check function 'func_' on returning non NULL value
 *
 * @param rpcs_    RPC server
 * @param retval_  return value (OUT)
 * @param func_    RPC function name to call (without rpc_ prefix)
 * @param args_    a set of arguments pased to the function
 */
#define RPC_FUNC_WITH_PTR_RETVAL(rpcs_, retval_, func_, args_...) \
    (retval_) =  rpc_ ## func_(rpcs_, args_)

/**
 * REMOVE IT.
 *
 * Macro to check function 'func_' on returning non NULL value
 *
 * @param rpcs_    RPC server
 * @param retval_  return value (OUT)
 * @param func_    RPC function name to call (without rpc_ prefix)
 */
#define RPC_FUNC_WITH_PTR_RETVAL0(rpcs_, retval_, func_) \
    (retval_) =  rpc_ ## func_(rpcs_)

/**
 * REMOVE IT.
 *
 * Macro to check function 'func_' on returning zero value
 *
 * @param rpcs_    RPC server
 * @param func_    RPC function name to call (without rpc_ prefix)
 * @param args_    a set of arguments pased to the function
 */
#define RPC_FUNC_ZERO_RETVAL(rpcs_, func_, args_...) \
    do {                                             \
        int rc_;                                     \
                                                     \
        (rc_) =  rpc_ ## func_(rpcs_, args_);        \
        if ((rc_) != 0)                              \
        {                                            \
            ERROR(#func_ "() returned unexpected "   \
                  "value %d instead of 0", rc_);     \
            MACRO_ERROR_EXIT;                        \
        }                                            \
    } while (0)


/**
 * Close a socket on a particular RPC server, non-changing variable
 * with socket descriptor.
 *
 * @param rpcs_   RPC server handle
 * @param sockd_  socket descriptor
 */
#define RPC_CLOSE_UNSAFE(rpcs_, sockd_)  rpc_close((rpcs_), (sockd_))

/**
 * Close a socket on a particular RPC server
 *
 * @param rpcs_   RPC server handle
 * @param sockd_  socket descriptor
 *
 * @note @p sockd_ parameter is updated to -1 after successfull
 * completion of the macro.
 */
#define RPC_CLOSE(rpcs_, sockd_) \
    do {                                             \
        rpc_close((rpcs_), (sockd_));                \
        (sockd_) = -1;                               \
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
        if ((rpcs_ != NULL) && (handler_) != NULL)                      \
        {                                                               \
            char *ret_handler_;                                         \
                                                                        \
            (ret_handler_) = rpc_signal((rpcs_), (signum_), (handler_));\
            if ((old_handler_) != NULL)                                 \
            {                                                           \
                if (ret_handler_ == NULL ||                             \
                    strcmp(ret_handler_, (old_handler_)) != 0)          \
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
 * Close a socket in cleanup part of the test
 *
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor to be closed
 */
#define CLEANUP_RPC_CLOSE(rpcs_, sockd_) \
    do {                                                            \
        if ((sockd_) >= 0 && (rpcs_) != NULL)                       \
        {                                                           \
            RPC_AWAIT_IUT_ERROR(rpcs_);                             \
            if (rpc_close((rpcs_), (sockd_)) != 0)                  \
                MACRO_TEST_ERROR;                                   \
        }                                                           \
    } while (0)

/**
 * Close a FTP control socket in cleanup part of the test
 *
 * @param rpcs_     RPC server handle
 * @param sockd_    Socket descriptor to be closed
 */
#define CLEANUP_RPC_FTP_CLOSE(rpcs_, sockd_) \
    do {                                                            \
        if ((sockd_) >= 0 && (rpcs_) != NULL)                       \
        {                                                           \
            RPC_AWAIT_IUT_ERROR(rpcs_);                             \
            if (rpc_ftp_close((rpcs_), (sockd_)) != 0)              \
                MACRO_TEST_ERROR;                                   \
        }                                                           \
    } while (0)

/**
 * Check current value of errno on a particular RPC server against
 * some expected value
 *
 * @param rpcs_        RPC server handle
 * @param exp_errno_   Expected value of errno on the server
 * @param err_msg_     Error message that should be used in case of failure
 * @param args_        Arguments of err_msg_
 *
 * @se In case of failure it jumps to "cleanup" label
 */
#define CHECK_RPC_ERRNO(rpcs_, exp_errno_, err_msg_, args_...) \
    do {                                                                \
        int err_ = RPC_ERRNO(rpcs_);                                    \
                                                                        \
        if (err_ != (int)(exp_errno_))                                  \
        {                                                               \
            if (!!((#args_)[0]))                                        \
            {                                                           \
                ERROR_VERDICT(err_msg_ ": errno is set to %s instead "  \
                              "of %s", args_ + 0, errno_rpc2str(err_),  \
                              errno_rpc2str(exp_errno_));               \
            }                                                           \
            else                                                        \
            {                                                           \
                ERROR_VERDICT(err_msg_ ": errno is set to %s instead "  \
                              "of %s", errno_rpc2str(err_),             \
                              errno_rpc2str(exp_errno_));               \
            }                                                           \
            MACRO_TEST_ERROR;                                           \
            MACRO_ERROR_EXIT;                                           \
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
    do {                                                                 \
        if (tapi_rpc_get_rw_ability(&(answer_), rpcs_, sockd_, timeout_, \
                               "READ") != 0)                             \
            TEST_STOP;                                                   \
    } while (0)

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
    do {                                                                  \
        if (tapi_rpc_get_rw_ability(&(answer_), rpcs_, sockd_, timeout_,  \
                                    "WRITE") != 0)                        \
            TEST_STOP;                                                    \
    } while (0)

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
        te_bool         answer_ = !(should_be_readable_);               \
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
    } while (0)

#endif /* !__TE_TAPI_RPCSOCK_MACROS_H__ */
