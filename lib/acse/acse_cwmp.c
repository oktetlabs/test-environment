/** @file
 * @brief ACSE CWMP Dispatcher
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

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "acse_internal.h"

#include "acse_soapH.h"
#include "cwmp.nsmap"

/** CWMP Dispatcher state machine states */
typedef enum { want_read, want_write } cwmp_t;

/** CWMP Dispatcher state machine private data */
typedef struct {
    cwmp_t state; /**< Session Requester state machine current state */
} cwmp_data_t;

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

static te_errno
recover_fds(void *data)
{
    UNUSED(data);
    return -1;
}

extern te_errno
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
    UNUSED(cwmp__Inform);

    printf("%s called. Header is %p, enc style is '%s'\n",
            __FUNCTION__, soap->header, soap->encodingStyle);
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
