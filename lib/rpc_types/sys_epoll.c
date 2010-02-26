/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/epoll.h.
 * 
 * 
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2010 Level5 Networks Corp.
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

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_POLL_H
#include <sys/epoll.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_sys_epoll.h"

/** Invalid epoll evend */
#define EPOLL_UNKNOWN 0xFFFFFFFF

/** All known poll events */
#define EPOLL_ALL        (EPOLLIN | EPOLLPRI | EPOLLOUT | \
                          EPOLLRDNORM | EPOLLWRNORM | \
                          EPOLLRDBAND | EPOLLWRBAND | \
                          EPOLLMSG | EPOLLERR | EPOLLHUP | \
                          EPOLLET)

uint32_t
epoll_event_rpc2h(uint32_t events)
{
    if ((events & ~RPC_EPOLL_ALL) != 0)
        return EPOLL_UNKNOWN;
        
    return (!!(events & RPC_EPOLLIN) * EPOLLIN) |
           (!!(events & RPC_EPOLLPRI) * EPOLLPRI) |
           (!!(events & RPC_EPOLLOUT) * EPOLLOUT) |
           (!!(events & RPC_EPOLLRDNORM) * EPOLLRDNORM) |
           (!!(events & RPC_EPOLLWRNORM) * EPOLLWRNORM) |
           (!!(events & RPC_EPOLLRDBAND) * EPOLLRDBAND) |
           (!!(events & RPC_EPOLLWRBAND) * EPOLLWRBAND) |
           (!!(events & RPC_EPOLLMSG) * EPOLLMSG) |
           (!!(events & RPC_EPOLLERR) * EPOLLERR) |
           (!!(events & RPC_EPOLLHUP) * EPOLLHUP) |
           (!!(events & RPC_EPOLLET) * EPOLLET);
}

uint32_t
epoll_event_h2rpc(uint32_t events)
{
    return (!!(events & ~EPOLL_ALL) * RPC_EPOLL_UNKNOWN)
           | (!!(events & EPOLLIN) * RPC_EPOLLIN)
           | (!!(events & EPOLLPRI) * RPC_EPOLLPRI)
           | (!!(events & EPOLLOUT) * RPC_EPOLLOUT)
           | (!!(events & EPOLLRDNORM) * RPC_EPOLLRDNORM)
           | (!!(events & EPOLLWRNORM) * RPC_EPOLLWRNORM)
           | (!!(events & EPOLLRDBAND) * RPC_EPOLLRDBAND)
           | (!!(events & EPOLLWRBAND) * RPC_EPOLLWRBAND)
           | (!!(events & EPOLLMSG) * RPC_EPOLLMSG)
           | (!!(events & EPOLLERR) * RPC_EPOLLERR)
           | (!!(events & EPOLLHUP) * RPC_EPOLLHUP)
           | (!!(events & EPOLLET) * RPC_EPOLLET)
           ;
}
