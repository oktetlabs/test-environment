%{
#define TE_LGR_USER      "Named Lexer"

#include "te_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "named.conf.h"


static YY_BUFFER_STATE state_stack[16], *state_stack_top = state_stack;

void lex_switch_buffer(const char *name)
{
    FILE *infile = fopen(name, "r");
    YY_BUFFER_STATE state;
    
    if(!infile)
        ERROR("Cannot open '%s'");
    else 
    {
        if(state_stack_top == state_stack + sizeof(state_stack) / sizeof(*state_stack))
        {
            ERROR("Too many nested include's");
            fclose(infile);
        }
        else
        {
            state = yy_create_buffer(infile, 1024);
            *state_stack_top++ = YY_CURRENT_BUFFER;
            yy_switch_to_buffer(state);
        }
    }
}

%}

%option nounput

%x COMMENT
%%
include { return DNS_INCLUDE_STATEMENT; }
options { return DNS_OPTIONS_STATEMENT; }
forwarders { return DNS_FORWARDERS_STATEMENT; }
recursion { return DNS_RECURSION_STATEMENT; }
directory { return DNS_DIRECTORY_STATEMENT; }
[[:digit:]]+(\.[[:digit:]]+)+ { yylval.str = strdup(yytext); return DNS_DOTTED_STRING; }
[[:digit:]]+(:[[:digit:]]+)+ { yylval.str = strdup(yytext); return DNS_COLON_STRING; }
yes|true { yylval.ival = 1; return DNS_NUMBER; }
no|false { yylval.ival = 0; return DNS_NUMBER; }
[[:digit:]]+ { yylval.ival = strtol(yytext, NULL, 10); return DNS_NUMBER; }
[-[:alnum:]]+ { return DNS_UNKNOWN_TOKEN; }
\"[^"]*\" { yytext[strlen(yytext) - 1] = '\0';
yylval.str = strdup(yytext + 1);
return DNS_QUOTED_STRING; }
"{" { return '{'; }
"}" { return '}'; }
";" { return ';'; }
"/" { return '/'; }
"!" { return '!'; }
"#".*\n {}
"//".*\n {}
[[:space:]]+ {}
"/*" { BEGIN(COMMENT); }
<COMMENT>[^*]* {}
<COMMENT>"*"+[^*/]* {}
<COMMENT>"*"+"/" { BEGIN(INITIAL); }
. { return DNS_UNKNOWN_TOKEN; }
<INITIAL,COMMENT><<EOF>> {
    fclose(yyin);
    if(state_stack_top == state_stack)
        yyterminate();
    yy_switch_to_buffer(*--state_stack_top);
    BEGIN(INITIAL);
}
   

    
