/** @file
 * @brief Tester Subsystem
 *
 * Interactive mode.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Sergey Levi <sergeyle@tercom.ru>
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include <stdio.h>
#if HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "test_path.h"
#include "tester_run.h"
#if HAVE_POPT_H
#include <popt.h>
#endif
#include "tester_interactive.h"


#if HAVE_READLINE    
static void
tester_interactive_print_usage()
{
    printf("Interactive-mode usage:\n"
           "\trun paths... - to run specified test paths\n"
           "\tresume       - to resume testing\n"
           "\tstop         - to finalize testing\n"
           "\t!command     - to execute shell command\n"
           "\t?            - to view this message\n");
}
#endif


/* See the description in tester_interactive.h */
enum interactive_mode_opts
tester_interactive_open_prompt(const tester_cfgs *cfgs,
                               test_paths        *paths,
                               testing_scenario  *scenario)
{
#if HAVE_READLINE    
    char           *user_choice;
    int             rc;
    int             argc;
    const char    **argv;
    int             i;

    printf("Entering Tester interactive mode. "
           "Please enter your choice.\n");

#define INVALID_SYNTAX_ERROR \
    "Error parsing input: invalid syntax. Type '?' to read usage.\n"

    while (1)
    {
        user_choice = readline("> ");
        
        if (poptParseArgvString(user_choice, &argc, &argv) < 0)
        {
            tester_interactive_print_usage();
            continue;
        }
        else if (strcmp(user_choice, "?") == 0)
        {
            tester_interactive_print_usage();
            continue;
        }
        else if(user_choice[0] == '!')
        {
            rc = system(user_choice + 1);
            if (rc != 0)
                INFO("Execution failed");
            continue;
        }
        else if(strcmp(argv[0], "run") == 0)
        {
            TAILQ_INIT(paths);
            TAILQ_INIT(scenario);

            if (argc < 2)
            {
                printf(INVALID_SYNTAX_ERROR);
                continue;
            }
            for (i = 1; i < argc; i++)
            {
                rc = test_path_new(paths, argv[i], TEST_PATH_RUN);
                if (rc != 0)
                {
                    ERROR("Failed to parse test path: '%s'", argv[i]);
                    return TESTER_INTERACTIVE_ERROR;
                }
                VERB("Test path '%s' was processed.\n", argv[i]);
            }
            rc = tester_process_test_paths(cfgs, paths, scenario, FALSE);
            if (rc != 0)
            {
                return TESTER_INTERACTIVE_ERROR;
            }

            free(argv);
            printf("New test paths were processed. Resuming testing...\n");
            return TESTER_INTERACTIVE_RUN;

        }
        else if (strcmp(argv[0], "resume") == 0)
        {
            printf("Resuming testing..\n");
            return TESTER_INTERACTIVE_RESUME;
        }
        else if (strcmp(argv[0], "stop") == 0)
        {
            printf("Finaliazing testing...\n");
            return TESTER_INTERACTIVE_STOP;
        }
        else
        {
            printf(INVALID_SYNTAX_ERROR);
            continue;
        }
    }
    
#undef INVALID_SYNTAX_ERROR

    return TESTER_INTERACTIVE_ERROR;

#else
    
    UNUSED(cfgs);
    UNUSED(paths);
    UNUSED(scenario);
    
    printf("Can't run in interactive mode: "
           "readline library is not availiable.\n");

    return TESTER_INTERACTIVE_STOP;

#endif
}
