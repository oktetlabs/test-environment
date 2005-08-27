/** @file
 * @brief API to deal with buffers
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TOOLS_BUFS_H__
#define __TE_TOOLS_BUFS_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fill buffer by random numbers.
 *
 * @param buf      Buffer pointer
 * @param len      Buffer length
 */
extern void te_fill_buf(void *buf, size_t len);

/**
 * Allocate buffer of random size from @b min to @b max and preset
 * it by random numbers.
 *
 * @param min     minimum size of buffer
 * @param max     maximum size of buffer
 * @param p_len   location for real size of allocated buffer
 *
 * @return Pointer to allocated buffer.
 */
extern void *te_make_buf(size_t min, size_t max, size_t *p_len);


/** 
 * Create a buffer of specified size
 * 
 * @param len    Buffer length
 *
 * @return Pointer to allocated buffer.
 */
static inline void *
te_make_buf_by_len(size_t len)
{
    size_t ret_len;

    return te_make_buf(len, len, &ret_len);
}

/** 
 * Create a buffer not shorter that specified length.
 * 
 * @param min       Minimum buffer length
 * @param p_len     Buffer length (OUT)
 *
 * @return Pointer to allocated buffer.
 */
static inline void *
te_make_buf_min(size_t min, size_t *p_len)
{
    return te_make_buf(min, min + 10, p_len);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_BUFS_H__ */
