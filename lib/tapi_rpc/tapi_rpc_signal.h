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

/**
 * Establish an action to be taken when a given signal @b signum occurs
 * on RPC server
 *
 * @param rpcs  RPC server handle
 * @param signum signal whose behavior is controlled 
 * @param handler signal handler
 *
 * return pointer to the signal handler function.
 */
extern char *rpc_signal(rcf_rpc_server *rpcs,
                        rpc_signum signum, const char *handler);

/**
 * Send signal with number @b signum to process or process group 
 * whose pid's @b pid.
 *
 * @param rpcs RPC server handle.
 * @param pid  process or process group pid.
 * @param signum number of signal to be sent.
 *
 * @retval  0 on success 
 *         -1 on failure   
 */
extern int rpc_kill(rcf_rpc_server *rpcs,
                    pid_t pid, rpc_signum signum);

/** Maximum length of function name */
#define RCF_RPC_MAX_FUNC_NAME    64

/** 
 * Store information of the action taken by the process
 * when a specific signal is receipt
 */
typedef struct rpc_struct_sigaction {
    char          mm_handler[RCF_RPC_MAX_FUNC_NAME]; /**< Action handler*/
    rpc_sigset_t *mm_mask;   /**< bitmask of signal numbers*/
    rpc_sa_flags  mm_flags;/**< flags that modify the signal handling 
                                process */
    char     mm_restorer[RCF_RPC_MAX_FUNC_NAME];/**< should not be 
                                                     used. --obsolete */
} rpc_struct_sigaction;


/**
 * Allow the calling process to examin or specify the action to be 
 * associated with a given signal
 *
 * @param rpcs    RPC server handle
 * @param signum  signal number
 * @param act     pointer to a rpc_struct_sigaction structure storing 
 *                information of action to be associated with the signal 
 *                or NULL
 * @param oldact  pointer to previously associated with the signal action 
 *                or NULL
 *
 * @retval 0 on sucess
 *        -1 on failure 
 */
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

/**
 * Examin or change list of currently blocked signal on RPC server side.
 *
 * @param rpcs    RPC server handle
 * @param how     indicates the type of change and can take following 
 *                valye:
 *                 - RPC_SIG_BLOCK   add @b set to currently blocked signal
 *                 - RPC_SIG_UNBLOCK unblock signal specified on set
 *                 - RPC_SIG_SETMASK replace the currently blocked set of 
 *                   signal by the one specified by @b set
 *
 * @param set    pointer to a new set of signals
 * @param oldset pointer to the old set of signals or NUL
 *
 * @retval       0 on success
 *              -1 on failure
 */
extern int rpc_sigprocmask(rcf_rpc_server *rpcs,
                           rpc_sighow how, const rpc_sigset_t *set,
                           rpc_sigset_t *oldset);

/**
 * Initialize the signal set pointed by @b set, so that all signals are 
 * excluded
 *
 * @param rpcs   RPC server handle
 * @param set    pointer to signal set.
 *
 * @retval  0 on success
 *         -1 on failure
 */
extern int rpc_sigemptyset(rcf_rpc_server *rpcs,
                           rpc_sigset_t *set);

/**
 * Initialize the signal set pointed by @b set, so that all signals are 
 * included
 *
 * @param rpcs   RPC server handle.
 * @param set    pointer to signal set.
 *
 * @retval  0 on success
 *         -1 on failure
 */
extern int rpc_sigfillset(rcf_rpc_server *rpcs,
                          rpc_sigset_t *set);
                          
/**
 * Add a given @b signum signal to a signal set on RPC server side.
 *
 * @param rpcs    RPC server handle
 * @param set     set to which signal is added.
 * @param signum  number of signal to be added.
 *
 * @retval  0 on success
 *         -1 on failure
 */
extern int rpc_sigaddset(rcf_rpc_server *rpcs,
                         rpc_sigset_t *set, rpc_signum signum);

/**
 * Delete a given @b signum signal from the signal set on RPC server side.
 *
 * @param rpcs    RPC server handle
 * @param set     set from which signal is deleted.
 * @param signum  number of signal to be deleted.
 *
 * @retval  0 on success
 *         -1 on failure
 */
extern int rpc_sigdelset(rcf_rpc_server *rpcs,
                         rpc_sigset_t *set, rpc_signum signum);

/**
 * Check membership of signal with number @b signum, in the 
 * signal set on RPC server side. 
 *
 * @param rpcs   RPC server handle
 * @param set    signal set
 * @param signum signal number
 *
 * @retval  1 signal is a member of the set
 *          0 signal is not a member of the set
 *         -1 on failure
 */
extern int rpc_sigismember(rcf_rpc_server *rpcs,
                           const rpc_sigset_t *set, rpc_signum signum);

/**
 * Return the set of signal blocked by the signal mask on RPC server side 
 * and waiting to be delivered.
 *
 * @param rpcs  RPC server handle
 * @param set   pointer to the set of pending signals
 *
 * @retval  0 on success
 *         -1 on failure
 */
extern int rpc_sigpending(rcf_rpc_server *rpcs, rpc_sigset_t *set);

/**
 * Replace the current signal mask on RPC server side by the set pointed  
 * by @b set and then suspend the process on server side until signal is
 * received.
 *
 * @param rpcs  RPC server handle
 * @param set   pointer to new set replacing the current one.
 *
 * @retval  alwyas -1 with errno normally set to RPC_EINTR (to be fixed)
 */
extern int rpc_sigsuspend(rcf_rpc_server *rpcs, const rpc_sigset_t *set);


#endif /* !__TE_TAPI_RPC_SIGNAL_H__ */
