/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from signal.h.
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
 
#ifndef __TE_RPC_SIGNAL_H__
#define __TE_RPC_SIGNAL_H__

#include "logger_api.h"

#include "te_rpc_defs.h"
#include "tarpc.h"

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
    RPC_SIGUNKNOWN,
} rpc_signum;
    
#if HAVE_SIGNAL_H
/** Convert RPC signal number to the native one */
static inline int
signum_rpc2h(rpc_signum s)
{
    switch (s)
    {
        case 0: return 0;
        RPC2H(SIGHUP);
        RPC2H(SIGINT);
        RPC2H(SIGQUIT);
        RPC2H(SIGILL);
        RPC2H(SIGABRT);
        RPC2H(SIGFPE);
        RPC2H(SIGKILL);
        RPC2H(SIGSEGV);
        RPC2H(SIGPIPE);
        RPC2H(SIGALRM);
        RPC2H(SIGTERM);
        RPC2H(SIGUSR1);
        RPC2H(SIGUSR2);
        RPC2H(SIGCHLD);
        RPC2H(SIGCONT);
        RPC2H(SIGSTOP);
        RPC2H(SIGTSTP);
        RPC2H(SIGTTIN);
        RPC2H(SIGTTOU);
        RPC2H(SIGIO);
#if defined(SIGUNUSED)
        default: return SIGUNUSED;
#elif defined(_SIG_MAXSIG)
        default: return _SIG_MAXSIG;
#elif defined(_NSIG)
        default: return _NSIG;
#elif defined(NSIG)
        default: return NSIG;
#else
#error There is no way to convert unknown/unsupported/unused signal number!
#endif
    }
}

/** Convert native signal number to the RPC one */
static inline rpc_signum
signum_h2rpc(int s)
{
    switch (s)
    {
        case 0: return 0;
        H2RPC(SIGHUP);
        H2RPC(SIGINT);
        H2RPC(SIGQUIT);
        H2RPC(SIGILL);
        H2RPC(SIGABRT);
        H2RPC(SIGFPE);
        H2RPC(SIGKILL);
        H2RPC(SIGSEGV);
        H2RPC(SIGPIPE);
        H2RPC(SIGALRM);
        H2RPC(SIGTERM);
        H2RPC(SIGUSR1);
        H2RPC(SIGUSR2);
        H2RPC(SIGCHLD);
        H2RPC(SIGCONT);
        H2RPC(SIGSTOP);
        H2RPC(SIGTSTP);
        H2RPC(SIGTTIN);
        H2RPC(SIGTTOU);
        H2RPC(SIGIO);
        default: return RPC_SIGUNKNOWN;
    }
}



#endif

/** Convert RPC signal number to string */
static inline const char *
signum_rpc2str(rpc_signum s)
{
    switch (s)
    {
        case 0: return "0";
        RPC2STR(SIGHUP);
        RPC2STR(SIGINT);
        RPC2STR(SIGQUIT);
        RPC2STR(SIGILL);
        RPC2STR(SIGABRT);
        RPC2STR(SIGFPE);
        RPC2STR(SIGKILL);
        RPC2STR(SIGSEGV);
        RPC2STR(SIGPIPE);
        RPC2STR(SIGALRM);
        RPC2STR(SIGTERM);
        RPC2STR(SIGUSR1);
        RPC2STR(SIGUSR2);
        RPC2STR(SIGCHLD);
        RPC2STR(SIGCONT);
        RPC2STR(SIGSTOP);
        RPC2STR(SIGTSTP);
        RPC2STR(SIGTTIN);
        RPC2STR(SIGTTOU);
        RPC2STR(SIGIO);
        default: return "<SIG_FATAL_ERROR>";
    }
}

/** TA-independent sigevent notification types */
typedef enum rpc_sigev_notify {
    RPC_SIGEV_SIGNAL,
    RPC_SIGEV_NONE,
    RPC_SIGEV_THREAD,
    RPC_SIGEV_UNKNOWN
} rpc_sigev_notify; 

#ifdef HAVE_SIGNAL_H

#ifndef SIGEV_MAX_SIZE
#define SIGEV_MAX_SIZE  64
#endif

/** Convert RPC signevent notification type native one */
static inline int
sigev_notify_rpc2h(rpc_sigev_notify notify)
{
    switch (notify)
    {
        RPC2H(SIGEV_SIGNAL);
        RPC2H(SIGEV_NONE);
#ifdef SIGEV_THREAD
        RPC2H(SIGEV_THREAD);
#endif
        default: return SIGEV_MAX_SIZE;
    }
}

/** Convert native signevent notification type to RPC one */
static inline rpc_sigev_notify
sigev_notify_h2rpc(int notify)
{
    switch (notify)
    {
        H2RPC(SIGEV_SIGNAL);
        H2RPC(SIGEV_NONE);
#ifdef SIGEV_THREAD
        H2RPC(SIGEV_THREAD);
#endif
        default: return RPC_SIGEV_UNKNOWN;
    }
}
#endif

/** Convert RPC sigevent notification type to string */
static inline const char *
sigev_notify_rpc2str(rpc_sigev_notify notify)
{
    switch (notify)
    {
        RPC2STR(SIGEV_SIGNAL);
        RPC2STR(SIGEV_NONE);
        RPC2STR(SIGEV_THREAD);
        default: return "SIGEV_UNKNOWN";
    }
}


typedef enum rpc_sighow {
    RPC_SIG_BLOCK,
    RPC_SIG_UNBLOCK,
    RPC_SIG_SETMASK
} rpc_sighow;

#define SIG_INVALID     0xFFFFFFFF

/**
 * In our RPC model rpc_signal() function always returns string, that 
 * equals to a function name that currently used as the signal handler,
 * and in case there is no signal handler registered rpc_signal()
 * returns "0x00000000", so in case of error function returns NULL.
 */
#define RPC_SIG_ERR     NULL

/** Convert RPC sigprocmask() how parameter to the native one */
static inline int
sighow_rpc2h(rpc_sighow how)
{
    switch (how)
    {
#ifdef SIG_BLOCK    
        RPC2H(SIG_BLOCK);
#endif        
#ifdef SIG_UNBLOCK    
        RPC2H(SIG_UNBLOCK);
#endif
#ifdef SIG_SETMASK
        RPC2H(SIG_SETMASK);
#endif        
        default: return SIG_INVALID;
    }
}


/**
 * TA-independent sigaction() flags.
 */
typedef enum rpc_sa_flags {
    RPC_SA_NOCLDSTOP  = 1,     /**< Don't receive notification when
                                    child process stop */
    RPC_SA_ONESHOT    = 2,     /* with alias */
    RPC_SA_RESETHAND  = 2,     /**< Restore the signal action to the
                                    default state once the signal handler
                                    has been called */
    RPC_SA_ONSTACK    = 4,     /**< Call the signal handler on an alternate
                                    signal  stack */
    RPC_SA_RESTART    = 8,     /**< Make certain system calls restartable
                                    across signals */
    RPC_SA_NOMASK     = 0x10,  /* with alias */
    RPC_SA_NODEFER    = 0x10,  /**< Do not prevent the signal from being
                                    received from within its own signal
                                    handler */
    RPC_SA_SIGINFO    = 0x20,  /**< In this case, sa_sigaction() should be
                                    set instead of sa_handler */
    RPC_SA_RESTORER   = 0x40,   /** < element is obsolete and
                                     should not be used
                                     (but Linux uses yet!) */
    RPC_SA_UNKNOWN    = 0x80   /**< Incorrect flag */
} rpc_sa_flags;

#define RPC_SA_FLAGS_ALL \
    (RPC_SA_NOCLDSTOP | RPC_SA_ONESHOT | RPC_SA_RESETHAND |     \
     RPC_SA_ONSTACK | RPC_SA_RESTART | RPC_SA_NOMASK |          \
     RPC_SA_NODEFER | RPC_SA_SIGINFO | RPC_SA_RESTORER)

#define SA_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(SA_NOCLDSTOP), \
    RPC_BIT_MAP_ENTRY(SA_ONESHOT),   \
    RPC_BIT_MAP_ENTRY(SA_RESETHAND), \
    RPC_BIT_MAP_ENTRY(SA_ONSTACK),   \
    RPC_BIT_MAP_ENTRY(SA_RESTART),   \
    RPC_BIT_MAP_ENTRY(SA_NOMASK),    \
    RPC_BIT_MAP_ENTRY(SA_NODEFER),   \
    RPC_BIT_MAP_ENTRY(SA_SIGINFO),   \
    RPC_BIT_MAP_ENTRY(SA_RESTORER),  \
    RPC_BIT_MAP_ENTRY(SA_UNKNOWN)


#if HAVE_SIGNAL_H

#include <signal.h>

#if HAVE_STRUCT_SIGACTION_SA_RESTORER
#ifndef SA_RESTORER
#define SA_RESTORER       0x04000000
#define HAVE_SA_RESTORER  1
#endif
#else
#define HAVE_SA_RESTORER  0
#define SA_RESTORER       0
#endif

#ifdef SA_NOCLDSTOP
#define HAVE_SA_NOCLDSTOP 1
#else
#define HAVE_SA_NOCLDSTOP 0
#define SA_NOCLDSTOP      0
#endif

#ifdef SA_ONESHOT
#define HAVE_SA_ONESHOT   1
#else
#define HAVE_SA_ONESHOT   0
#define SA_ONESHOT        0
#endif

#ifdef SA_RESETHAND
#define HAVE_SA_RESETHAND 1
#else
#define HAVE_SA_RESETHAND 0
#define SA_RESETHAND      0
#endif

#ifdef SA_ONSTACK
#define HAVE_SA_ONSTACK   1
#else
#define HAVE_SA_ONSTACK   0
#define SA_ONSTACK        0
#endif

#ifdef SA_RESTART
#define HAVE_SA_RESTART   1
#else
#define HAVE_SA_RESTART   0
#define SA_RESTART        0
#endif

#ifdef SA_NOMASK
#define HAVE_SA_NOMASK    1
#else
#define HAVE_SA_NOMASK    0
#define SA_NOMASK         0
#endif

#ifdef SA_NODEFER
#define HAVE_SA_NODEFER   1
#else
#define HAVE_SA_NODEFER   0
#define SA_NODEFER        0
#endif

#ifdef SA_SIGINFO
#define HAVE_SA_SIGINFO   1
#else
#define HAVE_SA_SIGINFO   0
#define SA_SIGINFO        0
#endif


#define SA_FLAGS_ALL \
    (SA_NOCLDSTOP | SA_ONESHOT | SA_RESETHAND |  \
     SA_ONSTACK | SA_RESTART | SA_NOMASK |       \
     SA_NODEFER | SA_SIGINFO | SA_RESTORER)


#define SA_FLAGS_UNKNOWN  0xFFFFFFFF


/** Convert RPC sigaction flags to native flags */
static inline int
sigaction_flags_rpc2h(unsigned int flags)
{
    if ((flags & ~RPC_SA_FLAGS_ALL) != 0)
        return SA_FLAGS_UNKNOWN;
    return
#ifdef SA_NOCLDSTOP
        (!!(flags & RPC_SA_NOCLDSTOP) * SA_NOCLDSTOP) |
#endif
#ifdef SA_ONESHOT
        (!!(flags & RPC_SA_ONESHOT) * SA_ONESHOT) |
#endif
#ifdef SA_RESETHAND
        (!!(flags & RPC_SA_RESETHAND) * SA_RESETHAND) |
#endif
#ifdef SA_ONSTACK
        (!!(flags & RPC_SA_ONSTACK) * SA_ONSTACK) |
#endif
#ifdef SA_RESTART
        (!!(flags & RPC_SA_RESTART) * SA_RESTART) |
#endif
#ifdef SA_NOMASK
        (!!(flags & RPC_SA_NOMASK) * SA_NOMASK) |
#endif
#ifdef SA_NODEFER
        (!!(flags & RPC_SA_NODEFER) * SA_NODEFER) |
#endif
#ifdef SA_SIGINFO
        (!!(flags & RPC_SA_SIGINFO) * SA_SIGINFO) |
#endif
#ifdef SA_RESTORER
        (!!(flags & RPC_SA_RESTORER) * SA_RESTORER) |
#endif
        0;
}

/** Convert native sigaction flags to RPC flags */
static inline unsigned int
sigaction_flags_h2rpc(unsigned int flags)
{
    return
#ifdef SA_NOCLDSTOP
        (!!(flags & SA_NOCLDSTOP) * RPC_SA_NOCLDSTOP) |
#endif
#ifdef SA_ONESHOT
        (!!(flags & SA_ONESHOT) * RPC_SA_ONESHOT) |
#endif
#ifdef SA_RESETHAND
        (!!(flags & SA_RESETHAND) * RPC_SA_RESETHAND) |
#endif
#ifdef SA_ONSTACK
        (!!(flags & SA_ONSTACK) * RPC_SA_ONSTACK) |
#endif
#ifdef SA_RESTART
        (!!(flags & SA_RESTART) * RPC_SA_RESTART) |
#endif
#ifdef SA_NOMASK
        (!!(flags & SA_NOMASK) * RPC_SA_NOMASK) |
#endif
#ifdef SA_NODEFER
        (!!(flags & SA_NODEFER) * RPC_SA_NODEFER) |
#endif
#ifdef SA_SIGINFO
        (!!(flags & SA_SIGINFO) * RPC_SA_SIGINFO) |
#endif
#ifdef SA_RESTORER
        (!!(flags & SA_RESTORER) * RPC_SA_RESTORER) |
#endif
        (!!(flags & ~SA_FLAGS_ALL) * RPC_SA_UNKNOWN);
}

#endif

/**
 * Convert RPC sigevent structure to string.
 *
 * @param sigevent      RPC sigevent structure
 *
 * @return human-readable string
 */
static inline const char *
tarpc_sigevent2str(const tarpc_sigevent *sigevent)
{
    static char buf[256];
    
    if (sigevent == NULL)
        return "NULL";
        
    TE_SPRINTF(buf, "{ notify %s signo %s sigval %u function %s }",
               sigev_notify_rpc2str(sigevent->notify),
               signum_rpc2str(sigevent->signo),
               (unsigned)sigevent->value.tarpc_sigval_u.sival_int,
               sigevent->function == NULL ? "NULL" : sigevent->function);
               
    return buf;               
}

#endif /* !__TE_TAPI_RPCSOCK_DEFS_H__ */
