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
 
#ifndef __TE_RPC_DIRENT_H__
#define __TE_RPC_DIRENT_H__

#include "te_rpc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * All known file control flags.
 */
typedef enum rpc_d_type {
    RPC_DT_UNKNOWN = 0,
    RPC_DT_FIFO = 1,
    RPC_DT_CHR = 2,
    RPC_DT_DIR = 4,
    RPC_DT_BLK = 8,
    RPC_DT_REG = 16,
    RPC_DT_LNK = 32,
    RPC_DT_SOCK = 64,
} rpc_d_type;

/** Convert RPC file type to native file type */
extern unsigned int d_type_rpc2h(unsigned int d_type);

/** Convert native file type to RPC file type */
extern unsigned int d_type_h2rpc(unsigned int d_type);

/** Convert RPC file type to string representation */
extern const char *d_type_rpc2str(rpc_d_type type);

/** Target system has 'd_type' field in struct dirent */
#define RPC_DIRENT_HAVE_D_TYPE   0x01
/** Target system has 'd_off' field in struct dirent */
#define RPC_DIRENT_HAVE_D_OFF    0x02
/** Target system has 'd_namelen' field in struct dirent */
#define RPC_DIRENT_HAVE_D_NAMLEN 0x04
/**
 * Target system has 'd_ino' field in struct dirent
 * (this field always present - it is mandatory). 
 */
#define RPC_DIRENT_HAVE_D_INO    0x08

#define DIRENT_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_TYPE), \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_OFF), \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_NAMLEN), \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_INO)

/**
 * dirent_props_rpc2str()
 */
RPCBITMAP2STR(dirent_props, DIRENT_FLAGS_MAPPING_LIST)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_DIRENT_H__ */

