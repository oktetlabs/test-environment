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
#ifdef HAVE_NET_IF_ARP_H
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


/** Check, if RPC is called successfully */
#define RPC_CALL_OK \
    ((rpcs->_errno == 0) || \
     ((rpcs->_errno >= RPC_EPERM) && (rpcs->_errno <= RPC_EMEDIUMTYPE)))

#define LOG_TE_ERROR(_func) \
    do {                                                        \
        if (!RPC_CALL_OK)                                       \
        {                                                       \
            ERROR("RPC (%s,%s): " #_func "() failed: %s",       \
                  rpcs->ta, rpcs->name,                     \
                  errno_rpc2str(RPC_ERRNO(rpcs)));            \
        }                                                       \
    } while (FALSE)

/** Return with check (for functions returning 0 or -1) */
#define RETVAL_RC(_func) \
    do {                                                                \
        int __retval = (out.retval);                                    \
                                                                        \
        LOG_TE_ERROR(_func);                                            \
        rcf_rpc_free_result(&out, (xdrproc_t)xdr_tarpc_##_func##_out);  \
                                                                        \
        if (!RPC_CALL_OK)                                               \
            return -1;                                                  \
                                                                        \
        if (__retval != 0 && __retval != -1)                            \
        {                                                               \
            ERROR("function %s returned incorrect value %d",            \
                  #_func, __retval);                                    \
            rpcs->_errno = TE_RC(TE_TAPI, ETECORRUPTED);              \
            return -1;                                                  \
        }                                                               \
        return __retval;                                                \
    } while(0)

/** Return with check (for functions returning value >= -1) */
#define RETVAL_VAL(_retval, _func) \
    do {                                                                \
        int __retval = (_retval);                                       \
                                                                        \
        LOG_TE_ERROR(_func);                                            \
        rcf_rpc_free_result(&out, (xdrproc_t)xdr_tarpc_##_func##_out);  \
                                                                        \
        if (!RPC_CALL_OK)                                               \
            return -1;                                                  \
                                                                        \
        if (__retval < -1)                                              \
        {                                                               \
            ERROR("function %s returned incorrect value %d",            \
                  #_func, __retval);                                    \
            rpcs->_errno = TE_RC(TE_TAPI, ETECORRUPTED);              \
            return -1;                                                  \
        }                                                               \
        return __retval;                                                \
    } while (0)

/** Return with check (for functions returning pointers) */
#define RETVAL_PTR(_retval, _func) \
    do {                                                                \
        void *__retval = (void *)(_retval);                             \
                                                                        \
        LOG_TE_ERROR(_func);                                            \
        rcf_rpc_free_result(&out, (xdrproc_t)xdr_tarpc_##_func##_out);  \
                                                                        \
        if (!RPC_CALL_OK)                                               \
            return NULL;                                                \
        else                                                            \
            return __retval;                                            \
    } while (0)

/** Return with check */
#define RETVAL_VOID(_func) \
    do {                                                                \
        LOG_TE_ERROR(_func);                                            \
        rcf_rpc_free_result(&out, (xdrproc_t)xdr_tarpc_##_func##_out);  \
    } while(0)

#endif /* !__TE_TAPI_RPC_INTERNAL_H__ */
