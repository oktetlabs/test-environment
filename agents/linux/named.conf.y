%{
    #include <stdio.h>
    #include <stdlib.h>
    extern void dns_parse_set_forwarder(const char *fwd);
    extern void dns_parse_set_directory(const char *dir);
    extern void dns_parse_set_recursion(int r);
    extern void lex_switch_buffer(const char *name);
    extern int yylex(void);
    extern void yyerror(const char *msg);
%}

%token DNS_INCLUDE_STATEMENT
%token DNS_OPTIONS_STATEMENT
%token DNS_FORWARDERS_STATEMENT
%token DNS_RECURSION_STATEMENT
%token DNS_DIRECTORY_STATEMENT
%token <str> DNS_DOTTED_STRING
%token <str> DNS_COLON_STRING
%token <ival> DNS_NUMBER
%token <str> DNS_QUOTED_STRING
%token DNS_UNKNOWN_TOKEN

%type <str> ip_address

%union
{
    char *str;
    int ival;
}

%%
config: /* empty */
      | config section
      ;

section: DNS_INCLUDE_STATEMENT DNS_QUOTED_STRING ';'
        {
            lex_switch_buffer($2);
        }
        |  DNS_OPTIONS_STATEMENT '{' options '}' ';'
        | DNS_UNKNOWN_TOKEN anything2 ';' {}
        ;

options: /* empty */ {}
       | options option {}
    ;

option: DNS_DIRECTORY_STATEMENT DNS_QUOTED_STRING ';'
         { dns_parse_set_directory($2); free($2); }
         | DNS_RECURSION_STATEMENT DNS_NUMBER ';' 
         { dns_parse_set_recursion($2); }
         | DNS_FORWARDERS_STATEMENT '{' forwarders '}' ';' 
         | DNS_UNKNOWN_TOKEN anything2 ';' {}
        ;

forwarders: /* empty */
          | forwarder anything
        ;

forwarder: ip_address ';'
          { dns_parse_set_forwarder($1); free($1); }
          | ip_address anything2 ';'
          { dns_parse_set_forwarder($1); free($1); }  

ip_address: DNS_DOTTED_STRING
            | DNS_COLON_STRING
        ;

anything: /* empty */ {}
         | anything anything2 ';' {}
        ;

anything2: single_anything {}
         | anything2 single_anything {}
        ;
single_anything: DNS_UNKNOWN_TOKEN {}
| '{' anything '}' {}
| DNS_NUMBER {}
| DNS_QUOTED_STRING { free($1); }
| DNS_DOTTED_STRING { free($1); }
| DNS_COLON_STRING  { free($1); } 
        ;

%%
