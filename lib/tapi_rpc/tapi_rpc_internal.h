/** @file
 * @brief Test API - RPC
 *
 * Internal definitions
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

#ifndef __TE_TAPI_RPC_INTERNAL_H__
#define __TE_TAPI_RPC_INTERNAL_H__

#define TE_LGR_USER     "TAPI RPC"

#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_rpc.h"
#include "logger_api.h"
#include "tapi_sockaddr.h"
#include "tapi_rpcsock_defs.h"

#include "tarpc.h"


/**
 * Log TAPI RPC call.
 * If RPC call status is OK, log as ring, else log as error.
 */
#define TAPI_RPC_LOG(_x...) \
    LGR_MESSAGE(RPC_IS_CALL_OK(rpcs) ? TE_LL_RING : TE_LL_ERROR, \
                TE_LGR_USER, _x)

/** Free memory used in RPC output */
#define TAPI_RPC_FREE_OUT(_func) \
    rcf_rpc_free_result(&out, (xdrproc_t)xdr_tarpc_##_func##_out)

/**
 * If RPC call status is OK, check condition and set specified variable
 * to _error_val and RPC server errno to ETECORRUPTED, if it is true.
 * If RPC call status is not OK, variable is set to -1 and RPC server
 * errno is not updated.
 *
 * The function assumes to have RPC server handle as 'rpcs' variable in
 * the context.
 *
 * @param _func         function
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
                      #_func, (_var));                              \
                rpcs->_errno = TE_RC(TE_TAPI, ETECORRUPTED);        \
                (_var) = (_error_val);                              \
            }                                                       \
        }                                                           \
        else                                                        \
        {                                                           \
            (_var) = (_error_val);                                  \
        }                                                           \
    } while (0)

/**
 * If RPC call status is OK, check that variable with function return
 * value is greater or equal to minus one and set specified variable
 * to -1 and RPC server errno to ETECORRUPTED, it is true.
 * If RPC call status is not OK, variable is set to -1 and RPC server
 * errno is not updated.
 *
 * The function assumes to have RPC server handle as 'rpcs' variable in
 * the context.
 *
 * @param _func     function
 * @param _var      variable with return value
 */
#define CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(_func, _var) \
    CHECK_RETVAL_VAR(_func, _var, ((_var) < -1), -1)

/**
 * If RPC call status is OK, check that variable with function return
 * value is zero or minus one and set specified variable to -1 and RPC
 * server errno to ETECORRUPTED, it is true.  If RPC call status is
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
        

/** Return with check (for functions returning value >= -1) */
#define RETVAL_VAL(_func, _retval) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_FREE_OUT(_func);                                   \
                                                                    \
        return __retval;                                            \
    } while (0)

/** Return with check (for functions returning 0 or -1) */
#define RETVAL_INT_ZERO_OR_MINUS_ONE(_func, _retval) \
    do {                                                            \
        int __retval = (_retval);                                   \
                                                                    \
        TAPI_RPC_FREE_OUT(_func);                                   \
                                                                    \
        return __retval;                                            \
    } while(0)

/** Return with check (for functions returning pointers) */
#define RETVAL_PTR(_func, _retval) \
    do {                                                            \
        void *__retval = (void *)(_retval);                         \
                                                                    \
        TAPI_RPC_FREE_OUT(_func);                                   \
                                                                    \
        if (!RPC_IS_CALL_OK(rpcs))                                  \
            return NULL;                                            \
        else                                                        \
            return __retval;                                        \
    } while (0)

/** Return with check */
#define RETVAL_VOID(_func) \
    do {                                                            \
        TAPI_RPC_FREE_OUT(_func);                                   \
        return;                                                     \
    } while(0)

#endif /* !__TE_TAPI_RPC_INTERNAL_H__ */
