/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of signals-related API
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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

#include "tapi_rpc_internal.h"
#include "tapi_rpc_signal.h"


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

    rpcs->op = RCF_RPC_CALL_WAIT;

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
    
    TAPI_RPC_LOG("RPC (%s,%s): signal(%s, %s) -> %s (%s)",
                 rpcs->ta, rpcs->name,
                 signum_rpc2str(signum),
                 (handler != NULL) ? handler : "(null)",
                 res != NULL ? res : "(null)",
                 errno_rpc2str(RPC_ERRNO(rpcs)));
        
    RETVAL_PTR(signal, res);
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

    rpcs->op = RCF_RPC_CALL_WAIT;

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
    
    TAPI_RPC_LOG("RPC (%s,%s): bsd_signal(%s, %s) -> %s (%s)",
                 rpcs->ta, rpcs->name,
                 signum_rpc2str(signum),
                 (handler != NULL) ? handler : "(null)",
                 res != NULL ? res : "(null)",
                 errno_rpc2str(RPC_ERRNO(rpcs)));
        
    RETVAL_PTR(bsd_signal, res);
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

    rpcs->op = RCF_RPC_CALL_WAIT;

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
    
    TAPI_RPC_LOG("RPC (%s,%s): sysv_signal(%s, %s) -> %s (%s)",
                 rpcs->ta, rpcs->name,
                 signum_rpc2str(signum),
                 (handler != NULL) ? handler : "(null)",
                 res != NULL ? res : "(null)",
                 errno_rpc2str(RPC_ERRNO(rpcs)));
        
    RETVAL_PTR(sysv_signal, res);
}

int
rpc_kill(rcf_rpc_server *rpcs, tarpc_pid_t pid, rpc_signum signum)
{
    rcf_rpc_op     op;
    tarpc_kill_in  in;
    tarpc_kill_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(kill, -1);
    }

    op = rpcs->op;

    in.signum = signum;
    in.pid = pid;

    rcf_rpc_call(rpcs, "kill", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(kill, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: kill(%d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 pid, signum_rpc2str(signum),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(kill, out.retval);
}

tarpc_pid_t
rpc_waitpid(rcf_rpc_server *rpcs, tarpc_pid_t pid, rpc_wait_status *status, 
            rpc_waitpid_opts options)
{
    rcf_rpc_op          op;
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

    op = rpcs->op;

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
    TAPI_RPC_LOG("RPC (%s,%s)%s: waitpid(%d, %p, 0x%x) -> %d (%s) "
                 "status %s 0x%x",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 pid, status, options,
                 out.pid, errno_rpc2str(RPC_ERRNO(rpcs)), 
                 wait_status_flag_rpc2str(stat.flag), stat.value);
    if (out.pid > 0 && (stat.flag != RPC_WAIT_STATUS_EXITED || 
                        stat.value != 0))
        ERROR("waitpid() returned non-zero status");

    if (status != NULL)
        *status = stat;
    RETVAL_INT_CHECK_WAIT_STATUS(waitpid, out.pid, stat);
}

int
rpc_ta_kill_death(rcf_rpc_server *rpcs, tarpc_pid_t pid)
{
    rcf_rpc_op     op;
    tarpc_kill_in  in;
    tarpc_kill_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ta_kill_death, -1);
    }

    op = rpcs->op;

    in.pid = pid;

    rcf_rpc_call(rpcs, "ta_kill_death", &in, &out);

    /* This function should not check errno */
    out.common.errno_changed = FALSE;
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(ta_kill_death, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: ta_kill_death(%d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 pid, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(ta_kill_death, out.retval);
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

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, "sigset_new", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): sigset_new() -> 0x%x (%s)",
                 rpcs->ta, rpcs->name,
                 (unsigned)out.set, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigset_delete", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): sigset_delete(0x%x) -> (%s)",
                 rpcs->ta, rpcs->name, (unsigned)set,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(sigset_delete);
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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;
    in.oldset = (tarpc_sigset_t)oldset;
    in.how = how;

    rcf_rpc_call(rpcs, "sigprocmask", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigprocmask, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): sigprocmask(%d, 0x%x, 0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name, how,
                 (unsigned)set, (unsigned)oldset,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigemptyset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigemptyset, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): sigemptyset(0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name, (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigpending", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigpending, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): sigpending(0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name, (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    TAPI_RPC_LOG("RPC (%s,%s): sigsuspend(0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name, (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, "sigreceived", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): sigreceived() -> 0x%x (%s)",
                 rpcs->ta, rpcs->name,
                 (unsigned)out.set, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(sigreceived, out.set);
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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;

    rcf_rpc_call(rpcs, "sigfillset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigfillset, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): sigfillset(0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;
    in.signum = signum;

    rcf_rpc_call(rpcs, "sigaddset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigaddset, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): sigaddset(%s, 0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 signum_rpc2str(signum), (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;
    in.signum = signum;

    rcf_rpc_call(rpcs, "sigdelset", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigdelset, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): sigdelset(%s, 0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 signum_rpc2str(signum), (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

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

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_sigset_t)set;
    in.signum = signum;

    rcf_rpc_call(rpcs, "sigismember", &in, &out);

    CHECK_RETVAL_VAR(sigismember, out.retval,
        (out.retval != 0 && out.retval != 1 && out.retval != -1), -1);

    TAPI_RPC_LOG("RPC (%s,%s): sigismember(%s, 0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 signum_rpc2str(signum), (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(sigismember, out.retval);
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

    tarpc_struct->restorer = strdup(rpc_struct->mm_restorer == NULL ?
                                        "" : rpc_struct->mm_restorer);
    if (tarpc_struct->restorer == NULL)
    {
        free(tarpc_struct->handler);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

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
    rcf_rpc_op          op;
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
    
    op = rpcs->op;

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
    free(in_act.restorer);
    free(in_oldact.handler);
    free(in_oldact.restorer);

    if (RPC_IS_CALL_OK(rpcs) && oldact != NULL)
    {
        struct tarpc_sigaction *out_oldact = out.oldact.oldact_val;

        TE_SPRINTF(oldact->mm_handler, out_oldact->handler);
        TE_SPRINTF(oldact->mm_restorer, out_oldact->restorer);
        oldact->mm_mask = (rpc_sigset_p)out_oldact->mask;
        oldact->mm_flags = out_oldact->flags;
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigaction, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: sigaction(%s, "
                 "%p{'%s', '%s', 0x%x, %s}, "
                 "%p{'%s', '%s', 0x%x, %s}) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 signum_rpc2str(signum),
                 act,
                 (act == NULL) ? "" : act->mm_handler,
                 (act == NULL) ? "" : act->mm_restorer,
                 (act == NULL) ? 0 : (unsigned)act->mm_mask,
                 (act == NULL) ? "0" :
                     sigaction_flags_rpc2str(act->mm_flags),
                 oldact,
                 (oldact == NULL) ? "" : oldact->mm_handler,
                 (oldact == NULL) ? "" : oldact->mm_restorer,
                 (oldact == NULL) ? 0 : (unsigned)oldact->mm_mask,
                 (oldact == NULL) ? "0" :
                     sigaction_flags_rpc2str(oldact->mm_flags),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(sigaction, out.retval);
}
