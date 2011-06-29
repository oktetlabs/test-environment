/** @file
 * @brief RCF RPC definitions
 *
 * Definition of functions to create/destroy RPC servers on Test
 * Agents and to set/get RPC server context parameters.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_RPC_DEFS_H__
#define __TE_RCF_RPC_DEFS_H__

/** Operations for RPC */
typedef enum {
    RCF_RPC_CALL,       /**< Call non-blocking RPC (if supported) */
    RCF_RPC_IS_DONE,    /**< Check whether non-blocking RPC is done
                             (to be used from rcf_rpc_is_op_done() only) */
    RCF_RPC_WAIT,       /**< Wait until non-blocking RPC is finished */
    RCF_RPC_CALL_WAIT   /**< Call blocking RPC */
} rcf_rpc_op;

#define RCF_RPC_NAME_LEN    32

#define RCF_RPC_MAX_BUF     1048576
#define RCF_RPC_MAX_IOVEC   32
#define RCF_RPC_MAX_CMSGHDR 8

#define RCF_RPC_MAX_MSGHDR  32

/** Check, if errno is RPC errno, not other TE errno */
#define RPC_IS_ERRNO_RPC(_errno) \
    (((_errno) == 0) || (TE_RC_GET_MODULE(_errno) == TE_RPC))



/** Flags for get/create RPC server */

/** Create a subthread of the existing RPC server */
#define RCF_RPC_SERVER_GET_THREAD   0x01
/** Get only existing RPC server */
#define RCF_RPC_SERVER_GET_EXISTING 0x02
/** Reuse existing RPC server if possible without restart */
#define RCF_RPC_SERVER_GET_REUSE    0x04
/**
 * Register in configuration tree existing RPC server process
 * (created without help of configurator previously, for example,
 * to avoid setting flags like RCF_RPC_SERVER_GET_INHERIT
 * automatically by configurator for the process creation).
 */
#define RCF_RPC_SERVER_GET_REGISTER 0x40

/** Next flags may be passed to RPC create_process */
/** exec after fork */
#define RCF_RPC_SERVER_GET_EXEC     0x08
/** Windows-specific: inherit file handles */
#define RCF_RPC_SERVER_GET_INHERIT  0x10
/** Windows-specific: initialize network */
#define RCF_RPC_SERVER_GET_NET_INIT 0x20

#endif /* !__TE_RCF_RPC_DEFS_H__ */
