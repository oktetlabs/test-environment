/** @file
 * @brief Test API - RPC
 *
 * Internal definitions
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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

#ifndef __TE_TAPI_RPC_INTERNAL_H__
#define __TE_TAPI_RPC_INTERNAL_H__

/** Logger user */
#define TE_LGR_USER     "TAPI RPC"

#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#ifdef HAVE_AIO_H
#include <aio.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_rpc.h"
#include "logger_api.h"
#include "tapi_sockaddr.h"
#include "te_rpc_types.h"

#include "tarpc.h"
#include "tapi_jmp.h"

/**
 * Log TAPI RPC call.
 * If RPC call status is OK, log as ring, else log as error.
 */
#define TAPI_RPC_LOG(_x...) \
    LOG_MSG(rpcs->err_log ? TE_LL_ERROR : TE_LL_RING, _x)

/** 
 * Free memory, check RPC error, jump in the case of RPC error or if 
 * _res is TRUE, set jump condition to default value.
 */
#define TAPI_RPC_OUT(_func, _res) \
    do {                                                                \
        if (rpcs != NULL)                                               \
        {                                                               \
            rcf_rpc_free_result(&out,                                   \
                                (xdrproc_t)xdr_tarpc_##_func##_out);    \
            if (!RPC_IS_CALL_OK(rpcs))                                  \
            {                                                           \
                rpcs->iut_err_jump = TRUE;                              \
                TAPI_JMP_DO(TE_EFAIL);                                  \
            }                                                           \
            if ((_res) && rpcs->iut_err_jump)                           \
                TAPI_JMP_DO(TE_EFAIL);                                  \
            rpcs->iut_err_jump = TRUE;                                  \
        }                                                               \
        else                                                            \
        {                                                               \
            /* Try to jump */                                           \
            TAPI_JMP_DO(TE_EFAIL);                                      \
        }                                                               \
    } while (0)

/**
 * If RPC call status is OK, check condition and set specified variable
 * to _error_val and RPC server errno to TE_ECORRUPTED, if it is true.
 * If RPC call status is not OK, variable is set to -1 and RPC server
 * errno is not updated.
 *
 * The function assumes to have RPC server handle as 'rpcs' variable in
 * the context.
 *
 * @param _func         function name
 * @param _var          variable with return value
 * @param _cond         condition to be checked, if RPC call status is OK
 * @param _error_val    value to be assigned in the case of error
 */
#define CHECK_RETVAL_VAR(_func, _var, _cond, _error_val) \
    do {                                                            \
        if (RPC_IS_CALL_OK(rpcs))                                   \
        {                                                           \
            if (_cond)                                              \
            {                                                       \
                ERROR("Function %s() returned incorrect value %d",  \
                      #_func, (int)(_var));                         \
                rpcs->_errno = TE_RC(TE_TAPI, TE_ECORRUPTED);       \
                (_var) = (_error_val);                              \
            }                                                       \
            else if (rpcs->errno_change_check &&                    \
                     (_var) != (_error_val) &&                      \
                     out.common.errno_changed)                      \
            {                                                       \
                ERROR("Function %s() returned %d, but changed "     \
                      "errno to %s", #_func, (int)(_var),           \
                      errno_rpc2str(RPC_ERRNO(rpcs)));              \
                rpcs->_errno = TE_RC(TE_TAPI, TE_ECORRUPTED);       \
                (_var) = (_error_val);                              \
            }                                                       \
            else if (RPC_ERRNO(rpcs) == RPC_ERPCNOTSUPP)            \
            {                                                       \
                RING("Function %s() is not supported",  #_func);    \
                (_var) = (_error_val);                              \
            }                                                       \
            if ((_var) == (_error_val) && rpcs->iut_err_jump)       \
                rpcs->err_log = TRUE;                               \
        }                                                           \
        else                                                        \
        {                                                           \
            (_var) = (_error_val);                                  \
            rpcs->err_log = TRUE;                                   \
        }                                                           \
    } while (0)

/**
 * If RPC call status is OK, check that variable with function return
 * value is greater or equal to minus one and set specified variable
 * to -1 and RPC server errno to TE_ECORRUPTED, it is true.
 * If RPC call status is not OK, variable is set to -1 and RPC server
 * errno is not updated.
 *
 * The function assumes to have RPC server handle as 'rpcs' variable in
 * the context.
 *
 * @param _func     function name
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, ((_var) < -1), -1)

/**
 * If RPC call status is OK, check that variable with function return
 * value is zero or minus one and set specified variable to -1 and RPC
 * server errno to TE_ECORRUPTED, it is true.  If RPC call status is
 * not OK, variable is set to -1 and RPC server errno is not updated.
 *
 * The function assumes to have RPC server handle as 'rpcs' variable in
 * the context.
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, (((_var) != 0) && ((_var) != -1)), -1)

/**
 * If RPC call status is OK, check that variable with function return
 * value is zero and set specified variable to -1 and RPC
 * server errno to TE_ECORRUPTED, it is true.  If RPC call status is
 * not OK, variable is set to -1 and RPC server errno is not updated.
 *
 * The function assumes to have RPC server handle as 'rpcs' variable in
 * the context.
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_ZERO(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, ((_var) != 0), (uint32_t)-1)

/** Return with check (for functions returning zero value) */
#define RETVAL_ZERO_INT(_func, _retval) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_OUT(_func, (__retval) != 0);                       \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning boolean value) */
#define RETVAL_TRUE(_func, _retval) \
    do {                                  \
        te_bool __retval = _retval;       \
                                          \
        TAPI_RPC_OUT(_func, !(__retval)); \
                                          \
        return __retval;                  \
    } while (0)


/** Return with check (for functions returning value >= -1) */
#define RETVAL_INT(_func, _retval) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_OUT(_func, (__retval) < 0);                        \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning pointers) */
#define RETVAL_PTR(_func, _retval) \
    do {                                                            \
        void *__retval = (void *)(_retval);                         \
                                                                    \
        TAPI_RPC_OUT(_func, __retval == NULL);                      \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning pointers) */
#define RETVAL_RPC_PTR(_func, _retval) \
    do {                                                            \
        rpc_ptr __retval = (_retval);                               \
                                                                    \
        TAPI_RPC_OUT(_func, __retval == RPC_NULL);                  \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for void functions) */
#define RETVAL_VOID(_func) \
    do {                                                            \
        TAPI_RPC_OUT(_func, FALSE);                                 \
        return;                                                     \
    } while(0)

/** 
 * Return wait_status with check. 
 */
#define RETVAL_WAIT_STATUS(_func, _retval) \
    do {                                                            \
        rpc_wait_status __retval = (_retval);                       \
                                                                    \
        TAPI_RPC_OUT(_func,                                         \
                     __retval.flag != RPC_WAIT_STATUS_EXITED ||     \
                     __retval.value != 0);                          \
                                                                    \
        return __retval;                                            \
    } while(0)

/** 
 * Return int value and check it and wait_status. 
 */
#define RETVAL_INT_CHECK_WAIT_STATUS(_func, _retval, _status) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_OUT(_func, __retval < 0 ||                         \
                     _status.flag != RPC_WAIT_STATUS_EXITED ||      \
                     _status.value != 0);                           \
                                                                    \
        return __retval;                                            \
    } while(0)

#endif /* !__TE_TAPI_RPC_INTERNAL_H__ */
