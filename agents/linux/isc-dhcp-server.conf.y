%{
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <errno.h>

#define TE_LOG_LEVEL    0xff
#define TE_LGR_USER     "ISC DHCP Server Cfg Parser"
    
#include "te_errno.h"
#include "te_stdint.h"
#include "logger_api.h"


#define YYDEBUG 1
int yydebug = 1;

#define LOG_PFX     "%d:%d-%d:%d: "

extern FILE *yyin;
extern int yylex(void);
extern int yyparse(void);

void
yyerror(const char *str)
{
    ERROR("%s", str);
}

/**
 * Parse ISC DHCP server configuration file.
 *
 * @param filename      Name of the configuration file
 *
 * @return Status code.
 */
int
isc_dhcp_server_cfg_parse(const char *filename)
{
    int rc = 0;

    yyin = fopen(filename, "r");
    if (yyin == NULL)
    {
        rc = errno;
        ERROR("Failed to open file '%s' for reading: %X",
              filename, rc);
        return rc;
    }

    if (yyparse() != 0)
    {
        ERROR("Failed to parse '%s'", filename);
        rc = EINVAL;
    }

    if (fclose(yyin) != 0)
    {
        RC_UPDATE(rc, errno);
        ERROR("%s(): fclose() failed: %X", __FUNCTION__, rc);
    }

    return rc;
}

%}

%token OBRACE EBRACE COMMA SEMICOLON;

%token ISC_DHCP_DDNS_UPDATE_STYLE;
%token <ival>   ISC_DHCP_DDNS_UPDATE_STYLE_VALUE;

%token ISC_DHCP_LOG_FACILITY;
%token ISC_DHCP_OMAPI_PORT;
%token ISC_DHCP_DEF_LEASE_TIME;
%token ISC_DHCP_MAX_LEASE_TIME;

%token ISC_DHCP_SHARED_NETWORK;
%token ISC_DHCP_SUBNET;
%token ISC_DHCP_NETMASK;
%token ISC_DHCP_OPTION;
%token ISC_DHCP_RANGE;
%token ISC_DHCP_GROUP;
%token ISC_DHCP_HOST;
%token ISC_DHCP_FILENAME;
%token ISC_DHCP_NEXT_SERVER;
%token ISC_DHCP_FIXED_ADDRESS;
%token ISC_DHCP_HARDWARE;
%token ISC_DHCP_ETHERNET;

%token ISC_DHCP_UNKNOWN_CLIENTS;
%token ISC_DHCP_BOOTP;

%token <str>    ISC_DHCP_QUOTED_STRING;
%token <str>    ISC_DHCP_NAME;
%token <ival>   ISC_DHCP_INT;
%token <ip4>    ISC_DHCP_IP4_ADDRESS;
%token <eth>    ISC_DHCP_ETH_ADDRESS;
%token <str>    ISC_DHCP_DNS_DOTTED_NAME;
%token <ival>   ISC_DHCP_ADI;

%union
{
    char           *str;
    int             ival;
    struct in_addr  ip4;
    uint8_t         eth[6];
}

%%
config:
    /* empty */
    | config statement
    ;

statement:
    global SEMICOLON
    {
    }
    | shared_network
    {
    }
    | subnet
    {
    }
    | host
    {
    }
    | group
    {
    }
    ;

global:
    ISC_DHCP_DDNS_UPDATE_STYLE ISC_DHCP_DDNS_UPDATE_STYLE_VALUE
    {
    }
    | ISC_DHCP_LOG_FACILITY ISC_DHCP_NAME
    {
    }
    | ISC_DHCP_OMAPI_PORT ISC_DHCP_INT
    {
    }
    | ISC_DHCP_DEF_LEASE_TIME ISC_DHCP_INT
    {
    }
    | ISC_DHCP_MAX_LEASE_TIME ISC_DHCP_INT
    {
    }
    | ISC_DHCP_ADI ISC_DHCP_UNKNOWN_CLIENTS
    {
    }
    | ISC_DHCP_ADI ISC_DHCP_BOOTP
    {
    }
    | ISC_DHCP_OPTION name ISC_DHCP_IP4_ADDRESS
    {
    }
    ;

shared_network:
    ISC_DHCP_SHARED_NETWORK name OBRACE shared_network_content EBRACE
    ;

shared_network_content:
    /* empty */
    | subnet
    ;

subnet:
    ISC_DHCP_SUBNET ISC_DHCP_IP4_ADDRESS
    ISC_DHCP_NETMASK ISC_DHCP_IP4_ADDRESS
    OBRACE subnet_content EBRACE
    ;

subnet_content:
    /* empty */
    | subnet_content subnet_content_statement
    ;

subnet_content_statement:
    range SEMICOLON
    | global SEMICOLON
    | group
    | host
    ;

range:
    ISC_DHCP_RANGE ISC_DHCP_IP4_ADDRESS ISC_DHCP_IP4_ADDRESS
    {
    }
    ;

host:
    ISC_DHCP_HOST name OBRACE host_content EBRACE
    ;
    
host_content:
    /* empty */
    | host_content host_content_statement
    ;
    
host_content_statement:
    ISC_DHCP_FIXED_ADDRESS ISC_DHCP_IP4_ADDRESS SEMICOLON
    | ISC_DHCP_HARDWARE ISC_DHCP_ETHERNET ISC_DHCP_ETH_ADDRESS SEMICOLON
    | ISC_DHCP_NEXT_SERVER ip4_or_dns SEMICOLON
    | ISC_DHCP_FILENAME name SEMICOLON
    ;

group:
    ISC_DHCP_GROUP name OBRACE group_content EBRACE
    |
    ISC_DHCP_GROUP OBRACE group_content EBRACE
    ;

group_content:
    global SEMICOLON
    | host
    ;

ip4_or_dns:
    ISC_DHCP_IP4_ADDRESS
    | ISC_DHCP_DNS_DOTTED_NAME
    | ISC_DHCP_NAME
    ;

name:
    ISC_DHCP_QUOTED_STRING
    {
    }
    | ISC_DHCP_NAME
    {
    }
    ;
%%
