/** @file
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Connections API
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */
#ifndef __TE_COMM_NET_AGENT_TESTS_LIB_DEBUG_H__
#define __TE_COMM_NET_AGENT_TESTS_LIB_DEBUG_H__
#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG(x...)                             \
    do {                                   \
        fprintf(stderr, x);                     \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif /* __TE_COMM_NET_AGENT_TESTS_LIB_DEBUG_H__ */
