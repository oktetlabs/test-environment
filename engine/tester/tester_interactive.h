
#ifndef __TE_TESTER_INTERACTIVE_H__
#define __TE_TESTER_INTERACTIVE_H__


#include "tester_conf.h"
#include "test_path.h"
#include "tester_run.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum interactive_mode_opts {
    TESTER_INTERACTIVE_RUN              = (1 << 0),
    TESTER_INTERACTIVE_RESUME           = (1 << 1),
    TESTER_INTERACTIVE_STOP             = (1 << 2),
    TESTER_INTERACTIVE_ERROR            = (1 << 3)
};

extern inline void tester_interactive_print_usage();

extern int tester_interactive_open_prompt(tester_cfgs *cfgs,
                                          int total_iters,
                                          test_paths *paths,
                                          testing_scenario *scenario);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TESTER_INTERACTIVE_H__ */
