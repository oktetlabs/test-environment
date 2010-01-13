/** @file
 * @brief ACSE Connection Dispatcher
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
 * $Id$
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

/** Connection Dispatcher state machine states */
typedef enum { want_read, want_write } conn_t;

/** Connection Dispatcher state machine private data */
typedef struct {
    conn_t state; /**< Session Requester state machine current state */
} conn_data_t;

static te_errno
before_select(void *data, fd_set *rd_set, fd_set *wr_set, int *fd_max)
{
    acs_item_t *item;

    UNUSED(data);
    UNUSED(rd_set);
    UNUSED(wr_set);
    UNUSED(fd_max);

    STAILQ_FOREACH(item, &acs_list, link)
    {
        if (item->acs.soap != NULL && item->acs.soap->master != -1)
        {
            FD_SET(item->acs.soap->master, rd_set);

            if (*fd_max < item->acs.soap->master + 1)
                *fd_max = item->acs.soap->master + 1;
        }
    }

    return 0;
}

static te_errno
after_select(void *data, fd_set *rd_set, fd_set *wr_set)
{
    acs_item_t *item;

    UNUSED(data);
    UNUSED(rd_set);
    UNUSED(wr_set);

    STAILQ_FOREACH(item, &acs_list, link)
    {
        if (item->acs.soap != NULL && item->acs.soap->master != -1)
        {
            if (FD_ISSET(item->acs.soap->master, rd_set))
            {
                soap_accept(item->acs.soap);

                if (soap_valid_socket(item->acs.soap->socket))
                {
                    /* FIXME: It's a debug message */
                    RING("Incoming SOAP connection, socket = %d",
                                item->acs.soap->socket);

                    /* FIXME: Here connection should be handled */
                    item->acs.soap->fclose(item->acs.soap);
                }

                soap_end(item->acs.soap);
            }
        }
    }

    return 0;
}

static te_errno
destroy(void *data)
{
    conn_data_t *conn = data;

    free(conn);
    return 0;
}

static te_errno
recover_fds(void *data)
{
    UNUSED(data);
    return -1;
}

extern te_errno
acse_conn_create(channel_t *channel)
{
    conn_data_t *conn = channel->data = malloc(sizeof *conn);

    if (conn == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    conn->state            = want_read;

    channel->before_select = &before_select;
    channel->after_select  = &after_select;
    channel->destroy       = &destroy;
    channel->recover_fds   = &recover_fds;

    return 0;
}
