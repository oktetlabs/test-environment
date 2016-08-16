/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from dlfcn.h.
 * 
 * 
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
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
 * @author Nikita Rastegaev <Nikita.Rastegaev@oktetlabs.ru>
 *
 * $Id: $
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "te_defs.h"
#include "te_rpc_defs.h"
#include "te_rpc_dlfcn.h"


/** Convert RPC dlopen mode flags to native flags */
unsigned int
dlopen_flags_rpc2h(unsigned int flags)
{
    UNUSED(flags);  /* Possibly unused */
    return 0
#ifdef RTLD_LAZY
        | (!!(flags & RPC_RTLD_LAZY) * RTLD_LAZY)
#endif
#ifdef RTLD_NOW
        | (!!(flags & RPC_RTLD_NOW) * RTLD_NOW)
#endif
#ifdef RTLD_NOLOAD
        | (!!(flags & RPC_RTLD_NOLOAD) * RTLD_NOLOAD)
#endif
#ifdef RTLD_DEEPBIND
        | (!!(flags & RPC_RTLD_DEEPBIND) * RTLD_DEEPBIND)
#endif
#ifdef RTLD_GLOBAL
        | (!!(flags & RPC_RTLD_GLOBAL) * RTLD_GLOBAL)
#endif
#ifdef RTLD_NODELETE
        | (!!(flags & RPC_RTLD_NODELETE) * RTLD_NODELETE)
#endif
        ;
}


/** Convert dlopen mode native flags to RPC flags */
unsigned int
dlopen_flags_h2rpc(unsigned int flags)
{
    UNUSED(flags);  /* Possibly unused */
    return 0
#ifdef RTLD_LAZY
        | (!!(flags & RTLD_LAZY) * RPC_RTLD_LAZY)
#endif
#ifdef RTLD_NOW
        | (!!(flags & RTLD_NOW) * RPC_RTLD_NOW)
#endif
#ifdef RTLD_NOLOAD
        | (!!(flags & RTLD_NOLOAD) * RPC_RTLD_NOLOAD)
#endif
#ifdef RTLD_DEEPBIND
        | (!!(flags & RTLD_DEEPBIND) * RPC_RTLD_DEEPBIND)
#endif
#ifdef RTLD_GLOBAL
        | (!!(flags & RTLD_GLOBAL) * RPC_RTLD_GLOBAL)
#endif
#ifdef RTLD_NODELETE
        | (!!(flags & RTLD_NODELETE) * RPC_RTLD_NODELETE)
#endif
        ;
}

