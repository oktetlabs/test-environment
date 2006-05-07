%{
#define TE_LOG_USER     "Env String Parser"
    
#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "logger_api.h"

#include "tapi_env.h"


#define YYDEBUG 0
#define YY_SKIP_YYWRAP


extern int yylex(void);
extern int yylex_destroy(void);
extern int yyparse(void);

int yydebug = 0;

const char *mybuf;
int myindex;

static tapi_env          *env;
static tapi_env_net      *curr_net;
static tapi_env_if       *curr_host_if;
static tapi_env_process  *curr_proc;


void
yyerror(const char *str)
{
    ERROR("YACC: %s", str);
}

int
env_cfg_parse(tapi_env *e, const char *cfg)
{
    /* Populate passed environment as static variable in the file */
    env = e;
    /* Reset pointers to current nodes to NULL */
    curr_net  = NULL;
    curr_host_if = NULL;
    curr_proc = NULL;
    
    mybuf = cfg;
    myindex = 0;
    
    yyparse();
    yylex_destroy();

    return 0;
}

static tapi_env_net *
create_net(void)
{
    tapi_env_net *p = calloc(1, sizeof(*p));

    assert(p != NULL);
    if (p != 0)
    {
        TAILQ_INIT(&p->ip4addrs);
        env->n_nets++;
        LIST_INSERT_HEAD(&env->nets, p, links);
    }
    
    return p;
}

static tapi_env_if *
create_host_if(void)
{
    tapi_env_if    *iface = calloc(1, sizeof(*iface));

    assert(iface != NULL);
    if (iface != NULL)
    {
        tapi_env_host  *host = calloc(1, sizeof(*host));

        assert(host != NULL);
        if (host != NULL)
        {
            LIST_INIT(&host->processes);
        }

        CIRCLEQ_INSERT_TAIL(&env->ifs, iface, links);

        if (curr_net == NULL)
            curr_net = create_net();

        iface->net = curr_net;
        iface->host = host;

        assert(iface->net != NULL);
        assert(iface->host != NULL);
    }

    return iface;
}

static tapi_env_process *
create_process(void)
{
    tapi_env_process *p = calloc(1, sizeof(*p));

    assert(p != NULL);
    if (p != NULL)
    {
        TAILQ_INIT(&p->pcos);
        if (curr_host_if == NULL)
            curr_host_if = create_host_if();
        if (curr_host_if != NULL && curr_host_if->host != NULL)
            LIST_INSERT_HEAD(&curr_host_if->host->processes, p, links);
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
%token <number> ENTITY_TYPE
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
    ENTITY_TYPE OBRACE hosts EBRACE
    { 
        if (curr_net == NULL)
            curr_net = create_net();
        if (curr_net != NULL)
            curr_net->type = $1;
        curr_net = NULL;
    }
    |
    quotedname OBRACE hosts EBRACE
    {
        char *name = $1;

        if (curr_net == NULL)
            curr_net = create_net();
        if (curr_net != NULL)
        {
            curr_net->name = name;
            name = NULL;
        }
        free(name);
        curr_net = NULL;
    }
    |
    quotedname COLON ENTITY_TYPE OBRACE hosts EBRACE
    {
        char *name = $1;

        if (curr_net == NULL)
            curr_net = create_net();
        if (curr_net != NULL)
        {
            curr_net->type = $3;
            curr_net->name = name;
            name = NULL;
        }
        free(name);
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
        if (curr_host_if == NULL)
            curr_host_if = create_host_if();
        if (curr_host_if != NULL && curr_host_if->host != NULL)
        {
            /* Host is unnamed, so it does not match any other */
            LIST_INSERT_HEAD(&env->hosts, curr_host_if->host, links);
        }
        curr_host_if = NULL;
    }
    |
    quotedname OBRACE host_items EBRACE
    {
        char *name = $1;

        if (curr_host_if == NULL)
            curr_host_if = create_host_if();
        if (curr_host_if != NULL && curr_host_if->host != NULL)
        {
            tapi_env_host  *p = NULL;

            curr_host_if->host->name = name;
            name = NULL;
            if (*curr_host_if->host->name != '\0')
            {
                for (p = env->hosts.lh_first;
                     p != NULL &&
                     (p->name == NULL ||
                      strcmp(p->name, curr_host_if->host->name) != 0);
                     p = p->links.le_next);

                if (p != NULL)
                {
                    tapi_env_process *proc;

                    /* Host with the same name found: */
                    /* - copy processes */
                    while ((proc = curr_host_if->host->
                                       processes.lh_first) != NULL)
                    {
                        LIST_REMOVE(proc, links);
                        LIST_INSERT_HEAD(&p->processes, proc, links);
                    }
                    /* - substitute reference in interface */
                    free(curr_host_if->host);
                    curr_host_if->host = p;
                }
            }
            if (p == NULL)
            {
                /* Host is unnamed or not found */
                LIST_INSERT_HEAD(&env->hosts, curr_host_if->host,
                                 links);
            }
        }
        free(name);
        curr_host_if = NULL;
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
    quotedname COLON ENTITY_TYPE
    {
        char         *name = $1;
        tapi_env_pco *p = calloc(1, sizeof(*p));

        if (curr_proc == NULL)
            curr_proc = create_process();

        assert(p != NULL);
        if (p != NULL)
        {
            p->name = name;
            name = NULL;
            p->type = $3;
            p->process = curr_proc;
            if (curr_proc != NULL)
            {
                TAILQ_INSERT_TAIL(&curr_proc->pcos, p, links);
            }
        }
        free(name);
    }
    ;

address:
    ADDRESS COLON quotedname COLON ADDR_FAMILY COLON ADDR_TYPE
    {
        char            *name = $3;
        tapi_env_addr   *p = calloc(1, sizeof(*p));

        if (curr_host_if == NULL)
            curr_host_if = create_host_if();

        assert(p != NULL);
        if (p != NULL)
        {
            p->name = name;
            name = NULL;
            p->iface = curr_host_if;
            p->family = $5;
            p->type = $7;
            p->handle = CFG_HANDLE_INVALID;
            CIRCLEQ_INSERT_TAIL(&env->addrs, p, links);
        }
        free(name);
    }
    ;

interface:
    INTERFACE COLON quotedname
    {
        char *name = $3;

        if (curr_host_if == NULL)
            curr_host_if = create_host_if();

        if (curr_host_if != NULL)
        {
            curr_host_if->name = name;
            name = NULL;
        }
        free(name);
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
        char             *alias = $1;
        char             *name = $3;
        tapi_env_alias   *p = calloc(1, sizeof(*p));

        assert(p != NULL);
        if (p != NULL)
        {
            p->alias = alias;
            p->name = name;
            LIST_INSERT_HEAD(&env->aliases, p, links);
        }
        else
        {
            free(alias);
            free(name);
        }
    }
    ;

quotedname:
    QUOTE WORD QUOTE
    {
        $$ = $2;
    }
    ;
