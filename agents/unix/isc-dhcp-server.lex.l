%{
#include "te_config.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "dhcp_server.h"
#include "isc-dhcp-server.conf.h"

#if 0
#define YY_INPUT(buf, result, max_size) \
    do { \
        int c = mybuf[myindex++]; \
                                    \
        result = (c == '\0') ? YY_NULL : (buf[0] = c, 1); \
    } while (0)
#endif


%}

%option nounput

%%
\#.*\n                  {}
[[:space:]]+            {}
"{"                     return OBRACE;
"}"                     return EBRACE;
","                     return COMMA;
";"                     return SEMICOLON;

log-facility            return ISC_DHCP_LOG_FACILITY;
omapi-port              return ISC_DHCP_OMAPI_PORT;
default-lease-time      return ISC_DHCP_DEF_LEASE_TIME;
max-lease-time          return ISC_DHCP_MAX_LEASE_TIME;

ddns-update-style       return ISC_DHCP_DDNS_UPDATE_STYLE;
none|ad-hoc|interim     {
                            yylval.ival =
                                (strcmp(yytext, "none") == 0) ? 
                                    TE_DHCPS_DDNS_UPDATE_NONE :
                                (strcmp(yytext, "ad-hoc") == 0) ?
                                    TE_DHCPS_DDNS_UPDATE_AD_HOC :
                                    TE_DHCPS_DDNS_UPDATE_INTERIM;
                            return ISC_DHCP_DDNS_UPDATE_STYLE_VALUE;
                        }

shared-network          return ISC_DHCP_SHARED_NETWORK;
subnet                  return ISC_DHCP_SUBNET;
netmask                 return ISC_DHCP_NETMASK;
option                  return ISC_DHCP_OPTION;
range                   return ISC_DHCP_RANGE;
group                   return ISC_DHCP_GROUP;
host                    return ISC_DHCP_HOST;
filename                return ISC_DHCP_FILENAME;
next-server             return ISC_DHCP_NEXT_SERVER;
fixed-address           return ISC_DHCP_FIXED_ADDRESS;
hardware                return ISC_DHCP_HARDWARE;
ethernet                return ISC_DHCP_ETHERNET;

unknown-clients         return ISC_DHCP_UNKNOWN_CLIENTS;
bootp                   return ISC_DHCP_BOOTP;

allow|deny|ignore       {
                            yylval.ival =
                                (strcmp(yytext, "allow") == 0) ?
                                    TE_DHCPS_ALLOW :
                                (strcmp(yytext, "deny") == 0) ?
                                    TE_DHCPS_DENY : TE_DHCPS_IGNORE;
                            return ISC_DHCP_ADI;
                        }

\"[^"]*\"               {
                            yytext[strlen(yytext) - 1] = '\0';
                            yylval.str = strdup(yytext + 1);
                            return ISC_DHCP_QUOTED_STRING;
                        }

[[:digit:]]+(\.[[:digit:]]+){3}   return ISC_DHCP_IP4_ADDRESS;

[[:xdigit:]]{1,2}(:[[:xdigit:]]{1,2}){5}    return ISC_DHCP_ETH_ADDRESS;

[[:digit:]]+            {
                            yylval.ival = strtol(yytext, NULL, 10);
                            return ISC_DHCP_INT;
                        }

[-[:alnum:]]+(\.[-[:alnum:]]+)+ {
                            yylval.str = strdup(yytext);
                            return ISC_DHCP_DNS_DOTTED_NAME;
                        }

[[:alpha:]]+[-[:alnum:]]*   {
                            yylval.str = strdup(yytext);
                            return ISC_DHCP_NAME;
                        }


%%
