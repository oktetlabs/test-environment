/** @file 
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Connections API
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
#ifndef __TE_COMM_NET_AGENT_TESTS_LIB_WORKSHOP_H__
#define __TE_COMM_NET_AGENT_TESTS_LIB_WORKSHOP_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"

#include <string.h>

/**
 * This macro checks the input and output buffers. If an inconsistency is
 * found, the macro bails out of the test. 
 *
 * @return n/a
 */
#define VERIFY_BUFFERS()                                              \
    do {                                                            \
        if (compare_buffers(input_buffer, declared_input_buffer_length,     \
                         output_buffer, declared_output_buffer_length)   \
           != 0)                                                     \
       {                                                            \
           fprintf(stderr, "ERROR: input (%d bytes) and output (%d bytes)" \
                  " buffers are not equal\n",                          \
                  declared_input_buffer_length,                         \
                  declared_output_buffer_length);                         \
           DEBUG("Here follows the input buffer:\n");                         \
           DEBUG("%s\n", output_buffer);                                \
           DEBUG("End of output buffer\n");                                \
           exit(3);                                                     \
       }                                                            \
    } while (0)

/**
 * This macro reports the success of the test. 
 * It should only be called from the main().
 *
 * @return n/a
 */
#define PRINT_TEST_OK()                                          \
    do {                                                 \
        char __x_buf[BUFSIZ], *__x_p;                            \
                                                            \
       __x_p = strrchr(argv[0], '/');                            \
       if (__x_p == NULL)                                   \
           __x_p = argv[0];                                   \
       else                                                 \
           __x_p++;                                          \
       strncpy(__x_buf, __x_p, sizeof(__x_buf) - 1);              \
       __x_buf[sizeof(__x_buf) - 1] = '\0';                     \
       fprintf(stderr, "%s: TEST PASSED OK\n", __x_buf);       \
    } while (0)

/**
 * This macro must be called when a message with an attachment
 * was received via rcf_comm_agent_wait() function before trying
 * to compare the input and output buffers. 
 *
 * This macro places a space character before the 'attach <len>'
 * sequence which was overwritten with a '\0' by the Network
 * Communication Library.
 *
 * @param  buf   Input buffer
 * @param  size  Size of the received message
 *
 * @return n/a
 */
#define ZERO_ADJUST_INPUT_BUFFER(buf, size)                     \
    do {                                                 \
        int l = strlen(buf);                                   \
                                                        \
       if (size > l + 1)  /* there is an attachment */              \
       {                                                 \
           buf[l] = ' ';   /* reset the '\0' with space */       \
       }                                                 \
    } while (0);

#ifdef __cplusplus
}
#endif
#endif /* __TE_COMM_NET_AGENT_TESTS_LIB_WORKSHOP_H__ */
