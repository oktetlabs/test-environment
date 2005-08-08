/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of signals-related API
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

    char *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(signal, NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.signum = signum;
    in.handler.handler_val = (char *)handler;
    in.handler.handler_len = (handler == NULL) ? 0 : (strlen(handler) + 1);

    rcf_rpc_call(rpcs, "signal", &in, &out);
                 
    TAPI_RPC_LOG("RPC (%s,%s): signal(%s, %s) -> %s (%s)",
                 rpcs->ta, rpcs->name,
                 signum_rpc2str(signum),
                 (handler != NULL) ? handler : "(null)",
                 out.handler.handler_val != NULL ?
                     out.handler.handler_val : "(null)",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    /* Yes, I know that it's memory leak, but what do you propose?! */
    if (RPC_IS_CALL_OK(rpcs) && out.handler.handler_val != NULL)
        res = strdup(out.handler.handler_val);
        
    RETVAL_PTR(signal, res);
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

    if (status != NULL)
        *status = stat;
    RETVAL_INT_CHECK_WAIT_STATUS(waitpid, out.pid, stat);
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

int
rpc_sigaction(rcf_rpc_server *rpcs, rpc_signum signum,
              const struct rpc_struct_sigaction *act,
              struct rpc_struct_sigaction *oldact)
{
    rcf_rpc_op          op;
    tarpc_sigaction_in  in;
    tarpc_sigaction_out out;

    struct tarpc_sigaction in_act;
    struct tarpc_sigaction in_oldact;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sigaction, out.retval);
    }
    if (act != NULL && act->mm_mask == RPC_NULL)
    {
        ERROR("%s(): Invalid 'act->mm_mask' argument",
                __FUNCTION__);
        rpcs->_errno = EINVAL;
        RETVAL_INT(sigaction, out.retval);
    }
    if (oldact != NULL && oldact->mm_mask == RPC_NULL)
    {
        ERROR("%s(): Invalid 'oldact->mm_mask' argument",
                __FUNCTION__);
        rpcs->_errno = EINVAL;
        RETVAL_INT(sigaction, -1);
    }

    op = rpcs->op;

    in.signum = signum;
    if (act != NULL)
    {
        memset(&in_act, 0, sizeof(in_act));
        in.act.act_val = &in_act;
        in.act.act_len = 1;

        in_act.xx_handler.xx_handler_val = (char *)act->mm_handler;
        in_act.xx_handler.xx_handler_len = sizeof(act->mm_handler);

        in_act.xx_restorer.xx_restorer_val = (char *)act->mm_restorer;
        in_act.xx_restorer.xx_restorer_len = sizeof(act->mm_restorer);

        in_act.xx_mask = (tarpc_sigset_t)act->mm_mask;
        in_act.xx_flags = act->mm_flags;
    }

    if (oldact != NULL)
    {
        memset(&in_oldact, 0, sizeof(in_oldact));
        in.oldact.oldact_val = &in_oldact;
        in.oldact.oldact_len = 1;

        in_oldact.xx_handler.xx_handler_val = oldact->mm_handler;
        in_oldact.xx_handler.xx_handler_len = sizeof(oldact->mm_handler);
        in_oldact.xx_restorer.xx_restorer_val = oldact->mm_restorer;
        in_oldact.xx_restorer.xx_restorer_len =
                                              sizeof(oldact->mm_restorer);
        in_oldact.xx_mask = (tarpc_sigset_t)oldact->mm_mask;
        in_oldact.xx_flags = oldact->mm_flags;
    }

    rcf_rpc_call(rpcs, "sigaction", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && oldact != NULL)
    {
        struct tarpc_sigaction *out_oldact = out.oldact.oldact_val;

        assert(out_oldact->xx_handler.xx_handler_val != NULL);
        assert(out_oldact->xx_handler.xx_handler_len <=
                   sizeof(oldact->mm_handler));
        memcpy(oldact->mm_handler, out_oldact->xx_handler.xx_handler_val,
               out_oldact->xx_handler.xx_handler_len);

        assert(out_oldact->xx_restorer.xx_restorer_val != NULL);
        assert(out_oldact->xx_restorer.xx_restorer_len <=
                   sizeof(oldact->mm_restorer));
        memcpy(oldact->mm_restorer,
               out_oldact->xx_restorer.xx_restorer_val,
               out_oldact->xx_restorer.xx_restorer_len);

        oldact->mm_mask = (rpc_sigset_p)out_oldact->xx_mask;
        oldact->mm_flags = out_oldact->xx_flags;
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(sigaction, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: sigaction(%s, %p, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 signum_rpc2str(signum), act, oldact,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(sigaction, out.retval);
}
