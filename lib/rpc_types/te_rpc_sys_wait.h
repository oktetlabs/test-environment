/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/wait.h.
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
 
#ifndef __TE_RPC_SYS_WAIT_H__
#define __TE_RPC_SYS_WAIT_H__

#include "te_rpc_defs.h"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif


/**
 * TA-independent waitpid options.
 */
typedef enum rpc_waitpid_opts {
    RPC_WNOHANG   = 0x1, /**< return immediately if no child has exited */
    RPC_WUNTRACED = 0x2, /**< to also return for children which are 
                              stopped and not traced */
} rpc_waitpid_opts;


/** Convert RPC waitpid options to native options */
static inline int
waitpid_opts_rpc2h(rpc_waitpid_opts opts)
{
    return (!!(opts & RPC_WNOHANG) * WNOHANG) |
           (!!(opts & RPC_WUNTRACED) * WUNTRACED);
}


/**
 * Flags to be used in TA-independent status structure for wait functions.
 */
typedef enum rpc_wait_status_flag {
    RPC_WAIT_STATUS_EXITED,
    RPC_WAIT_STATUS_SIGNALED,
    RPC_WAIT_STATUS_STOPPED,
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
static inline const char *
wait_status_flag_rpc2str(rpc_wait_status_flag flag)
{
    return
        flag == RPC_WAIT_STATUS_EXITED ? "EXITED" :
        flag == RPC_WAIT_STATUS_SIGNALED ? "SIGNALED" :
        flag == RPC_WAIT_STATUS_STOPPED ? "STOPPED" :
        flag == RPC_WAIT_STATUS_CORED ? "CORED" : "UNKNOWN";
}


/** Convert native status value to RPC status structure */
static inline rpc_wait_status
wait_status_h2rpc(int st)
{
    rpc_wait_status ret;

    if (WIFEXITED(st))
    {
        ret.flag = RPC_WAIT_STATUS_EXITED;
        ret.value = WEXITSTATUS(st);
    } 
    else if (WIFSIGNALED(st))
    {
        ret.value = WTERMSIG(st);
#ifdef WCOREDUMP
        if (WCOREDUMP(st))
            ret.flag = RPC_WAIT_STATUS_CORED;
        else
#endif
            ret.flag = RPC_WAIT_STATUS_SIGNALED;
    }
    else if (WIFSTOPPED(st))
    {
        ret.flag = RPC_WAIT_STATUS_STOPPED;
        ret.value = WSTOPSIG(st);
    }

    return ret;
}

#endif /* !__TE_RPC_SYS_WAIT_H__ */
