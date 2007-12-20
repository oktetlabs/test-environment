
#ifndef __TE_TESTER_INTERACTIVE_H__
#define __TE_TESTER_INTERACTIVE_H__


#include "tester_conf.h"
#include "test_path.h"
#include "tester_run.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interactive tester UI return codes
 */
    
enum interactive_mode_opts {
    TESTER_INTERACTIVE_RUN     = (1 << 0), /**< Process and run a new test path */
    TESTER_INTERACTIVE_RESUME  = (1 << 1), /**< Continue the testing (as if it wasn't interrupted) */
    TESTER_INTERACTIVE_STOP    = (1 << 2), /**< Stop the testing */
    TESTER_INTERACTIVE_ERROR   = (1 << 3)  /**< An error occured */
};

extern void tester_interactive_print_usage();

/**
 * Open the interactive tester user prompt and return the user choice
 * 
 * @param cfgs          Configurations
 * @param total_iters   Total number of iterations
 * @param paths         Location for test paths
 * @param scenario      Location for testing scenario
 */
extern int tester_interactive_open_prompt(tester_cfgs *cfgs,
                                          int total_iters,
                                          test_paths *paths,
                                          testing_scenario *scenario);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TESTER_INTERACTIVE_H__ */
