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
#ifndef WINDOWS
#include "tarpc.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of file descriptors passed to the poll */
#define RPC_POLL_NFDS_MAX       64


typedef enum rpc_poll_event {
    RPC_POLLIN       = 0x0001, /**< There is data to read */
    RPC_POLLPRI      = 0x0002, /**< There is urgent data to read */
    RPC_POLLOUT      = 0x0004, /**< Writing now will not block */
    RPC_POLLRDNORM   = 0x0008, /**< Normal data is readable */
    RPC_POLLWRNORM   = 0x0010, /**< Normal data is writeable */
    RPC_POLLRDBAND   = 0x0020, /**< Out-of-band data is readable */
    RPC_POLLWRBAND   = 0x0040, /**< Out-of-band data is writeable */
    RPC_POLLERR      = 0x0080, /**< Error condition */
    RPC_POLLHUP      = 0x0100, /**< Hung up */
    RPC_POLLNVAL     = 0x0200, /**< Invalid request: fd not open */
    RPC_POLL_UNKNOWN = 0x0400  /**< Invalid poll event */
} rpc_poll_event;

/** All known poll events */
#define RPC_POLL_ALL        (RPC_POLLIN | RPC_POLLPRI | RPC_POLLOUT | \
                             RPC_POLLRDNORM | RPC_POLLWRNORM | \
                             RPC_POLLRDBAND | RPC_POLLWRBAND | \
                             RPC_POLLERR | RPC_POLLHUP | RPC_POLLNVAL)

/** List of mapping numerical value to string for 'rpc_poll_event' */
#define POLL_EVENT_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(POLLIN), \
            RPC_BIT_MAP_ENTRY(POLLPRI), \
            RPC_BIT_MAP_ENTRY(POLLOUT), \
            RPC_BIT_MAP_ENTRY(POLLRDNORM), \
            RPC_BIT_MAP_ENTRY(POLLWRNORM), \
            RPC_BIT_MAP_ENTRY(POLLRDBAND), \
            RPC_BIT_MAP_ENTRY(POLLWRBAND), \
            RPC_BIT_MAP_ENTRY(POLLERR), \
            RPC_BIT_MAP_ENTRY(POLLHUP), \
            RPC_BIT_MAP_ENTRY(POLLNVAL), \
            RPC_BIT_MAP_ENTRY(POLL_UNKNOWN)

/**
 * poll_event_rpc2str()
 */
RPCBITMAP2STR(poll_event, POLL_EVENT_MAPPING_LIST)

extern unsigned int poll_event_rpc2h(unsigned int events);

extern unsigned int poll_event_h2rpc(unsigned int events);


static inline const char *
iomux2str(iomux_func iomux)
{
    switch (iomux)
    {
        case FUNC_SELECT:
            return "select";
        case FUNC_PSELECT:
            return "pselect";
        case FUNC_POLL:
            return "poll";
        case FUNC_EPOLL:
            return "epoll";
        case FUNC_EPOLL_PWAIT:
            return "epoll_pwait";
        default:
            return "<unknown>";
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_POLL_H__ */
