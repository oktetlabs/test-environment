/** @file
 * @brief ACSE CWMP Dispatcher
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

#define TE_LGR_USER     "ACSE CWMP dispatcher"

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stddef.h>
#include<string.h>

#include "acse_internal.h"
#include "httpda.h"

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"

#include "acse_soapH.h"

#include "acse_mem.h"



 
/** XML namespaces for gSOAP */
SOAP_NMAC struct Namespace namespaces[] =
{
    {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/",
                "http://www.w3.org/*/soap-encoding", NULL},
    {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/",
                "http://www.w3.org/*/soap-envelope", NULL},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance",
                "http://www.w3.org/*/XMLSchema-instance", NULL},
    {"xsd", "http://www.w3.org/2001/XMLSchema",
                "http://www.w3.org/*/XMLSchema", NULL},
    {"cwmp", "urn:dslforum-org:cwmp-1-0",
                "urn:dslforum-org:cwmp-1-*", NULL},
    {NULL, NULL, NULL, NULL}
};

/** Single REALM for Basic/Digest Auth. which we support. */
const char *authrealm = "tr-069";


/**
 * Find URL for ConnectionRequest in Inform parameter list.
 *
 * @param cwmp__Inform          received Inform.
 * @param cpe_item              CPE record, where URL will be stored.
 *
 * @return status code
 */
te_errno
cpe_find_conn_req_url(struct _cwmp__Inform *cwmp__Inform, cpe_t *cpe_item)
{
    cwmp__ParameterValueStruct *param_v;
    int i;

    for (i = 0; i < cwmp__Inform->ParameterList->__size; i++)
    {
        param_v =
            cwmp__Inform->ParameterList->__ptrParameterValueStruct[i];
        VERB("%s, param name '%s', \n    val '%s'",
             __FUNCTION__, param_v->Name, param_v->Value);
        if (strcmp(param_v->Name, 
            "InternetGatewayDevice.ManagementServer.ConnectionRequestURL")
            == 0)
        {
            if (cpe_item->url)
                free((char *)cpe_item->url);
            cpe_item->url = strdup(param_v->Value);
            RING("Found new ConnReq URL in Inform: '%s', save it.",
                 param_v->Value);
            break;
        }
    }
    return 0;
}


/**
 * Store received Inform in CPE record.
 *
 * @param cwmp__Inform  deserialized CWMP Inform.
 * @param cpe_item      CPE record, corresponding to Inform.
 * @param heap          Memory heap containing Inform data.
 *
 * @return status code
 */
te_errno
cpe_store_inform(struct _cwmp__Inform *cwmp__Inform,
                 cpe_t *cpe_item, mheap_t heap)
{

    cpe_inform_t *inf_store = malloc(sizeof(cpe_inform_t));
    cpe_inform_t *inf_last = LIST_FIRST(&(cpe_item->inform_list));
    int last_index = (inf_last != NULL) ? inf_last->index : 0;

    inf_store->inform = cwmp__Inform;
    inf_store->index = last_index + 1;
    mheap_add_user(heap, inf_store);

    LIST_INSERT_HEAD(&(cpe_item->inform_list), inf_store, links);
    return 0;
}

/**
 * Check authentication for incoming connection, response if necessary.
 * If auth passed, fills @p cpe with ptr to CPE record.
 *
 * @param soap          internal gSOAP connection descriptor.
 * @param session       ACSE descriptor of CWMP session.
 * @param cpe           CPE record, to which connection is
 *                      authenticated (OUT).
 *
 * @return TRUE if auth passed, FALSE if not.
 */
int
acse_cwmp_auth(struct soap *soap, cwmp_session_t *session, cpe_t **cpe)
{
    cpe_t *cpe_item = NULL;

    VERB("Start authenticate, state %d, for '%s'", session->state, 
        session->acs_owner->name);
    switch (session->state)
    {
        case CWMP_LISTEN:
            session->state = CWMP_WAIT_AUTH;
            break;

        case CWMP_WAIT_AUTH:

            if (!(soap->userid))
            {
                ERROR("%s(): No userid information in WAIT_AUTH state", 
                      __FUNCTION__);
                soap->keep_alive = 0; 
                soap->error = 500;

                return FALSE;
            }

            if (session->acs_owner->auth_mode == ACSE_AUTH_DIGEST &&
                (!soap->authrealm || strcmp(soap->authrealm, authrealm)))
            {
                RING("Digest Auth failed: wrong realm '%s', need '%s'",
                    soap->authrealm, authrealm);
                break;
            }

            LIST_FOREACH(cpe_item, &(session->acs_owner->cpe_list), links)
            {
                if (strcmp(cpe_item->acs_auth.login, soap->userid) == 0)
                    break; /* from LIST_FOREACH */
            }

            if (cpe_item == NULL)
            {
                RING("%s() userid '%s' not found, auth fail", 
                        __FUNCTION__, soap->userid);
                break;
            }

            VERB("check auth for user '%s', pass '%s'",
                soap->userid, cpe_item->acs_auth.passwd);
            if (session->acs_owner->auth_mode == ACSE_AUTH_DIGEST)
            {
                if (http_da_verify_post(soap, cpe_item->acs_auth.passwd))
                {
                    RING("Digest Auth failed: verify not pass ");
                    break;
                }
            }
            else
            {
                if (strcmp (soap->passwd, cpe_item->acs_auth.passwd) != 0)
                {
                    RING("Basic Auth failed: passwd not match");
                    break;
                }
            }

            RING("%s(): Authentication passed, CPE '%s', username '%s'", 
                __FUNCTION__, cpe_item->name, cpe_item->acs_auth.login);

            *cpe = cpe_item;

            return TRUE;

        default:
            /* TODO: maybe, with SSL authentication it will be correct
               path, not error.. */
            ERROR("%s(): unexpected session state %d", 
                  __FUNCTION__, session->state);
            soap->error = 500;
            return FALSE;
    }

    VERB("Auth failed, send authrealm, etc.. to client");
    if (soap->authrealm)
        soap_dealloc(soap, soap->authrealm);

    soap->authrealm = soap_strdup(soap, authrealm);
    soap->keep_alive = 1; 

    soap->error = SOAP_OK;
    soap_serializeheader(soap);
    soap_begin_count(soap);
    soap_end_count(soap);
    soap_response(soap, 401);
    soap_end_send(soap);
    soap->keep_alive = 1; 

    soap->error = SOAP_OK;

    return FALSE;
}


/** gSOAP callback for GetRPCMethods ACS Method */
SOAP_FMAC5 int SOAP_FMAC6
__cwmp__GetRPCMethods(struct soap *soap, 
            struct _cwmp__GetRPCMethods *cwmp__GetRPCMethods, 
            struct _cwmp__GetRPCMethodsResponse
                                  *cwmp__GetRPCMethodsResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__GetRPCMethods);
    UNUSED(cwmp__GetRPCMethodsResponse);

    return 0;
}


/** gSOAP callback for Inform ACS Method */
SOAP_FMAC5 int SOAP_FMAC6 
__cwmp__Inform(struct soap *soap,
               struct _cwmp__Inform *cwmp__Inform,
               struct _cwmp__InformResponse *cwmp__InformResponse)
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    int auth_pass = FALSE;
    cpe_t *cpe_item;

    if(NULL == session)
    {
        ERROR("%s(): NULL user pointer in soap!", __FUNCTION__);
        /* TODO: correct processing */
        return 500; 
    }

    VERB("%s called. Header is %p, enc style is '%s', inform Dev is '%s'",
            __FUNCTION__, soap->header, soap->encodingStyle,
            cwmp__Inform->DeviceId->OUI);
    soap->keep_alive = 1; 

    if (!(session->acs_owner))
    {
        /* TODO: think, what should be done with session in such 
            ugly internal error??? */
        ERROR("catch Inform, session should have ACS owner");
        soap->keep_alive = 0; 
        return 500;
    }


    switch (session->acs_owner->auth_mode)
    {
        case ACSE_AUTH_NONE:
            cpe_item = LIST_FIRST(&(session->acs_owner->cpe_list));
            if (cpe_item == NULL)
            {
                ERROR("catch Inform for ACS without CPE.");
                soap->keep_alive = 0; 
                return 500;
            }
            auth_pass = TRUE;
            break;

        case ACSE_AUTH_BASIC:
        case ACSE_AUTH_DIGEST:
            auth_pass = acse_cwmp_auth(soap, session, &cpe_item);
            break;
    }
    if (TRUE == auth_pass)
    {
        session->state = CWMP_SERVE;
        session->cpe_owner = cpe_item;
        session->acs_owner = NULL;
        cpe_item->session = session;
    }
    else
    {
        if (SOAP_OK == soap->error)
            return SOAP_STOP; /* HTTP response already sent. */
        else
            return soap->error;
    }



    if (soap->header == NULL)
    {
        soap->header = soap_malloc(soap, sizeof(struct SOAP_ENV__Header));
        memset(soap->header, 0, sizeof(*(soap->header)));
    }

    if (soap->header->cwmp__HoldRequests == NULL)
    {
        soap->header->cwmp__HoldRequests = 
                soap_malloc(soap, sizeof(_cwmp__HoldRequests));
    }

    if (soap->encodingStyle)
    {
        soap->encodingStyle = NULL;
    }

    cpe_find_conn_req_url(cwmp__Inform, cpe_item);
    cpe_store_inform(cwmp__Inform, cpe_item, session->def_heap);

    cwmp__InformResponse->MaxEnvelopes = 1;
    soap->header->cwmp__HoldRequests->__item = 0;
    soap->header->cwmp__HoldRequests->SOAP_ENV__mustUnderstand = "1";
    soap->header->cwmp__ID = NULL;
    soap->keep_alive = 1; 

    return SOAP_OK;
}


/** gSOAP callback for TransferComplete ACS Method */
SOAP_FMAC5 int SOAP_FMAC6 
__cwmp__TransferComplete(struct soap *soap, 
                    struct _cwmp__TransferComplete *cwmp__TransferComplete,
                    struct _cwmp__TransferCompleteResponse
                            *cwmp__TransferCompleteResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__TransferComplete);
    UNUSED(cwmp__TransferCompleteResponse);

    return 0;
}

/** gSOAP callback for AutonomousTransferComplete ACS Method */
SOAP_FMAC5 int SOAP_FMAC6 
__cwmp__AutonomousTransferComplete(struct soap *soap, 
            struct _cwmp__AutonomousTransferComplete
                            *cwmp__AutonomousTransferComplete,
            struct _cwmp__AutonomousTransferCompleteResponse 
                            *cwmp__AutonomousTransferCompleteResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__AutonomousTransferComplete);
    UNUSED(cwmp__AutonomousTransferCompleteResponse);

    return 0;
}

/** gSOAP callback for RequestDownload ACS Method */
SOAP_FMAC5 int SOAP_FMAC6
__cwmp__RequestDownload(struct soap *soap, 
        struct _cwmp__RequestDownload *cwmp__RequestDownload, 
        struct _cwmp__RequestDownloadResponse
                                     *cwmp__RequestDownloadResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__RequestDownload);
    UNUSED(cwmp__RequestDownloadResponse);

    return 0;
}

/** gSOAP callback for Kicked ACS Method */
SOAP_FMAC5 int SOAP_FMAC6 
__cwmp__Kicked(struct soap *soap, 
                struct _cwmp__Kicked *cwmp__Kicked, 
                struct _cwmp__KickedResponse *cwmp__KickedResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__Kicked);
    UNUSED(cwmp__KickedResponse);

    return 0;
}




/** 
 * Callback for I/O ACSE channel, called before poll() 
 * It fills @p pfd according with specific channel situation.
 * Its prototype matches with field #channel_t::before_poll.
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct (OUT)
 *
 * @return status code.
 */
te_errno
cwmp_before_poll(void *data, struct pollfd *pfd)
{
    cwmp_session_t *cwmp_sess = data;

    if (cwmp_sess == NULL ||
        cwmp_sess->state == CWMP_NOP)
    {
        return TE_EINVAL;
    }

    pfd->fd = cwmp_sess->m_soap.socket;
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
cwmp_after_poll(void *data, struct pollfd *pfd)
{
    cwmp_session_t *cwmp_sess = data;

    if (!(pfd->revents & POLLIN))
        return 0;

    switch(cwmp_sess->state)
    {
        case CWMP_LISTEN:
        case CWMP_WAIT_AUTH:
        case CWMP_SERVE:
            /* Now, after poll() on soap socket, it should not block */
            soap_serve(&cwmp_sess->m_soap);
            RING("status after serve: %d", cwmp_sess->m_soap.error);
            if (cwmp_sess->m_soap.error == SOAP_EOF)
            {
                RING(" CWMP processing, EOF");
                return TE_ENOTCONN;
            }
            break;

        case CWMP_WAIT_RESPONSE:
            /* Now, after poll() on soap socket, it should not block */
            acse_soap_serve_response(cwmp_sess);
            RING("status after serve: %d", cwmp_sess->m_soap.error);
            if (cwmp_sess->m_soap.error == SOAP_EOF)
            {
                RING(" CWMP processing, EOF");
                return TE_ENOTCONN;
            }
            break;
        default: /* do nothing here */
            WARN("CWMP after poll, unexpected state %d\n",
                    (int)cwmp_sess->state);
            break;
    }

    return 0;
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
cwmp_destroy(void *data)
{
    cwmp_session_t *cwmp_sess = data;

    return cwmp_close_session(cwmp_sess);
}


/* see description in acse_internal.h */
te_errno
cwmp_accept_cpe_connection(acs_t *acs, int socket)
{
    if (LIST_EMPTY(&acs->cpe_list))
    {
        RING("%s: conn refused: no CPE for this ACS.", __FUNCTION__);
        return TE_ECONNREFUSED;
    }
    /* TODO: real check, now accept all, if any CPE registered. */

    return cwmp_new_session(socket, acs);
}

/** Callback fserveloop for gSOAP */
int
cwmp_serveloop(struct soap *soap)
{
    soap->error = SOAP_STOP; /* to stop soap_serve loop. */
    return soap->error;
}

/** Callback fparse for gSOAP, to return STOP if empty POST was received.*/
int
cwmp_fparse(struct soap *soap)
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    int rc = session->orig_fparse(soap);

    if (rc == SOAP_OK && soap->length == 0)
    {
        rc = SOAP_STOP;
    }

    return rc;
}

/* see description in acse_internal.h */
te_errno
cwmp_new_session(int socket, acs_t *acs)
{
    cwmp_session_t *new_sess = malloc(sizeof(*new_sess));
    channel_t      *channel  = malloc(sizeof(*channel));

    if (new_sess == NULL || channel == NULL)
        return TE_ENOMEM;

    soap_init(&(new_sess->m_soap));

    if (acs->ssl)
    {
        /* TODO: Investigate, how to pass SSL certificate correct.
                 Test SSL. */
        if (soap_ssl_server_context(&new_sess->m_soap,
              SOAP_SSL_REQUIRE_SERVER_AUTHENTICATION |
              SOAP_SSLv3_TLSv1 | SOAP_SSL_RSA,
              acs->cert, /* keyfile: required when server must
                            authenticate to clients */
              "", /* password to read the key file */
              "cacert.pem", /* optional cacert file to store
                               trusted certificates */
              NULL, /* optional capath to directory with
                        trusted certificates */
              NULL, /* DH file, if NULL use RSA */
              NULL, /* if randfile!=NULL: use a file with
                        random data to seed randomness */
              NULL /* optional server identification
                    to enable SSL session cache (must be a unique name) */
            ))
        {
            soap_print_fault(&new_sess->m_soap, stderr);
            ERROR("soap_ssl_server_context failed, soap error %d",
                new_sess->m_soap.error);
            free(new_sess);
            free(channel);
            /* TODO: what error return here? */
            return TE_ECONNREFUSED;
        }
    }

    new_sess->state = CWMP_NOP;
    new_sess->acs_owner = acs;
    new_sess->cpe_owner = NULL;
    new_sess->channel = channel;
    new_sess->rpc_item = NULL;
    new_sess->def_heap = mheap_create(new_sess);
    new_sess->m_soap.user = new_sess;
    new_sess->m_soap.socket = socket;
    new_sess->m_soap.fserveloop = cwmp_serveloop;
    new_sess->m_soap.fmalloc = acse_cwmp_malloc;

    soap_imode(&new_sess->m_soap, SOAP_IO_KEEPALIVE);
    soap_omode(&new_sess->m_soap, SOAP_IO_KEEPALIVE);

    if (acs->ssl)
    {
        if (soap_ssl_accept(&new_sess->m_soap))
        {
            RING("soap_ssl_accept failed, soap error %d",
                new_sess->m_soap.error);
            soap_free(&new_sess->m_soap);
            free(new_sess);
            free(channel);
            return TE_ECONNREFUSED;
        }
    }

    /* TODO: investigate, is it possible in gSOAP to use 
       Digest auth. over SSL connection */
    if (acs->auth_mode == ACSE_AUTH_DIGEST)
        soap_register_plugin(&new_sess->m_soap, http_da); 

    VERB("init CWMP session for ACS '%s', auth mode %d",
            acs->name, (int)acs->auth_mode);

    new_sess->m_soap.max_keep_alive = 10;

    channel->data = new_sess;
    channel->before_poll = cwmp_before_poll;
    channel->after_poll = cwmp_after_poll;
    channel->destroy = cwmp_destroy;

    new_sess->orig_fparse = new_sess->m_soap.fparse;
    new_sess->m_soap.fparse = cwmp_fparse;

    new_sess->state = CWMP_LISTEN; 
    acse_add_channel(channel);

    return 0;
}

te_errno 
cwmp_close_session(cwmp_session_t *sess)
{
    INFO("close cwmp session");

    /* TODO: investigate, what else should be closed.. */

    /* free all heaps, where this session was user of memory */
    mheap_free_user(MHEAP_NONE, sess); 

    soap_dealloc(&(sess->m_soap), NULL);
    soap_end(&(sess->m_soap));
    soap_done(&(sess->m_soap));

    if (sess->acs_owner)
        sess->acs_owner->session = NULL;
    if (sess->cpe_owner)
        sess->cpe_owner->session = NULL;

    free(sess);

    return 0;
}


/* See description in acse_internal.h */
te_errno
acse_enable_acs(acs_t *acs)
{
    struct sockaddr_in *sin;
    te_errno rc;

    if (acs == NULL || acs->port == 0)
        return TE_EINVAL;
    sin = malloc(sizeof(struct sockaddr_in));
    sin->sin_family = AF_INET;

    /* TODO: get host from URL field of ACS */
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_port = htons(acs->port);

    acs->addr_listen = SA(sin);
    acs->addr_len = sizeof(struct sockaddr_in);


    rc = conn_register_acs(acs);
    if (rc == 0)
        acs->enabled = 1;

    return rc;
}

/* See description in acse_internal.h */
te_errno
acse_disable_acs(acs_t *acs)
{
    /* TODO: think, should it stop already established CWMP sessions,
      or should only disable new CWMP sessions? */
    /* TODO operation itself... */
    return TE_EFAULT;
}


/**
 * Put CWMP data into gSOAP buffer.
 * @return gSOAP status code.
 */
int 
acse_soap_put_cwmp(struct soap *soap, acse_epc_cwmp_data_t *request)
{
    switch(request->rpc_cpe)
    {
        case CWMP_RPC_get_rpc_methods:
            {
                _cwmp__GetRPCMethods arg;
                soap_default__cwmp__GetRPCMethods(soap, &arg);
                return soap_put__cwmp__GetRPCMethods(soap, &arg,
                                            "cwmp:GetRPCMethods", "");
            }
            break;
        case CWMP_RPC_set_parameter_values:
            return soap_put__cwmp__SetParameterValues(soap, 
                            request->to_cpe.set_parameter_values,
                            "cwmp:SetParameterValues", "");
        case CWMP_RPC_get_parameter_values:
            return soap_put__cwmp__GetParameterValues(soap, 
                            request->to_cpe.get_parameter_values,
                            "cwmp:GetParameterValues", "");
        case CWMP_RPC_get_parameter_names:
            return soap_put__cwmp__GetParameterNames(soap, 
                            request->to_cpe.get_parameter_names,
                            "cwmp:GetParameterNames", "");
        case CWMP_RPC_set_parameter_attributes:
        case CWMP_RPC_get_parameter_attributes:
        case CWMP_RPC_add_object:
        case CWMP_RPC_delete_object:
        case CWMP_RPC_reboot:
        case CWMP_RPC_download:
        case CWMP_RPC_upload:
        case CWMP_RPC_factory_reset:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_get_options:
            RING("TODO send RPC with code %d", request->rpc_cpe);
        default:
            break; /* do nothing */
    }
    return 0;
}

/**
 * Send RPC to CPE.
 *
 * This function check RPC queue in CPE record, take first item
 * if it present, and send it to CPE in the HTTP response.
 * If there is no RPC items in queue, it send empty HTTP response
 * with no keep-alive to finish CWMP session.
 *
 * After sending RPC to CPE request item is moved to results queue
 * in CPE record, where response from CPE will be stored, when 
 * it will be received later.
 *
 * @param soap        gSOAP struct.
 * @param session     current CWMP session.
 * 
 * @return gSOAP status.
 */
int 
acse_cwmp_send_rpc(struct soap *soap, cwmp_session_t *session)
{
    acse_epc_cwmp_data_t *request;
    cpe_t *cpe = session->cpe_owner;

    if (TAILQ_EMPTY(&cpe->rpc_queue))
    {
        /* TODO add check, whether HoldRequests was set on */

        RING("Empty POST for '%s', empty list of RPC calls, response 204",
              cpe->name);
        soap->keep_alive = 0;
        soap_begin_count(soap);
        soap_end_count(soap);
        soap_response(soap, 204);
        soap_end_send(soap);
        session->state = CWMP_SERVE;
        return 0;
    }
    session->rpc_item = TAILQ_FIRST(&cpe->rpc_queue);
    session->rpc_item->heap = mheap_create(session->rpc_item);
    mheap_add_user(session->rpc_item->heap, session);

    request = session->rpc_item->params;

    RING("%s(): Try send RPC for '%s', rpc type %d, ind %d ",
         __FUNCTION__, cpe->name, request->rpc_cpe, request->index);

    if (soap->header == NULL)
    {
        soap->header = soap_malloc(soap, sizeof(*(soap->header)));
        memset(soap->header, 0, sizeof(*(soap->header)));
    }

    if (soap->header->cwmp__HoldRequests == NULL)
    {
        soap->header->cwmp__HoldRequests = 
                soap_malloc(soap, sizeof(_cwmp__HoldRequests));
    }
    soap->header->cwmp__HoldRequests->__item = request->hold_requests;
    soap->header->cwmp__HoldRequests->SOAP_ENV__mustUnderstand = "1";
    soap->header->cwmp__ID = NULL;
    soap->keep_alive = 1; 
    soap->error = SOAP_OK;
    soap_serializeheader(soap);

    switch(request->rpc_cpe)
    {
        case CWMP_RPC_get_rpc_methods: 
        {
            _cwmp__GetRPCMethods arg;
            soap_default__cwmp__GetRPCMethods(soap, &arg);
            soap_serialize__cwmp__GetRPCMethods(soap, &arg);
        }
            break;
        case CWMP_RPC_set_parameter_values:
        case CWMP_RPC_get_parameter_values:
        case CWMP_RPC_get_parameter_names:
        case CWMP_RPC_set_parameter_attributes:
        case CWMP_RPC_get_parameter_attributes:
        case CWMP_RPC_add_object:
        case CWMP_RPC_delete_object:
        case CWMP_RPC_reboot:
        case CWMP_RPC_download:
        case CWMP_RPC_upload:
        case CWMP_RPC_factory_reset:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_get_options:
            RING("TODO send RPC with code %d", request->rpc_cpe);
        default: /* do nothing */
            break;
    }

    if (soap_begin_count(soap))
        return soap->error;


    if (soap->mode & SOAP_IO_LENGTH)
    {
        if (soap_envelope_begin_out(soap)
             || soap_putheader(soap)
             || soap_body_begin_out(soap)
             || acse_soap_put_cwmp(soap, request)
             || soap_body_end_out(soap)
             || soap_envelope_end_out(soap))
        {
            ERROR("%s(): 1, soap error %d", __FUNCTION__, soap->error);
            return soap->error;
        }
    }
    if (soap_end_count(soap)
     || soap_response(soap, SOAP_OK)
     || soap_envelope_begin_out(soap)
     || soap_putheader(soap)
     || soap_body_begin_out(soap)
     || acse_soap_put_cwmp(soap, request)
     || soap_body_end_out(soap)
     || soap_envelope_end_out(soap)
     || soap_end_send(soap))
    {
        ERROR("%s(): 2, soap error %d", __FUNCTION__, soap->error);
        return soap->error;
    }
    session->state = CWMP_WAIT_RESPONSE;

    TAILQ_REMOVE(&cpe->rpc_queue, session->rpc_item, links);

    TAILQ_INSERT_TAIL(&cpe->rpc_results, session->rpc_item, links);

    return 0;
}

/**
 * Process empty HTTP POST from CPE, send response.
 * 
 * @param soap        gSOAP struct.
 * 
 * @return gSOAP status.
 */
int
acse_cwmp_empty_post(struct soap* soap)
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    cpe_t          *cpe;

    if (session == NULL || (cpe = session->cpe_owner) == NULL)
    {
        ERROR("Internal ACSE error at empty POST processing");
        soap->keep_alive = 0;
        soap_closesock(soap);
        return 500;
    }

    if (session->state != CWMP_SERVE)
    {
        /* TODO: think, what would be correct in unexpected state?..
         * Now just close connection. */
        ERROR("Empty POST processing, cpe '%s', unexpected state %d",
                cpe->name, session->state);
        soap->keep_alive = 0;
        soap_closesock(soap);
        return 500;
    } 

    return  acse_cwmp_send_rpc(soap, session);
}


/* See description in acse_internal.h */
void
acse_soap_serve_response(cwmp_session_t *cwmp_sess)
{
    struct soap *soap = &(cwmp_sess->m_soap);

    acse_epc_cwmp_data_t *request = cwmp_sess->rpc_item->params;
    VERB("Start of serve reponse: processed rpc_item: %p",
           cwmp_sess->rpc_item);

    /* This function works in state WAIT_RESPONSE, when CWMP session
       is already associated with particular CPE. */ 

    if (soap_begin_recv(soap)
         || soap_envelope_begin_in(soap)
         || soap_recv_header(soap)
         || soap_body_begin_in(soap))
    {
        ERROR("serve CWMP resp, soap err %d", soap->error);
        return; /* TODO: study, do soap_closesock() here ??? */
    }

    switch (request->rpc_cpe)
    {
        case CWMP_RPC_get_rpc_methods:
        {
            _cwmp__GetRPCMethodsResponse *resp = 
                                    soap_malloc(soap, sizeof(*resp));
            soap_default__cwmp__GetRPCMethodsResponse(soap, resp);
            soap_get__cwmp__GetRPCMethodsResponse(soap, resp,
                                    "cwmp:GetRPCMethodsResponse", "");
            request->from_cpe.get_rpc_methods_r = resp;
        }
        break;
        case CWMP_RPC_set_parameter_values:
        case CWMP_RPC_get_parameter_values:
        case CWMP_RPC_get_parameter_names:
        case CWMP_RPC_set_parameter_attributes:
        case CWMP_RPC_get_parameter_attributes:
        case CWMP_RPC_add_object:
        case CWMP_RPC_delete_object:
        case CWMP_RPC_reboot:
        case CWMP_RPC_download:
        case CWMP_RPC_upload:
        case CWMP_RPC_factory_reset:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_get_options:
            /* TODO */
        default:
            break;
    }

    cwmp_sess->rpc_item = NULL; /* It is already processed. */

    if (soap->error)
    {
        ERROR("Fail get SOAP response, error %d", soap->error);
        /* TODO: think, return here? */
    }
    if (soap_body_end_in(soap)
         || soap_envelope_end_in(soap)
         || soap_end_recv(soap))
    {
        ERROR("after get SOAP body, error %d", soap->error);
        /* TODO: think, return here? */
    }

    VERB("End of serve reponse: first rpc_item in queue: %p\n",
            TAILQ_FIRST(&(cwmp_sess->cpe_owner->rpc_queue)));
    acse_cwmp_send_rpc(soap, cwmp_sess);
}




/* See description in acse_internal.h */
void *
acse_cwmp_malloc(struct soap *soap, size_t n)
{
    cwmp_session_t *session;
    mheap_t heap;

    if (NULL == soap)
        return NULL;

    if (NULL == soap->user)
        return SOAP_MALLOC(soap, n);

    session = (cwmp_session_t *)soap->user;
    if (NULL == session->rpc_item)
        heap = session->def_heap;
    else
        heap = session->rpc_item->heap;

    return mheap_alloc(heap, n);
}
