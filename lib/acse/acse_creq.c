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
#include "httpda.h"

#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"

typedef struct conn_req_t {
    struct soap     m_soap;
    cpe_t       *cpe_item;    /**< CPE - object of Connection Request */
} conn_req_t;



te_errno
conn_req_before_poll(void *data, struct pollfd *pfd)
{
    conn_req_t *conn_req = data;

    pfd->fd = conn_req->m_soap.socket;
    pfd->events = POLLIN;
    pfd->revents = 0;

    return 0;
}

te_errno
conn_req_after_poll(void *data, struct pollfd *pfd)
{
    conn_req_t *conn_req = data;
    struct soap *soap;
    struct http_da_info info;

    if (!(pfd->revents & POLLIN))
        return 0;

    soap = &(conn_req->m_soap);
    /* should not block after poll() */
    if (soap_begin_recv(soap))
    {
        if (soap->error == 401)
        {
            RING("ConnectionRequest, attempt failed, realm: '%s'\n",
                  soap->authrealm);
            /* save userid and passwd for basic or digest authentication */
            http_da_save(soap, &info, soap->authrealm,
                         conn_req->cpe_item->username,
                         conn_req->cpe_item->password);
            soap_begin_count(soap);
            soap_end_count(soap);

            info.qop = soap_strdup(soap, "auth");
            http_da_restore(soap, &info);
            soap_connect_command(soap, SOAP_GET,
                                 conn_req->cpe_item->url, "");
            soap_end_send(soap);
            return 0;
        }
        else
        {
            ERROR("Recv after Conn.Req., soap error %d", soap->error);
        }
    }
    else
        soap_end_recv(soap);


    RING("Recv after Conn req, status %d", soap->error);

    soap_closesock(soap);
    return TE_ENOTCONN;
}


te_errno
conn_req_destroy(void *data)
{
    conn_req_t *conn_req = data;

    soap_end(&(conn_req->m_soap));
    free(conn_req);
    return 0;
}


extern te_errno
acse_init_connection_request(cpe_t *cpe_item)
{
    conn_req_t *conn_req_data = malloc(sizeof(*conn_req_data));
    struct soap *soap;
    channel_t   *channel = malloc(sizeof(*channel)); 

    if (conn_req_data == NULL || channel == NULL)
        return TE_ENOMEM;

    soap_init(&(conn_req_data->m_soap));
    soap = &(conn_req_data->m_soap);

    soap_register_plugin(soap, http_da);

    soap_begin(soap);

    conn_req_data->cpe_item = cpe_item;


    soap_begin_count(soap);
    soap_end_count(soap);

    if (soap_connect_command(soap, SOAP_GET, cpe_item->url, ""))
    {
        soap_print_fault(soap, stderr); 
        ERROR("%s failed, soap error %d", __FUNCTION__, soap->error);
        soap_end(soap);
        free(conn_req_data);
        free(channel);
        return TE_EFAIL;
    } 

    channel->data = conn_req_data;
    channel->before_poll = conn_req_before_poll;
    channel->after_poll = conn_req_after_poll;
    channel->destroy = conn_req_destroy;

    acse_add_channel(channel);

    return 0;
}


#if 0
int
cwmp_SendConnectionRequest(const char *endpoint,
                           const char *username, 
                           const char *password)
{
    struct soap *soap = soap_new();
    struct http_da_info info;

    soap_register_plugin(soap, http_da);

    soap_begin(soap);
    soap_begin_count(soap);
    soap_end_count(soap);

    if (soap_connect_command(soap, SOAP_GET, endpoint, "")
        || soap_end_send(soap))
    {
        soap_print_fault(soap, stderr); 
        return 1;
    } 
    if (soap_begin_recv(soap))
    {
        if (soap->error == 401)
        {
            char local_realm[100];
            // soap_end_recv(soap);
            printf("UUUUUUUU first attempt failed, AUTH, realm: '%s'\n",
                  soap->authrealm);
            strcpy(local_realm, soap->authrealm);
            /* save userid and passwd for basic or digest authentication */
            http_da_save(soap, &info, local_realm, username, password);
#if 1
            soap_begin_count(soap);
            soap_end_count(soap);
#endif
            info.qop = soap_strdup(soap, "auth");
            http_da_restore(soap, &info);
            soap_connect_command(soap, SOAP_GET, endpoint, "");
            soap_end_send(soap);
            soap_begin_recv(soap);
            soap_end_recv(soap);
            printf("UUUUUUUUU second attempt result %d\n", soap->error);
            if (soap->error != SOAP_OK)
                soap_print_fault(soap, stderr);
        }
        else 
            soap_print_fault(soap, stderr); 
        return 1;
    } 

    soap_done(soap);
    soap_end(soap); 
    return 0;
}
#endif
