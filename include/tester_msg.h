/** @file
 * @brief Sending messages to Tester
 *
 * Definitions for sending messages to Tester.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TESTER_MSG_H__
#define __TE_TESTER_MSG_H__

/** Name of the environment variable with Tester IPC server name */
#define TESTER_IPC_SERVER_ENV   "TE_TESTER"

/** Type of IPC used by Tester IPC */
#define TESTER_IPC              (TRUE)  /** Connection-oriented */


/**
 * Representation of the message header passed from tests to Tester.
 * Message body is passed just after the header.
 */
typedef struct tester_test_msg_hdr {
        uint32_t    id;     /**< ID of the test */
} tester_test_msg_hdr;

#endif /* !__TE_TESTER_MSG_H__ */
