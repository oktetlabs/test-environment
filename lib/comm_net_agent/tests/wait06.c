/** @file
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
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
 *
 * @return Test result
 * @retval 0            Test succeeded
 * @retval positive     Test failed
 *
 */
int
test_rcf_comm_agent_wait06(void);
