#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>


#include "test_path.h"
#include "tester_run.h"
#if HAVE_POPT_H
#include <popt.h>
#endif
#include "tester_interactive.h"

void
tester_interactive_print_usage()
{
    printf("Interactive-mode usage:\n"
           "\trun paths... - to run specified test paths\n"
           "\tresume       - to resume testing\n"
           "\tstop         - to finalize testing\n"
           "\t!command     - to execute shell command\n"
           "\t?            - to view this message\n");
}


int
tester_interactive_open_prompt(
        tester_cfgs *cfgs,
        int total_iters,
        test_paths *paths,
        testing_scenario *scenario)
{
    
    char    *user_choice;
    int     rc;
    int     argc;
    char    **argv;
    int     i;

#define INVALID_SYNTAX_ERROR "Error parsing input: invalid syntax.\
        Type '?' to read usage\n"
    while (1)
    {
        user_choice = readline("Entering tester interactive mode.\n"
            "Please enter your choice.\n"
            ">");
        
        if(poptParseArgvString(user_choice, &argc, &argv) < 0)
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
            system(user_choice + 1);
            continue;
        }
        else if(strcmp(argv[0], "run") == 0)
        {
            test_paths_free(paths);
            scenario_free(scenario);
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
                printf("Test path %s was processed.\n", argv[i]);
            }
            rc = tester_process_test_paths(cfgs, total_iters, paths,
                    scenario);
            if (rc != 0)
            {
                return TESTER_INTERACTIVE_ERROR;
            }

            free(argv);
            printf("New test paths were processed\n"
                   "Resuming testing...\n");
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
}
