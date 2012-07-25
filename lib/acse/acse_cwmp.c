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
#include <sys/socket.h>
#include <unistd.h>

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <poll.h>

/* For timer_create(), etc. */
#include <signal.h>
#include <time.h>

/* ACSE headers */
#include "acse_internal.h"
#include "httpda.h"

/* TE headers */
#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "te_sockaddr.h"
#include "logger_api.h"

#include "acse_soapH.h"
#include "acse_mem.h"

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

static int susp_dummy_pipe[2] = {-1, -1};

/**
 * Force stop CWMP session ignoring its current state and
 * protocol semantic, really just close TCP connection and
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
    static char tmpbuf[SEND_FILE_BUF];
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

    RING("acse_http_get(): GET to '%s' received", soap->path);

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
        char msgbuf[512];

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
        soap->count =
        soap->length = snprintf(msgbuf, sizeof(msgbuf),
                                "<html><body>%s</body></html>\r\n",
                                err_descr);
        session->state = CWMP_CLOSE;
        soap->keep_alive = 0;
        soap_response(soap, http_status);
        soap_send(soap, msgbuf);
        soap_end_send(soap);
        soap->error = SOAP_OK;
        return SOAP_OK;
    }

    RING("%s(): reply with %d bytes...",
         __FUNCTION__ , (int)fs.st_size);

    session->sending_fd = fd;

    soap->http_content = "application/octet-stream";
    soap->length = fs.st_size;

    soap_response(soap, SOAP_FILE);
    session->state = CWMP_SEND_FILE;
    acse_send_file_portion(session);
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
    char *subname_place;

    for (i = 0; i < cwmp__Inform->ParameterList->__size; i++)
    {
        param_v =
            cwmp__Inform->ParameterList->__ptrParameterValueStruct[i];
        subname_place = index(param_v->Name, '.');

        VERB("%s, param name '%s', \n    val '%s', subname '%s'",
             __FUNCTION__, param_v->Name, param_v->Value, subname_place);
        if (subname_place == NULL)
            continue;
        if (strcmp(subname_place,
                   ".ManagementServer.ConnectionRequestURL") == 0)
        {
            free((char *)cpe_item->url); /* const_cast */
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
                ERROR("Digest Auth failed: wrong realm '%s', need '%s'",
                      soap->authrealm, authrealm);
                break;
            }

            LIST_FOREACH(cpe_item, &(session->acs_owner->cpe_list), links)
            {
                if (strcmp(cpe_item->acs_auth.login, soap->userid) == 0)
                    break;
            }

            if (cpe_item == NULL)
            {
                ERROR("%s: userid '%s' not found, auth fail",
                      __FUNCTION__, soap->userid);
                break;
            }

            VERB("check auth for user '%s', pass '%s'",
                 soap->userid, cpe_item->acs_auth.passwd);
            if (session->acs_owner->auth_mode == ACSE_AUTH_DIGEST)
            {
                if (http_da_verify_post(soap, cpe_item->acs_auth.passwd))
                {
                    ERROR("%s: Digest Auth verify for '%s' failed",
                          __FUNCTION__, cpe_item->acs_auth.passwd);
                    break;
                }
            }
            else if (strcmp(soap->passwd, cpe_item->acs_auth.passwd) != 0)
            {
                ERROR("%s: Basic Auth failed passwds differs '%s' != '%s'",
                      __FUNCTION__, soap->passwd,
                      cpe_item->acs_auth.passwd);
                break;
            }

            RING("%s: Authentication passed, CPE '%s', username '%s'",
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


int
acse_check_auth(struct soap *soap, cpe_t *cpe)
{
    if (cpe->acs->auth_mode == ACSE_AUTH_DIGEST &&
        (soap->userid == NULL || strlen(soap->userid) == 0) )
    {

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

    if (strcmp(cpe->acs_auth.login, soap->userid) != 0)
    {
        RING("Auth failed for CPE %s, incoming login '%s'",
             cpe->name, soap->userid);
        return FALSE;
    }
    if (cpe->acs->auth_mode == ACSE_AUTH_DIGEST)
    {
        if (http_da_verify_post(soap, cpe->acs_auth.passwd))
        {
            ERROR("%s: Digest Auth verify for '%s' failed",
                  __FUNCTION__, cpe->acs_auth.passwd);
            return FALSE;
        }
    }
    else if (strcmp (soap->passwd, cpe->acs_auth.passwd) != 0)
    {
        ERROR("%s: Basic Auth failed passwds differs '%s' = '%s'",
              __FUNCTION__, soap->passwd, cpe->acs_auth.passwd);
        return FALSE;
    }

    return TRUE;
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
    static struct MethodList m_list;

    m_list.__size = sizeof(acs_method_list)/sizeof(acs_method_list[0]);
    m_list.__ptrstring = acs_method_list;

    UNUSED(soap);
    UNUSED(cwmp__GetRPCMethods);

    cwmp__GetRPCMethodsResponse->MethodList_ = &m_list;

    return 0;
}


static void
cwmp_prepare_soap_header(struct soap *soap, cpe_t *cpe)
{
    if (NULL == soap->header)
    {
        soap->header = soap_malloc(soap, sizeof(struct SOAP_ENV__Header));
        memset(soap->header, 0, sizeof(*(soap->header)));
    }

    soap->encodingStyle = NULL;

    if (cpe->hold_requests >= 0)
    {
        if (NULL == soap->header->cwmp__HoldRequests)
        {
            soap->header->cwmp__HoldRequests =
                    soap_malloc(soap, sizeof(_cwmp__HoldRequests));
        }
        soap->header->cwmp__HoldRequests->__item = cpe->hold_requests;
        soap->header->cwmp__HoldRequests->SOAP_ENV__mustUnderstand = "1";
    }
    else /* Negative value note that field should absent. */
    {
        soap->header->cwmp__HoldRequests = NULL;
    }
    soap->header->cwmp__ID = NULL;
    soap->keep_alive = 1;


    if ((soap->header == NULL ||
         soap->header->cwmp__HoldRequests == NULL ||
         soap->header->cwmp__HoldRequests->__item == 0) &&
        CWMP_EP_CLEAR == cpe->session->ep_status)
    {
        VERB("CPE '%s', set empPost status to Wait", cpe->name);
        cpe->session->ep_status = CWMP_EP_WAIT;
    }
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

    if (NULL == session)
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

        VERB("ACS '%s': process Inform, HTTP response set, %d, %s",
            session->acs_owner->name, http_code, loc);

        acse_cwmp_send_http(soap, NULL,
                            session->acs_owner->http_response->http_code,
                            session->acs_owner->http_response->location);

        free(session->acs_owner->http_response);
        session->acs_owner->http_response = NULL;

        return SOAP_STOP; /* HTTP response already sent. */
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

        if (NULL != cpe_item->http_response)
        {
            int http_code = cpe_item->http_response->http_code;
            char *loc = cpe_item->http_response->location;
            VERB("Process Inform, for CPE is HTTP response setting, %d, %s",
                http_code, loc);

            acse_cwmp_send_http(soap, session,
                                cpe_item->http_response->http_code,
                                cpe_item->http_response->location);

            free(cpe_item->http_response);
            cpe_item->http_response = NULL;

            return SOAP_STOP; /* HTTP response already sent. */
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

    VERB("CPE %s, now send InformResponse, empPost is %d",
         cpe_item->name, session->ep_status);

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

    if (NULL == session)
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

    if (!acse_check_auth(soap, cpe_item))
    {
        WARN("%s(): Auth failed.", __FUNCTION__);
        session->state = CWMP_WAIT_AUTH;
        return SOAP_STOP; /* HTTP response already sent. */
    }

    cpe_store_acs_rpc(CWMP_RPC_transfer_complete, cwmp__TransferComplete,
                      cpe_item, session->def_heap);

    cwmp_prepare_soap_header(soap, cpe_item);

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

    if (NULL == session)
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
 * @param deadline  Timestamp until wait, keep untouched if not need (OUT)
 *
 * @return status code.
 */
te_errno
cwmp_before_poll(void *data, struct pollfd *pfd, struct timeval *deadline)
{
    cwmp_session_t *cwmp_sess = data;

    VERB("before poll, sess ptr %p, state %d, soap status %d",
         cwmp_sess, cwmp_sess->state, cwmp_sess->m_soap.error);

    if (deadline != NULL && cwmp_sess->last_sent.tv_sec > 0)
    {
        deadline->tv_sec  = cwmp_sess->last_sent.tv_sec + CWMP_TIMEOUT;
        deadline->tv_usec = cwmp_sess->last_sent.tv_usec;
        VERB("before poll, set deadline %d.%d",
            (int)deadline->tv_sec, (int)deadline->tv_usec);
    }

    if (cwmp_sess == NULL ||
        cwmp_sess->state == CWMP_NOP)
    {
        return TE_EINVAL;
    }

    if (cwmp_sess != NULL && cwmp_sess->state == CWMP_SUSPENDED)
    {
        pfd->fd = susp_dummy_pipe[0];
        pfd->events = POLLIN;
        pfd->revents = 0;

        return 0;
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
    te_errno rc = 0;
    assert(cwmp_sess != NULL);

    VERB("Start after poll, sess ptr %p, state %d, SOAP error %d",
          cwmp_sess, cwmp_sess->state, cwmp_sess->m_soap.error);

    if (pfd == NULL)
    {
        WARN("after serve %s %s/%s timeout occured (pfd is NULL)",
            cwmp_sess->acs_owner ? "ACS" : "CPE",
            cwmp_sess->acs_owner ? cwmp_sess->acs_owner->name :
                                 cwmp_sess->cpe_owner->acs->name,
            cwmp_sess->acs_owner ? "(none)":
                                 cwmp_sess->cpe_owner->name);

        switch (cwmp_sess->state)
        {
            case CWMP_WAIT_AUTH:
            case CWMP_WAIT_RESPONSE:
            case CWMP_SERVE:
                if (cwmp_sess->m_soap.socket >= 0)
                {
                    close(cwmp_sess->m_soap.socket);
                    cwmp_sess->m_soap.socket = -1;
                }
                /* fall through... */
            case CWMP_SUSPENDED:
                RING("%s: pfd is NULL, closing sess %p in state %d",
                      __FUNCTION__, cwmp_sess, cwmp_sess->state);
                return TE_ENOTCONN;
            default:
                WARN("CWMP session state %d, unexpected timeout",
                     cwmp_sess->state);
        }
        return 0;
    }

    if (!(pfd->revents))
        return 0;

    switch (cwmp_sess->state)
    {
        case CWMP_LISTEN:
        case CWMP_WAIT_AUTH:
        case CWMP_SERVE:
            /* Now, after poll() on soap socket, it should not block */
            soap_serve(&cwmp_sess->m_soap);
            VERB("after serve, sess ptr %p, state %d, SOAP error %d",
                  cwmp_sess, cwmp_sess->state, cwmp_sess->m_soap.error);
            if (cwmp_sess->m_soap.error == SOAP_EOF)
            {
                VERB("after serve %s %s/%s(sess ptr %p, state %d): EOF",
                    cwmp_sess->acs_owner ? "ACS" : "CPE",
                    cwmp_sess->acs_owner ? cwmp_sess->acs_owner->name :
                                         cwmp_sess->cpe_owner->acs->name,
                    cwmp_sess->acs_owner ? "(none)":
                                         cwmp_sess->cpe_owner->name,
                    cwmp_sess, cwmp_sess->state);

                if (CWMP_EP_WAIT == cwmp_sess->ep_status)
                {
                    cwmp_suspend_session(cwmp_sess);
                    return 0;
                }
                else
                {
                    RING("%s: EOF in state %d ep_status %d, "
                         "closing sess %p", __FUNCTION__, cwmp_sess->state,
                         cwmp_sess->ep_status, cwmp_sess);
                    return TE_ENOTCONN;
                }
            }
            break;

        case CWMP_WAIT_RESPONSE:
            /* Now, after poll() on soap socket, it should not block */
            rc = acse_soap_serve_response(cwmp_sess);
            if (cwmp_sess->m_soap.error == SOAP_EOF)
            {
                RING("after serve %s %s/%s(sess ptr %p, state %d): EOF",
                    cwmp_sess->acs_owner ? "ACS" : "CPE",
                    cwmp_sess->acs_owner ? cwmp_sess->acs_owner->name :
                                         cwmp_sess->cpe_owner->acs->name,
                    cwmp_sess->acs_owner ? "(none)":
                                         cwmp_sess->cpe_owner->name,
                    cwmp_sess, cwmp_sess->state);
                return TE_ENOTCONN;
            }
            if (rc != 0)
                RING("acse_soap_serve_response returned rc %r", rc);
            break;
        case CWMP_SEND_FILE:
            if (pfd->revents & POLLOUT)
                return acse_send_file_portion(cwmp_sess);
            break;
        case CWMP_PENDING:
            if (pfd->revents & POLLIN)
            {
                char buf[1024];
                size_t r;

                r = cwmp_sess->orig_frecv(&(cwmp_sess->m_soap), buf,
                                          sizeof(buf));
                if (r == 0)
                {
                    WARN("Unexpected EOF in state PENDING, ACS/CPE %s/%s",
                         cwmp_sess->cpe_owner->acs->name,
                         cwmp_sess->cpe_owner->name);
                    return TE_ENOTCONN;
                }
                else
                {
                    WARN("Unexpected data (%d b) in state PENDING; %s/%s",
                         r, cwmp_sess->cpe_owner->acs->name,
                         cwmp_sess->cpe_owner->name);
                }
            }
            else
            {
                int saved_errno = errno;

                ERROR("Unexpected PENDING, CPE %s, revents 0x%x, errno %d",
                     cwmp_sess->cpe_owner->name, (int)pfd->revents,
                     saved_errno);
                return TE_EFAIL;
            }
            break;
        case CWMP_CLOSE:
            RING("%s: session %p state is CLOSE", __FUNCTION__, cwmp_sess);
            return TE_ENOTCONN;
        case CWMP_SUSPENDED:
            break;
        default: /* do nothing here */
            WARN("CWMP after poll, unexpected state %d\n",
                    (int)cwmp_sess->state);
            break;
    }

    return rc;
}


/**
 * Callback for I/O ACSE channel, called at channel destroy.
 * Its prototype matches with field #channel_t::destroy.
 *
 * @param data      Channel-specific private data.
 */
void
cwmp_destroy(void *data)
{
    cwmp_session_t *cwmp_sess = data;

    cwmp_close_session(cwmp_sess);
}


/* see description in acse_internal.h */
te_errno
cwmp_accept_cpe_connection(acs_t *acs, int socket)
{
    int is_not_get = 1;
    cpe_t *cpe;
    static char addr_name[100] = {0,};

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
        buf[len] = '\0';
        VERB("cwmp_accept_cpe_conn(): peeked msg buf: '%s'", buf);
        if (strncmp(buf, "POST ", 5) &&
            (is_not_get = strncmp(buf, "GET ", 4)))
            return TE_ECONNREFUSED; /* It is not POST or GET request */
        req_url_p = buf + 4;
        while (isspace(*req_url_p)) req_url_p++;

        /* without any URL specified we accept all connections */
        if (acs->url != NULL &&
            strncmp(req_url_p, acs->url, strlen(acs->url)))
        {
            RING("CWMP NOT accepted, ACS '%s', our URL '%s', come URL '%s'",
                 acs->name, acs->url, req_url_p);
            return TE_ECONNREFUSED; /* It is not our URL */
        }
    }
    /*
      Now we try find suspended CWMP session, if it is our URL.
     */
    LIST_FOREACH(cpe, &(acs->cpe_list), links)
    {
        if (cpe->session != NULL && CWMP_SUSPENDED == cpe->session->state)
        {
            /* match IP address */
            static struct sockaddr_storage peer_addr;
            size_t a_len = sizeof(peer_addr);
            void *in_a = NULL;
            void *susp_a = NULL;
            size_t mlen = 4;

            if (getpeername(socket, SA(&peer_addr), &a_len) < 0)
            {
                perror("getpeername(): ");
                break;
            }

            switch (peer_addr.ss_family)
            {
                case AF_INET:
                    in_a = &(SIN(&peer_addr)->sin_addr);
                    susp_a = &(SIN(&(cpe->session->cpe_addr))->sin_addr);
                    mlen = 4;
                    break;
                case AF_INET6:
                    in_a = &(SIN6(&peer_addr)->sin6_addr);
                    susp_a = &(SIN6(&(cpe->session->cpe_addr))->sin6_addr);
                    mlen = 16;
                    break;
            }

            memset(addr_name, 0, sizeof(addr_name));
            if (inet_ntop(peer_addr.ss_family, in_a,
                          addr_name, sizeof(addr_name))
                != NULL)
            {
                RING("%s: found suspended session, al %d, "
                      "match it with incoming addr '%s', l %d",
                     __FUNCTION__, cpe->session->cpe_addr_len,
                     addr_name, (int)a_len);
            }
            else
                perror("CWMP accept, inet_ntop failed: ");

            if (peer_addr.ss_family == cpe->session->cpe_addr.ss_family &&
                memcmp(in_a, susp_a, mlen) == 0)
            {
                RING("%s: address matches, resume session", __FUNCTION__);
                return cwmp_resume_session(cpe->session, socket);
            }
            RING("%s: address do not matches.... :(", __FUNCTION__);
        }
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
    VERB("cwmp_fparse, rc %d, soap err %d, soap len %d",
         rc, soap->error, soap->length);

    if (rc == SOAP_OK && soap->length == 0)
        rc = SOAP_STOP;

    return rc;
}

/* This is really (LOGFORK_MAXLEN - prefix), but that constant is internal,
and maynot not be used here. */
#define LOG_MAX 4000

/** Callback fsend for gSOAP, to log XML output */
int
acse_send(struct soap *soap, const char *s, size_t n)
{
    cwmp_session_t *session = (cwmp_session_t *)soap->user;
    size_t          log_len = n;
    char           *log_buf;

    if (((session->acs_owner) &&
         (session->acs_owner->traffic_log)) ||
        ((session->cpe_owner) &&
         (session->cpe_owner->acs->traffic_log)))
    {
        if (log_len >= LOG_MAX)
            log_len = LOG_MAX-1;
        log_buf = mheap_alloc(session->def_heap, log_len + 1);

        if (NULL != log_buf)
        {
            memcpy(log_buf, s, log_len);
            log_buf[log_len] = '\0';

            RING("Send %u bytes to %s %s/%s: (printed %u bytes)\n%s", n,
                session->acs_owner ? "ACS" : "CPE",
                session->acs_owner ? session->acs_owner->name :
                                     session->cpe_owner->acs->name,
                session->acs_owner ? "(none)" : session->cpe_owner->name,
                log_len, log_buf);
        }
    }
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
    char           *log_buf;

    /* call standard gSOAP frecv */
    log_len = rc = session->orig_frecv(soap, s, n);

    if (log_len >= LOG_MAX)
        log_len = LOG_MAX-1;

    if (((session->acs_owner) &&
         (session->acs_owner->traffic_log)) ||
        ((session->cpe_owner) &&
         (session->cpe_owner->acs->traffic_log)))
    {
        log_buf = mheap_alloc(session->def_heap, log_len + 1);

        if (NULL != log_buf)
        {
            memcpy(log_buf, s, log_len);
            log_buf[log_len] = '\0';

            RING("Recv %u bytes from %s %s/%s: (pr %u bytes)\n%s", rc,
                session->acs_owner ? "ACS" : "CPE",
                session->acs_owner ? session->acs_owner->name :
                                     session->cpe_owner->acs->name,
                session->acs_owner ? "(none)" : session->cpe_owner->name,
                log_len, log_buf);
        }
    }
    return rc;
}

te_errno
cwmp_init_soap(cwmp_session_t *sess, int socket)
{
    assert(sess != NULL);

    memset(&(sess->m_soap), 0, sizeof(struct soap));

    soap_init(&(sess->m_soap));
    /* TODO: investigate more correct way to set version,
       maybe specify correct SOAPENV in namespaces .. */
    sess->m_soap.version = 1;

    sess->m_soap.user = sess;
    sess->m_soap.socket = socket;
    sess->m_soap.fserveloop = cwmp_serveloop;
    sess->m_soap.fmalloc = acse_cwmp_malloc;
    sess->m_soap.fget = acse_http_get;

    soap_imode(&sess->m_soap, SOAP_IO_KEEPALIVE);
    soap_omode(&sess->m_soap, SOAP_IO_KEEPALIVE);

    sess->m_soap.max_keep_alive = 10;

    sess->orig_fparse = sess->m_soap.fparse;
    sess->m_soap.fparse = cwmp_fparse;
    sess->orig_fsend = sess->m_soap.fsend;
    sess->m_soap.fsend = acse_send;
    sess->orig_frecv = sess->m_soap.frecv;
    sess->m_soap.frecv = acse_recv;

    return 0;
}

/* see description in acse_internal.h */
te_errno
cwmp_new_session(int socket, acs_t *acs)
{
    cwmp_session_t *new_sess = malloc(sizeof(*new_sess));
    channel_t      *channel  = malloc(sizeof(*channel));

    if (new_sess == NULL || channel == NULL)
        return TE_ENOMEM;

    new_sess->ep_status = CWMP_EP_CLEAR;
    new_sess->last_sent.tv_sec = 0;
    new_sess->cpe_addr_len = sizeof(new_sess->cpe_addr);
    getpeername(socket, SA(&(new_sess->cpe_addr)),
               &(new_sess->cpe_addr_len));

    new_sess->state = CWMP_NOP;
    new_sess->acs_owner = acs;
    new_sess->cpe_owner = NULL;
    new_sess->channel = channel;
    new_sess->rpc_item = NULL;
    new_sess->sending_fd = NULL;
    new_sess->def_heap = mheap_create(new_sess);

    cwmp_init_soap(new_sess, socket);

    VERB("Init session for ACS '%s', sess ptr %p, acs ptr %p",
         acs->name, new_sess, acs);
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
            mheap_free_user(new_sess->def_heap, new_sess);
            free(new_sess);
            free(channel);
            /* TODO: what error return here? */
            return TE_ECONNREFUSED;
        }
        if (soap_ssl_accept(&new_sess->m_soap))
        {
            RING("soap_ssl_accept failed, soap error %d",
                new_sess->m_soap.error);
            soap_done(&new_sess->m_soap);
            mheap_free_user(new_sess->def_heap, new_sess);
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

    channel->data = new_sess;
    channel->before_poll = cwmp_before_poll;
    channel->after_poll = cwmp_after_poll;
    channel->destroy = cwmp_destroy;
    channel->name = strdup("CWMP-session");

    new_sess->state = CWMP_LISTEN;
    acse_add_channel(channel);

    return 0;
}

void
cwmp_close_session(cwmp_session_t *sess)
{
    assert(NULL != sess);
    assert((NULL != sess->acs_owner) || (NULL != sess->cpe_owner));

    RING("close cwmp session (sess ptr %p) on %s '%s/%s'", sess,
          sess->acs_owner ? "ACS" : "CPE",
          sess->acs_owner ? sess->acs_owner->name :
                            sess->cpe_owner->acs->name,
          sess->acs_owner ? "(none)" : sess->cpe_owner->name);

    /* TODO: investigate, what else should be closed. */

    /* free all heaps, where this session was user of memory */
    mheap_free_user(MHEAP_NONE, sess);

    if (sess->m_soap.socket >= 0)
    {
        close(sess->m_soap.socket);
        sess->m_soap.socket = -1;
    }

    if (CWMP_SUSPENDED != sess->state)
    {
        soap_dealloc(&(sess->m_soap), NULL);
        soap_end(&(sess->m_soap));
        soap_done(&(sess->m_soap));
    }

    if (sess->acs_owner)
        sess->acs_owner->session = NULL;
    if (sess->cpe_owner)
        sess->cpe_owner->session = NULL;

    free(sess);
}

/**
 * Suspend CWMP session due to terminating of TCP connection.
 */
te_errno
cwmp_suspend_session(cwmp_session_t *sess)
{
    static char addr_name[100] = {0,};
    assert(NULL != sess);
    assert((NULL != sess->acs_owner) || (NULL != sess->cpe_owner));

    if (susp_dummy_pipe[0] < 0)
        pipe(susp_dummy_pipe);

    if (inet_ntop(sess->cpe_addr.ss_family,
              &(SIN(&(sess->cpe_addr))->sin_addr),
              addr_name, sizeof(addr_name)) != NULL)
    {

    RING("suspend cwmp session (sess ptr %p) on %s '%s/%s' from addr '%s'",
          sess,
          sess->acs_owner ? "ACS" : "CPE",
          sess->acs_owner ? sess->acs_owner->name :
                            sess->cpe_owner->acs->name,
          sess->acs_owner ? "(none)" : sess->cpe_owner->name,
          addr_name);
    }
    else
        perror("suspend cwmp session, inet_ntop failed:");

    if (sess->m_soap.socket >= 0)
    {
        close(sess->m_soap.socket);
        sess->m_soap.socket = -1;
    }

    soap_dealloc(&(sess->m_soap), NULL);
    soap_end(&(sess->m_soap));
    soap_done(&(sess->m_soap));

    sess->state = CWMP_SUSPENDED;

    return 0;
}



te_errno
cwmp_resume_session(cwmp_session_t *sess, int socket)
{
    struct soap *soap;
    cwmp_init_soap(sess, socket);

    soap = &(sess->m_soap);

    sess->state = CWMP_SERVE;

    if (sess->cpe_owner->acs->auth_mode == ACSE_AUTH_DIGEST)
    {
        soap_register_plugin(soap, http_da);
    }
    return 0;
}



void
acse_timer_handler(int sig, siginfo_t *info, void *p)
{
    cwmp_session_t *sess;
    UNUSED(sig);
    UNUSED(p);

    sess = info->si_value.sival_ptr;
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

    switch (request->rpc_cpe)
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
    switch (request->rpc_cpe)
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
    switch (request->rpc_cpe)
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

    rpc_item = TAILQ_FIRST(&cpe->rpc_queue);

    RING("%s() called, cwmp sess state %d, sync_mode %d, rpc_item %p",
         __FUNCTION__, session->state, cpe->sync_mode, rpc_item);

    if (TAILQ_EMPTY(&cpe->rpc_queue) && cpe->sync_mode)
    {
        /* do nothing, wait for EPC with RPC to be sent */
        RING("sess %p queue is empty, sync mode; state <- PENDING",
             session);
        session->state = CWMP_PENDING;
        session->last_sent.tv_sec = 0;
        return 0;
    }

    if (cpe->chunk_mode)
        soap_set_omode(soap, SOAP_IO_CHUNK);
    else
        soap_clr_omode(soap, SOAP_IO_CHUNK);

    /* TODO add check, whether HoldRequests was set on - think, for what? */

    if (TAILQ_EMPTY(&cpe->rpc_queue) ||
        CWMP_RPC_NONE == rpc_item->params->rpc_cpe)
    {

        RING("CPE '%s', empty list of RPC calls, response 204", cpe->name);
        acse_cwmp_send_http(soap, session, 204, NULL);
        if (rpc_item != NULL)
            TAILQ_REMOVE(&cpe->rpc_queue, rpc_item, links);
        return 0;
    }
    session->rpc_item = rpc_item;

    rpc_item->heap = mheap_create(session->rpc_item);
    mheap_add_user(rpc_item->heap, session);

    request = rpc_item->params;

    INFO("%s(): Sending RPC %s to CPE '%s', id %u",
         __FUNCTION__, cwmp_rpc_cpe_string(request->rpc_cpe),
         cpe->name, request->request_id);

    cwmp_prepare_soap_header(soap, cpe);
    acse_soap_default_req(soap, request);

    soap->keep_alive = 1;
    soap->error = SOAP_OK;
    soap_serializeheader(soap);

    acse_soap_serialize_cwmp(soap, request);

    if (soap_begin_count(soap))
    {
        ERROR("%s: 0, soap error %d", __FUNCTION__, soap->error);
        return soap->error;
    }

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
    gettimeofday(&(session->last_sent), NULL);
    VERB("%s(): RPC %s sent, set last_sent to %d.%d",
         __FUNCTION__, cwmp_rpc_cpe_string(request->rpc_cpe),
         (int)session->last_sent.tv_sec,
         (int)session->last_sent.tv_usec);

    TAILQ_REMOVE(&cpe->rpc_queue, rpc_item, links);

    TAILQ_INSERT_TAIL(&cpe->rpc_results, rpc_item, links);

    return SOAP_OK;
}


/* see description in acse_internal.h */
int
acse_cwmp_send_http(struct soap *soap, cwmp_session_t *session,
                    int http_code, const char *str)
{
    INFO("CPE '%s', special HTTP response %d, '%s'",
         session ? session->cpe_owner->name : "unknown",
         http_code, str);
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
    {
        gettimeofday(&(session->last_sent), NULL);
        VERB("%s():%d set last_sent %d.%d",
             __FUNCTION__, __LINE__,
             (int)session->last_sent.tv_sec,
             (int)session->last_sent.tv_usec);
        session->state = CWMP_SERVE;
    }
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

    if (session != NULL && session->state == CWMP_CLOSE)
        return SOAP_OK;

    if (NULL != session && CWMP_EP_WAIT == session->ep_status)
    {
        RING("CPE '%s', sess %p, set empPost to GOT",
             session->cpe_owner->name, session);
        session->ep_status = CWMP_EP_GOT;
    }

    if (NULL == session || NULL == (cpe = session->cpe_owner))
    {

        ERROR("Internal ACSE error at empty POST, "
               "soap %p, ss %p, soap_err %d",
                soap, session, soap->error);
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

    return acse_cwmp_send_rpc(soap, session);
}

/**
 * Check for type of SOAP RPC in incoming buffer.
 */
static te_cwmp_rpc_cpe_t
acse_soap_get_response_rpc_id(struct soap *soap)
{
    soap_peek_element(soap);

    if (soap_match_tag(soap, soap->tag, "SOAP-ENV:Fault") == 0)
        return CWMP_RPC_FAULT;

#define SOAP_MATCH_RESPONSE(_rpc_name, _id) \
    do {                                                                \
        if (soap_match_tag(soap, soap->tag,                             \
                           "cwmp:" #_rpc_name "Response" ) == 0)        \
            return CWMP_RPC_ ## _id;                                    \
    } while (0)

    SOAP_MATCH_RESPONSE(GetRPCMethods, get_rpc_methods);
    SOAP_MATCH_RESPONSE(SetParameterValues, set_parameter_values);
    SOAP_MATCH_RESPONSE(GetParameterValues, get_parameter_values);
    SOAP_MATCH_RESPONSE(GetParameterNames, get_parameter_names);
    SOAP_MATCH_RESPONSE(SetParameterAttributes, set_parameter_attributes);
    SOAP_MATCH_RESPONSE(GetParameterAttributes, get_parameter_attributes);
    SOAP_MATCH_RESPONSE(AddObject, add_object);
    SOAP_MATCH_RESPONSE(DeleteObject, delete_object);
    SOAP_MATCH_RESPONSE(Reboot, reboot);
    SOAP_MATCH_RESPONSE(Download, download);
    SOAP_MATCH_RESPONSE(Upload, upload);
    SOAP_MATCH_RESPONSE(FactoryReset, factory_reset);
    SOAP_MATCH_RESPONSE(GetQueuedTransfers, get_queued_transfers);
    SOAP_MATCH_RESPONSE(GetAllQueuedTransfers, get_all_queued_transfers);
    SOAP_MATCH_RESPONSE(ScheduleInform, schedule_inform);
    SOAP_MATCH_RESPONSE(SetVouchers, set_vouchers);
    SOAP_MATCH_RESPONSE(GetOptions, get_options);
#undef SOAP_MATCH_RESPONSE

    return CWMP_RPC_NONE;
}


/**
 * Get SOAP response into our internal structs.
 */
static te_errno
acse_soap_get_response(struct soap *soap, acse_epc_cwmp_data_t *request)
{
    te_cwmp_rpc_cpe_t received_rpc;
    te_errno rc = 0;

#define SOAP_GET_RESPONSE(_rpc_name, _leaf) \
    do {                                                                   \
            _cwmp__ ##_rpc_name *resp = soap_malloc(soap, sizeof(*resp));  \
                                                                           \
            soap_default__cwmp__ ## _rpc_name(soap, resp);                 \
            request->from_cpe. _leaf =                                     \
                soap_get__cwmp__ ## _rpc_name(soap, resp,                  \
                                              "cwmp:" #_rpc_name , "");    \
            if (request->from_cpe. _leaf == NULL)                          \
                rc = TE_GSOAP_ERROR;                                       \
    } while(0)

    assert(request != NULL);

    if (soap_envelope_begin_in(soap) != 0 ||
        soap_recv_header(soap) != 0 ||
        soap_body_begin_in(soap) != 0)
    {
        return TE_GSOAP_ERROR;
    }
    received_rpc = acse_soap_get_response_rpc_id(soap);

    if (received_rpc != CWMP_RPC_FAULT && received_rpc != request->rpc_cpe)
    {
        ERROR("Received RPC '%s' while expecting %sResponse",
              soap->tag, cwmp_rpc_cpe_string(request->rpc_cpe));
        request->rpc_cpe = CWMP_RPC_NONE;
        request->from_cpe.p = NULL;
        return TE_EFAIL;
    }
    switch (received_rpc)
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
            return TE_EOPNOTSUPP;
            break;
        case CWMP_RPC_NONE:
            assert(0);
            break;
        case CWMP_RPC_FAULT:
        {
            _cwmp__Fault *c_fault;

            if (soap_getfault(soap) != 0)
                return TE_GSOAP_ERROR;

            /* soap_print_fault(soap, stderr); */
            if (soap->fault->detail == NULL)
            {
                ERROR("%s: SOAP fault does not have 'detail' element",
                      __FUNCTION__);
                return TE_EFAIL;
            }
            RING("%s(): fault SOAP type %d.", __FUNCTION__,
                 soap->fault->detail->__type);
            if (SOAP_TYPE__cwmp__Fault != soap->fault->detail->__type)
            {
                ERROR("%s: SOAP fault does not have 'cwmp:Fault' element",
                      __FUNCTION__);
                return TE_EFAIL;
            }
            c_fault = soap->fault->detail->fault;
            WARN("CWMP fault received %s (%s), SetParameterValuesFaults %d",
                 c_fault->FaultCode, c_fault->FaultString,
                 c_fault->__sizeSetParameterValuesFault);

            request->from_cpe.fault = c_fault;
            request->rpc_cpe = CWMP_RPC_FAULT;
            break;
        }
    }
#undef SOAP_GET_RESPONSE
    if (soap_body_end_in(soap) != 0 ||
        soap_envelope_end_in(soap) != 0)
    {
        return TE_GSOAP_ERROR;
    }
    return rc;
}

/* See description in acse_internal.h */
te_errno
acse_soap_serve_response(cwmp_session_t *cwmp_sess)
{
    struct soap *soap = &(cwmp_sess->m_soap);
    acse_epc_cwmp_data_t *request;
    te_errno rc = 0;

    assert(cwmp_sess->state == CWMP_WAIT_RESPONSE);
    assert(cwmp_sess->rpc_item != NULL);

    VERB("%s: processed rpc_item: %p", __FUNCTION__, cwmp_sess->rpc_item);
    request = cwmp_sess->rpc_item->params;

    cwmp_sess->last_sent.tv_sec = 0;

    /* This function works in state WAIT_RESPONSE, when CWMP session
       is already associated with particular CPE. */

    if (soap_begin_recv(soap) != 0)
    {
        /* TODO: If connection is lost, wait for response
           in the next connection */
        /* TODO: if connection is broken after part of HTTP received,
           close the session (?) */
        WARN("%s: soap_begin_recv returns %d", __FUNCTION__, soap->error);
        rc = TE_EFAIL;
    }
    else
    {
        if ((rc = acse_soap_get_response(soap, request)) != 0)
        {
            if (rc == TE_GSOAP_ERROR)
            {
                const char **descr = soap_faultstring(soap);

                soap_set_fault(soap);
                ERROR("%s: RPC %s: GSOAP error %d: %s",
                      __FUNCTION__, cwmp_rpc_cpe_string(request->rpc_cpe),
                      soap->error, (descr != NULL && *descr != NULL) ?
                      *descr : "[no description]"
                     );
            }
            request->rpc_cpe = CWMP_RPC_NONE;
            request->from_cpe.p = NULL;
        }
        cwmp_sess->rpc_item = NULL; /* It is already processed. */
        soap_end_recv(soap);
    }

    if (soap->error == SOAP_EOF)
        return TE_ENOTCONN;

    if (rc != 0)
    {
        /* Terminate CWMP session unexpectedly */
        soap->keep_alive = 0;
        acse_cwmp_send_http(soap, cwmp_sess, 400, NULL);
        return TE_ENOTCONN;
    }

    /* Continue CWMP session */
    VERB("End of serve response: received %s, next rpc_item in queue: %p\n",
         cwmp_rpc_cpe_string(request->rpc_cpe),
         TAILQ_FIRST(&(cwmp_sess->cpe_owner->rpc_queue)));

    acse_cwmp_send_rpc(soap, cwmp_sess);
    return 0;
}

/* See description in acse_internal.h */
void *
acse_cwmp_malloc(struct soap *soap, size_t n)
{
    cwmp_session_t *session;
    mheap_t heap;

    if (NULL == soap)
        return NULL;

    session = (cwmp_session_t *)soap->user;
    if (NULL == session)
        return SOAP_MALLOC(soap, n);

    if (NULL == session->rpc_item)
        heap = session->def_heap;
    else
        heap = session->rpc_item->heap;

    return mheap_alloc(heap, n);
}
