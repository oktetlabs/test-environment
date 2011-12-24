/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from fcntl.h.
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

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_defs.h"
#include "te_rpc_defs.h"
#include "te_rpc_fcntl.h"


/** Convert RPC file control flags to native flags */
unsigned int
fcntl_flags_rpc2h(unsigned int flags)
{
    UNUSED(flags);  /* Possibly unused */
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
#ifdef O_DIRECT
        | (!!(flags & RPC_O_DIRECT) * O_DIRECT)
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
#ifdef O_CLOEXEC
        | (!!(flags & RPC_O_CLOEXEC) * O_CLOEXEC)
#endif
        ;
}


/** Convert file control native flags to RPC flags */
unsigned int
fcntl_flags_h2rpc(unsigned int flags)
{
    UNUSED(flags);  /* Possibly unused */
    return 0
#ifdef O_RDONLY
        | (!!(flags & O_RDONLY) * RPC_O_RDONLY)
#endif
#ifdef O_WRONLY
        | (!!(flags & O_WRONLY) * RPC_O_WRONLY)
#endif
#ifdef O_RDWR
        | (!!(flags & O_RDWR) * RPC_O_RDWR)
#endif
#ifdef O_CREAT
        | (!!(flags & O_CREAT) * RPC_O_CREAT)
#endif
#ifdef O_EXCL
        | (!!(flags & O_EXCL) * RPC_O_EXCL)
#endif
#ifdef O_NOCTTY
        | (!!(flags & O_NOCTTY) * RPC_O_NOCTTY)
#endif
#ifdef O_TRUNC
        | (!!(flags & O_TRUNC) * RPC_O_TRUNC)
#endif
#ifdef O_APPEND
        | (!!(flags & O_APPEND) * RPC_O_APPEND)
#endif
#ifdef O_NONBLOCK
        | (!!(flags & O_NONBLOCK) * RPC_O_NONBLOCK)
#endif
#ifdef O_NDELAY
        | (!!(flags & O_NDELAY) * RPC_O_NDELAY)
#endif
#ifdef O_SYNC
        | (!!(flags & O_SYNC) * RPC_O_SYNC)
#endif
#ifdef O_FSYNC
        | (!!(flags & O_FSYNC) * RPC_O_FSYNC)
#endif
#ifdef O_ASYNC
        | (!!(flags & O_ASYNC) * RPC_O_ASYNC)
#endif
#ifdef O_DIRECT
        | (!!(flags & O_DIRECT) * RPC_O_DIRECT)
#endif
#ifdef O_DIRECTORY
        | (!!(flags & O_DIRECTORY) * RPC_O_DIRECTORY)
#endif
#ifdef O_NOFOLLOW
        | (!!(flags & O_NOFOLLOW) * RPC_O_NOFOLLOW)
#endif
#ifdef O_DSYNC
        | (!!(flags & O_DSYNC) * RPC_O_DSYNC)
#endif
#ifdef O_RSYNC
        | (!!(flags & O_RSYNC) * RPC_O_RSYNC)
#endif
#ifdef O_LARGEFILE
        | (!!(flags & O_LARGEFILE) * RPC_O_LARGEFILE)
#endif
#ifdef O_CLOEXEC
        | (!!(flags & O_CLOEXEC) * RPC_O_CLOEXEC)
#endif
        ;
}


const char *
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
        default: return "<F_UNKNOWN>";
    }
}


#define F_UNKNOWN        0xFFFFFFFF


/** Convert RPC fcntl commands to native ones. */
int
fcntl_rpc2h(rpc_fcntl_command cmd)
{
    switch(cmd)
    {
        RPC2H_CHECK(F_DUPFD);
        RPC2H_CHECK(F_DUPFD_CLOEXEC);
        RPC2H_CHECK(F_GETFD);
        RPC2H_CHECK(F_SETFD);
        RPC2H_CHECK(F_GETFL);
        RPC2H_CHECK(F_SETFL);
        RPC2H_CHECK(F_GETLK);
        RPC2H_CHECK(F_SETLK);
        RPC2H_CHECK(F_SETLKW);
        RPC2H_CHECK(F_GETOWN);
        RPC2H_CHECK(F_SETOWN);
        RPC2H_CHECK(F_GETSIG);
        RPC2H_CHECK(F_SETSIG);
        RPC2H_CHECK(F_GETLEASE);
        RPC2H_CHECK(F_SETLEASE);
        RPC2H_CHECK(F_NOTIFY);
        default: return F_UNKNOWN;
    }
}


const char *
lseek_mode_rpc2str(rpc_lseek_mode mode)
{
    switch (mode)
    {
        RPC2STR(SEEK_SET);
        RPC2STR(SEEK_CUR);
        RPC2STR(SEEK_END);
        default: return "invalid";
    }
}

int
lseek_mode_rpc2h(rpc_lseek_mode mode)
{
    switch (mode)
    {
#ifdef SEEK_SET
        RPC2H_CHECK(SEEK_SET);
#endif
#ifdef SEEK_CUR
        RPC2H_CHECK(SEEK_CUR);
#endif
#ifdef SEEK_END
        RPC2H_CHECK(SEEK_END);
#endif
        default: return -1;
    }    
}

rpc_lseek_mode
lseek_mode_h2rpc(int mode)
{
    switch (mode)
    {
#ifdef SEEK_SET
        H2RPC_CHECK(SEEK_SET);
#endif
#ifdef SEEK_CUR
        H2RPC_CHECK(SEEK_CUR);
#endif
#ifdef SEEK_END
        H2RPC_CHECK(SEEK_END);
#endif
        default: return RPC_SEEK_INVALID;
    }    
}
