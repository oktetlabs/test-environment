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

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_netdb.h"


#ifdef AI_PASSIVE

#define AI_INVALID      0xFFFFFFFF
#define AI_ALL_FLAGS    (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)

/** Convert RPC AI flags to native ones */
int
ai_flags_rpc2h(rpc_ai_flags flags)
{
    return (!!(flags & RPC_AI_PASSIVE) * AI_PASSIVE) |
           (!!(flags & RPC_AI_CANONNAME) * AI_CANONNAME) |
           (!!(flags & RPC_AI_NUMERICHOST) * AI_NUMERICHOST) |
           (!!(flags & RPC_AI_UNKNOWN) * AI_INVALID);
}

/** Convert native AI flags to RPC ones */
rpc_ai_flags
ai_flags_h2rpc(int flags)
{
    if ((flags & ~AI_ALL_FLAGS) != 0)
        return RPC_AI_UNKNOWN;
        
    return (!!(flags & AI_PASSIVE) * RPC_AI_PASSIVE) |
           (!!(flags & AI_CANONNAME) * RPC_AI_CANONNAME) |
           (!!(flags & AI_NUMERICHOST) * RPC_AI_NUMERICHOST);
}


rpc_ai_rc
ai_rc_h2rpc(int rc)
{
    switch (rc)
    {
        H2RPC(EAI_BADFLAGS);
        H2RPC(EAI_NONAME);
        H2RPC(EAI_AGAIN);
        H2RPC(EAI_FAIL);
#ifdef EAI_NODATA
#if (EAI_NODATA != EAI_NONAME)
        H2RPC(EAI_NODATA);
#endif
#endif
        H2RPC(EAI_FAMILY);
        H2RPC(EAI_SOCKTYPE);
        H2RPC(EAI_SERVICE);
#ifdef EAI_ADDRFAMILY
        H2RPC(EAI_ADDRFAMILY);
#endif
        H2RPC(EAI_MEMORY);
#ifdef EAI_SYSTEM        
        H2RPC(EAI_SYSTEM);
#endif        
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

#endif /* !__TE_RPC_NETDB_H__ */
