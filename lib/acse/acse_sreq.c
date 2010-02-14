/** @file
 * @brief ACSE Connection Requester
 *
 * ACS Emulator support
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stddef.h>
#include<string.h>

#include "acse_internal.h"

#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"

typedef struct srec_data_t {
    LIST_ENTRY(srec_data_t) links;

    int              socket;  /**< TCP listening socket. */
    struct sockaddr *addr;    /**< Network TCP address, which @p socket 
                                      is bound. */

    cpe_t       *cpe_item;    /**< CPE - object of Connection Request */
} sreq_data_t;

static LIST_HEAD(sreq_list_t, sreq_data_t)
        sreq_list = LIST_HEAD_INITIALIZER(&sreq_list); 


te_errno
sreq_before_poll(void *data, struct pollfd *pfd)
{
    sreq_data_t *sreq = data;

    pfd->fd = sreq->socket;
    pfd->events = POLLIN;
    pfd->revents = 0;

    return 0;
}

te_errno
sreq_after_poll(void *data, struct pollfd *pfd)
{
    if (!(pfd->revents & POLLIN))
        return 0;
    /* TODO */

    return 0;
}


te_errno
sreq_destroy(void *data)
{
    sreq_data_t *sreq = data;
    /* TODO: release all */

    free(sreq);
    return 0;
}


extern te_errno
acse_init_connection_request(cpe_t *cpe_item)
{
    sreq_data_t *sreq_data = malloc(sizeof(*sreq_data));

    channel_t *channel;

    sreq_data->cpe_item = cpe_item;
    sreq_data->addr = malloc(sizeof(struct sockaddr_in));
    sreq_data->socket = -1; /* TODO !! */

    channel = malloc(sizeof(*channel)); 
    channel->data = sreq_data;
    channel->before_poll = sreq_before_poll;
    channel->after_poll = sreq_after_poll;
    channel->destroy = sreq_destroy;

    acse_add_channel(channel);


    return 0;
}

