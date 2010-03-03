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

#include "acse_internal.h"
#include "acse_soapStub.h"

#include "te_defs.h"
#include "te_errno.h"

#include "logger_api.h"
#include "logger_file.h"

DEFINE_LGR_ENTITY("ACSE");


#if 0
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
#endif

#include "httpda.h"

#if 1
int 
main(int argc, char **argv)
{
    FILE   *logfile;
    char    *msg_sock_name;
    int      port;
    acs_t   *acs;
    cpe_t   *cpe;

    te_errno rc;

    db_add_acs("ACS");
    db_add_cpe("ACS", "cpe-dummy");

    acs = db_find_acs("ACS");
    cpe = db_find_cpe(acs, "ACS", "cpe-dummy");

    acs->port = (argc > 1 ? atoi(argv[1]) : 8080);

    if (argc > 2)
    {
        logfile = fopen(argv[2], "a");
        if (logfile == NULL)
        {
            perror("open logfile failed");
            return 2;
        }
        te_log_message_file_out = logfile;
    }
    if (argc > 3)
        msg_sock_name = strdup(argv[3]);
    else
        msg_sock_name = strdup(EPC_ACSE_SOCK);

    cpe->acs_auth.login = 
        strdup("000261-Home Gateway-V60200000000-0010501606");
    cpe->acs_auth.passwd = strdup("passwd");


    db_add_cpe("ACS", "CPE-box");
    cpe = db_find_cpe(acs, "ACS", "CPE-box");

    cpe->acs_auth.login =
        strdup("000261-Home Gateway-V601L622R1A0-1001742119");
    cpe->acs_auth.passwd = strdup("z7cD7CTDA1DrQKUb");

    cpe->cr_auth.login  = strdup(cpe->acs_auth.login);
    cpe->cr_auth.passwd = strdup(cpe->acs_auth.passwd);
            


    acse_enable_acs(acs);

    if ((rc = acse_epc_disp_init(msg_sock_name, EPC_MMAP_AREA)) != 0)
    {
        ERROR("Fail create EPC dispatcher %r", rc);
        return 1;
    }

    acse_loop();

    return 0;
}
#else
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

#if 1
    if (argc == 1)
    {
        cwmp_SendConnectionRequest(
            "http://10.20.1.2:8082/5c2dfeabaf9650fe",
            "000261-Home Gateway-V601L622R1A0-1001742119", 
            "z7cD7CTDA1DrQKUb");

        return 1;
    }
#endif
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

    soap_register_plugin(&my_soap, http_da); 

    my_soap.max_keep_alive = 2;

    printf("before serve\n");
    if (soap_serve(&my_soap) != SOAP_OK)
    {
        printf("was serve error, try again..\n");
        //my_soap.socket = accept(l_sock, &conn_sa, &conn_sa_size);
        //soap_imode(&my_soap, SOAP_IO_KEEPALIVE);
        //soap_omode(&my_soap, SOAP_IO_KEEPALIVE);
        soap_serve(&my_soap);
    }
    printf("after serve\n");

    soap_done(&my_soap);
    soap_end(&my_soap);

    return 0;
}

#endif
