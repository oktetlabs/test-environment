/** @file
 * @brief Test Coverage Estimation
 *
 * TCE data collector library external definitions.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_LIB_TCE_COLLECTOR_H
#define __TE_LIB_TCE_COLLECTOR_H

#ifndef IN_TCE_COLLECTOR
/** The PID of a TCE collector subprocess */
extern pid_t tce_collector_pid;

/** Should be set to TRUE when TCE is run not under a Test Agent */
extern te_bool tce_standalone;
#endif

/** Specifies TCE collector parameters:
 *   - argv[0]: TAR file prefix
 *   - argv[1]..argv[argc-1]: connection designators:
 *              + fifo:filename
 *              + unix:socket-filename
 *              + abstract:abstract-socketname
 *              + tcp:port[:host]
 *              + kallsyms:kernel-sym-table 
 *              (pseudo-designator causing coverage data
 *               to be obtained from the kernel)
 * @returns status code
 */
extern int tce_init_collector(int argc, char **argv);


/** The main TCE collector loop. 
 *  It will return only after SIGTERM.
 *  The user should not normally call this function,
 *  call tce_run_collector instead.
 *
 *  @returns status code
 */
extern int tce_collector(void);

/** Runs TCE collector as a sub-process. 
 * 
 * @returns status code
 * @sa tce_init_tce_collector
 * @sa tce_collector
 */
extern int tce_run_collector(int argc, char *argv[]);

/** Causes a TCE collector subprocess to dump the data.
 * 
 * @sa tce_collector_pid
 */
extern int tce_dump_collector(void);

/** Terminates a TCE collector subprocess.
 * 
 * @sa tce_collector_pid
 */
extern int tce_stop_collector(void);

/** Notifies a TCE collector subprocess that a new peer
 *  has been started.
 *  This allows to detect case when some peers have not
 *  send their data to the collector.
 * 
 * @sa tce_collector_pid
 */
extern int tce_notify_collector(void);

/** @returns the first connection designator */
const char *tce_obtain_principal_connect(void);

/** @returns the default peer id for this collector process */
extern int tce_obtain_principal_peer_id(void);

#endif /* __TE_LIB_TCE_COLLECTOR_H */
