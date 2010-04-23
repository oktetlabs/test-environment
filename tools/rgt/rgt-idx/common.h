/** @file 
 * @brief Test Environment: RGT - log index utilities - common declarations
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
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
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RGT_IDX_COMMON_H__
#define __TE_RGT_IDX_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define OFF_T_MAX   (off_t)((uint64_t)-1 >> \
                            ((sizeof(uint64_t) - sizeof(off_t)) * 8 + 1))

#define ERROR(_fmt, _args...) fprintf(stderr, _fmt "\n", ##_args)

#define ERROR_CLEANUP(_fmt, _args...) \
    do {                                \
        ERROR(_fmt, ##_args);           \
        goto cleanup;                   \
    } while (0)

#define ERROR_USAGE_RETURN(_fmt, _args...) \
    do {                                                \
        ERROR(_fmt, ##_args);                           \
        usage(stderr, program_invocation_short_name);   \
        return 1;                                       \
    } while (0)


/** Message reading result code */
typedef enum read_message_rc {
    READ_MESSAGE_RC_ERR         = -2,   /**< A reading error occurred or
                                             unexpected EOF was reached */
    READ_MESSAGE_RC_WRONG_VER   = -1,   /**< A message of unsupported
                                             version was encountered */
    READ_MESSAGE_RC_EOF         = 0,    /**< The EOF was encountered instead
                                             of the message */
    READ_MESSAGE_RC_OK          = 1,    /**< The message was read
                                              successfully */
} read_message_rc;


/** Index entry */
typedef uint64_t entry[2];


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_RGT_IDX_COMMON_H__ */
