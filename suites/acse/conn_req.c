/** @file
 * @brief Test Environment
 *
 * Check ACSE functionality: ConnectionRequest to CPE
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
 */
