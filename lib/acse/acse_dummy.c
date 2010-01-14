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

#include "logger_api.h"

DEFINE_LGR_ENTITY("ACSE_DUMMY");

#include "acse_soapStub.h"
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

    if (listen(l_sock, 5) < 0)
    {
        perror("listen failed:");
        return 2;
    }

    soap_init(&my_soap);
    if ((my_soap.socket = accept(l_sock, &conn_sa, &conn_sa_size)) < 0)
    {
        perror("accept failed:");
        return 2;
    }
    soap_imode(&my_soap, SOAP_IO_KEEPALIVE);
    soap_omode(&my_soap, SOAP_IO_KEEPALIVE);
    soap_set_namespaces(&my_soap, namespaces);

    my_soap.max_keep_alive = 2;

    printf("before serve\n");
    soap_serve(&my_soap);

    soap_done(&my_soap);
    soap_end(&my_soap);

    return 0;
}

