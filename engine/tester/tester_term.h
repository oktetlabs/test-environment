/** @file
 * @brief Tester Subsystem
 *
 * Declaration of routines to interact with terminal.
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

#ifndef __TE_TESTER_TERM_H__
#define __TE_TESTER_TERM_H__

#include "te_errno.h"
#include "te_test_result.h"

#include "tester_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log test start to terminal.
 *
 * @param flags     Tester context flags
 * @param type      Type of run item
 * @param name      Name of the test
 * @param parent    Parent ID
 * @param self      Self ID of the test
 */
extern void tester_term_out_start(unsigned int flags,
                                  run_item_type type, const char *name,
                                  test_id parent, test_id self);

/**
 * Log test done to terminal.
 *
 * @param flags     Tester context flags
 * @param type      Type of run item
 * @param name      Name of the test
 * @param parent    Parent ID
 * @param self      Self ID of the test
 * @param status    Test status 
 * @param trcv      TRC result
 */
extern void tester_term_out_done(unsigned int flags,
                                 run_item_type type, const char *name,
                                 test_id parent, test_id self,
                                 te_test_status status, trc_verdict trcv);

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
