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

#define TE_LGR_USER     "ACSE ConnectionRequester"

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

/** Struct for ConnectionRequest descriptor */
typedef struct conn_req_t {
    struct soap     m_soap;   /**< internal gSOAP environment */
    cpe_t       *cpe_item;    /**< CPE - object of Connection Request */
} conn_req_t;



/**
 * Callback for I/O ACSE channel, called before poll().
 * It fills @p pfd according with specific channel situation.
 * Its prototype matches with field #channel_t::before_poll.
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct (OUT)
 * @param deadline  Timestamp until wait, keep untouched if not need (OUT)
 *
 * @return status code.
 */
te_errno
conn_req_before_poll(void *data, struct pollfd *pfd,
                     struct timeval *deadline)
{
    conn_req_t *conn_req = data;

    UNUSED(deadline);

    pfd->fd = conn_req->m_soap.socket;
    pfd->events = POLLIN;
    pfd->revents = 0;

    return 0;
}

/**
 * Callback for I/O ACSE channel, called after poll()
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
conn_req_after_poll(void *data, struct pollfd *pfd)
{
    conn_req_t *conn_req = data;
    struct soap *soap;
    struct http_da_info info;

    if (pfd == NULL)
    {
        WARN("%s(): pfd is NULL, timeout should not occure!", __FUNCTION__);
        return 0;
    }

    if (!(pfd->revents & POLLIN))
        return 0;

    VERB("Processing ConnectionRequest to '%s/%s', data ptr %p\n",
         conn_req->cpe_item->acs->name,
         conn_req->cpe_item->name, data);

    soap = &(conn_req->m_soap);
    /* should not block after poll() */
    if (soap_begin_recv(soap))
    {
        /* was something wrong for gSOAP */
        if (soap->error == 401)
        {
            const char *userid = conn_req->cpe_item->cr_auth.login;
            const char *passwd = conn_req->cpe_item->cr_auth.passwd;

            /* If ConnReq auth params not specified, use ones for
                login CPE->ACS */
            if (NULL == userid)
                userid = conn_req->cpe_item->acs_auth.login;
            if (NULL == passwd)
                passwd = conn_req->cpe_item->acs_auth.passwd;

            VERB("ConnectionRequest, attempt failed, again... "
                 "realm: '%s'; try login '%s'",
                  soap->authrealm, userid);
            /* save userid and passwd for basic or digest authentication */
            http_da_save(soap, &info, soap->authrealm, userid, passwd);
            soap_begin_count(soap);
            soap_end_count(soap);

            info.qop = soap_strdup(soap, "auth");
            http_da_restore(soap, &info);
            soap_connect_command(soap, SOAP_GET,
                                 conn_req->cpe_item->url, "");
            soap_end_send(soap);
            return 0; /* Continue operation on this I/O channel */
        }

        if (soap->error == SOAP_NO_DATA || soap->status == 204)
            conn_req->cpe_item->cr_state = CR_DONE;
        else
        {
            ERROR("Recv after Conn.Req., soap error %d", soap->error);
            conn_req->cpe_item->cr_state = CR_ERROR;
        }
    }
    else
        conn_req->cpe_item->cr_state = CR_DONE;

    soap_end_recv(soap);

    VERB("Recv after Conn req to '%s/%s', status %d",
         conn_req->cpe_item->acs->name,
         conn_req->cpe_item->name, soap->error);

    soap_closesock(soap);
    return TE_ENOTCONN; /* Finish this I/O channel */
}


/**
 * Callback for I/O ACSE channel, called at channel destroy.
 * Its prototype matches with field #channel_t::destroy.
 *
 * @param data      Channel-specific private data.
 *
 * @return status code.
 */
te_errno
conn_req_destroy(void *data)
{
    conn_req_t *conn_req = data;

    soap_end(&(conn_req->m_soap));
    soap_done(&(conn_req->m_soap));
    free(conn_req);
    return 0;
}


/* see description in acse_internal.h */
te_errno
acse_init_connection_request(cpe_t *cpe_item)
{
    conn_req_t *conn_req_data;
    struct soap *soap;
    channel_t   *channel;

    if (cpe_item->url == NULL)
    {
        ERROR("%s() for %s/%s: NULL Conn.Req. URL",
            __FUNCTION__, cpe_item->acs->name, cpe_item->name);
        return TE_EFAIL;
    }

    if ((conn_req_data = malloc(sizeof(*conn_req_data))) == NULL ||
         (channel = malloc(sizeof(*channel))) == NULL)
        return TE_ENOMEM;

    soap_init(&(conn_req_data->m_soap));
    soap = &(conn_req_data->m_soap);
    soap->version = 1;

    soap_register_plugin(soap, http_da);

    soap_begin(soap);

    conn_req_data->cpe_item = cpe_item;


    soap_begin_count(soap);
    soap_end_count(soap);

    if (soap_connect_command(soap, SOAP_GET, cpe_item->url, ""))
    {
        char fault_buf[1000];
        soap_sprint_fault(soap, fault_buf, sizeof(fault_buf));
        ERROR("%s() failed, soap error %d, descr: %s",
              __FUNCTION__, soap->error, fault_buf);
        soap_end(soap);
        free(conn_req_data);
        free(channel);
        return TE_EFAIL;
    }
    cpe_item->cr_state = CR_WAIT_AUTH;

    channel->data = conn_req_data;
    channel->before_poll = conn_req_before_poll;
    channel->after_poll = conn_req_after_poll;
    channel->destroy = conn_req_destroy;
    channel->name = strdup("ConnRequestor");

    RING("%s() to %s/%s \n CR URL <%s>, wait.. data ptr %p",
        __FUNCTION__, cpe_item->acs->name, cpe_item->name,
        cpe_item->url, channel->data);

    acse_add_channel(channel);

    return 0;
}


