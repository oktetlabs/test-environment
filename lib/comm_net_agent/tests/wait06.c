/** @file 
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side
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
 * Author: Pavel A. Bolokhov <Pavel.Bolokhov@oktetlabs.ru>
 * 
 * @(#) $Id$
 */

/** @page test_rcf_net_agent_wait06 rcf_comm_agent_wait() random attachments
 *
 * @descr Generate and send a message consisting of several commands with 
 * randomly assigned attachments. Call the function specifying a buffer 
 * large enough to room the message enough times (equal to the number of 
 * commands) to ensure that the whole message came consistently.
 *
 * THIS TEST IS DEPRECATED
 *
 * @post Once successful, the test is repeated sending several messages 
 * with different combinations of sizes of the commands and their attachments
 * prior to doing the main check.
 *
 * @author Pavel A. Bolokhov <Pavel.Bolokhov@oktetlabs.ru>
 *
 * @return Test result
 * @retval 0            Test succeeded
 * @retval positive     Test failed
 *
 */
int 
test_rcf_comm_agent_wait06(void);
