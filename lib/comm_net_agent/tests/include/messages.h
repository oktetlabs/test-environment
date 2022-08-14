/** @file
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Operations with Messages API
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_COMM_NET_AGENT_TESTS_LIB_MESSAGES_H__
#define __TE_COMM_NET_AGENT_TESTS_LIB_MESSAGES_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compares binary contents of two buffers.
 *
 * @param buffer1       first buffer
 * @param len1          length of the first buffer
 * @param curr_point    second buffer
 * @param len2          length of the second buffer
 *
 * @retval  0           buffers are equal
 * @retval  1           buffers are not equal
 */
extern int compare_buffers(char *buffer1, int len1, char *buffer2, int len2);

/**
 * Generates a command/answer of size 'cmd_size' plus 'attachment_size'.
 * Note that if attachment_size is not zero, cmd_size cannot be less
 * than 20.
 *
 * @param buffer          Buffer to be filled with the command
 * @param cmd_size        Size of the command part of the message including
 *                        the trailing separator
 * @param attachment_size Size of the attachment to be generated
 *
 * @return              n/a
 *
 */
extern void generate_command(char *buffer, int cmd_size, int attachment_size);

#ifdef __cplusplus
}
#endif
#endif /* __TE_COMM_NET_AGENT_TESTS_LIB_MESSAGES_H__ */
