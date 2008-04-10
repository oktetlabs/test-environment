/** @file
 * @brief Tester Subsystem
 *
 * Interactive mode.
 *
 *
 * Copyright (C) 2008 Test Environment authors (see file AUTHORS
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
 * @author Sergey Levi <sergeyle@tercom.ru>
 *
 * $Id$
 */

#ifndef __TE_TESTER_INTERACTIVE_H__
#define __TE_TESTER_INTERACTIVE_H__


#include "tester_conf.h"
#include "test_path.h"
#include "tester_run.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interactive Tester UI return codes
 */
enum interactive_mode_opts {
    TESTER_INTERACTIVE_RUN     = (1 << 0), /**< Process and run
                                                a new test path */
    TESTER_INTERACTIVE_RESUME  = (1 << 1), /**< Continue the testing (as if
                                                it wasn't interrupted) */
    TESTER_INTERACTIVE_STOP    = (1 << 2), /**< Stop the testing */
    TESTER_INTERACTIVE_ERROR   = (1 << 3)  /**< An error occured */
};


/**
 * Open the interactive tester user prompt and return the user choice.
 * 
 * @param cfgs          Configurations
 * @param paths         Location for test paths
 * @param scenario      Location for testing scenario
 *
 * @return Interactive Tester UI return code.
 */
extern enum interactive_mode_opts
    tester_interactive_open_prompt(const tester_cfgs *cfgs,
                                   test_paths        *paths,
                                   testing_scenario  *scenario);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_INTERACTIVE_H__ */
