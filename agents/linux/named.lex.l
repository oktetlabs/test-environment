%{
#include <stdio.h>
#include <stdlib.h>
#include "te_errno.h"
#include "te_defs.h"
#define TE_LGR_USER      "Daemons Lexer"
#include "logger_ta.h"
#include "named.conf.h"


void lex_switch_buffer(const char *name)
{
    FILE *infile = fopen(name, "r");
    YY_BUFFER_STATE state;
    
    if(infile)
    {
        state = yy_create_buffer(infile, 1024);
        yypush_buffer_state(state);
    }
}

%}
%x COMMENT
%%
include { return DNS_INCLUDE_STATEMENT; }
options { return DNS_OPTIONS_STATEMENT; }
forwarders { return DNS_FORWARDERS_STATEMENT; }
recursion { return DNS_RECURSION_STATEMENT; }
directory { return DNS_DIRECTORY_STATEMENT; }
[[:digit:]]+(\.[[:digit:]])+ { yylval.str = strdup(yytext); return DNS_DOTTED_STRING; }
[[:digit:]]+(:[[:digit:]])+ { yylval.str = strdup(yytext); return DNS_COLON_STRING; }
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
    yypop_buffer_state();
    if (!YY_CURRENT_BUFFER)
        yyterminate();
    BEGIN(INITIAL);
}
   

    
