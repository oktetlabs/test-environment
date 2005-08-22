/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from fcntl.h.
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
 
#ifndef __TE_RPC_FCNTL_H__
#define __TE_RPC_FCNTL_H__

#include "te_rpc_defs.h"


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
    RPC_O_DIRECY    = 0x00002000,
    RPC_O_DIRECTORY = 0x00004000,
    RPC_O_NOFOLLOW  = 0x00008000,
    RPC_O_DSYNC     = 0x00010000,
    RPC_O_RSYNC     = 0x00020000,
    RPC_O_LARGEFILE = 0x00040000,
} rpc_fcntl_flags;

/** Access mode */
#define RPC_O_ACCMODE   (RPC_O_RDONLY | RPC_O_WRONLY | RPC_O_RDWR)


#define FCNTL_FLAGS_MAPPING_LIST \
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
    RPC_BIT_MAP_ENTRY(O_DIRECY), \
    RPC_BIT_MAP_ENTRY(O_DIRECTORY), \
    RPC_BIT_MAP_ENTRY(O_NOFOLLOW), \
    RPC_BIT_MAP_ENTRY(O_DSYNC), \
    RPC_BIT_MAP_ENTRY(O_RSYNC), \
    RPC_BIT_MAP_ENTRY(O_LARGEFILE)

/**
 * fcntl_flags_rpc2str()
 */
RPCBITMAP2STR(fcntl_flags, FCNTL_FLAGS_MAPPING_LIST)

/** Convert RPC file control flags to native flags */
static inline int
fcntl_flags_rpc2h(unsigned int flags)
{
    return 0
#ifdef O_RDONLY
        | (!!(flags & RPC_O_RDONLY) * O_RDONLY)
#endif
#ifdef O_WRONLY
        | (!!(flags & RPC_O_WRONLY) * O_WRONLY)
#endif
#ifdef O_RDWR
        | (!!(flags & RPC_O_RDWR) * O_RDWR)
#endif
#ifdef O_CREAT
        | (!!(flags & RPC_O_CREAT) * O_CREAT)
#endif
#ifdef O_EXCL
        | (!!(flags & RPC_O_EXCL) * O_EXCL)
#endif
#ifdef O_NOCTTY
        | (!!(flags & RPC_O_NOCTTY) * O_NOCTTY)
#endif
#ifdef O_TRUNC
        | (!!(flags & RPC_O_TRUNC) * O_TRUNC)
#endif
#ifdef O_APPEND
        | (!!(flags & RPC_O_APPEND) * O_APPEND)
#endif
#ifdef O_NONBLOCK
        | (!!(flags & RPC_O_NONBLOCK) * O_NONBLOCK)
#endif
#ifdef O_NDELAY
        | (!!(flags & RPC_O_NDELAY) * O_NDELAY)
#endif
#ifdef O_SYNC
        | (!!(flags & RPC_O_SYNC) * O_SYNC)
#endif
#ifdef O_FSYNC
        | (!!(flags & RPC_O_FSYNC) * O_FSYNC)
#endif
#ifdef O_ASYNC
        | (!!(flags & RPC_O_ASYNC) * O_ASYNC)
#endif
#ifdef O_DIRECY
        | (!!(flags & RPC_O_DIRECY) * O_DIRECY)
#endif
#ifdef O_DIRECTORY
        | (!!(flags & RPC_O_DIRECTORY) * O_DIRECTORY)
#endif
#ifdef O_NOFOLLOW
        | (!!(flags & RPC_O_NOFOLLOW) * O_NOFOLLOW)
#endif
#ifdef O_DSYNC
        | (!!(flags & RPC_O_DSYNC) * O_DSYNC)
#endif
#ifdef O_RSYNC
        | (!!(flags & RPC_O_RSYNC) * O_RSYNC)
#endif
#ifdef O_LARGEFILE
        | (!!(flags & RPC_O_LARGEFILE) * O_LARGEFILE)
#endif
        ;
}


typedef enum rpc_fcntl_command {
    RPC_F_DUPFD,
    RPC_F_GETFD,
    RPC_F_SETFD,
    RPC_F_GETFL,
    RPC_F_SETFL,
    RPC_F_GETLK,
    RPC_F_SETLK,
    RPC_F_SETLKW,
    RPC_F_SETOWN,
    RPC_F_GETOWN,
    RPC_F_SETSIG,
    RPC_F_GETSIG,
    RPC_F_SETLEASE,
    RPC_F_GETLEASE,
    RPC_F_NOTIFY,
    RPC_F_UNKNOWN
} rpc_fcntl_command;


#define F_UNKNOWN        0xFFFFFFFF


/** Convert RPC fcntl commands to native ones. */
static inline int
fcntl_rpc2h(rpc_fcntl_command cmd)
{
    switch(cmd)
    {
#ifdef F_DUPFD
        RPC2H(F_DUPFD);
#endif
#ifdef F_GETFD
        RPC2H(F_GETFD);
#endif
#ifdef F_SETFD
        RPC2H(F_SETFD);
#endif
#ifdef F_GETFL
        RPC2H(F_GETFL);
#endif
#ifdef F_SETFL
        RPC2H(F_SETFL);
#endif
#ifdef F_GETLK
        RPC2H(F_GETLK);
#endif
#ifdef F_SETLK
        RPC2H(F_SETLK);
#endif
#ifdef F_SETLKW
        RPC2H(F_SETLKW);
#endif
#ifdef F_GETOWN
        RPC2H(F_GETOWN);
#endif
#ifdef F_SETOWN
        RPC2H(F_SETOWN);
#endif
#ifdef F_GETSIG
        RPC2H(F_GETSIG);
#endif
#ifdef F_SETSIG
        RPC2H(F_SETSIG);
#endif
#ifdef F_GETLEASE
        RPC2H(F_GETLEASE);
#endif
#ifdef F_SETLEASE
        RPC2H(F_SETLEASE);
#endif
#ifdef F_NOTIFY
        RPC2H(F_NOTIFY);
#endif
        default: return F_UNKNOWN;
    }
}

static inline const char *
fcntl_rpc2str(rpc_fcntl_command cmd)
{
    switch (cmd)
    {
        RPC2STR(F_DUPFD);
        RPC2STR(F_GETFD);
        RPC2STR(F_SETFD);
        RPC2STR(F_GETFL);
        RPC2STR(F_SETFL);
        RPC2STR(F_GETLK);
        RPC2STR(F_SETLK);
        RPC2STR(F_SETLKW);
        RPC2STR(F_SETOWN);
        RPC2STR(F_GETOWN);
        RPC2STR(F_SETSIG);
        RPC2STR(F_GETSIG);
        RPC2STR(F_SETLEASE);
        RPC2STR(F_GETLEASE);
        RPC2STR(F_NOTIFY);
        default: return "F_UNKNOWN";
    }
}

#endif /* !__TE_RPC_FCNTL_H__ */
