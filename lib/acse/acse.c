/** @file
 * @brief ACSE dispatcher framework
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
 * $Id: acse.c 45424 2007-12-18 11:01:12Z edward $
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stddef.h>
#include<string.h>

#include "te_errno.h"
#include "te_queue.h"
#include "logger_api.h"
#include "acse_internal.h"

/** The list of "channels" */
static STAILQ_HEAD(channel_list_t, channel_item_t)
    channel_list = STAILQ_HEAD_INITIALIZER(&channel_list); 

static void
destroy_item(channel_item_t *item)
{
    (*item->channel.error_destroy)(item->channel.data);
}

static void
destroy_all_items(void)
{
    channel_item_t *item;
    channel_item_t *tmp;

    STAILQ_FOREACH_SAFE(item, &channel_list, link, tmp)
    {
        (*item->channel.error_destroy)(item->channel.data);
    }
}

static int
recover_on_select_failure(void)
{
    channel_item_t *item;
    channel_item_t *tmp;

    STAILQ_FOREACH_SAFE(item, &channel_list, link, tmp)
    {
        int    fd_max = 0;
        fd_set rd_set;
        fd_set wr_set;

        struct timeval t = { .tv_sec = 0, .tv_usec = 0 };

        FD_ZERO(&rd_set);
        FD_ZERO(&wr_set);

        if ((*item->channel.before_select)(item->channel.data,
                                           &rd_set, &wr_set,
                                           &fd_max) != 0 ||
            select(fd_max, &rd_set, &wr_set, NULL, &t) == -1)
        {
            switch (item->channel.type)
            {
                case lrpc_type:
                    destroy_all_items();
                    return -1;
                default:
                    destroy_item(item);
                    break;
            }
        }
    }

    return 0;
}

/**
 * See description in acse.h
 */
void acse_loop(params_t *params, int rd_fd, int wr_fd)
{
    channel_item_t *item = malloc(sizeof *item);

    if (item != NULL)
    {
        if (acse_lrpc_create(&item->channel, params, rd_fd, wr_fd) == 0)
        {
            STAILQ_INSERT_TAIL(&channel_list, item, link);

            RING("I am acse_loop, now from the ACSE library!!!");

            for(;;)
            {
                int        r;
                int        fd_max = 0;
                fd_set     rd_set;
                fd_set     wr_set;

                FD_ZERO(&rd_set);
                FD_ZERO(&wr_set);

                STAILQ_FOREACH(item, &channel_list, link)
                {
                    if ((*item->channel.before_select)(item->channel.data,
                                                       &rd_set, &wr_set,
                                                       &fd_max) != 0)
                    {
                        destroy_all_items();
                        return;
                    }
                }

                switch (r = select(fd_max, &rd_set, &wr_set, NULL, NULL))
                {
                    case -1:
                        if (recover_on_select_failure() != 0)
                            return;

                        break;
                    default:
                        STAILQ_FOREACH(item, &channel_list, link)
                        {
                            if ((*item->channel.after_select)(
                                    item->channel.data,
                                    &rd_set, &wr_set) != 0)
                            {
                                destroy_all_items();
                                return;
                            }

                            if (item == STAILQ_LAST(&channel_list,
                                                    channel_item_t, link))
                            {
                                break;
                            }
                        }

                        break;
                }
            }
        }
    }
}
