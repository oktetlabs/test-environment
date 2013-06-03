/** @file
 * @brief Serial console parser events handler thread of the Tester
 *
 * Thread interaction functions definition
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */
 
/**
 * Start the Tester thread to handle serial parsers events
 * 
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_start_serial_thread(void);

/**
 * Cleanup and stop the tester serial thread
 * 
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_stop_serial_thread(void);

/**
 * Set the process identifier of the current test 
 * 
 * @param i_pid     PID of the current test process
 * 
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_set_serial_pid(pid_t i_pid);

/**
 * Release the process identifier of the current test
 * 
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_release_serial_pid(void);

/**
 * Check the flag to stop test sequence 
 * 
 * @return 
 * @retval TRUE     Test sequence should be stopped
 * @retval FALSE    Continue work
 */
extern te_bool tester_check_serial_stop(void);
