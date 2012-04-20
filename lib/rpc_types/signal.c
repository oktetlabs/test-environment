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

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "te_defs.h"
#include "logger_api.h"

#include "te_rpc_defs.h"
#include "tarpc.h"
#include "te_rpc_signal.h"


/** Convert RPC signal number to string */
const char *
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
        RPC2STR(SIGBUS);
        RPC2STR(SIGTRAP);
        default: return "<SIG_FATAL_ERROR>";
    }
}
    
/** Convert RPC signal number to the native one */
int
signum_rpc2h(rpc_signum s)
{
    switch (s)
    {
        case 0: return 0;
#ifdef SIGHUP
        RPC2H(SIGHUP);
#endif
#ifdef SIGINT
        RPC2H(SIGINT);
#endif
#ifdef SIGQUIT
        RPC2H(SIGQUIT);
#endif
#ifdef SIGILL
        RPC2H(SIGILL);
#endif
#ifdef SIGABRT
        RPC2H(SIGABRT);
#endif
#ifdef SIGFPE
        RPC2H(SIGFPE);
#endif
#ifdef SIGKILL
        RPC2H(SIGKILL);
#endif
#ifdef SIGSEGV
        RPC2H(SIGSEGV);
#endif
#ifdef SIGPIPE
        RPC2H(SIGPIPE);
#endif
#ifdef SIGALRM
        RPC2H(SIGALRM);
#endif
#ifdef SIGTERM
        RPC2H(SIGTERM);
#endif
#ifdef SIGUSR1
        RPC2H(SIGUSR1);
#endif
#ifdef SIGUSR2
        RPC2H(SIGUSR2);
#endif
#ifdef SIGCHLD
        RPC2H(SIGCHLD);
#endif
#ifdef SIGCONT
        RPC2H(SIGCONT);
#endif
#ifdef SIGSTOP
        RPC2H(SIGSTOP);
#endif
#ifdef SIGTSTP
        RPC2H(SIGTSTP);
#endif
#ifdef SIGTTIN
        RPC2H(SIGTTIN);
#endif
#ifdef SIGTTOU
        RPC2H(SIGTTOU);
#endif
#ifdef SIGIO
        RPC2H(SIGIO);
#endif
#ifdef SIGBUS
        RPC2H(SIGBUS);
#endif
#ifdef SIGTRAP
        RPC2H(SIGTRAP);
#endif
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
rpc_signum
signum_h2rpc(int s)
{
    switch (s)
    {
        case 0: return 0;
#ifdef SIGHUP
        H2RPC(SIGHUP);
#endif
#ifdef SIGINT
        H2RPC(SIGINT);
#endif
#ifdef SIGQUIT
        H2RPC(SIGQUIT);
#endif
#ifdef SIGILL
        H2RPC(SIGILL);
#endif
#ifdef SIGABRT
        H2RPC(SIGABRT);
#endif
#ifdef SIGFPE
        H2RPC(SIGFPE);
#endif
#ifdef SIGKILL
        H2RPC(SIGKILL);
#endif
#ifdef SIGSEGV
        H2RPC(SIGSEGV);
#endif
#ifdef SIGPIPE
        H2RPC(SIGPIPE);
#endif
#ifdef SIGALRM
        H2RPC(SIGALRM);
#endif
#ifdef SIGTERM
        H2RPC(SIGTERM);
#endif
#ifdef SIGUSR1
        H2RPC(SIGUSR1);
#endif
#ifdef SIGUSR2
        H2RPC(SIGUSR2);
#endif
#ifdef SIGCHLD
        H2RPC(SIGCHLD);
#endif
#ifdef SIGCONT
        H2RPC(SIGCONT);
#endif
#ifdef SIGSTOP
        H2RPC(SIGSTOP);
#endif
#ifdef SIGTSTP
        H2RPC(SIGTSTP);
#endif
#ifdef SIGTTIN
        H2RPC(SIGTTIN);
#endif
#ifdef SIGTTOU
        H2RPC(SIGTTOU);
#endif
#ifdef SIGIO
        H2RPC(SIGIO);
#endif
#ifdef SIGBUS
        H2RPC(SIGBUS);
#endif
#ifdef SIGTRAP
        H2RPC(SIGTRAP);
#endif
        default: return RPC_SIGUNKNOWN;
    }
}

/** Convert RPC signal code to string */
const char *
si_code_rpc2str(rpc_si_code si)
{
    switch (si)
    {
        RPC2STR(SI_ASYNCNL);
        RPC2STR(SI_TKILL);
        RPC2STR(SI_SIGIO);
        RPC2STR(SI_ASYNCIO);
        RPC2STR(SI_MESGQ);
        RPC2STR(SI_TIMER);
        RPC2STR(SI_QUEUE);
        RPC2STR(SI_USER);
        RPC2STR(SI_KERNEL);
        RPC2STR(ILL_ILLOPC);
        RPC2STR(ILL_ILLOPN);
        RPC2STR(ILL_ILLADDR);
        RPC2STR(ILL_ILLTRP);
        RPC2STR(ILL_PRVOPC);
        RPC2STR(ILL_PRVREG);
        RPC2STR(ILL_COPROC);
        RPC2STR(ILL_BADSTK);
        RPC2STR(FPE_INTDIV);
        RPC2STR(FPE_INTOVF);
        RPC2STR(FPE_FLTDIV);
        RPC2STR(FPE_FLTOVF);
        RPC2STR(FPE_FLTUND);
        RPC2STR(FPE_FLTRES);
        RPC2STR(FPE_RLTINV);
        RPC2STR(FPE_FLTSUB);
        RPC2STR(SEGV_MAPERR);
        RPC2STR(SEGV_ACCERR);
        RPC2STR(BUS_ADRALN);
        RPC2STR(BUS_ADRERR);
        RPC2STR(BUS_OBJERR);
        RPC2STR(TRAP_BRKPT);
        RPC2STR(TRAP_TRACE);
        RPC2STR(CLD_EXITED);
        RPC2STR(CLD_KILLED);
        RPC2STR(CLD_DUMPED);
        RPC2STR(CLD_TRAPPED);
        RPC2STR(CLD_STOPPED);
        RPC2STR(CLD_CONTINUED);
        RPC2STR(POLL_IN);
        RPC2STR(POLL_OUT);
        RPC2STR(POLL_MSG);
        RPC2STR(POLL_ERR);
        RPC2STR(POLL_PRI);
        RPC2STR(POLL_HUP);
        RPC2STR(SI_UNKNOWN);
        default: return "<SI_CODE_FATAL_ERROR>";
    }
}
    
/** Convert RPC signal code to the native one */
int
si_code_rpc2h(rpc_si_code si)
{
    switch (si)
    {
        RPC2H_CHECK(SI_ASYNCNL);
        RPC2H_CHECK(SI_TKILL);
        RPC2H_CHECK(SI_SIGIO);
        RPC2H_CHECK(SI_ASYNCIO);
        RPC2H_CHECK(SI_MESGQ);
        RPC2H_CHECK(SI_TIMER);
        RPC2H_CHECK(SI_QUEUE);
        RPC2H_CHECK(SI_USER);
        RPC2H_CHECK(SI_KERNEL);
        RPC2H_CHECK(ILL_ILLOPC);
        RPC2H_CHECK(ILL_ILLOPN);
        RPC2H_CHECK(ILL_ILLADDR);
        RPC2H_CHECK(ILL_ILLTRP);
        RPC2H_CHECK(ILL_PRVOPC);
        RPC2H_CHECK(ILL_PRVREG);
        RPC2H_CHECK(ILL_COPROC);
        RPC2H_CHECK(ILL_BADSTK);
        RPC2H_CHECK(FPE_INTDIV);
        RPC2H_CHECK(FPE_INTOVF);
        RPC2H_CHECK(FPE_FLTDIV);
        RPC2H_CHECK(FPE_FLTOVF);
        RPC2H_CHECK(FPE_FLTUND);
        RPC2H_CHECK(FPE_FLTRES);
        RPC2H_CHECK(FPE_RLTINV);
        RPC2H_CHECK(FPE_FLTSUB);
        RPC2H_CHECK(SEGV_MAPERR);
        RPC2H_CHECK(SEGV_ACCERR);
        RPC2H_CHECK(BUS_ADRALN);
        RPC2H_CHECK(BUS_ADRERR);
        RPC2H_CHECK(BUS_OBJERR);
        RPC2H_CHECK(TRAP_BRKPT);
        RPC2H_CHECK(TRAP_TRACE);
        RPC2H_CHECK(CLD_EXITED);
        RPC2H_CHECK(CLD_KILLED);
        RPC2H_CHECK(CLD_DUMPED);
        RPC2H_CHECK(CLD_TRAPPED);
        RPC2H_CHECK(CLD_STOPPED);
        RPC2H_CHECK(CLD_CONTINUED);
        RPC2H_CHECK(POLL_IN);
        RPC2H_CHECK(POLL_OUT);
        RPC2H_CHECK(POLL_MSG);
        RPC2H_CHECK(POLL_ERR);
        RPC2H_CHECK(POLL_PRI);
        RPC2H_CHECK(POLL_HUP);
        RPC2H_CHECK(SI_UNKNOWN);

        default:
            return 0;
    }
}

/** Convert native signal code to the RPC one */
rpc_si_code
si_code_h2rpc(rpc_signum s, int si)
{
    switch (si)
    {
        H2RPC_CHECK(SI_ASYNCNL = 1);
        H2RPC_CHECK(SI_TKILL);
        H2RPC_CHECK(SI_SIGIO);
        H2RPC_CHECK(SI_ASYNCIO);
        H2RPC_CHECK(SI_MESGQ);
        H2RPC_CHECK(SI_TIMER);
        H2RPC_CHECK(SI_QUEUE);
        H2RPC_CHECK(SI_USER);
        H2RPC_CHECK(SI_KERNEL);
    }

    if (s == RPC_SIGILL)
    {
        switch (si)
        {
            H2RPC_CHECK(ILL_ILLOPC);
            H2RPC_CHECK(ILL_ILLOPN);
            H2RPC_CHECK(ILL_ILLADDR);
            H2RPC_CHECK(ILL_ILLTRP);
            H2RPC_CHECK(ILL_PRVOPC);
            H2RPC_CHECK(ILL_PRVREG);
            H2RPC_CHECK(ILL_COPROC);
            H2RPC_CHECK(ILL_BADSTK);
        }
    }
    else if (s == RPC_SIGFPE)
    {
        switch (si)
        {
            H2RPC_CHECK(FPE_INTDIV);
            H2RPC_CHECK(FPE_INTOVF);
            H2RPC_CHECK(FPE_FLTDIV);
            H2RPC_CHECK(FPE_FLTOVF);
            H2RPC_CHECK(FPE_FLTUND);
            H2RPC_CHECK(FPE_FLTRES);
            H2RPC_CHECK(FPE_RLTINV);
            H2RPC_CHECK(FPE_FLTSUB);
        }
    }
    else if (s == RPC_SIGSEGV)
    {
        switch (si)
        {
            H2RPC_CHECK(SEGV_MAPERR);
            H2RPC_CHECK(SEGV_ACCERR);
        }
    }
    else if (s == RPC_SIGBUS)
    {
        switch (si)
        {
            H2RPC_CHECK(BUS_ADRALN);
            H2RPC_CHECK(BUS_ADRERR);
            H2RPC_CHECK(BUS_OBJERR);
        }
    }
    else if (s == RPC_SIGTRAP)
    {
        switch (si)
        {
            H2RPC_CHECK(TRAP_BRKPT);
            H2RPC_CHECK(TRAP_TRACE);
        }
    }
    else if (s == RPC_SIGCHLD)
    {
        switch (si)
        {
            H2RPC_CHECK(CLD_EXITED);
            H2RPC_CHECK(CLD_KILLED);
            H2RPC_CHECK(CLD_DUMPED);
            H2RPC_CHECK(CLD_TRAPPED);
            H2RPC_CHECK(CLD_STOPPED);
            H2RPC_CHECK(CLD_CONTINUED);
        }
    }
    else
    {
        /*
         *  These signal codes can be used for
         *  any signal defined via @b fcntl(@c F_SETSIG)
         */
        switch (si)
        {
            H2RPC_CHECK(POLL_IN);
            H2RPC_CHECK(POLL_OUT);
            H2RPC_CHECK(POLL_MSG);
            H2RPC_CHECK(POLL_ERR);
            H2RPC_CHECK(POLL_PRI);
            H2RPC_CHECK(POLL_HUP);
        }
    }

    return RPC_SI_UNKNOWN;
}

/** Convert RPC sigevent notification type to string */
const char *
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

#ifndef SIGEV_MAX_SIZE
#define SIGEV_MAX_SIZE  64
#endif

/** Convert RPC signevent notification type native one */
int
sigev_notify_rpc2h(rpc_sigev_notify notify)
{
    switch (notify)
    {
#ifdef SIGEV_SIGNAL
        RPC2H(SIGEV_SIGNAL);
#endif
#ifdef SIGEV_NONE
        RPC2H(SIGEV_NONE);
#endif
#ifdef SIGEV_THREAD
        RPC2H(SIGEV_THREAD);
#endif
        default: return SIGEV_MAX_SIZE;
    }
}

/** Convert native signevent notification type to RPC one */
rpc_sigev_notify
sigev_notify_h2rpc(int notify)
{
    switch (notify)
    {
#ifdef SIGEV_SIGNAL
        H2RPC(SIGEV_SIGNAL);
#endif
#ifdef SIGEV_NONE
        H2RPC(SIGEV_NONE);
#endif
#ifdef SIGEV_THREAD
        H2RPC(SIGEV_THREAD);
#endif
        default: return RPC_SIGEV_UNKNOWN;
    }
}


#define SIG_INVALID     0xFFFFFFFF

/** Convert RPC sigprocmask() how parameter to the native one */
int
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
unsigned int
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
unsigned int
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


/**
 * Convert RPC sigevent structure to string.
 *
 * @param sigevent      RPC sigevent structure
 *
 * @return human-readable string
 */
const char *
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
