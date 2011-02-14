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

#include <sys/stat.h>
#include <unistd.h>

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <poll.h>

#include "acse_internal.h"
#include "httpda.h"

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"

#include "acse_soapH.h"

#include "acse_mem.h"

/* local buffer for I/O XML logging, seems enough, increase if need */
#define LOG_XML_BUF 0x4000

#define SEND_FILE_BUF 0x4000

/** XML namespaces for gSOAP */
SOAP_NMAC struct Namespace namespaces[] =
{
    {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/",
                "http://www.w3.org/*/soap-envelope", NULL},
    {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/",
                "http://www.w3.org/*/soap-encoding", NULL},
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

static char send_log_buf[LOG_XML_BUF];
static char recv_log_buf[LOG_XML_BUF];

/**
 * Force stop CWMP session ignoring its current state and 
 * protocol semantic, really just close TPC connection and 
 * accurate free all internal resources. 
 */
static te_errno
cwmp_force_stop_session(cwmp_session_t *sess)
{
    if (NULL == sess || NULL == sess->channel)
        return TE_EINVAL;

    if (sess->m_soap.fclose)
        sess->m_soap.fclose(&sess->m_soap);

    acse_remove_channel(sess->channel);

    return 0;
}


te_errno
acse_send_file_portion(cwmp_session_t *session)
{
    struct soap *soap;
    size_t read;
    static int8_t tmpbuf[SEND_FILE_BUF];
    FILE *fd;

    assert(session->state == CWMP_SEND_FILE);
    soap = &(session->m_soap);

    fd = session->sending_fd;

    read = fread(tmpbuf, 1, sizeof(tmpbuf), fd);

    if (!read || soap_send_raw(soap, tmpbuf, read))
    {
        if (!read)
            RING("fread return zero, finish send file");
        else
        {
            /* can't send, but little we can do about that */
            fprintf(stderr, "acse_send_file: soap_send_raw fail\n");
            WARN("acse_send_file_portion(): soap_send_raw fail,"
                 " soap err %d", soap->error);
        }
        fclose(fd);
        soap_end_send(soap);
        session->state = CWMP_SERVE;
        session->sending_fd = NULL;
    }
    return 0;
}

/**
 * HTTP GET callback.
 */
int
acse_http_get(struct soap *soap)
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;

    acs_t   *acs = NULL;
    char     path_buf[1024] = "";

    FILE        *fd;
    struct stat  fs; 
    
    soap_end_recv(soap);

    RING("acse_http_get(): Yaahooo, GET to '%s' received", soap->path);

    if (NULL != session && 
        (NULL != (acs = session->acs_owner) || 
         NULL != (acs = session->cpe_owner->acs)) &&
        NULL != acs->http_root)
    {
        char *relative_path = soap->path;
        size_t i = 0;
        if (acs->url != NULL)
        {
            for (i = 0; acs->url[i] != '\0' && acs->url[i] == soap->path[i];
                 i++);
        }
        relative_path = soap->path + i;

        snprintf(path_buf, sizeof(path_buf), "/%s/%s",
                 acs->http_root, relative_path);
        RING("%s() construct real local filesystem path '%s'", 
             __FUNCTION__, path_buf);
    }
    else
    {
        RING("GET error: session %p, acs %p, http_root %p",
            session, acs, acs ? acs->http_root : NULL);
    }

    if (strlen(path_buf) == 0 || 
        stat(path_buf, &fs) != 0 ||
        NULL == (fd = fopen(path_buf, "r")))
    {
        int http_status;
        char *err_descr = strerror(errno);

        WARN("%s(): stat|fopen (%s) failed %d (%s)",
                __FUNCTION__, path_buf, errno, err_descr);
        switch (errno)
        {
            case EFAULT: http_status = 400; break;
            case EACCES: http_status = 403; break;
            case ENOENT: http_status = 404; break;
            default:
                if (acs != NULL && NULL == acs->http_root)
                {
                    WARN("HTTP GET received, but not http_root, reply 503");
                    http_status = 503;
                    err_descr = "HTTP dir not configured";
                }
                else
                {
                    http_status = 500;
                    err_descr = "Internal ACSE error";
                }
        }
        soap->http_content = "text/html";
        soap_response(soap, http_status); 
        soap_send(soap, "<HTML><body>");
        soap_send(soap, err_descr);
        soap_send(soap, "</body></HTML>");
        soap_end_send(soap);
        return SOAP_OK;
    }

    RING("%s(): reply with %d bytes...",
         __FUNCTION__ , (int)fs.st_size);

    session->sending_fd = fd;

    soap->http_content = "application/octet-stream";
    soap->length = fs.st_size;

    soap_response(soap, SOAP_FILE); 
#if 1
    session->state = CWMP_SEND_FILE;
    acse_send_file_portion(session);
#else
    while (1)
    {
        size_t r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);
        struct pollfd pfd;
        int pollrc;

        if (!r)
        {
            fprintf(stderr, "fread return zero, errno %s\n",
                    strerror(errno));
            break;
        }

        pfd.fd = soap->socket;
        pfd.events = POLLOUT;
        pfd.revents = 0;

        if ((pollrc = poll(&pfd, 1, 10)) <= 0)
        {
            ERROR("acse_http_get(): poll rc %d, can't send file", pollrc);
            if (pollrc < 0)
                perror("acse poll failed");
            fprintf(stderr, 
                  "acse_http_get(): poll rc %d, can't send file; break\n",
                  pollrc);
            break;
        }
#if 0
        fprintf(stderr, "UUUUUUUUUUUU pollrc %d\n", pollrc);
#endif

        if (soap_send_raw(soap, soap->tmpbuf, r))
        {
            fprintf(stderr, "acse_http_get(): soap_send_raw fail\n");
            break; // can't send, but little we can do about that
        }
    }
    fclose(fd);
    soap_end_send(soap);
#endif
    return SOAP_OK;
}

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
 * Store received ACS RPC in CPE record.
 *
 * @param rpc_acs_type  type of ACS RPC received.
 * @param rpc_acs_data  deserialized ACS RPC in gSOAP presentation.
 * @param cpe_item      CPE record, from which RPC is received.
 * @param heap          Memory heap containing RPC data.
 *
 * @return status code
 */
te_errno
cpe_store_acs_rpc(te_cwmp_rpc_acs_t rpc_acs_type,
                  void *rpc_acs_data, cpe_t *cpe_item, mheap_t heap)
{
    cpe_rpc_item_t  *rpc_item = calloc(1, sizeof(*rpc_item));
    acse_epc_cwmp_data_t *c_data = calloc(1, sizeof(*c_data));

    rpc_item->request_id = 0;
    rpc_item->heap = heap;
    rpc_item->params = c_data;

    c_data->from_cpe.p = rpc_acs_data;
    c_data->rpc_acs = rpc_acs_type;

    mheap_add_user(heap, rpc_item);

    TAILQ_INSERT_TAIL(&(cpe_item->rpc_results), rpc_item, links);
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
    int last_index = (inf_last != NULL) ? inf_last->request_id : 0;

    inf_store->inform = cwmp__Inform;
    inf_store->request_id = last_index + 1;
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
    acse_cwmp_send_http(soap, NULL, 401, NULL);
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
    static char *acs_method_list[] = {
            "GetRPCMethods",
            "Inform",
            "TransferComplete",
        };
    struct MethodList m_list;

    m_list.__size = sizeof(acs_method_list)/sizeof(acs_method_list[0]);
    m_list.__ptrstring = acs_method_list;

    UNUSED(soap);
    UNUSED(cwmp__GetRPCMethods);

    cwmp__GetRPCMethodsResponse->MethodList_ = &m_list;

    return 0;
}


static inline void
cwmp_prepare_soap_header(struct soap *soap, cpe_t *cpe)
{
    if (NULL == soap->header)
    {
        soap->header = soap_malloc(soap, sizeof(struct SOAP_ENV__Header));
        memset(soap->header, 0, sizeof(*(soap->header)));
    }

    if (NULL == soap->header->cwmp__HoldRequests)
    {
        soap->header->cwmp__HoldRequests = 
                soap_malloc(soap, sizeof(_cwmp__HoldRequests));
    }

    if (soap->encodingStyle)
    {
        soap->encodingStyle = NULL;
    }

    soap->header->cwmp__HoldRequests->__item = cpe->hold_requests;
    soap->header->cwmp__HoldRequests->SOAP_ENV__mustUnderstand = "1";
    soap->header->cwmp__ID = NULL;
    soap->keep_alive = 1; 
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
    if (NULL != session->acs_owner->http_response)
    {
        int http_code = session->acs_owner->http_response->http_code;
        char *loc = session->acs_owner->http_response->location;
        RING("Process Inform, found HTTP response setting, %d, %s",
            http_code, loc);

        strcpy(soap->endpoint, loc);
        free(session->acs_owner->http_response);
        session->acs_owner->http_response = NULL;

        return http_code;
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
        /* State of ConnectionRequest to this CPE is not actual now. */
        cpe_item->cr_state = CR_NONE; 
        if (NULL != cpe_item->http_response)
        {
            int http_code = cpe_item->http_response->http_code;
            char *loc = cpe_item->http_response->location;
            RING("Process Inform, for CPE is HTTP response setting, %d, %s",
                http_code, loc);

            strcpy(soap->endpoint, loc);
            free(cpe_item->http_response);
            cpe_item->http_response = NULL;

            return http_code;
        }
    }
    else
    {
        if (SOAP_OK == soap->error)
            return SOAP_STOP; /* HTTP response already sent. */
        else
            return soap->error;
    }

    cpe_find_conn_req_url(cwmp__Inform, cpe_item);
    cpe_store_inform(cwmp__Inform, cpe_item, session->def_heap);

    cpe_store_acs_rpc(CWMP_RPC_inform, cwmp__Inform, cpe_item,
                      session->def_heap);

    cwmp__InformResponse->MaxEnvelopes = 1;

    cwmp_prepare_soap_header(soap, cpe_item);

    return SOAP_OK;
}


/** gSOAP callback for TransferComplete ACS Method */
SOAP_FMAC5 int SOAP_FMAC6 
__cwmp__TransferComplete(struct soap *soap, 
                    struct _cwmp__TransferComplete *cwmp__TransferComplete,
                    struct _cwmp__TransferCompleteResponse
                            *cwmp__TransferCompleteResponse)
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    cpe_t *cpe_item;

    if(NULL == session)
    {
        ERROR("%s(): NULL user pointer in soap!", __FUNCTION__);
        return 500; 
    }
    if (NULL == (cpe_item = session->cpe_owner))
    {
        ERROR("%s(): NULL CPE pointer in session!", __FUNCTION__);
        return 500; 
    }
    RING("%s(): for CPE record %s/%s, Key '%s'", __FUNCTION__, 
         cpe_item->acs->name, cpe_item->name, 
         cwmp__TransferComplete->CommandKey);

    cpe_store_acs_rpc(CWMP_RPC_transfer_complete, cwmp__TransferComplete,
                      cpe_item, session->def_heap);
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
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    cpe_t *cpe_item;

    if(NULL == session)
    {
        ERROR("%s(): NULL user pointer in soap!", __FUNCTION__);
        return 500; 
    }
    if (NULL == (cpe_item = session->cpe_owner))
    {
        ERROR("%s(): NULL CPE pointer in session!", __FUNCTION__);
        return 500; 
    }
    RING("%s(): for CPE record %s/%s, URL '%s'", __FUNCTION__, 
         cpe_item->acs->name, cpe_item->name, 
         cwmp__AutonomousTransferComplete->AnnounceURL);

    cpe_store_acs_rpc(CWMP_RPC_autonomous_transfer_complete,
                      cwmp__AutonomousTransferComplete,
                      cpe_item, session->def_heap);
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
    RING("%s(): File type '%s'", __FUNCTION__,
         cwmp__RequestDownload->FileType);

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

    if (cwmp_sess->state == CWMP_SEND_FILE)
        pfd->events = POLLOUT;
    else
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

    if (!(pfd->revents))
        return 0;

    switch(cwmp_sess->state)
    {
        case CWMP_LISTEN:
        case CWMP_WAIT_AUTH:
        case CWMP_SERVE:
            /* Now, after poll() on soap socket, it should not block */
            soap_serve(&cwmp_sess->m_soap);
            VERB("status after serve: %d", cwmp_sess->m_soap.error);
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
        case CWMP_SEND_FILE:
            if (pfd->revents & POLLOUT)
                return acse_send_file_portion(cwmp_sess);
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
    int is_not_get = 1;

    if (LIST_EMPTY(&acs->cpe_list))
    {
        RING("%s: conn refused: no CPE for this ACS.", __FUNCTION__);
        return TE_ECONNREFUSED;
    }

    /* If ACS enables SSL, any incoming connection desired as SSL 
       and accepted */
    if (!acs->ssl)
    {
        char buf[1024]; /* seems enough for HTTP header */
        ssize_t len = sizeof(buf)-1;
        char *req_url_p;

        len = recv(socket, buf, len, MSG_PEEK);
        buf[len]=0;
        VERB("cwmp_accept_cpe_conn(): peeked msg buf: '%s'", buf);
        if (strncmp(buf, "POST ", 5) && 
            (is_not_get = strncmp(buf, "GET ", 4))) 
            return TE_ECONNREFUSED; /* It is not POST or GET request */
        req_url_p = buf + 4; while(isspace(*req_url_p)) req_url_p++;

        /* without any URL specified we accept all connections */
        if (acs->url != NULL &&
            strncmp(req_url_p, acs->url, strlen(acs->url))) 
            return TE_ECONNREFUSED; /* It is not our URL */
    }

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


/** Callback fsend for gSOAP, to log XML output */
int
acse_send(struct soap *soap, const char *s, size_t n) 
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    size_t          log_len = n;
    if (n >= LOG_XML_BUF)
    {
        RING("Out XML length %d, more then log buf, cut", n);
        log_len = LOG_XML_BUF - 1;
    }
    memcpy(send_log_buf, s, log_len);
    send_log_buf[log_len] = '\0';

    /* TODO: should we make loglevel customizable here? */
    RING("Send to %s %s: \n%s", 
        session->cpe_owner ? "CPE" : "ACS", 
        session->cpe_owner ? session->cpe_owner->name : 
                             session->acs_owner->name, 
        send_log_buf);
    /* call standard gSOAP fsend */
    return session->orig_fsend(soap, s, n);
}

/** Callback frecv for gSOAP, to log XML output */
size_t
acse_recv(struct soap *soap, char *s, size_t n) 
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    size_t          rc;
    size_t          log_len;

    /* call standard gSOAP frecv */
    log_len = rc = session->orig_frecv(soap, s, n);

    if (rc >= LOG_XML_BUF)
    {
        RING("Income XML length %u, more then log buf, cut", rc);
        log_len = LOG_XML_BUF - 1;
    }
    memcpy(recv_log_buf, s, log_len);
    recv_log_buf[log_len] = '\0';

    /* TODO: should we make loglevel customizable here? */
    RING("Recv from %s %s: \n%s", 
        session->cpe_owner ? "CPE" : "ACS", 
        session->cpe_owner ? session->cpe_owner->name : 
                             session->acs_owner->name, 
        recv_log_buf);
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
    /* TODO: investigate more correct way to set version,
       maybe specify correct SOAPENV in namespaces .. */
    new_sess->m_soap.version = 1;

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
    new_sess->sending_fd = NULL;
    new_sess->def_heap = mheap_create(new_sess);
    new_sess->m_soap.user = new_sess;
    new_sess->m_soap.socket = socket;
    new_sess->m_soap.fserveloop = cwmp_serveloop;
    new_sess->m_soap.fmalloc = acse_cwmp_malloc;
    new_sess->m_soap.fget = acse_http_get;

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

    RING("init CWMP session for ACS '%s', auth mode %d",
            acs->name, (int)acs->auth_mode);

    new_sess->m_soap.max_keep_alive = 10;

    channel->data = new_sess;
    channel->before_poll = cwmp_before_poll;
    channel->after_poll = cwmp_after_poll;
    channel->destroy = cwmp_destroy;

    new_sess->orig_fparse = new_sess->m_soap.fparse;
    new_sess->m_soap.fparse = cwmp_fparse;
    new_sess->orig_fsend = new_sess->m_soap.fsend;
    new_sess->m_soap.fsend = acse_send;
    new_sess->orig_frecv = new_sess->m_soap.frecv;
    new_sess->m_soap.frecv = acse_recv;

    new_sess->state = CWMP_LISTEN; 
    acse_add_channel(channel);

    return 0;
}

te_errno 
cwmp_close_session(cwmp_session_t *sess)
{
    assert(NULL != sess);

    RING("close cwmp session on %s '%s'", 
        sess->acs_owner ? "ACS" : "CPE", 
        sess->acs_owner ? sess->acs_owner->name : sess->cpe_owner->name);

    /* TODO: investigate, what else should be closed. */

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

    if (acs == NULL || acs->port == 0)
        return TE_EINVAL;
    sin = malloc(sizeof(struct sockaddr_in));
    sin->sin_family = AF_INET;

    /* TODO: get host from URL field of ACS */
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_port = htons(acs->port);

    acs->addr_listen = SA(sin);
    acs->addr_len = sizeof(struct sockaddr_in); 

    return conn_register_acs(acs);
}

/* See description in acse_internal.h */
te_errno
acse_disable_acs(acs_t *acs)
{
    te_errno rc;
    cpe_t *cpe_item = NULL;

    if ((rc = conn_deregister_acs(acs)) != 0)
        return rc;

    if (acs->session)
    {
        cwmp_force_stop_session(acs->session);
        acs->session = NULL;
    }
    /* Now stop active CWMP sessions, if any, and clear all caches */
    LIST_FOREACH(cpe_item, &(acs->cpe_list), links)
    {
        acse_disable_cpe(cpe_item);
    }

    return 0;
}

/* See description in acse_internal.h */
te_errno
acse_disable_cpe(cpe_t *cpe)
{
    if (cpe->session)
    {
        cwmp_force_stop_session(cpe->session);
        cpe->session = NULL;
    }
    db_clear_cpe(cpe);
    cpe->enabled = FALSE;
    return 0;
}

void
acse_soap_default_req(struct soap *soap, acse_epc_cwmp_data_t *request)
{
    if (request->to_cpe.p != NULL)
        return;

    switch(request->rpc_cpe)
    {
#define SOAP_DEF_INIT_GEN(_r, _f, _t) \
        case CWMP_RPC_##_r : \
            request->to_cpe. _f = calloc(1, sizeof(_cwmp__## _t ) ); \
            soap_default__cwmp__## _t (soap, request->to_cpe. _f); \
            break;

#define SOAP_DEF_INIT(_r, _t) SOAP_DEF_INIT_GEN(_r, _r, _t)
#define SOAP_DEF_INIT_EMPTY(_r, _t) SOAP_DEF_INIT_GEN(_r, p, _t)
            
        SOAP_DEF_INIT_EMPTY(get_rpc_methods, GetRPCMethods)
        SOAP_DEF_INIT(set_parameter_values, SetParameterValues)
        SOAP_DEF_INIT(get_parameter_values, GetParameterValues)
        SOAP_DEF_INIT(get_parameter_names, GetParameterNames)
        SOAP_DEF_INIT(download, Download)
        SOAP_DEF_INIT(add_object, AddObject)
        SOAP_DEF_INIT(delete_object, DeleteObject)
        SOAP_DEF_INIT(reboot, Reboot)
        SOAP_DEF_INIT_EMPTY(factory_reset, FactoryReset)
        SOAP_DEF_INIT(set_parameter_attributes, SetParameterAttributes)
        SOAP_DEF_INIT(get_parameter_attributes, GetParameterAttributes)
        SOAP_DEF_INIT(upload, Upload)
        SOAP_DEF_INIT_EMPTY(get_queued_transfers, GetQueuedTransfers)
        SOAP_DEF_INIT_EMPTY(get_all_queued_transfers, GetAllQueuedTransfers)
        SOAP_DEF_INIT(schedule_inform, ScheduleInform)
        SOAP_DEF_INIT(set_vouchers, SetVouchers)
        SOAP_DEF_INIT(get_options, GetOptions)
#undef SOAP_DEF_INIT_GEN
#undef SOAP_DEF_INIT_EMPTY
#undef SOAP_DEF_INIT

        default:
            break; /* do nothing */
    }
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
#define SOAP_PUT_CWMP_GEN(_r, _f, _t) \
        case CWMP_RPC_##_r : \
            return soap_put__cwmp__##_t (soap, request->to_cpe. _f, \
                                         "cwmp:" #_t, "");

#define SOAP_PUT_CWMP(_r, _t) SOAP_PUT_CWMP_GEN(_r, _r, _t)
#define SOAP_PUT_CWMP_EMPTY(_r, _t) SOAP_PUT_CWMP_GEN(_r, p, _t)

        SOAP_PUT_CWMP_EMPTY(get_rpc_methods, GetRPCMethods)
        SOAP_PUT_CWMP(set_parameter_values, SetParameterValues)
        SOAP_PUT_CWMP(get_parameter_values, GetParameterValues)
        SOAP_PUT_CWMP(get_parameter_names, GetParameterNames)
        SOAP_PUT_CWMP(download, Download)
        SOAP_PUT_CWMP(add_object, AddObject)
        SOAP_PUT_CWMP(delete_object, DeleteObject)
        SOAP_PUT_CWMP(reboot, Reboot)
        SOAP_PUT_CWMP_EMPTY(factory_reset, FactoryReset)
        SOAP_PUT_CWMP(set_parameter_attributes, SetParameterAttributes)
        SOAP_PUT_CWMP(get_parameter_attributes, GetParameterAttributes)
        SOAP_PUT_CWMP(upload, Upload)
        SOAP_PUT_CWMP_EMPTY(get_queued_transfers, GetQueuedTransfers)
        SOAP_PUT_CWMP_EMPTY(get_all_queued_transfers, GetAllQueuedTransfers)
        SOAP_PUT_CWMP(schedule_inform, ScheduleInform)
        SOAP_PUT_CWMP(set_vouchers, SetVouchers)
        SOAP_PUT_CWMP(get_options, GetOptions)

#undef SOAP_PUT_CWMP_GEN
#undef SOAP_PUT_CWMP_EMPTY
#undef SOAP_PUT_CWMP

#if 0
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
            VERB("%s(): send GetParameterNames for '%s'",
                __FUNCTION__,
                request->to_cpe.get_parameter_names->ParameterPath[0]);
            return soap_put__cwmp__GetParameterNames(soap, 
                            request->to_cpe.get_parameter_names,
                            "cwmp:GetParameterNames", "");
        case CWMP_RPC_download:
            return soap_put__cwmp__Download(soap, 
                            request->to_cpe.download,
                            "cwmp:Download", "");
        case CWMP_RPC_add_object:
            return soap_put__cwmp__AddObject(soap, 
                            request->to_cpe.add_object,
                            "cwmp:AddObject", ""); 
        case CWMP_RPC_delete_object:
            return soap_put__cwmp__DeleteObject(soap, 
                            request->to_cpe.delete_object,
                            "cwmp:DeleteObject", "");
        case CWMP_RPC_reboot:
            return soap_put__cwmp__Reboot(soap, 
                            request->to_cpe.reboot,
                            "cwmp:Reboot", "");
        case CWMP_RPC_factory_reset:
            {
                _cwmp__FactoryReset arg;
                soap_default__cwmp__FactoryReset(soap, &arg);
                return soap_put__cwmp__FactoryReset(soap, &arg,
                                            "cwmp:FactoryReset", "");
            }
            break;
        case CWMP_RPC_set_parameter_attributes:
        case CWMP_RPC_get_parameter_attributes:
        case CWMP_RPC_upload:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_get_options:
            RING("TODO send RPC with code %d", request->rpc_cpe);
#endif
        default:
            break; /* do nothing */
    }
    return 0;
}


/**
 * Serialize CWMP data into gSOAP buffer.
 * @return gSOAP status code.
 */
int 
acse_soap_serialize_cwmp(struct soap *soap, acse_epc_cwmp_data_t *request)
{
    switch(request->rpc_cpe)
    {
#define SOAP_SER_CWMP_GEN(_r, _f, _t) \
        case CWMP_RPC_##_r : \
            soap_serialize__cwmp__##_t (soap, request->to_cpe. _f); \
            break;

#define SOAP_SER_CWMP(_r, _t) SOAP_SER_CWMP_GEN(_r, _r, _t)
#define SOAP_SER_CWMP_EMPTY(_r, _t) SOAP_SER_CWMP_GEN(_r, p, _t)

        SOAP_SER_CWMP_EMPTY(get_rpc_methods, GetRPCMethods)
        SOAP_SER_CWMP(set_parameter_values, SetParameterValues)
        SOAP_SER_CWMP(get_parameter_values, GetParameterValues)
        SOAP_SER_CWMP(get_parameter_names, GetParameterNames)
        SOAP_SER_CWMP(download, Download)
        SOAP_SER_CWMP(add_object, AddObject)
        SOAP_SER_CWMP(delete_object, DeleteObject)
        SOAP_SER_CWMP(reboot, Reboot)
        SOAP_SER_CWMP_EMPTY(factory_reset, FactoryReset)
        SOAP_SER_CWMP(set_parameter_attributes, SetParameterAttributes)
        SOAP_SER_CWMP(get_parameter_attributes, GetParameterAttributes)
        SOAP_SER_CWMP(upload, Upload)
        SOAP_SER_CWMP_EMPTY(get_queued_transfers, GetQueuedTransfers)
        SOAP_SER_CWMP_EMPTY(get_all_queued_transfers, GetAllQueuedTransfers)
        SOAP_SER_CWMP(schedule_inform, ScheduleInform)
        SOAP_SER_CWMP(set_vouchers, SetVouchers)
        SOAP_SER_CWMP(get_options, GetOptions)

#undef SOAP_SER_CWMP_GEN
#undef SOAP_SER_CWMP_EMPTY
#undef SOAP_SER_CWMP

#if 0
        case CWMP_RPC_get_rpc_methods: 
        {
            _cwmp__GetRPCMethods arg;
            soap_default__cwmp__GetRPCMethods(soap, &arg);
            soap_serialize__cwmp__GetRPCMethods(soap, &arg);
        }
            break;
        case CWMP_RPC_set_parameter_values:
            soap_serialize__cwmp__SetParameterValues(soap,
                    request->to_cpe.set_parameter_values);
            break;
        case CWMP_RPC_get_parameter_values:
            soap_serialize__cwmp__GetParameterValues(soap,
                    request->to_cpe.get_parameter_values);
            break;
        case CWMP_RPC_get_parameter_names:
            soap_serialize__cwmp__GetParameterNames(soap,
                    request->to_cpe.get_parameter_names);
            break;
        case CWMP_RPC_download:
            soap_serialize__cwmp__Download(soap,
                    request->to_cpe.download);
            break;
        case CWMP_RPC_add_object:
            soap_serialize__cwmp__AddObject(soap,
                    request->to_cpe.add_object);
            break;
        case CWMP_RPC_delete_object:
            soap_serialize__cwmp__DeleteObject(soap,
                    request->to_cpe.delete_object);
            break;
        case CWMP_RPC_reboot:
            soap_serialize__cwmp__Reboot(soap,
                    request->to_cpe.reboot);
            break;
        case CWMP_RPC_factory_reset:
            {
                _cwmp__FactoryReset arg;
                soap_default__cwmp__FactoryReset(soap, &arg);
                soap_serialize__cwmp__FactoryReset(soap, &arg);
            }
            break;
        case CWMP_RPC_set_parameter_attributes:
        case CWMP_RPC_get_parameter_attributes:
        case CWMP_RPC_upload:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_get_options:
            RING("TODO send RPC with code %d", request->rpc_cpe);
#endif
        default: /* do nothing */
            break;
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
    cpe_rpc_item_t       *rpc_item;

    cpe_t *cpe = session->cpe_owner;

    INFO("%s() called, cwmp sess state %d", __FUNCTION__, session->state);

    if (TAILQ_EMPTY(&cpe->rpc_queue) && cpe->sync_mode)
    { 
        /* do nothing, wait for EPC with RPC to be sent */
        session->state = CWMP_PENDING;
        return 0; 
    }
    rpc_item = TAILQ_FIRST(&cpe->rpc_queue);

    /* TODO add check, whether HoldRequests was set on - think, for what? */

    if (TAILQ_EMPTY(&cpe->rpc_queue) || 
        CWMP_RPC_NONE == rpc_item->params->rpc_cpe)
    {

        INFO("CPE '%s', empty list of RPC calls, response 204", cpe->name);
        // soap->keep_alive = 0;
        acse_cwmp_send_http(soap, session, 204, NULL);
        if (rpc_item != NULL)
            TAILQ_REMOVE(&cpe->rpc_queue, rpc_item, links);
        return 0;
    }
    session->rpc_item = rpc_item;

    rpc_item->heap = mheap_create(session->rpc_item);
    mheap_add_user(rpc_item->heap, session);

    request = rpc_item->params;

    INFO("%s(): Try send RPC for '%s', rpc type %d, id %u ",
         __FUNCTION__, cpe->name, request->rpc_cpe, request->request_id);

    cwmp_prepare_soap_header(soap, cpe);
    acse_soap_default_req(soap, request);

    soap->keep_alive = 1; 
    soap->error = SOAP_OK;
    soap_serializeheader(soap);

    acse_soap_serialize_cwmp(soap, request);

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

    TAILQ_REMOVE(&cpe->rpc_queue, rpc_item, links);

    TAILQ_INSERT_TAIL(&cpe->rpc_results, rpc_item, links);

    return 0;
}


/* see description in acse_internal.h */
int 
acse_cwmp_send_http(struct soap *soap, cwmp_session_t *session,
                    int http_code, const char *str)
{
    INFO("CPE '%s', special HTTP response %d, '%s'",
         session ? session->cpe_owner->name : "unknown",
         http_code, str);
    // soap->keep_alive = 0;
    if (NULL != str)
    {
        size_t sz = strlen(str);
        if (sz >= sizeof(soap->endpoint))
        {
            WARN("gSOAP cannot process location with length %u", sz);
            return SOAP_LENGTH;
        }
        strcpy(soap->endpoint, str);
    }
    if (soap_begin_count(soap) ||
        soap_end_count(soap) ||
        soap_response(soap, http_code) ||
        soap_end_send(soap))
    {
        ERROR("%s(): gSOAP internal error %d", __FUNCTION__, soap->error);
        return soap->error;
    }
    if (NULL != session)
        session->state = CWMP_SERVE;
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

    VERB("%s(): soap error %d", __FUNCTION__, soap->error);

    if (NULL == session || NULL == (cpe = session->cpe_owner))
    {
        ERROR("Internal ACSE error at empty POST, soap %p, ss %p",
                soap, session);
        soap->keep_alive = 0;
        soap_closesock(soap);
        return 500;
    }

    if (session->state != CWMP_SERVE)
    {
        /* TODO: think, what would be correct in unexpected state?..
         * Now just close connection. */
        ERROR("Empty POST processing, cpe '%s', state is %d, not SERVE",
                cpe->name, session->state);
        soap->keep_alive = 0;
        soap_closesock(soap);
        return 500;
    } 

    return  acse_cwmp_send_rpc(soap, session);
}


/**
 * Get SOAP response into our internal structs. 
 */
int
acse_soap_get_response(struct soap *soap, acse_epc_cwmp_data_t *request)
{
#define SOAP_GET_RESPONSE(_rpc_name, _leaf) \
    do { \
            _cwmp__ ##_rpc_name *resp = soap_malloc(soap, sizeof(*resp)); \
            soap_default__cwmp__ ## _rpc_name(soap, resp);                \
            soap_get__cwmp__ ## _rpc_name(soap, resp,                     \
                                          "cwmp:" #_rpc_name , "");       \
            request->from_cpe. _leaf = resp;                              \
    } while(0)

    switch (request->rpc_cpe)
    {
        case CWMP_RPC_get_rpc_methods:
            SOAP_GET_RESPONSE(GetRPCMethodsResponse, get_rpc_methods_r);
            break;
        case CWMP_RPC_set_parameter_values:
            SOAP_GET_RESPONSE(SetParameterValuesResponse,
                              set_parameter_values_r);
            break;
        case CWMP_RPC_get_parameter_values:
            SOAP_GET_RESPONSE(GetParameterValuesResponse,
                              get_parameter_values_r);
            break;
        case CWMP_RPC_get_parameter_names:
            SOAP_GET_RESPONSE(GetParameterNamesResponse,
                              get_parameter_names_r);
            break;
        case CWMP_RPC_download:
            SOAP_GET_RESPONSE(DownloadResponse, download_r);
            break; 
        case CWMP_RPC_add_object:
            SOAP_GET_RESPONSE(AddObjectResponse, add_object_r);
            break;
        case CWMP_RPC_delete_object:
            SOAP_GET_RESPONSE(DeleteObjectResponse, delete_object_r);
            break;
        case CWMP_RPC_reboot:
            /* RebootResponse is empty, so do not store it */
            SOAP_GET_RESPONSE(RebootResponse, p);
            break;
        case CWMP_RPC_factory_reset:
            /* FactoryResetResponse is empty, so do not store it */
            SOAP_GET_RESPONSE(FactoryResetResponse, p);
            break;
        case CWMP_RPC_set_parameter_attributes:
            /* FactoryResetResponse is empty, so do not store it */
            SOAP_GET_RESPONSE(SetParameterAttributesResponse, p);
            break;
        case CWMP_RPC_get_parameter_attributes:
            SOAP_GET_RESPONSE(GetParameterAttributesResponse, 
                              get_parameter_attributes_r);
            break;
        case CWMP_RPC_upload:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_get_options:
            /* TODO */
            RING("TODO receive RPC resp with code %d", request->rpc_cpe);
        default:
            break;
    }
#undef SOAP_GET_RESPONSE
    return 0;
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

    if (!soap_getfault(soap))
    {
        soap_print_fault(soap, stderr);
        if(soap->fault && soap->fault->detail)
        {
            RING("%s(): fault SOAP type %d.", __FUNCTION__, 
                 soap->fault->detail->__type);
            if (SOAP_TYPE__cwmp__Fault == soap->fault->detail->__type)
            {
                _cwmp__Fault *c_fault = soap->fault->detail->fault;
                WARN("CWMP fault %s (%s), size %d",
                    c_fault->FaultCode, c_fault->FaultString,
                    c_fault->__sizeSetParameterValuesFault);
                request->from_cpe.fault = c_fault;
                request->rpc_cpe = CWMP_RPC_FAULT;
            }
        }
        WARN("%s(): Fault received '%s'",
                __FUNCTION__, *(soap_faultdetail(soap)));
        /* do not return here, we have to continue normal CWMP session.*/
    }
    else 
        acse_soap_get_response(soap, request);

    cwmp_sess->rpc_item = NULL; /* It is already processed. */

    if (soap->error)
    {
        ERROR("Fail get SOAP response, error %d", soap->error);
        return;
    }
    if (soap_body_end_in(soap)
         || soap_envelope_end_in(soap)
         || soap_end_recv(soap))
    {
        ERROR("after get SOAP body, error %d", soap->error);
        return;
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
