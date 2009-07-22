/** @file
 * @brief ACSE Session Requester
 *
 * ACS Emulator support
 *
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id: acse_sreq.c 45424 2007-12-18 11:01:12Z edward $
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stddef.h>
#include<string.h>

#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "acse_internal.h"

/** Session Requester state machine states */
typedef enum { want_read, want_write } sreq_t;

/** Session Requester state machine private data */
typedef struct {
    sreq_t state; /**< Session Requester state machine current state */
} sreq_data_t;

static te_errno
before_select(void *data, fd_set *rd_set, fd_set *wr_set, int *fd_max)
{
    UNUSED(data);
    UNUSED(rd_set);
    UNUSED(wr_set);
    UNUSED(fd_max);

    return 0;
}

static te_errno
after_select(void *data, fd_set *rd_set, fd_set *wr_set)
{
    UNUSED(data);
    UNUSED(rd_set);
    UNUSED(wr_set);

    return 0;
}

static te_errno
destroy(void *data)
{
    sreq_data_t *sreq = data;

    free(sreq);
    return 0;
}

static te_errno
recover_fds(void *data)
{
    UNUSED(data);
    return -1;
}

extern te_errno
acse_sreq_create(channel_t *channel)
{
    sreq_data_t *sreq = channel->data = malloc(sizeof *sreq);

    if (sreq == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    sreq->state            = want_read;

    channel->before_select = &before_select;
    channel->after_select  = &after_select;
    channel->destroy       = &destroy;
    channel->recover_fds   = &recover_fds;

    return 0;
}

