/** @file
 * @brief ACSE dispatcher framework
 *
 * ACS Emulator support. Processing of EPC in ACSE.
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "ACSE main loop"

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stddef.h>
#include<string.h>
#include<assert.h>

#include "acse_internal.h"

#include "te_errno.h"
#include "te_queue.h"
#include "logger_api.h"

/** The list of "channels" */
static LIST_HEAD(channel_list_t, channel_t)
        channel_list = LIST_HEAD_INITIALIZER(&channel_list); 
/** Number of channels */
static int channel_number = 0;

/* see description in acse_internal.h */
void
acse_clear_channels(void)
{
    channel_t *item;
    channel_t *tmp;

    LIST_FOREACH_SAFE(item, &channel_list, links, tmp)
    {
        (*item->destroy)(item->data);
        free(item);
    }
    LIST_INIT(&channel_list);
    channel_number = 0;
}

/* see description in acse_internal.h */
void
acse_add_channel(channel_t *ch_item)
{
    channel_t *ch_first = LIST_FIRST(&channel_list);
    assert(ch_item != NULL);
    /* First channel, if present, is EPC disp, it should remain first.*/
    if (NULL == ch_first)
        LIST_INSERT_HEAD(&channel_list, ch_item, links);
    else
        LIST_INSERT_AFTER(ch_first, ch_item, links);

    channel_number++;
}

/* see description in acse_internal.h */
void
acse_remove_channel(channel_t *ch_item)
{
    assert(ch_item != NULL);
    LIST_REMOVE(ch_item, links);
    channel_number--;
    ch_item->destroy(ch_item->data);
    free(ch_item);
}
/**
 * normal exit handler, clear all resourcess, close connections, etc.
 */
static void
acse_exit_handler(void)
{
    RING("Normal exit from CLI");
    acse_clear_channels();
    /* TODO close logger */
}

/* See description in acse.h */
void
acse_loop(void)
{
    atexit(acse_exit_handler);
    /* EPC pipe should be already established */

    while(acse_epc_socket() > 0)
    {
        int         r_poll;
        int         i;
        channel_t  *item;
        te_errno    rc;
        struct pollfd *pfd =
                calloc(channel_number, sizeof(struct pollfd));

        i = 0;
        LIST_FOREACH(item, &channel_list, links)
        {
            if ((*item->before_poll)(item->data, pfd + i) != 0)
            {
                /* TODO something? */
                break;
            }
            i++;
        }

        r_poll = poll(pfd, channel_number, -1);

        if (r_poll == -1)
        {
            perror("poll failed");
            /* TODO something? */
            break;
        }
        VERB("acse_loop, poll return %d", r_poll);

#if 0
        channel_t *last_item = LINK_LAST(
                                        &channel_list,
                                        channel_item_t,
                                        links);
        channel_t *tmp;
#endif

        i = 0;
        LIST_FOREACH(item, &channel_list, links)
        {
            VERB("acse_loop, process channel N %d", i);
            if (pfd[i].revents != 0)
            {
                rc = (*item->after_poll)(item->data, pfd + i);

                if (rc != 0)
                {
                    if (TE_RC_GET_ERROR(rc) != TE_ENOTCONN)
                        WARN("acse_loop, error on channel, rc %r", rc);
                    acse_remove_channel(item);
                    break; /* from LIST_FOREACH */
                }
            }

#if 0
            if (item == last_item)
                break;
#endif
            i++;
        }

        free(pfd);
    }
    acse_clear_channels();
}


