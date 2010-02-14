/** @file
 * @brief ACSE TCP Listener Dispatcher
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

#define CONN_IS_LIST 0

typedef struct conn_data_t {
#if CONN_IS_LIST
    LIST_ENTRY(conn_data_t) links;
#endif /* CONN_IS_LIST */
    int              socket;  /**< TCP listening socket. */
    struct sockaddr *addr;    /**< Network TCP address, which @p socket 
                                      is bound. */

    acs_t      **acs_objects;  /**< ACS objects listening on this 
                                    TCP address. */
    int          acs_number;   /**< number of ACS objects. */ 
} conn_data_t;

#if CONN_IS_LIST
static LIST_HEAD(conn_list_t, conn_data_t)
        conn_list = LIST_HEAD_INITIALIZER(&conn_list); 
#endif /* CONN_IS_LIST */ 

te_errno
conn_before_poll(void *data, struct pollfd *pfd)
{
    conn_data_t *conn = data;

    pfd->fd = conn->socket;
    pfd->events = POLLIN;
    pfd->revents = 0;

    return 0;
}

te_errno
conn_after_poll(void *data, struct pollfd *pfd)
{
    conn_data_t *conn = data;
    struct sockaddr_storage remote_addr;
    socklen_t addr_len;
    int sock_acc;
    int i;

    if (!(pfd->revents & POLLIN))
        return 0;

    sock_acc = accept(conn->socket, SA(&remote_addr), &addr_len);
    if (sock_acc < 0)
    {
        perror("CWMP connection accept failed!");
        return TE_RC(TE_ACSE, errno);
    }

    for (i = 0; i < conn->acs_number; i++)
    {
        te_errno rc;

        if (!(conn->acs_objects[i]->enabled))
            continue;

        rc = cwmp_accept_cpe_connection(conn->acs_objects[i], sock_acc); 
        RING("%s: cwmp_accept_cpe rc %r", __FUNCTION__, rc);

        switch (rc)
        {
            case 0: /* Stop processing */ return 0;
            case TE_ECONNREFUSED: continue;
            default:
                WARN("check accepted socket fails, %r", rc);
                return TE_RC(TE_ACSE, rc);
        }
    }
    /* No ACS found, which accept this connection */
    close(sock_acc);

    return 0;
}

te_errno
conn_destroy(void *data)
{
    conn_data_t *conn = data;
    /* TODO: release all */

    free(conn);
    return 0;
}


te_errno
conn_register_acs(acs_t *acs)
{
    conn_data_t *new_conn;

    if (acs == NULL || acs->addr_listen == NULL)
        return TE_RC(TE_ACSE, TE_EINVAL);

    /* TODO: check presense of ACS with same address in the list,
     * if present, do not create new record, but add to the found one. */


    new_conn = malloc(sizeof(*new_conn));
    do 
    {
        channel_t *new_ch; 

        new_conn->socket =
            socket(acs->addr_listen->sa_family, SOCK_STREAM, 0);
        if (new_conn->socket < 0)
        {
            ERROR("fail new socket");
            break;
        }

        if (bind(new_conn->socket, acs->addr_listen, acs->addr_len) < 0)
        {
            ERROR("fail bind socket");
            break;
        }

        if (listen(new_conn->socket, 10) < 0)
        {
            ERROR("fail listen socket");
            break;
        }

        new_conn->addr = malloc(acs->addr_len);
        memcpy(new_conn->addr, acs->addr_listen, acs->addr_len);
        new_conn->acs_objects = malloc(sizeof(acs_t *));
        new_conn->acs_objects[0] = acs;
        new_conn->acs_number = 1;
        acs->enabled = TRUE;
#if CONN_IS_LIST
        LIST_INSERT_HEAD(&conn_list, new_conn, links);
#endif /* CONN_IS_LIST */ 

        new_ch = malloc(sizeof(*new_ch)); 
        new_ch->data = new_conn;
        new_ch->before_poll = conn_before_poll;
        new_ch->after_poll = conn_after_poll;
        new_ch->destroy = conn_destroy;

        acse_add_channel(new_ch);

        return 0;
    } while (0);

    perror("Register ACSE");
    if (new_conn->socket > 0)
        close(new_conn->socket);
    free(new_conn);
    return TE_RC(TE_ACSE, errno);
}


extern te_errno
acse_conn_create(void)
{
    return 0;
}
