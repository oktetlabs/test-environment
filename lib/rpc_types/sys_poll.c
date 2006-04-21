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

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_sys_poll.h"


/** Invalid poll evend */
#define POLL_UNKNOWN 0xFFFF

#ifndef POLLRDNORM
#define POLLRDNORM  0
#endif
#ifndef POLLWRNORM
#define POLLWRNORM  0
#endif
#ifndef POLLRDBAND
#define POLLRDBAND  0
#endif
#ifndef POLLWRBAND
#define POLLWRBAND  0
#endif

/** All known poll events */
#define POLL_ALL        (POLLIN | POLLPRI | POLLOUT | \
                         POLLRDNORM | POLLWRNORM | \
                         POLLRDBAND | POLLWRBAND | \
                         POLLERR | POLLHUP | POLLNVAL)

unsigned int
poll_event_rpc2h(unsigned int events)
{
    if ((events & ~RPC_POLL_ALL) != 0)
        return POLL_UNKNOWN;
        
    return (!!(events & RPC_POLLIN) * POLLIN) |
           (!!(events & RPC_POLLPRI) * POLLPRI) |
           (!!(events & RPC_POLLOUT) * POLLOUT) |
           (!!(events & RPC_POLLRDNORM) * POLLRDNORM) |
           (!!(events & RPC_POLLWRNORM) * POLLWRNORM) |
           (!!(events & RPC_POLLRDBAND) * POLLRDBAND) |
           (!!(events & RPC_POLLWRBAND) * POLLWRBAND) |
           (!!(events & RPC_POLLERR) * POLLERR) |
           (!!(events & RPC_POLLHUP) * POLLHUP) |
           (!!(events & RPC_POLLNVAL) * POLLNVAL);
}

unsigned int
poll_event_h2rpc(unsigned int events)
{
    return (!!(events & ~POLL_ALL) * RPC_POLL_UNKNOWN)
           | (!!(events & POLLIN) * RPC_POLLIN)
           | (!!(events & POLLPRI) * RPC_POLLPRI)
           | (!!(events & POLLOUT) * RPC_POLLOUT)
           | (!!(events & POLLRDNORM) * RPC_POLLRDNORM)
           | (!!(events & POLLWRNORM) * RPC_POLLWRNORM)
           | (!!(events & POLLRDBAND) * RPC_POLLRDBAND)
           | (!!(events & POLLWRBAND) * RPC_POLLWRBAND)
           | (!!(events & POLLERR) * RPC_POLLERR)
           | (!!(events & POLLHUP) * RPC_POLLHUP)
           | (!!(events & POLLNVAL) * RPC_POLLNVAL)
           ;
}
