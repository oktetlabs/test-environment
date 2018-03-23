/** @file
 * @brief IPC library
 *
 * Common routines for IPC client and server.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
        return TE_RC(TE_IPC, TE_ENOMEM);

    p->buffer = data;
    p->octets = len;
    p->sa     = *addr;
    p->sa_len = addr_len;

    TAILQ_INSERT_TAIL(p_pool, p, links);

    return 0;
}
