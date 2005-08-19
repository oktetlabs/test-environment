%{
#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#define TE_LOG_USER     "Environment Cfg Parser"
#include "logger_api.h"

#include "sockts_env.h"

#define YYDEBUG 0
#define YY_SKIP_YYWRAP


extern int yylex(void);
extern int yyparse(void);

int yydebug = 0;

const char *mybuf;
int myindex;

static sockts_env          *env;
static sockts_env_net      *curr_net;
static sockts_env_host     *curr_host;
static sockts_env_process  *curr_proc;


void
yyerror(const char *str)
{
    ERROR("YACC: %s", str);
}

int
env_cfg_parse(sockts_env *e, const char *cfg)
{
    /* Populate passed environment as static variable in the file */
    env = e;
    /* Reset pointers to current nodes to NULL */
    curr_net  = NULL;
    curr_host = NULL;
    curr_proc = NULL;
    
    mybuf = cfg;
    myindex = 0;
    
    yyparse();

    return 0;
}

static sockts_env_net *
create_net(void)
{
    sockts_env_net *p = calloc(1, sizeof(*p));

    if (p != 0)
    {
        TAILQ_INIT(&p->ip4addrs);
        env->n_nets++;
        LIST_INSERT_HEAD(&env->nets, p, links);
    }
    
    return p;
}

static sockts_env_host *
create_host(void)
{
    sockts_env_host *p = calloc(1, sizeof(*p));

    if (p != 0)
    {
        CIRCLEQ_INSERT_TAIL(&env->hosts, p, links);

        LIST_INIT(&p->nets);
        LIST_INIT(&p->processes);

        if (curr_net == NULL)
            curr_net = create_net();
        if (curr_net != NULL)
        {
            curr_net->n_hosts++;
            p->n_nets++;
            LIST_INSERT_HEAD(&p->nets, curr_net, inhost);
        }
    }

    return p;
}

static sockts_env_process *
create_process(void)
{
    sockts_env_process *p = calloc(1, sizeof(*p));

    if (p != 0)
    {
        TAILQ_INIT(&p->pcos);
        if (curr_host == NULL)
            curr_host = create_host();
        if (curr_host != NULL)
            LIST_INSERT_HEAD(&curr_host->processes, p, links);
    }

    return p;
}

%}

%token OBRACE EBRACE QUOTE COLON COMMA EQUAL
%token ADDRESS INTERFACE


%union 
{
    int   number;
    char *string;
}


%token <string> WORD
%token <number> PCO_TYPE
%token <number> ADDR_FAMILY
%token <number> ADDR_TYPE


%type <string> quotedname


%%

env_cfg_str:
    nets COMMA aliases
    |
    nets
    ;

nets:
    net
    |
    nets COMMA net
    ;

net:
    OBRACE hosts EBRACE
    { 
        if (curr_net == NULL)
            (void)create_net();
        else
            curr_net = NULL;
    }
    |
    quotedname OBRACE hosts EBRACE
    {
        const char *name = $1;

        if (strlen(name) < SOCKTS_NAME_MAX)
        {
            if (curr_net == NULL)
                curr_net = create_net();
            if (curr_net != NULL)
                strcpy(curr_net->name, name);
        }
        curr_net = NULL;
    }
    ;

hosts: 
    host
    |
    hosts COMMA host
    ;

host:
    OBRACE host_items EBRACE
    {
        if (curr_host == NULL)
            (void)create_host();
        else
            curr_host = NULL;
    }
    |
    quotedname OBRACE host_items EBRACE
    {
        const char *name = $1;

        if (strlen(name) < SOCKTS_NAME_MAX)
        {
            if (curr_host == NULL)
                curr_host = create_host();
            if (curr_host != NULL)
            {
                strcpy(curr_host->name, name);
            }
        }
        curr_host = NULL;
    }
    ;

host_items:
    host_item
    |
    host_items COMMA host_item
    ;

host_item:
    process
    |
    address
    |
    interface
    ;

process:
    OBRACE pcos EBRACE
    {
        if (curr_proc == NULL)
            (void)create_process();
        else
            curr_proc = NULL;
    }
    ;

pcos:
    pco
    |
    pcos COMMA pco
    ;

pco:
    quotedname COLON PCO_TYPE
    {
        const char *name = $1;

        if (curr_proc == NULL)
            curr_proc = create_process();

        if (strlen(name) < SOCKTS_NAME_MAX)
        {
            sockts_env_pco *p = calloc(1, sizeof(*p));

            if (p != NULL)
            {
                strcpy(p->name, name);
                p->type = $3;
                p->process = curr_proc;
                if (curr_proc != NULL)
                {
                    TAILQ_INSERT_TAIL(&curr_proc->pcos, p, links);
                }
            }
        }
    }
    ;

address:
    ADDRESS COLON quotedname COLON ADDR_FAMILY COLON ADDR_TYPE
    {
        const char *name = $3;

        if (curr_host == NULL)
            curr_host = create_host();

        if (strlen(name) < SOCKTS_NAME_MAX)
        {
            sockts_env_addr   *p = calloc(1, sizeof(*p));

            if (p != NULL)
            {
                strcpy(p->name, name);
                p->net = curr_net;
                p->host = curr_host;
                p->family = $5;
                p->type = $7;
                p->handle = CFG_HANDLE_INVALID;
                CIRCLEQ_INSERT_TAIL(&env->addrs, p, links);
            }
        }
    }
    ;

interface:
    INTERFACE COLON quotedname
    {
        const char *name = $3;

        if (strlen(name) < SOCKTS_NAME_MAX)
        {
            sockts_env_if  *p = calloc(1, sizeof(*p));

            if (curr_host == NULL)
                curr_host = create_host();

            if (p != NULL)
            {
                strcpy(p->name, name);
                p->net = curr_net;
                p->host = curr_host;
                LIST_INSERT_HEAD(&env->ifs, p, links);
            }
        }
    }
    ;

aliases:
    |
    alias
    |
    aliases COMMA alias
    ;

alias:
    quotedname EQUAL quotedname
    {
        const char *alias = $1;
        const char *name = $3;

        if ((strlen(alias) < SOCKTS_NAME_MAX) ||
            (strlen(name) < SOCKTS_NAME_MAX))
        {
            sockts_env_alias   *p = calloc(1, sizeof(*p));

            if (p != NULL)
            {
                strcpy(p->alias, alias);
                strcpy(p->name, name);
                LIST_INSERT_HEAD(&env->aliases, p, links);
            }
        }
    }
    ;

quotedname:
    QUOTE WORD QUOTE
    {
        $$ = $2;
    }
    ;
