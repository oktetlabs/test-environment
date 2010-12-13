/** @file
 * @brief ACSE TCP Listener Dispatcher
 *
 * ACS Emulator support
 *
 *
 * Copyright (C) 2009-2010 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "ACSE TCP listener"

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

/** TCP Connection Listener descriptor.
 */
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
    channel_t   *own_channel;  /**< main loop channel ref. */
} conn_data_t;

#if CONN_IS_LIST
static LIST_HEAD(conn_list_t, conn_data_t)
        conn_list = LIST_HEAD_INITIALIZER(&conn_list); 
#endif /* CONN_IS_LIST */ 

/** 
 * Callback for I/O ACSE channel, called before poll() 
 * Its prototype matches with field #channel_t::before_poll.
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct (OUT)
 *
 * @return status code.
 */
te_errno
conn_before_poll(void *data, struct pollfd *pfd)
{
    conn_data_t *conn = data;

    pfd->fd = conn->socket;
    pfd->events = POLLIN;
    pfd->revents = 0;

    return 0;
}

/** 
 * Callback for I/O ACSE channel, called before poll() 
 * Its prototype matches with field #channel_t::after_poll.
 * This function should process detected event (usually, incoming data).
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct with marks, which 
 *                  event happen.
 *
 * @return status code.
 */
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

    addr_len = sizeof(remote_addr);
    memset(&remote_addr, 0, sizeof(remote_addr));

    sock_acc = accept(conn->socket, SA(&remote_addr), &addr_len);
    if (sock_acc < 0)
    {
        perror("CWMP connection accept failed!");
        return TE_RC(TE_ACSE, errno);
    }

    for (i = 0; i < conn->acs_number; i++)
    {
        te_errno rc;

        rc = cwmp_accept_cpe_connection(conn->acs_objects[i], sock_acc); 
        RING("conn_after_poll(): cwmp_accept_cpe rc %r", rc);

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

/** 
 * Callback for I/O ACSE channel, called at channel destroy. 
 * It fills @p pfd according with specific channel situation.
 * Its prototype matches with field #channel_t::destroy.
 *
 * @param data      Channel-specific private data.
 *
 * @return status code.
 */
te_errno
conn_destroy(void *data)
{
    conn_data_t *conn = data;
    close(conn->socket);
    free(conn->addr);
    free(conn->acs_objects);
    free(conn);
    return 0;
}


/* See description in acse_internal.h */
te_errno
conn_register_acs(acs_t *acs)
{
    conn_data_t *new_conn;
    int s_errno = 0;

    if (acs == NULL || acs->addr_listen == NULL)
        return TE_RC(TE_ACSE, TE_EINVAL);

    /* TODO: check presense of ACS with same address in the list,
     * if present, do not create new record, but add to the found one. */


    new_conn = malloc(sizeof(*new_conn));
    do 
    {
        int opt;
        channel_t *new_ch; 

        new_conn->socket =
            socket(acs->addr_listen->sa_family, SOCK_STREAM, 0);
        if (new_conn->socket < 0)
        {
            ERROR("%s(): fail new socket", __FUNCTION__);
            break;
        }

        opt = 1;
        if (setsockopt(new_conn->socket, SOL_SOCKET, SO_REUSEADDR, 
                       (void *)&opt, sizeof(opt)) != 0)
        {
            ERROR("%s(): fail sockopt SO_REUSEADDR", __FUNCTION__);
            break;
        }


        if (bind(new_conn->socket, acs->addr_listen, acs->addr_len) < 0)
        {
            ERROR("%s(): fail bind socket", __FUNCTION__);
            break;
        }

        if (listen(new_conn->socket, 10) < 0)
        {
            ERROR("%s(): fail listen socket", __FUNCTION__);
            break;
        }

        new_conn->addr = malloc(acs->addr_len);
        memcpy(new_conn->addr, acs->addr_listen, acs->addr_len);
        new_conn->acs_objects = malloc(sizeof(acs_t *));
        new_conn->acs_objects[0] = acs;
        new_conn->acs_number = 1;
        acs->conn_listen = new_conn;

#if CONN_IS_LIST
        LIST_INSERT_HEAD(&conn_list, new_conn, links);
#endif /* CONN_IS_LIST */ 

        new_ch = malloc(sizeof(*new_ch)); 
        new_ch->data = new_conn;
        new_ch->before_poll = conn_before_poll;
        new_ch->after_poll = conn_after_poll;
        new_ch->destroy = conn_destroy;

        new_conn->own_channel = new_ch;

        acse_add_channel(new_ch);

        RING("ACS '%s' registered to listen incoming connections, sock %d",
              acs->name, new_conn->socket);


        return 0;
    } while (0);
    s_errno = errno;

    perror("Register ACS");
    if (new_conn->socket > 0)
        close(new_conn->socket);
    free(new_conn);

    ERROR("Register ACS fail, OS errno %d(%s)", s_errno, strerror(s_errno));

    return TE_RC(TE_ACSE, s_errno);
}

te_errno
conn_deregister_acs(acs_t *acs)
{
    conn_data_t *conn;
    int i;
    if (NULL == acs || NULL == acs->conn_listen)
        return TE_EINVAL;
    conn = acs->conn_listen;
    for (i = 0; i < conn->acs_number; i++)
        if (acs == conn->acs_objects[i])
            break;
    if (i == conn->acs_number)
    {
        ERROR("%s(): generic fail, not found ACS ptr in conn descriptor",
              __FUNCTION__);
        return TE_EFAIL;
    }
    acs->conn_listen = NULL;
    conn->acs_objects[i] = NULL;
    conn->acs_number--;
    for (;i < conn->acs_number; i++)
        conn->acs_objects[i] = conn->acs_objects[i+1];

    if (0 == conn->acs_number)
        acse_remove_channel(conn->own_channel);

    return 0;
}

te_errno
acse_conn_create(void)
{
    return 0;
}
