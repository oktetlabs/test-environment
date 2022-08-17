/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of signals-related API
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "log_bufs.h"
#include "te_str.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_misc.h"
#include "tapi_test_log.h"

/**
 * Check whether value returned by signal() and related calls
 * indicates error.
 *
 * @param retval_   Returned value.
 */
#define SIGNAL_RETVAL_IS_ERR(retval_) \
    (retval_ == NULL || strcmp(retval_, "SIG_ERR") == 0)

char *
rpc_signal(rcf_rpc_server *rpcs,
           rpc_signum signum, const char *handler)
{
    tarpc_signal_in  in;
    tarpc_signal_out out;

    char *copy = NULL;

    char *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(signal, NULL);
    }

    if ((copy = strdup(handler != NULL ? handler : "")) == NULL)
    {
        ERROR("Out of memory");
        rpcs->_errno = TE_ENOMEM;
        RETVAL_PTR(signal, NULL);
    }

    in.signum = signum;
    in.handler = copy;

    rcf_rpc_call(rpcs, "signal", &in, &out);

    /* Yes, I know that it's memory leak, but what do you propose?! */
    if (RPC_IS_CALL_OK(rpcs))
    {
        res = out.handler;
        out.handler = NULL;
    }
    free(copy);

    CHECK_RETVAL_VAR_ERR_COND(signal, res, FALSE,
                              res, SIGNAL_RETVAL_IS_ERR(res));

    TAPI_RPC_LOG(rpcs, signal, "%s, %s", "%s", signum_rpc2str(signum),
                 (handler != NULL) ? handler : "(null)",
                 res != NULL ? res : "(null)");
    TAPI_RPC_OUT(signal, SIGNAL_RETVAL_IS_ERR(res));
    return res;
}

char *
rpc_bsd_signal(rcf_rpc_server *rpcs,
           rpc_signum signum, const char *handler)
{
    tarpc_bsd_signal_in  in;
    tarpc_bsd_signal_out out;

    char *copy = NULL;

    char *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(bsd_signal, NULL);
    }

    if ((copy = strdup(handler != NULL ? handler : "")) == NULL)
    {
        ERROR("Out of memory");
        rpcs->_errno = TE_ENOMEM;
        RETVAL_PTR(bsd_signal, NULL);
    }

    in.signum = signum;
    in.handler = copy;

    rcf_rpc_call(rpcs, "bsd_signal", &in, &out);

    /* Yes, I know that it's memory leak, but what do you propose?! */
    if (RPC_IS_CALL_OK(rpcs))
    {
        res = out.handler;
        out.handler = NULL;
    }
    free(copy);

    CHECK_RETVAL_VAR_ERR_COND(bsd_signal, res, FALSE,
                              res, SIGNAL_RETVAL_IS_ERR(res));

    TAPI_RPC_LOG(rpcs, bsd_signal, "%s, %s", "%s", signum_rpc2str(signum),
                 (handler != NULL) ? handler : "(null)",
                 res != NULL ? res : "(null)");
    TAPI_RPC_OUT(bsd_signal, SIGNAL_RETVAL_IS_ERR(res));
    return res;
}

int
rpc_siginterrupt(rcf_rpc_server *rpcs, rpc_signum signum, int flag)
{
    tarpc_siginterrupt_in  in;
    tarpc_siginterrupt_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(siginterrupt, -1);
    }

    in.signum = signum;
    in.flag = flag;
    rcf_rpc_call(rpcs, "siginterrupt", &in, &out);

    TAPI_RPC_LOG(rpcs, siginterrupt, "%s, %d", "%d",
                 signum_rpc2str(signum), flag, out.retval);
    RETVAL_INT(siginterrupt, out.retval);
}

char *
rpc_sysv_signal(rcf_rpc_server *rpcs,
           rpc_signum signum, const char *handler)
{
    tarpc_sysv_signal_in  in;
    tarpc_sysv_signal_out out;

    char *copy = NULL;

    char *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(sysv_signal, NULL);
    }

    if ((copy = strdup(handler != NULL ? handler : "")) == NULL)
    {
        ERROR("Out of memory");
        rpcs->_errno = TE_ENOMEM;
        RETVAL_PTR(sysv_signal, NULL);
    }

    in.signum = signum;
    in.handler = copy;

    rcf_rpc_call(rpcs, "sysv_signal", &in, &out);

    /* Yes, I know that it's memory leak, but what do you propose?! */
    if (RPC_IS_CALL_OK(rpcs))
    {
        res = out.handler;
        out.handler = NULL;
    }
    free(copy);

    CHECK_RETVAL_VAR_ERR_COND(sysv_signal, res, FALSE,
                              res, SIGNAL_RETVAL_IS_ERR(res));

    TAPI_RPC_LOG(rpcs, sysv_signal, "%s, %s", "%s", signum_rpc2str(signum),
                 (handler != NULL) ? handler : "(null)",
                 res != NULL ? res : "(null)");
    TAPI_RPC_OUT(sysv_signal, SIGNAL_RETVAL_IS_ERR(res));
    return res;
}

char *
rpc___sysv_signal(rcf_rpc_server *rpcs,
                  rpc_signum signum, const char *handler)
{
    tarpc___sysv_signal_in  in;
    tarpc___sysv_signal_out out;

    char *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(__sysv_signal, NULL);
    }

    in.signum = signum;
    in.handler = strdup(handler == NULL ? "" : handler);

    if (in.handler == NULL)
    {
        ERROR("%s(): out of memory", __FUNCTION__);
        rpcs->_errno = TE_ENOMEM;
        RETVAL_PTR(__sysv_signal, NULL);
    }

    rcf_rpc_call(rpcs, "__sysv_signal", &in, &out);
    free(in.handler);

    if (RPC_IS_CALL_OK(rpcs))
    {
        res = out.handler;
        out.handler = NULL;
    }

    CHECK_RETVAL_VAR_ERR_COND(__sysv_signal, res, FALSE,
                              res, SIGNAL_RETVAL_IS_ERR(res));

    TAPI_RPC_LOG(rpcs, __sysv_signal, "%s, %s", "%s", signum_rpc2str(signum),
                 (handler != NULL ? handler : "(null)"),
                 (res != NULL ? res : "(null)"));
    TAPI_RPC_OUT(__sysv_signal, SIGNAL_RETVAL_IS_ERR(res));
    return res;
}

int
rpc_kill(rcf_rpc_server *rpcs, tarpc_pid_t pid, rpc_signum signum)
{
    tarpc_kill_in  in;
    tarpc_kill_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(kill, -1);
    }

    in.signum = signum;
    in.pid = pid;

    rcf_rpc_call(rpcs, "kill", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(kill, out.retval);
    TAPI_RPC_LOG(rpcs, kill, "%d, %s", "%d",
                 pid, signum_rpc2str(signum), out.retval);
    RETVAL_INT(kill, out.retval);
}

int
rpc_pthread_kill(rcf_rpc_server *rpcs, tarpc_pthread_t tid,
                 rpc_signum signum)
{
    tarpc_pthread_kill_in  in;
    tarpc_pthread_kill_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pthread_kill, -1);
    }

    if (signum != RPC_SIGUSR1 && signum != RPC_SIGUSR2)
        WARN("%s(): sending to thread signal other than "
             "RPC_SIGUSR1 and RPC_SIGUSR2 can be dangerous!",
             __FUNCTION__);

    in.signum = signum;
    in.tid = tid;

    rcf_rpc_call(rpcs, "pthread_kill", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pthread_kill, out.retval);
    TAPI_RPC_LOG(rpcs, pthread_kill, "%llu, %s", "%d",
                 (unsigned long long int)tid, signum_rpc2str(signum),
                 out.retval);
    RETVAL_INT(pthread_kill, out.retval);
}

int
rpc_tgkill(rcf_rpc_server *rpcs, tarpc_int tgid,
           tarpc_int tid, rpc_signum sig)
{
    tarpc_call_tgkill_in  in;
    tarpc_call_tgkill_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(call_tgkill, -1);
    }

    if (sig != RPC_SIGUSR1 && sig != RPC_SIGUSR2)
        WARN("%s(): sending to thread signal other than "
             "RPC_SIGUSR1 and RPC_SIGUSR2 can be dangerous!",
             __FUNCTION__);

    in.sig = sig;
    in.tgid = tgid;
    in.tid = tid;

    rcf_rpc_call(rpcs, "call_tgkill", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(call_tgkill, out.retval);
    TAPI_RPC_LOG(rpcs, call_tgkill, "%d, %d, %s", "%d",
                 tgid, tid, signum_rpc2str(sig),
                 out.retval);
    RETVAL_INT(call_tgkill, out.retval);
}

tarpc_pid_t
rpc_waitpid(rcf_rpc_server *rpcs, tarpc_pid_t pid, rpc_wait_status *status,
            rpc_waitpid_opts options)
{
    tarpc_waitpid_in    in;
    tarpc_waitpid_out   out;
    rpc_wait_status     stat;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(waitpid, -1);
    }

    in.pid = pid;
    in.options = (rpc_waitpid_opts)options;

    rcf_rpc_call(rpcs, "waitpid", &in, &out);
    memset(&stat, 0, sizeof(stat));
    if (out.pid > 0)
    {
        stat.value = out.status_value;
        stat.flag = out.status_flag;
    }
    else
        stat.flag = RPC_WAIT_STATUS_UNKNOWN;

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(waitpid, out.pid);
    TAPI_RPC_LOG(rpcs, waitpid, "%d, %p, 0x%x", "%d status %s 0x%x",
                 pid, status, options, out.pid,
                 wait_status_flag_rpc2str(stat.flag), stat.value);
    if (out.pid > 0 && (stat.flag != RPC_WAIT_STATUS_EXITED ||
                        stat.value != 0))
        INFO("waitpid() returned non-zero status");

    if (status != NULL)
        *status = stat;
    RETVAL_INT_CHECK_WAIT_STATUS(waitpid, out.pid, stat);
}

int
rpc_ta_kill_death(rcf_rpc_server *rpcs, tarpc_pid_t pid)
{
    tarpc_ta_kill_death_in  in;
    tarpc_ta_kill_death_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ta_kill_death, -1);
    }

    in.pid = pid;

    rcf_rpc_call(rpcs, "ta_kill_death", &in, &out);

    /* This function should not check errno */
    out.common.errno_changed = FALSE;
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(ta_kill_death, out.retval);
    TAPI_RPC_LOG(rpcs, ta_kill_death, "%d", "%d", pid, out.retval);
    RETVAL_INT(ta_kill_death, out.retval);
}

int
rpc_ta_kill_and_wait(rcf_rpc_server *rpcs, tarpc_pid_t pid, rpc_signum sig,
                     unsigned int timeout_s)
{
    tarpc_ta_kill_and_wait_in  in;
    tarpc_ta_kill_and_wait_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.pid = pid;
    in.sig = sig;
    in.timeout = timeout_s;
    out.retval = -1;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = rpcs->def_timeout + TE_SEC2MS(timeout_s);

    rcf_rpc_call(rpcs, "ta_kill_and_wait", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEGATIVE(ta_kill_and_wait, out.retval);
    TAPI_RPC_LOG(rpcs, ta_kill_and_wait, "%d, %s, %u", "%d",
                 pid, signum_rpc2str(sig), timeout_s, out.retval);
    RETVAL_INT(ta_kill_and_wait, out.retval);
}

rpc_sigset_p
rpc_sigset_new(rcf_rpc_server *rpcs)
{
    tarpc_sigset_new_in  in;
    tarpc_sigset_new_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(sigset_new, RPC_NULL);
    }

    rcf_rpc_call(rpcs, "sigset_new", &in, &out);

    TAPI_RPC_LOG(rpcs, sigset_new, "", "0x%x", (unsigned)out.set);
    RETVAL_RPC_PTR(sigset_new, out.set);
}

void
rpc_sigset_delete(rcf_rpc_server *rpcs, rpc_sigset_p set)
{
    tarpc_sigset_delete_in  in;
    tarpc_sigset_delete_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(sigset_delete);
    }

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigset_delete", &in, &out);

    TAPI_RPC_LOG(rpcs, sigset_delete, "0x%x", "", (unsigned)set);
    RETVAL_VOID(sigset_delete);
}


int
rpc_sigset_cmp(rcf_rpc_server *rpcs,
               const rpc_sigset_p first_set,
               const rpc_sigset_p second_set)
{
    tarpc_sigset_cmp_in  in;
    tarpc_sigset_cmp_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigset_cmp, -2);
    }

    in.first_set = (tarpc_sigset_t)first_set;
    in.second_set = (tarpc_sigset_t)second_set;

    rcf_rpc_call(rpcs, "sigset_cmp", &in, &out);

    CHECK_RETVAL_VAR(sigset_cmp, out.retval,
                     !(out.retval >= -1 && out.retval <= 1), -1);
    TAPI_RPC_LOG(rpcs, sigset_cmp, "0x%x, 0x%x", "%d",
                 (unsigned)first_set, (unsigned)second_set, out.retval);
    TAPI_RPC_OUT(sigset_cmp, FALSE);
    return (int)out.retval;
}

int
rpc_sigprocmask(rcf_rpc_server *rpcs,
                rpc_sighow how, const rpc_sigset_p set,
                rpc_sigset_p oldset)
{
    tarpc_sigprocmask_in  in;
    tarpc_sigprocmask_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigprocmask, -1);
    }

    in.set = (tarpc_sigset_t)set;
    in.oldset = (tarpc_sigset_t)oldset;
    in.how = how;

    rcf_rpc_call(rpcs, "sigprocmask", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigprocmask, out.retval);
    TAPI_RPC_LOG(rpcs, sigprocmask, "%d, 0x%x, 0x%x", "%d",
                 how, (unsigned)set, (unsigned)oldset, out.retval);
    RETVAL_INT(sigprocmask, out.retval);
}


int
rpc_sigemptyset(rcf_rpc_server *rpcs, rpc_sigset_p set)
{
    tarpc_sigemptyset_in  in;
    tarpc_sigemptyset_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigemptyset, -1);
    }

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigemptyset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigemptyset, out.retval);
    TAPI_RPC_LOG(rpcs, sigemptyset, "0x%x", "%d",
                 (unsigned)set, out.retval);
    RETVAL_INT(sigemptyset, out.retval);
}

int
rpc_sigpending(rcf_rpc_server *rpcs, rpc_sigset_p set)
{
    tarpc_sigpending_in  in;
    tarpc_sigpending_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigpending, -1);
    }

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigpending", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigpending, out.retval);
    TAPI_RPC_LOG(rpcs, sigpending, "0x%x", "%d",
                 (unsigned)set, out.retval);
    RETVAL_INT(sigpending, out.retval);
}

int
rpc_sigsuspend(rcf_rpc_server *rpcs, const rpc_sigset_p set)
{
    tarpc_sigsuspend_in  in;
    tarpc_sigsuspend_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigsuspend, -1);
    }

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigsuspend", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigsuspend, out.retval);
    TAPI_RPC_LOG(rpcs, sigsuspend, "0x%x", "%d",
                 (unsigned)set, out.retval);
    RETVAL_INT(sigsuspend, out.retval);
}

rpc_sigset_p
rpc_sigreceived(rcf_rpc_server *rpcs)
{
    tarpc_sigreceived_in  in;
    tarpc_sigreceived_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(sigreceived, RPC_NULL);
    }

    rcf_rpc_call(rpcs, "sigreceived", &in, &out);

    TAPI_RPC_LOG(rpcs, sigreceived, "", "0x%x", (unsigned)out.set);
    RETVAL_RPC_PTR(sigreceived, out.set);
}

int
rpc_siginfo_received(rcf_rpc_server *rpcs, tarpc_siginfo_t *siginfo)
{
#define PRINT_SI_FIELD(_field) \
    do {                                                                 \
        if (strcmp(#_field, "signo") == 0)                               \
            te_log_buf_append(str, "sig_signo: %s ",                     \
                              signum_rpc2str(siginfo->sig_signo));       \
        else if (strcmp(#_field, "errno") == 0)                          \
            te_log_buf_append(str, "sig_errno: %s ",                     \
                              errno_rpc2str(siginfo->sig_errno));        \
        else if (strcmp(#_field, "code") == 0)                           \
            te_log_buf_append(str, "sig_code: %s ",                      \
                              si_code_rpc2str(siginfo->sig_code));       \
        else                                                             \
            te_log_buf_append(str, "sig_" #_field ": %lld ",             \
                              ((long long int)siginfo->sig_ ## _field)); \
    } while(0)

    tarpc_siginfo_received_in  in;
    tarpc_siginfo_received_out out;

    te_log_buf  *str = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(siginfo_received, -1);
    }

    if (siginfo == NULL)
    {
        ERROR("%s(): siginfo pointer should not be NULL", __FUNCTION__);
        RETVAL_INT(siginfo_received, -1);
    }

    rcf_rpc_call(rpcs, "siginfo_received", &in, &out);

    memcpy(siginfo, &out.siginfo, sizeof(out.siginfo));

    str = te_log_buf_alloc();
    if (str == NULL)
    {
        ERROR("%s(): te_log_buf_alloc() failed", __FUNCTION__);
        RETVAL_INT(siginfo_received, -1);
    }

    siginfo->sig_signo = signum_h2rpc(siginfo->sig_signo);
    siginfo->sig_errno = errno_h2rpc(siginfo->sig_errno);
    siginfo->sig_code = si_code_h2rpc(siginfo->sig_signo,
                                      siginfo->sig_code);

    te_log_buf_append(str, "{ ");
    PRINT_SI_FIELD(signo);
    PRINT_SI_FIELD(errno);
    PRINT_SI_FIELD(code);
    PRINT_SI_FIELD(trapno);
    PRINT_SI_FIELD(pid);
    PRINT_SI_FIELD(uid);
    PRINT_SI_FIELD(status);
    PRINT_SI_FIELD(utime);
    PRINT_SI_FIELD(stime);

    te_log_buf_append(str, "sig_value: %u ",
                      (unsigned)siginfo->sig_value.
                                             tarpc_sigval_u.
                                                     sival_int);
    PRINT_SI_FIELD(int);
    PRINT_SI_FIELD(ptr);
    PRINT_SI_FIELD(overrun);
    PRINT_SI_FIELD(timerid);
    PRINT_SI_FIELD(addr);
    PRINT_SI_FIELD(band);
    PRINT_SI_FIELD(fd);
    PRINT_SI_FIELD(addr_lsb);
    te_log_buf_append(str, " }");

    TAPI_RPC_LOG(rpcs, siginfo_received, "%p", "%s", siginfo,
                 te_log_buf_get(str));
    te_log_buf_free(str);
    RETVAL_INT(siginfo_received, 0);

#undef PRINT_SI_FIELD
}

int
rpc_sigfillset(rcf_rpc_server *rpcs, rpc_sigset_p set)
{
    tarpc_sigfillset_in  in;
    tarpc_sigfillset_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigfillset, -1);
    }

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigfillset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigfillset, out.retval);
    TAPI_RPC_LOG(rpcs, sigfillset, "0x%x", "%d",
                 (unsigned)set, out.retval);
    RETVAL_INT(sigfillset, out.retval);
}

int
rpc_sigaddset(rcf_rpc_server *rpcs, rpc_sigset_p set, rpc_signum signum)
{
    tarpc_sigaddset_in  in;
    tarpc_sigaddset_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigaddset, -1);
    }

    in.set = (tarpc_sigset_t)set;
    in.signum = signum;

    rcf_rpc_call(rpcs, "sigaddset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigaddset, out.retval);
    TAPI_RPC_LOG(rpcs, sigaddset, "%s, 0x%x", "%d",
                 signum_rpc2str(signum), (unsigned)set, out.retval);
    RETVAL_INT(sigaddset, out.retval);
}

int
rpc_sigdelset(rcf_rpc_server *rpcs, rpc_sigset_p set, rpc_signum signum)
{
    tarpc_sigdelset_in  in;
    tarpc_sigdelset_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigdelset, -1);
    }

    in.set = (tarpc_sigset_t)set;
    in.signum = signum;

    rcf_rpc_call(rpcs, "sigdelset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigdelset, out.retval);
    TAPI_RPC_LOG(rpcs, sigdelset, "%s, 0x%x", "%d",
                 signum_rpc2str(signum), (unsigned)set, out.retval);
    RETVAL_INT(sigdelset, out.retval);
}

int
rpc_sigismember(rcf_rpc_server *rpcs,
                const rpc_sigset_p set, rpc_signum signum)
{
    tarpc_sigismember_in  in;
    tarpc_sigismember_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigismember, -1);
    }

    in.set = (tarpc_sigset_t)set;
    in.signum = signum;

    rcf_rpc_call(rpcs, "sigismember", &in, &out);

    CHECK_RETVAL_VAR(sigismember, out.retval,
        (out.retval != 0 && out.retval != 1 && out.retval != -1), -1);
    TAPI_RPC_LOG(rpcs, sigismember, "%s, 0x%x", "%d",
                 signum_rpc2str(signum), (unsigned)set, out.retval);
    RETVAL_INT(sigismember, out.retval);
}

/* See description in tapi_rpc_signal.h */
void
rpc_sigaction_init(rcf_rpc_server *rpcs, struct rpc_struct_sigaction *sa)
{
    if (sa == NULL)
        TEST_FAIL("Argument 'sa' is NULL");

    memset(sa, 0, sizeof(*sa));
    sa->mm_mask = rpc_sigset_new(rpcs);
}

/* See description in tapi_rpc_signal.h */
void
rpc_sigaction_release(rcf_rpc_server *rpcs, struct rpc_struct_sigaction *sa)
{
    if (rpcs == NULL || sa == NULL)
        return;

    if (sa->mm_mask != RPC_NULL)
    {
        rpc_sigset_delete(rpcs, sa->mm_mask);
        sa->mm_mask = RPC_NULL;
    }
}

/**
 * Convert 'rpc_struct_sigaction' to 'tarpc_sigaction'.
 *
 * @param[in]  rpc_struct       'rpc_struct_sigaction' structure
 * @param[out] tarpc_struct     'struct tarpc_sigaction' structure
 *
 * @return Status code.
 */
static te_errno
rpc_struct_sigaction_to_tarpc_sigaction(
    const struct rpc_struct_sigaction *rpc_struct,
    struct tarpc_sigaction *tarpc_struct)
{
    assert(rpc_struct != NULL);
    assert(tarpc_struct != NULL);

    memset(tarpc_struct, 0, sizeof(*tarpc_struct));

    tarpc_struct->handler = strdup(rpc_struct->mm_handler == NULL ?
                                       "" : rpc_struct->mm_handler);
    if (tarpc_struct->handler == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    tarpc_struct->restorer = rpc_struct->mm_restorer;
    tarpc_struct->mask = (tarpc_sigset_t)rpc_struct->mm_mask;
    tarpc_struct->flags = rpc_struct->mm_flags;

    return 0;
}

int
rpc_sigaction(rcf_rpc_server *rpcs, rpc_signum signum,
              const struct rpc_struct_sigaction *act,
              struct rpc_struct_sigaction *oldact)
{
    te_errno            rc;
    tarpc_sigaction_in  in;
    tarpc_sigaction_out out;

    struct tarpc_sigaction in_act;
    struct tarpc_sigaction in_oldact;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&in_act, 0, sizeof(in_act));
    memset(&in_oldact, 0, sizeof(in_oldact));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigaction, -1);
    }

    if (act != NULL && act->mm_mask == RPC_NULL)
    {
        ERROR("%s(): Invalid 'act->mm_mask' argument",
                __FUNCTION__);
        rpcs->_errno = TE_EINVAL;
        RETVAL_INT(sigaction, -1);
    }

    in.signum = signum;
    if (act != NULL)
    {
        rc = rpc_struct_sigaction_to_tarpc_sigaction(act, &in_act);
        if (rc != 0)
        {
            rpcs->_errno = rc;
            RETVAL_INT(sigaction, -1);
        }

        in.act.act_val = &in_act;
        in.act.act_len = 1;
    }
    if (oldact != NULL)
    {
        rc = rpc_struct_sigaction_to_tarpc_sigaction(oldact, &in_oldact);
        if (rc != 0)
        {
            rpcs->_errno = rc;
            RETVAL_INT(sigaction, -1);
        }

        in.oldact.oldact_val = &in_oldact;
        in.oldact.oldact_len = 1;
    }

    rcf_rpc_call(rpcs, "sigaction", &in, &out);

    free(in_act.handler);
    free(in_oldact.handler);

    if (RPC_IS_CALL_OK(rpcs) && oldact != NULL)
    {
        struct tarpc_sigaction *out_oldact = out.oldact.oldact_val;

        TE_SPRINTF(oldact->mm_handler, "%s", out_oldact->handler);
        oldact->mm_restorer = out_oldact->restorer;
        oldact->mm_mask = (rpc_sigset_p)out_oldact->mask;
        oldact->mm_flags = out_oldact->flags;
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigaction, out.retval);
    TAPI_RPC_LOG(rpcs, sigaction, "%s, "
                 "%p{'%s', '%"TE_PRINTF_64"u', 0x%x, %s}, "
                 "%p{'%s', '%"TE_PRINTF_64"u', 0x%x, %s}", "%d",
                 signum_rpc2str(signum),
                 act,
                 (act == NULL) ? "" : act->mm_handler,
                 (act == NULL) ? 0 : act->mm_restorer,
                 (act == NULL) ? 0 : (unsigned)act->mm_mask,
                 (act == NULL) ? "0" :
                     sigaction_flags_rpc2str(act->mm_flags),
                 oldact,
                 (oldact == NULL) ? "" : oldact->mm_handler,
                 (oldact == NULL) ? 0 : oldact->mm_restorer,
                 (oldact == NULL) ? 0 : (unsigned)oldact->mm_mask,
                 (oldact == NULL) ? "0" :
                     sigaction_flags_rpc2str(oldact->mm_flags),
                 out.retval);
    RETVAL_INT(sigaction, out.retval);
}

int
rpc_sigaltstack(rcf_rpc_server *rpcs, tarpc_stack_t *ss,
                tarpc_stack_t *oss)
{
    tarpc_sigaltstack_in  in;
    tarpc_sigaltstack_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigaltstack, -1);
    }

    if (ss != NULL)
    {
        in.ss.ss_val = ss;
        in.ss.ss_len = 1;
    }
    if (oss != NULL)
    {
        in.oss.oss_val = oss;
        in.oss.oss_len = 1;
    }

    rcf_rpc_call(rpcs, "sigaltstack", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && oss != NULL &&
        rpcs->op != RCF_RPC_CALL)
        *oss = *(out.oss.oss_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigaltstack, out.retval);
    TAPI_RPC_LOG(rpcs, sigaltstack, "%p{0x%x, %s, %u}}, "
                 "%p{0x%x, %s, %u}}", "%d",
                 ss,
                 (ss == NULL) ? 0 : ss->ss_sp,
                 (ss == NULL) ? "" :
                    sigaltstack_flags_rpc2str(ss->ss_flags),
                 (ss == NULL) ? 0 : ss->ss_size,
                 oss,
                 (oss == NULL) ? 0 : oss->ss_sp,
                 (oss == NULL) ? "" :
                    sigaltstack_flags_rpc2str(oss->ss_flags),
                 (oss == NULL) ? 0 : oss->ss_size,
                 out.retval);
    RETVAL_INT(sigaltstack, out.retval);
}

void
rpc_signal_registrar_cleanup(rcf_rpc_server *rpcs)
{
    tarpc_signal_registrar_cleanup_in  in;
    tarpc_signal_registrar_cleanup_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "signal_registrar_cleanup", &in, &out);
    TAPI_RPC_LOG(rpcs, signal_registrar_cleanup, "%s", "%s", "void", "void");

    RETVAL_VOID(signal_registrar_cleanup);
}
