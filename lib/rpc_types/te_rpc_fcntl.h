/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from fcntl.h.
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
 
#ifndef __TE_RPC_FCNTL_H__
#define __TE_RPC_FCNTL_H__

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_rpc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * All known file control flags.
 */
typedef enum rpc_fcntl_flags {
    RPC_O_RDONLY    = 0x00000001,
    RPC_O_WRONLY    = 0x00000002,
    RPC_O_RDWR      = 0x00000004,
    RPC_O_CREAT     = 0x00000008,
    RPC_O_EXCL      = 0x00000010,
    RPC_O_NOCTTY    = 0x00000020,
    RPC_O_TRUNC     = 0x00000040,
    RPC_O_APPEND    = 0x00000080,
    RPC_O_NONBLOCK  = 0x00000100,
    RPC_O_NDELAY    = 0x00000200,
    RPC_O_SYNC      = 0x00000400,
    RPC_O_FSYNC     = 0x00000800,
    RPC_O_ASYNC     = 0x00001000,
    RPC_O_DIRECT    = 0x00002000,
    RPC_O_DIRECTORY = 0x00004000,
    RPC_O_NOFOLLOW  = 0x00008000,
    RPC_O_DSYNC     = 0x00010000,
    RPC_O_RSYNC     = 0x00020000,
    RPC_O_LARGEFILE = 0x00040000,
    RPC_O_CLOEXEC   = 0x00080000,
} rpc_fcntl_flags;

/** Access mode */
#define RPC_O_ACCMODE   (RPC_O_RDONLY | RPC_O_WRONLY | RPC_O_RDWR)

#define FCNTL_FLAGS_MAPPING_LIST_AUX \
    RPC_BIT_MAP_ENTRY(O_RDONLY), \
    RPC_BIT_MAP_ENTRY(O_WRONLY), \
    RPC_BIT_MAP_ENTRY(O_RDWR), \
    RPC_BIT_MAP_ENTRY(O_CREAT), \
    RPC_BIT_MAP_ENTRY(O_EXCL), \
    RPC_BIT_MAP_ENTRY(O_NOCTTY), \
    RPC_BIT_MAP_ENTRY(O_TRUNC), \
    RPC_BIT_MAP_ENTRY(O_APPEND), \
    RPC_BIT_MAP_ENTRY(O_NONBLOCK), \
    RPC_BIT_MAP_ENTRY(O_NDELAY), \
    RPC_BIT_MAP_ENTRY(O_SYNC), \
    RPC_BIT_MAP_ENTRY(O_FSYNC), \
    RPC_BIT_MAP_ENTRY(O_ASYNC), \
    RPC_BIT_MAP_ENTRY(O_DIRECT), \
    RPC_BIT_MAP_ENTRY(O_DIRECTORY), \
    RPC_BIT_MAP_ENTRY(O_NOFOLLOW), \
    RPC_BIT_MAP_ENTRY(O_DSYNC), \
    RPC_BIT_MAP_ENTRY(O_RSYNC), \
    RPC_BIT_MAP_ENTRY(O_LARGEFILE)

#ifdef O_CLOEXEC
#define FCNTL_FLAGS_MAPPING_LIST \
    FCNTL_FLAGS_MAPPING_LIST_AUX, \
    RPC_BIT_MAP_ENTRY(O_CLOEXEC)
#else
#define FCNTL_FLAGS_MAPPING_LIST \
    FCNTL_FLAGS_MAPPING_LIST_AUX
#endif

/**
 * fcntl_flags_rpc2str()
 */
RPCBITMAP2STR(fcntl_flags, FCNTL_FLAGS_MAPPING_LIST)

/** Convert RPC file control flags to native flags */
extern unsigned int fcntl_flags_rpc2h(unsigned int flags);

/** Convert file control native flags to RPC flags */
extern unsigned int fcntl_flags_h2rpc(unsigned int flags);


typedef enum rpc_fcntl_command {
    RPC_F_DUPFD,
    RPC_F_DUPFD_CLOEXEC,
    RPC_F_GETFD,
    RPC_F_SETFD,
    RPC_F_GETFL,
    RPC_F_SETFL,
    RPC_F_GETLK,
    RPC_F_SETLK,
    RPC_F_SETLKW,
    RPC_F_SETOWN,
    RPC_F_GETOWN,
    RPC_F_SETOWN_EX,
    RPC_F_GETOWN_EX,
    RPC_F_SETSIG,
    RPC_F_GETSIG,
    RPC_F_SETLEASE,
    RPC_F_GETLEASE,
    RPC_F_NOTIFY,
    RPC_F_UNKNOWN
} rpc_fcntl_command;

#define F_UNKNOWN        0xFFFFFFFF

extern const char * fcntl_rpc2str(rpc_fcntl_command cmd);

/** Convert RPC fcntl commands to native ones. */
extern int fcntl_rpc2h(rpc_fcntl_command cmd);


/**
 * Seek modes
 */
typedef enum rpc_lseek_mode {
    RPC_SEEK_SET,
    RPC_SEEK_CUR,
    RPC_SEEK_END,
    RPC_SEEK_INVALID = -1
} rpc_lseek_mode;

extern const char * lseek_mode_rpc2str(rpc_lseek_mode mode);

extern int lseek_mode_rpc2h(rpc_lseek_mode mode);

extern rpc_lseek_mode lseek_mode_h2rpc(int mode);


/**
 * Splice flags
 */
typedef enum rpc_splice_flags {
    RPC_SPLICE_F_MOVE     = 0x1,
    RPC_SPLICE_F_NONBLOCK = 0x2,
    RPC_SPLICE_F_MORE     = 0x4,
    RPC_SPLICE_F_GIFT     = 0x8,
} rpc_splice_flags;

#define SPLICE_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(SPLICE_F_MOVE), \
    RPC_BIT_MAP_ENTRY(SPLICE_F_NONBLOCK), \
    RPC_BIT_MAP_ENTRY(SPLICE_F_MORE), \
    RPC_BIT_MAP_ENTRY(SPLICE_F_GIFT)

/**
 * splice_flags_rpc2str()
 */
RPCBITMAP2STR(splice_flags, FCNTL_FLAGS_MAPPING_LIST)

extern unsigned int splice_flags_rpc2h(rpc_splice_flags mode);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_FCNTL_H__ */
