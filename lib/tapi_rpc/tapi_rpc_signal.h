/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of functions to work with
 * signals and signal sets.
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

#ifndef __TE_TAPI_RPC_SIGNAL_H__
#define __TE_TAPI_RPC_SIGNAL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "rcf_rpc.h"
#include "tapi_rpcsock_defs.h"


extern char *rpc_signal(rcf_rpc_server *rpcs,
                        rpc_signum signum, const char *handler);

extern int rpc_kill(rcf_rpc_server *rpcs,
                    pid_t pid, rpc_signum signum);


#define RCF_RPC_MAX_FUNC_NAME    64

typedef struct rpc_struct_sigaction {
    char          mm_handler[RCF_RPC_MAX_FUNC_NAME];
    rpc_sigset_t *mm_mask;
    rpc_sa_flags  mm_flags;
    char          mm_restorer[RCF_RPC_MAX_FUNC_NAME];
} rpc_struct_sigaction;

extern int rpc_sigaction(rcf_rpc_server *rpcs,
                         rpc_signum signum,
                         const struct rpc_struct_sigaction *act,
                         struct rpc_struct_sigaction *oldact);


/**
 * Allocate new signal set on RPC server side.
 *
 * @param rpcs      RPC server handle
 *
 * @return Handle of allocated signal set or NULL.
 */
extern rpc_sigset_t *rpc_sigset_new(rcf_rpc_server *rpcs);

/**
 * Free allocated using rpc_sigset_new() signal set.
 *
 * @param rpcs      RPC server handle
 * @param set       signal set handler
 */
extern void rpc_sigset_delete(rcf_rpc_server *rpcs, rpc_sigset_t *set);

/**
 * Get rpcs of set of signals received by special signal handler
 * 'signal_registrar'.
 *
 * @param rpcs      RPC server handle
 *
 * @return Handle of the signal set (unique for RPC server,
 *         i.e. for each thread).
 */
extern rpc_sigset_t *rpc_sigreceived(rcf_rpc_server *rpcs);


extern int rpc_sigprocmask(rcf_rpc_server *rpcs,
                           rpc_sighow how, const rpc_sigset_t *set,
                           rpc_sigset_t *oldset);

extern int rpc_sigemptyset(rcf_rpc_server *rpcs,
                           rpc_sigset_t *set);

extern int rpc_sigfillset(rcf_rpc_server *rpcs,
                          rpc_sigset_t *set);

extern int rpc_sigaddset(rcf_rpc_server *rpcs,
                         rpc_sigset_t *set, rpc_signum signum);

extern int rpc_sigdelset(rcf_rpc_server *rpcs,
                         rpc_sigset_t *set, rpc_signum signum);

extern int rpc_sigismember(rcf_rpc_server *rpcs,
                           const rpc_sigset_t *set, rpc_signum signum);

extern int rpc_sigpending(rcf_rpc_server *rpcs, rpc_sigset_t *set);

extern int rpc_sigsuspend(rcf_rpc_server *rpcs, const rpc_sigset_t *set);


#endif /* !__TE_TAPI_RPC_SIGNAL_H__ */
