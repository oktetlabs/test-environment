/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2005-2022 OKTET Labs Ltd. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>

#if 0
#include<ucd-snmp/asn1.h>
#include<ucd-snmp/snmp.h>
#include<ucd-snmp/snmp_impl.h>
#include<ucd-snmp/snmp_api.h>
#include<ucd-snmp/snmp_client.h>
#else
#define UCD_COMPATIBLE
#include<net-snmp/net-snmp-config.h>
#include<net-snmp/session_api.h>
#include<net-snmp/pdu_api.h>

#ifndef RECEIVED_MESSAGE
#define RECEIVED_MESSAGE NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE
#endif

#ifndef TIMED_OUT
#define TIMED_OUT NETSNMP_CALLBACK_OP_TIMED_OUT
#endif

#endif



void
print_oid(unsigned long * subids, int len)
{
    int i;

    if (subids == NULL)
        printf(".NULL. :-)");

    for (i = 0; i < len; i++)
        printf(".%d", subids[i]);
}

static int was_input = 0;

int
snmp_input(
    int op,
    struct snmp_session *session,
    int reqid,
    struct snmp_pdu *pdu,
    void *magic)
{
    printf("Variable retrieved successfully!!!\n");
    printf("Callback 'snmp_input' called!!!!\n");
    was_input = 1;

    if (op == RECEIVED_MESSAGE)
    {
        struct variable_list *vars, *v;
        for (vars = pdu->variables; vars; vars = vars->next_variable)
        {
            printf ("\nvar :");
            print_oid( vars->name, vars->name_length);
            printf ("\ntype: %d, val: ", vars->type);
            switch(vars->type)
            {
                case ASN_INTEGER:
                    printf("%d\n", *(vars->val.integer));
                    break;
                case ASN_OCTET_STR:
                    printf("%s\n", vars->val.string);
                    break;
                default:
                    printf("not impl.\n");
            }
        }
    }

    if ( op == TIMED_OUT )
    {
        printf ("==========timeout is received in 'snmp_server_fifo_input'!\n" ) ;
    }
    return 1;
}

int main(void)
{
    char community[] = "public";

    unsigned long syst_oid [] = {1,3,6,1,2,1};
    fd_set fdst;
    int n_fds = 0;
    int block = 1;
    struct timeval timeout;
    int rc;
    struct snmp_session session, *ss;
    struct snmp_pdu *pdu, *response;
    void * sessp;


    memset (&session, 0, sizeof(session));

    session.version = 1;
    session.retries = 1;
    session.timeout = 10000000;

    session.peername = strdup("127.0.0.1");
    session.remote_port = 0;
    session.local_port = 5112;
    session.community = strdup(community);
    session.community_len = strlen(community);
    session.callback = snmp_input;
    session.callback_magic = NULL;

    ss = snmp_open(&session);
    if (ss == NULL)
    {
        snmp_perror("open session:");
        return 2;
    }

    FD_ZERO(&fdst);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    snmp_select_info(&n_fds, &fdst, &timeout, &block);
    printf ("from select info: n_fds %d, sec %d, used %d, block %d\n",
            n_fds, timeout.tv_sec, timeout.tv_usec, block);

    was_input = 0;
    rc = select (n_fds, &fdst, 0, 0, &timeout);
    printf ("select returns %d\n", rc);

    if (rc > 0){
        snmp_read(&fdst);
        if (was_input)
            printf("callback allready was called\n");
        else
            printf("callback was NOT called\n");
    }
    else
       printf("snmp_read was NOT called\n");

    if (rc < 0)
    {
        perror("select error:");
        return 1;
    }
    snmp_close(ss);
}
