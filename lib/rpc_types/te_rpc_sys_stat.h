/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/stat.h.
 * 
 * 
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 
#ifndef __TE_RPC_SYS_STAT_H__
#define __TE_RPC_SYS_STAT_H__

#include "te_rpc_defs.h"

/**
 * All known file mode flags.
 */
typedef enum rpc_mode_flags {
    RPC_S_ISUID  = (1 << 1),
    RPC_S_ISGID  = (1 << 2),
    RPC_S_IRUSR  = (1 << 3),
    RPC_S_IWUSR  = (1 << 4),
    RPC_S_IXUSR  = (1 << 5),
    RPC_S_IRWXU  = (1 << 6),
    RPC_S_IREAD  = (1 << 7),
    RPC_S_IWRITE = (1 << 8),
    RPC_S_IEXEC  = (1 << 9),
    RPC_S_IRGRP  = (1 << 10),
    RPC_S_IWGRP  = (1 << 11),
    RPC_S_IXGRP  = (1 << 12),
    RPC_S_IRWXG  = (1 << 13),
    RPC_S_IROTH  = (1 << 14),
    RPC_S_IWOTH  = (1 << 15),
    RPC_S_IXOTH  = (1 << 16),
    RPC_S_IRWXO  = (1 << 17),
} rpc_mode_flags;

#define MODE_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(S_ISUID), \
    RPC_BIT_MAP_ENTRY(S_ISGID), \
    RPC_BIT_MAP_ENTRY(S_IRUSR), \
    RPC_BIT_MAP_ENTRY(S_IWUSR), \
    RPC_BIT_MAP_ENTRY(S_IXUSR), \
    RPC_BIT_MAP_ENTRY(S_IRWXU), \
    RPC_BIT_MAP_ENTRY(S_IREAD), \
    RPC_BIT_MAP_ENTRY(S_IWRITE), \
    RPC_BIT_MAP_ENTRY(S_IEXEC), \
    RPC_BIT_MAP_ENTRY(S_IRGRP), \
    RPC_BIT_MAP_ENTRY(S_IWGRP), \
    RPC_BIT_MAP_ENTRY(S_IXGRP), \
    RPC_BIT_MAP_ENTRY(S_IRWXG), \
    RPC_BIT_MAP_ENTRY(S_IROTH), \
    RPC_BIT_MAP_ENTRY(S_IWOTH), \
    RPC_BIT_MAP_ENTRY(S_IXOTH), \
    RPC_BIT_MAP_ENTRY(S_IRWXO)

/**
 * mode_flags_rpc2str()
 */
RPCBITMAP2STR(mode_flags, MODE_FLAGS_MAPPING_LIST)

#ifdef S_ISUID
/** Convert RPC mode flags to native flags */
static inline int
mode_flags_rpc2h(unsigned int flags)
{
    return 
        (!!(flags & RPC_S_ISUID) * S_ISUID) |
        (!!(flags & RPC_S_ISGID) * S_ISGID) |
        (!!(flags & RPC_S_IRUSR) * S_IRUSR) |
        (!!(flags & RPC_S_IWUSR) * S_IWUSR) |
        (!!(flags & RPC_S_IXUSR) * S_IXUSR) |
        (!!(flags & RPC_S_IRWXU) * S_IRWXU) |
        (!!(flags & RPC_S_IREAD) * S_IREAD) |
        (!!(flags & RPC_S_IWRITE) * S_IWRITE) |
        (!!(flags & RPC_S_IEXEC) * S_IEXEC) |
        (!!(flags & RPC_S_IRGRP) * S_IRGRP) |
        (!!(flags & RPC_S_IWGRP) * S_IWGRP) |
        (!!(flags & RPC_S_IXGRP) * S_IXGRP) |
        (!!(flags & RPC_S_IRWXG) * S_IRWXG) |
        (!!(flags & RPC_S_IROTH) * S_IROTH) |
        (!!(flags & RPC_S_IWOTH) * S_IWOTH) |
        (!!(flags & RPC_S_IXOTH) * S_IXOTH) |
        (!!(flags & RPC_S_IRWXO) * S_IRWXO);
}
#endif

#endif /* !__TE_RPC_SYS_STAT_H__ */
