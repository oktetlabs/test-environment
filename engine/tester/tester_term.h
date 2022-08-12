/** @file
 * @brief Tester Subsystem
 *
 * Declaration of routines to interact with terminal.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TESTER_TERM_H__
#define __TE_TESTER_TERM_H__

#include "te_errno.h"
#include "tester_result.h"

#include "tester_defs.h"
#include "tester_flags.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log test start to terminal.
 *
 * @param flags     Tester context flags
 * @param type      Type of run item
 * @param name      Name of the test
 * @param tin       Test identification number
 * @param parent    Parent ID
 * @param self      Self ID of the test
 */
extern void tester_term_out_start(tester_flags flags,
                                  run_item_type type, const char *name,
                                  unsigned int tin,
                                  test_id parent, test_id self);

/**
 * Log test done to terminal.
 *
 * @param flags     Tester context flags
 * @param type      Type of run item
 * @param name      Name of the test
 * @param tin       Test identification number
 * @param parent    Parent ID
 * @param self      Self ID of the test
 * @param status    Test status
 * @param trcv      TRC result
 */
extern void tester_term_out_done(tester_flags flags,
                                 run_item_type type, const char *name,
                                 unsigned int tin,
                                 test_id parent, test_id self,
                                 tester_test_status status,
                                 trc_verdict trcv);

/**
 * Cleanup curses structures to make valgrind happy. Should not be called
 * before any other terminal-handling functions.
 *
 * @return 0 on success
 */
extern int tester_term_cleanup(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_TERM_H__ */
