/** @file
 * @brief IPC library
 *
 * Common routines for IPC client and server.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_errno.h"

#include "ipc_internal.h"


/* See description in ipc_internal.h */
int
ipc_remember_datagram(struct ipc_datagrams *p_pool, void *data, size_t len,
                      struct sockaddr_un *addr, size_t addr_len)
{
    struct ipc_datagram *p;

    assert(p_pool != NULL);
    assert(data != NULL);
    assert(len > 0);
    assert(len <= IPC_SEGMENT_SIZE);
    assert(addr != NULL);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_IPC, ENOMEM);

    p->buffer = data;
    p->octets = len;
    p->sa     = *addr;
    p->sa_len = addr_len;

    TAILQ_INSERT_TAIL(p_pool, p, links);

    return 0;
}
