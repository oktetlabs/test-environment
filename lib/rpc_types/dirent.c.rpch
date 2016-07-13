/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from dirent.h.
 * 
 * 
 * Copyright (C) 2013 Test Environment authors (see file AUTHORS
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "RPC types"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#include "te_defs.h"
#include "te_rpc_defs.h"
#include "logger_api.h"

#include "te_rpc_dirent.h"

#ifdef HAVE_STRUCT_DIRENT_D_TYPE
/** Convert RPC file type to native file type */
unsigned int
d_type_rpc2h(unsigned int type)
{
    switch (type)
    {
#define D_TYPE_RPC2H(type_) \
        case RPC_ ## type_: return type_

        D_TYPE_RPC2H(DT_UNKNOWN);
        D_TYPE_RPC2H(DT_FIFO);
        D_TYPE_RPC2H(DT_CHR);
        D_TYPE_RPC2H(DT_DIR);
        D_TYPE_RPC2H(DT_BLK);
        D_TYPE_RPC2H(DT_REG);
        D_TYPE_RPC2H(DT_LNK);
        D_TYPE_RPC2H(DT_SOCK);

#undef D_TYPE_RPC2H
    }
    return DT_UNKNOWN;
}
#endif /* HAVE_STRUCT_DIRENT_D_TYPE */

/** Convert file type native to RPC file type */
unsigned int
d_type_h2rpc(unsigned int type)
{
    UNUSED(type);

#ifdef HAVE_STRUCT_DIRENT_D_TYPE
    switch (type)
    {
#define D_TYPE_H2RPC(type_) \
        case type_: return RPC_ ## type_

        D_TYPE_H2RPC(DT_UNKNOWN);
        D_TYPE_H2RPC(DT_FIFO);
        D_TYPE_H2RPC(DT_CHR);
        D_TYPE_H2RPC(DT_DIR);
        D_TYPE_H2RPC(DT_BLK);
        D_TYPE_H2RPC(DT_REG);
        D_TYPE_H2RPC(DT_LNK);
        D_TYPE_H2RPC(DT_SOCK);

#undef D_TYPE_H2RPC
    }
    WARN("Not supported 'd_type' value %u", type);
#endif /* HAVE_STRUCT_DIRENT_D_TYPE */
    return RPC_DT_UNKNOWN;
}

const char *
d_type_rpc2str(rpc_d_type type)
{
    switch (type)
    {
#ifdef HAVE_STRUCT_DIRENT_D_TYPE
        RPC2STR(DT_UNKNOWN);
        RPC2STR(DT_FIFO);
        RPC2STR(DT_CHR);
        RPC2STR(DT_DIR);
        RPC2STR(DT_BLK);
        RPC2STR(DT_REG);
        RPC2STR(DT_LNK);
        RPC2STR(DT_SOCK);
#endif /* HAVE_STRUCT_DIRENT_D_TYPE */
        default: return "<DT_UNKNOWN>";
    }
}

