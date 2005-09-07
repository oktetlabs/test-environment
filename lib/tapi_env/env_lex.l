%{
#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <string.h>
#endif

#include "tapi_env.h"
#include "env_gram.h"


#define YY_INPUT(buf, result, max_size) \
    do {                                                    \
        int c = mybuf[myindex++];                           \
                                                            \
        result = (c == '\0') ? YY_NULL : (buf[0] = c, 1);   \
    } while (0)

extern char *mybuf;
extern int myindex;

%}

%option nounput

%%
[[:space:]]+        {}
IUT|tester          {
                        yylval.number = (strcmp(yytext, "IUT") == 0) ?
                            TAPI_ENV_IUT : TAPI_ENV_TESTER;
                        return ENTITY_TYPE;
                    }
loopback|unicast|broadcast|wildcard|alien|multicast|fake|ip4mapped_uc {
    yylval.number = ((strcmp(yytext, "loopback") == 0) ?
                        TAPI_ENV_ADDR_LOOPBACK :
                    ((strcmp(yytext, "unicast") == 0) ?
                        TAPI_ENV_ADDR_UNICAST :
                    ((strcmp(yytext, "broadcast") == 0) ?
                        TAPI_ENV_ADDR_BROADCAST :
                    ((strcmp(yytext, "wildcard") == 0) ?
                        TAPI_ENV_ADDR_WILDCARD :
                    ((strcmp(yytext, "alien") == 0) ?
                        TAPI_ENV_ADDR_ALIEN :
                    ((strcmp(yytext, "multicast") == 0) ?
                        TAPI_ENV_ADDR_MULTICAST :
                    ((strcmp(yytext, "ip4mapped_uc") == 0) ?
                        TAPI_ENV_ADDR_IP4MAPPED_UC :
                        TAPI_ENV_ADDR_FAKE_UNICAST)))))));
    return ADDR_TYPE;
}
ether|inet6|inet    {
                        yylval.number =
                            ((strcmp(yytext, "inet") == 0) ? RPC_AF_INET :
                            ((strcmp(yytext, "inet6") == 0) ? RPC_AF_INET6 :
                            RPC_AF_ETHER));
                        return ADDR_FAMILY;
                    }
addr                { return ADDRESS; }
if                  { return INTERFACE; }
\{                  { return OBRACE; }
\}                  { return EBRACE; }
\'                  { return QUOTE; }
:                   { return COLON; }
,                   { return COMMA; }
=                   { return EQUAL; }
[[:alpha:]][_[:alnum:]]*    {
                                yylval.string = strdup(yytext);
                                return WORD;
                            }
.+                  { REJECT; }
%%
