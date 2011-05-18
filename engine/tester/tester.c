/** @file
 * @brief Tester Subsystem
 *
 * Application main file
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
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
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for Tester
#endif

#include "te_defs.h"
#include "tq_string.h"

#include "tester.h"

#if WITH_TRC
#include "te_trc.h"
#endif

/** Logging entity name of the Tester subsystem */
DEFINE_LGR_ENTITY("Tester");


extern int test_path_lex_destroy(void);

/** Is SIGINT signal received? */
te_bool tester_sigint_received = FALSE;


tester_global tester_global_context;

/**
 * Initialize Tester global context.
 *
 * @param global        Global context to be initialized
 *
 * @return Status code.
 */
static te_errno
tester_global_init(tester_global *global)
{
    memset(global, 0, sizeof(*global));

    /* By default, random seed uses time from Epoch in seconds */
    global->rand_seed = (unsigned int)time(NULL);

    /* By default verbosity level is set to 1 */
    global->flags |= TESTER_VERBOSE | TESTER_NO_TRC | TESTER_QUIET_SKIP;

    global->cfgs.total_iters = 0;
    TAILQ_INIT(&global->cfgs.head);
    TAILQ_INIT(&global->suites);
    TAILQ_INIT(&global->paths);
    TAILQ_INIT(&global->reqs);

    global->targets = NULL;

    global->trc_db = NULL;
    TAILQ_INIT(&global->trc_tags);

    TAILQ_INIT(&global->scenario);

    return 0;
}

/**
 * Free Tester global context.
 *
 * @param global        Global context to be freed
 *
 * @return Status code.
 */
static void
tester_global_free(tester_global *global)
{
    tester_cfgs_free(&global->cfgs);
    test_suites_info_free(&global->suites);
    test_paths_free(&global->paths);
    logic_expr_free(global->targets);
#if WITH_TRC 
    trc_db_close(global->trc_db);
    tq_strings_free(&global->trc_tags, free);
#endif
    scenario_free(&global->scenario);
}


/**
 * Handler of SIGINT signal.
 *
 * @param signum        Signal number (unused)
 */
static void
tester_sigint_handler(int signum)
{
    UNUSED(signum);
    WARN("SIGINT received");
    tester_sigint_received = TRUE;
}


/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated
 * if some new options are going to be added.
 *
 * @param global        Tester global context
 * @param argc          Number of elements in array "argv"
 * @param argv          Array of strings that represents all command
 *                      line arguments
 *
 * @return Status code.
 */
static te_errno
process_cmd_line_opts(tester_global *global, int argc, char **argv)
{
    /**
     * Tester application command line options. Values must be started
     * from one, since zero has special meaning for popt.
     *
     * @attention Order of TESTER_OPT_RUN..TESTER_OPT_MIX_SESSIONS
     *            should be exactly the same as items of
     *            test_path_type enumeration.
     */
    enum {
        TESTER_OPT_VERSION = 1,

        TESTER_OPT_QUIET,
        TESTER_OPT_VERBOSE,
        TESTER_OPT_OUT_TIN,
        TESTER_OPT_OUT_TEST_PARAMS,
        TESTER_OPT_OUT_EXPECTED,
        TESTER_OPT_IGNORE_RUN_NAME,
        TESTER_OPT_INTERACTIVE,

        TESTER_OPT_NO_BUILD,
        TESTER_OPT_NO_RUN,
        TESTER_OPT_NO_TRC,
        TESTER_OPT_NO_CS,
        TESTER_OPT_NO_CFG_TRACK,
        TESTER_OPT_NO_LOGUES,
        TESTER_OPT_NO_SIMULT,

        TESTER_OPT_REQ,
        TESTER_OPT_REQ_LIST,
        TESTER_OPT_QUIET_SKIP,
        TESTER_OPT_VERB_SKIP,

        TESTER_OPT_RUN,
        TESTER_OPT_RUN_FORCE,
        TESTER_OPT_RUN_FROM,
        TESTER_OPT_RUN_TO,
        TESTER_OPT_RUN_EXCLUDE,

        TESTER_OPT_VALGRIND,
        TESTER_OPT_GDB,

        TESTER_OPT_MIX,
        TESTER_OPT_MIX_VALUES,
        TESTER_OPT_MIX_ARGS,
        TESTER_OPT_MIX_TESTS,
        TESTER_OPT_MIX_ITERS,
        TESTER_OPT_MIX_SESSIONS,
        TESTER_OPT_NO_MIX,

        TESTER_OPT_FAKE,

        TESTER_OPT_SUITE_PATH,

        TESTER_OPT_TRC_DB,
        TESTER_OPT_TRC_TAG,
        TESTER_OPT_TRC_COMPARISON,
        TESTER_OPT_BREAK_SESSION,
    };

    /* Option Table */
    const struct poptOption options_table[] = {
        { "interactive", 'i', POPT_ARG_NONE, NULL, TESTER_OPT_INTERACTIVE,
          "Enter interactive mode after initial test scenario "
          "execution.", NULL },

        { "suite", 's', POPT_ARG_STRING, NULL, TESTER_OPT_SUITE_PATH,
          "Specify path to the Test Suite.", "<name>:<path>" },

        { "no-run", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NO_RUN,
          "Don't run any tests.", NULL },

        { "no-build", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NO_BUILD,
          "Don't build any Test Suites.", NULL },
        { "nobuild", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NO_BUILD,
          "(obsolete) Don't build any Test Suites.", NULL },

        { "no-trc", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NO_TRC,
          "Don't use Testing Results Comparator.", NULL },

        { "no-cs", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NO_CS,
          "Don't interact with Configurator.", NULL },
        { "no-cfg-track", '\0', POPT_ARG_NONE, NULL,
          TESTER_OPT_NO_CFG_TRACK,
          "Don't track configuration changes.", NULL },

        { "no-logues", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_NO_LOGUES,
          "Disable prologues and epilogues globally.", NULL },

#if 0
        { "no-simultaneous", '\0', POPT_ARG_NONE, NULL,
          TESTER_OPT_NO_SIMULT,
          "Force to run all tests in series. Usefull for debugging.",
          NULL },
#endif

        { "req", 'R', POPT_ARG_STRING, NULL, TESTER_OPT_REQ,
          "Requirements to be tested (logical expression).",
          "REQS" },
        { "reqs-list", 'R', POPT_ARG_NONE, NULL, TESTER_OPT_REQ_LIST,
          "Print all requirements mentioned in the packages into the log",
          NULL },
        { "quietskip", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_QUIET_SKIP,
          "Quietly skip tests which do not meet specified requirements.",
          NULL },

        { "verbskip", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_VERB_SKIP,
          "Shout when skiping tests which do not meet "
          "specified requirements.",
          NULL },

        { "fake", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_FAKE,
          "Don't run any test scripts, just emulate test scenario.",
          "<testpath>" },

        { "run", 'r', POPT_ARG_STRING, NULL, TESTER_OPT_RUN,
          "Run test under the path.", "<testpath>" },
#if 0
        { "run-force", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_RUN_FORCE,
          "Run test with specified arguments even if such values are "
          "not mentioned in package.xml.", "<testpath>" },
#endif
        { "run-from", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_RUN_FROM,
          "Run tests starting from the test path.",
          "<testpath>" },
        { "run-to", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_RUN_TO,
          "Run tests up to the test path.",
          "<testpath>" },
        { "exclude", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_RUN_EXCLUDE,
          "Exclude tests specified by path from testing campaign.",
          "<testpath>" },

        { "vg", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_VALGRIND,
          "Run test scripts under specified path using valgrind.",
          "<testpath>" },
        { "gdb", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_GDB,
          "Run test scripts under specified path using gdb.",
          "<testpath>" },

#if 0
        { "mix", '\0', POPT_ARG_STRING, NULL,
          TESTER_OPT_MIX,
          "Mix everything in boundaries of sessions.",
          "<testpath>" },
        { "mix-values", '\0', POPT_ARG_STRING, NULL,
          TESTER_OPT_MIX_VALUES,
          "Mix values of arguments.",
          "<testpath>" },
        { "mix-args", '\0', POPT_ARG_STRING, NULL,
          TESTER_OPT_MIX_ARGS,
          "Mix iteration order of arguments.",
          "<testpath>" },
        { "mix-tests", '\0', POPT_ARG_STRING, NULL,
          TESTER_OPT_MIX_TESTS,
          "Mix tests in boundaries of sessions.",
          "<testpath>" },
        { "mix-iters", '\0', POPT_ARG_STRING, NULL,
          TESTER_OPT_MIX_ITERS,
          "Mix iterations of different tests in boundaries of session",
          "<testpath>" },
        { "mix-sessions", '\0', POPT_ARG_STRING, NULL,
          TESTER_OPT_MIX_SESSIONS,
          "Mix elements of different sessions.",
          "<testpath>" },
        { "no-mix", '\0', POPT_ARG_STRING, NULL,
          TESTER_OPT_NO_MIX,
          "Disable all enabled mixture setting.",
          "<testpath>" },
#endif

        { "random-seed", '\0', POPT_ARG_INT, &global->rand_seed, 0,
          "Random seed to initialize pseudo-random number generator",
          "<number>" },

        { "trc-db", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_TRC_DB,
          "TRC database to be used.", NULL },
        { "trc-tag", '\0', POPT_ARG_STRING, NULL, TESTER_OPT_TRC_TAG,
          "Tags to customize TRC expectations.", NULL },
        { "trc-comparison", '\0', POPT_ARG_STRING, NULL, 
          TESTER_OPT_TRC_COMPARISON,
          "Parameter comparison method (default is 'exact').",
          "exact|casefold|normalised|tokens" },

        { "out-tin", 't', POPT_ARG_NONE, NULL, TESTER_OPT_OUT_TIN,
          "Output Test Identification Numbers (TINs) to terminal.", NULL },

        { "out-test-params", '\0', POPT_ARG_NONE, NULL,
          TESTER_OPT_OUT_TEST_PARAMS,
          "Output Test Iteration parameters to the terminal.", NULL },

        { "out-expected", 'e', POPT_ARG_NONE, NULL, TESTER_OPT_OUT_EXPECTED,
          "If result is expected, output the result just after OK.", NULL },

        { "ignore-run-name", '\0', POPT_ARG_NONE, NULL,
          TESTER_OPT_IGNORE_RUN_NAME,
          "Force testing flow logging to ignore run item names.", NULL },

        { "verbose", 'v', POPT_ARG_NONE, NULL, TESTER_OPT_VERBOSE,
          "Increase verbosity of the Tester (the first level is set by "
          "default).", NULL },

        { "quiet", 'q', POPT_ARG_NONE, NULL, TESTER_OPT_QUIET,
          "Decrease verbosity of the Tester.", NULL },

        { "break-session", '\0', POPT_ARG_NONE, NULL,
          TESTER_OPT_BREAK_SESSION,
          "Skip session epilogue when session is broken "
          "with user's Ctrl-C. Use with caution!!!", NULL },

        { "version", '\0', POPT_ARG_NONE, NULL, TESTER_OPT_VERSION,
          "Display version information.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    poptContext  optCon;
    int          rc;
    const char  *cfg_file;
#if WITH_TRC
    te_bool      no_trc = FALSE;
#else
    te_bool      no_trc = TRUE;
#endif
    te_bool      warn_no_trc = TRUE;


    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);

    poptSetOtherOptionHelp(optCon,
                           "[OPTIONS] <cfg-file1> [<cfg-file2> ...]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        switch (rc)
        {
            case TESTER_OPT_NO_LOGUES:
                global->flags |= TESTER_NO_LOGUES;
                break;

            case TESTER_OPT_NO_SIMULT:
                global->flags |= TESTER_NO_SIMULT;
                break;

            case TESTER_OPT_NO_RUN:
                global->flags |= TESTER_NO_RUN;
                break;

            case TESTER_OPT_NO_BUILD:
                global->flags |= TESTER_NO_BUILD;
                break;

            case TESTER_OPT_NO_TRC:
                global->flags |= TESTER_NO_TRC;
                no_trc = TRUE;
                break;

            case TESTER_OPT_NO_CS:
                global->flags |= (TESTER_NO_CS | TESTER_NO_CFG_TRACK);
                break;

            case TESTER_OPT_NO_CFG_TRACK:
                global->flags |= TESTER_NO_CFG_TRACK;
                break;

            case TESTER_OPT_QUIET_SKIP:
                global->flags |= TESTER_QUIET_SKIP;
                break;

            case TESTER_OPT_VERB_SKIP:
                global->flags |= TESTER_VERB_SKIP;
                break;

            case TESTER_OPT_VERBOSE:
                if (!(global->flags & TESTER_VERBOSE))
                    global->flags |= TESTER_VERBOSE;
                else if (!(global->flags & TESTER_VVERB))
                    global->flags |= TESTER_VVERB;
                else
                    WARN("Extra 'verbose' option is ignored");
                break;

            case TESTER_OPT_QUIET:
                if (global->flags & TESTER_VVERB)
                    global->flags &= ~TESTER_VVERB;
                else if (global->flags & TESTER_VERBOSE)
                    global->flags &= ~TESTER_VERBOSE;
                else
                    WARN("Extra 'quiet' option is ignored");
                break;

            case TESTER_OPT_OUT_TIN:
                global->flags |= TESTER_OUT_TIN;
                break;

            case TESTER_OPT_OUT_TEST_PARAMS:
                global->flags |= TESTER_OUT_TEST_PARAMS;
                break;

            case TESTER_OPT_OUT_EXPECTED:
                global->flags |= TESTER_OUT_EXP;
                break;

            case TESTER_OPT_IGNORE_RUN_NAME:
                global->flags |= TESTER_LOG_IGNORE_RUN_NAME;
                break;

            case TESTER_OPT_INTERACTIVE:
                global->flags |= TESTER_INTERACTIVE;
                break;

            case TESTER_OPT_SUITE_PATH:
            {
                const char         *opt = poptGetOptArg(optCon);
                const char         *s = index(opt, ':');
                int                 name_len;
                test_suite_info    *p;

                if ((s == NULL) || ((name_len = (s - opt)) <= 0))
                {
                    ERROR("Invalid suite path info: %s", opt);
                    poptFreeContext(optCon);
                    return TE_EINVAL;
                }

                if (((p = calloc(1, sizeof(*p))) == NULL) ||
                    ((p->name = malloc(name_len + 1)) == NULL) ||
                    ((p->src = strdup(s + 1)) == NULL))
                {
                    if (p != NULL)
                    {
                        free(p->name);
                        free(p);
                    }

                    ERROR("Memory allocation failed");
                    poptFreeContext(optCon);
                    return TE_ENOMEM;
                }
                memcpy(p->name, opt, name_len);
                p->name[name_len] = '\0';
                TAILQ_INSERT_TAIL(&global->suites, p, links);
                break;
            }

            case TESTER_OPT_RUN:
            case TESTER_OPT_RUN_FORCE:
            case TESTER_OPT_RUN_FROM:
            case TESTER_OPT_RUN_TO:
            case TESTER_OPT_RUN_EXCLUDE:
            case TESTER_OPT_VALGRIND:
            case TESTER_OPT_GDB:
            case TESTER_OPT_MIX:
            case TESTER_OPT_MIX_VALUES:
            case TESTER_OPT_MIX_ARGS:
            case TESTER_OPT_MIX_TESTS:
            case TESTER_OPT_MIX_ITERS:
            case TESTER_OPT_MIX_SESSIONS:
            case TESTER_OPT_NO_MIX:
            case TESTER_OPT_FAKE:
            {
                const char *s = poptGetOptArg(optCon);

                rc = test_path_new(&global->paths, s,
                                   TEST_PATH_RUN +
                                       (rc - TESTER_OPT_RUN));
                if (rc != 0)
                {
                    ERROR("Failed to add new test path '%s'", s);
                    poptFreeContext(optCon);
                    return rc;
                }
                break;
            }

            case TESTER_OPT_REQ:
                rc = tester_new_target_reqs(&global->targets,
                                            poptGetOptArg(optCon));
                if (rc != 0)
                {
                    poptFreeContext(optCon);
                    return rc;
                }
                break;
            case TESTER_OPT_REQ_LIST:
                global->flags |= TESTER_LOG_REQS_LIST;
                break;

            case TESTER_OPT_TRC_DB:
            case TESTER_OPT_TRC_TAG:
            case TESTER_OPT_TRC_COMPARISON:
                if (!no_trc)
                {
#if WITH_TRC
                    /* Initialize TRC instance, if necessary */
                    if (rc == TESTER_OPT_TRC_DB)
                    {
                        rc = trc_db_open(poptGetOptArg(optCon),
                                         &global->trc_db);
                        if (rc != 0)
                        {
                            poptFreeContext(optCon);
                            return rc;
                        }
                        global->flags &= ~TESTER_NO_TRC;
                    }
                    else if (rc == TESTER_OPT_TRC_COMPARISON)
                    {
                        const char *method = poptGetOptArg(optCon);
                        
                        if (strcmp(method, "exact") == 0)
                        {
                            trc_db_compare_values = strcmp;
                        }
                        else if (strcmp(method, "casefold") == 0)
                        {
                            trc_db_compare_values = strcasecmp;
                        }
                        else if (strcmp(method, "normalised") == 0)
                        {
                            trc_db_compare_values = trc_db_strcmp_normspace;
                        }
                        else if (strcmp(method, "tokens") == 0)
                        {
                            trc_db_compare_values = trc_db_strcmp_tokens;
                        }
                        else
                        {
                            poptFreeContext(optCon);
                            return TE_RC(TE_TESTER, TE_EINVAL);
                        }
                    }
                    else
                    {
                        te_errno rc;

                        rc = trc_add_tag(&global->trc_tags,
                                         (char *)poptGetOptArg(optCon));
                        if (rc != 0) 
                        {
                            poptFreeContext(optCon);
                            return TE_RC(TE_TESTER, TE_EINVAL);
                        }
                    }
#else
                    /* Unreachable */
                    assert(FALSE);
#endif
                }
                else if (warn_no_trc)
                {
                    warn_no_trc = FALSE;
                    WARN("No TRC, related command-line options are "
                         "ignored");
                }
                break;

            case TESTER_OPT_BREAK_SESSION:
                global->flags |= TESTER_BREAK_SESSION;
                break;

            case TESTER_OPT_VERSION:
                printf("Test Environment: %s\n\n%s\n", PACKAGE_STRING,
                       TE_COPYRIGHT);
                poptFreeContext(optCon);
                return TE_EINTR;

            default:
                ERROR("Unexpected option number %d", rc);
                poptFreeContext(optCon);
                return TE_EINVAL;
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s", poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        poptFreeContext(optCon);
        return TE_EINVAL;
    }

    /* Get Tester configuration file names */
    while ((cfg_file = poptGetArg(optCon)) != NULL)
    {
        tester_cfg *cfg = tester_cfg_new(cfg_file);

        if (cfg == NULL)
        {
            poptFreeContext(optCon);
            return TE_ENOMEM;
        }
        VERB("Configuration file to be processed: %s", cfg_file);
        TAILQ_INSERT_TAIL(&global->cfgs.head, cfg, links);
    }

    poptFreeContext(optCon);

    return 0;
}

#define TESTER_ENV_SIZE 1024 * 256
#define TESTER_ENV_VAL_SIZE  1024

static void
tester_log_global(void)
{
    int i;
    /* fixme: this is shit! */
    char *glob = calloc(1, TESTER_ENV_SIZE);
    int rc = 0;
    char *p;

    rc += sprintf(glob, "Tester global variables list:\n");

    for (i = 0; environ[i] != NULL; i++)
    {
        if (strncmp(environ[i], TEST_ARG_ENV_PREFIX,
                    strlen(TEST_ARG_ENV_PREFIX)) == 0)
        {
            char *c;
            char *eq;
            char e[TESTER_ENV_VAL_SIZE];

            /* need to convert __ back to . */
            strncpy(e, environ[i], sizeof(e));
            eq = strchr(e, '='); /* separator */

            p = e + strlen(TEST_ARG_ENV_PREFIX);

            rc += sprintf(glob + rc, "  ");

            do {
                c = strstr(p, "__");
                RING("%s rc=%d c=%p eq=%p", p, rc, c, eq);
                if (c != NULL && c < eq) {
                    *c = '\0';
                    rc += snprintf(glob + rc, TESTER_ENV_SIZE - rc,
                                   "%s.", p);
                    RING("%s rc=%d", p, rc);
                    p = c + 2;
                }
                else
                {
                    c = NULL;
                    rc += snprintf(glob + rc, TESTER_ENV_SIZE - rc,
                                   "%s\n", p);
                    RING("%s rc=%d", p, rc);
                }
            } while (c != NULL);
        }
    }
    TE_LOG_RING("Globals", "%s", glob);
    free(glob);
}

/* Seems to be enough :-) */
#define TESTER_REQS_LEN 1024 * 1024

/* Log list of requirements known to tester */
static void
tester_log_reqs(void)
{
    char *reqs_string = calloc(1, TESTER_REQS_LEN);
    int rc = 0;
    test_requirement *p;

    rc += sprintf(reqs_string, "Requirements known to tester:\n");
    TAILQ_FOREACH(p, &tester_global_context.reqs, links)
    {
        rc += sprintf(reqs_string + rc, "  %s %s\n",
                      p->id,
                      (p->ref != NULL) ? p->ref : "");
    }
    TE_LOG_RING("Known reqs", "%s", reqs_string);
    free(reqs_string);
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
    int             result = EXIT_FAILURE;
    te_errno        rc;

#if HAVE_SIGNAL_H
    (void)signal(SIGINT, tester_sigint_handler);
#endif

    if (tester_global_init(&tester_global_context) != 0)
    {
        ERROR("Initialization of Tester context failed");
        goto exit;
    }

    if (tester_init_types() != 0)
    {
        ERROR("Initialization of Tester types support failed");
        goto exit;
    }

    if (process_cmd_line_opts(&tester_global_context, argc, argv) != 0)
    {
        ERROR("Command line options processing failure");
        goto exit;
    }

    if (tester_global_context.targets != NULL)
    {
        TE_LOG_RING("Target Requirements", "%s",
                tester_reqs_expr_to_string(tester_global_context.targets));
    }

    /*
     * Initialize pseudo-random number generator after command-line
     * options processing, since random seed may be passed as
     * command-line option.
     */
    srand(tester_global_context.rand_seed);
    RING("Random seed is %u", tester_global_context.rand_seed);

    /*
     * Build Test Suites specified in command line.
     */
    if ((~tester_global_context.flags & TESTER_NO_BUILD) &&
        !TAILQ_EMPTY(&tester_global_context.suites))
    {
        RING("Building Test Suites specified in command line...");
        rc = tester_build_suites(&tester_global_context.suites,
                !!(tester_global_context.flags & TESTER_VERBOSE));
        if (rc != 0)
        {
            goto exit;
        }
    }

    /*
     * Parse configuration files, build and parse test suites data.
     */
    rc = tester_parse_configs(&tester_global_context.cfgs,
                !(tester_global_context.flags & TESTER_NO_BUILD),
                !!(tester_global_context.flags & TESTER_VERBOSE));
    if (rc != 0)
    {
        goto exit;
    }

    /*
     * Prepare configurations to be processed by testing scenario
     * generator.
     */
    rc = tester_prepare_configs(&tester_global_context.cfgs);
    if (rc != 0)
    {
        goto exit;
    }
    INFO("Total number of iteration is %u",
         tester_global_context.cfgs.total_iters);

    /*
     * Create testing scenario.
     */
    rc = tester_process_test_paths(&tester_global_context.cfgs,
              &tester_global_context.paths,
              &tester_global_context.scenario,
              !(tester_global_context.flags & TESTER_INTERACTIVE));
    if (rc != 0)
    {
        goto exit;
    }

    /*
     * Execure testing scenario.
     */
    if ((~tester_global_context.flags & TESTER_NO_RUN) &&
        !TAILQ_EMPTY(&tester_global_context.cfgs.head))
    {
        RING("Starting...");
        /* Log global variables so TRC can get them and */
        (void)tester_log_global();
        if (!!(tester_global_context.flags & TESTER_LOG_REQS_LIST))
            (void)tester_log_reqs();
        rc = tester_run(&tester_global_context.scenario,
                        tester_global_context.targets,
                        &tester_global_context.cfgs,
                        &tester_global_context.paths,
                        tester_global_context.trc_db,
                        &tester_global_context.trc_tags,
                        tester_global_context.flags);
        if (rc != 0)
        {
#if 1
            /*
             * Temporary override exit status before clean up to follow
             * rules defined in dispatcher.sh.
             */
            result = EXIT_SUCCESS;
#endif
            goto exit;
        }
    }

    result = EXIT_SUCCESS;
    RING("Done");

exit:
    tester_global_free(&tester_global_context);
    tester_term_cleanup();

    (void)logic_expr_int_lex_destroy();
    (void)test_path_lex_destroy();

    return result;
}
