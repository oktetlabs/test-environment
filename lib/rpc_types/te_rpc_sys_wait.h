/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/wait.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_SYS_WAIT_H__
#define __TE_RPC_SYS_WAIT_H__

#include "te_stdint.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * TA-independent waitpid options.
 */
typedef enum rpc_waitpid_opts {
    RPC_WNOHANG    = 0x1, /**< return immediately if no child has exited */
    RPC_WUNTRACED  = 0x2, /**< also return for children which are 
                               stopped and not traced */
    RPC_WCONTINUED = 0x4, /**< also return for children which are 
                               resumed by SIGCONT */
    RPC_WSYSTEM    = 0x8, /**< Call waitpid() not using ta_waitpid() */
} rpc_waitpid_opts;

/** Convert RPC waitpid options to native options */
extern int waitpid_opts_rpc2h(rpc_waitpid_opts opts);


/**
 * Flags to be used in TA-independent status structure for wait functions.
 */
typedef enum rpc_wait_status_flag {
    RPC_WAIT_STATUS_EXITED,
    RPC_WAIT_STATUS_SIGNALED,
    RPC_WAIT_STATUS_STOPPED,
    RPC_WAIT_STATUS_RESUMED,
    RPC_WAIT_STATUS_CORED,
    RPC_WAIT_STATUS_UNKNOWN
} rpc_wait_status_flag;


/**
 * TA-independent status structure to be used for wait functions.
 */
typedef struct rpc_wait_status {
    rpc_wait_status_flag    flag;
    uint32_t                value;
} rpc_wait_status;

/** Convert status flag to string */
extern const char * wait_status_flag_rpc2str(rpc_wait_status_flag flag);

/** Convert native status value to RPC status structure */
extern rpc_wait_status wait_status_h2rpc(int st);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_WAIT_H__ */
