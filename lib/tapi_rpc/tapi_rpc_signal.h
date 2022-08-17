/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of functions to work with
 * signals and signal sets.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_RPC_SIGNAL_H__
#define __TE_TAPI_RPC_SIGNAL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "rcf_rpc.h"
#include "te_rpc_signal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_signal TAPI for signal and signal sets remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Establish an action to be taken when a given signal @b signum occurs
 * on RPC server.
 *
 * @param rpcs      RPC server handle
 * @param signum    Signal whose behavior is controlled
 * @param handler   Signal handler
 *
 * return Pointer to the signal handler function.
 */
extern char *rpc_signal(rcf_rpc_server *rpcs,
                        rpc_signum signum, const char *handler);

/**
 * Establish an action to be taken when a given signal @b signum occurs
 * on RPC server.
 *
 * @param rpcs      RPC server handle
 * @param signum    Signal whose behavior is controlled
 * @param handler   Signal handler
 *
 * return Pointer to the signal handler function.
 */
extern char *rpc_bsd_signal(rcf_rpc_server *rpcs,
                            rpc_signum signum, const char *handler);

/**
 * Establish an action to be taken when a given signal @b signum occurs
 * on RPC server.
 *
 * @param rpcs      RPC server handle
 * @param signum    Signal whose behavior is controlled
 * @param handler   Signal handler
 *
 * return Pointer to the signal handler function.
 */
extern char *rpc_sysv_signal(rcf_rpc_server *rpcs,
                            rpc_signum signum, const char *handler);

/**
 * With help of __sysv_signal() establish an action to be taken
 * when a given signal @b signum occurs on RPC server.
 *
 * @param rpcs      RPC server handle
 * @param signum    Signal whose behavior is controlled
 * @param handler   Signal handler
 *
 * return Pointer to the previous signal handler name
 *        (should be released by caller).
 */
extern char *rpc___sysv_signal(rcf_rpc_server *rpcs,
                               rpc_signum signum, const char *handler);

/**
 * Send signal with number @b signum to process or process group
 * whose pid's @b pid.
 *
 * @param rpcs      RPC server handle
 * @param pid       Process or process group PID
 * @param signum    Number of signal to be sent
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_kill(rcf_rpc_server *rpcs,
                    tarpc_pid_t pid, rpc_signum signum);

/**
 * Send signal with number @b signum to thread whose tid's @b tid.
 *
 * @param rpcs      RPC server handle
 * @param tid       Thread ID
 * @param signum    Number of signal to be sent
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_pthread_kill(rcf_rpc_server *rpcs,
                            tarpc_pthread_t tid, rpc_signum signum);

/**
 * Send signal with number @b signum to thread whose tid is @p tid and
 * tgid is @p tgid. This is linux-specific system call.
 *
 * @param rpcs      RPC server handle
 * @param tgid      Thread GID
 * @param tid       Thread ID
 * @param sig       Number of signal to be sent
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_tgkill(rcf_rpc_server *rpcs, tarpc_int tgid, tarpc_int tid,
                      rpc_signum sig);

/**
 * Wait for termination of process @b pid or process group whose pid's @b
 * pid.
 *
 * @param rpcs      RPC server handle
 * @param pid       Process or process group PID
 * @param status    Status of the process
 * @param options   Options how to wait for process
 *
 * @return PID of the process exited, or zero if none is exited,
 *         or -1 on error.
 */
extern tarpc_pid_t rpc_waitpid(rcf_rpc_server *rpcs,
                               tarpc_pid_t pid, rpc_wait_status *status,
                               rpc_waitpid_opts options);

/** Maximum length of function name */
#define RCF_RPC_MAX_FUNC_NAME    64

/**
 * Store information of the action taken by the process
 * when a specific signal is receipt
 */
typedef struct rpc_struct_sigaction {
    char          mm_handler[RCF_RPC_MAX_FUNC_NAME];  /**< Action handler*/
    uint64_t      mm_restorer;  /**< Opaque restorer value */
    rpc_sigset_p  mm_mask;      /**< Bitmask of signal numbers*/
    rpc_sa_flags  mm_flags;     /**< Flags that modify the signal handling
                                     process */
} rpc_struct_sigaction;

/**
 * Initializer of @ref rpc_struct_sigaction structure.
 */
#define __RPC_STRUCT_SIGACTION_INITIALIZER \
    { .mm_handler = "",     \
      .mm_restorer = 0,     \
      .mm_mask = RPC_NULL,  \
      .mm_flags = 0 }

/**
 * Declare initialized @ref rpc_struct_sigaction structure.
 */
#define DEFINE_RPC_STRUCT_SIGACTION(sa_) \
    rpc_struct_sigaction sa_ = __RPC_STRUCT_SIGACTION_INITIALIZER

/**
 * Initialise @c rpc_struct_sigaction structure.
 *
 * @note Function jumps to cleanup in case of failure.
 *
 * @param rpcs  RPC server handle
 * @param sa    Pointer to a rpc_struct_sigaction structure to be initialised
 */
extern void rpc_sigaction_init(rcf_rpc_server *rpcs,
                               struct rpc_struct_sigaction *sa);

/**
 * Release @c rpc_struct_sigaction structure.
 *
 * @note Function jumps to cleanup in case of failure.
 *
 * @param rpcs  RPC server handle
 * @param sa    Pointer to a rpc_struct_sigaction structure to be released
 */
extern void rpc_sigaction_release(rcf_rpc_server *rpcs,
                                  struct rpc_struct_sigaction *sa);

/**
 * Relese and initialise @c rpc_struct_sigaction structure. The function can
 * be useful for subsequent calls of rpc_sigaction() wich reuse the same
 * structure in argument @c oldact.
 *
 * @note Function jumps to cleanup in case of failure.
 *
 * @param rpcs  RPC server handle
 * @param sa    Pointer to a rpc_struct_sigaction structure to be
 *              reinitialised
 */
static inline void
rpc_sigaction_reinit(rcf_rpc_server *rpcs, struct rpc_struct_sigaction *sa)
{
    rpc_sigaction_release(rpcs, sa);
    rpc_sigaction_init(rpcs, sa);
}

/**
 * Allow the calling process to examin or specify the action to be
 * associated with a given signal.
 *
 * @param rpcs      RPC server handle
 * @param signum    Signal number
 * @param act       Pointer to a rpc_struct_sigaction structure storing
 *                  information of action to be associated with
 *                  the signal or NULL
 * @param oldact    Pointer to previously associated with the signal
 *                  action or NULL. Note! RPC pointer can be allocated to keep
 *                  pointer, so @p oldact (if not NULL) must be released using
 *                  function @c rpc_sigaction_release().
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigaction(rcf_rpc_server *rpcs,
                         rpc_signum signum,
                         const struct rpc_struct_sigaction *act,
                         struct rpc_struct_sigaction *oldact);

/**
 * Set and/or get signal stack context.
 *
 * @param rpcs      RPC server handle
 * @param ss        New aternate signal stack or NULL
 * @param oss       Pointer where to save information
 *                  about current alternate signal stack
 *                  or NULL
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigaltstack(rcf_rpc_server *rpcs, tarpc_stack_t *ss,
                           tarpc_stack_t *oss);

/**
 * Allocate new signal set on RPC server side.
 *
 * @param rpcs      RPC server handle
 *
 * @return Handle of allocated signal set or NULL.
 */
extern rpc_sigset_p rpc_sigset_new(rcf_rpc_server *rpcs);

/**
 * Free allocated using rpc_sigset_new() signal set.
 *
 * @param rpcs      RPC server handle
 * @param set       Signal set handler
 */
extern void rpc_sigset_delete(rcf_rpc_server *rpcs, rpc_sigset_p set);

/**
 * Get rpcs of set of signals received by special signal handler
 * 'signal_registrar'.
 *
 * @param rpcs      RPC server handle
 *
 * @return Handle of the signal set (unique for RPC server,
 *         i.e. for each thread).
 */
extern rpc_sigset_p rpc_sigreceived(rcf_rpc_server *rpcs);

/**
 * Get sininfo_t structure lastly received by special signal
 * handler 'signal_registrar_siginfo'.
 *
 * @param rpcs      RPC server handle
 * @param siginfo   Where to save siginfo_t structure
 *
 * @return 0 or error code
 */
extern int rpc_siginfo_received(rcf_rpc_server *rpcs,
                                tarpc_siginfo_t *siginfo);
/**
 * Compare two signal masks.
 *
 * @param rpcs          RPC server handle
 * @param sig_first     The first signal mask
 * @param sig_second    The second signal mask
 *
 * @return -1, 0 or 1 as a result of comparison
 */
extern int rpc_sigset_cmp(rcf_rpc_server *rpcs,
                          const rpc_sigset_p first_set,
                          const rpc_sigset_p second_set);

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
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigprocmask(rcf_rpc_server *rpcs,
                           rpc_sighow how, const rpc_sigset_p set,
                           rpc_sigset_p oldset);

/**
 * Initialize the signal set pointed by @b set, so that all signals are
 * excluded.
 *
 * @param rpcs   RPC server handle
 * @param set    pointer to signal set.
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigemptyset(rcf_rpc_server *rpcs,
                           rpc_sigset_p set);

/**
 * Initialize the signal set pointed by @b set, so that all signals are
 * included.
 *
 * @param rpcs   RPC server handle.
 * @param set    pointer to signal set.
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigfillset(rcf_rpc_server *rpcs,
                          rpc_sigset_p set);

/**
 * Add a given @b signum signal to a signal set on RPC server side.
 *
 * @param rpcs    RPC server handle
 * @param set     set to which signal is added.
 * @param signum  number of signal to be added.
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigaddset(rcf_rpc_server *rpcs,
                         rpc_sigset_p set, rpc_signum signum);

/**
 * Delete a given @b signum signal from the signal set on RPC server side.
 *
 * @param rpcs    RPC server handle
 * @param set     set from which signal is deleted.
 * @param signum  number of signal to be deleted.
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigdelset(rcf_rpc_server *rpcs,
                         rpc_sigset_p set, rpc_signum signum);

/**
 * Check membership of signal with number @b signum, in the
 * signal set on RPC server side.
 *
 * @param rpcs   RPC server handle
 * @param set    signal set
 * @param signum signal number
 *
 * @return  1 signal is a member of the set
 *          0 signal is not a member of the set
 *         -1 on failure
 */
extern int rpc_sigismember(rcf_rpc_server *rpcs,
                           const rpc_sigset_p set, rpc_signum signum);

/**
 * Return the set of signal blocked by the signal mask on RPC server side
 * and waiting to be delivered.
 *
 * @param rpcs  RPC server handle
 * @param set   pointer to the set of pending signals
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_sigpending(rcf_rpc_server *rpcs, rpc_sigset_p set);

/**
 * Replace the current signal mask on RPC server side by the set pointed
 * by @b set and then suspend the process on server side until signal is
 * received.
 *
 * @param rpcs  RPC server handle
 * @param set   pointer to new set replacing the current one.
 *
 * @return  Always -1 with errno normally set to RPC_EINTR (to be fixed)
 */
extern int rpc_sigsuspend(rcf_rpc_server *rpcs, const rpc_sigset_p set);

/**
 * Kill a child process.
 *
 * @param rpcs  RPC server handle
 * @param pid   PID of the child to be killed
 *
 * @return 0 if child was exited or killed successfully or
 *         -1 there is no such child.
 *
 * @sa rpc_ta_kill_and_wait
 */
extern int rpc_ta_kill_death(rcf_rpc_server *rpcs, tarpc_pid_t pid);

/**
 * Kill a child process and wait for process to change state
 *
 * @param rpcs          RPC server handle
 * @param pid           PID of the child process to be killed
 * @param sig           Signal to be sent to child process
 * @param timeout_s     Time to wait for process to change state
 *
 * @return Status code
 * @retval  0   Successful result
 * @retval -1   Failed to kill the child process, of @p rpcs is @c NULL
 * @retval -2   Timed out to wait for changed state of the child process
 *
 * @sa rpc_ta_kill_death
 */
extern int rpc_ta_kill_and_wait(rcf_rpc_server *rpcs, tarpc_pid_t pid,
                                rpc_signum sig, unsigned int timeout_s);

extern int rpc_siginterrupt(rcf_rpc_server *rpcs, rpc_signum signum,
                            int flag);

/**
 * Delete all of the signals from set of signals received
 * by the @p rpcs process.
 *
 * @param rpcs  RPC server handle
 */
extern void rpc_signal_registrar_cleanup(rcf_rpc_server *rpcs);

/**@} <!-- END te_lib_rpc_signal --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_SIGNAL_H__ */
