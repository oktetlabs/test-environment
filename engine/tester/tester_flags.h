/** @file
 * @brief Tester Subsystem
 *
 * Tester flags definitions.
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

#ifndef __TE_TESTER_FLAGS_H__
#define __TE_TESTER_FLAGS_H__


/** Flags of the Tester global context */
enum tester_flags {
    TESTER_NOLOGUES     = 0x0001,   /**< Prologues/epilogues are disabled */
    TESTER_NORANDOM     = 0x0002,   /**< Disable randomness */
    TESTER_NOSIMULT     = 0x0004,   /**< Disable simultaneousness */
    TESTER_FAKE         = 0x0008,   /**< Fake run */

    TESTER_VERBOSE      = 0x0010,   /**< The first level of verbosity:
                                         output of run path to stdout */
    TESTER_VVERB        = 0x0020,   /**< The second level of verbosity:
                                         additional output of test IDs
                                         on run paths */
    TESTER_VVVERB       = 0x0040,   /**< The third level of verbosity */
    TESTER_QUIET_SKIP   = 0x0080,   /**< Quiet skip of tests */

    TESTER_VALGRIND     = 0x0100,   /**< Run scripts under valgrind */
    TESTER_GDB          = 0x0200,   /**< Run scripts under GDB in
                                         interactive mode */
    TESTER_RUNPATH      = 0x0400,   /**< Flag to be used on run path only */
    TESTER_RUN          = 0x0800,   /**< End of run path */

    TESTER_NOBUILD      = 0x1000,   /**< Don't build any Test Suites */
    TESTER_NO_CS        = 0x2000,   /**< Don't interact with Configurator */
    TESTER_NOCFGTRACK   = 0x4000,   /**< Don't track configuration 
                                         changes */
    TESTER_INLOGUE      = 0x8000,   /**< Is in prologue/epilogue */
    TESTER_FORCE_RUN    = 0x10000,  /**< Run tests regardless requested 
                                         path */
    TESTER_SHUTDOWN     = 0x20000,  /**< Shutdown test scenario */
};

#endif /* !__TE_TESTER_FLAGS_H__ */
