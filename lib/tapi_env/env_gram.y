%{
/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2005-2022 OKTET Labs Ltd. All rights reserved. */
#define TE_LOG_USER     "Env String Parser"

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "logger_api.h"
#include "te_alloc.h"

#include "tapi_env.h"


#define YYDEBUG 1
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

#define YYERROR_VERBOSE 1

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
    tapi_env_net *p = TE_ALLOC(sizeof(*p));

    STAILQ_INIT(&p->net_addrs);
    env->n_nets++;
    SLIST_INSERT_HEAD(&env->nets, p, links);

    return p;
}

static tapi_env_if *
create_host_if(void)
{
    tapi_env_if *iface = TE_ALLOC(sizeof(*iface));
    tapi_env_host *host = TE_ALLOC(sizeof(*host));

    SLIST_INIT(&host->processes);
    host->net_stats = TE_VEC_INIT(struct tapi_cfg_net_stats);

    CIRCLEQ_INSERT_TAIL(&env->ifs, iface, links);

    if (curr_net == NULL)
        curr_net = create_net();

    iface->net = curr_net;
    iface->host = host;
    iface->stats = TE_VEC_INIT(struct tapi_cfg_if_stats);

    return iface;
}

static tapi_env_process *
create_process(void)
{
    tapi_env_process *p = TE_ALLOC(sizeof(*p));

    STAILQ_INIT(&p->pcos);
    STAILQ_INIT(&p->ifs);
    if (curr_host_if == NULL)
        curr_host_if = create_host_if();
    p->net = curr_net;
    SLIST_INSERT_HEAD(&curr_host_if->host->processes, p, links);

    return p;
}

static void
add_process(tapi_env_host *host, tapi_env_process *anew)
{
    tapi_env_process *p;

    /* Merge processes with the same name */
    if (anew->name != NULL)
    {
        SLIST_FOREACH(p, &host->processes, links)
        {
            if (p->name != NULL && strcmp(p->name, anew->name) == 0)
            {
                STAILQ_CONCAT(&p->pcos, &anew->pcos);
                STAILQ_CONCAT(&p->ifs, &anew->ifs);
                /*
                 * If process is used in many networks, avoid usage of
                 * network type to determine PCO type.
                 */
                p->net = NULL;

                free(anew->name);
                free(anew);
                return;
            }
        }
    }
    SLIST_INSERT_HEAD(&host->processes, anew, links);
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
            SLIST_INSERT_HEAD(&env->hosts, curr_host_if->host, links);
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
                for (p = SLIST_FIRST(&env->hosts);
                     p != NULL &&
                     (p->name == NULL ||
                      strcmp(p->name, curr_host_if->host->name) != 0);
                     p = SLIST_NEXT(p, links));

                if (p != NULL)
                {
                    tapi_env_process *proc;

                    /* Host with the same name found: */
                    /* - copy processes */
                    while ((proc = SLIST_FIRST(&curr_host_if->host->
                                                   processes)) != NULL)
                    {
                        SLIST_REMOVE(&curr_host_if->host->processes,
                                     proc, tapi_env_process, links);
                        add_process(p, proc);
                    }
                    /* - substitute reference in interface */
                    free(curr_host_if->host);
                    curr_host_if->host = p;
                }
            }
            if (p == NULL)
            {
                /* Host is unnamed or not found */
                SLIST_INSERT_HEAD(&env->hosts, curr_host_if->host,
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
    OBRACE process_items EBRACE
    {
        if (curr_proc == NULL)
            (void)create_process();
        else
            curr_proc = NULL;
    }
    |
    quotedname OBRACE process_items EBRACE
    {
        /* Current process is always created by process items */
        assert(curr_proc != NULL);
        curr_proc->name = $1;

        /* Temporarily remove process from the list to merge it */
        SLIST_REMOVE(&curr_host_if->host->processes, curr_proc,
                     tapi_env_process, links);
        add_process(curr_host_if->host, curr_proc);
        curr_proc = NULL;
    }
    ;

process_items:
    pcos
    |
    pcos COMMA interface
    {
        tapi_env_ps_if *ps_if = TE_ALLOC(sizeof(*ps_if));

        ps_if->iface = curr_host_if;

        assert(curr_proc != NULL);
        STAILQ_INSERT_TAIL(&curr_proc->ifs, ps_if, links);
    }
    |
    interface
    {
        tapi_env_ps_if *ps_if = TE_ALLOC(sizeof(*ps_if));

        ps_if->iface = curr_host_if;

        curr_proc = create_process();
        STAILQ_INSERT_TAIL(&curr_proc->ifs, ps_if, links);
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
        tapi_env_pco *p = TE_ALLOC(sizeof(*p));

        if (curr_proc == NULL)
            curr_proc = create_process();

        p->name = $1;
        p->type = $3;
        p->process = curr_proc;
        STAILQ_INSERT_TAIL(&curr_proc->pcos, p, links);
    }
    ;

address:
    ADDRESS COLON quotedname COLON ADDR_FAMILY COLON ADDR_TYPE
    {
        tapi_env_addr *p = TE_ALLOC(sizeof(*p));

        if (curr_host_if == NULL)
            curr_host_if = create_host_if();

        p->name = $3;
        p->source = NULL;
        p->iface = curr_host_if;
        p->family = $5;
        p->type = $7;
        p->handle = CFG_HANDLE_INVALID;
        CIRCLEQ_INSERT_TAIL(&env->addrs, p, links);
    }
    |
    ADDRESS COLON quotedname COLON ADDR_FAMILY COLON ADDR_TYPE COLON quotedname
    {
        tapi_env_addr *p = TE_ALLOC(sizeof(*p));

        if (curr_host_if == NULL)
            curr_host_if = create_host_if();

        p->name = $3;
        p->source = $9;
        p->iface = curr_host_if;
        p->family = $5;
        p->type = $7;
        p->handle = CFG_HANDLE_INVALID;
        CIRCLEQ_INSERT_TAIL(&env->addrs, p, links);
    }
    ;

interface:
    INTERFACE COLON quotedname
    {
        char *name = $3;

        if (curr_host_if == NULL)
            curr_host_if = create_host_if();

        if (curr_host_if->name != NULL &&
            strcmp(curr_host_if->name, name) != 0)
        {
            ERROR("Interface name conflict: '%s' vs '%s'",
                  curr_host_if->name, name);
        }
        free(curr_host_if->name);
        curr_host_if->name = name;
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
        tapi_env_alias *p = TE_ALLOC(sizeof(*p));

        p->alias = $1;
        p->name = $3;
        SLIST_INSERT_HEAD(&env->aliases, p, links);
    }
    ;

quotedname:
    QUOTE WORD QUOTE
    {
        $$ = $2;
    }
    ;
