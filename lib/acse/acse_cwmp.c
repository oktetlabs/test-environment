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

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "acse_internal.h"

#include "httpda.h"

#include "acse_soapH.h"

/** Single REALM for Digest Auth. which we support. */
const char *authrealm = "tr-069";

static te_errno
before_select(void *data, fd_set *rd_set, fd_set *wr_set, int *fd_max)
{
    UNUSED(data);
    UNUSED(rd_set);
    UNUSED(wr_set);
    UNUSED(fd_max);

    return 0;
}

static te_errno
after_select(void *data, fd_set *rd_set, fd_set *wr_set)
{
    UNUSED(data);
    UNUSED(rd_set);
    UNUSED(wr_set);

    return 0;
}

static te_errno
destroy(void *data)
{
    cwmp_data_t *cwmp = data;

    free(cwmp);
    return 0;
}

te_errno
acse_cwmp_create(channel_t *channel)
{
    cwmp_data_t *cwmp = channel->data = malloc(sizeof *cwmp);

    if (cwmp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    cwmp->state            = want_read;

    channel->before_select = &before_select;
    channel->after_select  = &after_select;
    channel->destroy       = &destroy;
    channel->recover_fds   = &recover_fds;

    return 0;
}


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

SOAP_FMAC5 int SOAP_FMAC6 
__cwmp__Inform(struct soap *soap,
               struct _cwmp__Inform *cwmp__Inform,
               struct _cwmp__InformResponse *cwmp__InformResponse)
{
    cpe_t *cpe_record = (cpe_t *)soap->user;

    if(cpe_record == NULL)
    {
        /* TODO: correct processing */
        return 500; 
    }
printf("%s called. Header is %p, enc style is '%s', inform Dev is '%s'\n",
            __FUNCTION__, soap->header, soap->encodingStyle,
            cwmp__Inform->DeviceId->OUI);

    soap->keep_alive = 1; 

    if (soap->authrealm && soap->userid)
    {
        printf("%s(): Digest auth, authrealm: '%s', userid '%s'\n", 
                __FUNCTION__, soap->authrealm, soap->userid);
        /* TODO: lookup passwd by userid */
        if (!strcmp(soap->authrealm, authrealm) &&
            !strcmp(soap->userid, cpe_record->username))
        {

#if 1
          char *passwd = "z7cD7CTDA1DrQKUb";
#else
          char *passwd = "passwd";
#endif

          if (http_da_verify_post(soap, passwd))
          {
            printf("%s(): Digest auth failed RRRRRRRRRRRR\n", __FUNCTION__);
              soap_dealloc(soap, soap->authrealm);
              soap->authrealm = soap_strdup(soap, authrealm);
              soap->keep_alive = 1; 
            return 401;
          }
          printf("%s(): AUTH passed\n", __FUNCTION__);
        }
        printf("%s(): Should be fault\n", __FUNCTION__);
    }
    else
    {
printf("%s(): Digest auth failed 2\n", __FUNCTION__);
      soap->authrealm = soap_strdup(soap, authrealm);
      soap->keep_alive = 1; 
#if 1
        soap->error = SOAP_OK;
        soap_serializeheader(soap);
        soap_begin_count(soap);
        soap_end_count(soap);
        soap_response(soap, 401);
        // soap_envelope_end_out(soap);
        soap_end_send(soap);
      soap->keep_alive = 1; 
      return SOAP_STOP;
#else
      return 401;
#endif
    }


    if (soap->header)
        printf("hold_request in Header: %d\n",
                (int)soap->header->cwmp__HoldRequests.__item);
    else
    {
        soap->header = soap_malloc(soap, sizeof(struct SOAP_ENV__Header));
    }

    if (soap->encodingStyle)
    {
        soap->encodingStyle = NULL;
    }

    

    cwmp__InformResponse->MaxEnvelopes = 255;
    soap->header->cwmp__HoldRequests.__item = 1;
    soap->header->cwmp__HoldRequests.SOAP_ENV__mustUnderstand = "1";
    soap->keep_alive = 1; 

    return SOAP_OK;
}


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


/**
 * Check wheather accepted TCP connection is related to 
 * particular ACS.
 *
 * @return      0 if connection accepted by this ACS;
 *              TE_ECONNREFUSED if connection NOT accepted by this ACS;
 *              other error status on some error.
 */
te_errno
cwmp_check_cpe_connection(acs_t *acs, int socket)
{
    /* TODO: real check, now accept all, if any CPE registered. */

    return 0;
}

