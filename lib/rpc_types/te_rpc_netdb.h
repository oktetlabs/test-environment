/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from netdb.h.
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
 
#ifndef __TE_RPC_NETDB_H__
#define __TE_RPC_NETDB_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/** TA-independent addrinfo flags */
typedef enum rpc_ai_flags {
    RPC_AI_PASSIVE     = 1,    /**< Socket address is intended for `bind' */
    RPC_AI_CANONNAME   = 2,    /**< Request for canonical name */
    RPC_AI_NUMERICHOST = 4,    /**< Don't use name resolution */
    RPC_AI_UNKNOWN     = 8     /**< Invalid flags */
} rpc_ai_flags;

#define AI_INVALID      0xFFFFFFFF
#define AI_ALL_FLAGS    (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)

/** Convert RPC AI flags to native ones */
extern int ai_flags_rpc2h(rpc_ai_flags flags);

/** Convert native AI flags to RPC ones */
extern rpc_ai_flags ai_flags_h2rpc(int flags);


/* TA-independent getaddrinfo() return codes  */
typedef enum rpc_ai_rc {
    RPC_EAI_BADFLAGS,       /**< Invalid value for `ai_flags' field */
    RPC_EAI_NONAME,         /**< NAME or SERVICE is unknown */
    RPC_EAI_AGAIN,          /**< Temporary failure in name resolution */
    RPC_EAI_FAIL,           /**< Non-recoverable failure in name res */
    RPC_EAI_NODATA,         /**< No address associated with NAME */
    RPC_EAI_FAMILY,         /**< `ai_family' not supported */
    RPC_EAI_SOCKTYPE,       /**< `ai_socktype' not supported */
    RPC_EAI_SERVICE,        /**< SERVICE not supported for `ai_socktype' */
    RPC_EAI_ADDRFAMILY,     /**< Address family for NAME not supported */
    RPC_EAI_MEMORY,         /**< Memory allocation failure */
    RPC_EAI_SYSTEM,         /**< System error returned in `errno' */
    RPC_EAI_INPROGRESS,     /**< Processing request in progress */
    RPC_EAI_CANCELED,       /**< Request canceled */
    RPC_EAI_NOTCANCELED,    /**< Request not canceled */
    RPC_EAI_ALLDONE,        /**< All requests done */
    RPC_EAI_INTR,           /**< Interrupted by a signal */
    RPC_EAI_UNKNOWN
} rpc_ai_rc;

extern rpc_ai_rc ai_rc_h2rpc(int rc);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_NETDB_H__ */
