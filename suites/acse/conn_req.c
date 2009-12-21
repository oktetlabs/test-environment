/** @file
 * @brief Test Environment
 *
 * Check ACSE functionality: ConnectionRequest to CPE
 * 
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS
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
 * $Id$
 */

/** @page CWMP test: Connection Request
 *
 * @objective Check that CPE initiate CWMP connection after receive
 *            ConnectionRequest from ACSE.
 *
 * @param cpe_ip                IP address of CPE
 * @param acse_port             TCP port to listen HTTP connection 
 *                              from CPE on ACSE
 * @param cpe_cr_url            URL for ConnectionRequest to CPE
 * @param cpe_cr_username       username for ConnectionRequest to CPE 
 * @param cpe_cr_password       password for ConnectionRequest to CPE 
 *
 * @par Scenario:
 *
 * -# Ensure ACSE is startedsuccesfully on TA.
 * -# Configure ACS object and CPE on ACSE, pass @p cpe_ip, @p acse_port,
 *    @p cpe_cr_url, @p cpe_cr_username, @p cpe_cr_password to ACSE.
 * -# Initiate ConnectionRequest to CPE from ACSE.
 * -# Wait ConnectionRequest results, check success, that is 
 *    established CWMP session.
 * -# Get from ACSE last received Inform from CPE, check it is 
 *    '6 CONNECTION REQUEST'.
 * -# Close CWMP session: wait for empty HTTP Post from CPE,
 *    response to it with empty HTTP OK. 
 * 
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 */
