/*
 * Proteos
 * Tester Interfaces
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * Author: Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * @(#) $Id: OKT-DD-0000089-TE_TS_IF.h,v 1.1 2003/10/28 09:35:41 arybchik Exp $
 */

/*
 * Tests are executed using system() call. Return code of the call is
 * analysed using WIFEXITED(), WEXITSTATUS(), WCOREDUMP() (if WCOREDUMP
 * is supported) macros. Other macros may be used for more sophisticated
 * analysis.
 *
 * Return code of the test is truncated to one signed byte. Non negative
 * value is considered as normal test executing, negative values are
 * reserved for system errors, macro SYS_ERROR must be used to map
 * system error to return code.
 * 
 * If test is passed, zero value (EXIT_SUCCESS) must be returned.
 * Positive values are used to indicate test failure. EXIT_FAILURE
 * should be used when recovery is not necessary and testing may be
 * continued. Other positive values are considered as exceptions and
 * passed to exception handler.
 *
 * If exception handler fails, execution of the package is terminated
 * and exception returned by exception handler is passed to upper level
 * exception handler. Tests, which were skipped until successfull
 * processing of the exception, are considered as not executed and do
 * not appear in logs.
 * 
 * Note that exception handlers, keep-alive validations, prologues and
 * epilogues use the same semantic of the return value. Moreover, the
 * same rules are applied for their execution and processing of results.
 */

#define SYS_ERROR(_errno)

/*
 * POSIX threads, mutexes, conditions and semaphores should be used in
 * tests to orginaze multiflow execution with necessary data 
 * protection and synchronization (pthread.h, semaphore.h).
 */
