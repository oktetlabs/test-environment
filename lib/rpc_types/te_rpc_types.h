/** @file
 * @brief Socket API RPC definitions
 *
 * Definition data types used in Socket API RPC.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_TYPES_H__
#define __TE_RPC_TYPES_H__


#define RPC_NULL    0

/** Option length should be calculated automatically */
#define RPC_OPTLEN_AUTO         0xFFFFFFFF


typedef uint32_t rpc_ptr;
typedef rpc_ptr rpc_fd_set_p;
typedef rpc_ptr rpc_sigset_p;
typedef rpc_ptr rpc_aiocb_p;


#include "te_rpc_aio.h"
#include "te_rpc_fcntl.h"
#include "te_rpc_netdb.h"
#include "te_rpc_net_if_arp.h"
#include "te_rpc_net_if.h"
#include "te_rpc_signal.h"
#include "te_rpc_sys_poll.h"
#include "te_rpc_sys_time.h"
#include "te_rpc_wsa.h"
#include "te_rpc_sys_socket.h"
#include "te_rpc_sys_stat.h"
#include "te_rpc_sys_wait.h"

/** 
 * Pattern passed to set_buf_pattern to indicate that the buffer should
 * be filled by random bytes.
 */
#define TAPI_RPC_BUF_RAND       256

#endif /* !__TE_RPC_TYPES_H__ */
