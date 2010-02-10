/* acse_soapServer.c
   Generated by gSOAP 2.7.9l from cwmp_acs.h
   Copyright(C) 2000-2007, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/
#include "acse_soapH.h"
#ifdef __cplusplus
extern "C" {
#endif

SOAP_SOURCE_STAMP("@(#) acse_soapServer.c ver 2.7.9l 2010-01-14 09:03:40 GMT")


SOAP_FMAC5 int SOAP_FMAC6 soap_serve(struct soap *soap)
{
#ifndef WITH_FASTCGI
	unsigned int k = soap->max_keep_alive;
#endif

	do
	{
#ifdef WITH_FASTCGI
		if (FCGI_Accept() < 0)
		{
			soap->error = SOAP_EOF;
			return soap_send_fault(soap);
		}
#endif

		soap_begin(soap);

#ifndef WITH_FASTCGI
		if (soap->max_keep_alive > 0 && !--k)
			soap->keep_alive = 0;
#endif

		if (soap_begin_recv(soap))
		{
                        if ((soap->error != SOAP_EOF) && 
                            (soap->error < SOAP_STOP))
			{
#ifdef WITH_FASTCGI
				soap_send_fault(soap);
#else 
				return soap_send_fault(soap);
#endif
			}
			soap_closesock(soap);

                    if (soap->error == SOAP_EOF)
                        printf("%s(): EOF detected \n", __FUNCTION__);

			continue;
		}

                if (soap->length == 0)
                {
                    /* TODO: Here should be processing of empty POST!!! */
                    printf("%s(): Content Length ZERO!\n", __FUNCTION__);
                }

		if (soap_envelope_begin_in(soap)
		 || soap_recv_header(soap)
		 || soap_body_begin_in(soap)
		 || soap_serve_request(soap)
		 || (soap->fserveloop && soap->fserveloop(soap)))
		{
#ifdef WITH_FASTCGI
			soap_send_fault(soap);
#else
			return soap_send_fault(soap);
#endif
		}

                printf("%s():%d; end loop body\n", __FUNCTION__, __LINE__);
#ifdef WITH_FASTCGI
		soap_destroy(soap);
		soap_end(soap);
	} while (1);
#else
	} while (soap->keep_alive);
#endif
	return SOAP_OK;
}

#ifndef WITH_NOSERVEREQUEST
SOAP_FMAC5 int SOAP_FMAC6 soap_serve_request(struct soap *soap)
{
	soap_peek_element(soap);
	if (!soap_match_tag(soap, soap->tag, "cwmp:GetRPCMethods"))
		return soap_serve___cwmp__GetRPCMethods(soap);
	if (!soap_match_tag(soap, soap->tag, "cwmp:Inform"))
		return soap_serve___cwmp__Inform(soap);
	if (!soap_match_tag(soap, soap->tag, "cwmp:TransferComplete"))
		return soap_serve___cwmp__TransferComplete(soap);
	if (!soap_match_tag(soap, soap->tag, "cwmp:AutonomousTransferComplete"))
		return soap_serve___cwmp__AutonomousTransferComplete(soap);
	if (!soap_match_tag(soap, soap->tag, "cwmp:RequestDownload"))
		return soap_serve___cwmp__RequestDownload(soap);
	if (!soap_match_tag(soap, soap->tag, "cwmp:Kicked"))
		return soap_serve___cwmp__Kicked(soap);
	return soap->error = SOAP_NO_METHOD;
}
#endif

SOAP_FMAC5 int SOAP_FMAC6 soap_serve___cwmp__GetRPCMethods(struct soap *soap)
{	struct __cwmp__GetRPCMethods soap_tmp___cwmp__GetRPCMethods;
	struct _cwmp__GetRPCMethodsResponse cwmp__GetRPCMethodsResponse;
	soap_default__cwmp__GetRPCMethodsResponse(soap, &cwmp__GetRPCMethodsResponse);
	soap_default___cwmp__GetRPCMethods(soap, &soap_tmp___cwmp__GetRPCMethods);
	if (!soap_get___cwmp__GetRPCMethods(soap, &soap_tmp___cwmp__GetRPCMethods, "-cwmp:GetRPCMethods", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = __cwmp__GetRPCMethods(soap, soap_tmp___cwmp__GetRPCMethods.cwmp__GetRPCMethods, &cwmp__GetRPCMethodsResponse);
	if (soap->error)
		return soap->error;
	soap_serializeheader(soap);
	soap_serialize__cwmp__GetRPCMethodsResponse(soap, &cwmp__GetRPCMethodsResponse);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put__cwmp__GetRPCMethodsResponse(soap, &cwmp__GetRPCMethodsResponse, "cwmp:GetRPCMethodsResponse", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put__cwmp__GetRPCMethodsResponse(soap, &cwmp__GetRPCMethodsResponse, "cwmp:GetRPCMethodsResponse", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve___cwmp__Inform(struct soap *soap)
{	struct __cwmp__Inform soap_tmp___cwmp__Inform;
	struct _cwmp__InformResponse cwmp__InformResponse;
	soap_default__cwmp__InformResponse(soap, &cwmp__InformResponse);
	soap_default___cwmp__Inform(soap, &soap_tmp___cwmp__Inform);
	if (!soap_get___cwmp__Inform(soap, &soap_tmp___cwmp__Inform, "-cwmp:Inform", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = __cwmp__Inform(soap, soap_tmp___cwmp__Inform.cwmp__Inform, &cwmp__InformResponse);
	if (soap->error)
		return soap->error;
	soap_serializeheader(soap);
	soap_serialize__cwmp__InformResponse(soap, &cwmp__InformResponse);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put__cwmp__InformResponse(soap, &cwmp__InformResponse, "cwmp:InformResponse", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put__cwmp__InformResponse(soap, &cwmp__InformResponse, "cwmp:InformResponse", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve___cwmp__TransferComplete(struct soap *soap)
{	struct __cwmp__TransferComplete soap_tmp___cwmp__TransferComplete;
	struct _cwmp__TransferCompleteResponse cwmp__TransferCompleteResponse;
	soap_default__cwmp__TransferCompleteResponse(soap, &cwmp__TransferCompleteResponse);
	soap_default___cwmp__TransferComplete(soap, &soap_tmp___cwmp__TransferComplete);
	if (!soap_get___cwmp__TransferComplete(soap, &soap_tmp___cwmp__TransferComplete, "-cwmp:TransferComplete", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = __cwmp__TransferComplete(soap, soap_tmp___cwmp__TransferComplete.cwmp__TransferComplete, &cwmp__TransferCompleteResponse);
	if (soap->error)
		return soap->error;
	soap_serializeheader(soap);
	soap_serialize__cwmp__TransferCompleteResponse(soap, &cwmp__TransferCompleteResponse);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put__cwmp__TransferCompleteResponse(soap, &cwmp__TransferCompleteResponse, "cwmp:TransferCompleteResponse", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put__cwmp__TransferCompleteResponse(soap, &cwmp__TransferCompleteResponse, "cwmp:TransferCompleteResponse", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve___cwmp__AutonomousTransferComplete(struct soap *soap)
{	struct __cwmp__AutonomousTransferComplete soap_tmp___cwmp__AutonomousTransferComplete;
	struct _cwmp__AutonomousTransferCompleteResponse cwmp__AutonomousTransferCompleteResponse;
	soap_default__cwmp__AutonomousTransferCompleteResponse(soap, &cwmp__AutonomousTransferCompleteResponse);
	soap_default___cwmp__AutonomousTransferComplete(soap, &soap_tmp___cwmp__AutonomousTransferComplete);
	if (!soap_get___cwmp__AutonomousTransferComplete(soap, &soap_tmp___cwmp__AutonomousTransferComplete, "-cwmp:AutonomousTransferComplete", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = __cwmp__AutonomousTransferComplete(soap, soap_tmp___cwmp__AutonomousTransferComplete.cwmp__AutonomousTransferComplete, &cwmp__AutonomousTransferCompleteResponse);
	if (soap->error)
		return soap->error;
	soap_serializeheader(soap);
	soap_serialize__cwmp__AutonomousTransferCompleteResponse(soap, &cwmp__AutonomousTransferCompleteResponse);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put__cwmp__AutonomousTransferCompleteResponse(soap, &cwmp__AutonomousTransferCompleteResponse, "cwmp:AutonomousTransferCompleteResponse", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put__cwmp__AutonomousTransferCompleteResponse(soap, &cwmp__AutonomousTransferCompleteResponse, "cwmp:AutonomousTransferCompleteResponse", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve___cwmp__RequestDownload(struct soap *soap)
{	struct __cwmp__RequestDownload soap_tmp___cwmp__RequestDownload;
	struct _cwmp__RequestDownloadResponse cwmp__RequestDownloadResponse;
	soap_default__cwmp__RequestDownloadResponse(soap, &cwmp__RequestDownloadResponse);
	soap_default___cwmp__RequestDownload(soap, &soap_tmp___cwmp__RequestDownload);
	if (!soap_get___cwmp__RequestDownload(soap, &soap_tmp___cwmp__RequestDownload, "-cwmp:RequestDownload", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = __cwmp__RequestDownload(soap, soap_tmp___cwmp__RequestDownload.cwmp__RequestDownload, &cwmp__RequestDownloadResponse);
	if (soap->error)
		return soap->error;
	soap_serializeheader(soap);
	soap_serialize__cwmp__RequestDownloadResponse(soap, &cwmp__RequestDownloadResponse);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put__cwmp__RequestDownloadResponse(soap, &cwmp__RequestDownloadResponse, "cwmp:RequestDownloadResponse", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put__cwmp__RequestDownloadResponse(soap, &cwmp__RequestDownloadResponse, "cwmp:RequestDownloadResponse", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve___cwmp__Kicked(struct soap *soap)
{	struct __cwmp__Kicked soap_tmp___cwmp__Kicked;
	struct _cwmp__KickedResponse cwmp__KickedResponse;
	soap_default__cwmp__KickedResponse(soap, &cwmp__KickedResponse);
	soap_default___cwmp__Kicked(soap, &soap_tmp___cwmp__Kicked);
	if (!soap_get___cwmp__Kicked(soap, &soap_tmp___cwmp__Kicked, "-cwmp:Kicked", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = __cwmp__Kicked(soap, soap_tmp___cwmp__Kicked.cwmp__Kicked, &cwmp__KickedResponse);
	if (soap->error)
		return soap->error;
	soap_serializeheader(soap);
	soap_serialize__cwmp__KickedResponse(soap, &cwmp__KickedResponse);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put__cwmp__KickedResponse(soap, &cwmp__KickedResponse, "cwmp:KickedResponse", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put__cwmp__KickedResponse(soap, &cwmp__KickedResponse, "cwmp:KickedResponse", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

#ifdef __cplusplus
}
#endif

/* End of acse_soapServer.c */
