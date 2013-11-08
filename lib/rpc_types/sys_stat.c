/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/stat.h.
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
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif 

#include "te_rpc_defs.h"
#include "te_rpc_sys_stat.h"


#ifdef S_ISUID
/** Convert RPC mode flags to native flags */
unsigned int
file_mode_flags_rpc2h(unsigned int flags)
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

#ifdef R_OK
int
access_mode_flags_rpc2h(int flags)
{
    if (flags == RPC_F_OK)
        return F_OK;
    return
        (!!(flags & RPC_R_OK) * R_OK) |
        (!!(flags & RPC_W_OK) * W_OK) |
        (!!(flags & RPC_X_OK) * X_OK);
}
#endif
