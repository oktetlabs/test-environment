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
 *
 * $Id: acse_cwmp.c 45424 2007-12-18 11:01:12Z edward $
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
#include "logger_api.h"
#include "acse_internal.h"

#include "soapH.h"
#include "cwmp.nsmap"


SOAP_FMAC5 int SOAP_FMAC6 __cwmp__GetRPCMethods(struct soap *soap, struct _cwmp__GetRPCMethods *cwmp__GetRPCMethods, struct _cwmp__GetRPCMethodsResponse *cwmp__GetRPCMethodsResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__GetRPCMethods);
    UNUSED(cwmp__GetRPCMethodsResponse);

    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __cwmp__Inform(struct soap *soap, struct _cwmp__Inform *cwmp__Inform, struct _cwmp__InformResponse *cwmp__InformResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__Inform);
    UNUSED(cwmp__InformResponse);

    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __cwmp__TransferComplete(struct soap *soap, struct _cwmp__TransferComplete *cwmp__TransferComplete, struct _cwmp__TransferCompleteResponse *cwmp__TransferCompleteResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__TransferComplete);
    UNUSED(cwmp__TransferCompleteResponse);

    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __cwmp__AutonomousTransferComplete(struct soap *soap, struct _cwmp__AutonomousTransferComplete *cwmp__AutonomousTransferComplete, struct _cwmp__AutonomousTransferCompleteResponse *cwmp__AutonomousTransferCompleteResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__AutonomousTransferComplete);
    UNUSED(cwmp__AutonomousTransferCompleteResponse);

    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __cwmp__RequestDownload(struct soap *soap, struct _cwmp__RequestDownload *cwmp__RequestDownload, struct _cwmp__RequestDownloadResponse *cwmp__RequestDownloadResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__RequestDownload);
    UNUSED(cwmp__RequestDownloadResponse);

    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __cwmp__Kicked(struct soap *soap, struct _cwmp__Kicked *cwmp__Kicked, struct _cwmp__KickedResponse *cwmp__KickedResponse)
{
    UNUSED(soap);
    UNUSED(cwmp__Kicked);
    UNUSED(cwmp__KickedResponse);

    return 0;
}
