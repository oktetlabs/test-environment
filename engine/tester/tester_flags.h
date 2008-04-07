/** @file
 * @brief Tester Subsystem
 *
 * Tester flags definitions.
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

#ifndef __TE_TESTER_FLAGS_H__
#define __TE_TESTER_FLAGS_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Flags of the Tester global context */
enum tester_flags {
    TESTER_VERBOSE      = (1 << 0),   /**< The first level of verbosity:
                                           output of run path to stdout */
    TESTER_VVERB        = (1 << 1),   /**< The second level of verbosity:
                                           additional output of status
                                           together with TRC verdict */
    TESTER_VVVERB       = (1 << 2),   /**< The third level of verbosity:
                                           additional output of test IDs
                                           on run paths */
    TESTER_VVVVERB      = (1 << 3),   /**< The third level of verbosity */

    TESTER_FAKE         = (1 << 4),   /**< Fake run */

    TESTER_QUIET_SKIP   = (1 << 5),   /**< Quiet skip of tests */

    TESTER_NO_BUILD     = (1 << 8),   /**< Don't build any Test Suites */
    TESTER_NO_RUN       = (1 << 9),   /**< Don't run any Test Suites */
    TESTER_NO_CS        = (1 << 10),  /**< Don't interact with
                                           Configurator */
    TESTER_NO_CFG_TRACK = (1 << 11),  /**< Don't track configuration 
                                           changes */
    TESTER_NO_LOGUES    = (1 << 12),  /**< Prologues/epilogues are
                                           disabled */
    TESTER_NO_SIMULT    = (1 << 13),  /**< Disable simultaneousness */
    TESTER_NO_TRC       = (1 << 14),  /**< Don't use TRC */

    TESTER_VALGRIND     = (1 << 16),  /**< Run scripts under valgrind */
    TESTER_GDB          = (1 << 17),  /**< Run scripts under GDB in
                                           interactive mode */
    TESTER_RUNPATH      = (1 << 18),  /**< Flag to be used on run path
                                           only */
    TESTER_RUN          = (1 << 19),  /**< End of run path */

    TESTER_FORCE_RUN    = (1 << 20),  /**< Run tests regardless requested 
                                           path */

    TESTER_MIX_VALUES   = (1 << 21),
    TESTER_MIX_ARGS     = (1 << 22),
    TESTER_MIX_TESTS    = (1 << 23),
    TESTER_MIX_ITERS    = (1 << 24),
    TESTER_MIX_SESSIONS = (1 << 25),

    TESTER_INLOGUE      = (1 << 28),  /**< Is in prologue/epilogue */

    TESTER_INTERACTIVE  = (1 << 29),  /**< Interactive mode */
    TESTER_SHUTDOWN     = (1 << 30),  /**< Shutdown test scenario */
};

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_FLAGS_H__ */
