/** @file
 * @brief Test API to deal with buffers
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg Kravtosv <Oleg.Kravtosv@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_BUFS_H__
#define __TE_TAPI_BUFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declaration of tapi_log_buf type, which is defined 
 * in the implementation, so user can allocate and operate only 
 * with pointer to this data structure.
 */
struct tapi_log_buf;
typedef struct tapi_log_buf *tapi_log_buf;


/* Log buffer related functions */

/**
 * Allocates a buffer to be used for accumulating log message.
 * Mainly used in tapi_snmp itself.
 * 
 * @return Pointer to the buffer.
 *
 * @note the caller does not have to check the returned 
 * value against NULL, the function blocks the caller until it
 * gets available buffer.
 *
 * @note This is thread safe function
 */
extern tapi_log_buf *tapi_log_buf_alloc();

/**
 * Appends format string to the log message, the behaviour of
 * the function is the same as ordinary printf-like function.
 *
 * @param buf  Pointer to the buffer allocated with tapi_log_buf_alloc()
 * @param fmt  Format string followed by parameters
 *
 * @return The number of characters appended
 *
 * @note This is NOT thread safe function, so you are not allowed 
 * to append the same buffer from different threads simultaneously.
 */
extern int tapi_log_buf_append(tapi_log_buf *buf, const char *fmt, ...);

/**
 * Returns current log message accumulated in the buffer.
 *
 * @param buf  Pointer to the buffer
 *
 * @return log message
 */
extern const char *tapi_log_buf_get(tapi_log_buf *buf);

/**
 * Release buffer allocated by tapi_log_buf_alloc()
 *
 * @param ptr  Pointer to the buffer
 *
 * @note This is thread safe function
 */
extern void tapi_log_buf_free(tapi_log_buf *buf);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_BUFS_H__ */
