/** @file
 * @brief ACSE CWMP experiments
 *
 * ACS Emulator dummy tool
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include<stdlib.h>
#include<stdio.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>

#include "te_defs.h"
#include "te_errno.h"

#include "soapStub.h"
#if 0
#include "cwmp.nsmap"

#else

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
    {"cwmp", "urn:dslforum-org:cwmp-1-1",
                "urn:dslforum-org:cwmp-1-*", NULL},
    {NULL, NULL, NULL, NULL}
};
#endif



int 
main(int argc, char **argv)
{
    struct soap my_soap;

    int l_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in l_sa, conn_sa;
    size_t conn_sa_size = sizeof(conn_sa);

    l_sa.sin_family = AF_INET;
    l_sa.sin_addr.s_addr = INADDR_ANY;
    l_sa.sin_port = htons(argc > 1 ? atoi(argv[1]) : 2000);

    if (bind(l_sock, &l_sa, sizeof(l_sa)) < 0)
    {
        perror("bind failed:");
        return 1;
    }
    printf("bound.\n");

    if (listen(l_sock, 5) < 0)
    {
        perror("listen failed:");
        return 2;
    }
    printf("listen\n");

    soap_init(&my_soap);
    if ((my_soap.socket = accept(l_sock, &conn_sa, &conn_sa_size)) < 0)
    {
        perror("accept failed:");
        return 2;
    }
    printf("accepted %d\n", my_soap.socket);
    send(my_soap.socket, "UUUUUUUUUUUUUUUUUUUUU", 20, 0);

    soap_set_namespaces(&my_soap, namespaces);

    printf("before serve\n");
    soap_serve(&my_soap);

    soap_done(&my_soap);
    soap_end(&my_soap);

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

    return SOAP_OK;
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
