/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/poll.h.
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
 
#ifndef __TE_RPC_SYS_POLL_H__
#define __TE_RPC_SYS_POLL_H__

#include "te_rpc_defs.h"


typedef enum rpc_poll_event {
    RPC_POLLIN       = 0x0001, /**< There is data to read */
    RPC_POLLPRI      = 0x0002, /**< There is urgent data to read */
    RPC_POLLOUT      = 0x0004, /**< Writing now will not block */
    RPC_POLLERR      = 0x0008, /**< Error condition */
    RPC_POLLHUP      = 0x0010, /**< Hung up */
    RPC_POLLNVAL     = 0x0020, /**< Invalid request: fd not open */
    RPC_POLL_UNKNOWN = 0x0040  /**< Invalid poll event */
} rpc_poll_event;

/** Invalid poll evend */
#define POLL_UNKNOWN 0xFFFF

/** All known poll events */
#define RPC_POLL_ALL        (RPC_POLLIN | RPC_POLLPRI | RPC_POLLOUT | \
                             RPC_POLLERR | RPC_POLLHUP | RPC_POLLNVAL)

/** List of mapping numerical value to string for 'rpc_poll_event' */
#define POLL_EVENT_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(POLLIN), \
            RPC_BIT_MAP_ENTRY(POLLPRI), \
            RPC_BIT_MAP_ENTRY(POLLOUT), \
            RPC_BIT_MAP_ENTRY(POLLERR), \
            RPC_BIT_MAP_ENTRY(POLLHUP), \
            RPC_BIT_MAP_ENTRY(POLLNVAL), \
            RPC_BIT_MAP_ENTRY(POLL_UNKNOWN)

/**
 * poll_event_rpc2str()
 */
RPCBITMAP2STR(poll_event, POLL_EVENT_MAPPING_LIST)


#ifdef POLLIN

/** All known poll events */
#define POLL_ALL        (POLLIN | POLLPRI | POLLOUT | \
                         POLLERR | POLLHUP | POLLNVAL)

static inline short
poll_event_rpc2h(rpc_poll_event events)
{
    if ((events & ~RPC_POLL_ALL) != 0)
        return POLL_UNKNOWN;
        
    return (!!(events & RPC_POLLIN) * POLLIN) |
           (!!(events & RPC_POLLPRI) * POLLPRI) |
           (!!(events & RPC_POLLOUT) * POLLOUT) |
           (!!(events & RPC_POLLERR) * POLLERR) |
           (!!(events & RPC_POLLHUP) * POLLHUP) |
           (!!(events & RPC_POLLNVAL) * POLLNVAL);
}

static inline rpc_poll_event
poll_event_h2rpc(short events)
{
    return (!!(events & POLLIN) * RPC_POLLIN) |
           (!!(events & POLLPRI) * RPC_POLLPRI) |
           (!!(events & POLLOUT) * RPC_POLLOUT) |
           (!!(events & POLLERR) * RPC_POLLERR) |
           (!!(events & POLLHUP) * RPC_POLLHUP) |
           (!!(events & POLLNVAL) * RPC_POLLNVAL) |
           (!!(events & ~POLL_ALL) * RPC_POLL_UNKNOWN);
}

/** Maximum number of file descriptors passed to the poll */
#define RPC_POLL_NFDS_MAX       64

#endif /* POLLIN */

#endif /* !__TE_RPC_SYS_POLL_H__ */
