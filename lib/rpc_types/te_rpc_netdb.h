/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from netdb.h.
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
 
#ifndef __TE_RPC_NETDB_H__
#define __TE_RPC_NETDB_H__

#include "te_rpc_defs.h"


/** TA-independent addrinfo flags */
typedef enum rpc_ai_flags {
    RPC_AI_PASSIVE     = 1,    /**< Socket address is intended for `bind' */
    RPC_AI_CANONNAME   = 2,    /**< Request for canonical name */
    RPC_AI_NUMERICHOST = 4,    /**< Don't use name resolution */
    RPC_AI_UNKNOWN     = 8     /**< Invalid flags */
} rpc_ai_flags;

#ifdef AI_PASSIVE

#define AI_INVALID      0xFFFFFFFF
#define AI_ALL_FLAGS    (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)

/** Convert RPC AI flags to native ones */
static inline int
ai_flags_rpc2h(rpc_ai_flags flags)
{
    return (!!(flags & RPC_AI_PASSIVE) * AI_PASSIVE) |
           (!!(flags & RPC_AI_CANONNAME) * AI_CANONNAME) |
           (!!(flags & RPC_AI_NUMERICHOST) * AI_NUMERICHOST) |
           (!!(flags & RPC_AI_UNKNOWN) * AI_INVALID);
}

/** Convert native AI flags to RPC ones */
static inline rpc_ai_flags
ai_flags_h2rpc(int flags)
{
    if ((flags & ~AI_ALL_FLAGS) != 0)
        return RPC_AI_UNKNOWN;
        
    return (!!(flags & AI_PASSIVE) * RPC_AI_PASSIVE) |
           (!!(flags & AI_CANONNAME) * RPC_AI_CANONNAME) |
           (!!(flags & AI_NUMERICHOST) * RPC_AI_NUMERICHOST);
}

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

static inline int
ai_rc_h2rpc(rpc_ai_rc rc)
{
    switch (rc)
    {
        H2RPC(EAI_BADFLAGS);
        H2RPC(EAI_NONAME);
        H2RPC(EAI_AGAIN);
        H2RPC(EAI_FAIL);
#if (EAI_NODATA != EAI_NONAME)
        H2RPC(EAI_NODATA);
#endif
        H2RPC(EAI_FAMILY);
        H2RPC(EAI_SOCKTYPE);
        H2RPC(EAI_SERVICE);
#ifdef EAI_ADDRFAMILY
        H2RPC(EAI_ADDRFAMILY);
#endif
        H2RPC(EAI_MEMORY);
        H2RPC(EAI_SYSTEM);
#if 0
        H2RPC(EAI_INPROGRESS);
        H2RPC(EAI_CANCELED);
        H2RPC(EAI_NOTCANCELED);
        H2RPC(EAI_ALLDONE);
        H2RPC(EAI_INTR);
#endif        
        case 0:  return 0;
        default: return RPC_EAI_UNKNOWN;
    }
}

#endif /* AI_PASSIVE */

#endif /* !__TE_RPC_NETDB_H__ */
