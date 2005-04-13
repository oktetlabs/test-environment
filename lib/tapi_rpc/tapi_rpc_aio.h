/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of asynchronous input/output
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

#ifndef __TE_TAPI_RPC_AIO_H__
#define __TE_TAPI_RPC_AIO_H__

#include "rcf_rpc.h"
#include "tapi_rpcsock_defs.h"


/**
 * Asynchronous read test procedure.
 *
 * @param rpcs          RPC server
 * @param s             a socket to be used for receiving
 * @param signum        signal to be used as notification event
 * @param timeout       timeout for blocking on select() in seconds
 * @param buf           buffer for received data
 * @param buflen        buffer length to be passed to the read
 * @param rlen          real buffer length
 * @param diag          location for diagnostic message
 * @param diag_len      maximum length of the diagnostic message
 *
 * @return Number of received bytes or -1 in the case of failure
 */
extern int rpc_aio_read_test(rcf_rpc_server *rpcs,
                             int s, rpc_signum signum, int timeout,
                             char *buf, int buflen, int rlen,
                             char *diag, int diag_len);

/**
 * Asynchronous error processing test procedure.
 *
 * @param rpcs          RPC server
 * @param diag          location for diagnostic message
 * @param diag_len      maximum length of the diagnostic message
 *
 * @return Number of received bytes or -1 in the case of failure
 */
extern int rpc_aio_error_test(rcf_rpc_server *rpcs,
                              char *diag, int diag_len);

/**
 * Asynchronous write test procedure.
 *
 * @param rpcs          RPC server
 * @param s             a socket to be used for receiving
 * @param signum        signal to be used as notification event
 * @param buf           buffer for data to be sent
 * @param buflen        data length
 * @param diag          location for diagnostic message
 * @param diag_len      maximum length of the diagnostic message
 *
 * @return Number of sent bytes or -1 in the case of failure
 */
extern int rpc_aio_write_test(rcf_rpc_server *rpcs,
                              int s, rpc_signum signum,
                              char *buf, int buflen,
                              char *diag, int diag_len);

/**
 * Suspending on asynchronous events test procedure.
 *
 * @param rpcs          RPC server
 * @param s             a socket to be user for receiving
 * @param s_aux         dummy socket
 * @param signum        signal to be used as notification event
 * @param timeout       timeout for blocking on ai_suspend() in seconds
 * @param buf           buffer for received data
 * @param buflen        buffer length to be passed to the read
 * @param diag          location for diagnostic message
 * @param diag_len      maximum length of the diagnostic message
 *
 * @return Number of received bytes or -1 in the case of failure
 */
extern int rpc_aio_suspend_test(rcf_rpc_server *rpcs,
                                int s, int s_aux, rpc_signum signum,
                                int timeout, char *buf, int buflen,
                                char *diag, int diag_len);

#endif /* !__TE_TAPI_RPC_AIO_H__ */
