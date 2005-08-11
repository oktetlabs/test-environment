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

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for Tester
#endif

#include "te_builder_ts.h"
#include "conf_api.h"

#include "internal.h"


/** Logging entity name of the Tester subsystem */
DEFINE_LGR_ENTITY("Tester");


/**
 * Tester application command line options. Values must be started from
 * one, since zero has special meaning for popt.
 */
enum {
    TESTER_OPT_NOLOGUES = 1,
    TESTER_OPT_NORANDOM,
    TESTER_OPT_NOSIMULT,
    TESTER_OPT_SUITE_PATH,
    TESTER_OPT_RUN,
    TESTER_OPT_VALGRIND,
    TESTER_OPT_GDB,
    TESTER_OPT_REQ,
    TESTER_OPT_NOBUILD,
    TESTER_OPT_FAKE,
    TESTER_OPT_TIMEOUT,
    TESTER_OPT_VERBOSE,
    TESTER_OPT_QUIET,
    TESTER_OPT_VERSION,
    TESTER_OPT_NO_CS,
    TESTER_OPT_NOCFGTRACK,
    TESTER_OPT_QUIET_SKIP,
};


static void run_item_free(run_item *run);
static void run_items_free(run_items *runs);

    
/**
 * Initialize Tester context.
 *
 * @param ctx           Context to be initialized
 *
 * @return Status code.
 */
static int
tester_ctx_init(tester_ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->id = tester_get_id();
    TAILQ_INIT(&ctx->suites);
    TAILQ_INIT(&ctx->reqs);
    ctx->path = calloc(1, sizeof(*(ctx->path)));
    if (ctx->path == NULL)
    {
        ERROR("%s(): Memory allocation failure", __FUNCTION__);
        return TE_ENOMEM;
    }
    TAILQ_INIT(&ctx->path->params);
    TAILQ_INIT(&ctx->path->paths);
    /* By default verbosity level is set to 1 */
    ctx->flags |= TESTER_VERBOSE;

    return 0;
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
    free(p->objective);
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
    free(p->ext);
    free(p->value);
    test_requirements_free(&p->reqs);
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
 * Free variable/argument attributes.
 *
 * @param attrs     Attributes to be freed
 */
static void
test_var_arg_attrs_free(test_var_arg_attrs *attrs)
{
    free(attrs->list);
}

/**
 * Free session variable.
 *
 * @param p         Session variable to be freed.
 */
static void
test_var_arg_free(test_var_arg *p)
{
    free(p->name);
    test_var_arg_values_free(&p->values);
    test_var_arg_attrs_free(&p->attrs);
    free(p);
}

/**
 * Free list of session variables.
 *
 * @param vars      List of session variables to be freed
 */
static void
test_vars_args_free(test_vars_args *vars)
{
    test_var_arg *p;

    while ((p = vars->tqh_first) != NULL)
    {
        TAILQ_REMOVE(vars, p, links);
        test_var_arg_free(p);
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
    test_vars_args_free(&p->vars);
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
    if (p == NULL)
        return;

    free(p->name);
    free(p->path);
    free(p->objective);
    persons_info_free(&p->authors);
    test_requirements_free(&p->reqs);
    test_session_free(&p->session);
    free(p);
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
    test_vars_args_free(&run->args);
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
 * Free test suite information.
 *
 * @param p         Information about test suite
 */
static void
test_suite_info_free(test_suite_info *p)
{
    free(p->name);
    free(p->src);
    free(p->bin);
    free(p);
}

/**
 * Free list of test suites information.
 *
 * @param suites    List of with test suites information
 */
static void
test_suites_info_free(test_suites_info *suites)
{
    test_suite_info *p;

    while ((p = suites->tqh_first) != NULL)
    {
        TAILQ_REMOVE(suites, p, links);
        test_suite_info_free(p);
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
    test_suites_info_free(&cfg->suites);
    tester_reqs_expr_free(cfg->targets);
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
        { "suite", 's', POPT_ARG_STRING, NULL, TESTER_OPT_SUITE_PATH,
          "Specify path to the Test Suite.", "NAME:PATH" },

        { "req", 'R', POPT_ARG_STRING, NULL, TESTER_OPT_REQ,
          "Requirements to be tested (logical expression).",
          "REQS" },

        { "quietskip", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_QUIET_SKIP,
          "Quietly skip tests which do not meet specified requirements.",
          NULL },

        { "run", 'r', POPT_ARG_STRING, NULL, TESTER_OPT_RUN,
          "Run Test Suite with specified arguments.", "PATH" },

        { "vg", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_VALGRIND,
          "Run test scripts in specified path under valgrind.", "PATH" },

        { "gdb", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_GDB,
          "Run test scripts in specified path under gdb.", "PATH" },

        { "nobuild", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NOBUILD,
          "Don't build any Test Suites.", NULL },

        { "no-cs", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NO_CS,
          "Don't interact with Configurator.", NULL },

        { "nocfgtrack", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NOCFGTRACK,
          "Don't track configuration changes.", NULL },

        { "norandom", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NORANDOM,
          "Force to run all tests in defined order as well as to get "
          "values of all arguments in defined order. Usefull for "
          "debugging.", NULL },

        { "nologues", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NOLOGUES,
          "Disable prologues and epilogues globally.", NULL },

        { "nosimultaneous", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NOSIMULT,
          "Force to run all tests in series. Usefull for debugging.",
          NULL },

        { "fake", 'T', POPT_ARG_NONE, NULL, TESTER_OPT_FAKE,
          "Don't run any test scripts, just emulate test failure.", NULL },

        { "timeout", 't', POPT_ARG_INT, &ctx->timeout, TESTER_OPT_TIMEOUT, 
          "Restrict execution time (in seconds).", NULL },

        { "verbose", 'v', POPT_ARG_NONE, NULL, TESTER_OPT_VERBOSE,
          "Increase verbosity of the Tester (the first level is set by "
          "default).", NULL },

        { "quiet", 'q', POPT_ARG_NONE, NULL, TESTER_OPT_QUIET,
          "Decrease verbosity of the Tester.", NULL },

        { "version", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_VERSION, 
          "Display version information.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };
    
    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);
  
    poptSetOtherOptionHelp(optCon,
                           "[OPTIONS] <cfg-file1> [<cfg-file2> ...]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        switch (rc)
        {
            case TESTER_OPT_NOLOGUES:
                ctx->flags |= TESTER_NOLOGUES;
                break;

            case TESTER_OPT_NORANDOM:
                ctx->flags |= TESTER_NORANDOM;
                break;

            case TESTER_OPT_NOSIMULT:
                ctx->flags |= TESTER_NOSIMULT;
                break;

            case TESTER_OPT_FAKE:
                ctx->flags |= TESTER_FAKE;
                break;

            case TESTER_OPT_NOBUILD:
                ctx->flags |= TESTER_NOBUILD;
                break;

            case TESTER_OPT_NO_CS:
                ctx->flags |= TESTER_NO_CS;
                break;

            case TESTER_OPT_NOCFGTRACK:
                ctx->flags |= TESTER_NOCFGTRACK;
                break;

            case TESTER_OPT_QUIET_SKIP:
                ctx->flags |= TESTER_QUIET_SKIP;
                break;

            case TESTER_OPT_VERBOSE:
                if (!(ctx->flags & TESTER_VERBOSE))
                    ctx->flags |= TESTER_VERBOSE;
                else if (!(ctx->flags & TESTER_VVERB))
                    ctx->flags |= TESTER_VVERB;
                else if (!(ctx->flags & TESTER_VVVERB))
                    ctx->flags |= TESTER_VVVERB;
                else
                    WARN("Extra 'verbose' option is ignored");
                break;

            case TESTER_OPT_QUIET:
                if (ctx->flags & TESTER_VVVERB)
                    ctx->flags &= ~TESTER_VVVERB;
                else if (ctx->flags & TESTER_VVERB)
                    ctx->flags &= ~TESTER_VVERB;
                else if (ctx->flags & TESTER_VERBOSE)
                    ctx->flags &= ~TESTER_VERBOSE;
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
                    ((p->src = strdup(s + 1)) == NULL))
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
            case TESTER_OPT_VALGRIND:
            case TESTER_OPT_GDB:
            {
                char *s = strdup(poptGetOptArg(optCon));

                if (s == NULL)
                {
                    ERROR("strdup() failed");
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                rc = tester_run_path_new(ctx->path, s,
                                         (rc == TESTER_OPT_VALGRIND) ?
                                             TESTER_VALGRIND :
                                         (rc == TESTER_OPT_GDB) ?
                                              TESTER_GDB :
                                         TESTER_RUN);
                if (rc != 0)
                {
                    ERROR("Failed to add new run path '%s'", s);
                    free(s);
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;
            }

            case TESTER_OPT_REQ:
                rc = tester_new_target_reqs(&ctx->targets,
                                            poptGetOptArg(optCon));
                if (rc != 0)
                {
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
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
tester_build_suite(unsigned int flags, const test_suite_info *suite)
{
    int rc;

    RING("Build Test Suite '%s' from '%s'",
         suite->name, suite->src);
    rc = builder_build_test_suite(suite->name, suite->src);
    if (rc != 0)
    {
        const char *te_build = getenv("TE_BUILD");

        ERROR("Build of Test Suite '%s' from '%s' failed, see "
              "%s/builder.log.%s.{1,2}",
              suite->name, suite->src, te_build, suite->name);
        if (flags & TESTER_VERBOSE)
        {
            fprintf(stderr,
                    "Build of Test Suite '%s' from '%s' failed, see\n"
                    "%s/builder.log.%s.{1,2}\n",
                    suite->name, suite->src, te_build, suite->name);
        }
        return rc;
    }
    return 0;
}

/**
 * Build list of Test Suites.
 *
 * @param ctx       Tester context
 * @param suites    List of Test Suites
 *
 * @return Status code.
 */
static int
tester_build_suites(const tester_ctx *ctx, const test_suites_info *suites)
{
    int                     rc;
    const test_suite_info  *suite;

    for (suite = suites->tqh_first;
         suite != NULL;
         suite = suite->links.tqe_next)
    {
        if (suite->src != NULL)
        {
            rc = tester_build_suite(ctx->flags, suite);
            if (rc != 0)
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
    int         result = EXIT_FAILURE;
    int         rc;
    tester_ctx  ctx;
    tester_cfgs cfgs;
    tester_cfg *cfg;

#if HAVE_SIGNAL_H
    (void)signal(SIGINT, SIG_DFL);
#endif

    TAILQ_INIT(&cfgs);
    if (tester_ctx_init(&ctx) != 0)
    {
        ERROR("Initialization of Tester context failed");
        goto exit;
    }

    if (process_cmd_line_opts(&ctx, &cfgs, argc, argv) != EXIT_SUCCESS)
    {
        ERROR("Command line options processing failure");
        goto exit;
    }
    
    if ((~ctx.flags & TESTER_NOBUILD) &&
        (ctx.suites.tqh_first != NULL))
    {
        RING("Building Test Suites specified in command line...");
        rc = tester_build_suites(&ctx, &ctx.suites);
        if (rc != 0)
        {
            goto exit;
        }
    }

    for (cfg = cfgs.tqh_first; cfg != NULL; cfg = cfg->links.tqe_next)
    {
        rc = tester_parse_config(cfg, &ctx);
        if (rc != 0)
        {
            ERROR("Preprocessing of Tester configuration files failed");
            goto exit;
        }
    }

    RING("Starting...");
    for (cfg = cfgs.tqh_first; cfg != NULL; cfg = cfg->links.tqe_next)
    {
        rc = tester_run_config(&ctx, cfg);
        if (rc != TE_ETESTPASS)
        {
            ERROR("Run of configuration from file failed:\n%s",
                  cfg->filename);
        }
    }

    result = EXIT_SUCCESS;
    RING("Done");

exit:
    while ((cfg = cfgs.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&cfgs, cfg, links);
        tester_cfg_free(cfg);
    }

    /* TODO */
    tester_reqs_expr_free(ctx.targets);
    tester_run_path_free(ctx.path);
    test_requirements_free(&ctx.reqs);
    
    return result;
}
