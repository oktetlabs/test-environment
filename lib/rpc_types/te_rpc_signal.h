/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from signal.h.
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
 
#ifndef __TE_RPC_SIGNAL_H__
#define __TE_RPC_SIGNAL_H__

#include "te_rpc_defs.h"
#include "tarpc.h"


#ifdef __cplusplus
extern "C" {
#endif

/** TA-independent signal constants */
typedef enum rpc_signum {
    RPC_SIGHUP = 1,
    RPC_SIGINT, 
    RPC_SIGQUIT, 
    RPC_SIGILL,  
    RPC_SIGABRT, 
    RPC_SIGFPE,  
    RPC_SIGKILL, 
    RPC_SIGSEGV, 
    RPC_SIGPIPE, 
    RPC_SIGALRM,
    RPC_SIGTERM, 
    RPC_SIGUSR1, 
    RPC_SIGUSR2, 
    RPC_SIGCHLD, 
    RPC_SIGCONT, 
    RPC_SIGSTOP, 
    RPC_SIGTSTP, 
    RPC_SIGTTIN, 
    RPC_SIGTTOU,
    RPC_SIGIO,
    RPC_SIGBUS,
    RPC_SIGTRAP,
    RPC_SIGUNKNOWN,
} rpc_signum;

/** Convert RPC signal number to string */
extern const char *signum_rpc2str(rpc_signum s);
    
/** Convert RPC signal number to the native one */
extern int signum_rpc2h(rpc_signum s);

/** Convert native signal number to the RPC one */
extern rpc_signum signum_h2rpc(int s);

/** TA-independent values of signal code */
typedef enum rpc_si_code {
    RPC_SI_ASYNCNL = 1,
    RPC_SI_TKILL,
    RPC_SI_SIGIO,
    RPC_SI_ASYNCIO,
    RPC_SI_MESGQ,
    RPC_SI_TIMER,
    RPC_SI_QUEUE,
    RPC_SI_USER,
    RPC_SI_KERNEL,
    RPC_ILL_ILLOPC,
    RPC_ILL_ILLOPN,
    RPC_ILL_ILLADDR,
    RPC_ILL_ILLTRP,
    RPC_ILL_PRVOPC,
    RPC_ILL_PRVREG,
    RPC_ILL_COPROC,
    RPC_ILL_BADSTK,
    RPC_FPE_INTDIV,
    RPC_FPE_INTOVF,
    RPC_FPE_FLTDIV,
    RPC_FPE_FLTOVF,
    RPC_FPE_FLTUND,
    RPC_FPE_FLTRES,
    RPC_FPE_RLTINV,
    RPC_FPE_FLTSUB,
    RPC_SEGV_MAPERR,
    RPC_SEGV_ACCERR,
    RPC_BUS_ADRALN,
    RPC_BUS_ADRERR,
    RPC_BUS_OBJERR,
    RPC_TRAP_BRKPT,
    RPC_TRAP_TRACE,
    RPC_CLD_EXITED,
    RPC_CLD_KILLED,
    RPC_CLD_DUMPED,
    RPC_CLD_TRAPPED,
    RPC_CLD_STOPPED,
    RPC_CLD_CONTINUED,
    RPC_POLL_IN,
    RPC_POLL_OUT,
    RPC_POLL_MSG,
    RPC_POLL_ERR,
    RPC_POLL_PRI,
    RPC_POLL_HUP,
    RPC_SI_UNKNOWN,
} rpc_si_code;

/** Convert RPC signal code to string */
extern const char *si_code_rpc2str(rpc_si_code si);
    
/** Convert RPC signal code to the native one */
extern int si_code_rpc2h(rpc_si_code si);

/**
 * Convert native signal code to the RPC one.
 *
 * @param s     Signal number
 * @param si    Native signal code
 *
 * @return RPC signal code
 */
extern rpc_si_code si_code_h2rpc(rpc_signum s, int si);

/** TA-independent sigevent notification types */
typedef enum rpc_sigev_notify {
    RPC_SIGEV_SIGNAL,
    RPC_SIGEV_NONE,
    RPC_SIGEV_THREAD,
    RPC_SIGEV_UNKNOWN
} rpc_sigev_notify; 

/** Convert RPC sigevent notification type to string */
extern const char * sigev_notify_rpc2str(rpc_sigev_notify notify);

/** Convert RPC signevent notification type native one */
extern int sigev_notify_rpc2h(rpc_sigev_notify notify);

/** Convert native signevent notification type to RPC one */
extern rpc_sigev_notify sigev_notify_h2rpc(int notify);


typedef enum rpc_sighow {
    RPC_SIG_BLOCK,
    RPC_SIG_UNBLOCK,
    RPC_SIG_SETMASK
} rpc_sighow;

/**
 * In our RPC model rpc_signal() function always returns string, that 
 * equals to a function name that currently used as the signal handler,
 * and in case there is no signal handler registered rpc_signal()
 * returns "0x00000000", so in case of error function returns NULL.
 */
#define RPC_SIG_ERR     NULL

/** Convert RPC sigprocmask() how parameter to the native one */
extern int sighow_rpc2h(rpc_sighow how);


/**
 * TA-independent sigaction() flags.
 */
typedef enum rpc_sa_flags {
    RPC_SA_NOCLDSTOP  = 1,     /**< Don't receive notification when
                                    child process stop */
    RPC_SA_NOCLDWAIT  = 2,     /**< Don't transform children into zombies
                                    when they terminate */
    RPC_SA_SIGINFO    = 4,     /**< In this case, sa_sigaction() should be
                                    set instead of sa_handler */
    RPC_SA_ONSTACK    = 8,     /**< Call the signal handler on an alternate
                                    signal  stack */
    RPC_SA_RESTART    = 0x10,  /**< Make certain system calls restartable
                                    across signals */
    RPC_SA_NODEFER    = 0x20,  /**< Do not prevent the signal from being
                                    received from within its own signal
                                    handler */
    RPC_SA_RESETHAND  = 0x40,  /**< Restore the signal action to the
                                    default state once the signal handler
                                    has been called */
    RPC_SA_NOMASK     = 0x20,  /**< Alias of RPC_SA_NODEFER */
    RPC_SA_ONESHOT    = 0x40,  /**< Alias of RPC_SA_RESETHAND */
    RPC_SA_STACK      = 8,     /**< Alias of RPC_SA_ONSTACK */
    RPC_SA_RESTORER   = 0x80,  /**< element is obsolete and
                                    should not be used
                                    (but Linux uses yet!) */
    RPC_SA_INTERRUPT  = 0x100, /**< Historical, deprecated */
    RPC_SA_UNKNOWN    = 0x200  /**< Incorrect flag */
} rpc_sa_flags;

#define RPC_SA_FLAGS_ALL \
    (RPC_SA_NOCLDSTOP | RPC_SA_NOCLDWAIT | RPC_SA_ONESHOT | \
     RPC_SA_RESETHAND | RPC_SA_ONSTACK | RPC_SA_RESTART | RPC_SA_NOMASK | \
     RPC_SA_NODEFER | RPC_SA_SIGINFO | RPC_SA_RESTORER | RPC_SA_INTERRUPT)

#define SA_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(SA_NOCLDSTOP), \
    RPC_BIT_MAP_ENTRY(SA_NOCLDWAIT), \
    RPC_BIT_MAP_ENTRY(SA_RESETHAND), \
    RPC_BIT_MAP_ENTRY(SA_ONESHOT),   \
    RPC_BIT_MAP_ENTRY(SA_ONSTACK),   \
    RPC_BIT_MAP_ENTRY(SA_RESTART),   \
    RPC_BIT_MAP_ENTRY(SA_NODEFER),   \
    RPC_BIT_MAP_ENTRY(SA_NOMASK),    \
    RPC_BIT_MAP_ENTRY(SA_SIGINFO),   \
    RPC_BIT_MAP_ENTRY(SA_RESTORER),  \
    RPC_BIT_MAP_ENTRY(SA_INTERRUPT), \
    RPC_BIT_MAP_ENTRY(SA_UNKNOWN)

/**
 * sigaction_flags_rpc2str()
 */
RPCBITMAP2STR(sigaction_flags, SA_FLAGS_MAPPING_LIST)

/** Convert RPC sigaction flags to native flags */
extern unsigned int sigaction_flags_rpc2h(unsigned int flags);

/** Convert native sigaction flags to RPC flags */
extern unsigned int sigaction_flags_h2rpc(unsigned int flags);

/**
 * TA-independent sigaltstack() flags.
 */
typedef enum rpc_ss_flags {
    RPC_SS_ONSTACK  = 1,    /**< The process is executing on the
                                 alternate signal stack */
    RPC_SS_DISABLE  = 2,    /**< The alternate signal stack is
                                 disabled */
    RPC_SS_UNKNOWN  = 4,    /**< Unknown flag */
} rpc_ss_flags;

#define SS_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(SS_ONSTACK), \
    RPC_BIT_MAP_ENTRY(SS_DISABLE)

/**
 * sigaltstack_flags_rpc2str()
 */
RPCBITMAP2STR(sigaltstack_flags, SS_FLAGS_MAPPING_LIST)

/** Convert RPC sigaltstack flags to native flags */
extern unsigned int sigaltstack_flags_rpc2h(rpc_ss_flags flags);

/** Convert native sigaltstack flags to RPC flags */
extern rpc_ss_flags sigaltstack_flags_h2rpc(unsigned int flags);

/**
 * Convert RPC sigevent structure to string.
 *
 * @param sigevent      RPC sigevent structure
 *
 * @return human-readable string
 */
extern const char * tarpc_sigevent2str(const tarpc_sigevent *sigevent);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPCSOCK_DEFS_H__ */
