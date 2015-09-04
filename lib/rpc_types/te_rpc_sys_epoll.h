/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/epoll.h.
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
 * @author Yurij Plotnikov <Yurij.Plotnikov@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_SYS_EPOLL_H__
#define __TE_RPC_SYS_EPOLL_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Valid opcodes ( "op" parameter ) to issue to epoll_ctl() */
#define RPC_EPOLL_CTL_ADD 1
#define RPC_EPOLL_CTL_DEL 2
#define RPC_EPOLL_CTL_MOD 3

typedef enum rpc_epoll_flags {
    RPC_EPOLL_CLOEXEC      = 0x1,
    RPC_EPOLL_NONBLOCK     = 0x2,
    RPC_EPOLL_FLAG_UNKNOWN = 0x4
} rpc_epoll_flags;

typedef enum rpc_epoll_evt {
    RPC_EPOLLIN      = 0x001,
    RPC_EPOLLPRI     = 0x002,
    RPC_EPOLLOUT     = 0x004,

    RPC_EPOLLRDNORM  = 0x040,
    RPC_EPOLLRDBAND  = 0x080,
    RPC_EPOLLWRNORM  = 0x100,
    RPC_EPOLLWRBAND  = 0x200,

    RPC_EPOLLMSG     = 0x400,

    RPC_EPOLLERR     = 0x008,
    RPC_EPOLLHUP     = 0x010,

    RPC_EPOLLRDHUP   = 0x2000,

    RPC_EPOLL_UNKNOWN = 0x800,  /**< Invalid poll event */

    RPC_EPOLLONESHOT = (1 << 30),
    RPC_EPOLLET      = (1 << 31)
} rpc_epoll_evt;

/** All known poll events */
#define RPC_EPOLL_ALL        (RPC_EPOLLIN | RPC_EPOLLPRI | RPC_EPOLLOUT | \
                              RPC_EPOLLRDNORM | RPC_EPOLLWRNORM | \
                              RPC_EPOLLRDBAND | RPC_EPOLLWRBAND | \
                              RPC_EPOLLMSG | \
                              RPC_EPOLLERR | RPC_EPOLLHUP | \
                              RPC_EPOLLRDHUP | \
                              RPC_EPOLLONESHOT | RPC_EPOLLET)

/** All known epoll flags */
#define RPC_EPOLL_FLAGS_ALL (RPC_EPOLL_CLOEXEC | RPC_EPOLL_NONBLOCK)

/** List of mapping numerical value to string for 'rpc_poll_event' */
#define EPOLL_EVENT_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(EPOLLIN), \
            RPC_BIT_MAP_ENTRY(EPOLLPRI), \
            RPC_BIT_MAP_ENTRY(EPOLLOUT), \
            RPC_BIT_MAP_ENTRY(EPOLLRDNORM), \
            RPC_BIT_MAP_ENTRY(EPOLLWRNORM), \
            RPC_BIT_MAP_ENTRY(EPOLLRDBAND), \
            RPC_BIT_MAP_ENTRY(EPOLLWRBAND), \
            RPC_BIT_MAP_ENTRY(EPOLLERR), \
            RPC_BIT_MAP_ENTRY(EPOLLHUP), \
            RPC_BIT_MAP_ENTRY(EPOLLRDHUP), \
            RPC_BIT_MAP_ENTRY(EPOLLMSG), \
            RPC_BIT_MAP_ENTRY(EPOLLONESHOT), \
            RPC_BIT_MAP_ENTRY(EPOLLET), \
            RPC_BIT_MAP_ENTRY(EPOLL_UNKNOWN)

#define EPOLL_FLAG_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(EPOLL_CLOEXEC), \
            RPC_BIT_MAP_ENTRY(EPOLL_NONBLOCK), \
            RPC_BIT_MAP_ENTRY(EPOLL_FLAG_UNKNOWN)

/**
 * epoll_event_rpc2str()
 */
RPCBITMAP2STR(epoll_event, EPOLL_EVENT_MAPPING_LIST)

RPCBITMAP2STR(epoll_flags, EPOLL_FLAG_MAPPING_LIST)

static inline char *
rpc_epoll_ctl_op2str(int op)
{
    switch (op) {
        case RPC_EPOLL_CTL_ADD: return "add";
        case RPC_EPOLL_CTL_DEL: return "del";
        case RPC_EPOLL_CTL_MOD: return "mod";
    }
    return "unknown";
}

extern unsigned int epoll_event_rpc2h(unsigned int events);

extern unsigned int epoll_event_h2rpc(unsigned int events);

extern unsigned int epoll_flags_rpc2h(unsigned int flags);

extern unsigned int epoll_flags_h2rpc(unsigned int flags);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_EPOLL_H__ */
