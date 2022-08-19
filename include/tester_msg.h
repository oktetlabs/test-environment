/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Sending messages to Tester
 *
 * Definitions for sending messages to Tester.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TESTER_MSG_H__
#define __TE_TESTER_MSG_H__

/** Name of the environment variable with Tester IPC server name */
#define TESTER_IPC_SERVER_ENV   "TE_TESTER"

/** Type of IPC used by Tester IPC */
#define TESTER_IPC              (TRUE)  /** Connection-oriented */


/** Initial test ID (assigned to root package) */
#define TE_TEST_ID_INIT 1

/**
 * Test ID of the root prologue if the prologue exists.
 *
 * Root package gets the initial ID. The next one is as typically root prologue.
 */
#define TE_TEST_ID_ROOT_PROLOGUE (TE_TEST_ID_INIT + 1)

/**
 * @def TE_CFG_TRC_TAGS_FMT
 * Macro for working with a branch of the configurator object tree for TRC tags.
 */
#define TE_CFG_TRC_TAGS_FMT "/local:/trc_tags:%s"

/**
 * Types of messages which tests send to Tester.
 */
typedef enum te_test_msg_type {
    TE_TEST_MSG_VERDICT,    /**< Test verdict */
    TE_TEST_MSG_ARTIFACT,   /**< Test artifact */
} te_test_msg_type;

/**
 * Representation of the message header passed from tests to Tester.
 * Message body is passed just after the header.
 */
typedef struct tester_test_msg_hdr {
        uint32_t    id;     /**< ID of the test */
        uint32_t    type;   /**< Message type (see tester_test_msg_type). */
} tester_test_msg_hdr;

#endif /* !__TE_TESTER_MSG_H__ */
