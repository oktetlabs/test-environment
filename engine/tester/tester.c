/** @file
 * @brief Tester Subsystem
 *
 * Application main file
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for Tester
#endif

#include "te_builder_ts.h"
#include "conf_api.h"

#include "internal.h"


DEFINE_LGR_ENTITY("Tester");


/** Tester application command line options */
enum {
    TESTER_OPT_NOLOGUES,
    TESTER_OPT_NORANDOM,
    TESTER_OPT_NOSIMULT,
    TESTER_OPT_SUITE_PATH,
    TESTER_OPT_RUN,
    TESTER_OPT_VALGRIND,
    TESTER_OPT_GDB,
    TESTER_OPT_REQ,
    TESTER_OPT_FAKE,
    TESTER_OPT_TIMEOUT,
    TESTER_OPT_VERBOSE,
    TESTER_OPT_QUIET,
    TESTER_OPT_VERSION
};


static void run_item_free(run_item *run);
static void run_items_free(run_items *runs);

    
/**
 * Initialize Tester context.
 *
 * @param ctx           Context to be initialized
 */
static void
tester_ctx_init(tester_ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->id = tester_get_id();
    TAILQ_INIT(&ctx->suites);
    TAILQ_INIT(&ctx->reqs);
    TAILQ_INIT(&ctx->paths);
    /* By default verbosity level is set to 1 */
    ctx->flags |= TESTER_CTX_VERBOSE;
}

/**
 * Allocatate and initialize Tester configuration.
 *
 * @param filename      Name of the file with configuration
 *
 * @return Pointer to allocated and initialized Tester configuration or
 *         NULL.
 */
static tester_cfg *
tester_cfg_new(const char *filename)
{
    tester_cfg *p = malloc(sizeof(*p));

    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return NULL;
    }
    memset(p, 0, sizeof(*p));
    p->filename = filename;
    TAILQ_INIT(&p->maintainers);
    TAILQ_INIT(&p->suites);
    TAILQ_INIT(&p->reqs);
    TAILQ_INIT(&p->options);
    TAILQ_INIT(&p->runs);

    return p;
}


/**
 * Free information about person.
 *
 * @param p         Information about person to be freed.
 */
static void
person_info_free(person_info *p)
{
    free(p->name);
    free(p->mailto);
    free(p);
}

/**
 * Free list of information about persons.
 *
 * @param persons   List of persons to be freed
 */
static void
persons_info_free(persons_info *persons)
{
    person_info *p;

    while ((p = persons->tqh_first) != NULL)
    {
        TAILQ_REMOVE(persons, p, links);
        person_info_free(p);
    }
}


/**
 * Free test script.
 *
 * @param p         Test script to be freed
 */
static void
test_script_free(test_script *p)
{
    free(p->name);
    free(p->descr);
    free(p->execute);
    test_requirements_free(&p->reqs);
}


/**
 * Free variable/argument value.
 *
 * @param p     Value to be freed
 */
static void
test_var_arg_value_free(test_var_arg_value *p)
{
    free(p->id);
    free(p->refvalue);
    free(p->ext);
    free(p->value);
    free(p);
}

/**
 * Free list of variable/argument values.
 *
 * @param values    List of variable/argument values to be freed
 */
static void
test_var_arg_values_free(test_var_arg_values *values)
{
    test_var_arg_value *p;

    while ((p = values->tqh_first) != NULL)
    {
        TAILQ_REMOVE(values, p, links);
        test_var_arg_value_free(p);
    }
}

/**
 * Free reffered variable/argument attributes.
 *
 * @param attrs     Attributes to be freed
 */
static void
test_ref_var_arg_attrs_free(test_ref_var_arg_attrs *attrs)
{
    free(attrs->refer);
}

/**
 * Free session variable.
 *
 * @param p         Session variable to be freed.
 */
static void
test_session_var_free(test_session_var *p)
{
    free(p->name);
    if (p->type == TEST_SESSION_VAR_SIMPLE)
        test_var_arg_values_free(&p->u.var.values);
    else if (p->type == TEST_SESSION_VAR_REFERRED)
        test_ref_var_arg_attrs_free(&p->u.ref.attrs);
    else
        assert(0);
    free(p);
}

/**
 * Free list of session variables.
 *
 * @param vars      List of session variables to be freed
 */
static void
test_session_vars_free(test_session_vars *vars)
{
    test_session_var *p;

    while ((p = vars->tqh_first) != NULL)
    {
        TAILQ_REMOVE(vars, p, links);
        test_session_var_free(p);
    }
}

/**
 * Free test session.
 *
 * @param p         Test session to be freed
 */
static void
test_session_free(test_session *p)
{
    free(p->name);
    test_session_vars_free(&p->vars);
    run_item_free(p->exception);
    run_item_free(p->keepalive);
    run_item_free(p->prologue);
    run_item_free(p->epilogue);
    run_items_free(&p->run_items);
}


/**
 * Free test package.
 *
 * @param p         Test package to be freed
 */
static void
test_package_free(test_package *p)
{
    free(p->name);
    free(p->path);
    free(p->descr);
    persons_info_free(&p->authors);
    test_requirements_free(&p->reqs);
    test_session_free(&p->session);
    free(p);
}


/**
 * Free session argument.
 *
 * @param p         Argument to be freed.
 */
static void
test_arg_free(test_arg *p)
{
    free(p->name);
    if (p->type == TEST_ARG_SIMPLE)
        test_var_arg_values_free(&p->u.arg.values);
    else if (p->type == TEST_ARG_REFERRED)
        test_ref_var_arg_attrs_free(&p->u.ref.attrs);
    else
        assert(0);
    free(p);
}

/**
 * Free list of arguments.
 *
 * @param args      List of arguments to be freed
 */
static void
test_args_free(test_args *args)
{
    test_arg *p;

    while ((p = args->tqh_first) != NULL)
    {
        TAILQ_REMOVE(args, p, links);
        test_arg_free(p);
    }
}

/**
 * Free run item.
 *
 * @param run       Run item to be freed
 */
static void
run_item_free(run_item *run)
{
    if (run == NULL)
        return;

    free(run->name);
    switch (run->type)
    {
        case RUN_ITEM_NONE:
            break;

        case RUN_ITEM_SCRIPT:
            test_script_free(&run->u.script);
            break;

        case RUN_ITEM_SESSION:
            test_session_free(&run->u.session);
            break;

        case RUN_ITEM_PACKAGE:
            test_package_free(run->u.package);
            break;

        default:
            assert(0);
    }
    test_args_free(&run->args);
    free(run);
}


/**
 * Free list of run items.
 *
 * @param runs      List of run items to be freed
 */
static void
run_items_free(run_items *runs)
{
    run_item *p;

    while ((p = runs->tqh_first) != NULL)
    {
        TAILQ_REMOVE(runs, p, links);
        run_item_free(p);
    }
}


/**
 * Free Tester configuration.
 *
 * @param cfg       Tester configuration to be freed
 */
static void
tester_cfg_free(tester_cfg *cfg)
{
    persons_info_free(&cfg->maintainers);
    free(cfg->descr);
    test_requirements_free(&cfg->reqs);
    run_items_free(&cfg->runs);
    free(cfg);
}


/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated 
 * if some new options are going to be added.
 *
 * @param ctx   Tester global context
 * @param cfgs  List of Tester configurations
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
process_cmd_line_opts(tester_ctx *ctx, tester_cfgs *cfgs,
                      int argc, char **argv)
{
    poptContext  optCon;
    int          rc;
    const char  *cfg_file;

    /* Option Table */
    struct poptOption options_table[] = {
        { "run", 'r', POPT_ARG_STRING, NULL, TESTER_OPT_RUN,
          "Run Test Suite with specified arguments.", "PATH" },

        { "vg", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_VALGRIND,
          "Run test scripts in specified path under valgrind.", "PATH" },

        { "gdb", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_GDB,
          "Run test scripts in specified path under gdb.", "PATH" },

        { "req", 'R', POPT_ARG_STRING, NULL, TESTER_OPT_REQ,
          "Requirement to be tested (or excluded, if its first symbol is !).",
          "[REQ|!REQ]" },

        { "suite", 's', POPT_ARG_STRING, NULL, TESTER_OPT_SUITE_PATH,
          "Specify path to the Test Suite.", "NAME:PATH" },

        { "timeout", 't', POPT_ARG_INT, &ctx->timeout, TESTER_OPT_TIMEOUT, 
          "Restrict execution time (in seconds).", NULL },

        { "norandom", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NORANDOM,
          "Force to run all tests in defined order as well as to get "
          "values of all arguments in defined order. Usefull for "
          "debugging.", NULL },

        { "nologues", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NOLOGUES,
          "Disable prologues and epilogues globally.", NULL },

        { "nosimultaneous", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NOSIMULT,
          "Force to run all tests in series. Usefull for debugging.", NULL },

        { "fake", 'T', POPT_ARG_NONE, NULL, TESTER_OPT_FAKE,
          "Don't run any test scripts, just emulate test failure.", NULL },

        { "verbose", 'v', POPT_ARG_NONE, NULL, TESTER_OPT_VERBOSE,
          "Increase verbosity of the Tester (the first level is set by "
          "default).", NULL },

        { "quiet", 'q', POPT_ARG_NONE, NULL, TESTER_OPT_QUIET,
          "Decrease verbosity of the Tester.", NULL },

        { "version", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_VERSION, 
          "Display version information.", NULL },

        POPT_AUTOHELP
        
        { NULL, 0, 0, NULL, 0, NULL, 0 },
    };
    
    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);
  
    poptSetOtherOptionHelp(optCon, "[OPTIONS] <cfg-file1> [<cfg-file2> ...]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        switch (rc)
        {
            case TESTER_OPT_NOLOGUES:
                ctx->flags |= TESTER_CTX_NOLOGUES;
                break;

            case TESTER_OPT_NORANDOM:
                ctx->flags |= TESTER_CTX_NORANDOM;
                break;

            case TESTER_OPT_NOSIMULT:
                ctx->flags |= TESTER_CTX_NOSIMULT;
                break;

            case TESTER_OPT_FAKE:
                ctx->flags |= TESTER_CTX_FAKE;
                break;

            case TESTER_OPT_VERBOSE:
                if (!(ctx->flags & TESTER_CTX_VERBOSE))
                    ctx->flags |= TESTER_CTX_VERBOSE;
                else if (!(ctx->flags & TESTER_CTX_VVERB))
                    ctx->flags |= TESTER_CTX_VVERB;
                else if (!(ctx->flags & TESTER_CTX_VVVERB))
                    ctx->flags |= TESTER_CTX_VVVERB;
                else
                    WARN("Extra 'verbose' option is ignored");
                break;

            case TESTER_OPT_QUIET:
                if (ctx->flags & TESTER_CTX_VVVERB)
                    ctx->flags &= ~TESTER_CTX_VVVERB;
                else if (ctx->flags & TESTER_CTX_VVERB)
                    ctx->flags &= ~TESTER_CTX_VVERB;
                else if (ctx->flags & TESTER_CTX_VERBOSE)
                    ctx->flags &= ~TESTER_CTX_VERBOSE;
                else
                    WARN("Extra 'quiet' option is ignored");
                break;

            case TESTER_OPT_TIMEOUT:
                if (ctx->timeout < 0)
                {
                    ERROR("Negative timeout was specified");
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

            case TESTER_OPT_SUITE_PATH:
            {
                const char *opt = poptGetOptArg(optCon);
                const char *s = index(opt, ':');
                int name_len;
                test_suite_info *p = calloc(1, sizeof(*p));

                if ((s == NULL) || ((name_len = (s - opt)) <= 0))
                {
                    ERROR("Invalid suite path info: %s", opt);
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                if ((p == NULL) ||
                    ((p->name = malloc(name_len + 1)) == NULL) ||
                    ((p->path = strdup(s + 1)) == NULL))
                {
                    if (p != NULL)
                        free(p->name);
                    ERROR("Memory allocation failed", sizeof(*p));
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                memcpy(p->name, opt, name_len);
                p->name[name_len] = '\0';
                TAILQ_INSERT_TAIL(&ctx->suites, p, links);
                break;
            }

            case TESTER_OPT_RUN:
                rc = tester_run_path_new(&ctx->paths, poptGetOptArg(optCon));
                if (rc != 0)
                {
                    ERROR("tester_run_path_new() failed");
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

            case TESTER_OPT_REQ:
                rc = test_requirement_new(&ctx->reqs, poptGetOptArg(optCon));
                if (rc != 0)
                {
                    ERROR("test_requirement_new() failed");
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;
            
            case TESTER_OPT_VALGRIND:
                ctx->flags |= TESTER_CTX_VALGRIND;
                if (poptGetOptArg(optCon) != NULL)
                {
                    WARN("All test scripts are run under valgrind");
                }
                break;

            case TESTER_OPT_GDB:
                ctx->flags |= TESTER_CTX_GDB;
                if (poptGetOptArg(optCon) != NULL)
                {
                    WARN("All test scripts are run under gdb");
                }
                break;

            case TESTER_OPT_VERSION:
                printf("Test Environment: %s\n\n%s\n", PACKAGE_STRING,
                       TE_COPYRIGHT);
                poptFreeContext(optCon);
                return EXIT_FAILURE;

            default:
                ERROR("Unexpected option number %d", rc);
                poptFreeContext(optCon);
                return EXIT_FAILURE;
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s", poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    /* Get Tester configuration file names */
    while ((cfg_file = poptGetArg(optCon)) != NULL)
    {
        tester_cfg *cfg = tester_cfg_new(cfg_file);

        if (cfg == NULL)
        {
            poptFreeContext(optCon);
            return EXIT_FAILURE;
        }
        VERB("Configuration file to be processed: %s", cfg_file);
        TAILQ_INSERT_TAIL(cfgs, cfg, links);
    }

    poptFreeContext(optCon);

    return EXIT_SUCCESS;
}


/* See description in internal.h */
int
tester_build_suites(test_suites_info *suites)
{
    int              rc;
    test_suite_info *suite;

    for (suite = suites->tqh_first;
         suite != NULL;
         suite = suite->links.tqe_next)
    {
        rc = builder_build_test_suite(suite->name, suite->path);
        if (rc != 0)
        {
            ERROR("Build of Test Suite '%s' from '%s' failed",
                  suite->name, suite->path);
            return rc;
        }
    }
    return 0;
}


/**
 * Application entry point.
 *
 * @param argc      Number of arguments including executable
 * @param argv      Array with pointers to arguments as strings
 *
 * @retval EXIT_SUCCESS     Success
 * @retval EXIT_FAILURE     Failure
 */
int
main(int argc, char *argv[])
{
    int         result = EXIT_SUCCESS;
    int         rc;
    tester_ctx  ctx;
    tester_cfgs cfgs;
    tester_cfg *cfg;

    TAILQ_INIT(&cfgs);
    tester_ctx_init(&ctx);

    if (process_cmd_line_opts(&ctx, &cfgs, argc, argv) != EXIT_SUCCESS)
    {
        ERROR("Command line options processing failure");
        result = EXIT_FAILURE;
        goto exit;
    }
    
    if (ctx.suites.tqh_first != NULL)
    {
        RING("Building Test Suites specified in command line...");
        rc = tester_build_suites(&ctx.suites);
        if (rc != 0)
        {
            result = EXIT_FAILURE;
            goto exit;
        }
    }

    for (cfg = cfgs.tqh_first; cfg != NULL; cfg = cfg->links.tqe_next)
    {
        rc = tester_parse_config(cfg);
        if (rc != 0)
        {
            ERROR("Preprocessing of Tester configuration files failed");
            result = EXIT_FAILURE;
            goto exit;
        }
    }

    RING("Starting...");
    for (cfg = cfgs.tqh_first; cfg != NULL; cfg = cfg->links.tqe_next)
    {
        rc = tester_run_config(&ctx, cfg);
        if (rc != 0)
        {
            ERROR("Run of configuration from file failed:\n%s",
                  cfg->filename);
            result = EXIT_FAILURE;
            goto exit;
        }
    }

    RING("Done");

exit:
    while ((cfg = cfgs.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&cfgs, cfg, links);
        tester_cfg_free(cfg);
    }

    /* TODO */
    tester_run_paths_free(&ctx.paths);
    test_requirements_free(&ctx.reqs);
    
    cfg_api_cleanup();
    log_client_close();

    return result;
}
